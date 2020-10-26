/**************************************************************************//**
 
******************************************************************************/


/******************************************************************************
                    Includes section
******************************************************************************/
#include "CommandParse.h"
#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "app/util/zigbee-framework/zigbee-device-common.h"


#ifdef SENGLED_UART_APP


/******************************************************************************
                    Defines section
******************************************************************************/
#define COORDINATOR_PROTOCOL_VERSION 0x0001

enum
{
  DC_POWER_ATTRIBUTE_ID,
  CURRENT_LEVEL_ATTRIBUTE_ID,
  CURRENT_COLOR_TEMPRATURE_ATTRIBUTE_ID
};

#define OTA_BUFFER_SIZE 64

#define STATIC_IMAGE_DATA { \
  0x1e, 0xf1, 0xee, 0x0b, 0x00, 0x01, 0x38, 0x00, \
  0x00, 0x00, 0x02, 0x10, 0x70, 0x56, 0x05, 0x00, \
  0x00, 0x00, 0x02, 0x00, 0x54, 0x68, 0x65, 0x20, \
  0x6c, 0x61, 0x74, 0x65, 0x73, 0x74, 0x20, 0x61, \
  0x6e, 0x64, 0x20, 0x67, 0x72, 0x65, 0x61, 0x74, \
  0x65, 0x73, 0x74, 0x20, 0x75, 0x70, 0x67, 0x72, \
  0x61, 0x64, 0x65, 0x2e, 0xb6, 0x00, 0x00, 0x00, \
  0x00, 0xf0, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x01, \
  0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, \
  0x02, 0x00, 0x30, 0x00, 0x00, 0x00, 0x03, 0x07, \
  0x79, 0x29, 0x47, 0xb3, 0x85, 0x0a, 0x95, 0x85, \
  0xbf, 0x8e, 0x25, 0xc1, 0x9d, 0x8e, 0x86, 0x78, \
  0x43, 0x4f, 0x58, 0x36, 0x00, 0x0d, 0x6f, 0x00, \
  0x00, 0x19, 0x8b, 0x36, 0x54, 0x45, 0x53, 0x54, \
  0x53, 0x45, 0x43, 0x41, 0x01, 0x09, 0x00, 0x00, \
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, \
  0x32, 0x00, 0x00, 0x00, 0x36, 0x8b, 0x19, 0x00, \
  0x00, 0x6f, 0x0d, 0x00, 0x01, 0xc2, 0xc3, 0x1c, \
  0xb8, 0xc4, 0x00, 0x64, 0xea, 0xbb, 0x31, 0x89, \
  0xad, 0x89, 0x69, 0xea, 0xc2, 0x58, 0x93, 0x18, \
  0x3a, 0x02, 0x3b, 0xd2, 0x8d, 0x5f, 0xb2, 0x13, \
  0x4d, 0x3e, 0x07, 0xb9, 0x2e, 0x06, 0xa2, 0x58, \
  0xe4, 0x78, 0xd2, 0x0c, 0x7a, 0xcc, \
}

#define STATIC_IMAGE_DATA_SIZE 182L

enum {
  NETWORK_FIND_NONE,
  NETWORK_FIND_FORM,
  NETWORK_FIND_JOIN,
  NETWORK_FIND_WAIT,
};

static int8u state = NETWORK_FIND_NONE;


/*******************************************************************************
                   functions section
*******************************************************************************/
static void ProcessInvalidCommand(int8u state);
static void ProcessGetProtocolVersionReq(int8u *rBuf, int8u len);
static void ProcessCreateNetworkReq(int8u *rBuf, int8u len);
static void processAddDeviceReq(int8u *rBuf, int8u len);
static void processRemoveDeviceReq(int8u *rBuf, int8u len);
static void processGetDeviceAttributeReq(int8u *rBuf, int8u len);
static void processGetCoordinatorAttributeReq(int8u *rBuf, int8u len);
static void processIdentifyReq(int8u *rBuf, int8u len);
static void processLevelReq(int8u *rBuf, int8u len);
static void processColorTempratureReq(int8u *rBuf, int8u len);
static void processOtaUpdataReq(int8u *rBuf, int8u len);
static void processOtaDeleteReq(int8u *rBuf, int8u len);
static void processOtaUpdataFinishReq(int8u *rBuf, int8u len);


