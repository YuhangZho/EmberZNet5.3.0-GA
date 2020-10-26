// *******************************************************************
// * color-control.c
// *
// *
// * Copyright 2007 by Ember Corporation. All rights reserved.              *80*
// *******************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/attribute-storage.h"

#define COLOR_TEMPRATURE_MAX 370 // Robin add
#define COLOR_TEMPRATURE_MIN 153 // Robin add

typedef struct {
  boolean active;
  int8u commandId;
  boolean ctMoveDirection; // TRUE for up
  int16u ctMoveToLevel;
  int32u eventDuration; // All time fields in milliseconds
  int32u elapsedTime;
  int32u transitionTime; 
} ColorControlState;

int16u colorTempPhysicalMin = COLOR_TEMPRATURE_MIN;
int16u ColorTempPhysicalMax = COLOR_TEMPRATURE_MAX;

static ColorControlState stateTable[EMBER_AF_COLOR_CONTROL_CLUSTER_SERVER_ENDPOINT_COUNT];

static ColorControlState *getColorControlState(int8u endpoint);

#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
static void colorControlClearRemainingTime(int8u endpoint);
#endif // ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE

static void colorControlSetColorModeToTwo(void);
static void colorControlSetColorTemprature(int8u endpoint, int16u colorTemprature);
static EmberAfStatus colorControlReadCurrentColorTemprature(int8u endpoint, int16u* colorTemprature);

#ifdef THE_GATEWAY_TYPE_SENGLED_WMS
void SetTheGatewayType(boolean type);
#endif

static ColorControlState *getColorControlState(int8u endpoint)
{
  int8u index = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_COLOR_CONTROL_CLUSTER_ID);
  return (index == 0xFF ? NULL : &stateTable[index]);
}

void emberAfColorControlClusterServerInitCallback(int8u endpoint)
{
  ColorControlState *state = getColorControlState(endpoint);
  if (state == NULL) {
    return;
  }

#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MIN_ATTRIBUTE
  emberAfReadServerAttribute(endpoint,
                            ZCL_COLOR_CONTROL_CLUSTER_ID,
                            ZCL_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MIN_ATTRIBUTE_ID,
                            (int8u *)&colorTempPhysicalMin,
                            sizeof(int16u));
#endif

#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MAX_ATTRIBUTE
  emberAfReadServerAttribute(endpoint,
                            ZCL_COLOR_CONTROL_CLUSTER_ID,
                            ZCL_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MAX_ATTRIBUTE_ID,
                            (int8u *)&ColorTempPhysicalMax,
                            sizeof(int16u));
#endif
  
  state->active = FALSE;
#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
  colorControlClearRemainingTime(endpoint);
#endif // ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
}

void emberAfColorControlClusterServerTickCallback(int8u endpoint)
{
  ColorControlState *state = getColorControlState(endpoint);
  EmberAfStatus status;
  int16u colorTemperature;
  boolean ctUp = state->ctMoveDirection;

  if (state == NULL) {
    return;
  }

  status = colorControlReadCurrentColorTemprature(endpoint, &colorTemperature);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("ERR: could not read current current temprature %x", 
                                      status);
    return;
  }

  switch(state->commandId) {
    case ZCL_MOVE_TO_COLOR_TEMPERATURE_COMMAND_ID:
    case ZCL_MOVE_COLOR_TEMPERATURE_COMMAND_ID:
    case ZCL_STEP_COLOR_TEMPERATUE_COMMAND_ID:
      colorTemperature = ctUp ? colorTemperature + 1 : colorTemperature - 1;
      if (ctUp == TRUE)
      {
        if (colorTemperature > ColorTempPhysicalMax)
        { colorTemperature = ColorTempPhysicalMax;}
      }      
      else 
      {
        if (colorTemperature < colorTempPhysicalMin)
        { colorTemperature = colorTempPhysicalMin;}
      }   

	  if(state->eventDuration == 0x0000)
	  {
	  	// if the transition time is 0, we do move directly, for qivicon requirements;
		colorTemperature = state->ctMoveToLevel;
	  }

      colorControlSetColorTemprature(endpoint, colorTemperature);
      state->active = (colorTemperature != state->ctMoveToLevel);
      break;
    default:
      emberAfColorControlClusterPrintln("ERR: unknown color control command.");
      return;
  }

