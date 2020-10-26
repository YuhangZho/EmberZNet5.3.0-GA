
#include "E1A_AC2_Public_Head.h"



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



