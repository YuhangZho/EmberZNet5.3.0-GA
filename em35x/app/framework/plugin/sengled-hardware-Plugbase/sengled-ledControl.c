/*
************************************************************************************************************************
*                                                      BUSINESS LIGHT
*                                                   TrackLight | E1E-CEA
*
*                                  (c) Copyright 2017-**; Sengled, Inc.; 
*                           All rights reserved.  Protected by international copyright laws.
*
*
* File    : sengled-ledControl.c
* Path   : app/framework/plugin/sengled-hardware-TrackLight
* By      : ROBIN
* Version : 0x00000001
*
* Description:
* ---------------
* Function
* (1) led control:
*           a. emberAfOnOffClusterServerAttributeChangedCallback   
*           b. emberAfLevelControlClusterServerAttributeChangedCallback
*           c. emberAfColorControlClusterServerAttributeChangedCallback
*           d. init_led_status
*           e. set_bulb_on
*           f.  set_bulb_off
*           g. set_pwm_level
*           h. emberAfPwmSetValuePA6
*           i.  emberAfPwmSetValuePB7
*           j.  emberAfPluginPwmControlInitCallback
*           k.  software_power_up
*           l.  led_control
*           m. cct_control
*           n. brightness_control 
*
* (2) power calculate:
*           a. powerReportFunction
*           b. powerConsumptionReportFunction
*           c. GetEfficiency
* History:
* ---------------
*
*
*
************************************************************************************************************************
*/

#include "app/framework/include/af.h"
#include "app/framework/util/attribute-storage.h"
#include "sengled-ledControl.h"
#include "app/framework/plugin/sengled-ha-common-Plugbase/sengled-ha-common.h"


EmberEventControl powerReportEventControl;
EmberEventControl powerConsumptionReportEventControl;


extern int16s adcData[2]; // 2 = ADC_CHANNEL_NUM
extern boolean program_run_start;
extern int8u on_off_remember_when_power_off;

void powerReportFunction (void)
{
	emberEventControlSetInactive(powerReportEventControl);
}

void powerConsumptionReportFunction (void)
{
	emberEventControlSetInactive(powerConsumptionReportEventControl);
}

void emberAfPluginPwmControlInitCallback (void)
{
	halResetWatchdog();
 
	//*********************************************************
	// TIM1 PWM init
	//*********************************************************
	// put PWM initialization code here.
  
	//According to emulator.h, LEVEL_CONTROL is on pin 15 which is mapped 
	//to channel 2 of TMR1
	TIM1_OR = 0;       //use 12MHz clock
	TIM1_PSC = 1;      //1^2=2 -> 12MHz/2 = 6 MHz = 6000 ticks per 1/1000 of a second
	TIM1_EGR = 1;      //trigger update event to load new prescaler value
	TIM1_CCMR1  = 0;   //start from a zeroed configuration
	TIM1_ARR = TICS_PER_PERIOD;  // set the period
	TIM1_CNT = 0; //force the counter back to zero to prevent missing LEVEL_CONTROL_TOP

    //*** set up PB6, which is channel 1 to be a PWM output
    //*** set up PB7, which is channel 2 to be a PWM output
    //*** set up PA6, which is channel 3 to be a PWM output
    //*** set up PA7, which is channel 4 to be a PWM output

    //Output waveform: toggle on CNT reaching TOP
	TIM1_CCMR2 |= (0x6 << TIM_OC3M_BIT); 
	TIM1_CCMR1 |= (0x6 << TIM_OC2M_BIT);   //Added by Shaoxian on 20140408  
  
	ATOMIC(
	TIM1_CCER |= TIM_CC2E;    //enable output on channel 2
	TIM1_CCER |= TIM_CC3E;    //enable output on channel 3
	TIM1_CR1  |= TIM_CEN;      //enable counting
	)

	emberEventControlSetActive(powerReportEventControl);
	emberEventControlSetActive(powerConsumptionReportEventControl);
}

void emberAfPwmSetValuePB7( int16u value )
{
	assert(value <= TICS_PER_PERIOD);
  
	TIM1_CCR2 = value;
} 