void zclBufferAddByte(int8u byte);
void zclBufferAddWord(int16u word);
void zclBufferSetup(int8u frameType, int16u clusterId, int8u commandId);
void emAfApsFrameEndpointSetup(int8u srcEndpoint, int8u dstEndpoint);
EmberStatus emberScanForUnusedPanId(int32u channelMask, int8u duration);

int32u emberAfOtaGetImageSize(void);


/******************************************************************************
                    Local variables section
******************************************************************************/
extern EmberApsFrame globalApsFrame;
extern int8u appZclBuffer[EMBER_AF_MAXIMUM_SEND_PAYLOAD_LENGTH];
extern int16u appZclBufferLen;
extern int16u mfgSpecificId;

static int8u sLen;
static int8u *sBuf;
//***************************************************************************
const CommandFunc protoclOperation[] =
{
  ProcessGetProtocolVersionReq
};

const CommandFunc networkOperation[] =
{
  ProcessCreateNetworkReq, processAddDeviceReq, processRemoveDeviceReq   
};
const CommandFunc controlOperation[] =
{
  processIdentifyReq, processLevelReq, processColorTempratureReq
};
const CommandFunc informationOperation[] =
{
  processGetDeviceAttributeReq, processGetCoordinatorAttributeReq
};

const CommandFunc otaOperation[] =
{
  processOtaUpdataReq, processOtaUpdataFinishReq, processOtaDeleteReq
};

/******************************************************************************
                    Implementation section
******************************************************************************/
void InitNodeInfo(void)
{
  ;
}
//***************************************************************************
static void ProcessInvalidCommand(int8u state)
{
  sBuf = GetSendBuffer();

  *((int16u*)sBuf) = INCALID_COMMAND;  
  sBuf[2] = state;  
  
  SerialPortDataSending(SEAT_COMMAND+3);
}

//***************************************************************************
void DataParse(int8u *rBuf, int8u len)
{
  int16u cmd;    

  cmd = *((int16u*)rBuf);
  rBuf += 2;

  if ((cmd&0xff00) == PROTOCOL_OPERATION_START)  
  {
    if (cmd < PROTOCOL_OPERATION_END)
    { 
      protoclOperation[cmd-PROTOCOL_OPERATION_START](rBuf, len);
      return;
    }
  }
  else if ((cmd&0xff00) == NETWORK_OPERATION_START)
  {
    if (cmd < NETWORK_OPERATION_END)
    { 
      networkOperation[cmd-NETWORK_OPERATION_START](rBuf, len);
      return;
    }
  }
  else if ((cmd&0xff00) == CONTROL_OPERATION_START)
  {
    if (cmd < CONTROL_OPERATION_END)
    { 
      controlOperation[cmd-CONTROL_OPERATION_START](rBuf, len);
      return;
    }
  }
  else if ((cmd&0xff00) == INFORMATION_OPERATION_START)
  {
    if (cmd < INFORMATION_OPERATION_END)
    { 
      informationOperation[cmd-INFORMATION_OPERATION_START](rBuf, len);
      return;
    }
  }
  else if ((cmd&0xff00) == OTA_OPERATION_START)
  {
    if (cmd < OTA_OPERATION_END)
    { 
      otaOperation[cmd-OTA_OPERATION_START](rBuf, len);
      return;
    }
  }
  
  ProcessInvalidCommand(NULL_COMMAND);
}  

