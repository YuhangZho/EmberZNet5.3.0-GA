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
#include "app/framework/plugin/sengled-ha-common-B815/sengled-ha-common.h"
#include "app/commonFunction/light_token.h"

EmberEventControl powerReportEventControl;
EmberEventControl powerConsumptionReportEventControl;
EmberEventControl softwarePowerOnEventControl;

static boolean  cctType;
static int16u   saveCctPwm;
int8u  currentLevel_delay = 255;
int16u colorTpt_delay     = 0;

int16u saveCctLevel;

int8u  currentLevelForLedBreath;
int16u currentColorTptForLedBreath;

extern int16s adcData[2]; // 2 = ADC_CHANNEL_NUM
extern boolean program_run_start;
// level = COLOR_TEMPRATURE_MIN  y = -0.5446x2 + 295x + 39876
// level != COLOR_TEMPRATURE_MIN  y = -0.8855x2 + 423.32x + 27991
int32s GetEfficiency(int8u level)
{
	int32u tmp = level;

	if (saveCctLevel == COLOR_WHITEST) {
		tmp = 295*tmp+39876-(tmp*tmp*5446/10000);
	} else { 
		tmp = (42332*tmp/100)+27991-(tmp*tmp*8855/10000);
	}

	if (tmp == 0) { 
		tmp = 0xffffffff;
	}

	return tmp;
}

void powerReportFunction (void)
{
	int32s p_in, p_out=0;
	static  int32s lastReportPower = 0;
	boolean isOn;
	int8u   current_level;
	int16s  currentIn;
	int32s  voltageIn;
	int8u  pwm_percent = 0;
	
	emberEventControlSetInactive(powerReportEventControl);
  
	emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
							ZCL_ON_OFF_CLUSTER_ID,
							ZCL_ON_OFF_ATTRIBUTE_ID,
							(int8u *)&isOn,
							sizeof(boolean));
    emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
 							ZCL_LEVEL_CONTROL_CLUSTER_ID,
							ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
							(int8u *)&current_level,
							sizeof(int8u));
	
    // 815  common 
	pwm_percent = 100 * current_level/DIMMING_MAX_LEVEL;
	

/*
	//E2K-CCA-N

	if (pwm_percent >= 95)
	{
		p_in = 459 * pwm_percent / 1000;
		p_in -= 6;
	}
	else if(pwm_percent >= 8)
	{
		p_in = 397 * pwm_percent;
		p_in = (p_in - 117) /1000;
	}

	p_in = p_in * 10;
*/

////// RDF2016022/E1E-CEA;  RDF2016023/E1A-CEA;  //////
//                1.2A 36V 50W                         //
//	p_in = 510 * pwm_percent + 1309;
//	p_in = p_in / 100;

////// RDF2017022  RDF2017023  ////////
//////         1.3A 25V 35W            ////////
//	p_in = 387 * pwm_percent + 1334;
//	p_in = p_in / 100;

////// RDF2017012/E1E-CBA;  ////////
////// RDF2017009/E1A-CBA;  ////////
////// RDF2017016/E1I-CBA;  ////////
////// RDF2017015/E1H-CBA;  ////////
//            0.6A 36V 25W                       //
//	p_in = 232 * pwm_percent + 1172;
//	p_in = p_in / 100;

////// RDF2017013/E1E-CCA;	////////
////// RDF2017010/E1A-CCA;	////////
//         0.9A 36V 35W  (dan)                //
//	p_in = 335 * pwm_percent + 1529;
//	p_in = p_in / 100;


////// RDF2017004/E2L-CCA;	////////
////// RDF2017002/E2K-CCA;	////////
//		0.9A 36V 35W (shuang)          //
	p_in = 363 * pwm_percent + 1286;
	p_in = p_in / 100;

////////////////////////////////////////////////////

	if (isOn == 0 || current_level == 0)
	{
		p_in = 0;
	}