void emberAfPwmSetValuePA6( int16u value )
{
	assert(value <= TICS_PER_PERIOD);

	if(on_off_remember_when_power_off == 1)
	{
		TIM1_CCR3 = value;
	}
	else 
	{
		TIM1_CCR3 = 0;
	}
}

void set_pwm_level(int16u dutyCycle, PinSelectEnum pin, boolean powerReportFlag)
{
	switch (pin) {
	case PIN_PA6: {
		emberAfPwmSetValuePA6(dutyCycle);
	}
	break;
	case PIN_PB7: {
		emberAfPwmSetValuePB7(dutyCycle);
	}
	}
  
	// power changed report
	if (powerReportFlag == TRUE) {
		emberEventControlSetDelayMS(powerReportEventControl,1000);
	}
}

void set_bulb_on (void)    
{
	boolean onoff_record;

	sengledGuaranteedPrintln("!!!!!PA6 OPEN!!!!");

	if (program_run_start == TRUE) {
		do {
			GPIO_PBSET = BIT(3); 
		} while(0);
	}
	on_off_remember_when_power_off = 1;
	halCommonSetToken(TOKEN_ON_OFF_STATUS, &on_off_remember_when_power_off);

	emberAfPwmSetValuePA6(TICS_PER_PERIOD);
	// do a status report to coordinator
	emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
								ZCL_ON_OFF_CLUSTER_ID,
								ZCL_ON_OFF_ATTRIBUTE_ID,
								(int8u *)&onoff_record,
								sizeof(boolean));
	
	sengledGuaranteedPrintln("onoff_record: %x",onoff_record);
	if (onoff_record != 1)
	{
		onoff_record = 1;
		emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
								ZCL_ON_OFF_CLUSTER_ID,
								ZCL_ON_OFF_ATTRIBUTE_ID,
								(int8u *)&onoff_record,
								ZCL_BOOLEAN_ATTRIBUTE_TYPE); 
	}


}

void set_bulb_off (void)   
{
	boolean onoff_record;
	
	sengledGuaranteedPrintln("!!!!!PA6 CLOSED!!!!");
	
	do { 		
		GPIO_PBCLR = BIT(3); 
	} while (0);
	
	on_off_remember_when_power_off = 0;
	halCommonSetToken(TOKEN_ON_OFF_STATUS, &on_off_remember_when_power_off);

	emberAfPwmSetValuePA6(0);
	// do a status report to coordinator
	emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
								ZCL_ON_OFF_CLUSTER_ID,
								ZCL_ON_OFF_ATTRIBUTE_ID,
								(int8u *)&onoff_record,
								sizeof(boolean));
	
	sengledGuaranteedPrintln("onoff_record: %x",onoff_record);
	if (onoff_record != 0)
	{
		onoff_record = 0;
		emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
								ZCL_ON_OFF_CLUSTER_ID,
								ZCL_ON_OFF_ATTRIBUTE_ID,
								(int8u *)&onoff_record,
								ZCL_BOOLEAN_ATTRIBUTE_TYPE); 
	}
}

void emberAfOnOffClusterServerAttributeChangedCallback(int8u endpoint, 
                                                       EmberAfAttributeId attributeId)
{
	boolean onoff_status   = 0;
  
	emberAfReadServerAttribute(endpoint,
								ZCL_ON_OFF_CLUSTER_ID,
								ZCL_ON_OFF_ATTRIBUTE_ID,
								(int8u *)&onoff_status,
								sizeof(boolean));
	sengledGuaranteedPrintln("OnOffCluster--PowerOffRemember: %x", on_off_remember_when_power_off); 


	if (onoff_status == LED_ON) {	
		on_off_remember_when_power_off = 1;
		set_bulb_on();
		emberAfPwmSetValuePA6(TICS_PER_PERIOD);
	} else if (onoff_status == LED_OFF) {
		on_off_remember_when_power_off = 0;
		set_bulb_off();
		emberAfPwmSetValuePA6(0);
	}

	
	halCommonSetToken(TOKEN_ON_OFF_STATUS, &on_off_remember_when_power_off);

}

