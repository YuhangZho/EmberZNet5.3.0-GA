// *******************************************************************
// * sengled-ha-common.h
// *
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *******************************************************************
#include "app/framework/plugin/sengled-ha-network-TrackLight/sengled-network.h"
#include "app/framework/plugin/sengled-hardware-TrackLight/sengled-ledControl.h"

#define SENGLED_Z01_A19_UART

#define COLOR_3000K 0x01   // warm color means output a low voltage  3000K == 0x014D
#define COLOR_4000K 0x02   // cold color means output a high voltage  4000K == 0x00FA

#define OTA_REBOOT_INFO 0x0201
void color_mode_selection(int8u mode);

void sengledAfMainInit(void);

void adc_control(void);
void adc_init(void);

