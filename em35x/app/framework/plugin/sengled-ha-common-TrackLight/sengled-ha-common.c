/*
************************************************************************************************************************
*                                                      BUSINESS LIGHT
*                                                    TrackLight | E1E-CEA
*
*                                  (c) Copyright 2017-**; Sengled, Inc.; 
*                           All rights reserved.  Protected by international copyright laws.
*
*
* File    : sengled-ha-common.c
* Path   : app/framework/plugin/sengled-ha-common-TrackLight/
* By      : ROBIN
* Version : 0x00000001
*
* Description:
* ---------------
* Function
* (1) Program User Init:  
*           a. emberAfMainInitCallback     
*           b. sengledAfMainInit
*
* (2) Factory Test:
*           a. emberProcessCommandSendled
*
* (3) Loop Main Tick:
*           a. emberAfMainTickCallback
*
* (4) Hardware Operate:
*           a. get_aps_num_type
*           b. set_aps_num_type
*
* (5) Time Delay:
*           a. delay_millisecond_tick
*
* (6) Event Trigger:
*           a. diagnosticsEventFunction
*
* (7) Led Control:
*           a. color_mode_selection
*
* (8) Adc Control
*           a. GetFilteredValue     
*           b. adc_init
*           c. adc_control

************************************************************************************************************************
*/

#include "app/framework/include/af.h"
#include "sengled-ha-common.h"
#include "app/framework/plugin/counters/counters.h"
#include "app/commonFunction/light_common.h"
#include "app/commonFunction/light_token.h"

//boolean doubleClickEnable = FALSE;
EmberEventControl diagnosticsEventControl;
EmberEventControl autoResetCheckEventControl;

/*
******************************************************
* Global Var: program_run_start
*
* Description:
* To tell application program that we can do our own fucntion now
* To avoid ember code call some callbacks automatically when power on
* such as emberAfLevelControlClusterServerAttributeChangedCallback
*
* Scope:
* sengeld-ha-common.c
* sengeld-ledcontrol.c
*
******************************************************
*/
boolean program_run_start = FALSE;

#define ADC_IN_VOLTAGE     ADC_SOURCE_ADC4_VREF2
#define GetCurrent(c) (((c)*10)/27+10)
#define GetVoltage(c) ((int16s)(((int32s)(c)*3671)/10000))

static int8u adcChannelIndex = 0;
const  int8u adcChannel[] = {ADC_IN_VOLTAGE};
#define ADC_CHANNEL_NUM (sizeof(adcChannel)/sizeof(adcChannel[0]))
int16s adcData[ADC_CHANNEL_NUM] = {0};

void get_aps_num_type (void)
{ 
    int8u num;

	halCommonGetToken(&num, TOKEN_THE_APSNUM_TYPE);
    SengledSetSequenceNumber(num+1);
}

void set_aps_num_type (int8u num)
{ 
    halCommonSetToken(TOKEN_THE_APSNUM_TYPE, &num);
}

void delay_millisecond_tick (int16u delay_millisecond)
{
    int16u nowTime, nextTime;

    nowTime = halCommonGetInt16uMillisecondTick();
    nextTime = halCommonGetInt16uMillisecondTick();
    while((nextTime - nowTime) < delay_millisecond) {
        halResetWatchdog(); 
        nextTime = halCommonGetInt16uMillisecondTick();
    }	
}

void autoResetCheckEventFunction(void)
{
	emberEventControlSetInactive(autoResetCheckEventControl);  
}


// extendedResetInfo:
// 0x0201: bootloader reboot, such as OTA reboot
// 0x0601: software reboot, such as you called halReboot();
void autoResetCheckOrStartJoinNetwork(void)
{
	int8u autoResetMark = 0x00;
	int16u extendedResetInfo = halGetExtendedResetInfo();

	emberAfReadManufacturerSpecificServerAttribute(emberAfPrimaryEndpoint(),
 												ZCL_AUTO_RESET_CLUSTER_ID,
												ZCL_AUTO_RESET_ATTRIBUTE_ID,
												0x1160,
												(int8u *)&autoResetMark,
												sizeof(int8u));
	
#ifdef ROBIN_DEBUG	
	emberAfGuaranteedPrintln("autoResetCheckEventFunction :%x, extendedResetInfo:%2x",autoResetMark,extendedResetInfo);			// Robin add for debug
#endif

	if (extendedResetInfo == OTA_REBOOT_INFO)
	{
		// it's a bootloader reboot, eg. OTA reboot;
		// force a mark for if the older device do not support it and in the gateway, we must keep it
		autoResetMark = GATEWAY_MARKED;
		emberAfWriteManufacturerSpecificServerAttribute(emberAfPrimaryEndpoint(),
											ZCL_AUTO_RESET_CLUSTER_ID,
											ZCL_AUTO_RESET_ATTRIBUTE_ID,
											0x1160,
											(int8u *)&autoResetMark,
											ZCL_INT8U_ATTRIBUTE_TYPE);
		join_network_request();
		return;
	}

	if (autoResetMark == GATEWAY_MARKED || autoResetMark == NO_MARK)
	{
		// start to search network
		join_network_request();
	}
	else if(autoResetMark == ONLY_ZC_MARKED)
	{
		// autoResetMark = NO_MARK;
		// do the reset job without LED blink
		emberLeaveNetwork();
		int8u joinNetworkFirstTimeFlag = 0x00;
		halCommonSetToken(TOKEN_THE_FIRST_JOINED_FLAG, &joinNetworkFirstTimeFlag);

		emberAfResetAttributes(emberAfPrimaryEndpoint());
		emberAfGroupsClusterClearGroupTableCallback(emberAfPrimaryEndpoint());
		emberAfScenesClusterClearSceneTableCallback(emberAfPrimaryEndpoint());
		emberClearBindingTable();
		emberAfClearReportTableCallback();
		halReboot();		
	}

}

