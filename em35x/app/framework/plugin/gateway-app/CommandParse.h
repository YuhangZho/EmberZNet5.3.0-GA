/**************************************************************************//**
  \file CommandParse.h

  \brief
    Serial interface console interface.

  \author
    Atmel Corporation: http://www.atmel.com \n
    Support email: avr@atmel.com

  Copyright (c) 2008-2012, Atmel Corporation. All rights reserved.
  Licensed under Atmel's Limited License Agreement (BitCloudTM).

  \internal
    History:
    22.05.12 N. Fomin - Created.
******************************************************************************/
#ifndef _COMMAND_PARSE_H_
#define _COMMAND_PARSE_H_

/******************************************************************************
                    Includes section
******************************************************************************/
//#include "hal/hal.h"
#include "hal/micro/cortexm3/compiler/iar.h"
#include "stack/include/ember-types.h"

#ifdef SENGLED_UART_APP


/******************************************************************************
                    enums section
******************************************************************************/
enum
{
  SEAT_COMMAND,
  SEAT_DAT,  
};
typedef enum
{
  NULL_COMMAND = 1,
  LENTH_NOT_CORRECT,
  DEVICE_TYPE_NOT_MATCH,
  
} InvalidCommandEnum;

typedef enum
{
  //******************************************************
  //Basic operation
  //******************************************************
  PROTOCOL_OPERATION_START = 0x0000,

  PROTOCOL_VERSION = PROTOCOL_OPERATION_START,

  PROTOCOL_OPERATION_END,
  
  NETWORK_OPERATION_START = 0x0100,
  
  CREATE_NETWORK = NETWORK_OPERATION_START, 
  ADD_DEVICE,
  REMOVE_DEVICE,  
  
  NETWORK_OPERATION_END,

  CONTROL_OPERATION_START = 0x0200,

  IDENTIFY = CONTROL_OPERATION_START,
  LEVEL,
  COLOR_TEMPRATURE,

  CONTROL_OPERATION_END,

  INFORMATION_OPERATION_START = 0x0300,

  GET_DEVICE_ATTRIBUTE = INFORMATION_OPERATION_START,
  GET_COORDINATOR_ATTRIBUTE,

  INFORMATION_OPERATION_END,

  OTA_OPERATION_START = 0x7000,
  
  OTA_UPDATA = OTA_OPERATION_START,
  OTA_UPDATA_FINISH,
  OTA_DELETE,
  
  OTA_OPERATION_END,

  RESPONSE       = 0x8000,
  INCALID_COMMAND = 0xffff,
  
} CommCommand_t;

/**************************************************************************//**
                    Types section
******************************************************************************/
typedef void (*CommandFunc)(int8u *, int8u);

//*********************************************************
void DataParse(int8u *rBuf, int8u len);
int8u* GetSendBuffer(void);
void SerialPortDataSending(int8u len);
void SerialPortInit(void);

#endif

#endif // _UART_CONSOLE_H


