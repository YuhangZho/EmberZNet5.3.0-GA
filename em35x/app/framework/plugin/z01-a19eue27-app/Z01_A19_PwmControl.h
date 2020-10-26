
#ifndef _PWM_CONTROL_
#define _PWM_CONTROL_



#define TICS_PER_PERIOD      6000  // based on a 6 MHz clock, the number of counts
                                   // in a 1 kHz signal.

typedef enum _PinSelect
{
  PIN_PA6,
  PIN_PB7,
} PinSelectEnum;


void SetPwmLevel(int16u dutyCycle, PinSelectEnum pin);




#endif

