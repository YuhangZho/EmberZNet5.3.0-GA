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

//*******************************************************************************
// Main
//*******************************************************************************
void emberAfMainInitCallback(void)
{
  halGpioConfig(PORTC_PIN(1), GPIOCFG_IN_PUD);
  
  halGpioConfig(PORTC_PIN(5), GPIOCFG_OUT_ALT);
  if (GPIO_PCIN & PC1_MASK) // Z02 ?
  { halGpioConfig(PORTC_PIN(6), GPIOCFG_OUT_ALT);}
  else
  { halGpioConfig(PORTC_PIN(6), GPIOCFG_IN);}

  halGpioConfig(PORTB_PIN(1), GPIOCFG_OUT_ALT);
  halGpioConfig(PORTB_PIN(2), GPIOCFG_IN);
}

/** @brief Main Tick
 *
 * Whenever main application tick is called, this callback will be called at
 * the end of the main tick execution.
 *
 */
void emberAfMainTickCallback(void)
{
  ;
}




