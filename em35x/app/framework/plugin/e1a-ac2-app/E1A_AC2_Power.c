
#include "E1A_AC2_Public_Head.h"


//****************************************************************
// Power and Consumption
// ADC0 -ADC5: PB5,PB6,PB7,PC1,PA4,PA5
//****************************************************************
#define ADC_IN_VOLTAGE     ADC_SOURCE_ADC4_VREF2
#define ADC_IN_CURRENT     ADC_SOURCE_ADC3_VREF2

#define GetCurrent(c) (((c)*10)/27+10)
#define GetVoltage(c) ((int16s)(((int32s)(c)*3671)/10000))

static int8u adcChannelIndex;
const  int8u adcChannel[] = {ADC_IN_CURRENT, ADC_IN_VOLTAGE};
#define ADC_CHANNEL_NUM (sizeof(adcChannel)/sizeof(adcChannel[0]))

EmberEventControl PowerConsumptionEventControl;
static int16s adcData[ADC_CHANNEL_NUM] = {0};
int8u powerCount = POWER_COUNT_MAX;
int16u saveCurrent;


// level >= 125  y = 44.998x + 65762
// level < 125    y = 201.6x + 46029
int32s GetEfficiency(int8u level)
{
  if (level >= 125)
  { return (int32u)((level*45)+65762);}
  else
  { return (int32u)((level*2016)/10+46029);}
}

//
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

//
void AdcControlFunction(void)
{
  static int16u value;
  static int16s fvolts;
      
  if (halRequestAdcData(ADC_USER_APP2, &value) == EMBER_ADC_CONVERSION_DONE)
  {     
    fvolts = halConvertValueToVolts(value / TEMP_SENSOR_SCALE_FACTOR);
    adcData[adcChannelIndex] = GetFilteredValue(fvolts, adcChannelIndex);
    
    adcChannelIndex++;
    if (adcChannelIndex >= ADC_CHANNEL_NUM)
    { adcChannelIndex = 0;}
    halStartAdcConversion(ADC_USER_APP2, 
                          ADC_REF_INT, 
                          adcChannel[adcChannelIndex],
                          ADC_CONVERSION_TIME_US_256);    
  } 
}

void PowerConsumptionEventFunction(void) 
{
  static boolean firstFlag = TRUE;
  static int16u shortPower = 0;
  int16s currentIn, voltageIn;
  int8u  currentLevel;
  int32s power, temp;
  static int32s powerSave = 0;
  boolean isOn;  
  static int64u powerConsumption;

  if (firstFlag == TRUE)
  {
    int8u readData[6];
    
    firstFlag = FALSE;

    emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
                              readData,
                              6);    
    readData[4] = 0;
    readData[5] = 0;
    emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
                              readData,
                              ZCL_INT48U_ATTRIBUTE_TYPE);
   
    emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
                              (int8u *)&powerConsumption,
                              6);
  }
  //Computing Power 
  currentIn = GetCurrent(adcData[0]);
  currentIn = (currentIn<0)?0:currentIn;
  voltageIn = GetVoltage(adcData[1]);
  voltageIn = (voltageIn<0)?0:voltageIn;
  //emberAfGuaranteedPrint("c=%d, v=%d, ",currentIn, voltageIn);

  emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                             ZCL_LEVEL_CONTROL_CLUSTER_ID,
                             ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                             (int8u *)&currentLevel,
                             sizeof(int8u));
  emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                             ZCL_ON_OFF_CLUSTER_ID,
                             ZCL_ON_OFF_ATTRIBUTE_ID,
                             (int8u *)&isOn,
                             sizeof(boolean));
  if (currentLevel >= 10)
  { power = ((int32s)currentIn*voltageIn)/GetEfficiency(currentLevel);}
  else
  { power = 13;}    
  
  if ((isOn == 0) || (currentLevel==DIMMING_MIN_LEVEL))
  { power = 0;}

  //emberAfGuaranteedPrint("p=%d; s=%d; C=%d",power, powerSave, powerCount);  
  if (powerCount != 0)
  {
    powerCount--;
    if (powerCount == 0)
    {
      powerSave = power;
      emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                                  ZCL_SIMPLE_METERING_CLUSTER_ID,
                                  ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID,
                                  (int8u *)&power,
                                  ZCL_INT24S_ATTRIBUTE_TYPE);
    }
  }
  else
  { powerCount = POWER_COUNT_MAX+1;}
  //emberAfGuaranteedPrint("\r\n");

  //Computing Consumption
  shortPower += power;
  if (shortPower >= 3600)
  {
    shortPower -= 3600;
    powerConsumption++;
    
    emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_SIMPLE_METERING_CLUSTER_ID,
                              ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
                              (int8u *)&powerConsumption,
                              ZCL_INT48U_ATTRIBUTE_TYPE);
  }
  
  emberEventControlSetDelayMS(PowerConsumptionEventControl, 1000);
}

// PowerAndConsumptionInit
void PowerAndConsumptionInit(void)
{
  adcChannelIndex = 0;
  halStartAdcConversion(ADC_USER_APP2, 
                        ADC_REF_INT, 
                        adcChannel[adcChannelIndex],
                        ADC_CONVERSION_TIME_US_256);
    
  emberEventControlSetDelayMS(PowerConsumptionEventControl, 1000);
}


