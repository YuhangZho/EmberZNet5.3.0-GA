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

#include "sengled-hardware-plugbase.h"
#include "app/framework/plugin/sengled-ha-common-Plugbase/sengled-ha-common.h"
#include "sengled-ledControl.h"
#include "app/framework/plugin/sengled-ha-cli/sengled-cli.h"

extern  int8u   joinNetworkFirstTimeFlag;
extern int8u on_off_remember_when_power_off;
extern boolean stopBlinkForeverEvent;

/////////////////////////////////////////////////////////////////

/* Button handler*/

/////////////////////////////////////////////////////////////////
static boolean buttonState;
static int8u buttonTimer;
static int8u start_counter = 0;
EmberEventControl buttonEventControl;
EmberEventControl blinkEventControl;

static boolean plugButton = TRUE; // when power on, default is on;
static int8u blink_event = NO_EVENT;
static int8u blink_times = 0;

void blinkEventFunction(void)
{
	emberEventControlSetInactive(blinkEventControl); 
	
	sengledGuaranteedPrintln("blink_times:%x", blink_times);
	
	if (blink_times == 0)
	{
		if(blink_event == RESET_EVENT_BLINK)
		{
			emberLeaveNetwork();
			stopBlinkForeverEvent = FALSE;	
			setLedPulseMode(M1);
			
			on_off_remember_when_power_off = 1;
			halCommonSetToken(TOKEN_ON_OFF_STATUS, &on_off_remember_when_power_off);
			
			joinNetworkFirstTimeFlag = SEARCH_NETWORK;
			halCommonSetToken(TOKEN_THE_FIRST_JOINED_FLAG, &joinNetworkFirstTimeFlag);

			emberAfResetAttributes(emberAfPrimaryEndpoint());
			emberAfGroupsClusterClearGroupTableCallback(emberAfPrimaryEndpoint());
			emberAfScenesClusterClearSceneTableCallback(emberAfPrimaryEndpoint());
			emberClearBindingTable();
			emberAfClearReportTableCallback();	
			halReboot();
		}
		else if(blink_event == REJOIN_EVENT_BLINK)
		{
      		emberFindAndRejoinNetworkWithReason(ENCRYPTION,
												EMBER_ALL_802_15_4_CHANNELS_MASK,
												EMBER_AF_REJOIN_DUE_TO_END_DEVICE_MOVE); // end device rejoin 			
		}
		return;
	}

	if (on_off_remember_when_power_off == 0) // we need to keep off when blink over;
	{
		if(blink_times % 2 == 0)
		{
			TIM1_CCR3 = TICS_PER_PERIOD;
		}
		else
		{
			TIM1_CCR3 = 0;
		}
	}
	else
	{
		if(blink_times % 2 == 0)
		{
			TIM1_CCR3 = 0;
		}
		else
		{
			TIM1_CCR3 = TICS_PER_PERIOD;
		}
	}
 
	blink_times--;	
	emberEventControlSetDelayMS(blinkEventControl, 500); 

}

void buttonEventFunction(void) 
{ 
	emberEventControlSetInactive(buttonEventControl);
  
	if (BUTTON_PRESSED == buttonState)
	{
		buttonTimer++;
		sengledGuaranteedPrintln("Press-buttonTimer: %x",buttonTimer); 
		emberEventControlSetDelayMS(buttonEventControl, 500); 
	}
	else
	{
		//sengledGuaranteedPrintln("Release-buttonTimer:%x",buttonTimer); 
		
		// button release now
		start_counter = 0;

		if ((buttonTimer > SINGLE_PRESS_LOWER_BOUND) && (buttonTimer <= SINGLE_PRESS_UPPER_BOUND))
		{
			if (joinNetworkFirstTimeFlag == JOINED_NETWORK)	
			{
				if (plugButton == TRUE && on_off_remember_when_power_off == 1)
				{
					// turn off plug
					sengledGuaranteedPrintln("TURN-OFF"); 
					plugButton = FALSE;	
					set_bulb_off();
				}
				else if (plugButton == TRUE && on_off_remember_when_power_off == 0)
				{
					// turn on plug
					sengledGuaranteedPrintln("TURN-ON"); 
					plugButton = FALSE;	
					set_bulb_on(); 
				}
				else if (plugButton == FALSE && on_off_remember_when_power_off == 1)
				{
					sengledGuaranteedPrintln("TURN-OFF");
					plugButton = TRUE;	
					set_bulb_off();

				}
				else if (plugButton == FALSE && on_off_remember_when_power_off == 0)
				{
					sengledGuaranteedPrintln("TURN-ON");
					plugButton = TRUE;	
					set_bulb_on();
				}	
			}
		}
		else if (buttonTimer > REJOIN_LOWER_BOUND && buttonTimer <= REJOIN_UPPER_BOUND)
		{
			if (joinNetworkFirstTimeFlag == JOINED_NETWORK)	
			{
				sengledGuaranteedPrintln(" > REJOIN_PRESS_LOWER_BOUND");  // check button 
				blink_event = REJOIN_EVENT_BLINK;
				blink_times = REJOIN_BLINK_TIMES;
				emberEventControlSetActive(blinkEventControl); 
			}
		}
		else if (buttonTimer > RESET_PRESS_LOWER_BOUND && buttonTimer <= RESET_PRESS_UPPER_BOUND)  
		{		
			sengledGuaranteedPrintln(" > RESET_PRESS_LOWER_BOUND");  // check button  
			blink_event = RESET_EVENT_BLINK;
			blink_times = RESET_BLINK_TIMES;
			emberEventControlSetActive(blinkEventControl); 
		}	
		else if (buttonTimer > EZ_MODE_LOWER_BOUND && buttonTimer <= EZ_MODE_UPPER_BOUND)
		{
			if (joinNetworkFirstTimeFlag == JOINED_NETWORK)	
			{
				// to start ezmode when joined network 
				sengledGuaranteedPrintln(" > EZ_MODE_LOWER_BOUND");  // check button  
				emberAfEzmodeServerCommission(emberAfPrimaryEndpoint());
				//emberAfPermitJoin(EMBER_AF_PLUGIN_NETWORK_STEERING_COMMISSIONING_TIME_S,
                //                 true); // Broadcast permit join?
			}
		}		
		buttonTimer = 0;
	}
}

void emberAfHalButtonIsrCallback (int8u button,
                                        int8u state)
{
	static int16u new_time,old_time;  // 2016.3.23 power on affect another when they all supplied by one power

	buttonState = state;
	
	if (button != BUTTON0)
		return;

	if (state == BUTTON_PRESSED)
	{
		sengledGuaranteedPrintln("PRESS");  // check button 
		old_time = halCommonGetInt16uMillisecondTick();
		start_counter++;
	}
	else
	{	
		new_time = halCommonGetInt16uMillisecondTick();
		if ( elapsedTimeInt16u(old_time, new_time) < PREVENT_SIGNAL_SHAKING_TIME) {
			sengledGuaranteedPrintln("Sigbal shake!");	
			return;
		} 
		sengledGuaranteedPrintln("RELEASE");
		start_counter = 1;
	}
	
	if (start_counter == 1)
	{
		emberEventControlSetActive(buttonEventControl);
	}
}

