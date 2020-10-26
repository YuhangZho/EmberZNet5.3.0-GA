#ifndef _E1A_AC2_PUBLIC_HEAD_DEF_
#define _E1A_AC2_PUBLIC_HEAD_DEF_

#include "app/framework/include/af.h"
#include "app/framework/util/attribute-storage.h"

#ifdef EMBER_AF_PLUGIN_SCENES
  #include "app/framework/plugin/scenes/scenes.h"
#endif //EMBER_AF_PLUGIN_SCENES

#ifdef EMBER_AF_PLUGIN_ON_OFF
  #include "app/framework/plugin/on-off/on-off.h"
#endif //EMBER_AF_PLUGIN_ON_OFF

#ifdef EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER
  #include "app/framework/plugin/zll-level-control-server/zll-level-control-
server.h"
#endif //EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER

#include <math.h>

#include "app/framework/plugin/ezmode-commissioning/ez-mode.h"
#include "app/framework/plugin/reporting/reporting.h"
#include "app/framework/plugin/counters/counters.h"

#include "E1A_AC2_PwmControl.h"
#include "E1A_AC2_Network.h"
#include "E1A_AC2_Button.h"
#include "E1A_AC2_Light.h"
#include "E1A_AC2_Power.h"


#define POWER_COUNT_MAX 2

extern int8u powerCount;
extern int16u saveCurrent;

#endif

