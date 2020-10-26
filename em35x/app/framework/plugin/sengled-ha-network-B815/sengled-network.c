/*
************************************************************************************************************************
*                                                      BUSINESS LIGHT
*                                                     TrackLight | E1E-CEA
*
*                                  (c) Copyright 2017-**; Sengled, Inc.; 
*                           All rights reserved.  Protected by international copyright laws.
*
*
* File    : sengled-network.c
* Path   : app/framework/plugin/sengled-ha-network-TrackLight
* By      : ROBIN
* Version : 0x00000001
*
* Description:
* ---------------
* Function
* (1) zigbee network status callback:
*           a. emberAfStackStatusCallback     
*           b. emberAfPluginNetworkFindFinishedCallback
* 
* (2) find network :
*           a. join_network_request
*           b. SearchNetworkTimeFunction
*
* History:
* ---------------
*
*
*
************************************************************************************************************************
*/

#include "app/framework/include/af.h"
#include "app/framework/plugin/sengled-hardware-B815/sengled-hardware-TrackLight.h"
#include "sengled-network.h"
#include "app/framework/plugin/sengled-hardware-B815/sengled-ledControl.h"
#include "app/framework/plugin/sengled-ha-common-B815/sengled-ha-token.h"
#include "app/commonFunction/light_common.h"
#include "app/commonFunction/light_token.h"

boolean searchingNetworkStatus   = NETWORK_SEARCH_STOP;
boolean rejoinHappened           = 0;
int8u   joinNetworkFirstTimeFlag;

boolean autoResetMarkWriteWhenJoinedZC = FALSE;

extern  int8u             blinkTimes;
extern  EmberEventControl networkOperationEventControl;

EmberEventControl searchNetworkTimeControl;

void SearchNetworkTimeFunction(void)
{
	emberEventControlSetInactive(searchNetworkTimeControl);
	searchingNetworkStatus = NETWORK_SEARCH_STOP;
}

// we need to put join_network_request after EM_AF_NETWORK_INIT();
// for EM_AF_NETWORK_INIT is used for initial network;
void join_network_request(void)
{
	EmberNetworkStatus status = emberAfNetworkState(); 
	EmberNodeId nodeId = emberAfGetNodeId();
	EmberPanId panId = emberAfGetPanId();
#ifdef ROBIN_DEBUG
	emberAfGuaranteedPrintln("nodeID: %2x, panID: %2x", nodeId, panId);  // Robin add for debug
#endif

	// if the node is not in any network when power on, start to search for a joinable network. 
	if((nodeId == 0xFFFE) || (panId == 0xFFFF)) {
#ifdef ROBIN_DEBUG
		emberAfGuaranteedPrintln("StartSearch:");  // Robin add for debug
#endif

		searchingNetworkStatus = NETWORK_SEARCHING;
		//emberEventControlSetDelayQS(searchNetworkTimeControl,NETWORK_SEARCH_TIME_UPPER_BOUND);
		emberEventControlSetDelayQS(searchNetworkTimeControl, NETWORK_SEARCH_TIME_UPPER_BOUND);
		emberAfStartSearchForJoinableNetwork();
	}
}

void emberAfPluginNetworkFindFinishedCallback(EmberStatus status)
{
	EmberNetworkStatus nStatus = emberAfNetworkState();    
  
	if ((nStatus == EMBER_JOINED_NETWORK) || (nStatus == EMBER_JOINED_NETWORK_NO_PARENT)) {
		emberEventControlSetActive(searchNetworkTimeControl);
#ifdef ROBIN_DEBUG
		emberAfGuaranteedPrintln("Joined:");  // Robin add for debug
#endif
	} else {
		if(searchingNetworkStatus == NETWORK_SEARCHING) {
			emberAfStartSearchForJoinableNetwork();
		}
	}
}

boolean emberAfStackStatusCallback(EmberStatus status)
{
	EmberNetworkStatus nStatus = emberAfNetworkState(); 
	int8u autoResetMark;
#ifdef ROBIN_DEBUG  
	emberAfGuaranteedPrintln("network status: %x %x", nStatus, status); // Robin add for debug
#endif
  
	if(status == EMBER_NETWORK_UP) {
    	switch(nStatus) {
		
		case EMBER_JOINED_NETWORK:  // joined  
		{
			if(rejoinHappened == 1) {
				// be in the network again through rejoin process;
				rejoinHappened = 0;
			} else {
				//joined network;		  
				halCommonGetToken(&joinNetworkFirstTimeFlag, TOKEN_THE_FIRST_JOINED_FLAG);
				if(joinNetworkFirstTimeFlag == 0x00) { 
					int8u currentLevel=DIMMING_MAX_LEVEL;
					int16u currentColorTemperature = COLOR_YELLOWEST;
					
					int8u autoResetMark = ONLY_ZC_MARKED;
					// to avoid another jump here, write autoResetMark should be only once;
					if (autoResetMarkWriteWhenJoinedZC == FALSE)
					{
						emberAfWriteManufacturerSpecificServerAttribute(emberAfPrimaryEndpoint(),
																	ZCL_AUTO_RESET_CLUSTER_ID,
																	ZCL_AUTO_RESET_ATTRIBUTE_ID,
																	0x1160,
																	(int8u *)&autoResetMark,
																	ZCL_INT8U_ATTRIBUTE_TYPE);
						autoResetMarkWriteWhenJoinedZC = TRUE;
					}		
			
					joinNetworkFirstTimeFlag = 0x01;
					halCommonSetToken(TOKEN_THE_FIRST_JOINED_FLAG, &joinNetworkFirstTimeFlag);
					init_led_status(TRUE); 
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
				} else {
					// because it's not the first time to join the network, 
					// only recover the light and color 
#ifdef ROBIN_DEBUG  
					emberAfGuaranteedPrintln("joinNetworkFirstTimeFlag == 1"); // Robin add for debug
#endif
					init_led_status(FALSE); 
				}
			}
		}
		break;

		case EMBER_JOINING_NETWORK:
	    break;
		
		case EMBER_JOINED_NETWORK_NO_PARENT:
	    break;
		
		default:
		break;
		}
	}
  	else if(status == EMBER_NETWORK_DOWN) {
		// leave the network;
		if(nStatus == EMBER_NO_NETWORK) {
			emberClearBindingTable();	  
			joinNetworkFirstTimeFlag = 0x00;
			halCommonSetToken(TOKEN_THE_FIRST_JOINED_FLAG, &joinNetworkFirstTimeFlag);

			autoResetMark = NO_MARK;
			emberAfWriteManufacturerSpecificServerAttribute(emberAfPrimaryEndpoint(),
															ZCL_AUTO_RESET_CLUSTER_ID,
															ZCL_AUTO_RESET_ATTRIBUTE_ID,
															0x1160,
															(int8u *)&autoResetMark,
															ZCL_INT8U_ATTRIBUTE_TYPE);
		
			join_network_request();
		} else if(nStatus == EMBER_JOINED_NETWORK_NO_PARENT) {
			// rejoin first happened;
			rejoinHappened = 1;  
			emberFindAndRejoinNetwork(0,  // 1=rejoin with encryption, 0=rejoin without encryption
									EMBER_ALL_802_15_4_CHANNELS_MASK); // end device rejoin 
		}
	}
	return FALSE;
}


