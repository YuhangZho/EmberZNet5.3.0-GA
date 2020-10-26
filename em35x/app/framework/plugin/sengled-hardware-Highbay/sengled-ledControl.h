// *******************************************************************
// * sengled-ledControl.h
// *
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *******************************************************************
#ifndef _SENGLED_LEDCONTROL_
#define _SENGLED_LEDCONTROL_

#include "app/framework/include/af.h"

#define DIMMING_MAX_LEVEL   255
#define DIMMING_MIN_LEVEL   0
#define COLOR_YELLOWEST     370   // 1000,000 / 370 = 2700K
#define COLOR_WHITEST       153   // 1000,000 /153 = 6500K
#define TICS_PER_PERIOD     6000  // based on a 6 MHz clock, the number of counts  in a 1 kHz signal.
#define DIMMING_CTRL_PIN    PIN_PA6                                   
#define COLOR_TEMP_CTRL_PIN PIN_PB7

enum{
  LED_OFF = 0,
  LED_ON  = 1,
};

enum {
 CCT_TYPE_YELLOW,
 CCT_TYPE_WHITE  
};


typedef enum _PinSelect
{
  PIN_PA6,
  PIN_PB7,
} PinSelectEnum;


void SetPwmLevel(int16u, PinSelectEnum, boolean );
void emberAfPwmSetValuePB7( int16u value );
void emberAfPwmSetValuePA6( int16u value );


void SetLed(boolean, int8u, int16u);

void SetOpenCircuit(void);
void SetCloseCircuit(void);  

void InitLedStatus(boolean);
void RecoverLedRecordedStatus(void);

void ColorTemperatureCompensation(void);
int32s GetEfficiency(int8u);

#endif
