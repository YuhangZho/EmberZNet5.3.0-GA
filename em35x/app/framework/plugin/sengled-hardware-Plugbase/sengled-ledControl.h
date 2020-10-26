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

#define TICS_PER_PERIOD           6000  // to avoid tracklight hardware issue, it can't be 99.5%

#define DIMMING_CTRL_PIN    PIN_PA6                                   
#define COLOR_TEMP_CTRL_PIN PIN_PB7

#define UNIT_TIME    512   // 1 means 512ms

#define BLINK_FOREVER 0xff  

#define M0 0
#define M1 1
#define M2 2
#define M3 3
#define M4 4
#define M5 5


enum{
  LED_OFF = 0,
  LED_ON  = 1,
};

typedef enum _PinSelect
{
  PIN_PA6,
  PIN_PB7,
} PinSelectEnum;


void set_pwm_level(int16u, PinSelectEnum, boolean );
void emberAfPwmSetValuePB7( int16u value );
void emberAfPwmSetValuePA6( int16u value );

void set_bulb_on(void);

void set_bulb_off(void);  

int32s GetEfficiency(int8u);

void setLedPulseStatus(int16u pwm, int8u count, int8u time);

#endif
