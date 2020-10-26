/*
************************************************************************************************************************
*                                                      BUSINESS LIGHT
*                                                   TrackLight | E1E-CEA
*
*                                  (c) Copyright 2017-**; Sengled, Inc.; 
*                           All rights reserved.  Protected by international copyright laws.
*
*
* File    : sengled-hardware-TrackLight.c
* Path   : app/framework/plugin/sengled-hardware-TrackLight
* By      : ROBIN
* Version : 0x00000001
*
* Description:
* ---------------
* Function
* (1) power off detect
*           a. emberAfHalButtonIsrCallback     
*           b. onDelaySetPB3EventFunction
*           c. OnOffStatusWhenPowerOffEventFunction
*
* (2) double click:
*           a. DoubleClickEventFunction
*           b. NetworkOperationEventFunction
*           c. LedBreathEventFunction
* History:
* ---------------
*
*
*
************************************************************************************************************************
*/

#include "app/framework/include/af.h"
#include "app/framework/util/attribute-storage.h"

#include "sengled-hardware-TrackLight.h"
#include "app/framework/plugin/sengled-ha-common-TrackLight/sengled-ha-common.h"
#include "sengled-ledControl.h"
#include "app/framework/plugin/sengled-ha-cli/sengled-cli.h"
#include "app/commonFunction/light_common.h"
#include "app/commonFunction/light_token.h"

EmberEventControl  doubleClickEventControl;
EmberEventControl  ledBreathEventControl;
EmberEventControl  networkOperationEventControl;
EmberEventControl  onOffStatusWhenPowerOffEventControl;
EmberEventControl  onDelaySetPB3EventControl;

extern  int8u   joinNetworkFirstTimeFlag;
static  int8u   doubleClickCount             = 0;
static  boolean doubleClickStart             = FALSE;
static  boolean doubleClickEnd               = TRUE;
static  int8u   breathTimes                  = 0;	
static  boolean upDown                       = BREATH_UP;  // by default, it changes from current to 100% level, so it's up;
extern  int8u   currentLevelForLedBreath;
extern  int16u  currentColorTptForLedBreath;
extern  int8u   currentLevel_delay;

int8u   blinkTimes                           = 0;
boolean avoidRebootSuddenBlink               = FALSE; 


void LedBreathEventFunction (void)
{
	int8u currentOnLevel= 0xff;

	emberEventControlSetInactive(ledBreathEventControl);

	if (currentLevelForLedBreath == DIMMING_MAX_LEVEL && upDown == BREATH_UP) {
		breathTimes--;
		upDown = BREATH_DOWN;
	} 
	else if(currentLevelForLedBreath == 1 && upDown == BREATH_DOWN) {    // by default it's up, so it's ok here
		breathTimes--;
		upDown = BREATH_UP;    
    }
	
	if (upDown == BREATH_DOWN) {
		currentLevelForLedBreath--;
    } else {
		currentLevelForLedBreath++;  
    }

	//set_bulb_on();
	
	brightness_control(currentLevelForLedBreath);

	if(breathTimes == 0) {
        doubleClickCount = 0; // we need to reset count when the breath finish;
        upDown = BREATH_UP;   // we need to init towards when the breath finish;
#ifdef ROBIN_DEBUG
        emberAfGuaranteedPrintln("breathFinish0: %d",currentLevelForLedBreath); // Robin add for debug
#endif
        emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
									ZCL_LEVEL_CONTROL_CLUSTER_ID,
									ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
									(int8u *)&currentLevelForLedBreath,
									ZCL_INT8U_ATTRIBUTE_TYPE); 

        emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
									ZCL_LEVEL_CONTROL_CLUSTER_ID,
									ZCL_ON_LEVEL_ATTRIBUTE_ID,
 									(int8u *)&currentOnLevel,
									sizeof(currentOnLevel));	
	
		if(currentOnLevel != 0xFF) {
			// when write onLevel, must be careful! Read first if it is 0xff, onlevel is not used
			emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
										ZCL_LEVEL_CONTROL_CLUSTER_ID,
										ZCL_ON_LEVEL_ATTRIBUTE_ID,
										(int8u *)&currentLevelForLedBreath,
										ZCL_INT8U_ATTRIBUTE_TYPE);
		}
		return;
	}

	emberEventControlSetDelayMS(ledBreathEventControl, 30); 
}

