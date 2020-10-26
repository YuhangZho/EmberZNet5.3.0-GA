// *******************************************************************
// * sengled-ledControl.c
// *
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *******************************************************************
#include "app/framework/include/af.h"
#include "app/framework/util/attribute-storage.h"
#include "sengled-ledControl.h"
#include "sengled-adc.h"


EmberEventControl cctDimmingEventControl;
EmberEventControl powerReportEventControl;
EmberEventControl powerConsumptionReportEventControl;
EmberEventControl offDelaySetPB6EventControl;
EmberEventControl onDelaySetPB3EventControl;


static boolean cctType;
static int16u saveCctPwm;
int8u currentLevel_delay = 0;
int16u colorTpt_delay = 0;
	
int16u saveCctLevel;

extern int16u saveCurrent;

int8u currentLevelForLedBreath;
int16u currentColorTptForLedBreath;

extern int16s adcData[2]; // 2 = ADC_CHANNEL_NUM

void delayMillisecondTick(int16u delayMillisecond)
{
	int16u nowTime, nextTime;

	nowTime = halCommonGetInt16uMillisecondTick();
	nextTime = halCommonGetInt16uMillisecondTick();
	while((nextTime - nowTime) < delayMillisecond)
	{
		halResetWatchdog(); 
		nextTime = halCommonGetInt16uMillisecondTick();
	}	
}

// level = COLOR_TEMPRATURE_MIN  y = -0.5446x2 + 295x + 39876
// level != COLOR_TEMPRATURE_MIN  y = -0.8855x2 + 423.32x + 27991
int32s GetEfficiency(int8u level)
{
  int32u tmp = level;

  if (saveCctLevel == COLOR_WHITEST)
  { tmp = 295*tmp+39876-(tmp*tmp*5446/10000);}
  else
  { tmp = (42332*tmp/100)+27991-(tmp*tmp*8855/10000);}

  if (tmp == 0)
  { tmp = 0xffffffff;}

  return tmp;
}


void powerReportFunction(void)
{
  int32s power=0;
  static int32s lastReportPower = 0;
  boolean isOn;
  int8u currentLevel;
  int16s currentIn, voltageIn;
  emberEventControlSetInactive(powerReportEventControl);
  
  emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                             ZCL_ON_OFF_CLUSTER_ID,
                             ZCL_ON_OFF_ATTRIBUTE_ID,
                             (int8u *)&isOn,
                             sizeof(boolean));
  emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                             ZCL_LEVEL_CONTROL_CLUSTER_ID,
                             ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                             (int8u *)&currentLevel,
                             sizeof(int8u));
  
  if(isOn == 0 || currentLevel == 0)
  {
    power = 0;
  }
  else
  {
    //Computing Power 
    currentIn = (adcData[0])*10/27+10; // GetCurrent
    currentIn = (currentIn<0)?0:currentIn;
    voltageIn = (int32s)(adcData[1])*3671/10000; //GetVoltage;
    voltageIn = (voltageIn<0)?0:voltageIn;  	
#ifdef ROBIN_DEBUG
//  emberAfGuaranteedPrintln("currentIn: %2x,voltageIn: %2x, adcData[1]: %2x",currentIn,voltageIn, adcData[1]); // Robin add for debug
#endif
    power = ((int32s)currentIn*voltageIn)/GetEfficiency(currentLevel);
  }

#ifdef ROBIN_DEBUG  
//  emberAfGuaranteedPrintln("power: %4x",power); // Robin add for debug
#endif

  if(lastReportPower > power)
  {
	if((lastReportPower - power) > 10 )
	{
		lastReportPower = power;
  		emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID,
                              (int8u *)&lastReportPower,
                              ZCL_INT24S_ATTRIBUTE_TYPE);		
	}
  }
  else
  {
	if((power - lastReportPower) > 10)
	{
		lastReportPower = power;
  		emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID,
                              (int8u *)&lastReportPower,
                              ZCL_INT24S_ATTRIBUTE_TYPE);		
	}
  }

  emberEventControlSetDelayQS(powerReportEventControl, 6);  // 1.5s
}