//***************************************************************************
static void ProcessGetProtocolVersionReq(int8u *rBuf, int8u len)
{
  if (len != 2)
  {
    ProcessInvalidCommand(LENTH_NOT_CORRECT);
    return;
  }
  
  sBuf = GetSendBuffer();

  *((int16u*)sBuf) = PROTOCOL_VERSION|RESPONSE;  
  *((int16u*)&sBuf[2]) = COORDINATOR_PROTOCOL_VERSION;
  
  SerialPortDataSending(SEAT_COMMAND+4);
}
//***************************************************************************
static void ProcessCreateNetworkReq(int8u *rBuf, int8u len)
{
  if (len != 6)
  {
    ProcessInvalidCommand(LENTH_NOT_CORRECT);
    return;
  }

  sLen = SEAT_COMMAND+2;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = CREATE_NETWORK|RESPONSE;

  if (state != NETWORK_FIND_NONE) 
  {
    sBuf[sLen++] = 0x02;
  }
  else 
  {
    EmberStatus status = EMBER_INVALID_CALL;
    int32u channelMask = *((int32u*)rBuf);
    
    status = emberScanForUnusedPanId(channelMask,
                                   EMBER_AF_PLUGIN_NETWORK_FIND_DURATION);
    if (status == EMBER_SUCCESS) 
    {
      state = NETWORK_FIND_FORM;
    }
    
    sBuf[sLen++] = status;
  }  
  
  SerialPortDataSending(sLen);
}
static void processAddDeviceReq(int8u *rBuf, int8u len)
{
  if (len != 2)
  {
    ProcessInvalidCommand(LENTH_NOT_CORRECT);
    return;
  }
  
  emAfPermitJoin(120, TRUE); // broadcast
}
boolean emberAfPluginTrustCenterJoinDecisionMakeJoinDecisionCallback(EmberNodeId nodeId,
                                                                     EmberEUI64 nodeEUI64)
{ 
  sLen = SEAT_COMMAND+2;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = ADD_DEVICE|RESPONSE;    
  sBuf[sLen++] = 0x00;
  *((int16u*)&sBuf[sLen]) = nodeId;
  sLen += 2;
  MEMCOPY(&sBuf[sLen], nodeEUI64, EUI64_SIZE);
  sLen += EUI64_SIZE;
  
  SerialPortDataSending(sLen);
  return TRUE;
}
static void processRemoveDeviceReq(int8u *rBuf, int8u len)
{
  EmberNodeId nodeId;
  EmberEUI64  nodeEUI64;
                                                                     
  if (len != 12)
  {
    ProcessInvalidCommand(LENTH_NOT_CORRECT);
    return;
  }

  nodeId = *((int16u*)rBuf);
  MEMCOPY(nodeEUI64, &rBuf[2], EUI64_SIZE);
  EmberStatus status = emberLeaveRequest(nodeId,
                                         nodeEUI64,
                                         EMBER_ZIGBEE_LEAVE_AND_REMOVE_CHILDREN,
                                         EMBER_APS_OPTION_NONE);

  sLen = SEAT_COMMAND+2;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = REMOVE_DEVICE|RESPONSE;    
  sBuf[sLen++] = status;
  *((int16u*)&sBuf[sLen]) = nodeId;
  sLen += 2;
  MEMCOPY(&sBuf[sLen], nodeEUI64, EUI64_SIZE);
  sLen += EUI64_SIZE;
  
  SerialPortDataSending(sLen);
}

static void processIdentifyReq(int8u *rBuf, int8u len)
{
  EmberStatus status;

  if (len != 4)
  {
    ProcessInvalidCommand(LENTH_NOT_CORRECT);
    return;
  }

  zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER, 
               ZCL_IDENTIFY_CLUSTER_ID, 
               ZCL_IDENTIFY_COMMAND_ID);
  zclBufferAddWord(180);
  emAfApsFrameEndpointSetup(1, 1);
  status = emberAfSendUnicast(EMBER_OUTGOING_DIRECT,
                            *((int16u*)rBuf),
                            &globalApsFrame,
                            appZclBufferLen,
                            appZclBuffer);

  sLen = SEAT_COMMAND+2;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = IDENTIFY|RESPONSE;
  sBuf[sLen++] = status;
  *((int16u*)&sBuf[sLen]) = *((int16u*)rBuf);
  sLen += 2;
  
  SerialPortDataSending(sLen);
}

