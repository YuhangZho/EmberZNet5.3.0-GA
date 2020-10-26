//

// This callback file is created for your convenience. You may add application
// code to this file. If you regenerate this file over a previous version, the
// previous version will be overwritten and any code you have added will be
// lost.

#include "app/framework/include/af.h"

#define SENGLED_GENENAL_ENDPOINT 1   // sengled general endpoint is 0x01, only one
#define MOTION_DETECTED        0x0021
#define MOTION_NOT_DETECTED    0x0020
EmberEventControl auto_test_motion_event_control;

boolean motion_status_toggle = 0;
boolean first_power_up = 1;
void auto_test_motion_event_function(void)
{
	emberEventControlSetInactive(auto_test_motion_event_control);
	if(first_power_up == 1)
	{
		emberAfGuaranteedPrintln("enrollWithClient"); // Robin add for debug
		int8u ieeeAddress[] = { 0x00, 0x00, 0xC2, 0x12, 0x18, 0x18, 0xCE, 0xB0 };
  		emberAfWriteAttribute(0x01,
                        ZCL_IAS_ZONE_CLUSTER_ID,
                        ZCL_IAS_CIE_ADDRESS_ATTRIBUTE_ID,
                        CLUSTER_MASK_SERVER,
                        (int8u*)ieeeAddress,
                        ZCL_IEEE_ADDRESS_ATTRIBUTE_TYPE);
		emberAfIasZoneClusterZoneEnrollResponseCallback(0x00, 0x01);
		first_power_up = 0;
		emberEventControlSetDelayQS(auto_test_motion_event_control, 20);  //5s
		return;
	}
	
	emberAfGuaranteedPrintln("Toggle:IasZoneServer"); // Robin add for debug
	emberAfPluginIasZoneServerUpdateZoneStatus(motion_status_toggle ? MOTION_DETECTED 
																	: MOTION_NOT_DETECTED,
                                             	1);
	if(motion_status_toggle == 0x01)
	{
		motion_status_toggle = 0x00;
	}
	else
	{
		motion_status_toggle = 0x01;
	}
	
	emberEventControlSetDelayQS(auto_test_motion_event_control, 20);  //5s
}

void sengledAfMainInit()
{
	emberEventControlSetDelayQS(auto_test_motion_event_control, 20);  //5s
}

void emberAfMainInitCallback()
{

}