void diagnosticsEventFunction (void) 
{
    int16u tmp;

    tmp = emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_SUCCESS]+emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_FAILED]+emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_RETRY];
    if (tmp != 0) { 
        tmp = emberCounters[EMBER_COUNTER_MAC_TX_UNICAST_RETRY]/tmp;
    
        emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                               ZCL_DIAGNOSTICS_CLUSTER_ID,
                               ZCL_AVERAGE_MAC_RETRY_PER_APS_MSG_SENT_ATTRIBUTE_ID,
                               (int8u *)&tmp,
                               ZCL_INT16U_ATTRIBUTE_TYPE);
    }  
  
    emberEventControlSetDelayQS(diagnosticsEventControl, 240);  //60s
}

/*
**********************************************************************
* dat: 
* 01-10:   level: 10% -100%
* 11-20: cct: 10% -100%
* 99:      firmware version
**********************************************************************
*/
int8u firmware_version[] = "E1L-CCA-017";

boolean emberProcessCommandSendled (int8u *input, 
                                               int8u  sizeOrPort)
{
    int8u dat;
	
    while (EMBER_SUCCESS == emberSerialReadByte(sizeOrPort, &dat)) {
        if ((1 <= dat) && (dat <= 10)) {
            set_pwm_level(TICS_PER_PERIOD-TICS_PER_PERIOD/10*dat, DIMMING_CTRL_PIN, FALSE);
            emberSerialWriteData(APP_SERIAL, &dat, 1);
        }
        else if ((11 <= dat) && (dat <= 20)) { 
            set_pwm_level((dat-10)*TICS_PER_PERIOD/10, COLOR_TEMP_CTRL_PIN, FALSE);
            emberSerialWriteData(APP_SERIAL, &dat, 1);
        }
        else if (99 == dat) {
            emberSerialWriteData(APP_SERIAL, firmware_version, 
                                 sizeof(firmware_version));
        }
    }

    return FALSE;
}

void color_mode_selection (int8u mode)
{
#ifdef ROBIN_DEBUG  
    emberAfGuaranteedPrintln("Mode: %x",mode);        // Robin add for debug	
#endif

    if(mode == COLOR_3000K) {
        GPIO_PBCLR = BIT(7); 
    }
    else if(mode == COLOR_4000K) {
        GPIO_PBSET = BIT(7); 
    }
}


static int16s GetFilteredValue(int16s v, int8u index)
{
	static int16s buf[ADC_CHANNEL_NUM][12] = {0};
	static int8u ptr[ADC_CHANNEL_NUM] = {0};
	static int16s max, min;
	static int32s total;
	static int8u  i;
  
	buf[index][ptr[index]] = v;  
	max = buf[index][0];
	min = buf[index][0];
	total = buf[index][0];
	for (i=1; i<12; i++) {
    	total += buf[index][i];
    	if (buf[index][i] > max) {
			max = buf[index][i];}
    	else if (min > buf[index][i]) {
			min = buf[index][i];
		}
	}
  
	ptr[index]++;
	if (ptr[index] >= 12)
	ptr[index] = 0;

	total -= max;
	total -= min;
	total /= 10;

	return (int16s)total;
}


static void adc_init(void)
{
	adcChannelIndex = 0;
	halStartAdcConversion(ADC_USER_APP2, 
						ADC_REF_INT, 
						adcChannel[adcChannelIndex],
						ADC_CONVERSION_TIME_US_256);
}