static void processLevelReq(int8u *rBuf, int8u len)
{
  EmberStatus status;

  if (len != 7)
  {
    ProcessInvalidCommand(LENTH_NOT_CORRECT);
    return;
  }

  zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER, 
               ZCL_LEVEL_CONTROL_CLUSTER_ID, 
               ZCL_MOVE_TO_LEVEL_COMMAND_ID);
  zclBufferAddByte(rBuf[2]);  
  zclBufferAddWord(*(int16u *)&rBuf[3]);
  emAfApsFrameEndpointSetup(1, 1);
  status = emberAfSendUnicast(EMBER_OUTGOING_DIRECT,
                            *((int16u*)rBuf),
                            &globalApsFrame,
                            appZclBufferLen,
                            appZclBuffer);

  sLen = SEAT_COMMAND+2;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = LEVEL|RESPONSE;
  sBuf[sLen++] = status;
  *((int16u*)&sBuf[sLen]) = *((int16u*)rBuf);
  sLen += 2;
  sBuf[sLen++] = rBuf[2];
  
  SerialPortDataSending(sLen);
}
static void processColorTempratureReq(int8u *rBuf, int8u len)
{
  EmberStatus status;

  if (len != 7)
  {
    ProcessInvalidCommand(LENTH_NOT_CORRECT);
    return;
  }
  
  emberAfFillCommandSengledClusterMoveTempToLevel(rBuf[2], *(int16u *)&rBuf[3]);
  emberAfSetCommandEndpoints(1, 1);
  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, *((int16u*)rBuf));

  sLen = SEAT_COMMAND+2;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = COLOR_TEMPRATURE|RESPONSE;
  sBuf[sLen++] = status;
  *((int16u*)&sBuf[sLen]) = *((int16u*)rBuf);
  sLen += 2;
  sBuf[sLen++] = rBuf[2];
  
  SerialPortDataSending(sLen);
}
static void processGetDeviceAttributeReq(int8u *rBuf, int8u len)
{
  if (len != 6)
  {
    ProcessInvalidCommand(LENTH_NOT_CORRECT);
    return;
  }
  
  if (*((int16u*)&rBuf[2]) != 0x0000)
  {
    ProcessInvalidCommand(DEVICE_TYPE_NOT_MATCH);
    return;
  }

  mfgSpecificId = 0x1901;
  zclBufferSetup(ZCL_PROFILE_WIDE_COMMAND,
               ZCL_SENGLED_APP_CLUSTER_ID,
               ZCL_READ_ATTRIBUTES_COMMAND_ID);
  zclBufferAddWord(ZCL_SL_DC_POWER_ATTRIBUTE_ID);
  zclBufferAddWord(ZCL_SL_ON_OFF_ATTRIBUTE_ID);    
  zclBufferAddWord(ZCL_SL_CURRENT_LEVEL_ATTRIBUTE_ID);
  zclBufferAddWord(ZCL_SL_COLOR_TEMPRATURE_ATTRIBUTE_ID);
  mfgSpecificId = EMBER_AF_NULL_MANUFACTURER_CODE;
  
  emAfApsFrameEndpointSetup(1, 1);
  emberAfSendUnicast(EMBER_OUTGOING_DIRECT,
                     *((int16u*)rBuf),
                     &globalApsFrame,
                     appZclBufferLen,
                     appZclBuffer);   
  
}
boolean emberAfPreCommandReceivedCallback(EmberAfClusterCommand* cmd)
{
  int8u *buf = cmd->buffer + cmd->payloadStartIndex;  

  if (ZCL_READ_ATTRIBUTES_RESPONSE_COMMAND_ID != cmd->commandId)
    return FALSE;
    
  if ((ZCL_ELECTRICAL_MEASUREMENT_CLUSTER_ID != cmd->apsFrame->clusterId) &&
       (ZCL_LEVEL_CONTROL_CLUSTER_ID != cmd->apsFrame->clusterId) &&
       (ZCL_SENGLED_CLUSTER_ID != cmd->apsFrame->clusterId))
    return FALSE;      

  sLen = SEAT_COMMAND+2;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = GET_DEVICE_ATTRIBUTE|RESPONSE;
  sBuf[sLen++] = buf[2];
  *((int16u*)&sBuf[sLen]) = cmd->source;
  sLen += 2;

  int8u rIndex = 4;
  sBuf[sLen++] = CURRENT_COLOR_TEMPRATURE_ATTRIBUTE_ID;
  sBuf[sLen++] = buf[rIndex++];
  rIndex += 4;
  sBuf[sLen++] = buf[rIndex++];
  rIndex += 4;
  *((int16u*)&sBuf[sLen]) = *((int16u *)&buf[rIndex]);
  sLen += 2;
  
  SerialPortDataSending(sLen);
  
  return TRUE;
}
static void processGetCoordinatorAttributeReq(int8u *rBuf, int8u len)
{
  if (len != 2)
  {
    ProcessInvalidCommand(LENTH_NOT_CORRECT);
    return;
  }
  
  sBuf = GetSendBuffer();

  *((int16u*)sBuf) = GET_COORDINATOR_ATTRIBUTE|RESPONSE;
  sBuf[sLen++] = 0x00;
  sBuf[sLen++] = 0x00;
  *((int16u*)&sBuf[sLen]) = 0x0000;
  sLen += 2;
  sBuf[sLen++] = 0x02;
  *((int16u*)&sBuf[sLen]) = 0x0000;
  sLen += 2;
  
  SerialPortDataSending(SEAT_COMMAND+8);
  
}