void powerConsumptionReportFunction(void)
{
  int32s currentPower=0;
  static int32s powerTotal;
  int64u powerConsumption=0;
  
  emberEventControlSetInactive(powerConsumptionReportEventControl);
  emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID,
                              (int8u *)&currentPower,
                              sizeof(int32s));
  
  emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
                              (int8u *)&powerConsumption,
                              sizeof(int64u));
#ifdef ROBIN_DEBUG  
//  emberAfGuaranteedPrintln("currentPower: %4x", currentPower); // Robin add for debug
#endif
  powerTotal += currentPower * 5; // 5s
  
#ifdef ROBIN_DEBUG  
//  emberAfGuaranteedPrintln("powerTotal: %d", powerTotal); // Robin add for debug
#endif
  currentPower = powerTotal / 3600;
  if(currentPower > 0)
  {
    powerConsumption += currentPower;
	powerTotal = powerTotal % 3600;
  }
#ifdef ROBIN_DEBUG  
//  emberAfGuaranteedPrintln("powerConsumption: %4x",powerConsumption); // Robin add for debug
#endif

  emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
                              (int8u *)&powerConsumption,
                              ZCL_INT48U_ATTRIBUTE_TYPE);
  
  emberEventControlSetDelayMS(powerConsumptionReportEventControl, 5000);  // 5s   
}
void emberAfPluginPwmControlInitCallback( void )
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

  //InitLedStatus(FALSE);  
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
void SetPwmLevel(int16u dutyCycle, PinSelectEnum pin, boolean powerReportFlag)
{
  switch (pin)
  {
    case PIN_PA6:
      emberAfPwmSetValuePA6(dutyCycle);
      break;
    case PIN_PB7:
      emberAfPwmSetValuePB7(dutyCycle);
  }
  
  // power changed report
  if(powerReportFlag == TRUE)
  {
    emberEventControlSetDelayMS(powerReportEventControl,1000);
  }
}

inline void SetOpenCircuit(void)    
{
	//GPIO_PBSET = BIT(3); 
	GPIO_PBSET |= (0x00000008u);
}

void SetCloseCircuit(void)   
{
  do 
  { 
    //GPIO_PBCLR = BIT(3); 
    GPIO_PBCLR |= (0x00000008u);
  } while (0);
}


void CctDimmingEventFunction(void) 
{
  if (cctType == CCT_TYPE_WHITE)
  {
    if (saveCctPwm > 3)
    { 
      saveCctPwm -= 3;
//      emberEventControlSetDelayMS(cctDimmingEventControl, 10);
    }
    else
    { 
      saveCctPwm = 0;
      emberEventControlSetInactive(cctDimmingEventControl);
    }
  }
  else if (cctType == CCT_TYPE_YELLOW)
  {
    if (saveCctPwm < TICS_PER_PERIOD-3)
    { 
      saveCctPwm += 3;
//      emberEventControlSetDelayMS(cctDimmingEventControl, 10);
    }
    else
    { 
      saveCctPwm = TICS_PER_PERIOD;
      emberEventControlSetInactive(cctDimmingEventControl);
    }
  }
#ifdef ROBIN_DEBUG	
  emberAfGuaranteedPrintln("CctDimmingEvent: %4x",saveCctPwm); // Robin add for debug
#endif	    
//  SetPwmLevel((int16u)saveCctPwm, COLOR_TEMP_CTRL_PIN, FALSE);
}


void SetLedLevel(int8u level)
{
  int16u dimLevel;

  dimLevel = TICS_PER_PERIOD*level/DIMMING_MAX_LEVEL;

  dimLevel  = dimLevel * 90 / 100  + dimLevel * 5 / 100 ;
  if(dimLevel > TICS_PER_PERIOD)
  {
	dimLevel = TICS_PER_PERIOD;
  }
  
#ifdef ROBIN_DEBUG	
	emberAfGuaranteedPrintln("LevelPwm: %2x",dimLevel); // Robin add for debug
#endif	 

  SetPwmLevel((int16u)dimLevel, DIMMING_CTRL_PIN, TRUE);  
}

