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
#include "app/framework/plugin/sengled-ha-common-TrackLight/sengled-ha-common.h"
#include "app/commonFunction/light_token.h"

EmberEventControl powerReportEventControl;
EmberEventControl powerConsumptionReportEventControl;
EmberEventControl softwarePowerOnEventControl;

static boolean  cctType;
static int16u   saveCctPwm;
int8u  currentLevel_delay = DIMMING_MAX_LEVEL;
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
#ifdef YACHT_DEMO
	int16u cL;
#endif
	int16s  currentIn;
	int32s  voltageIn;

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
  
	if (isOn == 0 || current_level == 0) {
		p_in = 0;
	} else {
		//Computing Power 
		//currentIn = (int16s)(1300 * currentLevel / DIMMING_MAX_LEVEL / 1000);            // GetCurrent
		if (current_level <= 25) {
			current_level = 25; // to avoid negative number
		}
#if (defined COLOR_VOLATILE) && (defined LIGHT_50W)
		currentIn = (5 * current_level - 53) * 90 / 120;    // mA
		voltageIn = (int16s)(adcData[0]) * 52;			// mV
#elif (defined COLOR_VOLATILE) && (defined LIGHT_35W)
		currentIn = (36 * current_level - 362) / 10;
		if(current_level <= 51)
		{
			voltageIn = (int16s)(adcData[0]) * 52 + 30000;
		}
		else if(current_level <= 127)
		{
			voltageIn = (int16s)(adcData[0]) * 52 + 15000;
		}
		else
		{
			voltageIn = (int16s)(adcData[0]) * 52;
		}
			
#elif (defined LIGHT_20W)
#ifdef YACHT_DEMO
	if (current_level == 1)
	{
		current_level = 2;
	}
	cL =  current_level *200/255;
	current_level = cL; 
	
#endif
		currentIn = (5 * current_level - 53) * 60 / 120;    // mA
		voltageIn = (int16s)(adcData[0]) * 52;			// mV
#elif (defined COLOR_STABLE) && ((defined LIGHT_50W)|| (defined LIGHT_35W))
		currentIn = 5 * current_level - 53;    // mA
		voltageIn = (int16s)(adcData[0]) * 40;			// mV
#elif (defined COLOR_STABLE) && (defined LIGHT_50W)

#endif
		voltageIn = (voltageIn<0)?0:voltageIn;  	

		p_out = currentIn * voltageIn / 1000;
#if defined(COLOR_VOLATILE) && defined (LIGHT_35W)
		p_in = (((int32s)p_out) * 113 + 85) / 100;
		if (current_level > 127)
		{
			p_in += 10000;      // +1W 
		}
		else
		{
			p_in += 5000;	// +0.5W 
		}

#else
		p_in = (((int32s)p_out) * 115 + 105) / 100;
#endif
		p_in = p_in / 1000;
	}

#ifdef ROBIN_DEBUG  
		emberAfGuaranteedPrintln("power: %4x, current:%4x, voltage:%4x", p_in,currentIn,voltageIn);
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
#ifdef YACHT_DEMO
		int16u cL;
#endif

	if (level != 0) {
		set_bulb_on();									  // avoid : level ->0, turn off , but if you movetolevel again ,the bulb won't on; 
	} else {
		set_bulb_off();
	}
#ifdef YACHT_DEMO
		cL = level * 200 / 255;	
		if (level == 1)
		{
			cL = 1;
		}
		level = cL;
#endif

	brightness_pwm = TICS_PER_PERIOD - TICS_PER_PERIOD*level/ 255;

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
		set_bulb_on();
		brightness_control(currentLevel_delay);
		cct_control(colorTpt_delay);	
		//GPIO_PBSET = BIT(6);
		//emberEventControlSetDelayMS(onDelaySetPB3EventControl,30);
	} else if (onOffStatus == LED_OFF || currentLevel == 0x00) { 	
		set_bulb_off();
		//GPIO_PBCLR = BIT(6);
		//set_pwm_level(0, DIMMING_CTRL_PIN, TRUE);	
		emberEventControlSetDelayMS(powerReportEventControl,1000);
	}
}

void softwarePowerOnEventFunction(void)
{

}


void software_power_up (void)
{	
	int8u  i = 0;
	int8u  current_level;
#ifdef YACHT_DEMO
	int16u cL;
#endif
	int16u brightness_pwm;
		
	emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
								ZCL_LEVEL_CONTROL_CLUSTER_ID,
								ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
								(int8u *)&current_level,
								sizeof(int8u));
