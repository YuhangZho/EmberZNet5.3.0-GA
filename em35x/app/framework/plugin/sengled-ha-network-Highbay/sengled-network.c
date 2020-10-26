// *******************************************************************
// * sengled-network.c
// *
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *******************************************************************
#include "app/framework/include/af.h"
#include "app/framework/plugin/sengled-hardware-Highbay/sengled-hardware-Highbay.h"
#include "sengled-network.h"
#include "app/framework/plugin/sengled-hardware-Highbay/sengled-ledControl.h"
#include "app/framework/plugin/sengled-ha-common-Highbay/sengled-ha-token.h"

boolean searchingNetworkStatus = NETWORK_SEARCH_STOP;
boolean rejoinHappened = 0;
int8u joinNetworkFirstTimeFlag;

extern int8u blinkTimes;
extern EmberEventControl  networkOperationEventControl;

EmberEventControl searchNetworkTimeControl;

void SearchNetworkTimeFunction(void)
{
  emberEventControlSetInactive(searchNetworkTimeControl);
  searchingNetworkStatus = NETWORK_SEARCH_STOP;
}

// we need to put JoinNetworkRequest after EM_AF_NETWORK_INIT();
// for EM_AF_NETWORK_INIT is used for initial network;
void JoinNetworkRequest(void)
{
  EmberNetworkStatus status = emberAfNetworkState(); 
  EmberNodeId nodeId = emberAfGetNodeId();
  EmberPanId panId = emberAfGetPanId();
#ifdef ROBIN_DEBUG
  emberAfGuaranteedPrintln("nodeID: %2x, panID: %2x", nodeId, panId);  // Robin add for debug
#endif

  // if the node is not in any network when power on, start to search for a joinable network. 
  if((nodeId == 0xFFFE) || (panId == 0xFFFF))
  {
#ifdef ROBIN_DEBUG
    emberAfGuaranteedPrintln("StartSearch:");  // Robin add for debug
#endif
    //Timer start:
    searchingNetworkStatus = NETWORK_SEARCHING;
    emberEventControlSetDelayQS(searchNetworkTimeControl,NETWORK_SEARCH_TIME_UPPER_BOUND);
    //join network:
    emberAfStartSearchForJoinableNetwork();
  }
}

void emberAfPluginNetworkFindFinishedCallback(EmberStatus status)
{
  EmberNetworkStatus nStatus = emberAfNetworkState();    
  
  if ((nStatus == EMBER_JOINED_NETWORK) || (nStatus == EMBER_JOINED_NETWORK_NO_PARENT))
  {
    emberEventControlSetActive(searchNetworkTimeControl);
#ifdef ROBIN_DEBUG
	emberAfGuaranteedPrintln("Joined:");  // Robin add for debug
#endif
  }
  else
  {
    if(searchingNetworkStatus == NETWORK_SEARCHING)
    {
      emberAfStartSearchForJoinableNetwork();
    }
  }
}

boolean emberAfStackStatusCallback(EmberStatus status)
{
  EmberNetworkStatus nStatus = emberAfNetworkState(); 
#ifdef ROBIN_DEBUG  
  emberAfGuaranteedPrintln("network status: %x %x", nStatus, status); // Robin add for debug
#endif
  
  if(status == EMBER_NETWORK_UP)
  {
    switch(nStatus)
    {
      // joined  
      case EMBER_JOINED_NETWORK:
	  {
        if(rejoinHappened == 1)
        {
          // be in the network again through rejoin process;
          rejoinHappened = 0;
          RecoverLedRecordedStatus();  
		}
		else
		{
		  //joined network;		  
		  halCommonGetToken(&joinNetworkFirstTimeFlag, TOKEN_THE_FIRST_JOINED_FLAG);
		  if(joinNetworkFirstTimeFlag == 0x00)
		  { 
		  	int8u currentLevel=DIMMING_MAX_LEVEL;
			int16u currentColorTemperature = COLOR_YELLOWEST;
			
		    joinNetworkFirstTimeFlag = 0x01;
			halCommonSetToken(TOKEN_THE_FIRST_JOINED_FLAG, &joinNetworkFirstTimeFlag);
			InitLedStatus(TRUE); 
#ifdef ROBIN_DEBUG  
			emberAfGuaranteedPrintln("joinNetworkFirstTimeFlag == 0x00,%d,%2d",currentLevel,currentColorTemperature); // Robin add for debug
#endif

    		emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_LEVEL_CONTROL_CLUSTER_ID,
                              ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                              (int8u *)&currentLevel,
                              ZCL_INT8U_ATTRIBUTE_TYPE); 

			// disable onLevel when joined network; unless get a command to enable it; 
			emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
							   ZCL_LEVEL_CONTROL_CLUSTER_ID,
							   ZCL_ON_LEVEL_ATTRIBUTE_ID,
							   (int8u *)&currentLevel,
							   ZCL_INT8U_ATTRIBUTE_TYPE);


	  		emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
	                      		ZCL_COLOR_CONTROL_CLUSTER_ID,
	                      		ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
	                      		(int8u *)&currentColorTemperature,
	                      		ZCL_INT16U_ATTRIBUTE_TYPE);	
			
			blinkTimes = NETWORK_JOINED_BLINK; 
			emberEventControlSetActive(networkOperationEventControl);
		  }
		  else
		  {
		    // because it's not the first time to join the network, 
		    // only recover the light and color 
#ifdef ROBIN_DEBUG  
			emberAfGuaranteedPrintln("joinNetworkFirstTimeFlag == 1"); // Robin add for debug
#endif
            InitLedStatus(FALSE); 
		  }
		}
	  }
	    break;

      case EMBER_JOINING_NETWORK:
	  {
	  }
	    break;
		
      case EMBER_JOINED_NETWORK_NO_PARENT:
      {
      }
	    break;
      default:
		break;
	}
  }
  else if(status == EMBER_NETWORK_DOWN)
  {
    // leave the network;
    if(nStatus == EMBER_NO_NETWORK)
    {
      emberClearBindingTable();	  
	  joinNetworkFirstTimeFlag = 0x00;
	  halCommonSetToken(TOKEN_THE_FIRST_JOINED_FLAG, &joinNetworkFirstTimeFlag);
      JoinNetworkRequest();
    }
	else if(nStatus == EMBER_JOINED_NETWORK_NO_PARENT)
	{
	  // rejoin first happened;
      rejoinHappened = 1;  
	  emberFindAndRejoinNetwork(0,  // 1=rejoin with encryption, 0=rejoin without encryption
                                EMBER_ALL_802_15_4_CHANNELS_MASK); // end device rejoin 
	}
  }
  
  return FALSE;
}


