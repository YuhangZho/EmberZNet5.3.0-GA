
#include "E1A_AC2_Public_Head.h"


//*******************************************************************************
// Button
//*******************************************************************************
#define SET_OPEN_CIRCUIT()    do { GPIO_PBCLR = BIT(6); } while (0)
#define SET_CLOSE_CIRCUIT()   do { GPIO_PBSET = BIT(6); } while (0)

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

//sengled wms 2016.1.7
extern int8u disableReportMinTime;

void emberAfPluginReportingTickEventHandler(void);

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
  ;
}


//
void indicateEventFunction(void) 
{  
  if (indicateCount >= indicateCountMax[indicateType])
  {
    emberEventControlSetInactive(indicateEventControl);
    
    if ((NETWORK_LEAVE_INDICATE != indicateType) && (HAL_REBOOT != indicateType))
    { emberAfOnOffClusterServerAttributeChangedCallback(emberAfPrimaryEndpoint(), ZCL_ON_OFF_ATTRIBUTE_ID);}
    
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
    
    disableReportMinTime = TRUE;
    emberAfPluginReportingTickEventHandler();
    disableReportMinTime = FALSE;
  }  
}
void DimmingEventFunction(void) 
{ 
  emberEventControlSetInactive(DimmingEventControl);

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
}

void buttonEventHandler(void) 
{ 
  if (5 <= onOffCount)
  {      
    emberLeaveNetwork();

    emberAfResetAttributes(emberAfPrimaryEndpoint());
    emberAfGroupsClusterClearGroupTableCallback(emberAfPrimaryEndpoint());
    emberAfScenesClusterClearSceneTableCallback(emberAfPrimaryEndpoint());
    
    emberClearBindingTable();
    emberAfClearReportTableCallback();

    onOffCount = 0;
    emberEventControlSetInactive(SwitchOvertimeEventControl);
    SetIndicateEvent(NETWORK_LEAVE_INDICATE);    
  }
/*
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
*/
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
    { 
      SET_CLOSE_CIRCUIT();
      controllerAndDoubleClickStatus = DUBLE_CLICK_ENABLE;
    }
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
  static int16u oldTime = 0, newTime;
  
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
      
      if (2 != onOffCount)
      { emberEventControlSetDelayMS(buttonEventControl, 600);}
    }
    else
    {
      //emberAfGuaranteedPrint("BUTTON_PRESSED, onOffCount = %d;\r\n", onOffCount);    
      newTime = halCommonGetInt16uMillisecondTick();
      if (50 < elapsedTimeInt16u(oldTime, newTime))
      { onOffCount++;}
      oldTime = halCommonGetInt16uMillisecondTick();
      
      emberEventControlSetInactive(buttonEventControl);
      if ((2 == onOffCount) && (TRUE == dimmingFlag))
      { emberEventControlSetActive(buttonEventControl);}
      
      saveOnOff = 0;
      emberEventControlSetDelayMS(SwitchOvertimeEventControl, 500);

      SetTheApsNumType(emberAfGetLastSequenceNumber());
    }
  }
}

void ButtonInit(void)
{
  SET_OPEN_CIRCUIT();
  emberEventControlSetDelayMS(controllerEventControl, 200);
}