void NetworkOperationEventFunction (void)
{
	int8u currentLevel=0;
	int16u currentColorTemperature = 0;
	if(blinkTimes == RESETF_BLINK && avoidRebootSuddenBlink == FALSE) {
		avoidRebootSuddenBlink = TRUE;
	}

	if(blinkTimes > 0) {
		if(blinkTimes & 0x01) {
			emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
										ZCL_LEVEL_CONTROL_CLUSTER_ID,
										ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
										(int8u *)&currentLevel,
										sizeof(int8u));
			emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
										ZCL_COLOR_CONTROL_CLUSTER_ID,
										ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
										(int8u *)&currentColorTemperature,
										sizeof(int16u));
#ifdef ROBIN_DEBUG	  
			emberAfGuaranteedPrintln("SETLED-ON:blink On %d",blinkTimes); // Robin add for debug
#endif

			led_control(LED_ON, currentLevel, currentColorTemperature);
		} else { 
			if(blinkTimes == 2 && avoidRebootSuddenBlink == TRUE) {
				blinkTimes = 0;
				avoidRebootSuddenBlink =FALSE;		
				led_control(LED_OFF, DIMMING_MAX_LEVEL, COLOR_YELLOWEST);
				
				emberEventControlSetDelayMS(networkOperationEventControl, 600);
				return;
			}
#ifdef ROBIN_DEBUG	  
			emberAfGuaranteedPrintln("blink Off %d",blinkTimes); // Robin add for debug
#endif
			led_control(LED_OFF, DIMMING_MAX_LEVEL, COLOR_YELLOWEST);
		}
		blinkTimes--;
	
		emberEventControlSetDelayMS(networkOperationEventControl, 500);
	} else {
#ifdef  SUPPORT_DOUBLE_CLICK
		if(doubleClickCount == REJOIN) {
			//emberFindAndRejoinNetwork(1, EMBER_ALL_802_15_4_CHANNELS_MASK);
			emberFindAndRejoinNetworkWithReason(1,  // 1=rejoin with encryption, 0=rejoin without encryption
												EMBER_ALL_802_15_4_CHANNELS_MASK,
												EMBER_AF_REJOIN_DUE_TO_END_DEVICE_MOVE); // end device rejoin 
		} else if(doubleClickCount >= RESETF) {
			emberLeaveNetwork();
			joinNetworkFirstTimeFlag = 0x00;
			halCommonSetToken(TOKEN_THE_FIRST_JOINED_FLAG, &joinNetworkFirstTimeFlag);
	  
			emberAfResetAttributes(emberAfPrimaryEndpoint());
			emberAfGroupsClusterClearGroupTableCallback(emberAfPrimaryEndpoint());
			emberAfScenesClusterClearSceneTableCallback(emberAfPrimaryEndpoint());
			emberClearBindingTable();
			emberAfClearReportTableCallback();
			halReboot();
		}
#else
	if(doubleClickCount >= RESETF) {		
		emberLeaveNetwork();
		joinNetworkFirstTimeFlag = 0x00;
		halCommonSetToken(TOKEN_THE_FIRST_JOINED_FLAG, &joinNetworkFirstTimeFlag);

		emberAfResetAttributes(emberAfPrimaryEndpoint());
		emberAfGroupsClusterClearGroupTableCallback(emberAfPrimaryEndpoint());
		emberAfScenesClusterClearSceneTableCallback(emberAfPrimaryEndpoint());
		emberClearBindingTable();
		emberAfClearReportTableCallback();
		halReboot();
	}
#endif
		doubleClickCount = 0;
		emberEventControlSetInactive(networkOperationEventControl);
	}
}


void DoubleClickEventFunction(void)
{
	int8u currentOnLevel = 0xff;

	emberEventControlSetInactive(doubleClickEventControl);

	if (doubleClickEnd == TRUE) {
		doubleClickStart = FALSE;
#ifdef ROBIN_DEBUG	
		emberAfGuaranteedPrintln("ClickCount: %d",doubleClickCount); // Robin add for debug
#endif

		switch (doubleClickCount) {
#ifdef  0
		case BREATH_START: {  // do not clear doubleClickCount
			breathTimes = BREATH_TOTAL_TIMES;
			
			emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
									ZCL_LEVEL_CONTROL_CLUSTER_ID,
									ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
									(int8u *)&currentLevelForLedBreath,
									sizeof(int8u));
			emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
									ZCL_COLOR_CONTROL_CLUSTER_ID,
									ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
									(int8u *)&currentColorTptForLedBreath,
									sizeof(int16u));	
			
			emberEventControlSetActive(ledBreathEventControl);
		}
		break;
		case BREATH_END: {
#ifdef ROBIN_DEBUG
			emberAfGuaranteedPrintln("breathFinish1: %d",currentLevelForLedBreath); // Robin add for debug
#endif
			// first record currentLevel
			emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
										ZCL_LEVEL_CONTROL_CLUSTER_ID,
										ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
										(int8u *)&currentLevelForLedBreath,
										ZCL_INT8U_ATTRIBUTE_TYPE);

			emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
										ZCL_LEVEL_CONTROL_CLUSTER_ID,
										ZCL_ON_LEVEL_ATTRIBUTE_ID,
										(int8u *)&currentOnLevel,
										sizeof(currentOnLevel));  

			if (currentOnLevel != 0xFF) {
				// when write onLevel, must be careful! Read first if it is 0xff, onlevel is not used
				emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
  											ZCL_LEVEL_CONTROL_CLUSTER_ID,
											ZCL_ON_LEVEL_ATTRIBUTE_ID,
											(int8u *)&currentLevelForLedBreath,
											ZCL_INT8U_ATTRIBUTE_TYPE);
			}

			breathTimes = 0;
			doubleClickCount = 0;
			upDown = BREATH_UP;   // we need to init towards when the breath finish;
          
			emberEventControlSetInactive(ledBreathEventControl);
		}
		break;
		case REJOIN: { // do not clear doubleClickCount right now
			blinkTimes = REJOIN_BLINK; 

			emberEventControlSetActive(networkOperationEventControl);
		}
		break;
		case EZMODE: {
			emberAfEzmodeServerCommission(emberAfPrimaryEndpoint());

			doubleClickCount = 0;
        }
        break;