#ifdef ROBIN_DEBUG  
//		emberAfGuaranteedPrintln("power: %4x, current:%4x, voltage:%4x", p_in,currentIn,voltageIn);
#endif

	if (lastReportPower > p_in) {
		if ((lastReportPower - p_in) > 10 ) {
			lastReportPower = p_in;
			emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
										ZCL_SIMPLE_METERING_CLUSTER_ID,
										ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID,
										(int8u *)&lastReportPower,
										ZCL_INT24S_ATTRIBUTE_TYPE);		
		}
	} else {
		if ((p_in - lastReportPower) > 10) {
			lastReportPower = p_in;
			emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
										ZCL_SIMPLE_METERING_CLUSTER_ID,
										ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID,
										(int8u *)&lastReportPower,
										ZCL_INT24S_ATTRIBUTE_TYPE);		
		}
	}
	
	emberEventControlSetDelayQS(powerReportEventControl, 6);  // 1.5s
}

void powerConsumptionReportFunction (void)
{
    static int32s p_total;
    int32s current_p      = 0;
    int64u p_consumption  = 0;
  
	emberEventControlSetInactive(powerConsumptionReportEventControl);

	emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
								ZCL_SIMPLE_METERING_CLUSTER_ID,
								ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID,
								(int8u *)&current_p,
								sizeof(int32s));
  
	emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
								ZCL_SIMPLE_METERING_CLUSTER_ID,
								ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
								(int8u *)&p_consumption,
								sizeof(int64u));
#ifdef ROBIN_DEBUG  
	// emberAfGuaranteedPrintln("currentPower: %4x", currentPower); // Robin add for debug
#endif
	p_total += current_p * 5; // 5s
  
#ifdef ROBIN_DEBUG  
	// emberAfGuaranteedPrintln("powerTotal: %d", powerTotal); // Robin add for debug
#endif
	current_p = p_total / 3600;
	if (current_p > 0) {
		p_consumption += current_p;
		p_total = p_total % 3600;
	}
#ifdef ROBIN_DEBUG  
	// emberAfGuaranteedPrintln("powerConsumption: %4x",powerConsumption); // Robin add for debug
#endif

	emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
								ZCL_SIMPLE_METERING_CLUSTER_ID,
								ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
								(int8u *)&p_consumption,
								ZCL_INT48U_ATTRIBUTE_TYPE);
  
	emberEventControlSetDelayMS(powerConsumptionReportEventControl, 5000);  // 5s   
}

void brightness_control (int8u level)
{
	int16u brightness_pwm;
	
	brightness_pwm = TICS_PER_PERIOD*level/DIMMING_MAX_LEVEL;

#ifdef ROBIN_DEBUG	
	//emberAfGuaranteedPrintln("LevelPwm: %2x",dimLevel); // Robin add for debug
#endif	 
	set_pwm_level(brightness_pwm, DIMMING_CTRL_PIN, TRUE);  
}

void cct_control(int16u colorTpt)
{
	int32u cct_pwm;

	if (colorTpt > COLOR_YELLOWEST)
	{
		colorTpt = COLOR_YELLOWEST;
	}
	if (colorTpt < COLOR_WHITEST)
	{
		colorTpt = COLOR_WHITEST;
	}
	
	cct_pwm = TICS_PER_PERIOD * (COLOR_YELLOWEST - colorTpt);
	cct_pwm = cct_pwm / (COLOR_YELLOWEST - COLOR_WHITEST);
#ifdef ROBIN_DEBUG
//	emberAfGuaranteedPrintln("CCT: %4x",cct_pwm); // Robin add for debug
#endif	

	set_pwm_level((int16u)cct_pwm, COLOR_TEMP_CTRL_PIN, TRUE);

}

void led_control(boolean onOffStatus, int8u currentLevel, int16u colorTpt)
{
	currentLevel_delay = currentLevel;
	colorTpt_delay = colorTpt;

	if (onOffStatus == LED_ON && currentLevel != 0x00 && program_run_start == TRUE) {
		brightness_control(currentLevel_delay);
		cct_control(colorTpt_delay);	

	} else if (onOffStatus == LED_OFF || currentLevel == 0x00) { 	
		brightness_control(0);
		emberEventControlSetDelayMS(powerReportEventControl,1000);
	}
}

