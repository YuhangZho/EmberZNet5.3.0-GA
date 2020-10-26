#include "app/framework/include/af.h"
#include "app/framework/util/attribute-storage.h"

#ifdef EMBER_AF_PLUGIN_SCENES
  #include "app/framework/plugin/scenes/scenes.h"
#endif //EMBER_AF_PLUGIN_SCENES

#ifdef EMBER_AF_PLUGIN_ON_OFF
  #include "app/framework/plugin/on-off/on-off.h"
#endif //EMBER_AF_PLUGIN_ON_OFF

#ifdef EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER
  #include "app/framework/plugin/zll-level-control-server/zll-level-control-server.h"
#endif //EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER

#include "app/framework/plugin/pwm-control/pwm-control.h"
#include "app/framework/plugin/reporting/reporting.h"

#include "app/framework/plugin/counters/counters.h"

#include "Z01_A19_HV2.h"


#include <math.h>

void SengledSetSequenceNumber(int8u num);


extern int16u colorTempPhysicalMin;
extern int16u ColorTempPhysicalMax;


void GetTheApsNumType(void)
{ 
  int8u num;

  halCommonGetToken(&num, TOKEN_THE_APSNUM_TYPE);
  SengledSetSequenceNumber(num+1);
}
void SetTheApsNumType(int8u num)
{ 
  halCommonSetToken(TOKEN_THE_APSNUM_TYPE, &num);
}

//****************************************************************
//¼æÈÝ¶àÆ½Ì¨
//****************************************************************
boolean theGatewayType = FALSE;

void SetTheGatewayType(boolean type)
{
  if (type != theGatewayType)
  { 
    theGatewayType = type;
    halCommonSetToken(TOKEN_THE_GATEWAY_TYPE, &theGatewayType);
  }
}

//****************************************************************
//Light Control
//****************************************************************
//Power
#define POWER_COUNT_MAX 2

int8u powerCount = POWER_COUNT_MAX;

// on-off
enum{
  SENGLED_OFF,
  SENGLED_ON,  
};
#define LAMP_ON_OFF_PIN PORTB_PIN(3)
#define SET_LAMP_OFF()  do { GPIO_PBSET = BIT(3); powerCount = POWER_COUNT_MAX;} while (0)
#define SET_LAMP_ON()   do { GPIO_PBCLR = BIT(3); powerCount = POWER_COUNT_MAX;} while (0)


// level
#define DIMMING_MAX_LEVEL  255
#define DIMMING_MIN_LEVEL  0
#define DIMMING_CTRL_PIN PIN_PA6
boolean disableLampOn;

//color
#define COLOR_TEMP_CTRL_PIN PIN_PB7

int16u saveCctLeve;
int16u saveCurrent;

int16u saveCctPwm;
int8u cctType;

EmberEventControl CctDimmingEventControl;

enum {
 CCT_YELLOW,
 CCT_WHITE  
};

void CctDimmingEventFunction(void) 
{
  if (cctType == CCT_WHITE)
  {
    if (saveCctPwm > 3)
    { 
      saveCctPwm -= 3;
      emberEventControlSetDelayMS(CctDimmingEventControl, 10);
    }
    else
    { 
      saveCctPwm = 0;
      emberEventControlSetInactive(CctDimmingEventControl);
    }
  }
  else if (cctType == CCT_YELLOW)
  {
    if (saveCctPwm < TICS_PER_PERIOD-3)
    { 
      saveCctPwm += 3;
      emberEventControlSetDelayMS(CctDimmingEventControl, 10);
    }
    else
    { 
      saveCctPwm = TICS_PER_PERIOD;
      emberEventControlSetInactive(CctDimmingEventControl);
    }
  }

  powerCount = POWER_COUNT_MAX;
  SetPwmLevel((int16u)saveCctPwm, COLOR_TEMP_CTRL_PIN);
}

// SetLedCct
//k = 0.0632*level+0.4735
//b = -41.341*level-5305.7
void SetLedCct(int16u cct)
{
  int32u cctPwm;

  emberEventControlSetInactive(CctDimmingEventControl);
  
  if (TRUE == theGatewayType)
  {
    if (cct >= ColorTempPhysicalMax)
    { 
      cctType = CCT_YELLOW;
      emberEventControlSetDelayMS(CctDimmingEventControl, 10);
      return;
    }
    else if (cct <= colorTempPhysicalMin)
    { 
      cctType = CCT_WHITE;
      emberEventControlSetDelayMS(CctDimmingEventControl, 10);
      return;
    } 
    else
    {
      int32s k = 632*saveCurrent/10+4735;
      int32s b = 53057000-413410*saveCurrent/10;
      k = (k*cct+b)/10000;
      if (k > TICS_PER_PERIOD)
      { cctPwm = TICS_PER_PERIOD;}
      else if (k < 0)
      { cctPwm = 0;}
      else
      { cctPwm = k;}
    }
  }
  else
  {
    if (cct >= DIMMING_MAX_LEVEL)
    { cctPwm =  1440;}
    else if (cct == DIMMING_MIN_LEVEL)
    { cctPwm = TICS_PER_PERIOD;}
    else
    {
      cctPwm = (int32s)cct;    
      cctPwm = (cct*2035)/100;
      if (cctPwm <= 6702)
      { cctPwm = 6702-cctPwm;}
      else
      { cctPwm = 0;} 
    }    
  }

  if (cctPwm > TICS_PER_PERIOD)
  { cctPwm = TICS_PER_PERIOD;}

  saveCctPwm = cctPwm;
  powerCount = POWER_COUNT_MAX;
  SetPwmLevel((int16u)cctPwm, COLOR_TEMP_CTRL_PIN);
  //emberAfGuaranteedPrint("%d %d ", midleLevel, cct);
}

