
#ifndef _Z02_GATEWAY_APP_
#define _Z02_GATEWAY_APP_

/******************************************************************************
                    Includes section
******************************************************************************/
//#include "hal/hal.h"
#include "hal/micro/cortexm3/compiler/iar.h"
#include "stack/include/ember-types.h"


#define COLOR_FULL_PERCENTAGE 100


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
  DELETE_NETWORK,
  CHANGE_CHANNEL,
  SET_PERMIT_JOINING_TIME,
  NEW_DEVICE_JOIN,
  RESET_TO_FACTORY,
  REMOVE_DEVICE,
  BINDING,
  CONFIG_REPORT,
  
  NETWORK_OPERATION_END,

  INFORMATION_OPERATION_START = 0x0200,

  GET_COORDINATOR_ATTRIBUTE = INFORMATION_OPERATION_START,
  GET_DEVICE_ATTRIBUTE,

  INFORMATION_OPERATION_END,

  CONTROL_OPERATION_START = 0x0300,

  ON_OFF = CONTROL_OPERATION_START,
  LEVEL,
  COLOR_TEMPRATURE,
  IDENTIFY,

  CONTROL_OPERATION_END,

  REPORTING_START = 0x0400,

  REPORTING = REPORTING_START,
  
  REPORTING_END,
  
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


