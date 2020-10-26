#include PLATFORM_HEADER

#ifdef EZSP_HOST
  // Includes needed for ember related functions for the EZSP host
  #include "stack/include/error.h"
  #include "stack/include/ember-types.h"
  #include "app/util/ezsp/ezsp-protocol.h"
  #include "app/util/ezsp/ezsp.h"
  #include "app/util/ezsp/serial-interface.h"
  extern int8u emberEndpointCount;
#else
  #include "stack/include/ember.h"
#endif

#include "hal/hal.h"
#include "app/util/serial/serial.h"
#include "app/util/serial/command-interpreter2.h"

#if defined(EMBER_REQUIRE_FULL_COMMAND_NAME) \
  || defined(EMBER_REQUIRE_EXACT_COMMAND_NAME)
  #undef EMBER_REQUIRE_EXACT_COMMAND_NAME
  #define EMBER_REQUIRE_EXACT_COMMAND_NAME TRUE
#else
  #define EMBER_REQUIRE_EXACT_COMMAND_NAME FALSE
#endif

#if !defined APP_SERIAL
  extern int8u serialPort;
  #define APP_SERIAL serialPort
#endif

#if defined EMBER_COMMAND_INTEPRETER_HAS_DESCRIPTION_FIELD
  #define printIfEntryHasDescription(entry, ...) \
  if ((entry)->description != NULL) {            \
    emberSerialPrintf(APP_SERIAL,                \
                      __VA_ARGS__);              \
    }
  #define printIfEntryHasArgumentDescriptions(entry, ...) \
  if ((entry)->argumentDescriptions != NULL) {            \
    emberSerialPrintf(APP_SERIAL,                         \
                      __VA_ARGS__);                       \
  }
#else
  #define printIfEntryHasDescription(entry, ...) 
  #define printIfEntryHasArgumentDescriptions(entry, ...)
#endif



/******************************************************************************
                    Defines section
******************************************************************************/
#ifdef SENGLED_UART_APP


#define COMM_HEAD1   0xa5
#define COMM_HEAD2   0x5a
#define COMM_TAILED  0x0d

#define SERIAL_PORT_RECEIVE_TIMER_INTERVAL   500  // ms
#define SERIAL_PORT_TRANSFER_TIMER_INTERVAL  500  // ms

#define isAck(l)  (1 == l)

/******************************************************************************
                    enums section
******************************************************************************/
enum
{
  SEAT_HEAD1,
  SEAT_HEAD2,
  SEAT_LEN,
  SEAT_DATA,  
};

typedef enum
{
  SUCCESS,
  RCV_OVERTIME,
  CHECK_SUM_ERROR,
  TOO_LONG
} AckState_t;

/******************************************************************************
                    Types section
******************************************************************************/
#if (APP_SERIAL==1)
#define SERIAL_RX_SIZE EMBER_SERIAL1_RX_QUEUE_SIZE
#define SERIAL_TX_SIZE EMBER_SERIAL1_TX_QUEUE_SIZE
#else
#define SERIAL_RX_SIZE EMBER_SERIAL0_RX_QUEUE_SIZE
#define SERIAL_TX_SIZE EMBER_SERIAL0_TX_QUEUE_SIZE
#endif


typedef struct _SerialPortStr
{
  struct
  {
    int8u buffer[SERIAL_RX_SIZE];
    int8u ptr;
    int8u len;
    int8u checkSum;
    int32u timer;
  } rcv;

  struct
  {
    int8u buffer[SERIAL_RX_SIZE];
    int8u retryCount;
    int8u checkSum;
  } send;
} SerialPortStr;



//#define SENGLED_UART_TEST


#include "app/framework/plugin/z01-a19-gateway-app/CommandParse.h"


/******************************************************************************
                    Local variables section
******************************************************************************/
static SerialPortStr serialPortData;