void ColorTemperatureCompensation(int8u level, int16u cct)
{
  int32u cctPwm, k, b;

  if (cct >= ColorTempPhysicalMax)
  { cctPwm = TICS_PER_PERIOD;}
  else if (cct <= colorTempPhysicalMin)
  { return;}
  else
  {
    k = 41-632*cct/10000;
    b = 5293+4722*cct/10000;
    
    cctPwm = k*level;
    if (b >= cctPwm)
    { cctPwm = b - cctPwm;}
    else
    { cctPwm = 0;}
  }

  if (cctPwm > TICS_PER_PERIOD)
  { cctPwm = TICS_PER_PERIOD;}

  saveCctPwm = cctPwm;
  powerCount = POWER_COUNT_MAX;
  SetPwmLevel((int16u)cctPwm, COLOR_TEMP_CTRL_PIN);
  //emberAfGuaranteedPrint("%d ",cctPwm);
}

// SetLedDimming
// y = -22.7x+5577.1
void SetLedDimming(int8u level)
{
  int32u dimLevel;

  if (level == DIMMING_MAX_LEVEL)
  { dimLevel = 0;}
  else if (level == 0)
  { dimLevel = TICS_PER_PERIOD;}
  else
  {
    dimLevel = (int32u)level;
    dimLevel = dimLevel*227;
    if (55771 >= dimLevel)
    { dimLevel = (55771-dimLevel)/10;}
    else
    { dimLevel = 0;}
  }

  if (dimLevel > TICS_PER_PERIOD)
  { dimLevel = TICS_PER_PERIOD;}

  powerCount = POWER_COUNT_MAX;
  if (FALSE == theGatewayType)
  { SetLedCct(level);}
  SetPwmLevel((int16u)dimLevel, DIMMING_CTRL_PIN);
  //emberAfGuaranteedPrint("%d\r\n", dimLevel);
}
// SetLedInitialValue
void SetLedInitialValue(void)
{
  int8u onLevel, currentLevel;
  int16u extendedResetInfo = halGetExtendedResetInfo();
  boolean onOff;

  // onOff attribute
  if (extendedResetInfo == 0x0301) //zheng chang shang dian 
  { onOff = 1;}
  else
  { emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                               ZCL_ON_OFF_CLUSTER_ID,
                               ZCL_ON_OFF_ATTRIBUTE_ID,
                               (int8u *)&onOff,
                               sizeof(boolean));    
  }

  // current level attribute and onLevel attribute
  emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_LEVEL_CONTROL_CLUSTER_ID,
                              ZCL_ON_LEVEL_ATTRIBUTE_ID,
                              (int8u *)&onLevel,
                              sizeof(int8u));

  disableLampOn = TRUE;
  if (0xff != onLevel)  // onLevel is active
  {
    if (onLevel <= DIMMING_MIN_LEVEL)
    {
      onLevel = DIMMING_MAX_LEVEL;
      emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                         ZCL_LEVEL_CONTROL_CLUSTER_ID,
                         ZCL_ON_LEVEL_ATTRIBUTE_ID,
                         (int8u *)&onLevel,
                         ZCL_INT8U_ATTRIBUTE_TYPE);      
    }

    emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                         ZCL_LEVEL_CONTROL_CLUSTER_ID,
                         ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                         (int8u *)&onLevel,
                         ZCL_INT8U_ATTRIBUTE_TYPE);
  }
  else
  {
    emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                            ZCL_LEVEL_CONTROL_CLUSTER_ID,
                            ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                            (int8u *)&currentLevel,
                            sizeof(int8u));
    
    if (currentLevel <= DIMMING_MIN_LEVEL)  
    { 
      currentLevel = DIMMING_MAX_LEVEL;
      emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                           ZCL_LEVEL_CONTROL_CLUSTER_ID,
                           ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                           (int8u *)&currentLevel,
                           ZCL_INT8U_ATTRIBUTE_TYPE);
    }
  }
  disableLampOn = FALSE;

  // color temprature attribute
  if (TRUE == theGatewayType)
  {
    int16u cct;
    emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                                ZCL_COLOR_CONTROL_CLUSTER_ID,
                                ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
                                (int8u *)&cct,
                                sizeof(int16u));
    saveCctLeve = cct;
    SetLedCct(cct);
  }

  // on-off
  emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                               ZCL_ON_OFF_CLUSTER_ID,
                               ZCL_ON_OFF_ATTRIBUTE_ID,
                               (int8u *)&onOff,
                               ZCL_BOOLEAN_ATTRIBUTE_TYPE);
}
// OnOff Cluster
void emberAfOnOffClusterServerAttributeChangedCallback(int8u endpoint, 
                                                       EmberAfAttributeId attributeId)
{
  if (ZCL_ON_OFF_ATTRIBUTE_ID == attributeId)
  {
    boolean isOn;

    emberAfReadServerAttribute(endpoint,
                             ZCL_ON_OFF_CLUSTER_ID,
                             ZCL_ON_OFF_ATTRIBUTE_ID,
                             (int8u *)&isOn,
                             sizeof(boolean));
  
    if (isOn)
    {      
      int8u currentLevel;
      emberAfReadServerAttribute(endpoint,
                              ZCL_LEVEL_CONTROL_CLUSTER_ID,
                              ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                              (int8u *)&currentLevel,
                              sizeof(int8u));
      if (currentLevel > DIMMING_MIN_LEVEL)
      {
        SetLedDimming(currentLevel);
        SET_LAMP_ON();
      }
      else
      { SET_LAMP_OFF();}
    }
    else
    { SET_LAMP_OFF();}
  }
}

