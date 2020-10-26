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



//****************************************************************
//Light Control
//****************************************************************
#define DIMMING_MAX_LEVEL  255
#define DIMMING_MIN_LEVEL  0

#define DIMMING_CTRL_PIN PIN_PA6

#define COLOR_TEMP_MAX_LEVEL  100
#define COLOR_TEMP_MIN_LEVEL  0

#define COLOR_TEMP_CTRL_PIN PIN_PB7

#define COMPARE_WITH_LOW_LIMIT(n)  (n < 25)

extern boolean underVoltageState;


static int32s power;
EmberEventControl WritePowerAttributeEventControl;

// WritePowerAttributeEventFunction
void WritePowerAttributeEventFunction(void) 
{
	EmberNetworkStatus nStatus = emberAfNetworkState();

    if (nStatus == EMBER_JOINED_NETWORK)
    {
    	emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID,
                              (int8u *)&power,
                              ZCL_INT24S_ATTRIBUTE_TYPE);
		emberEventControlSetInactive(WritePowerAttributeEventControl);
    }
	
}

// SetLedCct
void SetLedCct(int16u cct)
{
  static int32u midleLevel;

  if (cct == DIMMING_MAX_LEVEL)
  { midleLevel =  1440;}
  else if (cct == 0)
  { midleLevel = TICS_PER_PERIOD;}
  else
  {
    midleLevel = (int32s)cct;    
    midleLevel = 6702-(cct*2035)/100;
  }
  
  if (midleLevel > 6000)
    midleLevel = 6000;
  
  SetPwmLevel((int16u)midleLevel, COLOR_TEMP_CTRL_PIN);
}

// SetLedDimming
void SetLedDimming(int8u level)
{
  static int32u midleLevel;

  if (level == DIMMING_MAX_LEVEL)
  { midleLevel = TICS_PER_PERIOD;}
  else if (level == 0)
  { midleLevel = 0;}
  else
  {
    midleLevel = (int32u)level;
    midleLevel = (midleLevel*1847)/100+1290;

    if (midleLevel > 6000)
    { midleLevel = 6000;}
    else if (midleLevel < 1200)
    { midleLevel = 1200;}
  } 

  SetPwmLevel((int16u)midleLevel, DIMMING_CTRL_PIN);

  if (FALSE == underVoltageState)
  { SetLedCct(level);}
}
// GetLedInitialValue
int8u GetLedInitialValue(void)
{
  int8u currentLevel;
  
  emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                          ZCL_LEVEL_CONTROL_CLUSTER_ID,
                          ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                          (int8u *)&currentLevel,
                          sizeof(currentLevel));
    
  if (COMPARE_WITH_LOW_LIMIT(currentLevel))
  { currentLevel = DIMMING_MAX_LEVEL;}  
  
  return currentLevel;
}
// SetLedInitialValue
void SetLedInitialValue(void)
{
  int16u oldTime = halCommonGetInt16uMillisecondTick();  
  int16u newTime = oldTime;
  int8u  currentLevel = GetLedInitialValue();
  
  for (int16u i=0; i<=currentLevel; i++)
  {
    SetLedDimming(i);
    while(elapsedTimeInt16u(oldTime, newTime) < 1)
    {
      halResetWatchdog();   // Periodically reset the watchdog.
      newTime = halCommonGetInt16uMillisecondTick();
    }

    oldTime = newTime; 
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
    }
    
    emberEventControlSetDelayMS(WritePowerAttributeEventControl, 1500);
  }
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
      
      SetLedDimming(currentLevel);
    }
    else
    {
      SetLedDimming(DIMMING_MIN_LEVEL);
    }

    emberEventControlSetDelayMS(WritePowerAttributeEventControl, 1500);
  }
}
// Color Temprature Control Cluster
void emberAfColorControlClusterServerAttributeChangedCallback(int8u endpoint,
                                                              EmberAfAttributeId attributeId)
{
  if (ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID == attributeId)
  {
    boolean isOn;
    
    emberAfReadServerAttribute(endpoint,
                             ZCL_ON_OFF_CLUSTER_ID,
                             ZCL_ON_OFF_ATTRIBUTE_ID,
                             (int8u *)&isOn,
                             sizeof(boolean));
    if (isOn)
    {
      int16u currentLevel;
      emberAfReadServerAttribute(endpoint,
                              ZCL_COLOR_CONTROL_CLUSTER_ID,
                              ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
                              (int8u *)&currentLevel,
                              sizeof(int16u));

      SetLedCct(currentLevel);
    }
  }
}
//****************************************************************
//Diagnostics Cluster
//****************************************************************
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
// FunctionDiagnosticsCluster
void FunctionDiagnosticsCluster(void)
{
  static int16u oldTime = 0;  
  static int16u newTime;
  static int16u tmp;
  
  newTime = halCommonGetInt16uQuarterSecondTick();
  if (elapsedTimeInt16u(oldTime, newTime) < 40)  // 10S
  { return;}

  oldTime = newTime;
  
  tmp = emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_SUCCESS]+emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_FAILED]+emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_RETRY];
  if (tmp != 0)
  { tmp = emberCounters[EMBER_COUNTER_MAC_TX_UNICAST_RETRY]/emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_SUCCESS];}
  
  emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                               ZCL_DIAGNOSTICS_CLUSTER_ID,
                               ZCL_AVERAGE_MAC_RETRY_PER_APS_MSG_SENT_ATTRIBUTE_ID,
                               (int8u *)&tmp,
                               ZCL_INT16U_ATTRIBUTE_TYPE);  
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
  
  flag = flag?FALSE:TRUE;
  SetLedDimming(flag?DIMMING_MAX_LEVEL:DIMMING_MIN_LEVEL);
  
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

  emberAfLevelControlClusterServerAttributeChangedCallback(1, ZCL_CURRENT_LEVEL_ATTRIBUTE_ID);
  
  emberEventControlSetInactive(emberAfPluginIdentifyFeedbackProvideFeedbackEventControl);
}
//*******************************************************************************
// Button
//*******************************************************************************
#define MIN_DIMMING_TIME  3

