// *******************************************************************
// * sengled-ha-common.c
// *
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *******************************************************************
#include "app/framework/include/af.h"
#include "sengled-ha-common.h"
#include "app/framework/plugin/sengled-hardware-Highbay/sengled-adc.h"
#include "app/framework/plugin/counters/counters.h"
#include "app/framework/plugin/sengled-ha-cli/sengled-cli.h"

//boolean doubleClickEnable = FALSE;
int16u dimmerWaveFormCounter;

EmberEventControl dimmerDetectControl;
EmberEventControl diagnosticsEventControl;
EmberEventControl powerOffDetectEventControl;

void GetTheApsNumType(void)
{ 
  int8u num;

  halCommonGetToken(&num, TOKEN_THE_APSNUM_TYPE);
  SengledSetSequenceNumber(num+1);
}
void SetTheApsNumType(int8u num)
{ 
  halCommonSetToken(TOKEN_THE_APSNUM_TYPE, &num);
}


void diagnosticsEventFunction(void) 
{
  int16u tmp;

  tmp = emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_SUCCESS]+emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_FAILED]+emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_RETRY];
  if (tmp != 0)
  { 
    tmp = emberCounters[EMBER_COUNTER_MAC_TX_UNICAST_RETRY]/tmp;
    
    emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                               ZCL_DIAGNOSTICS_CLUSTER_ID,
                               ZCL_AVERAGE_MAC_RETRY_PER_APS_MSG_SENT_ATTRIBUTE_ID,
                               (int8u *)&tmp,
                               ZCL_INT16U_ATTRIBUTE_TYPE);
  }  
  
  emberEventControlSetDelayQS(diagnosticsEventControl, 240);  //60s
}

//*******************************************************************************
// 1-10: level: 10% -100%
// 11-20: cct: 10% -100%
// 99: firmware version
//*******************************************************************************
int8u firmwareVersion[] = "Z01_TrackLight";
boolean emberProcessCommandSendled(int8u *input, int8u sizeOrPort)
{
  int8u dat;
  
  while (EMBER_SUCCESS == emberSerialReadByte(sizeOrPort, &dat))
  {
    if ((1 <= dat) && (dat <= 10)) 
    {
      SetPwmLevel(TICS_PER_PERIOD-600*dat, DIMMING_CTRL_PIN, FALSE);
      emberSerialWriteData(APP_SERIAL, &dat, 1);
    }
    else if ((11 <= dat) && (dat <= 20))
    { 
      SetPwmLevel((dat-10)*600, COLOR_TEMP_CTRL_PIN, FALSE);
      emberSerialWriteData(APP_SERIAL, &dat, 1);
    }
    else if (99 == dat)
    { emberSerialWriteData(APP_SERIAL, firmwareVersion, sizeof(firmwareVersion));}
  }

  return FALSE;
}


void DimmerDetectFunction(void)
{
  emberEventControlSetInactive(dimmerDetectControl);
#ifdef ROBIN_DEBUG  
  emberAfGuaranteedPrintln("DimmerDetectFunction"); // Robin add for debug
#endif
/*
  if(dimmerWaveFormCounter <= DIMMER_CONNECTED_LINE)
  {
    // there is no dimmer connetced, so we also enable doubelClick
	SetCloseCircuit();
	doubleClickEnable = TRUE;
  }
  dimmerWaveFormCounter = 0;
*/
/*
  //start from a fresh state just in case
  BUTTON1_INTCFG = 0;              //disable BUTTON1 triggering
  INT_CFGCLR = BUTTON1_INT_EN_BIT; //clear BUTTON1 top level int enable
  INT_GPIOFLAG = BUTTON1_FLAG_BIT; //clear stale BUTTON1 interrupt
  INT_MISS = BUTTON1_MISS_BIT;     //clear stale missed BUTTON1 interrupt
*/ 
}

#define FORBIDDEN_ENTER 0
#define PERMIT_ENTER 1
static int8u powerOffEnterCondition = FORBIDDEN_ENTER;
static int8u powerOnEnterCondition = PERMIT_ENTER;