// LevelControl Cluster
void emberAfLevelControlClusterServerAttributeChangedCallback(int8u endpoint,
                                                              EmberAfAttributeId attributeId)
{
  if (ZCL_CURRENT_LEVEL_ATTRIBUTE_ID == attributeId)
  {
    boolean isOn;    
    
    emberAfReadServerAttribute(endpoint,
                             ZCL_ON_OFF_CLUSTER_ID,
                             ZCL_ON_OFF_ATTRIBUTE_ID,
                             (int8u *)&isOn,
                             sizeof(boolean));

    
    if (isOn)    
    {
      int8u currentLevel;
      emberAfReadServerAttribute(endpoint,
                              ZCL_LEVEL_CONTROL_CLUSTER_ID,
                              ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                              (int8u *)&currentLevel,
                              sizeof(int8u));

      SetLedDimming(currentLevel);

      if (currentLevel > DIMMING_MIN_LEVEL) 
      { 
        if (disableLampOn == FALSE)
        { SET_LAMP_ON();}
      }
      else
      { SET_LAMP_OFF();}
    }
  }
}
// Color Temprature Control Cluster
void emberAfColorControlClusterServerAttributeChangedCallback(int8u endpoint,
                                                              EmberAfAttributeId attributeId)
{
  if (ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID == attributeId)
  {    
    int16u currentLevel;

    emberAfReadServerAttribute(endpoint,
                              ZCL_COLOR_CONTROL_CLUSTER_ID,
                              ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
                              (int8u *)&currentLevel,
                              sizeof(int16u));
    saveCctLeve = currentLevel;   
    
    SetLedCct(currentLevel);

    //emberAfGuaranteedPrint("currentLevel = %d\r\n", currentLevel);
  }
}
//****************************************************************
//Diagnostics Cluster
//****************************************************************
EmberEventControl diagnosticsEventControl;

void diagnosticsEventFunction(void) 
{
  int16u tmp;

  tmp = emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_SUCCESS]+emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_FAILED]+emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_RETRY];
  if (tmp != 0)
  { 
    tmp = emberCounters[EMBER_COUNTER_MAC_TX_UNICAST_RETRY]/tmp;
    
    emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                               ZCL_DIAGNOSTICS_CLUSTER_ID,
                               ZCL_AVERAGE_MAC_RETRY_PER_APS_MSG_SENT_ATTRIBUTE_ID,
                               (int8u *)&tmp,
                               ZCL_INT16U_ATTRIBUTE_TYPE);
  }  
  
  emberEventControlSetDelayQS(diagnosticsEventControl, 240);  //60s
}

boolean emberAfPreMessageReceivedCallback(EmberAfIncomingMessage* incomingMessage)
{
  emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                               ZCL_DIAGNOSTICS_CLUSTER_ID,
                               ZCL_LAST_MESSAGE_LQI_ATTRIBUTE_ID,
                               (int8u *)&(incomingMessage->lastHopLqi),
                               ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                               ZCL_DIAGNOSTICS_CLUSTER_ID,
                               ZCL_LAST_MESSAGE_RSSI_ATTRIBUTE_ID,
                               (int8u *)&(incomingMessage->lastHopRssi),
                               ZCL_INT8S_ATTRIBUTE_TYPE);
  
  return FALSE;
}
//****************************************************************
//identify
//****************************************************************
#define BLINK_CYCLE  500

EmberEventControl emberAfPluginIdentifyFeedbackProvideFeedbackEventControl;

