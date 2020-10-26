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
#include "app/framework/plugin/sengled-hardware-Plugbase/sengled-ledControl.h"
#include "app/framework/plugin/sengled-ha-common-plugbase/sengled-ha-common.h"

int8u ezmode_led_blink_times = 0; // means blink 3 times

EmberEventControl ezmodeEventControl;

void EzmodeEventFunction(void)
{
	boolean isOn;
	emberEventControlSetInactive(ezmodeEventControl);

	sengledGuaranteedPrintln("Ezmodeezmode_led_blink_times:%d ", ezmode_led_blink_times);

  	if(ezmode_led_blink_times > 0)
  	{
  		if((ezmode_led_blink_times % 2) == 0)
  		{
  			// off	  
			sengledGuaranteedPrintln("Ezmode-OFF");
			TIM1_CCR3 = 0;//emberAfPwmSetValuePA6(0);
		}
		else
		{
			// on	
			sengledGuaranteedPrintln("Ezmode-ON");	
			TIM1_CCR3 = TICS_PER_PERIOD;//emberAfPwmSetValuePA6(TICS_PER_PERIOD);
		}
		ezmode_led_blink_times--;	
		emberEventControlSetDelayMS(ezmodeEventControl, BLINK_CYCLE); 
  	}	
	else // ezmode_led_blink_times ==0
	{
		emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
									ZCL_ON_OFF_CLUSTER_ID,
									ZCL_ON_OFF_ATTRIBUTE_ID,
									(int8u *)&isOn,
									sizeof(boolean));
		if(isOn == 0)
		{
			TIM1_CCR3 = 0;//emberAfPwmSetValuePA6(0);
		}
		else
		{
			TIM1_CCR3 = TICS_PER_PERIOD;//emberAfPwmSetValuePA6(TICS_PER_PERIOD);
		}
	}
}

void emberAfPluginIdentifyStartFeedbackCallback(int8u endpoint, int16u identifyTime)
{
	int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_IDENTIFY_CLUSTER_ID);

	if (ep == 0xFF) {
		return;
	}
	
	sengledGuaranteedPrintln("Ezmode start");
	
	ezmode_led_blink_times = 6; // means blink 3 times
	
	emberEventControlSetDelayMS(ezmodeEventControl, BLINK_CYCLE);
}

void emberAfPluginIdentifyStopFeedbackCallback(int8u endpoint)
{
	int8u ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_IDENTIFY_CLUSTER_ID);
	boolean isOn; 
  
	if (ep == 0xFF) {
		return;
	}
	
	ezmode_led_blink_times = 0;

 
	emberAfReadServerAttribute(endpoint,
								ZCL_ON_OFF_CLUSTER_ID,
								ZCL_ON_OFF_ATTRIBUTE_ID,
								(int8u *)&isOn,
								sizeof(boolean));

	sengledGuaranteedPrintln("Ezmode stop- OnOrOff: %x",isOn);	
	emberAfWriteServerAttribute(endpoint,
								ZCL_ON_OFF_CLUSTER_ID,
								ZCL_ON_OFF_ATTRIBUTE_ID,
								(int8u *)&isOn,
								ZCL_BOOLEAN_ATTRIBUTE_TYPE);  
	
	emberEventControlSetInactive(ezmodeEventControl);
}

	