#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
  {
    int16u remainingTime;

    state->elapsedTime = (state->active 
                          ? state->elapsedTime + state->eventDuration
                          : state->transitionTime); 

    // If we're done, we should clear the remaining time
    if (state->elapsedTime == state->transitionTime) {
      colorControlClearRemainingTime(endpoint);
      return;
    }

    // The remainingTime attribute expects values in tenths of seconds;
    // we maintain in milliseconds
    remainingTime = state->elapsedTime / 100;
    status = emberAfWriteServerAttribute(endpoint,
                                         ZCL_COLOR_CONTROL_CLUSTER_ID,
                                         ZCL_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE_ID,
                                         (int8u *)&remainingTime,
                                         ZCL_INT16U_ATTRIBUTE_TYPE);
    if (status != EMBER_ZCL_STATUS_SUCCESS) {                                                                             
      emberAfColorControlClusterPrintln("ERR: writing remaining time %x", status);                                        
      return;                                                                                                             
    }    
  }
#endif // ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE

  if (!state->active) {
    return;
  }

  //schedule the next tick
  emberAfScheduleServerTick(endpoint,
                            ZCL_COLOR_CONTROL_CLUSTER_ID,
                            state->eventDuration);
}

// Clear remaining time
#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
static void colorControlClearRemainingTime(int8u endpoint)
{
  //int16u data = 0xFFFF;
  int16u data = 0;
  EmberAfStatus status = emberAfWriteServerAttribute(endpoint,
                                                     ZCL_COLOR_CONTROL_CLUSTER_ID,
                                                     ZCL_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE_ID,
                                                     (int8u *)&data,
                                                     ZCL_INT16U_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("ERR: writing remaining time %x", status);
    return;
  }
}
#endif // ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
static void colorControlSetColorTemprature(int8u endpoint, int16u colorTemprature)
{
  EmberAfStatus status = emberAfWriteServerAttribute(endpoint,
                                                     ZCL_COLOR_CONTROL_CLUSTER_ID,
                                                     ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
                                                     (int8u *)&colorTemprature,
                                                     ZCL_INT16U_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("ERR: writing current color temprature %x", status); emberAfColorControlClusterFlush();
    return;
  }
  emberAfDebugPrintln("colorTemprature=%x", colorTemprature);
}

static void colorControlSetColorModeToTwo(void)
{
  EmberAfStatus status;
    
  int8u colorMode = 2;
  
#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_COLOR_MODE_ATTRIBUTE
  // Set the optional Color Mode attribute to two as per the spec.  If an
  // error occurs, log it, but ignore it.
  status = emberAfWriteServerAttribute(emberAfCurrentEndpoint(),
                                       ZCL_COLOR_CONTROL_CLUSTER_ID, 
                                       ZCL_COLOR_CONTROL_COLOR_MODE_ATTRIBUTE_ID, 
                                       &colorMode,
                                       ZCL_ENUM8_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("ERR: writing color mode%x", status);
  }
  emberAfDebugPrintln("colorMode=%x", colorMode); emberAfColorControlClusterFlush();
#else
  emberAfDebugPrintln("no color mode attribute"); emberAfColorControlClusterFlush();
#endif //ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_COLOR_MODE_ATTRIBUTE

#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_ENHANCED_COLOR_MODE_ATTRIBUTE
  // Set the optional Color Mode attribute to two as per the spec.  If an
  // error occurs, log it, but ignore it.
  status = emberAfWriteServerAttribute(emberAfCurrentEndpoint(),
                                       ZCL_COLOR_CONTROL_CLUSTER_ID, 
                                       ZCL_COLOR_CONTROL_ENHANCED_COLOR_MODE_ATTRIBUTE_ID, 
                                       &colorMode,
                                       ZCL_ENUM8_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("ERR: writing enhanced color mode%x", status);
  }
  emberAfDebugPrintln("enhancedColorMode=%x", colorMode); emberAfColorControlClusterFlush();
#else
  emberAfDebugPrintln("no enhanced color mode attribute"); emberAfColorControlClusterFlush();
#endif //ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_COLOR_MODE_ATTRIBUTE
}

