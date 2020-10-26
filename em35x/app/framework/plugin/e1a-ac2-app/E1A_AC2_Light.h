
#ifndef _E1A_AC2_LIGHT_DEF_
#define _E1A_AC2_LIGHT_DEF_



// on off control
enum{
  SENGLED_OFF,
  SENGLED_ON,  
};

#define LAMP_ON_OFF_PIN   PORTB_PIN(3)
#define SET_LAMP_OFF()    do { GPIO_PBSET = BIT(3); powerCount = POWER_COUNT_MAX;} while (0)
#define SET_LAMP_ON()     do { GPIO_PBCLR = BIT(3); powerCount = POWER_COUNT_MAX;} while (0)

// level control
#define DIMMING_MAX_LEVEL  255
#define DIMMING_MIN_LEVEL  0
#define DIMMING_CTRL_PIN   PIN_PA6

// color control
#define COLOR_TEMP_CTRL_PIN PIN_PB7

void ColorTemperatureCompensation(int8u level);



#endif