static void processOtaUpdataReq(int8u *rBuf, int8u len)
{
  int8u status = EEPROM_ERR;
  int16u totalIndex = *((int16u*)rBuf);
  int16u index = *((int16u*)&rBuf[2]);
  int8u  lenth = rBuf[4];

  if (len < 7)
  {
    ProcessInvalidCommand(LENTH_NOT_CORRECT);
    return;
  }
  
  if (emberAfOtaStorageDriverWriteCallback(&rBuf[5],
                                       (int32u)index*OTA_BUFFER_SIZE,
                                       (int32u)lenth)){
    status = EEPROM_SUCCESS;
  }

  sLen = SEAT_COMMAND+2;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = OTA_UPDATA|RESPONSE;
  sBuf[sLen++] = status;
  *((int16u*)&sBuf[sLen]) = totalIndex;
  sLen += 2;
  *((int16u*)&sBuf[sLen]) = index;
  sLen += 2;
  sBuf[sLen++] = rBuf[4];
  emberAfOtaStorageDriverReadCallback((int32u)index*OTA_BUFFER_SIZE,
                                      (int32u)lenth,
                                       &sBuf[sLen]);
  sLen += lenth;
  SerialPortDataSending(sLen);
}
static void processOtaUpdataFinishReq(int8u *rBuf, int8u len)
{
  int8u status = EEPROM_ERR;
  int32u imageSize;
  int16u temp;

  if (len != 4)
  {
    ProcessInvalidCommand(LENTH_NOT_CORRECT);
    return;
  }
  
  emberAfOtaStorageInitCallback();
  //imageSize = emberAfOtaGetImageSize();
  
  sLen = SEAT_COMMAND+2;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = OTA_UPDATA_FINISH|RESPONSE;
  sBuf[sLen++] = status;
  temp = (int16u)(imageSize/OTA_BUFFER_SIZE);
  if (imageSize%OTA_BUFFER_SIZE)
    temp++;  
  *((int16u*)&sBuf[sLen]) = temp;
  sLen += 2;
  SerialPortDataSending(sLen);
}

static void processOtaDeleteReq(int8u *rBuf, int8u len)
{
  int8u status;

  if (len != 2)
  {
    ProcessInvalidCommand(LENTH_NOT_CORRECT);
    return;
  }
  
  status = emberAfOtaStorageDriverInvalidateImageCallback();
  
  sLen = SEAT_COMMAND+2;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = OTA_DELETE|RESPONSE;
  sBuf[sLen++] = status;
  SerialPortDataSending(sLen);
}
#else
boolean emberAfPreCommandReceivedCallback(EmberAfClusterCommand* cmd)
{
  return TRUE;
}
boolean emberAfPluginTrustCenterJoinDecisionMakeJoinDecisionCallback(EmberNodeId nodeId,
                                                                     EmberEUI64 nodeEUI64)
{
  return TRUE;
}
#endif //SENGLED_UART_APP
// eof ciConsole.c


