// *****************************************************************************
// * sengled-hardware-adc.c
// *
// * This code provides support for managing the hardware for CIA19.
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *****************************************************************************
#include "app/framework/include/af.h"
#include "sengled-adc.h"
#include "app/framework/plugin/sengled-hardware-Downlight/sengled-ledControl.h"

#define ADC_IN_VOLTAGE     ADC_SOURCE_ADC4_VREF2
#define ADC_IN_CURRENT     ADC_SOURCE_ADC3_VREF2
#define GetCurrent(c) (((c)*10)/27+10)
#define GetVoltage(c) ((int16s)(((int32s)(c)*3671)/10000))

static int8u adcChannelIndex;
const  int8u adcChannel[] = {ADC_IN_CURRENT, ADC_IN_VOLTAGE};
#define ADC_CHANNEL_NUM (sizeof(adcChannel)/sizeof(adcChannel[0]))
int16s adcData[ADC_CHANNEL_NUM] = {0};

int16u saveCurrent = 0;

int16s GetFilteredValue(int16s v, int8u index)
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
  for (i=1; i<12; i++)
  {
    total += buf[index][i];
    if (buf[index][i] > max)
    { max = buf[index][i];}
    else if (min > buf[index][i])
    { min = buf[index][i];}
  }
  
  ptr[index]++;
  if (ptr[index] >= 12)
    ptr[index] = 0;

  total -= max;
  total -= min;
  total /= 10;

  return (int16s)total;
}


void AdcControlInit(void)
{
  adcChannelIndex = 0;
  halStartAdcConversion(ADC_USER_APP2, 
						  ADC_REF_INT, 
						  adcChannel[adcChannelIndex],
						  ADC_CONVERSION_TIME_US_256);

}

void AdcControlFunction(void)
{
  static int16u value;
  static int16s fvolts;
  
  if(halRequestAdcData(ADC_USER_APP2, &value) == EMBER_ADC_CONVERSION_DONE)
  {     
    fvolts = halConvertValueToVolts(value / TEMP_SENSOR_SCALE_FACTOR);
    adcData[adcChannelIndex] = GetFilteredValue(fvolts, adcChannelIndex);
        
    adcChannelIndex++;
    if (adcChannelIndex >= ADC_CHANNEL_NUM)
    { 
      adcChannelIndex = 0;
	}
	
    halStartAdcConversion(ADC_USER_APP2, 
                          ADC_REF_INT, 
                          adcChannel[adcChannelIndex],
                          ADC_CONVERSION_TIME_US_256);    
                          
  } 
}