// returns a ZCL status, EMBER_ZCL_STATUS_SUCCESS when it works. 
static EmberAfStatus colorControlReadCurrentColorTemprature(int8u endpoint, int16u* colorTemperature)
{
  EmberAfStatus status = emberAfReadServerAttribute(endpoint,
                                                    ZCL_COLOR_CONTROL_CLUSTER_ID,
                                                    ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
                                                    (int8u *)colorTemperature,
                                                    sizeof(int16u));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfColorControlClusterPrintln("ERR: reading current hue %x", status);
  }
  return status;
}


/****************** COMMAND HANDLERS **********************/

// Move hue to a given hue, taking transitionTime until completed.
boolean emberAfColorControlClusterMoveToHueCallback(int8u hue,
                                                    int8u direction,
                                                    int16u transitionTime)
{  
  return FALSE;
}

// Move hue continuously at the given rate. If mode is stop, then stop.
boolean emberAfColorControlClusterMoveHueCallback(int8u moveMode, int8u rate)
{  
  return FALSE;
}


// Step hue by one step, taking time as specified.
boolean emberAfColorControlClusterStepHueCallback(int8u stepMode,
                                                  int8u stepSize,
                                                  int8u transitionTime)
{  
  return FALSE;
}


// Move saturation to a given saturation, taking transitionTime until completed.
boolean emberAfColorControlClusterMoveToSaturationCallback(int8u saturation,
                                                           int16u transitionTime)
{  
  return FALSE;
}


// Move sat continuously at the given rate. If mode is stop, then stop.
boolean emberAfColorControlClusterMoveSaturationCallback(int8u moveMode, int8u rate)
{  
  return FALSE;
}


// Step sat by one step, taking time as specified.
boolean emberAfColorControlClusterStepSaturationCallback(int8u stepMode,
                                                         int8u stepSize,
                                                         int8u transitionTime)
{  
  return FALSE;
}


// Move hue and saturation to a given values, taking time as specified.
boolean emberAfColorControlClusterMoveToHueAndSaturationCallback(int8u hue,
                                                                 int8u saturation,
                                                                 int16u transitionTime)
{  
  return FALSE;
}


/** @brief Move To Color Temperature
 *
 * 
 *
 * @param colorTemperature   Ver.: always
 * @param transitionTime   Ver.: always
 */
boolean emberAfColorControlClusterMoveToColorTemperatureCallback(int16u colorTemperature,
                                                                 int16u transitionTime)
{
  ColorControlState *state = getColorControlState(emberAfCurrentEndpoint());
  EmberAfStatus status;
  int16u currentColorTemperature;

#ifdef THE_GATEWAY_TYPE_SENGLED_WMS 
  SetTheGatewayType(TRUE);
#endif

  emberAfColorControlClusterPrintln("ColorControl: MoveToColorTemprature (%x, %2x)",
                                    colorTemperature,
                                    transitionTime);

  if (state == NULL) {
    status = EMBER_ZCL_STATUS_FAILURE;
    goto send_default_response;
  }

  // If the color specified is not achievable by the hardware, then the
  // color shall not be set and a ZCL default response command shall be
  // generated with status code equal to INVALID_VALUE.
  status = colorControlReadCurrentColorTemprature(emberAfCurrentEndpoint(), 
                                      &currentColorTemperature);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    goto send_default_response;
  }
#if 0  
  if(!emberAfPluginColorControlIsColorSupportedCallback(currentHue, saturation)) {
    status = EMBER_ZCL_STATUS_INVALID_VALUE;
    goto send_default_response;
  }