void SetLedCct(int16u colorTpt)
{
  int32u cctPwm;
  int8u currentLevel;	
  emberEventControlSetInactive(cctDimmingEventControl);
  
  saveCctLevel = colorTpt;	

  if (colorTpt <= COLOR_WHITEST)
  { 
    cctType = CCT_TYPE_WHITE;
//    emberEventControlSetDelayMS(cctDimmingEventControl, 10);
    return;
  } 

  if (cctPwm > TICS_PER_PERIOD)
  { 
    cctPwm = TICS_PER_PERIOD;
  }

  saveCctPwm = cctPwm;
  
#ifdef ROBIN_DEBUG	
  emberAfGuaranteedPrintln("cctPwm: %4x",cctPwm); // Robin add for debug
#endif	  
  // tracklight debug
  cctPwm = COLOR_WHITEST;
  SetPwmLevel((int16u)cctPwm, COLOR_TEMP_CTRL_PIN, TRUE);
}

void SetLed(boolean onOffStatus, int8u currentLevel, int16u colorTpt)
{
  currentLevel_delay = currentLevel;
  colorTpt_delay = colorTpt;

  if(onOffStatus == LED_ON && currentLevel != 0x00)
  {
	//SetOpenCircuit();
	//delayMillisecondTick(30);
	SetLedLevel(currentLevel_delay);
    SetLedCct(colorTpt_delay);	
	GPIO_PBSET = BIT(6);

  	//emberEventControlSetDelayMS(onDelaySetPB3EventControl,30);
  }
  else if(onOffStatus == LED_OFF || currentLevel == 0x00)
  {
	//SetCloseCircuit();
	//GPIO_PBCLR = BIT(6);
	SetPwmLevel(0, DIMMING_CTRL_PIN, TRUE);	
	emberEventControlSetDelayMS(powerReportEventControl,1000);
  }
}

void onDelaySetPB3EventFunction(void)
{
#ifdef ROBIN_DEBUG
	emberAfGuaranteedPrintln("onDelaySetPB3EventFunction"); // Robin add for debug
#endif

	emberEventControlSetInactive(onDelaySetPB3EventControl);		
	SetLedLevel(currentLevel_delay);
    SetLedCct(colorTpt_delay);	
	GPIO_PBSET = BIT(6);
}

void offDelaySetPB6EventFunction(void)
{
#ifdef ROBIN_DEBUG
	emberAfGuaranteedPrintln("offDelaySetPB6EventFunction"); // Robin add for debug
#endif
	SetOpenCircuit();

	emberEventControlSetInactive(offDelaySetPB6EventControl);	
}


void ColorTemperatureCompensation(void)
{
  int32u cctPwm, k, b;

  if (saveCctLevel >= COLOR_YELLOWEST)
  { cctPwm = TICS_PER_PERIOD;}
  else if (saveCctLevel <= COLOR_WHITEST)
  { return;}
  else
  {
    k = 41-632*saveCctLevel/10000;
    b = 5293+4722*saveCctLevel/10000;
    
    cctPwm = k*saveCurrent/10;
    if (b >= cctPwm)
    { cctPwm = b - cctPwm;}
    else
    { cctPwm = 0;}
  }

  if (cctPwm > TICS_PER_PERIOD)
  { cctPwm = TICS_PER_PERIOD;}

  saveCctPwm = cctPwm;
  
//  SetPwmLevel((int16u)cctPwm, COLOR_TEMP_CTRL_PIN, FALSE);
}

void InitLedStatus(boolean firstjoinedNetworkFlag)
{
  int8u currentLevel = DIMMING_MAX_LEVEL;
  int16u currentColorTemperature = COLOR_YELLOWEST;
  int8u onLevel = 0xff; // by default 
  boolean onOff = 1;
  int16u extendedResetInfo = halGetExtendedResetInfo();
  
  if(firstjoinedNetworkFlag == 0)
  {
    emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_LEVEL_CONTROL_CLUSTER_ID,
                              ZCL_ON_LEVEL_ATTRIBUTE_ID,
                              (int8u *)&onLevel,
                              sizeof(int8u)); 
    if(onLevel == 0xff)
	{
      emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
								  ZCL_LEVEL_CONTROL_CLUSTER_ID,
								  ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
								  (int8u *)&currentLevel,
								  sizeof(int8u)); 
	}
	else
	{
      currentLevel = onLevel;
	}
	
	emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                                ZCL_COLOR_CONTROL_CLUSTER_ID,
                                ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
                                (int8u *)&currentColorTemperature,
                                sizeof(int16u));
  }