// Event control struct declarations
EmberEventControl buttonEventControl;

static boolean buttonState;
static int8u buttonTimer;
static boolean buttonFirst = TRUE;
static boolean enableJoinNetworkFlag;
static boolean joinedNetworkFlag = FALSE;

// GetStepLevel
int8u GetStepLevel(int8u level)
{
  if (level > 191)  //255 * 75% = 191
    return 191;
  else if (level > 128) //255 * 50% = 128
    return 128;
  else if (level > 64) //255 * 25% = 64
    return 64;
  else if (level > 0) //255 * 0% = 0
    return 0;
  else
    return 255;
}
void buttonEventHandler(void) 
{ 
  if (buttonFirst == TRUE)
  {
    buttonFirst = FALSE;
    emberEventControlSetInactive(buttonEventControl);
    return;
  }

  if (BUTTON_PRESSED == buttonState)
  {
	if (buttonTimer == 20)       // 10s blink 1 time
    { SetLedDimming(DIMMING_MAX_LEVEL);}
    else if (buttonTimer == 21)  // 10s blink 1 time
    { SetLedDimming(DIMMING_MIN_LEVEL);}
    else if (buttonTimer == 22)  // 10s blink 1 time
    { SetLedDimming(DIMMING_MAX_LEVEL);}
    else if (buttonTimer == 40)  // 20s blink 2 time
    { SetLedDimming(DIMMING_MIN_LEVEL);}
    else if (buttonTimer == 41)  // 20s blink 2 time
    { SetLedDimming(DIMMING_MAX_LEVEL);}
    else if (buttonTimer == 42)  // 20s blink 2 time
    { SetLedDimming(DIMMING_MIN_LEVEL);}
    else if (buttonTimer == 43)  // 20s blink 2 time
    { SetLedDimming(DIMMING_MAX_LEVEL);}
    else if (buttonTimer > 44)  
    { buttonTimer = 44;}
    buttonTimer++;

    emberEventControlSetDelayMS(buttonEventControl, 500);   // 1s
  }
  else
  {
    if (buttonTimer < 20)
    {
      boolean isOn;
      int8u currentLevel;
    
      emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                               ZCL_ON_OFF_CLUSTER_ID,
                               ZCL_ON_OFF_ATTRIBUTE_ID,
                               (int8u *)&isOn,
                               sizeof(boolean));
      if (isOn)
      {        
        emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                                ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                (int8u *)&currentLevel,
                                sizeof(int8u));
        currentLevel = GetStepLevel(currentLevel);
        if (currentLevel == 0)
        {
          isOn = 0;
          emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                                   ZCL_ON_OFF_CLUSTER_ID,
                                   ZCL_ON_OFF_ATTRIBUTE_ID,
                                   (int8u *)&isOn,
                                   ZCL_BOOLEAN_ATTRIBUTE_TYPE);
        }
        else
        {
          // Bug: C-M-150412-01
          // Author: Robin
          // Tester: Robin
          // Description: When light device is controled by button, level is 75% or 50% or 25%, 
		  //              when you send "off" and "on" command through panel, the level will change to 100%; 
		  //              pls write the attribute onLevel(ZCL_ON_OFF_ATTRIBUTE_ID), when you press the button.
          //              if the network is up
		  //EmberNetworkStatus nStatus = emberAfNetworkState();  
		  if (joinedNetworkFlag == TRUE)
		  {
            emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                                 ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                 ZCL_ON_LEVEL_ATTRIBUTE_ID,
                                 (int8u *)&currentLevel,
                                 ZCL_INT8U_ATTRIBUTE_TYPE);   
		  }
		

          emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                                 ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                 ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                 (int8u *)&currentLevel,
                                 ZCL_INT8U_ATTRIBUTE_TYPE);          
        }  
      }
      else
      {
        currentLevel = 255;
        emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                               ZCL_LEVEL_CONTROL_CLUSTER_ID,
                               ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                               (int8u *)&currentLevel,
                               ZCL_INT8U_ATTRIBUTE_TYPE);
        isOn = 1;
        emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                                   ZCL_ON_OFF_CLUSTER_ID,
                                   ZCL_ON_OFF_ATTRIBUTE_ID,
                                   (int8u *)&isOn,
                                   ZCL_BOOLEAN_ATTRIBUTE_TYPE);
      }
    }
    else if (buttonTimer < 40)
    {
      joinedNetworkFlag = FALSE ;
	  
      SetLedDimming(DIMMING_MIN_LEVEL);

      emberFindAndRejoinNetworkWithReason(1,  // 1=rejoin with encryption, 0=rejoin without encryption
                                         EMBER_ALL_802_15_4_CHANNELS_MASK,
                                         EMBER_AF_REJOIN_DUE_TO_CLI_COMMAND);
    }
    else
    {
      emberLeaveNetwork();
      
      emberAfResetAttributes(emberAfPrimaryEndpoint());
      emberAfGroupsClusterClearGroupTableCallback(emberAfPrimaryEndpoint());
      emberAfScenesClusterClearSceneTableCallback(emberAfPrimaryEndpoint());
      emberAfPluginBasicResetToFactoryDefaultsCallback(emberAfCurrentEndpoint());
      
      halReboot();
    }
    
    emberEventControlSetInactive(buttonEventControl);
  }
}