// emberAfPluginIdentifyFeedbackProvideFeedbackEventHandler
void emberAfPluginIdentifyFeedbackProvideFeedbackEventHandler(void)
{
  static boolean flag = FALSE;

  SET_LAMP_ON();
  
  if (flag == TRUE)
  {
    flag = FALSE;    
    SetLedDimming(DIMMING_MAX_LEVEL);
  }
  else
  { 
    flag = TRUE;
    SetLedDimming(DIMMING_MIN_LEVEL);
  }
  
  emberEventControlSetDelayMS(emberAfPluginIdentifyFeedbackProvideFeedbackEventControl, BLINK_CYCLE);
}
// emberAfPluginIdentifyStartFeedbackCallback
void emberAfPluginIdentifyStartFeedbackCallback(int8u endpoint, int16u identifyTime)
{
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_IDENTIFY_CLUSTER_ID);

  if (ep == 0xFF) 
  { return;}
  
  emberEventControlSetDelayMS(emberAfPluginIdentifyFeedbackProvideFeedbackEventControl,
                              BLINK_CYCLE);
}
// emberAfPluginIdentifyStopFeedbackCallback
void emberAfPluginIdentifyStopFeedbackCallback(int8u endpoint)
{
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_IDENTIFY_CLUSTER_ID);
  
  if (ep == 0xFF) 
  { return;}

  emberAfOnOffClusterServerAttributeChangedCallback(emberAfPrimaryEndpoint(), ZCL_ON_OFF_ATTRIBUTE_ID);
  
  emberEventControlSetInactive(emberAfPluginIdentifyFeedbackProvideFeedbackEventControl);
}
//*******************************************************************************
// Button
//*******************************************************************************
enum{
  REJOIN_INDICATE = 0,
  NETWORK_LEAVE_INDICATE,
  JOIN_NETWORK_SUCCESS,
  HAL_REBOOT
};

EmberEventControl indicateEventControl;
const int8u indicateCountMax[] = {4, 8, 6};
static int8u indicateType;
static int8u indicateCount;


EmberEventControl SwitchOvertimeEventControl;
EmberEventControl buttonEventControl;
EmberEventControl DimmingEventControl;

enum {
  DIMMABLE_UP = 0,
  DIMMABLE_DOWN,  
};
static int8u   onOffCount = 0;

static boolean dimmingFlag = FALSE;
static boolean dimmableUpDown = DIMMABLE_UP; 
static int8u   dimmableStep;
static int8u   dimmingLevel;

enum{
  CONTROLLER_INIT = 0,
  CONTROLLER_OPERATION,
  CONTROLLER_FINISH,
  DOUBLE_CLICK_DISABLE,
  DUBLE_CLICK_ENABLE
};

EmberEventControl controllerEventControl;
int8u controllerAndDoubleClickStatus = CONTROLLER_INIT;
static int8u controllerCounter;

static boolean saveOnOff;

#define ENABLE_DIMMING

void indicateEventFunction(void) 
{  
  if (indicateCount >= indicateCountMax[indicateType])
  {
    if ((NETWORK_LEAVE_INDICATE != indicateType) && (HAL_REBOOT != indicateType))
    { emberAfOnOffClusterServerAttributeChangedCallback(emberAfPrimaryEndpoint(), ZCL_ON_OFF_ATTRIBUTE_ID);}
    emberEventControlSetInactive(indicateEventControl);
    //emberAfGuaranteedPrint("indicateFinish\r\n");
    if (NETWORK_LEAVE_INDICATE == indicateType)
    { 
      SET_LAMP_OFF();
      emberAfPluginBasicResetToFactoryDefaultsCallback(emberAfCurrentEndpoint());
      indicateType = HAL_REBOOT;
      emberEventControlSetDelayMS(indicateEventControl, 300);
    }
    else if (HAL_REBOOT == indicateType)
    { halReboot();}
  }
  else
  {
    SET_LAMP_ON();
    if (indicateCount & 0x01)
    { SetLedDimming(DIMMING_MAX_LEVEL);}
    else
    { SetLedDimming(DIMMING_MIN_LEVEL);}
    
    indicateCount++;
    emberEventControlSetDelayMS(indicateEventControl, 500);
    //emberAfGuaranteedPrint("indicate\r\n");
  }    
}
void SetIndicateEvent(int8u type)
{
  indicateCount = 0;
  indicateType = type;  
  emberEventControlSetActive(indicateEventControl);  
}
void SwitchOvertimeEventFunction(void) 
{
  onOffCount = 0;
  emberEventControlSetInactive(SwitchOvertimeEventControl);

  if (saveOnOff == 0)
  {
    boolean isOn = 0;
    emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                             ZCL_ON_OFF_CLUSTER_ID,
                             ZCL_ON_OFF_ATTRIBUTE_ID,
                             (int8u *)&isOn,
                             ZCL_BOOLEAN_ATTRIBUTE_TYPE);
  }  
}
void DimmingEventFunction(void) 
{ 
  emberEventControlSetInactive(DimmingEventControl);

  #ifdef ENABLE_DIMMING
  if (dimmableUpDown == DIMMABLE_UP)
  {
    if (dimmingLevel >= DIMMING_MAX_LEVEL)
    {
      dimmableStep++;
      if (dimmableStep >= 2)
      {
        onOffCount = 0;
        dimmingFlag = FALSE;
        emberEventControlSetInactive(DimmingEventControl);
        return;
      }
      dimmableUpDown = DIMMABLE_DOWN;
      dimmingLevel--;
    }
    else
    { dimmingLevel++;}
  }
  else
  {
    if (dimmingLevel <= 1)
    {
      dimmableUpDown = DIMMABLE_UP;
      dimmingLevel++;
    }
    else
    { dimmingLevel--;}
  }

  emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                             ZCL_LEVEL_CONTROL_CLUSTER_ID,
                             ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                             (int8u *)&dimmingLevel,
                             ZCL_INT8U_ATTRIBUTE_TYPE);
  
  int8u currentLevel;
  emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_LEVEL_CONTROL_CLUSTER_ID,
                              ZCL_ON_LEVEL_ATTRIBUTE_ID,
                              (int8u *)&currentLevel,
                              sizeof(int8u));
  if (0xff != currentLevel)
  {
    emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                             ZCL_LEVEL_CONTROL_CLUSTER_ID,
                             ZCL_ON_LEVEL_ATTRIBUTE_ID,
                             (int8u *)&dimmingLevel,
                             ZCL_INT8U_ATTRIBUTE_TYPE);
  }  
  
  emberEventControlSetDelayMS(DimmingEventControl, 30);
  #endif
}