#ifdef ROBIN_DEBUG	
  emberAfGuaranteedPrintln("onL: %x, currentL: %x, color: %2x",onLevel, currentLevel,currentColorTemperature); // Robin add for debug
#endif

  currentLevelForLedBreath = currentLevel;
  currentColorTptForLedBreath  = currentColorTemperature;

  emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
								 ZCL_ON_OFF_CLUSTER_ID,
								 ZCL_ON_OFF_ATTRIBUTE_ID,
								 (int8u *)&onOff,
								 sizeof(boolean));	

  if(extendedResetInfo != 0x0301 && onOff == 0)
  {
	// it may be a ota finished reboot, if the lamp is off, then keep it off;
	SetLed(LED_OFF, currentLevel, currentColorTemperature);	
  }
  else
  {
  	onOff = 1;
  	emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
					          ZCL_ON_OFF_CLUSTER_ID,
					          ZCL_ON_OFF_ATTRIBUTE_ID,
					          (int8u *)&onOff,
					          ZCL_BOOLEAN_ATTRIBUTE_TYPE);  
#ifdef ROBIN_DEBUG	
  emberAfGuaranteedPrintln("SETLED-ON: InitLEDStatus"); // Robin add for debug
#endif  
  	SetLed(LED_ON, currentLevel, currentColorTemperature);	
  }

}

void RecoverLedRecordedStatus(void)
{

}


void emberAfOnOffClusterServerAttributeChangedCallback(int8u endpoint, 
                                                       EmberAfAttributeId attributeId)
{
  boolean onOffStatus = 0;
  int8u currentLevel = 0x00;
  int16u currentColorTemperature = 0x00;
  emberAfReadServerAttribute(endpoint,
                             ZCL_ON_OFF_CLUSTER_ID,
                             ZCL_ON_OFF_ATTRIBUTE_ID,
                             (int8u *)&onOffStatus,
                             sizeof(boolean));

  if(onOffStatus == LED_ON)
  {
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
#ifdef ROBIN_DEBUG
	emberAfGuaranteedPrintln("SetLED-ON:OnoffCB: %x, L: %x, C: %2x",onOffStatus,currentLevel,currentColorTemperature); // Robin add for debug
#endif
	SetLed(LED_ON, currentLevel, currentColorTemperature);
  }
  else if(onOffStatus == LED_OFF)
  {
#ifdef ROBIN_DEBUG
	emberAfGuaranteedPrintln("OnoffCB: off"); // Robin add for debug
#endif
	SetLed(LED_OFF, currentLevel, currentColorTemperature);
  }
}

void emberAfLevelControlClusterServerAttributeChangedCallback(int8u endpoint,
                                                              EmberAfAttributeId attributeId)
{
  int8u currentLevel;
  emberAfReadServerAttribute(endpoint,
							ZCL_LEVEL_CONTROL_CLUSTER_ID,
							ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
							(int8u *)&currentLevel,
							sizeof(int8u));

#ifdef ROBIN_DEBUG
  emberAfGuaranteedPrintln("LevelCB: crt:%x",currentLevel); // Robin add for debug	
#endif
  SetLedLevel(currentLevel);	
}

void emberAfColorControlClusterServerAttributeChangedCallback(int8u endpoint,
                                                              EmberAfAttributeId attributeId)
{
  int16u currentColorTemperature;

  emberAfReadServerAttribute(endpoint,
                              ZCL_COLOR_CONTROL_CLUSTER_ID,
                              ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
                              (int8u *)&currentColorTemperature,
                              sizeof(int16u));
#ifdef ROBIN_DEBUG  
  emberAfGuaranteedPrintln("ColorCB: %2x",currentColorTemperature); // Robin add for debug	
#endif
  saveCctLevel = currentColorTemperature;
  SetLedCct(currentColorTemperature);  
}




