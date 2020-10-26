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

EmberEventControl diagnosticsEventControl;

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
int8u disable_permit_join_from_device = FALSE;

#ifdef SUPPORT_ADC
#define ADC_IN_VOLTAGE     ADC_SOURCE_ADC4_VREF2
static int8u adcChannelIndex = 0;
const  int8u adcChannel[] = {ADC_IN_VOLTAGE};
#define ADC_CHANNEL_NUM (sizeof(adcChannel)/sizeof(adcChannel[0]))
int16s adcData[ADC_CHANNEL_NUM] = {0};
#endif

#define GetCurrent(c) (((c)*10)/27+10)
#define GetVoltage(c) ((int16s)(((int32s)(c)*3671)/10000))

int8u on_off_remember_when_power_off = 0;
extern int8u joinNetworkFirstTimeFlag;;
extern boolean searchingNetworkStatus;
extern boolean stopBlinkForeverEvent;

void NewDeviceJoinReport(EmberNodeId nodeId, EmberEUI64 nodeEUI64)
{ 
}

int8u get_on_off_status (void)
{ 
    int8u on_off;

	halCommonGetToken(&on_off, TOKEN_ON_OFF_STATUS);
	return on_off;
}

void set_on_off_status (int8u on_off)
{ 
    halCommonSetToken(TOKEN_ON_OFF_STATUS, &on_off);
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

/////////////////////		Factory Production		/////////////////////
/****************************************************************
* int8u dat: 
* 01-10:   level: 10% -100%
* 11-20: cct: 10% -100%
* 99:      firmware version
*****************************************************************/
int8u firmware_version[] = "E1C-NB6-V0.0.22";

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
        else if (GET_FIRMWARE_VER == dat) {
            emberSerialWriteData(APP_SERIAL, firmware_version, 
                                 sizeof(firmware_version));
        }
    }

    return FALSE;
}

#ifdef SUPPORT_ADC
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
	} 

	halStartAdcConversion(ADC_USER_APP2, 
						ADC_REF_INT, 
						adcChannel[adcChannelIndex],
						ADC_CONVERSION_TIME_US_256);     
}
#endif

/////////////////		Program Init   	/////////////////
void emberAfMainInitCallback (void)
{
	int8u sengled_install_code[20]={0x06,0x00,
						 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
						 0x00,0x64,0x65,0x6C,0x67,0x6E,0x65,0x73,
						 0x8D,0xA0};
	int8u default_install_code[20] ={0xff,0xff,
							0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
							0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
							0xff,0xff};
	
	halInternalInitButton();
	emberAfPluginPwmControlInitCallback();

	halGpioConfig(PORTB_PIN(3), GPIOCFG_OUT);

#ifdef SUPPORT_ADC
	adc_init(); 
#endif
	
	on_off_remember_when_power_off = get_on_off_status();
	halCommonGetToken(&joinNetworkFirstTimeFlag, TOKEN_THE_FIRST_JOINED_FLAG);

	sengledGuaranteedPrint("joinNetworkFirstTimeFlag: %x",joinNetworkFirstTimeFlag);		
	
	halCommonGetMfgToken(&default_install_code, TOKEN_MFG_INSTALLATION_CODE);
	if(default_install_code[0] == 0xff && default_install_code[1] == 0xff) // flag .0xffff means not written
	{
		halCommonSetMfgToken(TOKEN_MFG_INSTALLATION_CODE, &sengled_install_code);
	}
#ifdef INSTALL_CODE_DEBUG	
	halCommonGetMfgToken(&sengled_install_code, TOKEN_MFG_INSTALLATION_CODE);
	sengledGuaranteedPrintln("install code:");
	for(int8u i = 0; i < 20; i++)
	{
		// flag:0x02,0x00 ---install_code[0],install_code[1]
		// code: 0x73,0x65,0x6E,0x67,0x6C,0x65,0x64,0x00 ---install_code[2]-[9]
		sengledGuaranteedPrint("0x%x ",sengled_install_code[i]);
		
	}
#endif
		
}

void sengledAfMainInit (void)
{  
    program_run_start = TRUE;
	
	sengledGuaranteedPrintln("Powerup Finished");

	if (on_off_remember_when_power_off == 0)
	{
		set_bulb_off();
	}
	else
	{
		// set plug on
		set_bulb_on();
	}

	// init mode
	setLedPulseMode(M1);
}

void emberAfMainTickCallback (void)
{  
#ifdef SUPPORT_ADC
    adc_control();
#endif
    emberEventControlSetDelayQS(diagnosticsEventControl, 240);  //60s
}


/** @brief Get Distributed Key
 *
 * This callback is fired when the Network Steering plugin needs to set the distributed
 * key. The application set the distributed key from Zigbee Alliance thru this callback
 * or the network steering will use the default test key.
 *
 * @param pointer to the distributed key struct
 * @return true if the key is loaded successfully, otherwise false.
 * level. Ver.: always
 */
boolean emberAfPluginNetworkSteeringGetDistributedKeyCallback(EmberKeyData * key)
{
}


/////////////////		Future Use		/////////////////

boolean emberAfMainStartCallback (int *returnCode,
                                        int  argc,
                                        char **argv)
{
    return FALSE;
}

void emberAfPluginBasicResetToFactoryDefaultsCallback(int8u endpoint)
{
}
