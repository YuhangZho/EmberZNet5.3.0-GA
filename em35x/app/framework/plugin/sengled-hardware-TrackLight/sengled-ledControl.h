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

#define TICS_PER_PERIOD_BASIC     300  // based on a 6 MHz clock, the number of counts  in a 20 kHz signal.
#define TICS_PER_PERIOD           300  // to avoid tracklight hardware issue, it can't be 99.5%

#define DIMMING_CTRL_PIN    PIN_PA6                                   
#define COLOR_TEMP_CTRL_PIN PIN_PB7

#define PWM_ALLOWED 1
#define PWM_FORBIDDEN 0

/*
enum{
	E1K_CBA,
	E1K_CCA,
	E1K_CEA,
	E1L_CBA,
	E1L_CCA,
	E1L_CEA
};
*/

#define COLOR_STABLE 
#define LIGHT_20W

//#define MODEL_ID E1K_CCA
/*
#if (MODEL_ID == E1K_CBA)
// color volatile
	#define COLOR_VOLATILE
	#define LIGHT_20W
	#define FM_VERSION "E1K-CBA-V006"
#elif (MODEL_ID == E1K_CCA)
	#define COLOR_VOLATILE
	#define LIGHT_35W
	#define FM_VERSION "E1K-CCA-V006"
#elif (MODEL_ID == E1K_CEA)
	#define COLOR_VOLATILE
	#define LIGHT_50W
	#define FM_VERSION "E1K-CEA-V006"
#elif (MODEL_ID == E1L_CBA)
	#define COLOR_VOLATILE
	#define LIGHT_20W
	#define FM_VERSION "E1L-CBA-V006"
#elif (MODEL_ID == E1L_CCA)
	#define COLOR_VOLATILE
	#define LIGHT_35W
	#define FM_VERSION "E1L-CCA-V006"
#elif (MODEL_ID == E1L_CEA)
	#define COLOR_VOLATILE
	#define LIGHT_50W
	#define FM_VERSION "E1L-CEA-V006"
// color stable
#elif (MODEL_ID == E1E_CBA)
	#define COLOR_STABLE 
	#define LIGHT_20W
	#define FM_VERSION "E1E-CBA-V006"
#elif (MODEL_ID == E1E_CCA)
	#define COLOR_STABLE 
	#define LIGHT_35W
	#define FM_VERSION "E1E-CCA-V006"
#elif (MODEL_ID == E1E_CEA)
	#define COLOR_STABLE 
	#define LIGHT_50W
	#define FM_VERSION "E1E-CEA-V006"
#elif (MODEL_ID == E1A_CBA)
	#define COLOR_STABLE 
	#define LIGHT_20W
	#define FM_VERSION "E1A-CBA-V006"
#elif (MODEL_ID == E1A_CCA)
	#define COLOR_STABLE 
	#define LIGHT_35W
	#define FM_VERSION "E1A-CEA-V006"
//#elif (MODEL_ID == E1E_CBA)
//	#define COLOR_STABLE 
//	#define LIGHT_50W
//	#define FM_VERSION "E1E-CBA-V003"
#endif
*/

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

void set_bulb_on(void);

void set_bulb_off(void);  

void init_led_status(boolean);

int32s GetEfficiency(int8u);

void software_power_up(void);


#endif
