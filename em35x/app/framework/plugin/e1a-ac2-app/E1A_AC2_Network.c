
#include "E1A_AC2_Public_Head.h"


//*******************************************************************************
// Join the network
//*******************************************************************************
EmberEventControl JoinTheNetworkEventControl;
static int16u  joinNetworkOverTime;
static boolean firstJoinNetworkFlag = TRUE;


// emberAfStackStatusCallback
boolean emberAfStackStatusCallback(EmberStatus status)
{
  EmberNetworkStatus nStatus = emberAfNetworkState();
  
  if ((nStatus == EMBER_JOINED_NETWORK) 
      || (nStatus == EMBER_JOINED_NETWORK_NO_PARENT)) 
  {
    boolean isOn;
    int8u currentLevel;
      
    if (0 != joinNetworkOverTime)
    {
      isOn = 1;
      currentLevel = 0xff;

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
    }

    if (nStatus == EMBER_JOINED_NETWORK)
    {
      joinNetworkOverTime = 0;
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
//
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
  if (joinNetworkOverTime <= NETWORK_SEARCH_TIME_UPPER_BOUND)
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