//////////////////////////////////////////////////////////
/*
create a method to handle different led mode 
API: setLedPulseMode
*/
//////////////////////////////////////////////////////////
static int16u pwm_level_status = 0;
static int16u blink_times = 0;
static int32u time_interval = 0;
int8u global_mode = 0;
EmberEventControl ledPulseStatusEventControl;
EmberEventControl blinkForeverEventControl;
int16u panId = 0xffff;

void setLedPulseMode(int8u mode)
{
	sengledGuaranteedPrintln("++ Mode %d ++",mode);

	global_mode = mode;
	switch(mode)
        {
    //setLedPulseStatus Description:
    //param1: means pwm;
    //param2: blink times;
    //param3: bright time/dark time: 1 means 500ms
	case M1: // M1: searching network...
		setLedPulseStatus(TICS_PER_PERIOD,BLINK_FOREVER,4);// 0xff means always until something turns it off, here, it need  at most 30min to turn it off
		break;
	case M2: // M2: joined network now
		setLedPulseStatus(TICS_PER_PERIOD,5,1);  
		break;
	case M3: // M3: means always bright 
		set_pwm_level(TICS_PER_PERIOD, PIN_PA6, TRUE); 
		break;
	case M4: // M4: search network over, > 30minites;
		setLedPulseStatus(TICS_PER_PERIOD,4,2); 
		break;
	default:
		break;
	}
}

void setLedPulseStatus(int16u pwm, int8u count, int8u time)
{
	pwm_level_status = pwm;

	time_interval = time * UNIT_TIME;
	
	if(count == BLINK_FOREVER)
	{
		blink_times = 2;
		if (global_mode == M1)
		{
			panId = join_network_request();
			if (panId != 0xffff)
			{
				setLedPulseMode(M3);
				return;
			}
		}

		emberEventControlSetActive(blinkForeverEventControl); 
		return;
	}
	else
	{
		blink_times = count*2;
	}
	sengledGuaranteedPrintln("+++setLedPulseStatus: BlinkTime:%d,PwmLevel:%d,Time:%d",blink_times,pwm_level_status,time_interval);

	emberEventControlSetDelayMS(ledPulseStatusEventControl, time_interval); 
}

void ledPulseStatusEventFunction(void)
{
	emberEventControlSetInactive(ledPulseStatusEventControl); 
	sengledGuaranteedPrintln("++ledPulseStatus: BlinkTime:%d,PwmLevel:%d,Time:%d",blink_times,pwm_level_status,time_interval);

	if(blink_times == 0)
	{
		sengledGuaranteedPrintln("++ Mode Finished ++");
		// M2: if joined network, set M3, give a always bright;
		// M4: if searching time > 30mins, give a always bright;
		if (global_mode == M2 || global_mode == M4)
		{
			setLedPulseMode(M3); 
		}
		return;

	}

	if(blink_times & 0x01)
	{		
		emberAfPwmSetValuePA6(pwm_level_status);
	}
	else
	{
		emberAfPwmSetValuePA6(0); // 0 means turn pwm off;
	}
	
	blink_times--;

	
	emberEventControlSetDelayMS(ledPulseStatusEventControl, time_interval); 
	
}

extern boolean stopBlinkForeverEvent; 

void blinkForeverEventFunction(void)
{
	emberEventControlSetInactive(blinkForeverEventControl); 
	
	// if we joined network or until 30min, do NOT do blink
	if (stopBlinkForeverEvent == FALSE)
	{
		sengledGuaranteedPrintln("++blinkForever: BlinkTime:%d,PwmLevel:%d,Time:%d",blink_times,pwm_level_status,time_interval); // Robin add for debug

		if(blink_times == 0)
		{
			blink_times = 2;
		}

		if(blink_times & 0x01)
		{		
			emberAfPwmSetValuePA6(pwm_level_status);
		}
		else
		{
			emberAfPwmSetValuePA6(0); // 0 means turn pwm off;
		}
		
		blink_times--;
	}
	else
	{
		// no matter before, now we should set LED Always Bright!
		setLedPulseMode(M3);
		
		sengledGuaranteedPrintln("++blinkForever SHOULD RETURN++");

		return;
	}
	
	emberEventControlSetDelayMS(blinkForeverEventControl, time_interval); 	
}



void emberAfOnOffClusterLevelControlEffectCallback(int8u endpoint,
                                                   boolean newValue)
{

}