/******************************************************************************
                    Implementation section
******************************************************************************/
//***************************************************************************
int8u* GetSendBuffer(void)
{
  return &serialPortData.send.buffer[SEAT_DATA];
}
//***************************************************************************
// len = 0x00  is ACK
//***************************************************************************
void SerialPortDataSending(int8u len)
{
  int8u i;
  
  serialPortData.send.buffer[SEAT_HEAD1] = COMM_HEAD1;
  serialPortData.send.buffer[SEAT_HEAD2] = COMM_HEAD2;
  serialPortData.send.buffer[SEAT_LEN]   = len;
  serialPortData.send.checkSum = 0;
  for (i=0; i<len; ++i)
  { serialPortData.send.checkSum += serialPortData.send.buffer[SEAT_DATA+i];}
  serialPortData.send.buffer[SEAT_DATA+len] = serialPortData.send.checkSum;
  
  emberSerialWriteData(APP_SERIAL, serialPortData.send.buffer, len+4);
}
//***************************************************************************
void SendAck(AckState_t state)
{
  int8u *buf;
  
  buf = GetSendBuffer();
  buf[0] = state;  
  SerialPortDataSending(1);
}
//***************************************************************************
void SerialPortInit(void)
{
  serialPortData.rcv.ptr = 0;
}
//***************************************************************************
void UartReciveTimeOut(int32u num)
{
  if (serialPortData.rcv.timer)
  {
    if (num >= serialPortData.rcv.timer)
    { serialPortData.rcv.ptr = 0;
      serialPortData.rcv.timer = 0;
      SerialPortDataSending(1);
    }
    else
    { serialPortData.rcv.timer -= num;}
      
  }
}
//***************************************************************************
boolean emberProcessCommand(int8u *input, int8u sizeOrPort)
{
  int8u dat;  
  
  while (EMBER_SUCCESS == emberSerialReadByte(sizeOrPort, &dat))
  {
    if (SEAT_HEAD1 == serialPortData.rcv.ptr)
    {
      if (COMM_HEAD1 == dat)
      { serialPortData.rcv.buffer[serialPortData.rcv.ptr++] = dat;}
    }
    else if (SEAT_HEAD2 == serialPortData.rcv.ptr)
    {
      if (COMM_HEAD2 == dat)
      { serialPortData.rcv.buffer[serialPortData.rcv.ptr++] = dat;}
      else if (COMM_HEAD1 == dat)
      {  serialPortData.rcv.ptr = SEAT_HEAD2;}
    }
    else if (SEAT_LEN == serialPortData.rcv.ptr)
    {
      serialPortData.rcv.len = dat;      
      serialPortData.rcv.checkSum = 0;
      serialPortData.rcv.buffer[serialPortData.rcv.ptr++] = dat;
    }
    else if ((SEAT_DATA+serialPortData.rcv.len) == serialPortData.rcv.ptr)
    {
      #ifdef SENGLED_UART_TEST
      if (COMM_TAILED == dat)  // Comparing the checksum
      #else
      if (serialPortData.rcv.checkSum == dat)  // Comparing the checksum
      #endif
      {
        if (!isAck(serialPortData.rcv.len))
        { SendAck(SUCCESS);  //ACK
          DataParse(&serialPortData.rcv.buffer[SEAT_DATA], serialPortData.rcv.len);
        }
      }
      else
      { SendAck(CHECK_SUM_ERROR);
        //callCommandAction(); 
      }
      serialPortData.rcv.ptr = 0;
      serialPortData.rcv.timer = 0;
    }
    else
    {
      if (serialPortData.rcv.ptr < SERIAL_RX_SIZE)  // Put to buffer
      {
        serialPortData.rcv.checkSum += dat;
        serialPortData.rcv.buffer[serialPortData.rcv.ptr++] = dat;
      }
      else
      { SendAck(TOO_LONG);}
    }
    
    serialPortData.rcv.timer = SERIAL_PORT_RECEIVE_TIMER_INTERVAL;
  }

  return FALSE;
}

#endif

