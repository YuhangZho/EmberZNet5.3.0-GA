/*
************************************************************************************************************************
*                                                      **COMMON USE**
*                                                     TrackLight | A19EU
*
*                                  (c) Copyright 2017-**; Sengled, Inc.; 
*                           All rights reserved.  Protected by international copyright laws.
*
*
* File    : sengled-ha-ezmode.c
* Path   : app/framework/plugin/sengled-ha-ezmode
* By      : ROBIN
* Version : 0x00000001
*
* Description:
* ---------------
* Function
* (1) EzMode fucntion
*           a. EzmodeEventFunction     
*           b. emberAfPluginIdentifyStartFeedbackCallback
*           c. emberAfPluginIdentifyStopFeedbackCallback
*
* History:
* ---------------
*
*
*
************************************************************************************************************************
*/

#include "sengled-ha-ezmode.h"
#include "app/framework/plugin/sengled-ha-common/sengled-ledControl.h"

EmberEventControl ezmodeEventControl;

void EzmodeEventFunction(void)
{
	static boolean flag = FALSE;
  
	flag = flag?FALSE:TRUE;
	if(flag == TRUE) {
		// on   
#ifdef ROBIN_DEBUG
		emberAfGuaranteedPrintln("Ezmode-ON"); // Robin add for debug
#endif	
		led_control(LED_ON, DIMMING_MAX_LEVEL, COLOR_YELLOWEST);  
	} else {
		// off    
#ifdef ROBIN_DEBUG
		emberAfGuaranteedPrintln("Ezmode-Off"); // Robin add for debug
#endif	
		led_control(LED_OFF, DIMMING_MAX_LEVEL, COLOR_YELLOWEST);        
 	}
	emberEventControlSetDelayMS(ezmodeEventControl, BLINK_CYCLE); 
}

void emberAfPluginIdentifyStartFeedbackCallback(int8u endpoint, int16u identifyTime)
{
	int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_IDENTIFY_CLUSTER_ID);

	if (ep == 0xFF) {
		return;
	}
#ifdef ROBIN_DEBUG	
	emberAfGuaranteedPrintln("Ezmode start"); // Robin add for debug
#endif
	emberEventControlSetDelayMS(ezmodeEventControl, BLINK_CYCLE);
}

void emberAfPluginIdentifyStopFeedbackCallback(int8u endpoint)
{
	int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_IDENTIFY_CLUSTER_ID);
	boolean isOn; 
  
	if (ep == 0xFF) {
		return;
	}
#ifdef ROBIN_DEBUG	
	emberAfGuaranteedPrintln("Ezmode stop"); // Robin add for debug 
#endif
    
	emberAfReadServerAttribute(endpoint,
								ZCL_ON_OFF_CLUSTER_ID,
								ZCL_ON_OFF_ATTRIBUTE_ID,
								(int8u *)&isOn,
								sizeof(boolean));
	emberAfWriteServerAttribute(endpoint,
								ZCL_ON_OFF_CLUSTER_ID,
								ZCL_ON_OFF_ATTRIBUTE_ID,
								(int8u *)&isOn,
								ZCL_BOOLEAN_ATTRIBUTE_TYPE);  
	
	emberAfLevelControlClusterServerAttributeChangedCallback(endpoint, ZCL_CURRENT_LEVEL_ATTRIBUTE_ID);
    //emberAfColorControlClusterServerAttributeChangedCallback(endpoint, ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID);
	emberEventControlSetInactive(ezmodeEventControl);
}

	