#endif  

  // Nothing to do, prevent divide-by-zero
  if ( colorTemperature == currentColorTemperature ) {
    status = EMBER_ZCL_STATUS_SUCCESS;
    goto send_default_response;
  }

  state->commandId = ZCL_MOVE_TO_COLOR_TEMPERATURE_COMMAND_ID;
  state->elapsedTime = 0;
  if (colorTemperature > ColorTempPhysicalMax)
  { state->ctMoveToLevel = ColorTempPhysicalMax;}
  else if (colorTemperature < colorTempPhysicalMin)
  { state->ctMoveToLevel = colorTempPhysicalMin;}
  else
  { state->ctMoveToLevel = colorTemperature;}
  state->ctMoveDirection = 
    (colorTemperature > currentColorTemperature ? TRUE : FALSE);
//#ifndef SUPPURT_QIVICON
//  state->transitionTime = transitionTime * MILLISECOND_TICKS_PER_SECOND / 10; 
//  state->eventDuration = state->transitionTime / ((state->ctMoveDirection) ? 
//                                                  (colorTemperature - currentColorTemperature) : 
//                                                  (currentColorTemperature - colorTemperature));
//#else
	if(transitionTime == 0x0000)
	{
		state->transitionTime = transitionTime;
		state->eventDuration = transitionTime;
	}
	else
	{
		state->transitionTime = transitionTime * MILLISECOND_TICKS_PER_SECOND / 10; 
		if(state->transitionTime < ((state->ctMoveDirection) ? (colorTemperature - currentColorTemperature) : 
             (currentColorTemperature - colorTemperature)))
    	{
			
			state->eventDuration  = 1; // minimum time is 1ms;
		}
		else
		{
			state->eventDuration = state->transitionTime / ((state->ctMoveDirection) ? 
											(colorTemperature - currentColorTemperature) : 
											(currentColorTemperature - colorTemperature));
		}
	}

//#endif
  // Set the Color Mode attribute to two as per the spec, ignoring any errors.
  colorControlSetColorModeToTwo();

  //schedule the next tick
  if (emberAfScheduleServerTick(emberAfCurrentEndpoint(),
                                ZCL_COLOR_CONTROL_CLUSTER_ID,
                                state->eventDuration)
      != EMBER_SUCCESS)
    status = EMBER_ZCL_STATUS_SOFTWARE_FAILURE;
  else
    status = EMBER_ZCL_STATUS_SUCCESS;

  goto send_default_response;

send_default_response:
  emberAfSendImmediateDefaultResponse(status);

  return TRUE;
}

/** @brief Color Control Cluster Move Color Temperature
 *
 * 
 *
 * @param moveMode   Ver.: always
 * @param rate   Ver.: always
 * @param colorTemperatureMinimum   Ver.: always
 * @param colorTemperatureMaximum   Ver.: always
 */
