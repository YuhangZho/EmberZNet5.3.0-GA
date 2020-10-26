// *******************************************************************
// * sengled-ha-common.h
// *
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *******************************************************************
#include "app/framework/plugin/sengled-ha-network-Plugbase/sengled-network.h"
#include "app/framework/plugin/sengled-hardware-Plugbase/sengled-ledControl.h"

void sengledAfMainInit(void);

void adc_control(void);
void adc_init(void);

#define SUPPORT_ADC

#define GET_FIRMWARE_VER  0x63

#define MFG_ENABLE        0x80 
#define MFG_START         0x81
#define MFG_END           0x82
#define MFG_TONE_START    0x83
#define MFG_TONE_STOP     0x84
#define MFG_STREAM_START  0x85
#define MFG_STREAM_STOP   0x86
#define MFG_SEND          0x87
#define MFG_SET_POWER     0x88
#define MFG_GET_POWER     0x89
#define MFG_SET_CHANNEL   0x8a
#define MFG_GET_STATUS    0x8b


#define DEBUG_COMMON

#ifdef DEBUG_COMMON
#define sengledGuaranteedPrint(...)   emberAfPrint(0xFFFF, __VA_ARGS__)

/**
 * @brief Println that can't be turned off.
 */
#define sengledGuaranteedPrintln(...) emberAfPrintln(0xFFFF, __VA_ARGS__)

/**
 * @brief Buffer print that can't be turned off.
 */
#define emberAfGuaranteedPrintBuffer(buffer, len, withSpace) emberAfPrintBuffer(0xFFFF, (buffer), (len), (withSpace))

/**
 * @brief String print that can't be turned off.
 */
#define emberAfGuaranteedPrintString(buffer) emberAfPrintString(0xFFFF, (buffer))

/**
 * @brief Long string print that can't be turned off.
 */
#define emberAfGuaranteedPrintLongString(buffer) emberAfPrintLongString(0xFFFF, (buffer))

/**
 * @brief Buffer flush for emberAfGuaranteedPrint(), emberAfGuaranteedPrintln(),
 * emberAfGuaranteedPrintBuffer(), and emberAfGuaranteedPrintString().
 */
#define emberAfGuaranteedFlush()      emberAfFlush(0xFFFF)
#else

#define sengledGuaranteedPrint(...)  

#define sengledGuaranteedPrintln(...) 

#define emberAfGuaranteedPrintBuffer(buffer, len, withSpace) 

#define emberAfGuaranteedPrintString(buffer) 

#define emberAfGuaranteedPrintLongString(buffer) 

#define emberAfGuaranteedFlush()      

#endif