#ifdef YACHT_DEMO
	cL = current_level * 200 / 255;
	current_level = cL;
	if (current_level <= 5)
	{
		current_level = 6;
	}
	
	set_bulb_on();	
	for (i = 0; i < current_level-5; i=i+5)
	{
		brightness_pwm = TICS_PER_PERIOD - TICS_PER_PERIOD*i/255;
		emberAfGuaranteedPrintln("brightness_pwm:%2x",brightness_pwm);
		emberAfPwmSetValuePA6(brightness_pwm);
		delay_millisecond_tick(1);
	}

#else
	set_bulb_on();	
	for (i = 0; i < current_level-5; i=i+5)
	{
		brightness_pwm = TICS_PER_PERIOD - TICS_PER_PERIOD*i/255;
		emberAfGuaranteedPrintln("brightness_pwm:%2x",brightness_pwm);
		emberAfPwmSetValuePA6(brightness_pwm);
		delay_millisecond_tick(1);
	}
#endif
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
	TIM1_ARR = TICS_PER_PERIOD_BASIC;  // set the period
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
		// according to hardware request
		// when controled by app, pwm must be under 90% (270/300)
		if (dutyCycle > 270)
		{
			dutyCycle = 270;
		}
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
#ifdef ROBIN_DEBUG
	emberAfGuaranteedPrintln("!!!!!PB3 OPEN!!!!"); // Robin add for debug
#endif	

	if (program_run_start == TRUE) {
		do {
			GPIO_PBSET = BIT(3); 
			//GPIO_PBSET |= (0x00000008u);
		} while(0);
	}
}

void set_bulb_off (void)   
{
#ifdef ROBIN_DEBUG
	emberAfGuaranteedPrintln("!!!!!PB3 CLOSED!!!!"); // Robin add for debug
#endif	

	do { 		
		GPIO_PBCLR = BIT(3); 
		//GPIO_PBCLR |= (0x00000008u);
	} while (0);
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
		currentLevel = DIMMING_MAX_LEVEL; // product defined
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
		led_control(LED_OFF, currentLevel, currentColorTemperature);	
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
		led_control(LED_ON, currentLevel, currentColorTemperature);	
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
			led_control(LED_ON, current_level, current_cct);
		} else if (onoff_status == LED_OFF) {
#ifdef ROBIN_DEBUG
			emberAfGuaranteedPrintln("OnoffCB: off");            // Robin add for debug
#endif
			led_control(LED_OFF, current_level, current_cct);
		}
	}
}

void emberAfLevelControlClusterServerAttributeChangedCallback(int8u endpoint,
                                                              EmberAfAttributeId attributeId)
{
	int8u currentLevel;

#ifdef YACHT_DEMO
	int16u cL;
#endif
	emberAfReadServerAttribute(endpoint,
								ZCL_LEVEL_CONTROL_CLUSTER_ID,
								ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
								(int8u *)&currentLevel,
								sizeof(int8u));
	
	
#ifdef YACHT_DEMO
	if (currentLevel == 1)
	{
		currentLevel = 2;
	}
	cL = currentLevel *200/255;

	currentLevel_delay = cL;
#else
	currentLevel_delay = currentLevel;
#endif

#ifdef ROBIN_DEBUG
//	emberAfGuaranteedPrintln("LevelCB: crt:%x",currentLevel); // Robin add for debug	
#endif

	if(program_run_start == TRUE) {
		brightness_control(currentLevel);	
	}
}

void emberAfColorControlClusterServerAttributeChangedCallback(int8u endpoint,
                                                               EmberAfAttributeId attributeId)
{
	int16u currentColorTemperature;
	
	//halGpioConfig(PORTB_PIN(7), GPIOCFG_OUT);

	emberAfReadServerAttribute(endpoint,
								ZCL_COLOR_CONTROL_CLUSTER_ID,
								ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
								(int8u *)&currentColorTemperature,
								sizeof(int16u));
	
	colorTpt_delay = currentColorTemperature;
#ifdef ROBIN_DEBUG  
//	emberAfGuaranteedPrintln("ColorCB: %2x",currentColorTemperature); // Robin add for debug	
#endif
	//saveCctLevel = currentColorTemperature;
	//if(currentColorTemperature > 0xF0 && currentColorTemperature < 0x0140) {
	//	color_mode_selection(COLOR_4000K);
	//} else if(currentColorTemperature > 0x0140 && currentColorTemperature < 0x0150) {
	//	color_mode_selection(COLOR_3000K);	
	//}
	cct_control(currentColorTemperature);  
}