boolean emberAfColorControlClusterMoveColorTemperatureCallback(int8u moveMode,
                                                               int16u rate,
                                                               int16u colorTemperatureMinimum,
                                                               int16u colorTemperatureMaximum)
{
  ColorControlState *state = getColorControlState(emberAfCurrentEndpoint());
  EmberAfStatus status;
  int16u currentColorTemperature;

  #ifdef THE_GATEWAY_TYPE_SENGLED_WMS 
  SetTheGatewayType(TRUE);
  #endif

  emberAfColorControlClusterPrintln("ColorControl: MoveColorTemprature (%x, %x, %x, %x)",
                                    moveMode,
                                    rate,
                                    colorTemperatureMinimum,
                                    colorTemperatureMaximum);

  if (state == NULL) {
    status = EMBER_ZCL_STATUS_FAILURE;
    goto send_default_response;
  }

  // If the rate is set to 0, then the command shall have no effect
  // and a ZCL default response command shall be generated with status 
  // code equal to INVALID_VALUE.

  if ((rate == 0) && (moveMode != EMBER_ZCL_HUE_MOVE_MODE_STOP))
  {
    status = EMBER_ZCL_STATUS_INVALID_FIELD;
    goto send_default_response;
  }

  state->active = TRUE;

  status = colorControlReadCurrentColorTemprature(emberAfCurrentEndpoint(), 
                                      &currentColorTemperature);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    goto send_default_response;
  }
  
  switch (moveMode) {
    case EMBER_ZCL_HUE_MOVE_MODE_STOP:
      state->active = FALSE;
      emberAfDeactivateServerTick(emberAfCurrentEndpoint(),
                                  ZCL_COLOR_CONTROL_CLUSTER_ID);
#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
      colorControlClearRemainingTime(emberAfCurrentEndpoint());
#endif // ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
      status = EMBER_ZCL_STATUS_SUCCESS;
      goto send_default_response;
    case EMBER_ZCL_HUE_MOVE_MODE_UP:
      state->ctMoveDirection = TRUE;
      state->ctMoveToLevel = (colorTemperatureMaximum<=ColorTempPhysicalMax)
                             ? colorTemperatureMaximum
                             : ColorTempPhysicalMax;
      //emberAfGuaranteedPrint("up = %d, %d\r\n", currentColorTemperature, state->ctMoveToLevel);
      if (currentColorTemperature > state->ctMoveToLevel)
      { 
        status = EMBER_ZCL_STATUS_INVALID_VALUE;
        goto send_default_response;
      }
      else if (currentColorTemperature == state->ctMoveToLevel)
      {
        status = EMBER_ZCL_STATUS_SUCCESS;
        goto send_default_response;
      }
      break;
    case EMBER_ZCL_HUE_MOVE_MODE_DOWN:
      state->ctMoveDirection = FALSE;
      state->ctMoveToLevel = (colorTemperatureMinimum>=colorTempPhysicalMin)
                             ? colorTemperatureMinimum
                             : colorTempPhysicalMin;
      //emberAfGuaranteedPrint("down = %d, %d\r\n", currentColorTemperature, state->ctMoveToLevel);
      if (currentColorTemperature < state->ctMoveToLevel)
      { 
        status = EMBER_ZCL_STATUS_INVALID_VALUE;
        goto send_default_response;
      }
      else if (currentColorTemperature == state->ctMoveToLevel)
      {
        status = EMBER_ZCL_STATUS_SUCCESS;
        goto send_default_response;
      }
      break;
    default:
      status = EMBER_ZCL_STATUS_INVALID_FIELD;
      goto send_default_response;
  }

  state->commandId = ZCL_MOVE_COLOR_TEMPERATURE_COMMAND_ID;
  if (state->active) {
    state->eventDuration = MILLISECOND_TICKS_PER_SECOND / rate;
  }

  // Set the Color Mode attribute to zero as per the spec, ignoring any errors.
  colorControlSetColorModeToTwo();

  //emberAfGuaranteedPrint("eventDuration = %d, %d\r\n", state->eventDuration, state->active);

  //schedule the next tick
  if (emberAfScheduleServerTick(emberAfCurrentEndpoint(),
                                ZCL_COLOR_CONTROL_CLUSTER_ID,
                                state->eventDuration)
      != EMBER_SUCCESS)
    status = EMBER_ZCL_STATUS_SOFTWARE_FAILURE;
  else
    status = EMBER_ZCL_STATUS_SUCCESS;

  
send_default_response:
  emberAfSendImmediateDefaultResponse(status);

  return TRUE;
  
}

/** @brief Color Control Cluster Step Color Temperatue
 *
 * 
 *
 * @param stepMode   Ver.: always
 * @param stepSize   Ver.: always
 * @param transitionTime   Ver.: always
 * @param colorTemperatureMinimum   Ver.: always
 * @param colorTemperatureMaximum   Ver.: always
 */