void led_control_815(int8u current_level, int16u color_Tpt)
{
	int16u brightness_pwm;
	int32u cct_pwm;

	if(current_level == 0)
	{
		emberAfPwmSetValuePA6(0);
	}
	else
	{
		brightness_pwm = TICS_PER_PERIOD*current_level/DIMMING_MAX_LEVEL;	
		if(brightness_pwm < 480) // 6000*8% = 480
		{
			brightness_pwm = 480;
		}

		if (color_Tpt > COLOR_YELLOWEST)
		{
			color_Tpt = COLOR_YELLOWEST;
		}
		if (color_Tpt < COLOR_WHITEST)
		{
			color_Tpt = COLOR_WHITEST;
		}
		
		cct_pwm = TICS_PER_PERIOD * (COLOR_YELLOWEST - color_Tpt);
		cct_pwm = cct_pwm / (COLOR_YELLOWEST - COLOR_WHITEST);		
		cct_pwm = cct_pwm * brightness_pwm / TICS_PER_PERIOD;
		
		emberAfPwmSetValuePA6(brightness_pwm);
		emberAfPwmSetValuePB7((int16u)cct_pwm);
	}
	
	emberEventControlSetDelayMS(powerReportEventControl,1000);
}

void softwarePowerOnEventFunction(void)
{

}


void software_power_up (void)
{	
	int8u  i = 0;
	int8u  current_level;
	int16u brightness_pwm;
		
	emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
								ZCL_LEVEL_CONTROL_CLUSTER_ID,
								ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
								(int8u *)&current_level,
								sizeof(int8u));

	for (i = 25; i < current_level-5; i=i+5)
	{
		brightness_pwm = TICS_PER_PERIOD*i/DIMMING_MAX_LEVEL;
		//emberAfGuaranteedPrintln("brightness_pwm:%2x",brightness_pwm);
		//emberAfPwmSetValuePA6(brightness_pwm);
		led_control_815(brightness_pwm, COLOR_YELLOWEST);
		delay_millisecond_tick(1);
	}
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

	//emberAfPwmSetValuePB7(TICS_PER_PERIOD);
	//emberAfPwmSetValuePA6(TICS_PER_PERIOD);
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

	TIM1_CCR3 = value;	
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

void init_led_status (boolean firstjoinedNetworkFlag)
{
	int8u   onLevel                 = 0xff; // by default 
	boolean onOff                   = 1;
	int8u   currentLevel            = DIMMING_MAX_LEVEL;
	int16u  extendedResetInfo       = halGetExtendedResetInfo();
	int16u  currentColorTemperature = COLOR_YELLOWEST;

  
	if (firstjoinedNetworkFlag == 0) {
		emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
									ZCL_LEVEL_CONTROL_CLUSTER_ID,
									ZCL_ON_LEVEL_ATTRIBUTE_ID,
									(int8u *)&onLevel,
									sizeof(int8u)); 
		if (onLevel == 0xff) {
			emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
										ZCL_LEVEL_CONTROL_CLUSTER_ID,
										ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
										(int8u *)&currentLevel,
										sizeof(int8u)); 
		} else {
			currentLevel = onLevel;
		}
	
		emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
									ZCL_COLOR_CONTROL_CLUSTER_ID,
									ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
									(int8u *)&currentColorTemperature,
									sizeof(int16u));
	}
#ifdef ROBIN_DEBUG	
	emberAfGuaranteedPrintln("onL: %x, currentL: %x, color: %2x",
							onLevel, currentLevel,currentColorTemperature); // Robin add for debug
#endif

	if(currentLevel == 0x00) {
		currentLevel = 0xff; // product defined
		halResetWatchdog(); // prevent watchdog reset;
		emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
									ZCL_LEVEL_CONTROL_CLUSTER_ID,
									ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
									(int8u *)&currentLevel,
									ZCL_INT8U_ATTRIBUTE_TYPE);
	}
  
	currentLevelForLedBreath     = currentLevel;
	currentColorTptForLedBreath  = currentColorTemperature;

	emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
								ZCL_ON_OFF_CLUSTER_ID,
								ZCL_ON_OFF_ATTRIBUTE_ID,
								(int8u *)&onOff,
								sizeof(boolean));	

	if (extendedResetInfo != 0x0301 && onOff == 0) {
		// it may be a ota finished reboot, if the lamp is off, then keep it off;
		led_control_815(0, currentColorTemperature);	
	} else {
		onOff = 1;
		emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
									ZCL_ON_OFF_CLUSTER_ID,
									ZCL_ON_OFF_ATTRIBUTE_ID,
									(int8u *)&onOff,
									ZCL_BOOLEAN_ATTRIBUTE_TYPE);  
