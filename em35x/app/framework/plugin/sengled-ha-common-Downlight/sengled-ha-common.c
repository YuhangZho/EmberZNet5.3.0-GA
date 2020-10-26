// *******************************************************************
// * sengled-ha-common.c
// *
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *******************************************************************
#include "app/framework/include/af.h"
#include "sengled-ha-common.h"
#include "app/framework/plugin/sengled-hardware-Downlight/sengled-adc.h"
#include "app/framework/plugin/counters/counters.h"
#include "sengled-ha-token.h"

//int16u dimmerWaveFormCounter;
int8u powerUpFirstTime;

EmberEventControl dimmerDetectControl;
EmberEventControl diagnosticsEventControl;


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
int8u firmwareVersion[] = "Z01_A19EUE27_V014_161123";
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
/*
  //start from a fresh state just in case
  BUTTON1_INTCFG = 0;              //disable BUTTON1 triggering
  INT_CFGCLR = BUTTON1_INT_EN_BIT; //clear BUTTON1 top level int enable
  INT_GPIOFLAG = BUTTON1_FLAG_BIT; //clear stale BUTTON1 interrupt
  INT_MISS = BUTTON1_MISS_BIT;     //clear stale missed BUTTON1 interrupt
*/
}

void emberAfMainInitCallback(void)
{
  emberAfPluginPwmControlInitCallback();
}

void sengledAfMainInit(void)
{
  // open dimmer when power on
  SetOpenCircuit();	
  
  // request to join network when power on 
  JoinNetworkRequest();

  // Adc Init
  AdcControlInit();

  // avoid APS sequence number init when power off and power on suddenly;
  GetTheApsNumType();
  
  // start to detct dimmer, by defalut dimmer is open, and if dimmer is open,
  // doubleClick should be disable;
  //dimmerWaveFormCounter = 0;
  //emberEventControlSetDelayMS(dimmerDetectControl, 1200);

  halCommonGetToken(&powerUpFirstTime, TOKEN_THE_FIRST_POWERUP);
#ifdef ROBIN_DEBUG	
	  emberAfGuaranteedPrintln("_____INITPFirstTime:%x",powerUpFirstTime); // Robin add for debug
#endif	

  softwarePowerUp();

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