// Hal Button ISR Callback
// This callback is called by the framework whenever a button is pressed on the 
// device. This callback is called within ISR context.
void emberAfHalButtonIsrCallback(int8u button, int8u state) 
{
  buttonState = state;

  if (button != BUTTON0)
    return;

  if (state == BUTTON_PRESSED)
  {
    buttonTimer = 0;
    emberAfGuaranteedPrint("DOWN");  // check button 
  }
  else
  {
    emberAfGuaranteedPrint("UP");
  }

  if (FALSE == enableJoinNetworkFlag)
    emberEventControlSetActive(buttonEventControl);
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
  // add by Robin 2015/4/4  according to BugList:  C-M-150405-01
  emberClearBindingTable(); 
}

//*******************************************************************************
// Reporting 
//*******************************************************************************
EmberEventControl ActivePowerDelayEventControl;


void ActivePowerDelayEventFunction(void) 
{
  emberEventControlSetInactive(ActivePowerDelayEventControl);
}
//*******************************************************************************
// Join the network
//*******************************************************************************
static int16u  joinNetworkOverTime;
EmberEventControl JoinTheNetworkEventControl;

void emberAfPluginNetworkFindFinishedCallback(EmberStatus status)
{
  EmberNetworkStatus nStatus = emberAfNetworkState();    
  
  // If we go up or down, let the user know, although the down case shouldn't
  // happen.
  if ((nStatus == EMBER_JOINED_NETWORK) || (nStatus == EMBER_JOINED_NETWORK_NO_PARENT))
    return;
  
  emberEventControlSetDelayMS(JoinTheNetworkEventControl, 1000);  //1s
}
// Event function stubs
void JoinTheNetworkEventFunction(void) 
{
  EmberNetworkStatus status = emberAfNetworkState();  

  if (joinNetworkOverTime >= 56)
  {
    boolean isOn = 0;
    enableJoinNetworkFlag = FALSE;
    emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                             ZCL_ON_OFF_CLUSTER_ID,
                             ZCL_ON_OFF_ATTRIBUTE_ID,
                             (int8u *)&isOn,
                             ZCL_BOOLEAN_ATTRIBUTE_TYPE);
  }
  else
  {
    joinNetworkOverTime++;
    enableJoinNetworkFlag = TRUE;
    emberAfStartSearchForJoinableNetwork();
  }  

  emberEventControlSetInactive(JoinTheNetworkEventControl);
}
// JoinTheNetworkInit
void JoinTheNetworkInit(void)
{  
  joinNetworkOverTime = 0;
  enableJoinNetworkFlag = FALSE;

  //emberEventControlSetActive(JoinTheNetworkEventControl);
  emberEventControlSetDelayMS(JoinTheNetworkEventControl, 3000);  //3s
}
// emberAfStackStatusCallback
boolean emberAfStackStatusCallback(EmberStatus status)
{
  EmberNetworkStatus nStatus = emberAfNetworkState();

  if (nStatus == EMBER_NO_NETWORK)
  { joinedNetworkFlag = FALSE;}
  
  // If we go up or down, let the user know, although the down case shouldn't
  // happen.
  if ((status == EMBER_NETWORK_UP) || (nStatus == EMBER_JOINED_NETWORK) || (nStatus == EMBER_JOINED_NETWORK_NO_PARENT)) 
  {
    if (joinedNetworkFlag == TRUE)
    { return FALSE;} 
    joinedNetworkFlag = TRUE;
    
    enableJoinNetworkFlag = FALSE;
    emberEventControlSetInactive(JoinTheNetworkEventControl);
    
    if (joinNetworkOverTime == 0)   // in network, before power on
    {    

	  if (nStatus == EMBER_JOINED_NETWORK_NO_PARENT)
	  {
	    int8u currentLevel;
      	emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_LEVEL_CONTROL_CLUSTER_ID,
                              ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                              (int8u *)&currentLevel,
                              sizeof(int8u));
      
        SetLedDimming(currentLevel); 
	  }
	  else
	  {
	  //joinedNetworkFlag = TRUE;
      boolean isOn = 1;
      emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                             ZCL_ON_OFF_CLUSTER_ID,
                             ZCL_ON_OFF_ATTRIBUTE_ID,
                             (int8u *)&isOn,
                             ZCL_BOOLEAN_ATTRIBUTE_TYPE);
      int8u currentLevel;
      emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_LEVEL_CONTROL_CLUSTER_ID,
                              ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                              (int8u *)&currentLevel,
                              sizeof(int8u));
      emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                             ZCL_LEVEL_CONTROL_CLUSTER_ID,
                             ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                             (int8u *)&currentLevel,
                             ZCL_INT8U_ATTRIBUTE_TYPE);
      
      SetLedDimming(currentLevel);
	  }
    }
    else  // not in network, before power on
    {
      boolean isOn = 1;
      int8u currentLevel = DIMMING_MAX_LEVEL;
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
      
      emberEventControlSetDelayMS(ActivePowerDelayEventControl, 5000);   // 5s
    }
  } 
  else  if (status == EMBER_NETWORK_DOWN)
  {
    joinNetworkOverTime = 0;
    enableJoinNetworkFlag = FALSE;
    emberEventControlSetDelayMS(JoinTheNetworkEventControl, 3000);  //3s
  }
  
  return FALSE;
}
//JoinTheNetworkFunction
void JoinTheNetworkFunction(void)
{
  if (TRUE == enableJoinNetworkFlag)
  { 
    static int16u  oldTime = 0;  
    int16u newTime;
    
    static int8u level = 255;
    static boolean upDown = TRUE;

    SetLedDimming(level);
    
    newTime = halCommonGetInt16uMillisecondTick();
    if (elapsedTimeInt16u(oldTime, newTime) < 20)  // 1S
    { return;}

    oldTime = newTime;

    level = upDown?level-5:level+5;
    if (level == 5)
       upDown = FALSE;
    else if (level == 255)
      upDown = TRUE;
  }
}