boolean emberAfColorControlClusterStepColorTemperatueCallback(int8u stepMode,
                                                              int16u stepSize,
                                                              int16u transitionTime,
                                                              int16u colorTemperatureMinimum,
                                                              int16u colorTemperatureMaximum)
{
  ColorControlState *state = getColorControlState(emberAfCurrentEndpoint());
  EmberAfStatus status;
  int16u currentColorTemperature, ctMaxMin;

  #ifdef THE_GATEWAY_TYPE_SENGLED_WMS 
  SetTheGatewayType(TRUE);
  #endif

  emberAfColorControlClusterPrintln("ColorControl: StepColorTemperatue (%x, %x, %x, %x, %x)",
                                    stepMode,
                                    stepSize,
                                    transitionTime,
                                    colorTemperatureMinimum,
                                    colorTemperatureMaximum);

  if (state == NULL) {
    status = EMBER_ZCL_STATUS_FAILURE;
    goto send_default_response;
  }

  status = colorControlReadCurrentColorTemprature(emberAfCurrentEndpoint(), 
                                                  &currentColorTemperature);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    goto send_default_response;
  }
  
  switch (stepMode) {
    case EMBER_ZCL_HUE_STEP_MODE_UP:
      state->ctMoveDirection = TRUE;
      ctMaxMin = (colorTemperatureMaximum<=ColorTempPhysicalMax)
                 ? colorTemperatureMaximum
                 : ColorTempPhysicalMax;
      if (currentColorTemperature > ctMaxMin)
      {
        status = EMBER_ZCL_STATUS_INVALID_VALUE;
        goto send_default_response;
      }
      else if (currentColorTemperature == ctMaxMin)
      {
        status = EMBER_ZCL_STATUS_SUCCESS;
        goto send_default_response;
      }
      else if (currentColorTemperature+stepSize > ctMaxMin)
      { state->ctMoveToLevel = ctMaxMin;}
      else
      { state->ctMoveToLevel = currentColorTemperature+stepSize;}
      break;
    case EMBER_ZCL_HUE_STEP_MODE_DOWN:
      state->ctMoveDirection = FALSE;
      ctMaxMin = (colorTemperatureMinimum>=colorTempPhysicalMin)
                 ? colorTemperatureMinimum
                 : colorTempPhysicalMin;
      if (currentColorTemperature < ctMaxMin)
      {
        status = EMBER_ZCL_STATUS_INVALID_VALUE;
        goto send_default_response;
      }
      else if (currentColorTemperature == ctMaxMin)
      {
        status = EMBER_ZCL_STATUS_SUCCESS;
        goto send_default_response;
      }
      else if (currentColorTemperature-stepSize < ctMaxMin)
      { state->ctMoveToLevel = ctMaxMin;}
      else
      { state->ctMoveToLevel = currentColorTemperature-stepSize;}
      break;
    default:
      status = EMBER_ZCL_STATUS_INVALID_FIELD;
      goto send_default_response;
  }

  state->commandId = ZCL_STEP_COLOR_TEMPERATUE_COMMAND_ID;
  state->transitionTime = transitionTime * MILLISECOND_TICKS_PER_SECOND / 10; 
  state->elapsedTime = 0;
  state->eventDuration = state->transitionTime / stepSize;

  state->active = TRUE;

  colorControlSetColorModeToTwo();

  //schedule the next tick
  if (emberAfScheduleServerTick(emberAfCurrentEndpoint(),
                                ZCL_COLOR_CONTROL_CLUSTER_ID,
                                state->eventDuration)
      != EMBER_SUCCESS)
    status = EMBER_ZCL_STATUS_SOFTWARE_FAILURE;
  else
    status = EMBER_ZCL_STATUS_SUCCESS;

  goto send_default_response;

send_default_response:
  emberAfSendImmediateDefaultResponse(status);

  return TRUE;
}
/** @brief Color Control Cluster Stop Move Step
 *
 * 
 *
 */
boolean emberAfColorControlClusterStopMoveStepCallback(void)
{
  ColorControlState *state = getColorControlState(emberAfCurrentEndpoint());
  EmberAfStatus status;
  
  state->active = FALSE;
  emberAfDeactivateServerTick(emberAfCurrentEndpoint(),
                              ZCL_COLOR_CONTROL_CLUSTER_ID);
#ifdef ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
  colorControlClearRemainingTime(emberAfCurrentEndpoint());
#endif // ZCL_USING_COLOR_CONTROL_CLUSTER_COLOR_CONTROL_REMAINING_TIME_ATTRIBUTE
  status = EMBER_ZCL_STATUS_SUCCESS;

  emberAfSendImmediateDefaultResponse(status);

  return TRUE;
}
