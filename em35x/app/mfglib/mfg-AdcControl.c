//#include "app/framework/include/af.h"
//#include "app/framework/util/attribute-storage.h"
#include PLATFORM_HEADER //compiler/micro specifics, types
#include "stack/include/ember.h"
#include "stack/include/mfglib.h"
#include "hal/hal.h"
#include "app/util/serial/serial.h"
#include "app/util/serial/cli.h"



//****************************************************************
//ADC
//****************************************************************
static int8u adcChannelIndex;
const  int8u adcChannel[] = {ADC_SOURCE_ADC1_VREF2, ADC_SOURCE_ADC3_VREF2, ADC_SOURCE_ADC4_VREF2, ADC_SOURCE_ADC0_VREF2};
                            // ADC0 -ADC5: PB5,PB6,PB7,PC1,PA4,PA5

#define ADC_CHANNEL_NUM (sizeof(adcChannel)/sizeof(adcChannel[0]))


int16s fvolts[ADC_CHANNEL_NUM] = {0};

//**************************************************************
void AdcControlInit(void)
{
  halGpioConfig(PORTB_PIN(5),GPIOCFG_ANALOG);
  halGpioConfig(PORTC_PIN(1),GPIOCFG_ANALOG);
  halGpioConfig(PORTA_PIN(4),GPIOCFG_ANALOG);
  halGpioConfig(PORTB_PIN(6),GPIOCFG_ANALOG);
  
  adcChannelIndex = 0;
  halStartAdcConversion(ADC_USER_APP2, 
                        ADC_REF_INT, 
                        adcChannel[adcChannelIndex],
                        ADC_CONVERSION_TIME_US_256);
}

int8u GetEfficiencyPtr(int8u level)
{
  if (level <= 0x33)
    return 4;
  else if (level <= 0x66)
    return 3;
  else if (level <= 0x99)
    return 2;
  else if (level <= 0xcc)
    return 1;
  else
    return 0;
}
//****************************************************************
//****************************************************************
// Convert volts to celsius in LM20 temp sensor.  The numbers
// are both in fixed point 4 digit format. I.E. 860 is 0.0860
static int16s voltsToCelsius (int16u voltage)
{  
#if 0
  // equation: temp = -0.17079*mV + 159.1887
  //return 1591887L - (171 * (int32s)voltage);
  int32s tmp = (171 * (int32s)voltage);
  tmp /= 10000;
  
  return 159 - tmp;
#else
  int32s temp;

  voltage /= 10;

  // -40 ~ -16
  if (voltage < 40)
  {
    temp = (-158 * voltage * voltage + 15101 * voltage - 514290) / 10000; 
  }
  //-15 ~ 0
  else if (voltage < 91)
  { 
    temp = (-19 * voltage * voltage + 5579 * voltage - 347260) / 10000; 
  }
  // 0~30
  else if (voltage < 315)
  {   
    temp = (-2 * voltage * voltage + 2322 * voltage - 185860) / 10000; 
  }
  //31~ 60
  else if (voltage < 670)
  { 
    temp = (842 * voltage + 43373) / 10000; 
  }
  // 61 ~ 75
  else if (voltage < 830)
  { 
    temp = (913 * voltage - 7245) / 10000; 
  } 
  // 75 ~ 85
  else if (voltage < 920)
  { 
    temp = (1089 * voltage - 152600) / 10000; 
  } 
  // 86 ~ 100
  else if (voltage < 1030)
  { 
    temp = (2 * voltage * voltage - 2862 * voltage + 1800000) / 10000; 
  }
  // 101 ~ 110
  else if (voltage < 1082)
  { 
    temp = (1879 * voltage - 932270) / 10000; 
  }
  // 110 ~ 125
  else if (voltage < 1200)
  { 
    temp = (8 * voltage * voltage - 16165 * voltage + 9229100) / 10000; 
  }

  return temp;
#endif
}

//**********************************************
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

//****************************************************************
//****************************************************************

/** @brief Main Tick
 *
 * Whenever main application tick is called, this callback will be called at
 * the end of the main tick execution.
 *
 */
void AdcControlFunction(void)
{
  static int16u value;
  static int16s tmp;  
  
  if (halRequestAdcData(ADC_USER_APP2, &value) == EMBER_ADC_CONVERSION_DONE)
  {     
    tmp = halConvertValueToVolts(value / TEMP_SENSOR_SCALE_FACTOR);
    if (adcChannelIndex == 3)
    {
      fvolts[adcChannelIndex] = voltsToCelsius(GetFilteredValue(tmp, adcChannelIndex));
    }
    else
    {
      fvolts[adcChannelIndex] = GetFilteredValue(tmp, adcChannelIndex);
    }  
    
    adcChannelIndex++;
    if (adcChannelIndex >= ADC_CHANNEL_NUM)
    { adcChannelIndex = 0;}
    halStartAdcConversion(ADC_USER_APP2, 
                          ADC_REF_INT, 
                          adcChannel[adcChannelIndex],
                          ADC_CONVERSION_TIME_US_256);    
  } 
}