//****************************************************************
//ADC
//****************************************************************
/*
#ifdef OFF_FORBID_CCT_CONTROL
#define MIN_VIN_RESULT_PWM 5600  
#define MAX_VIN_RESULT_PWM 8400  
#else
#define MIN_VIN_RESULT_PWM 9000 
#define MAX_VIN_RESULT_PWM 14000  
#endif
*/
#define MIN_VIN_RESULT_PWM 5600
#define MAX_VIN_RESULT_PWM 8400  

#define UNDER_POWER_TIMEOUT 10  
#define LOST_POWER_TIMEOUT  60  


static int8u adcChannelIndex;
const  int8u adcChannel[] = {ADC_SOURCE_ADC1_VREF2, ADC_SOURCE_ADC3_VREF2, ADC_SOURCE_ADC4_VREF2, ADC_SOURCE_ADC0_VREF2};
                            // ADC0 -ADC5: PB5,PB6,PB7,PC1,PA4,PA5

#define ADC_CHANNEL_NUM (sizeof(adcChannel)/sizeof(adcChannel[0]))

const int16u efficiency[] = {8330, 8392, 8133, 7763, 6880};

boolean underVoltageState = FALSE;

int64u powerConsumption;
EmberEventControl PowerConsumptionEventControl;


// AdcControlInit
void AdcControlInit(void)
{
  adcChannelIndex = 0;
  halStartAdcConversion(ADC_USER_APP2, 
                        ADC_REF_INT, 
                        adcChannel[adcChannelIndex],
                        ADC_CONVERSION_TIME_US_256);
    
  emberEventControlSetDelayMS(PowerConsumptionEventControl, 1000);
}
//
int8u GetEfficiencyPtr(int8u level)
{
  if (level <= 0x33)
    return 4;
  else if (level <= 0x66)
    return 3;
  else if (level <= 0x99)
    return 2;
  else if (level <= 0xcc)
    return 1;
  else
    return 0;
}
// Convert volts to celsius in LM20 temp sensor.  The numbers
// are both in fixed point 4 digit format. I.E. 860 is 0.0860
static int16s voltsToCelsius (int16u voltage)
{  
#if 0
  // equation: temp = -0.17079*mV + 159.1887
  //return 1591887L - (171 * (int32s)voltage);
  int32s tmp = (171 * (int32s)voltage);
  tmp /= 10000;
  
  return 159 - tmp;
#else
  int32s temp;

  voltage /= 10;

  // -40 ~ -16
  if (voltage < 40)
  {
    temp = (-158 * voltage * voltage + 15101 * voltage - 514290) / 10000; 
  }
  //-15 ~ 0
  else if (voltage < 91)
  { 
    temp = (-19 * voltage * voltage + 5579 * voltage - 347260) / 10000; 
  }
  // 0~30
  else if (voltage < 315)
  {   
    temp = (-2 * voltage * voltage + 2322 * voltage - 185860) / 10000; 
  }
  //31~ 60
  else if (voltage < 670)
  { 
    temp = (842 * voltage + 43373) / 10000; 
  }
  // 61 ~ 75
  else if (voltage < 830)
  { 
    temp = (913 * voltage - 7245) / 10000; 
  } 
  // 75 ~ 85
  else if (voltage < 920)
  { 
    temp = (1089 * voltage - 152600) / 10000; 
  } 
  // 86 ~ 100
  else if (voltage < 1030)
  { 
    temp = (2 * voltage * voltage - 2862 * voltage + 1800000) / 10000; 
  }
  // 101 ~ 110
  else if (voltage < 1082)
  { 
    temp = (1879 * voltage - 932270) / 10000; 
  }
  // 110 ~ 125
  else if (voltage < 1200)
  { 
    temp = (8 * voltage * voltage - 16165 * voltage + 9229100) / 10000; 
  }

  return temp;
#endif
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
void AdcControlFunction(void)
{
  static int16u value;
  static int16s fvolts;
  static int16s tmp[ADC_CHANNEL_NUM] = {0};
      
  if (halRequestAdcData(ADC_USER_APP2, &value) == EMBER_ADC_CONVERSION_DONE)
  {     
    fvolts = halConvertValueToVolts(value / TEMP_SENSOR_SCALE_FACTOR);
    tmp[adcChannelIndex] = GetFilteredValue(fvolts, adcChannelIndex);
    if (0 == adcChannelIndex)   // Vin
    {
      #if 1
      static int8u high_counter = 0;
      static int8u low_counter = 0;  
      static boolean isOn = 0;
      boolean OnorOff = 0;
      
      if(tmp[adcChannelIndex] >= MAX_VIN_RESULT_PWM)
      {
        low_counter = 0;
        high_counter++;
        if(high_counter == UNDER_POWER_TIMEOUT)
        {
          underVoltageState = FALSE;
          if(isOn == 1)
          {
            emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                                ZCL_ON_OFF_CLUSTER_ID,
                                ZCL_ON_OFF_ATTRIBUTE_ID,
                                (int8u *)&isOn,
                                ZCL_BOOLEAN_ATTRIBUTE_TYPE);
          }
        }
        else if (high_counter > UNDER_POWER_TIMEOUT)
        { high_counter = UNDER_POWER_TIMEOUT;}
      }

      if(tmp[adcChannelIndex] <= MIN_VIN_RESULT_PWM)
      {
        static int16u saveUnderPower;
        
        high_counter = 0;
        low_counter++;
        if(low_counter == UNDER_POWER_TIMEOUT)
        {
          saveUnderPower = tmp[adcChannelIndex];
          emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                             ZCL_ON_OFF_CLUSTER_ID,
                             ZCL_ON_OFF_ATTRIBUTE_ID,
                             (int8u *)&isOn,
                             ZCL_BOOLEAN_ATTRIBUTE_TYPE);
          
          OnorOff = 0;            
          underVoltageState = TRUE;
          emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                     ZCL_ON_OFF_CLUSTER_ID,
                     ZCL_ON_OFF_ATTRIBUTE_ID,
                     (int8u *)&OnorOff,
                     ZCL_BOOLEAN_ATTRIBUTE_TYPE);
        }
        else if (low_counter == LOST_POWER_TIMEOUT)
        {
          if (saveUnderPower > tmp[adcChannelIndex])
          { halReboot();}   
        }
        else if (low_counter > LOST_POWER_TIMEOUT)
        { 
          low_counter = LOST_POWER_TIMEOUT;
        }
      }
      #endif
    }
    else if (1 == adcChannelIndex)  // Iout
    {
      tmp[adcChannelIndex] = (tmp[adcChannelIndex]*10)/27;
      
    }
    else if (2 == adcChannelIndex)  // Vout
    {
      static int8u  currentLevel;
      static int16u oldTime = 0;  
      static int16u newTime;
      
      newTime = halCommonGetInt16uMillisecondTick();
      if (elapsedTimeInt16u(oldTime, newTime) > 1000)  //  1S
      { 
        oldTime = newTime;
        
        tmp[adcChannelIndex] = (int16s)(((int32s)tmp[adcChannelIndex]*3671)/10000);

        emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                                ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                (int8u *)&currentLevel,
                                sizeof(int8u));
        
        power = ((int32s)tmp[1]*tmp[2])/(efficiency[GetEfficiencyPtr(currentLevel)]*10);
      }
    }
    else if (3 == adcChannelIndex) //Temprature
    {
      tmp[adcChannelIndex] = voltsToCelsius(tmp[adcChannelIndex]);
      emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                     ZCL_DEVICE_TEMP_CLUSTER_ID,
                     ZCL_CURRENT_TEMPERATURE_ATTRIBUTE_ID,
                     (int8u *)&tmp[adcChannelIndex],
                     ZCL_INT16S_ATTRIBUTE_TYPE);
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

  if (firstFlag == TRUE)
  {
    firstFlag = FALSE;
    emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
                              (int8u *)&powerConsumption,
                              6); 
  }  

  shortPower += power;
  if (shortPower >= 3600)
  {
    shortPower -= 3600;
    powerConsumption++;
    emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
                              (int8u *)&powerConsumption,
                              ZCL_INT48U_ATTRIBUTE_TYPE);
    
  }    
    
  emberEventControlSetDelayMS(PowerConsumptionEventControl, 1000);
}


//*******************************************************************************
// Main
//*******************************************************************************
void emberAfMainInitCallback(void)
{
  JoinTheNetworkInit();
  AdcControlInit();
}
//
boolean emberAfMainStartCallback(int* returnCode,
                                 int argc,
                                 char** argv)
{
  int16u oldTime = halCommonGetInt16uMillisecondTick();  
  int16u newTime = oldTime;
  
  while(elapsedTimeInt16u(oldTime, newTime) < 200)
  {
    halResetWatchdog();   // Periodically reset the watchdog.
    newTime = halCommonGetInt16uMillisecondTick();
  }

  return FALSE;
}
//
void emberAfMainTickCallback(void)
{
  //*****************************************************
  // check in network
  //*****************************************************    
  JoinTheNetworkFunction();

  //*****************************************************
  // set power attribute
  //*****************************************************  
  AdcControlFunction();

  //*****************************************************
  // Diagnostics Cluster
  //*****************************************************  
  FunctionDiagnosticsCluster();
}





