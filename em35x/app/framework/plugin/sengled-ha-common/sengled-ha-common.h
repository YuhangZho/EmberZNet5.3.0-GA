// *******************************************************************
// * sengled-ha-common.h
// *
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *******************************************************************
#include "sengled-network.h"
#include "sengled-ledControl.h"
#define EMBER_SOC
//#define EMBER_EZSP

//#define SUPPORT_TRACK_LIGHT  // if we support tracklight not EUE27 project ,open this macro;

// if dimmerWaveFormCounter is larger than 40, it means dimmer is connected when power on;
#define DIMMER_CONNECTED_LINE 40

//#define COLOR_SELF_CHANGE
void sengledAfMainInit(void);
