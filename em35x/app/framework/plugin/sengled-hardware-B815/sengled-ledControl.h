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
#define COLOR_YELLOWEST     370  // 1000,000 / 370 = 2700K
#define COLOR_MIDDLE        286
#define COLOR_WHITEST       250   // 1000,000 /250 = 4000K

#define TICS_PER_PERIOD           6000  // to avoid tracklight hardware issue, it can't be 99.5%

#define DIMMING_CTRL_PIN    PIN_PA6                                   
#define COLOR_TEMP_CTRL_PIN PIN_PB7

#define PWM_ALLOWED 1
#define PWM_FORBIDDEN 0

//#define COLOR_VOLATILE 

// E1E-CEA specific information

#define COLOR_STABLE // 3000K
#define LIGHT_35W
#define VOLTAGE_25V
#define CURRENT_1300MA

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


void set_pwm_level(int16u, PinSelectEnum, boolean );
void emberAfPwmSetValuePB7( int16u value );
void emberAfPwmSetValuePA6( int16u value );


void led_control(boolean, int8u, int16u);

void init_led_status(boolean);

int32s GetEfficiency(int8u);

void software_power_up(void);


#endif