extern int8u doubleClickCount;
extern boolean doubleClickStart;
extern boolean doubleClickEnd;
extern EmberEventControl  onOffStatusWhenPowerOffEventControl;
extern EmberEventControl  doubleClickEventControl;
void powerOffDetectEventFunction(void)
{
	emberEventControlSetDelayMS(powerOffDetectEventControl,5);
  
	static int8u offCount;
	int32u inputResult;
	int8u onOffStatusWhenPowerOff = 0x00;
	
	inputResult = GPIO_PBIN & 0x00000008; 

#ifdef ROBIN_DEBUG  
//	emberAfGuaranteedPrintln("@@@@input: %4x",inputResult); // Robin add for debug
//	emberAfGuaranteedPrintln("   @@@@powerOffCdt,powerOnCdt: %x,%x",powerOffEnterCondition,powerOnEnterCondition); // Robin add for debug
#endif

	if(inputResult != 0)
	{
		//power on
		powerOffEnterCondition = PERMIT_ENTER;
		offCount = 0;

		// power on need enter only once until enter power off;
		if(powerOnEnterCondition == PERMIT_ENTER)
		{
			powerOnEnterCondition = FORBIDDEN_ENTER;
			if(doubleClickStart == TRUE)
		  	{
		  	#ifdef ROBIN_DEBUG  
				emberAfGuaranteedPrintln("===POWER-ON-TRIGGER==="); // Robin add for debug
			#endif
		  		doubleClickEnd = TRUE;
				doubleClickCount++;
				onOffStatusWhenPowerOff = 1;
				emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
											ZCL_ON_OFF_CLUSTER_ID,
											ZCL_ON_OFF_ATTRIBUTE_ID,
											(int8u *)&onOffStatusWhenPowerOff,
											ZCL_BOOLEAN_ATTRIBUTE_TYPE);  

				emberEventControlSetDelayMS(doubleClickEventControl,1300);		
			}			
		}
	}
	else
	{
		offCount++;
	}

	// power off, if power off, then do not enter any more until power on,
	if(powerOffEnterCondition == PERMIT_ENTER)
	{
		if(offCount >= 12)  // 60ms
		{
			powerOffEnterCondition = FORBIDDEN_ENTER;
			offCount = 0;
		#ifdef ROBIN_DEBUG  
			emberAfGuaranteedPrintln("===POWER-OFF-TRIGGER==="); // Robin add for debug
		#endif	
		
			powerOnEnterCondition = PERMIT_ENTER;

			doubleClickStart = TRUE;
	  		doubleClickEnd = FALSE;

	  		SetTheApsNumType(emberAfGetLastSequenceNumber());

	  		sengledGlobalReportCommand(ZCL_ON_OFF_CLUSTER_ID,ZCL_ON_OFF_ATTRIBUTE_ID, 
								ZCL_BOOLEAN_ATTRIBUTE_TYPE, CLUSTER_IS_SERVER);
	  		emAfSengledSendCommand(COORDINATOR_NODE_ID);	  
	  		emberEventControlSetDelayMS(onOffStatusWhenPowerOffEventControl,150);
		}
	}
}

void emberAfMainInitCallback(void)
{
  emberAfPluginPwmControlInitCallback();
}

void sengledAfMainInit(void)
{  
  halGpioConfig(PORTB_PIN(3), GPIOCFG_IN);

  emberEventControlSetActive(powerOffDetectEventControl);	// 

  // request to join network when power on 
  JoinNetworkRequest();

  // Adc Init
  AdcControlInit();
  
  // avoid APS sequence number init when power off and power on suddenly;
  GetTheApsNumType();  

  // start to detct dimmer, by defalut dimmer is open, and if dimmer is open,
  // doubleClick should be disable;
  dimmerWaveFormCounter = 0;
  //doubleClickEnable = FALSE; 
  //emberEventControlSetDelayMS(dimmerDetectControl, 1200);

  // 2016.4.20
  InitLedStatus(FALSE);
}

#ifdef EMBER_SOC

boolean emberAfMainStartCallback(int* returnCode,
                                 int argc,
                                 char** argv)
{
	return FALSE;
}
#endif

void emberAfMainTickCallback(void)
{  
  AdcControlFunction();
  emberEventControlSetDelayQS(diagnosticsEventControl, 240);  //60s
}

void emberAfPluginBasicResetToFactoryDefaultsCallback(int8u endpoint)
{
}
