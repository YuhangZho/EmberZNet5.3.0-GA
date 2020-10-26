#include "app/framework/include/af.h"
#include "app/framework/util/attribute-storage.h"

#ifdef EMBER_AF_PLUGIN_SCENES
  #include "app/framework/plugin/scenes/scenes.h"
#endif //EMBER_AF_PLUGIN_SCENES

#ifdef EMBER_AF_PLUGIN_ON_OFF
  #include "app/framework/plugin/on-off/on-off.h"
#endif //EMBER_AF_PLUGIN_ON_OFF

#ifdef EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER
  #include "app/framework/plugin/zll-level-control-server/zll-level-control-server.h"
#endif //EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER

#include "E1A_AC2_Public_Head.h"


void SetLedInitialValue(void);

void emberAfPwmSetValuePB7( int16u value );
void emberAfPwmSetValuePA6( int16u value );


//void emberAFPwmInit( void )
void emberAfPluginPwmControlInitCallback( void )
{
  halResetWatchdog();
  
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
  TIM1_CR1  |= TIM_CEN;      //enable counting
  )

  emberAfPwmSetValuePB7(TICS_PER_PERIOD);
  emberAfPwmSetValuePA6(TICS_PER_PERIOD);

  SetLedInitialValue();
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
