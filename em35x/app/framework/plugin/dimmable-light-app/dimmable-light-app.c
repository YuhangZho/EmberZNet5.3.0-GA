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


//****************************************************************
//Adjust the brightness and color temperature
//****************************************************************
#define DIMMING_MAX_LEVEL  255
#define DIMMING_MIN_LEVEL  0

#define DIMMING_CTRL_PIN PIN_PA6

#define COLOR_TEMP_MAX_LEVEL  100
#define COLOR_TEMP_MIN_LEVEL  0

#define COLOR_TEMP_CTRL_PIN PIN_PB7


//*******************************************************************************
// LevelControl Cluster
//*******************************************************************************
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
                              sizeof(currentLevel));

      SetPwmLevel(currentLevel, DIMMING_MAX_LEVEL, DIMMING_CTRL_PIN);
    }
  }
}
//*******************************************************************************
// OnOff Cluster
//*******************************************************************************
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
                              sizeof(currentLevel));

      SetPwmLevel(currentLevel, DIMMING_MAX_LEVEL, DIMMING_CTRL_PIN);
    }
    else
    {
      SetPwmLevel(DIMMING_MIN_LEVEL, DIMMING_MAX_LEVEL, DIMMING_CTRL_PIN);
    }
  }
}
//*******************************************************************************
// Color Temprature Control Cluster
//*******************************************************************************
void emberAfSengledClusterServerManufacturerSpecificAttributeChangedCallback(int8u endpoint,
                                                                                            EmberAfAttributeId attributeId,
                                                                                            int16u manufacturerCode)
{
  if ((ZCL_CURRENT_COLOR_TEMPRATURE_ATTRIBUTE_ID == attributeId)
      && (0x1901 == manufacturerCode))
  {
    int8u currentLevel;

    emberAfReadManufacturerSpecificServerAttribute(endpoint,
                                      ZCL_SENGLED_CLUSTER_ID,
                                      ZCL_CURRENT_COLOR_TEMPRATURE_ATTRIBUTE_ID,
                                      0x1901,
                                      (int8u *)&currentLevel,
                                      sizeof(currentLevel));
        
    SetPfmLevel(currentLevel*6+126);
  }
}
//****************************************************************
//identify
//****************************************************************
#define BLINK_CYCLE  1000

EmberEventControl emberAfPluginIdentifyFeedbackProvideFeedbackEventControl;

void emberAfPluginIdentifyFeedbackProvideFeedbackEventHandler(void);

//*******************************************************************************
// Identify Cluster
//*******************************************************************************
void emberAfPluginIdentifyFeedbackProvideFeedbackEventHandler(void)
{
  static boolean flag = FALSE;
  
  flag = flag?FALSE:TRUE;
  SetPwmLevel(flag?DIMMING_MAX_LEVEL:DIMMING_MIN_LEVEL, DIMMING_MAX_LEVEL, DIMMING_CTRL_PIN);
  
  emberEventControlSetDelayMS(emberAfPluginIdentifyFeedbackProvideFeedbackEventControl, BLINK_CYCLE);
}

void emberAfPluginIdentifyStartFeedbackCallback(int8u endpoint, int16u identifyTime)
{
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_IDENTIFY_CLUSTER_ID);

  if (ep == 0xFF) 
  { return;}
  
  emberEventControlSetDelayMS(emberAfPluginIdentifyFeedbackProvideFeedbackEventControl,
                              BLINK_CYCLE);
}

void emberAfPluginIdentifyStopFeedbackCallback(int8u endpoint)
{
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_IDENTIFY_CLUSTER_ID);
  
  if (ep == 0xFF) 
  { return;}

  emberAfLevelControlClusterServerAttributeChangedCallback(1, ZCL_CURRENT_LEVEL_ATTRIBUTE_ID);
  
  emberEventControlSetInactive(emberAfPluginIdentifyFeedbackProvideFeedbackEventControl);
}

//****************************************************************
//ADC
//****************************************************************
static int8u adcChannelIndex;
const  int8u adcChannel[] = {ADC_SOURCE_ADC0_VREF2, ADC_SOURCE_ADC3_VREF2, ADC_SOURCE_ADC4_VREF2};
                            // ADC0 -ADC5: PB5,PB6,PB7,PC1,PA4,PA5
static boolean networkFindFinishedFlag = TRUE;
static int16u  threeMinutesTime;


void emberAfPluginNetworkFindFinishedCallback(EmberStatus status)
{
  networkFindFinishedFlag = TRUE;
}

//*******************************************************************************
// Main
//*******************************************************************************
void emberAfMainInitCallback(void)
{
  threeMinutesTime = halCommonGetInt16uQuarterSecondTick();
  
  // ADC select channel
  adcChannelIndex = 0;
#if 1
  halStartAdcConversion(ADC_USER_APP2, 
                        ADC_REF_INT, 
                        ADC_SOURCE_ADC3_VREF2,
                        ADC_CONVERSION_TIME_US_256);
#else
  halStartAdcConversion(ADC_USER_APP2, 
                        ADC_REF_INT, 
                        adcChannel[adcChannelIndex],
                        ADC_CONVERSION_TIME_US_256);
#endif
}

/** @brief Main Tick
 *
 * Whenever main application tick is called, this callback will be called at
 * the end of the main tick execution.
 *
 */
void emberAfMainTickCallback(void)
{
  static boolean flag = FALSE;
  static int16u  oldTime = 0;  
  int16u newTime;

  int16u data;
  int16s fvolts;

  //*****************************************************
  // check in network
  //*****************************************************    
  if (FALSE == flag)
  {    
    if (TRUE == networkFindFinishedFlag)
    {
      EmberNetworkStatus status = emberAfNetworkState();
      
      newTime = halCommonGetInt16uQuarterSecondTick();
      if (elapsedTimeInt16u(oldTime, newTime) < 12)  // 3S
      { return;}

      oldTime = newTime;    
      
      if (status == EMBER_NO_NETWORK)
      {
        networkFindFinishedFlag = FALSE;
        emberAfStartSearchForJoinableNetwork();
      }
      else if (status == EMBER_JOINED_NETWORK)
      {
        flag = TRUE;
      }

      if (elapsedTimeInt16u(threeMinutesTime, newTime) > 540)  // 3 Minutes
      {
        flag = TRUE;
      }
    }
  }
  
  //*****************************************************
  // set power attribute
  //*****************************************************  
  if (halRequestAdcData(ADC_USER_APP2, &data) == EMBER_ADC_CONVERSION_DONE){ 
    fvolts = halConvertValueToVolts(data / TEMP_SENSOR_SCALE_FACTOR);
    emberAfWriteManufacturerSpecificServerAttribute(1,
                                     ZCL_SENGLED_CLUSTER_ID,
                                     ZCL_SENGLED_DC_POWER_ATTRIBUTE_ID,
                                     0x1901,
                                     (int8u *)&fvolts,
                                     sizeof(int16s));      
    adcChannelIndex++;
    if (adcChannelIndex >= (sizeof(adcChannel)/sizeof(adcChannel[0])))
    { adcChannelIndex = 0;}
    #if 1
    halStartAdcConversion(ADC_USER_APP2, 
                          ADC_REF_INT, 
                          ADC_SOURCE_ADC3_VREF2,
                          ADC_CONVERSION_TIME_US_256);
    #else
    halStartAdcConversion(ADC_USER_APP2, 
                          ADC_REF_INT, 
                          adcChannel[adcChannelIndex],
                          ADC_CONVERSION_TIME_US_256);
    #endif
  }
}