void buttonEventHandler(void) 
{ 
  if (10 <= onOffCount)
  {      
    emberLeaveNetwork();

    emberAfResetAttributes(emberAfPrimaryEndpoint());
    emberAfGroupsClusterClearGroupTableCallback(emberAfPrimaryEndpoint());
    emberAfScenesClusterClearSceneTableCallback(emberAfPrimaryEndpoint());
    SetTheGatewayType(FALSE);
    
    emberClearBindingTable();
    emberAfClearReportTableCallback();

    onOffCount = 0;
    emberEventControlSetInactive(SwitchOvertimeEventControl);
    SetIndicateEvent(NETWORK_LEAVE_INDICATE);    
  }
  else if (7 == onOffCount)
  {
    emberAfEzmodeServerCommission(emberAfPrimaryEndpoint());
    onOffCount = 0;
    emberEventControlSetInactive(SwitchOvertimeEventControl);
  }
  else if (5 == onOffCount)
  {
    SetIndicateEvent(REJOIN_INDICATE);
    emberFindAndRejoinNetworkWithReason(0,  // 1=rejoin with encryption, 0=rejoin without encryption
                                        EMBER_ALL_802_15_4_CHANNELS_MASK,
                                        EMBER_AF_REJOIN_DUE_TO_END_DEVICE_MOVE); // end device rejoin 
    onOffCount = 0;
    emberEventControlSetInactive(SwitchOvertimeEventControl);
  }
  #ifdef ENABLE_DIMMING
  else if (1 == onOffCount)
  {
    boolean isOn = 1;
    
    dimmingFlag = TRUE;
    emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                            ZCL_LEVEL_CONTROL_CLUSTER_ID,
                            ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                            (int8u *)&dimmingLevel,
                            sizeof(int8u));
  
    emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                           ZCL_ON_OFF_CLUSTER_ID,
                           ZCL_ON_OFF_ATTRIBUTE_ID,
                           (int8u *)&isOn,
                           ZCL_BOOLEAN_ATTRIBUTE_TYPE);
    dimmableStep = 0;
    dimmableUpDown = DIMMABLE_UP;
    emberEventControlSetActive(DimmingEventControl);
    emberEventControlSetInactive(SwitchOvertimeEventControl);
  }  
  else if ((2 == onOffCount) && (dimmingFlag == TRUE))
  {
    dimmingFlag = FALSE;
    emberEventControlSetInactive(DimmingEventControl);
    onOffCount = 0;
    emberEventControlSetInactive(SwitchOvertimeEventControl);
  }
  #endif
  else
  { onOffCount = 0;}
    
  emberEventControlSetInactive(buttonEventControl);
}

void controllerEventFunction(void)
{
  emberEventControlSetInactive(controllerEventControl);
  if (controllerAndDoubleClickStatus == CONTROLLER_INIT)
  {
    controllerCounter = 0;
    controllerAndDoubleClickStatus = CONTROLLER_OPERATION;
    emberEventControlSetDelayMS(controllerEventControl, 1000);
  }
  else if (controllerAndDoubleClickStatus == CONTROLLER_OPERATION)
  {
    if (controllerCounter <= 40)
    { controllerAndDoubleClickStatus = DUBLE_CLICK_ENABLE;}
    else
    { controllerAndDoubleClickStatus = CONTROLLER_FINISH;}
  }
  else if (controllerAndDoubleClickStatus == DOUBLE_CLICK_DISABLE)
  {
    controllerAndDoubleClickStatus = DUBLE_CLICK_ENABLE;
  }
}

