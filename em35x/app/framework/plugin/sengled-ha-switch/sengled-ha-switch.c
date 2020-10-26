// *******************************************************************
// * Sengled-ha-switch.c
// *
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *******************************************************************

#include "app/framework/include/af.h"

void sengledHaCliSwitchOn(void)
{
    int16u cliValue = (int16u)emberUnsignedCommandArgument(0);

    emberAfAppPrintln("Switch value: %2x", cliValue);
}

