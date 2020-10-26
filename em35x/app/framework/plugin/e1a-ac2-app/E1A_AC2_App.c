
#include "E1A_AC2_Public_Head.h"
#include "E1A_AC2_App.h"


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

//*******************************************************************************
// Main
//*******************************************************************************
void emberAfMainInitCallback(void)
{
  GetTheApsNumType();
  JoinTheNetworkInit();
  PowerAndConsumptionInit();
  ButtonInit();
  emberEventControlSetDelayQS(diagnosticsEventControl, 240);  //60s
}
//
boolean emberAfMainStartCallback(int* returnCode,
                                 int argc,
                                 char** argv)
{
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