// Hal Button ISR Callback
// This callback is called by the framework whenever a button is pressed on the 
// device. This callback is called within ISR context.
void emberAfHalButtonIsrCallback(int8u button, int8u state) 
{ 
  static int16u oldTime, newTime;
  
  if (button == BUTTON1)
  {
    if ((state == BUTTON_RELEASED) 
         && (controllerAndDoubleClickStatus == CONTROLLER_OPERATION))
    { controllerCounter++;}
  }
  else if ((button == BUTTON0) 
           && (controllerAndDoubleClickStatus == DUBLE_CLICK_ENABLE))
  { 
    boolean isOn;
    emberEventControlSetInactive(SwitchOvertimeEventControl);
    if (state == BUTTON_RELEASED)
    {
      //emberAfGuaranteedPrint("BUTTON_RELEASED, onOffCount = %d;\r\n", onOffCount);      
      isOn = 1;
      saveOnOff = 1;
      emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                                 ZCL_ON_OFF_CLUSTER_ID,
                                 ZCL_ON_OFF_ATTRIBUTE_ID,
                                 (int8u *)&isOn,
                                 ZCL_BOOLEAN_ATTRIBUTE_TYPE);
      if (onOffCount == 0)
      { return;}

      newTime = halCommonGetInt16uMillisecondTick();
      if (50 > elapsedTimeInt16u(oldTime, newTime))
      {
        onOffCount = 0;
        return;
      }
      
      if (2 != onOffCount)
      { emberEventControlSetDelayMS(buttonEventControl, 600);}
    }
    else
    {
      //emberAfGuaranteedPrint("BUTTON_PRESSED, onOffCount = %d;\r\n", onOffCount);      
      oldTime = halCommonGetInt16uMillisecondTick();
      onOffCount++;
      emberEventControlSetInactive(buttonEventControl);
      if ((2 == onOffCount) && (TRUE == dimmingFlag))
      { emberEventControlSetActive(buttonEventControl);}
      
      saveOnOff = 0;
      emberEventControlSetDelayMS(SwitchOvertimeEventControl, 500);

      SetTheApsNumType(emberAfGetLastSequenceNumber());
    }
  }
}
/** @brief Reset To Factory Defaults
 *
 * This function is called by the Basic server plugin when a request to reset to
 * factory defaults is received.  The plugin will reset attributes managed by
 * the framework to their default values.  The application should preform any
 * other necessary reset-related operations in this callback, including
 * resetting any externally-stored attributes.
 *
 * @param endpoint   Ver.: always
 */

void emberAfPluginBasicResetToFactoryDefaultsCallback(int8u endpoint)
{
  theGatewayType = FALSE;
  SetTheGatewayType(FALSE);
}

//*******************************************************************************
// Join the network
//*******************************************************************************
static int16u  joinNetworkOverTime;
EmberEventControl JoinTheNetworkEventControl;
boolean firstJoinNetworkFlag = TRUE;

void emberAfPluginNetworkFindFinishedCallback(EmberStatus status)
{
  EmberNetworkStatus nStatus = emberAfNetworkState();    
  
  // If we go up or down, let the user know, although the down case shouldn't
  // happen.
  if ((nStatus == EMBER_JOINED_NETWORK) || (nStatus == EMBER_JOINED_NETWORK_NO_PARENT))
    return;
  
  emberEventControlSetActive(JoinTheNetworkEventControl);
}
// Event function stubs
void JoinTheNetworkEventFunction(void) 
{
  if (joinNetworkOverTime <= 85)
  {
    joinNetworkOverTime++;
    emberAfStartSearchForJoinableNetwork();
  }  

  emberEventControlSetInactive(JoinTheNetworkEventControl);
}
// JoinTheNetworkInit
void JoinTheNetworkInit(void)
{  
  joinNetworkOverTime = 0;
  emberEventControlSetDelayMS(JoinTheNetworkEventControl, 3000);  //will Join a network after 3 seconds
}

// emberAfStackStatusCallback
boolean emberAfStackStatusCallback(EmberStatus status)
{
  EmberNetworkStatus nStatus = emberAfNetworkState();
  
  if ((nStatus == EMBER_JOINED_NETWORK) 
      || (nStatus == EMBER_JOINED_NETWORK_NO_PARENT)) 
  {
    boolean isOn;
    int8u currentLevel;
    int16u cctLevel;
      
    if (0 != joinNetworkOverTime)
    {
      isOn = 1;
      currentLevel = 0xff;
      cctLevel = ColorTempPhysicalMax;

      if (TRUE == firstJoinNetworkFlag)
      {
        firstJoinNetworkFlag = FALSE;
        SetIndicateEvent(JOIN_NETWORK_SUCCESS);
      }
    }
    else
    {
      emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                               ZCL_ON_OFF_CLUSTER_ID,
                               ZCL_ON_OFF_ATTRIBUTE_ID,
                               (int8u *)&isOn,
                               sizeof(boolean));
      emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                                ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                (int8u *)&currentLevel,
                                sizeof(int8u));
      emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                                ZCL_COLOR_CONTROL_CLUSTER_ID,
                                ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
                                (int8u *)&cctLevel,
                                sizeof(int16u));
    }

    if (nStatus == EMBER_JOINED_NETWORK)
    {
      joinNetworkOverTime = 0;      
      if (TRUE == theGatewayType)
      {
        emberAfWriteServerAttribute(emberAfPrimaryEndpoint(), 
                                  ZCL_COLOR_CONTROL_CLUSTER_ID, 
                                  ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID, 
                                  (int8u*)&cctLevel, 
                                  ZCL_INT16U_ATTRIBUTE_TYPE); 
      }
      emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                                 ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                 ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                 (int8u *)&currentLevel,
                                 ZCL_INT8U_ATTRIBUTE_TYPE);
      emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                                 ZCL_ON_OFF_CLUSTER_ID,
                                 ZCL_ON_OFF_ATTRIBUTE_ID,
                                 (int8u *)&isOn,
                                 ZCL_BOOLEAN_ATTRIBUTE_TYPE);
    }
    else
    { 
      if (TRUE == isOn)
      {
        SetLedDimming(currentLevel);
        if (TRUE == theGatewayType)
        { SetLedCct(cctLevel);}
      }  
    }
    
    emberEventControlSetInactive(JoinTheNetworkEventControl);
  }  
  else if (status == EMBER_NETWORK_DOWN)
  {
    joinNetworkOverTime = 0;
    firstJoinNetworkFlag = TRUE;
    emberEventControlSetDelayMS(JoinTheNetworkEventControl, 3000);  //3s
    emberClearBindingTable();
  }
  
  return FALSE;
}

