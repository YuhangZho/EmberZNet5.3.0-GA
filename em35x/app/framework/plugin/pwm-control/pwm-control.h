
#ifndef _PWM_CONTROL_
#define _PWM_CONTROL_


#define TICS_PER_PERIOD      6000  // based on a 6 MHz clock, the number of counts
                              // in a 1 kHz signal.
#define PWM_LEVEL_INTERCEPT  1200
#define PWM_LEVEL_AMOUNT     (TICS_PER_PERIOD-PWM_LEVEL_INTERCEPT)

#define PWM_CTL_INTERCEPT    1980
#define PWM_CTL_MAX          4200
#define PWM_CTL_AMOUNT       (PWM_CTL_MAX-PWM_CTL_INTERCEPT)


typedef enum _PinSelect
{
  PIN_PA6,
  PIN_PB7,
} PinSelectEnum;

int16u ConvertLevelToPWM(int8u level, int8u ratio);
int16u ConvertColorTempToPWM(int16u ctLevel, int8u ratio);
void SetPwmLevel(int16u dutyCycle, PinSelectEnum pin);




#endif