#endif
		default:  // 2016.4.20
			if (doubleClickCount < RESETF) {
				doubleClickCount = 0; // do not record  if useless;
			}
		}
	
		if (doubleClickCount >= RESETF) {
			blinkTimes = RESETF_BLINK;
		
			emberEventControlSetActive(networkOperationEventControl);
		}
	}
}

void OnOffStatusWhenPowerOffEventFunction(void)
{
	emberEventControlSetInactive(onOffStatusWhenPowerOffEventControl);  
#ifdef ROBIN_DEBUG
	emberAfGuaranteedPrintln("OnOffStatusWhenPowerOff"); // Robin add for debug
#endif

	attribute_report(ZCL_SIMPLE_METERING_CLUSTER_ID,ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID, 
					ZCL_INT24S_ATTRIBUTE_TYPE, CLUSTER_IS_SERVER);
	send_zcl_cmd_unicast(COORDINATOR_NODE_ID);
}

void onDelaySetPB3EventFunction (void)
{
	emberEventControlSetInactive(onDelaySetPB3EventControl);	
#ifdef ROBIN_DEBUG
	emberAfGuaranteedPrintln("onDelaySetPB3Event"); // Robin add for debug
#endif

	set_bulb_on();
	brightness_control(currentLevel_delay);
	//cct_control(colorTpt_delay);		
}

void emberAfHalButtonIsrCallback (int8u button,
                                        int8u state)
{
	boolean onoff_status;
	static int16u new_time,old_time;  // 2016.3.23 power on affect another when they all supplied by one power
  
	if (button == DOUBLE_CLICK) {
		if (state == DOUBLE_CLICK_ON) {
			if (doubleClickStart == TRUE) {
#ifdef ROBIN_DEBUG
				emberAfGuaranteedPrintln("DOUBLE_CLICK: ON"); // Robin add for debug
#endif

				doubleClickEnd = TRUE;

				new_time = halCommonGetInt16uMillisecondTick();
				if ( elapsedTimeInt16u(old_time, new_time) < PREVENT_SIGNAL_SHAKING_TIME) {
					//do nothing to filter PB0 signal shake
#ifdef ROBIN_DEBUG
					emberAfGuaranteedPrintln("Filter PB0 Signal Shake"); // Robin add for debug
#endif

				} else {
					doubleClickCount++;
#ifdef ROBIN_DEBUG
					emberAfGuaranteedPrintln("doubleClickCount:%d",doubleClickCount); // Robin add for debug
#endif

					set_pwm_level(TICS_PER_PERIOD, DIMMING_CTRL_PIN, FALSE);
					//delay 30ms then do open circuit;
					emberEventControlSetDelayMS(onDelaySetPB3EventControl,30);	
					onoff_status = 1;
					emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
												ZCL_ON_OFF_CLUSTER_ID,
												ZCL_ON_OFF_ATTRIBUTE_ID,
												(int8u *)&onoff_status,
												ZCL_BOOLEAN_ATTRIBUTE_TYPE);  

					emberEventControlSetDelayMS(doubleClickEventControl,1300);
				}
			}
		} else if (state == DOUBLE_CLICK_OFF) {
#ifdef ROBIN_DEBUG
            emberAfGuaranteedPrintln("DOUBLE_CLICK: OFF"); // Robin add for debug
#endif

			old_time = halCommonGetInt16uMillisecondTick();
            
			set_bulb_off();
			
			set_pwm_level(TICS_PER_PERIOD, DIMMING_CTRL_PIN, FALSE); 
		
			doubleClickStart = TRUE;
			doubleClickEnd = FALSE;
  
			set_aps_num_type(emberAfGetLastSequenceNumber());

			attribute_report(ZCL_ON_OFF_CLUSTER_ID,ZCL_ON_OFF_ATTRIBUTE_ID, 
                                       ZCL_BOOLEAN_ATTRIBUTE_TYPE, CLUSTER_IS_SERVER);
			send_zcl_cmd_unicast(COORDINATOR_NODE_ID);

			emberEventControlSetDelayMS(onOffStatusWhenPowerOffEventControl,150);
		}
	}
}