#ifdef ROBIN_DEBUG	
		emberAfGuaranteedPrintln("SETLED-ON: InitLEDStatus");     // Robin add for debug
#endif 
		led_control_815(currentLevel, currentColorTemperature);	
	}
}

void emberAfOnOffClusterServerAttributeChangedCallback(int8u endpoint, 
                                                       EmberAfAttributeId attributeId)
{
	boolean onoff_status   = 0;
	int8u   current_level  = 0x00;
	int16u  current_cct    = 0x00;
	static int8u onOffOnce = 0x00;
  
	emberAfReadServerAttribute(endpoint,
								ZCL_ON_OFF_CLUSTER_ID,
								ZCL_ON_OFF_ATTRIBUTE_ID,
								(int8u *)&onoff_status,
								sizeof(boolean));
  
	emberAfReadServerAttribute(endpoint,
								ZCL_LEVEL_CONTROL_CLUSTER_ID,
								ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
								(int8u *)&current_level,
								sizeof(int8u)); 
  
	emberAfReadServerAttribute(endpoint,
								ZCL_COLOR_CONTROL_CLUSTER_ID,
								ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
								(int8u *)&current_cct,
								sizeof(int16u));

	if (onOffOnce != onoff_status) {
		onOffOnce = onoff_status;

		if (onoff_status == LED_ON) {	
#ifdef ROBIN_DEBUG
			emberAfGuaranteedPrintln("SetLED-ON:OnoffCB: %x, L: %x, C: %2x",
									onoff_status, current_level, current_cct); // Robin add for debug
#endif
			led_control_815(current_level, current_cct);
		} else if (onoff_status == LED_OFF) {
#ifdef ROBIN_DEBUG
			emberAfGuaranteedPrintln("OnoffCB: off");            // Robin add for debug
#endif
			led_control_815(0, current_cct);
		}
	}
}

void emberAfLevelControlClusterServerAttributeChangedCallback(int8u endpoint,
                                                              EmberAfAttributeId attributeId)
{
	int8u currentLevel;
	int16u current_cct;
	emberAfReadServerAttribute(endpoint,
								ZCL_LEVEL_CONTROL_CLUSTER_ID,
								ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
								(int8u *)&currentLevel,
								sizeof(int8u));
	emberAfReadServerAttribute(endpoint,
								ZCL_COLOR_CONTROL_CLUSTER_ID,
								ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
								(int8u *)&current_cct,
								sizeof(int16u));
	
	currentLevel_delay = currentLevel;

#ifdef ROBIN_DEBUG
//	emberAfGuaranteedPrintln("LevelCB: crt:%x",currentLevel); // Robin add for debug	
#endif

	if(program_run_start == TRUE) {
		led_control_815(currentLevel,current_cct);	
	}
}

void emberAfColorControlClusterServerAttributeChangedCallback(int8u endpoint,
                                                               EmberAfAttributeId attributeId)
{
	int16u currentColorTemperature;
	int8u currentLevel;

	emberAfReadServerAttribute(endpoint,
								ZCL_LEVEL_CONTROL_CLUSTER_ID,
								ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
								(int8u *)&currentLevel,
								sizeof(int8u));
	emberAfReadServerAttribute(endpoint,
								ZCL_COLOR_CONTROL_CLUSTER_ID,
								ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
								(int8u *)&currentColorTemperature,
								sizeof(int16u));
	
	colorTpt_delay = currentColorTemperature;
#ifdef ROBIN_DEBUG  
//	emberAfGuaranteedPrintln("ColorCB: %2x",currentColorTemperature); // Robin add for debug	
#endif

	led_control_815(currentLevel,currentColorTemperature);	
}




