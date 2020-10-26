//#include "app/framework/include/af.h"
//#include "app/framework/util/attribute-storage.h"
#include PLATFORM_HEADER //compiler/micro specifics, types
#include "stack/include/ember.h"
#include "stack/include/mfglib.h"
#include "hal/hal.h"
#include "app/util/serial/serial.h"
#include "app/util/serial/cli.h"


#include "mfg-PwmControl.h"


void emberAfPwmSetValuePB7( int16u value );
void emberAfPwmSetValuePA6( int16u value );


//void emberAFPwmInit( void )
void emberAfPluginPwmControlInitCallback( void )
{
  halResetWatchdog();

  halGpioConfig(PORTA_PIN(6),GPIOCFG_OUT_ALT);
  halGpioConfig(PORTB_PIN(7),GPIOCFG_OUT_ALT);
  //*********************************************************
  // TIM1 PWM init
  //*********************************************************
  // put PWM initialization code here.
  
  //According to emulator.h, LEVEL_CONTROL is on pin 15 which is mapped 
  //to channel 2 of TMR1
  TIM1_OR = 0;       //use 12MHz clock
  TIM1_PSC = 1;      //1^2=2 -> 12MHz/2 = 6 MHz = 6000 ticks per 1/1000 of a second
  TIM1_EGR = 1;      //trigger update event to load new prescaler value
  TIM1_CCMR1  = 0;   //start from a zeroed configuration
  TIM1_ARR = TICS_PER_PERIOD;  // set the period
  TIM1_CNT = 0; //force the counter back to zero to prevent missing LEVEL_CONTROL_TOP

  //*** set up PB6, which is channel 1 to be a PWM output
  //*** set up PB7, which is channel 2 to be a PWM output
  //*** set up PA6, which is channel 3 to be a PWM output
  //*** set up PA7, which is channel 4 to be a PWM output

  //Output waveform: toggle on CNT reaching TOP
  TIM1_CCMR2 |= (0x6 << TIM_OC3M_BIT); 
  TIM1_CCMR1 |= (0x6 << TIM_OC2M_BIT);   //Added by Shaoxian on 20140408  
  
  ATOMIC(
  TIM1_CCER |= TIM_CC2E;    //enable output on channel 2
  TIM1_CCER |= TIM_CC3E;    //enable output on channel 3
  TIM1_CR1 |= TIM_CEN;      //enable counting
  )

  emberAfPwmSetValuePB7(0);
  emberAfPwmSetValuePA6(0);

  int8u currentLevel = 255;  

  {
    int16u oldTime = halCommonGetInt16uMillisecondTick();  
    int16u newTime = oldTime;
    
    for (int16u i=0; i<=(currentLevel); i++)
    {
      SetPwmLevel(ConvertLevelToPWM(i, 255), PIN_PB7);
      SetPwmLevel(ConvertColorTempToPWM(i, 255), PIN_PA6);
      while(elapsedTimeInt16u(oldTime, newTime) < 1)
      {
        halResetWatchdog();   // Periodically reset the watchdog.
        newTime = halCommonGetInt16uMillisecondTick();
      }

      oldTime = newTime; 
    }
  }
}

void emberAfPwmSetValuePB7( int16u value )
{
  assert(value <= TICS_PER_PERIOD);
  
  TIM1_CCR2 = value;
} 
void emberAfPwmSetValuePA6( int16u value )
{
  assert(value <= TICS_PER_PERIOD);

  TIM1_CCR3 = value;
}

// update drive level based on linear power delivered to the light
//int16u LevelToPwm(int8u level, int8u ratio)
int16u ConvertLevelToPWM(int8u level, int8u ratio)
{
  int32u midleLevel;
    
  if (level == ratio)
  { return TICS_PER_PERIOD;}
  else if (level == 0)
  { return 0;}
  midleLevel = (int32u)level;
  midleLevel = midleLevel*PWM_LEVEL_AMOUNT*100;
  midleLevel = midleLevel/(ratio*100)+PWM_LEVEL_INTERCEPT;
  
  return (int16u)midleLevel;
}  
int16u ConvertColorTempToPWM(int16u ctLevel, int8u ratio)
{
  int32u midleLevel;
    
  if (ctLevel == ratio)
  { return TICS_PER_PERIOD;}
  else if (ctLevel == 0)
  { return 0;}
  midleLevel = (int32u)ctLevel;
  midleLevel = midleLevel*PWM_CTL_AMOUNT*100;
  midleLevel = midleLevel/(ratio*100)+PWM_CTL_INTERCEPT;
  
  return (int16u)midleLevel;
} 
void SetPwmLevel(int16u dutyCycle, PinSelectEnum pin)
{
  switch (pin)
  {
    case PIN_PA6:
      emberAfPwmSetValuePA6(dutyCycle);
      break;
    case PIN_PB7:
      emberAfPwmSetValuePB7(dutyCycle);
  }
}