static void adc_control (void)
{
	static int16u value;
	static int16s fvolts;
  
	if (halRequestAdcData(ADC_USER_APP2, &value) == EMBER_ADC_CONVERSION_DONE) {     
		fvolts = halConvertValueToVolts(value / TEMP_SENSOR_SCALE_FACTOR);
		adcData[adcChannelIndex] = GetFilteredValue(fvolts, adcChannelIndex);
#ifdef ROBIN_DEBUG  
//	    emberAfGuaranteedPrintln("value: %2x, fvolts: %2x, adcData[0]: %2x", value, fvolts, adcData[adcChannelIndex]); // Robin add for debug
#endif
	} 

	halStartAdcConversion(ADC_USER_APP2, 
						ADC_REF_INT, 
						adcChannel[adcChannelIndex],
						ADC_CONVERSION_TIME_US_256);     
}

void emberAfMainInitCallback (void)
{
    emberAfPluginPwmControlInitCallback();

    halGpioConfig(PORTB_PIN(3), GPIOCFG_OUT);

	// init set PB3 low according hardware request;
	// init set PA6 100%pwm according hardware request 
	// to avoid sudden blink when power up
	
	set_bulb_off();  
	emberAfPwmSetValuePA6(TICS_PER_PERIOD);

    adc_init();                                               // Adc Init
  
    get_aps_num_type();                                       // avoid APS sequence number init when power off and power on suddenly;
    
    //color_mode_selection(COLOR_3000K); 
}

#if 1
void sengledAfMainInit (void)
{  
	autoResetCheckOrStartJoinNetwork();
	
    delay_millisecond_tick(20);                         // hardware request by wangzhonghua
    program_run_start = TRUE;
	
#ifdef ROBIN_DEBUG  
    emberAfGuaranteedPrintln("30ms delay!!!");          // Robin add for debug	
#endif

    software_power_up();   
// software up avoid sudden blink
#ifdef ROBIN_DEBUG  
	emberAfGuaranteedPrintln("Powerup Finished");			// Robin add for debug	
#endif

    init_led_status(FALSE);
	halGpioConfig(PORTB_PIN(7), GPIOCFG_OUT_ALT);	
}
#else
EmberEventControl startShowLoopEventControl;
int8u step = 0;

void software_power_up_with_level (int8u level)
{	
	int8u  i = 0;
	int16u brightness_pwm;

	set_bulb_on();	
	for (i = 0; i < level-5; i=i+5)
	{
		brightness_pwm = TICS_PER_PERIOD - TICS_PER_PERIOD*i/DIMMING_MAX_LEVEL;
		emberAfPwmSetValuePA6(brightness_pwm);
		emberAfPwmSetValuePB7(0);
		delay_millisecond_tick(1);
	}
}


void startShowLoopEventFunction(void)
{
	switch (step)
	{
	case 1:
	{
		//set_bulb_on(); 
		//emberAfPwmSetValuePB7(150);
		//set_pwm_level(50, DIMMING_CTRL_PIN, TRUE);
		led_control(LED_ON, 128, COLOR_YELLOWEST);
	}
	break;
	
	case 2:
	{
		led_control(LED_ON, 128, COLOR_YELLOWEST);
	}
	break;

	case 3:
	{
		led_control(LED_ON, 128, COLOR_YELLOWEST);
	}
	break;

	case 4:
	{
		led_control(LED_ON, 128, COLOR_YELLOWEST);
	}
	break;

	case 5:
	{
		led_control(LED_ON, 255, COLOR_YELLOWEST);
	}
	break;

	case 6:
	{
		led_control(LED_ON, 255, COLOR_YELLOWEST);
	}
	break;

	default:
	{
		step = 1;
		emberEventControlSetActive(startShowLoopEventControl);	
		return;
	}
	break;
	}

	step++;
	// 39 means 39*256ms = 9984 ~= 10s
	emberEventControlSetDelayQS(startShowLoopEventControl, 39);
}

void sengledAfMainInit (void)
{  	
	//join_network_request(); 

    delay_millisecond_tick(20);                         // hardware request by wangzhonghua
    program_run_start = TRUE;
	//set_bulb_on();
	
    software_power_up_with_level(128);
    
	halGpioConfig(PORTB_PIN(7), GPIOCFG_OUT_ALT);

	step = 1;
	emberEventControlSetActive(startShowLoopEventControl);	
}

#endif
boolean emberAfMainStartCallback (int *returnCode,
                                        int  argc,
                                        char **argv)
{
    return FALSE;
}


void emberAfMainTickCallback (void)
{  
    adc_control();
    emberEventControlSetDelayQS(diagnosticsEventControl, 240);  //60s
}

void emberAfPluginBasicResetToFactoryDefaultsCallback(int8u endpoint)
{
}
void emberAfAutoResetClusterServerManufacturerSpecificAttributeChangedCallback(int8u endpoint,
                                                                               EmberAfAttributeId attributeId,
                                                                               int16u manufacturerCode)
{

}