//****************************************************************
// Power and Consumption
// ADC0 -ADC5: PB5,PB6,PB7,PC1,PA4,PA5
//****************************************************************
#define ADC_IN_VOLTAGE     ADC_SOURCE_ADC4_VREF2
#define ADC_IN_CURRENT     ADC_SOURCE_ADC3_VREF2

#define ADC_LEVEL_MIN (255*18/100)
static int8u adcChannelIndex;
const  int8u adcChannel[] = {ADC_IN_CURRENT, ADC_IN_VOLTAGE};
#define ADC_CHANNEL_NUM (sizeof(adcChannel)/sizeof(adcChannel[0]))

EmberEventControl PowerConsumptionEventControl;
static int16s adcData[ADC_CHANNEL_NUM] = {0};


// PowerAndConsumptionInit
void PowerAndConsumptionInit(void)
{
  adcChannelIndex = 0;
  halStartAdcConversion(ADC_USER_APP2, 
                        ADC_REF_INT, 
                        adcChannel[adcChannelIndex],
                        ADC_CONVERSION_TIME_US_256);
    
  emberEventControlSetDelayMS(PowerConsumptionEventControl, 1000);
}
// level >= 113  y = 70.41x + 60355
// level < 113    y = 415.62x + 21856

int32s GetEfficiency(int8u level)
{
  if (level >= 113)
  { return (int32u)((level*704)/10+60355);}
  else
  { return (int32u)((level*4156)/10+21856);}
}

//
int16s GetFilteredValue(int16s v, int8u index)
{
  static int16s buf[ADC_CHANNEL_NUM][12] = {0};
  static int8u ptr[ADC_CHANNEL_NUM] = {0};
  static int16s max, min;
  static int32s total;
  static int8u  i;
  
  buf[index][ptr[index]] = v;  
  max = buf[index][0];
  min = buf[index][0];
  total = buf[index][0];
  for (i=1; i<12; i++)
  {
    total += buf[index][i];
    if (buf[index][i] > max)
    { max = buf[index][i];}
    else if (min > buf[index][i])
    { min = buf[index][i];}
  }
  
  ptr[index]++;
  if (ptr[index] >= 12)
    ptr[index] = 0;

  total -= max;
  total -= min;
  total /= 10;

  return (int16s)total;
}
//
#define GetCurrent(c) (((c)*10)/27+10)
#define GetVoltage(c) ((int16s)(((int32s)(c)*3671)/10000))
void AdcControlFunction(void)
{
  static int16u value;
  static int16s fvolts;
      
  if (halRequestAdcData(ADC_USER_APP2, &value) == EMBER_ADC_CONVERSION_DONE)
  {     
    fvolts = halConvertValueToVolts(value / TEMP_SENSOR_SCALE_FACTOR);
    adcData[adcChannelIndex] = GetFilteredValue(fvolts, adcChannelIndex);
    
    if (TRUE == theGatewayType)
    {
      if (adcChannelIndex == 0)
      {
        int16s currentDifference;
        int16s currentIn;
         
        currentIn = GetCurrent(adcData[0]);
        currentIn = (currentIn<0)?0:currentIn;
        currentDifference = (saveCurrent>currentIn)?(saveCurrent-currentIn):(currentIn-saveCurrent);
        if (currentDifference >= 10)
        {
          saveCurrent = currentIn;
          ColorTemperatureCompensation(currentIn/10,saveCctLeve);
        }
        //emberAfGuaranteedPrint("%d, %d\r\n",currentDifference, currentIn);
      }
    }
    
    adcChannelIndex++;
    if (adcChannelIndex >= ADC_CHANNEL_NUM)
    { adcChannelIndex = 0;}
    halStartAdcConversion(ADC_USER_APP2, 
                          ADC_REF_INT, 
                          adcChannel[adcChannelIndex],
                          ADC_CONVERSION_TIME_US_256);    
  } 
}

