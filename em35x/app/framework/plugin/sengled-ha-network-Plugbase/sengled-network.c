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
#include "app/framework/plugin/sengled-hardware-Plugbase/sengled-hardware-Plugbase.h"
#include "sengled-network.h"
#include "app/framework/plugin/sengled-hardware-Plugbase/sengled-ledControl.h"
#include "app/framework/plugin/sengled-ha-common-Plugbase/sengled-ha-token.h"


boolean searchingNetworkStatus   = NETWORK_SEARCH_STOP;
boolean rejoinHappened           = 0;
int8u   joinNetworkFirstTimeFlag;
boolean stopBlinkForeverEvent = FALSE; 
extern int8u on_off_remember_when_power_off;

EmberEventControl searchNetworkTimeControl;

void SearchNetworkTimeFunction(void)
{
	emberEventControlSetInactive(searchNetworkTimeControl);
	searchingNetworkStatus = NETWORK_SEARCH_STOP;
	// set true to stop led blink
	stopBlinkForeverEvent = TRUE;
	// only enable push-button
	joinNetworkFirstTimeFlag = JOINED_NETWORK;
	
	setLedPulseMode(M4); 
}

// we need to put join_network_request after EM_AF_NETWORK_INIT()!!!
// for EM_AF_NETWORK_INIT is used for initial network;
int16u join_network_request(void)
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
	return panId;
}

void emberAfPluginNetworkFindFinishedCallback(EmberStatus status)
{

}

static void configOnOffClusterForZigbeeCertified(void)
{
	EmberAfPluginReportingEntry reportingEntry;
	reportingEntry.direction = EMBER_ZCL_REPORTING_DIRECTION_REPORTED;
	reportingEntry.endpoint = emberAfPrimaryEndpoint();
	reportingEntry.clusterId = ZCL_ON_OFF_CLUSTER_ID;
	reportingEntry.attributeId = ZCL_ON_OFF_ATTRIBUTE_ID;
	reportingEntry.mask = CLUSTER_MASK_SERVER;
	reportingEntry.manufacturerCode = EMBER_AF_NULL_MANUFACTURER_CODE;
	reportingEntry.data.reported.minInterval = 0x3d;
	reportingEntry.data.reported.maxInterval = 0xfffe;
	reportingEntry.data.reported.reportableChange = 0; // unused
	emberAfPluginReportingConfigureReportedAttribute(&reportingEntry);
}

boolean emberAfStackStatusCallback(EmberStatus status)
{
	EmberNetworkStatus nStatus = emberAfNetworkState(); 
	
#ifdef ROBIN_DEBUG  
	emberAfGuaranteedPrintln("network status: %x %x", nStatus, status); // Robin add for debug
#endif
  
	if(status == EMBER_NETWORK_UP) {
    	switch(nStatus) {
		
		case EMBER_JOINED_NETWORK:  // joined  
		{
			stopBlinkForeverEvent = TRUE; 
			
			if(rejoinHappened == 1) {
				// be in the network again through rejoin process;
				rejoinHappened = 0;
				setLedPulseMode(M3); 
			} else {
				//joined network;		  
				halCommonGetToken(&joinNetworkFirstTimeFlag, TOKEN_THE_FIRST_JOINED_FLAG);				

				if(joinNetworkFirstTimeFlag == SEARCH_NETWORK) { 
					joinNetworkFirstTimeFlag = JOINED_NETWORK;
					halCommonSetToken(TOKEN_THE_FIRST_JOINED_FLAG, &joinNetworkFirstTimeFlag);
					
#ifdef ROBIN_DEBUG  
					emberAfGuaranteedPrintln("joinNetworkFirstTimeFlag is JOINED_NETWORK"); // Robin add for debug
#endif
					configOnOffClusterForZigbeeCertified();

					setLedPulseMode(M2); 								
				} else {
					// because it's not the first time to join the network, 
					// only recover the light and color 
					if(on_off_remember_when_power_off == 1)
					{
						setLedPulseMode(M3); 
					}
#ifdef ROBIN_DEBUG  
					emberAfGuaranteedPrintln("joinNetworkFirstTimeFlag == 1"); // Robin add for debug
#endif
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
			joinNetworkFirstTimeFlag = SEARCH_NETWORK;
			halCommonSetToken(TOKEN_THE_FIRST_JOINED_FLAG, &joinNetworkFirstTimeFlag);
			
			on_off_remember_when_power_off = 1;
			halCommonSetToken(TOKEN_ON_OFF_STATUS, &on_off_remember_when_power_off);

			emberAfResetAttributes(emberAfPrimaryEndpoint());
			emberAfGroupsClusterClearGroupTableCallback(emberAfPrimaryEndpoint());
			emberAfScenesClusterClearSceneTableCallback(emberAfPrimaryEndpoint());
			emberClearBindingTable();
			emberAfClearReportTableCallback();
			halReboot();	

			//stopBlinkForeverEvent = FALSE;	
		} else if(nStatus == EMBER_JOINED_NETWORK_NO_PARENT) {
			// rejoin first happened;
			rejoinHappened = 1;  
			emberFindAndRejoinNetwork(1,  // 1=rejoin with encryption, 0=rejoin without encryption
									EMBER_ALL_802_15_4_CHANNELS_MASK); // end device rejoin 
		}
	}
	return FALSE;
}