void PowerConsumptionEventFunction(void) 
{
  static boolean firstFlag = TRUE;
  static int16u shortPower = 0;
  int16s currentIn, voltageIn;
  int8u  currentLevel;
  int32s power;
  boolean isOn;  
  static int64u powerConsumption;

  if (firstFlag == TRUE)
  {
    int8u readData[6];
    
    firstFlag = FALSE;

    emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
                              readData,
                              6);    
    readData[4] = 0;
    readData[5] = 0;
    emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
                              readData,
                              ZCL_INT48U_ATTRIBUTE_TYPE);
   
    emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
                              (int8u *)&powerConsumption,
                              6);
  }
  //Computing Power 
  currentIn = GetCurrent(adcData[0]);
  currentIn = (currentIn<0)?0:currentIn;
  voltageIn = GetVoltage(adcData[1]);
  voltageIn = (voltageIn<0)?0:voltageIn;
  //emberAfGuaranteedPrint("c=%d, v=%d, ",currentIn, voltageIn);

  emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                             ZCL_LEVEL_CONTROL_CLUSTER_ID,
                             ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                             (int8u *)&currentLevel,
                             sizeof(int8u));
  emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                             ZCL_ON_OFF_CLUSTER_ID,
                             ZCL_ON_OFF_ATTRIBUTE_ID,
                             (int8u *)&isOn,
                             sizeof(boolean));
  if (currentLevel >= 10)
  { power = ((int32s)currentIn*voltageIn)/GetEfficiency(currentLevel);}
  else
  { power = 13;}    
  
  if ((isOn == 0) || (currentLevel==DIMMING_MIN_LEVEL))
  { power = 0;}

  //emberAfGuaranteedPrint("p=%d;",power);
  //emberAfGuaranteedPrint("\r\n");
  if (powerCount != 0)
  {
    powerCount--;
    if (powerCount == 0)
    {
      emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                                  ZCL_SIMPLE_METERING_CLUSTER_ID,
                                  ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID,
                                  (int8u *)&power,
                                  ZCL_INT24S_ATTRIBUTE_TYPE);
    }
  }
  //Computing Consumption
  shortPower += power;
  if (shortPower >= 3600)
  {
    //emberAfGuaranteedPrint("%d, ",powerConsumption);
    shortPower -= 3600;
    powerConsumption++;
    
    emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
                              (int8u *)&powerConsumption,
                              ZCL_INT48U_ATTRIBUTE_TYPE);
    //emberAfGuaranteedPrint("%d\r\n",powerConsumption);
    
  }
  
  emberEventControlSetDelayMS(PowerConsumptionEventControl, 1000);
}

//*******************************************************************************
// 1-10: level: 10% -100%
// 11-20: cct: 10% -100%
// 99: firmware version
//*******************************************************************************
int8u firmwareVersion[] = "Z01-A19NAE26-v1.0.7";
boolean emberProcessCommandSendled(int8u *input, int8u sizeOrPort)
{
  int8u dat;
  
  while (EMBER_SUCCESS == emberSerialReadByte(sizeOrPort, &dat))
  {
    if ((1 <= dat) && (dat <= 10)) 
    {
      powerCount = POWER_COUNT_MAX;
      SetPwmLevel(TICS_PER_PERIOD-600*dat, DIMMING_CTRL_PIN);
      emberSerialWriteData(APP_SERIAL, &dat, 1);
    }
    else if ((11 <= dat) && (dat <= 20))
    { 
      powerCount = POWER_COUNT_MAX;
      SetPwmLevel((dat-10)*600, COLOR_TEMP_CTRL_PIN);
      emberSerialWriteData(APP_SERIAL, &dat, 1);
    }
    else if (99 == dat)
    { emberSerialWriteData(APP_SERIAL, firmwareVersion, sizeof(firmwareVersion));}
  }

  return FALSE;
}

//*******************************************************************************
// Main
//*******************************************************************************
void emberAfMainInitCallback(void)
{
  GetTheApsNumType();
  JoinTheNetworkInit();
  PowerAndConsumptionInit();
  emberEventControlSetDelayQS(diagnosticsEventControl, 240);  //60s

  emberEventControlSetDelayMS(controllerEventControl, 200);

  halCommonGetToken(&theGatewayType, TOKEN_THE_GATEWAY_TYPE);
}
//
boolean emberAfMainStartCallback(int* returnCode,
                                 int argc,
                                 char** argv)
{
#if 0  
  int16u oldTime = halCommonGetInt16uMillisecondTick();  
  int16u newTime = oldTime;
  
  while(elapsedTimeInt16u(oldTime, newTime) < 200)
  {
    halResetWatchdog();   // Periodically reset the watchdog.
    newTime = halCommonGetInt16uMillisecondTick();
  }
#endif
  return FALSE;
}
//
void emberAfMainTickCallback(void)
{
  //*****************************************************
  // set power attribute
  //*****************************************************  
  AdcControlFunction();
}





