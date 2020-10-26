/**************************************************************************//**
 
******************************************************************************/


/******************************************************************************
                    Includes section
******************************************************************************/
#include "CommandParse.h"
#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "app/util/zigbee-framework/zigbee-device-common.h"
#include <string.h>     // ""



#define FIRMWARE_VERSION (0x00000001ul)
#define HARDWARE_VERSION 0x01


#define COLOR_TEMPRATURE_MAX 370
#define COLOR_TEMPRATURE_MIN 153
#define COLOR_TEMPRATURE_D_VALUE  (COLOR_TEMPRATURE_MAX-COLOR_TEMPRATURE_MIN)


extern EmberApsFrame globalApsFrame;
extern int8u appZclBuffer[EMBER_AF_MAXIMUM_SEND_PAYLOAD_LENGTH];
extern int16u appZclBufferLen;
extern int16u mfgSpecificId;

static int8u sLen;
static int8u *sBuf;
static const int8u manufacturer[] = "sengled";
static const int8u model[] = "Z01-A19-Gateway";


void zclBufferAddByte(int8u byte);
void zclBufferAddWord(int16u word);
void zclBufferSetup(int8u frameType, int16u clusterId, int8u commandId);
void emAfApsFrameEndpointSetup(int8u srcEndpoint, int8u dstEndpoint);
EmberStatus emberScanForUnusedPanId(int32u channelMask, int8u duration);

//*********************************************************************
// Type conversion
//*********************************************************************
/*
  CharToHex
*/
int8u CharToHex(int8u src)
{
  int8u tmp;
  
  if (('0' <= src) && (src <= '9'))
  { tmp = '0';}
  else if (('a' <= src) && (src <= 'f'))
  { tmp = 'a'-0x0a;}
  else if (('A' <= src) && (src <= 'F'))
  { tmp = 'A'-0x0a;}

  return (src-tmp);
}
/*
  CharMacToHexMac
*/
void CharMacToHexMac(int8u* src, int8u* des)
{
  int8u i, tmp1, tmp2;

  for (i=0; i<8; i++)
  {
    tmp1 = CharToHex(src[0+(i<<1)]);    
    tmp2 = CharToHex(src[1+(i<<1)]);
    *des = (tmp1<<4)|tmp2;
    des++;
  }
}
int8u HexToChar(int8u src, boolean high)
{
  if (high == TRUE)
  { src >>= 4;}
  else
  { src &= 0x0f;}

  if (src <= 0x09)
  { return src+'0';}
  else if ((0x0a<=src) && (src<=0x0f))
  { return src+'a'-0x0a;}
}
void HexMacToCharMac(int8u *des, int8u *src, int8u len)
{
  while (len--)
  {
    *des = HexToChar(*src, TRUE);
    des++;
    *des = HexToChar(*src, FALSE);
    des++;
    src++;
  }
  *des = 0;
  des++;
  *des = 0;
}
//*********************************************************************
// Zcl Memory
//*********************************************************************
/*
  SetZclMemoryToZero
*/
void SetZclMemoryToZero(int8u len)
{
  do
  { 
    zclBufferAddByte(0);
  } while (--len) ;
}
/*
  CopyZclMemory
*/
void CopyZclMemory(int8u* scr, int8u len)
{
  do
  {
    zclBufferAddByte(*scr);
    scr++;
  } while (len--);
}
//*********************************************************************
// Current Mac
//*********************************************************************
static int8u currentMac[18];

#define GetCurrentMacPtr  currentMac  

void SaveCurrentMac(int8u *buf)
{
  for (int8u i=0; i<16; i++)
  { currentMac[i] = buf[i];}
  currentMac[16] = 0;
  currentMac[17] = 0;
}

//*********************************************************************
// Send Command
/*
   EMBER_ERR_FATAL::
   EMBER_TABLE_FULL::
   EMBER_INVALID_ENDPOINT::
   EMBER_MESSAGE_TOO_LONG::
   EMBER_NO_BUFFERS::
   EMBER_BAD_ARGUMENT::
   EMBER_INVALID_BINDING_INDEX::
   EMBER_NETWORK_DOWN::
   EMBER_MAX_MESSAGE_LIMIT_REACHED::
   EMBER_NETWORK_BUSY::
*/
//*********************************************************************
static EmberStatus SengledSend(int16u desNetAddress)
{    
  emAfApsFrameEndpointSetup(1, 1);  
  if (EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS == desNetAddress)
  {
    return emberAfSendBroadcast(desNetAddress,
                            &globalApsFrame,
                            appZclBufferLen,
                            appZclBuffer);
  }
  else
  {
    return emberAfSendUnicast(EMBER_OUTGOING_DIRECT,
                            desNetAddress,
                            &globalApsFrame,
                            appZclBufferLen,
                            appZclBuffer);
  }
}
//*********************************************************************
// Send Command
/*
   EMBER_ERR_FATAL::
   EMBER_TABLE_FULL::
   EMBER_INVALID_ENDPOINT::
   EMBER_MESSAGE_TOO_LONG::
   EMBER_NO_BUFFERS::
   EMBER_BAD_ARGUMENT::
   EMBER_INVALID_BINDING_INDEX::
   EMBER_NETWORK_DOWN::
   EMBER_MAX_MESSAGE_LIMIT_REACHED::
   EMBER_NETWORK_BUSY::
*/
//*********************************************************************
static EmberStatus SengledBraodcastSend(int16u desNetAddress)
{
  emAfApsFrameEndpointSetup(1, 1);  
  return emberAfSendBroadcast(desNetAddress,
                            &globalApsFrame,
                            appZclBufferLen,
                            appZclBuffer);
}

//*********************************************************************
// Send Command
/*
   EMBER_ERR_FATAL::
   EMBER_TABLE_FULL::
   EMBER_INVALID_ENDPOINT::
   EMBER_MESSAGE_TOO_LONG::
   EMBER_NO_BUFFERS::
   EMBER_BAD_ARGUMENT::
   EMBER_INVALID_BINDING_INDEX::
   EMBER_NETWORK_DOWN::
   EMBER_MAX_MESSAGE_LIMIT_REACHED::
   EMBER_NETWORK_BUSY::
*/
//*********************************************************************
static EmberStatus SengledGroupSend(int16u groupId)
{
  emAfApsFrameEndpointSetup(1, 1);  
  return emberAfSendMulticast(groupId,
                            &globalApsFrame,
                            appZclBufferLen,
                            appZclBuffer);
}

//*********************************************************************
// Sengled Send Response
//*********************************************************************
void SengledSendResponse(int16u sysId, int16u comId, EmberStatus status, int16u desNetAddress)
{
  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = sysId;
  sLen += 2;  
  *((int16u*)&sBuf[sLen]) = comId;
  sLen += 2;
  sBuf[sLen] = (status==EMBER_SUCCESS)?0:1; //flag
  sLen += 1;
  sBuf[sLen] = status;
  sLen += 1;
  *((int16u*)&sBuf[sLen]) = desNetAddress; //nwkAddr
  sLen += 2;
  SerialPortDataSending(sLen);
}
//*********************************************************************
// Sengled Send Plus Response
//*********************************************************************
void SengledSendPlusResponse(int16u sysId, int16u comId, EmberStatus status, int16u desNetAddress, int16u attributeId)
{
  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = sysId; //sysid
  sLen += 2;  
  *((int16u*)&sBuf[sLen]) = comId; //comid
  sLen += 2;
  sBuf[sLen] = (status==EMBER_SUCCESS)?0:1; //flag
  sLen += 1;
  sBuf[sLen] = status; //status
  sLen += 1;
  *((int16u*)&sBuf[sLen]) = desNetAddress; //nwkAddr
  sLen += 2;
  *((int16u*)&sBuf[sLen]) = attributeId; //attributeId
  sLen += 2;
  
  SerialPortDataSending(sLen);
}

static void SengledSendZclResponse(int16u sysId, int16u comId, EmberStatus status, int16u desNetAddress, int8u *mac)
{
  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = sysId; //SUBSYS_ID
  sLen += 2;  
  *((int16u*)&sBuf[sLen]) = comId; //CMD_ID
  sLen += 2; 
  sBuf[sLen] = (status==EMBER_SUCCESS)?0:1; //flag
  sLen += 1;
  sBuf[sLen] = status; //Status
  sLen += 1;
  for (int8u i=0; i<18; i++) //LAMP_MAC
  { 
    sBuf[sLen] = mac[i];
    sLen += 1;
  }
  *((int16u*)&sBuf[sLen]) = desNetAddress; //DEVICE_NTID
  sLen += 2;
  sBuf[sLen] = 1; //DEVICE_EDID
  sLen += 1;
  SerialPortDataSending(sLen);
}
//*********************************************************************
// Sengled Send Response
//*********************************************************************
void SengledSendBoradcastResponse(int16u sysId, int16u comId, EmberStatus status)
{
  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = sysId;
  sLen += 2;  
  *((int16u*)&sBuf[sLen]) = comId;
  sLen += 2;
  sBuf[sLen] = (status==EMBER_SUCCESS)?0:1; //flag
  sLen += 1;
  sBuf[sLen] = status;
  sLen += 1;
  SerialPortDataSending(sLen);
}
//*********************************************************************
// Sengled Send Response
//*********************************************************************
void SengledSendBoradcastPlusResponse(int16u sysId, int16u comId, EmberStatus status, int16u attributeId)
{
  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = sysId;
  sLen += 2;  
  *((int16u*)&sBuf[sLen]) = comId;
  sLen += 2;
  sBuf[sLen] = (status==EMBER_SUCCESS)?0:1; //flag
  sLen += 1;
  sBuf[sLen] = status;
  sLen += 1;
  *((int16u*)&sBuf[sLen]) = attributeId; //attributeId
  sLen += 2;
  SerialPortDataSending(sLen);
}

//*********************************************************************
// Create Network
//*********************************************************************
/** @brief Finished
 *
 * This callback is fired when the network-find plugin is finished with the
 * forming or joining process.  The result of the operation will be returned in
 * the status parameter.
 *
 * @param status   Ver.: always
 
  * Possible error responses and their meanings:
  * - ::EMBER_MAC_SCANNING, we are already scanning.
  * - ::EMBER_MAC_BAD_SCAN_DURATION, we have set a duration value that is not 0..14 inclusive.
  * - ::EMBER_MAC_INCORRECT_SCAN_TYPE, we have requested an undefined scanning type; 
  * - ::EMBER_MAC_INVALID_CHANNEL_MASK, our channel mask did not specify any valid channels on the current platform.
 */
void emberAfPluginNetworkFindFinishedCallback(EmberStatus status)
{  
  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = 3;  //SUBSYS_ID
  sLen += 2;  
  *((int16u*)&sBuf[sLen]) = 130; //CMD_ID
  sLen += 2;   
  sBuf[sLen] = (status==EMBER_SUCCESS)?0:1; //flag
  sLen += 1;
  sBuf[sLen] = status; //Status
  sLen += 1;
  sBuf[sLen] = emberGetRadioChannel(); //Channel ID
  sLen += 1;
  *((int16u*)&sBuf[sLen]) = emberGetPanId(); //Pan ID
  sLen += 2;
  
  SerialPortDataSending(sLen);
}
/*
  Create Network
  param duration     Sets the exponent of the number of scan periods,   
  where a scan period is 960 symbols, and a symbol is 16 microseconds.  
  The scan will occur for ((2^duration) + 1) scan periods.  
  The value of duration must be less than 15.  
  The time corresponding to the first few values are as follows: 
  0 = 31 msec, 1 = 46 msec, 2 = 77 msec, 3 = 138 msec, 4 = 261 msec, 5 = 507 msec, 6 = 998 msec.
*/
static void ProcessCreateNetworkReq(int8u *rBuf, int8u len)
{
  //int32u channelMask = 0x07fff800;
  //int32u channelMask = 0x02000000;
  int32u channelMask = *((int32u*)rBuf);
  
  EmberNetworkStatus status = emberAfNetworkState();

  if (status == EMBER_JOINED_NETWORK)
  {
    emberAfPluginNetworkFindFinishedCallback(EMBER_INVALID_CALL);
    return;
  }

  emberScanForUnusedPanId(channelMask,
                          EMBER_AF_PLUGIN_NETWORK_FIND_DURATION);  
}
//*********************************************************************
// Delete Network
//*********************************************************************
/*
  EMBER_INVALID_CALL:: indicates that the node is either not joined to a network or is already in the process of leaving.
*/
static void ProcessDeleteNetworkReq(int8u *rBuf, int8u len)
{
  EmberStatus status = emberLeaveNetwork();

  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = 4; //SUBSYS_ID
  sLen += 2;  
  *((int16u*)&sBuf[sLen]) = 1; //CMD_ID
  sLen += 2;
  sBuf[sLen] = (status==EMBER_SUCCESS)?0:1; //flag
  sLen += 1;
  sBuf[sLen] = status; //State
  sLen += 1;

  SerialPortDataSending(sLen);
}
//*********************************************************************
// Change Channel
//*********************************************************************
/*
* @return An ::EmberStatus value. 
* - ::EMBER_SUCCESS
* - ::EMBER_NO_BUFFERS
* - ::EMBER_NETWORK_DOWN
* - ::EMBER_NETWORK_BUSY
*/
static void ProcessChangeChannelReq(int8u *rBuf, int8u len)
{ 
  EmberStatus status = emberChannelChangeRequest(rBuf[0]);
  
  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = 4; //SUBSYS_ID
  sLen += 2;  
  *((int16u*)&sBuf[sLen]) = 2; //CMD_ID
  sLen += 2;
  sBuf[sLen] = (status==EMBER_SUCCESS)?0:1; //flag
  sLen += 1;
  sBuf[sLen] = status; //State
  sLen += 1;

  SerialPortDataSending(sLen);
}
//*********************************************************************
// Set Permit Joining Time
//*********************************************************************
/*
* @return An ::EmberStatus value. 
* - ::EMBER_SUCCESS
* - ::EMBER_NO_BUFFERS
* - ::EMBER_NETWORK_DOWN
* - ::EMBER_NETWORK_BUSY
*/
static void ProcessSetPermitJoiningTimeReq(int8u *rBuf, int8u len)
{
  EmberStatus status = emAfPermitJoin(rBuf[0], TRUE); // broadcast  

  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = 4; //SUBSYS_ID
  sLen += 2;  
  *((int16u*)&sBuf[sLen]) = 3; //CMD_ID
  sLen += 2;
  sBuf[sLen] = (status==EMBER_SUCCESS)?0:1;  //flag
  sLen += 1;
  sBuf[sLen] = status; //State
  sLen += 1;
  sBuf[sLen] = rBuf[0]; //OpenTime
  sLen += 1;

  SerialPortDataSending(sLen);
}
//*********************************************************************
// New Device Join Report
//*********************************************************************
void NewDeviceJoinReport(EmberNodeId nodeId, EmberEUI64 nodeEUI64)
{ 
  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = 4; //SUBSYS_ID
  sLen += 2;  
  *((int16u*)&sBuf[sLen]) = 4; //CMD_ID
  sLen += 2;
  sBuf[sLen] = 0x00; //flag
  sLen += 1;
  sBuf[sLen] = 0x00; //State
  sLen += 1;  
  *((int16u*)&sBuf[sLen]) = nodeId; //nwkAddr
  sLen += 2;
  HexMacToCharMac(&sBuf[sLen], nodeEUI64, EUI64_SIZE); //ieeeAddr
  sLen += 18;
  
  SerialPortDataSending(sLen);
}
//*********************************************************************
// Reset To The Factory
//*********************************************************************
static void ProcessResetToTheFactoryReq(int8u *rBuf, int8u len)
{
  zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
                 ZCL_BASIC_CLUSTER_ID, 
                 ZCL_RESET_TO_FACTORY_DEFAULTS_COMMAND_ID);
  int16u netAddress = *((int16u*)rBuf);
  EmberStatus status = SengledSend(netAddress);

  if (EMBER_SUCCESS != status)
  { SengledSendResponse(4, 5, status, netAddress);}
}
//*********************************************************************
// Remove Device
//*********************************************************************
/*
* @return An ::EmberStatus value. For any result other than
 *    ::EMBER_SUCCESS, the message will not be sent.
 * - ::EMBER_SUCCESS - The message has been submitted for transmission.
 * - ::EMBER_INVALID_BINDING_INDEX - The \c bindingTableIndex  refers to a non-unicast binding.
 * - ::EMBER_NETWORK_DOWN - The node is not part of a network.
 * - ::EMBER_MESSAGE_TOO_LONG - The message is too large to fit in a MAC layer frame.
 * - ::EMBER_MAX_MESSAGE_LIMIT_REACHED - The EMBER_APS_UNICAST_MESSAGE_COUNT limit has been reached.
*/
static void ProcessRemoveDeviceReq(int8u *rBuf, int8u len)
{
  EmberNodeId nodeId;
  EmberEUI64  nodeEUI64;

  SaveCurrentMac(rBuf); //Save Current Mac
  
  CharMacToHexMac(rBuf, nodeEUI64);
  nodeId = *((int16u*)&rBuf[18]);
  
  EmberStatus status = emberLeaveRequest(nodeId,
                                         nodeEUI64,
                                         EMBER_ZIGBEE_LEAVE_AND_REMOVE_CHILDREN,
                                         EMBER_APS_OPTION_NONE);
  if (EMBER_SUCCESS != status)
  { SengledSendZclResponse(3, 131, status, nodeId, rBuf);}
}
//*********************************************************************
// binding
//*********************************************************************
static EmberNodeId desNodeId;
static EmberEUI64  srcNodeEUI64;
static int8u       bindingStep;
static boolean     bindingEnable;
EmberEventControl  BindingEventControl;
static const int16u bindingClusterTable[] = {ZCL_ON_OFF_CLUSTER_ID,ZCL_LEVEL_CONTROL_CLUSTER_ID,ZCL_COLOR_CONTROL_CLUSTER_ID,ZCL_SIMPLE_METERING_CLUSTER_ID};

enum {
  CLUSTER_ON_OFF,
  CLUSTER_LEVEL_CONTROL,
  CLUSTER_COLOR_CONTROL,
  CLUSTER_METERING,
};
/*
* @return An ::EmberStatus value. For any result other than
 *    ::EMBER_SUCCESS, the message will not be sent.
 * - ::EMBER_SUCCESS - The message has been submitted for transmission.
 * - ::EMBER_INVALID_BINDING_INDEX - The \c bindingTableIndex  refers to a non-unicast binding.
 * - ::EMBER_NETWORK_DOWN - The node is not part of a network.
 * - ::EMBER_MESSAGE_TOO_LONG - The message is too large to fit in a MAC layer frame.
 * - ::EMBER_MAX_MESSAGE_LIMIT_REACHED - The EMBER_APS_UNICAST_MESSAGE_COUNT limit has been reached.
*/
void BindingEventFunction(void) 
{
  if (TRUE ==bindingEnable)
  {
    bindingEnable = FALSE;
    
    EmberStatus status = emberBindRequest(desNodeId, // who gets the bind req
                                        srcNodeEUI64, // source eui IN the binding
                                        1,
                                        bindingClusterTable[bindingStep],       
                                        UNICAST_BINDING, // binding type
                                        emberGetEui64(),    // destination eui IN the binding
                                        0,               // groupId for new binding
                                        1,
                                        EMBER_AF_DEFAULT_APS_OPTIONS);

    if (status != EMBER_SUCCESS)
    { SengledSendResponse(4, 6, status, desNodeId);}
  }
}
/*
  Binding
*/
static void ProcessBindingReq(int8u *rBuf, int8u len)
{
  desNodeId = *((int16u*)rBuf);
  CharMacToHexMac(&rBuf[2], srcNodeEUI64);

  bindingEnable = TRUE;
  bindingStep = CLUSTER_ON_OFF;
  
  emberEventControlSetDelayMS(BindingEventControl, 500); //0.5s
}
//*********************************************************************
// Config Report
//*********************************************************************
enum {
  ATTRIBUTE_ON_OFF,
  ATTRIBUTE_LEVEL,
  ATTRIBUTE_COLOR_TEMPRATURE,
  ATTRIBUTE_POWER,
  ATTRIBUTE_CONSUMPTION
};
/*
  Config Report Response
*/
boolean emberAfConfigureReportingResponseCallback(EmberAfClusterId clusterId, 
                                                  int8u *buffer, 
                                                  int16u bufLen)
{
  int8u status;
  
  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = 4;
  sLen += 2;  
  *((int16u*)&sBuf[sLen]) = 7;
  sLen += 2;
  status = (EmberAfStatus)emberAfGetInt8u(buffer, 0, bufLen);
  sBuf[sLen] = (status==EMBER_SUCCESS)?0:1; //flag
  sLen += 1;
  sBuf[sLen] = status;
  sLen += 1;
  *((int16u*)&sBuf[sLen]) = emberAfCurrentCommand()->source;;
  sLen += 2;
  SerialPortDataSending(sLen);
  
  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}
/*
  Config Report
*/
static void ProcessConfigReportReq(int8u *rBuf, int8u len)
{
  int16u clusterId;
  int16u attributeId;  
  int16u attrId;
  int8u dataType;
  
  attrId = *((int16u*)&rBuf[2]);
  
  if (attrId == ATTRIBUTE_ON_OFF)
  {
    clusterId = ZCL_ON_OFF_CLUSTER_ID;
    attributeId = ZCL_ON_OFF_ATTRIBUTE_ID;
    dataType = ZCL_BOOLEAN_ATTRIBUTE_TYPE;
  }
  else if (attrId == ATTRIBUTE_LEVEL)
  {
    clusterId = ZCL_LEVEL_CONTROL_CLUSTER_ID;
    attributeId = ZCL_CURRENT_LEVEL_ATTRIBUTE_ID;
    dataType = ZCL_INT8U_ATTRIBUTE_TYPE;
  }
  else if (attrId == ATTRIBUTE_COLOR_TEMPRATURE)
  { 
    clusterId = ZCL_COLOR_CONTROL_CLUSTER_ID;
    attributeId = ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID;
    dataType = ZCL_INT16U_ATTRIBUTE_TYPE;
  }
  else if (attrId == ATTRIBUTE_POWER)
  { 
    clusterId = ZCL_SIMPLE_METERING_CLUSTER_ID;
    attributeId = ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID;
    dataType = ZCL_INT24S_ATTRIBUTE_TYPE;
  }
  else if (attrId == ATTRIBUTE_CONSUMPTION)
  { 
    clusterId = ZCL_SIMPLE_METERING_CLUSTER_ID;
    attributeId = ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID;
    dataType = ZCL_INT48U_ATTRIBUTE_TYPE;
  }
    
  zclBufferSetup(ZCL_PROFILE_WIDE_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK,
                 clusterId,
                 ZCL_CONFIGURE_REPORTING_COMMAND_ID);  
  zclBufferAddByte(EMBER_ZCL_REPORTING_DIRECTION_REPORTED);  
  zclBufferAddWord(attributeId);
  zclBufferAddByte(dataType);
  zclBufferAddWord(*((int16u*)&rBuf[4])); // minimum reporting interval
  zclBufferAddWord(*((int16u*)&rBuf[6])); // maximum reporting interval

  // If the data type is analog, then the reportable change field is the same
  // size as the data type.  Otherwise, it is omitted.
  if (emberAfGetAttributeAnalogOrDiscreteType(dataType)
      == EMBER_AF_DATA_TYPE_ANALOG) {
    int8u dataSize = emberAfGetDataSize(dataType);    
    CopyZclMemory(&rBuf[8], dataSize);
  }

  int16u netAddress = *((int16u*)rBuf);
  EmberStatus status = SengledSend(netAddress);

  if (EMBER_SUCCESS != status)
  { SengledSendResponse(4, 7, status, netAddress);}
}
//*********************************************************************
// Get Device Attribute
//*********************************************************************
enum {
  INFORMATION_MANUFACTURE,
  INFORMATION_MODEL,
  INFORMATION_FIRMWARE_VERSION,
  INFORMATION_HARDWARE_VERSION,
  INFORMATION_NETWORK_STATE,
  INFORMATION_CHANNEL,
  INFORMATION_ON_OFF,
  INFORMATION_CURRENT_LEVEL,
  INFORMATION_COLOR_TEMPRATURE,
  INFORMATION_POWER,
  INFORMATION_CONSUMPTION,
};
#define COORDINATOR_NETWORK_ID 0x0000

int16u GetInformation(int16u cluster, int16u attribute)
{
  int16u information;
  
  if (cluster == ZCL_BASIC_CLUSTER_ID)  //attributeId
  {
    if (attribute == ZCL_MANUFACTURER_NAME_ATTRIBUTE_ID)
    { information = INFORMATION_MANUFACTURE;}
    else if (attribute == ZCL_MODEL_IDENTIFIER_ATTRIBUTE_ID)
    { information = INFORMATION_MODEL;}
    else if (attribute == ZCL_HW_VERSION_ATTRIBUTE_ID)
    { information = INFORMATION_HARDWARE_VERSION;}  
  }
  else if (cluster == ZCL_OTA_BOOTLOAD_CLUSTER_ID)
  {
    if (attribute == ZCL_CURRENT_FILE_VERSION_ATTRIBUTE_ID)
    { information = INFORMATION_FIRMWARE_VERSION;}
  }
  else if (cluster == ZCL_ON_OFF_CLUSTER_ID)
  {
    if (attribute == ZCL_ON_OFF_ATTRIBUTE_ID)
    { information = INFORMATION_ON_OFF;}
  }
  else if (cluster == ZCL_LEVEL_CONTROL_CLUSTER_ID)
  {
    if (attribute == ZCL_CURRENT_LEVEL_ATTRIBUTE_ID)
    { information = INFORMATION_CURRENT_LEVEL;}
  }
  else if (cluster == ZCL_COLOR_CONTROL_CLUSTER_ID)
  {
    if (attribute == ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID)
    { information = INFORMATION_COLOR_TEMPRATURE;}
  }
  else if (cluster == ZCL_SIMPLE_METERING_CLUSTER_ID)
  {
    if (attribute == ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID)
    { information = INFORMATION_POWER;}
    else if (attribute == ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID)
    { information = INFORMATION_CONSUMPTION;}
  }
  
  return information;
}

/*
  Read Attributes Response
*/
boolean emberAfReadAttributesResponseCallback(EmberAfClusterId clusterId, 
                                              int8u *buffer, 
                                              int16u bufLen)
{
  int16u bufIndex = 0;
  EmberAfAttributeId attributeId;
  int8u status;
  
  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = 4; sLen += 2;  //SUBSYS_ID
  *((int16u*)&sBuf[sLen]) = 8; sLen += 2; //CMD_ID
  attributeId = (EmberAfAttributeId)emberAfGetInt16u(buffer, bufIndex, bufLen);
  bufIndex += 2;
  status = (EmberAfStatus)emberAfGetInt8u(buffer, bufIndex, bufLen);
  bufIndex++;
  sBuf[sLen] = (status==EMBER_SUCCESS)?0:1; //flag
  sLen += 1;
  sBuf[sLen] = status; sLen += 1;     //state  
  *((int16u*)&sBuf[sLen]) = emberAfCurrentCommand()->source; sLen += 2;  //desNwkAddr  
  *((int16u*)&sBuf[sLen]) = GetInformation(clusterId, attributeId); sLen += 2; 

  if (status == EMBER_ZCL_STATUS_SUCCESS) //attributeValue
  {
    int8u dataType, dataSize;
    dataType = emberAfGetInt8u(buffer, bufIndex, bufLen);
    bufIndex++;

    // For strings, the data size is the length of the string (specified by
    // the first byte of data) plus one for the length byte itself.  For
    // everything else, the size is just the size of the data type.
    dataSize = (emberAfIsThisDataTypeAStringType(dataType)
                ? emberAfStringLength(buffer + bufIndex) + 1
                : emberAfGetDataSize(dataType));

    if (bufIndex + dataSize <= bufLen) 
    {
      for (int8u i=0; i<dataSize; i++)
      { sBuf[sLen] = buffer[bufIndex+i]; sLen += 1;}
    }
  }
  SerialPortDataSending(sLen);
  
  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}
void GetClusterAndAttribute(int16u information, int16u *cluster, int16u *attribute)
{
  if (information == INFORMATION_MANUFACTURE)
  {
    *cluster = ZCL_BASIC_CLUSTER_ID;
    *attribute = ZCL_MANUFACTURER_NAME_ATTRIBUTE_ID;
  }
  else if (information == INFORMATION_MODEL)
  {
    *cluster = ZCL_BASIC_CLUSTER_ID;
    *attribute = ZCL_MODEL_IDENTIFIER_ATTRIBUTE_ID;
  }
  else if (information == INFORMATION_FIRMWARE_VERSION)
  {
    *cluster = ZCL_OTA_BOOTLOAD_CLUSTER_ID;
    *attribute = ZCL_CURRENT_FILE_VERSION_ATTRIBUTE_ID;
  }
  else if (information == INFORMATION_HARDWARE_VERSION)
  {
    *cluster = ZCL_BASIC_CLUSTER_ID;
    *attribute = ZCL_HW_VERSION_ATTRIBUTE_ID;
  }
  else if (information == INFORMATION_ON_OFF)
  {
    *cluster = ZCL_ON_OFF_CLUSTER_ID;
    *attribute = ZCL_ON_OFF_ATTRIBUTE_ID;
  }
  else if (information == INFORMATION_CURRENT_LEVEL)
  {
    *cluster = ZCL_LEVEL_CONTROL_CLUSTER_ID;
    *attribute = ZCL_CURRENT_LEVEL_ATTRIBUTE_ID;
  }
  else if (information == INFORMATION_COLOR_TEMPRATURE)
  {
    *cluster = ZCL_COLOR_CONTROL_CLUSTER_ID;
    *attribute = ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID;
  }
  else if (information == INFORMATION_POWER)
  {
    *cluster = ZCL_SIMPLE_METERING_CLUSTER_ID;
    *attribute = ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID;
  }
  else if (information == INFORMATION_CONSUMPTION)
  {
    *cluster = ZCL_SIMPLE_METERING_CLUSTER_ID;
    *attribute = ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID;
  }
}
/*
  Get Device Attribute
*/
static void processGetDeviceAttributeReq(int8u *rBuf, int8u len)
{
  int8u i;
  int16u information = *((int16u*)&rBuf[2]);
    
  if (COORDINATOR_NETWORK_ID == *((int16u*)rBuf))
  {
    sLen = SEAT_COMMAND;
    sBuf = GetSendBuffer();
    *((int16u*)sBuf) = 4; sLen += 2;  //SUBSYS_ID
    *((int16u*)&sBuf[sLen]) = 8; sLen += 2; //CMD_ID
    sBuf[sLen] = 0x00; sLen += 1;     //Flag
    sBuf[sLen] = 0x00; sLen += 1;     //state
    *((int16u*)&sBuf[sLen]) = COORDINATOR_NETWORK_ID; sLen += 2;  //desNwkAddr
    *((int16u*)&sBuf[sLen]) = information; sLen += 2; //information
    if (information == INFORMATION_MANUFACTURE) //informationValue
    {
      int8u len = strlen(manufacturer);
      sBuf[sLen] = len; sLen += 1;
      for (i=0; i<len; i++)
      { sBuf[sLen] = manufacturer[i]; sLen += 1;}      
    }
    else if (information == INFORMATION_MODEL)
    {
      int8u len = strlen(model);
      sBuf[sLen] = len; sLen += 1;
      for (i=0; i<len; i++)
      { sBuf[sLen] = model[i]; sLen += 1;}      
    }
    else if (information == INFORMATION_FIRMWARE_VERSION)
    { *((int32u*)&sBuf[sLen]) = FIRMWARE_VERSION; sLen += 4;}
    else if (information == INFORMATION_HARDWARE_VERSION)
    { sBuf[sLen] = HARDWARE_VERSION; sLen += 1;}
    else if (information == INFORMATION_NETWORK_STATE)
    { sBuf[sLen] = emberAfNetworkState(); sLen += 1;}
    else if (information == INFORMATION_CHANNEL)
    { sBuf[sLen] = emberGetRadioChannel(); sLen += 1;}
    
    SerialPortDataSending(sLen);
  }
  else
  {
    int16u clusterId;
    int16u attributeId;
    
    GetClusterAndAttribute(information, &clusterId, &attributeId);
    
    if (information == INFORMATION_FIRMWARE_VERSION)
    {
      zclBufferSetup(ZCL_PROFILE_WIDE_COMMAND|ZCL_FRAME_CONTROL_SERVER_TO_CLIENT | ZCL_DISABLE_DEFAULT_RESPONSE_MASK,
                   clusterId,
                   ZCL_READ_ATTRIBUTES_COMMAND_ID);
    }
    else
    {
      zclBufferSetup(ZCL_PROFILE_WIDE_COMMAND|ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK,
                   clusterId,
                   ZCL_READ_ATTRIBUTES_COMMAND_ID);
    }
    zclBufferAddWord(attributeId);

    int16u netAddress = *((int16u*)rBuf);
    EmberStatus status = SengledSend(netAddress);

    if (EMBER_SUCCESS != status)
    { SengledSendPlusResponse(4, 8, status, netAddress, information);}
  }  
}
//*********************************************************************
// Set Device Attribute
//*********************************************************************
enum {
  INFORMATION_ONLEVEL,
};

static void processSetDeviceAttributeReq(int8u *rBuf, int8u len)
{
  int16u information = *((int16u*)&rBuf[2]);
  int16u clusterId;
  int16u attributeId;
  int8u  type;

  if (INFORMATION_ONLEVEL == information)
  {
    clusterId = ZCL_LEVEL_CONTROL_CLUSTER_ID;
    attributeId = ZCL_ON_LEVEL_ATTRIBUTE_ID;
    type = ZCL_INT8U_ATTRIBUTE_TYPE;
  }
  
  zclBufferSetup(ZCL_PROFILE_WIDE_COMMAND|ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK,
                 clusterId,
                 ZCL_WRITE_ATTRIBUTES_COMMAND_ID);
  zclBufferAddWord(attributeId);
  zclBufferAddByte(type);
  zclBufferAddByte(rBuf[4]);

  int16u netAddress = *((int16u*)rBuf);
  EmberStatus status = SengledSend(netAddress);

  if (EMBER_SUCCESS != status)
  { SengledSendPlusResponse(4, 10, status, netAddress, information);}
}
static void processAllSetDeviceAttributeReq(int8u *rBuf, int8u len)
{
  int16u information = *((int16u*)rBuf);
  int16u clusterId;
  int16u attributeId;
  int8u  type;

  if (INFORMATION_ONLEVEL == information)
  {
    clusterId = ZCL_LEVEL_CONTROL_CLUSTER_ID;
    attributeId = ZCL_ON_LEVEL_ATTRIBUTE_ID;
    type = ZCL_INT8U_ATTRIBUTE_TYPE;
  }
  
  zclBufferSetup(ZCL_PROFILE_WIDE_COMMAND|ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK,
                 clusterId,
                 ZCL_WRITE_ATTRIBUTES_COMMAND_ID);
  zclBufferAddWord(attributeId);
  zclBufferAddByte(type);
  zclBufferAddByte(rBuf[2]);

  EmberStatus status = SengledBraodcastSend(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS);
  
  if (EMBER_SUCCESS != status)
  { SengledSendBoradcastPlusResponse(4, 11, status, information);}
}

//*********************************************************************
// Report Attributes
//*********************************************************************
int8u ColorTempratureToPercentage(int16u ct)
{
  // 153 -370  6500 - 2700k
  if (ct <= COLOR_TEMPRATURE_MIN)
  { return 100;}
  else if (COLOR_TEMPRATURE_MAX <= ct)
  { return 0;}
  else
  { return 100 - ((ct-COLOR_TEMPRATURE_MIN)*100)/COLOR_TEMPRATURE_D_VALUE;}
}
// emberAfReportAttributesCallback
boolean emberAfReportAttributesCallback(EmberAfClusterId clusterId, 
                                                   int8u *buffer, 
                                                   int16u bufLen)
{
  int8u i;
  int16u bufIndex = 0;
  EmberAfAttributeId attributeId;
  
  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = 4; sLen += 2;  //SUBSYS_ID
  *((int16u*)&sBuf[sLen]) = 9; sLen += 2; //CMD_ID
  sBuf[sLen] = 0x00; sLen += 1;     //Flag
  sBuf[sLen] = 0x00; sLen += 1;     //state  
  *((int16u*)&sBuf[sLen]) = emberAfCurrentCommand()->source; sLen += 2;  //desNwkAddr
  
  attributeId = (EmberAfAttributeId)emberAfGetInt16u(buffer, bufIndex, bufLen); //messageId,message
  bufIndex += 2;bufIndex++;  
  if (clusterId == ZCL_ON_OFF_CLUSTER_ID)
  {
    if (attributeId == ZCL_ON_OFF_ATTRIBUTE_ID)
    { 
      *((int16u*)&sBuf[sLen]) = ATTRIBUTE_ON_OFF; sLen += 2;
      sBuf[sLen] = buffer[bufIndex];sLen += 1;
      for (i=1; i<8; i++)
      { sBuf[sLen] = 0;sLen += 1;}
    }  
  }
  else if (clusterId == ZCL_LEVEL_CONTROL_CLUSTER_ID)
  {
    if (attributeId == ZCL_CURRENT_LEVEL_ATTRIBUTE_ID)
    { 
      *((int16u*)&sBuf[sLen]) = ATTRIBUTE_LEVEL; sLen += 2;
      sBuf[sLen] = buffer[bufIndex];sLen += 1;
      for (i=1; i<8; i++)
      { sBuf[sLen] = 0;sLen += 1;}
    }
  }
  else if (clusterId == ZCL_COLOR_CONTROL_CLUSTER_ID)
  {
    if (attributeId == ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID)
    { 
      *((int16u*)&sBuf[sLen]) = ATTRIBUTE_COLOR_TEMPRATURE; sLen += 2; //attributeId
      sBuf[sLen] = ColorTempratureToPercentage(*((int16u*)&buffer[bufIndex]));sLen += 1;
      for (i=1; i<8; i++)
      { sBuf[sLen] = 0;sLen += 1;}
    }
  }
  else if (clusterId == ZCL_SIMPLE_METERING_CLUSTER_ID)
  {
    if (attributeId == ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID)
    { 
      *((int16u*)&sBuf[sLen]) = ATTRIBUTE_CONSUMPTION; sLen += 2; //attributeId
      for (i=0; i<6; i++)
      { sBuf[sLen+i] = buffer[bufIndex+i];}
      for (i=6; i<8; i++)
      { sBuf[sLen+i] = 0;}
      sLen += 8;
    }
    else if (attributeId == ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID)
    { 
      *((int16u*)&sBuf[sLen]) = ATTRIBUTE_POWER; sLen += 2; //attributeId
      for (i=0; i<3; i++)
      { sBuf[sLen+i] = buffer[bufIndex+i];}
      for (i=3; i<8; i++)
      { sBuf[sLen+i] = 0;}
      sLen += 8;
    }
  }
  SerialPortDataSending(sLen);
  
  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return TRUE;
}
//*********************************************************************
// Group add
//*********************************************************************
static void processGroupAddReq(int8u *rBuf, int8u len)
{
  SaveCurrentMac(rBuf); //Save Current Mac
  
  zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
               ZCL_GROUPS_CLUSTER_ID, 
               ZCL_ADD_GROUP_COMMAND_ID);
  zclBufferAddWord(*((int16u*)&rBuf[21]));
  zclBufferAddByte(0);
  int16u netAddress = *((int16u*)&rBuf[18]);  
  EmberStatus status = SengledSend(netAddress);
  
  if (EMBER_SUCCESS != status)
  { SengledSendZclResponse(1, 144, status, netAddress, rBuf);}
}
//*********************************************************************
// Group remove
//*********************************************************************
static void processGroupRemoveReq(int8u *rBuf, int8u len)
{
  SaveCurrentMac(rBuf); //Save Current Mac
  
  zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
               ZCL_GROUPS_CLUSTER_ID, 
               ZCL_REMOVE_GROUP_COMMAND_ID);
  zclBufferAddWord(*((int16u*)&rBuf[21]));
  int16u netAddress = *((int16u*)&rBuf[18]);  
  EmberStatus status = SengledSend(netAddress);
  
  if (EMBER_SUCCESS != status)
  { SengledSendZclResponse(1, 145, status, netAddress, rBuf);}
}

//*********************************************************************
// Onoff
//*********************************************************************
static void processOnoffReq(int8u *rBuf, int8u len)
{
  int8u command;

  SaveCurrentMac(rBuf); //Save Current Mac
  
  if (rBuf[18] == 0) 
  { command = ZCL_OFF_COMMAND_ID;}
  else
  { command = ZCL_ON_COMMAND_ID;}  
  zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
               ZCL_ON_OFF_CLUSTER_ID, 
               command);
  int16u netAddress = *((int16u*)&rBuf[19]);
  EmberStatus status = SengledSend(netAddress);
  
  if (EMBER_SUCCESS != status)
  { SengledSendZclResponse(1, 128, status, netAddress, rBuf);}
}
//*********************************************************************
// Level
//*********************************************************************
static void processLevelReq(int8u *rBuf, int8u len)
{
  SaveCurrentMac(rBuf); //Save Current Mac
  
  zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
                 ZCL_LEVEL_CONTROL_CLUSTER_ID, 
                 ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID);
  zclBufferAddByte(rBuf[18]);  
  zclBufferAddWord(*(int16u *)&rBuf[19]);
  int16u netAddress = *((int16u*)&rBuf[21]);
  EmberStatus status = SengledSend(netAddress);
  
  if (EMBER_SUCCESS != status)
  { SengledSendZclResponse(1, 127, status, netAddress, rBuf);}
}
//*********************************************************************
// Color Temprature
//*********************************************************************
/*
  Get Color Temprature
*/
int16u PercentageToColorTemprature(int8u ct)
{
  // 153 -370  6500 - 2700k
  if (ct == 0)
  { return COLOR_TEMPRATURE_MAX;}
  else if (100 <= ct)
  { return COLOR_TEMPRATURE_MIN;}
  else
  { return COLOR_TEMPRATURE_MAX - (ct*COLOR_TEMPRATURE_D_VALUE)/100;}
}

/*
  Color Temprature
*/
static void processColorTempratureReq(int8u *rBuf, int8u len)
{
  SaveCurrentMac(rBuf); //Save Current Mac
  
  zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
                 ZCL_COLOR_CONTROL_CLUSTER_ID, 
                 ZCL_MOVE_TO_COLOR_TEMPERATURE_COMMAND_ID);
  
  zclBufferAddWord(PercentageToColorTemprature(rBuf[18]));  
  zclBufferAddWord(*(int16u *)&rBuf[19]);
  int16u netAddress = *((int16u*)&rBuf[21]);
  EmberStatus status = SengledSend(netAddress);
  
  if (EMBER_SUCCESS != status)
  { SengledSendZclResponse(1, 126, status, netAddress, rBuf);}
}
//*********************************************************************
// Identify
//*********************************************************************
static void processIdentifyReq(int8u *rBuf, int8u len)
{
  SaveCurrentMac(rBuf); //Save Current Mac

  zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
                 ZCL_IDENTIFY_CLUSTER_ID, 
                 ZCL_IDENTIFY_COMMAND_ID);
  
  zclBufferAddWord(120);  // 2m
  int16u netAddress = *((int16u*)&rBuf[18]);
  EmberStatus status = SengledSend(netAddress);
  
  if (EMBER_SUCCESS != status)
  { SengledSendZclResponse(1, 125, status, netAddress, rBuf);}
}
//*********************************************************************
// AllOnoff
//*********************************************************************
static void processAllOnoffReq(int8u *rBuf, int8u len)
{
  int8u command;

  if (rBuf[0] == 0) 
  { command = ZCL_OFF_COMMAND_ID;}
  else
  { command = ZCL_ON_COMMAND_ID;}  
  zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
               ZCL_ON_OFF_CLUSTER_ID, 
               command);
  EmberStatus status = SengledBraodcastSend(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS);
  
  if (EMBER_SUCCESS != status)
  { SengledSendBoradcastResponse(1, 138, status);}
}
//*********************************************************************
// All Level
//*********************************************************************
static void processAllLevelReq(int8u *rBuf, int8u len)
{
  zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
                 ZCL_LEVEL_CONTROL_CLUSTER_ID, 
                 ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID);
  zclBufferAddByte(rBuf[0]);  
  zclBufferAddWord(*(int16u *)&rBuf[1]);
  EmberStatus status = SengledBraodcastSend(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS);
  
  if (EMBER_SUCCESS != status)
  { SengledSendBoradcastResponse(1, 139, status);}
}
/*
  Color Temprature
*/
static void processAllColorTempratureReq(int8u *rBuf, int8u len)
{
  zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
                 ZCL_COLOR_CONTROL_CLUSTER_ID, 
                 ZCL_MOVE_TO_COLOR_TEMPERATURE_COMMAND_ID);
  
  zclBufferAddWord(PercentageToColorTemprature(rBuf[0]));  
  zclBufferAddWord(*(int16u *)&rBuf[1]);
  EmberStatus status = SengledBraodcastSend(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS);
  
  if (EMBER_SUCCESS != status)
  { SengledSendBoradcastResponse(1, 140, status);}
}
//*********************************************************************
// Group Onoff
//*********************************************************************
static void processGroupOnoffReq(int8u *rBuf, int8u len)
{
  int8u command;

  if (rBuf[2] == 0) 
  { command = ZCL_OFF_COMMAND_ID;}
  else
  { command = ZCL_ON_COMMAND_ID;}  
  zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
               ZCL_ON_OFF_CLUSTER_ID, 
               command);
  EmberStatus status = SengledGroupSend(*((int16u*)rBuf));
  
  if (EMBER_SUCCESS != status)
  { SengledSendBoradcastResponse(1, 141, status);}
}
//*********************************************************************
// Group  Level
//*********************************************************************
static void processGroupLevelReq(int8u *rBuf, int8u len)
{
  zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
                 ZCL_LEVEL_CONTROL_CLUSTER_ID, 
                 ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID);
  zclBufferAddByte(rBuf[2]);  
  zclBufferAddWord(*(int16u *)&rBuf[3]);
  EmberStatus status = SengledGroupSend(*((int16u*)rBuf));
  
  if (EMBER_SUCCESS != status)
  { SengledSendBoradcastResponse(1, 142, status);}
}
/*
  Color Temprature
*/
static void processGroupColorTempratureReq(int8u *rBuf, int8u len)
{
  zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
                 ZCL_COLOR_CONTROL_CLUSTER_ID, 
                 ZCL_MOVE_TO_COLOR_TEMPERATURE_COMMAND_ID);
  
  zclBufferAddWord(PercentageToColorTemprature(rBuf[2]));  
  zclBufferAddWord(*(int16u *)&rBuf[3]);
  EmberStatus status = SengledGroupSend(*((int16u*)rBuf));
  
  if (EMBER_SUCCESS != status)
  { SengledSendBoradcastResponse(1, 143, status);}
}

//*********************************************************************
// OTA
//*********************************************************************
boolean emberAfOtaStorageDriverWriteCallback(const int8u* dataToWrite,
                                             int32u offset, 
                                             int32u length);
boolean emberAfOtaStorageDriverReadCallback(int32u offset, 
                                            int32u length,
                                            int8u* returnData);
int8u* GetOtaUpdataBuffer(void);


static boolean otaFileState = FALSE;
boolean otaFileDataUpdata = FALSE;

boolean GetOtaFileState(void)
{
  return otaFileState;
}

void NoticeZIGBDeviceNewOTAFile(int8u *rBuf, int8u len)
{
  if (rBuf[0] == 1)
    otaFileState = TRUE;
  else
    otaFileState = FALSE;

  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = 0x0005; //SUBSYS_ID
  sLen += 2;  
  *((int16u*)&sBuf[sLen]) = 0x0001; //CMD_ID
  sLen += 2; 
  sBuf[sLen] = 0; //flag
  sLen += 1;
  sBuf[sLen] = 0x00; //Status
  sLen += 1;
  SerialPortDataSending(sLen);
}
void SendGetOtaFileCommand(int32u offset, int32u length)
{
  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = 0x0005; //SUBSYS_ID
  sLen += 2;  
  *((int16u*)&sBuf[sLen]) = 0x0002; //CMD_ID
  sLen += 2; 
  *((int32u*)&sBuf[sLen]) = offset; 
  sLen += 4;
  *((int32u*)&sBuf[sLen]) = length; 
  sLen += 4; 
  SerialPortDataSending(sLen);
}
void GetOtaFileData(int8u *rBuf, int8u len)
{
  int8u i;
  int32u length;
  int8u* buf = GetOtaUpdataBuffer();
    
  otaFileDataUpdata = TRUE;
  length = *((int32u*)&rBuf[2]);
  length = length>64?64:length;
  for (i=0; i<length; i++)
  { buf[i] = rBuf[6+i];}
}

void SengledOtaEndIndicated(EmberNodeId source, const EmberAfOtaImageId* imageId)
{
  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = 0x0005; //SUBSYS_ID
  sLen += 2;  
  *((int16u*)&sBuf[sLen]) = 0x0003; //CMD_ID
  sLen += 2; 
  *((int16u*)&sBuf[sLen]) = source; 
  sLen += 2;
  *((int16u*)&sBuf[sLen]) = imageId->manufacturerId; 
  sLen += 2;
  *((int16u*)&sBuf[sLen]) = imageId->imageTypeId; 
  sLen += 2;
  *((int16u*)&sBuf[sLen]) = imageId->firmwareVersion; 
  sLen += 4;
  SerialPortDataSending(sLen);
}
static void processOtaUploaderReq(int8u *rBuf, int8u len)
{
  emberAfOtaStorageDriverWriteCallback(&rBuf[9], *(int32u *)&rBuf[1], *(int32u *)&rBuf[5]);

  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = 0x0005; //SUBSYS_ID
  sLen += 2;  
  *((int16u*)&sBuf[sLen]) = 0x0004; //CMD_ID
  sLen += 2; 
  sBuf[sLen] = 0; //flag
  sLen += 1;
  sBuf[sLen] = 0x00; //Status
  sLen += 1;
  SerialPortDataSending(sLen);
}
static void processGetOtaDataReq(int8u *rBuf, int8u len)
{
  sLen = SEAT_COMMAND;
  sBuf = GetSendBuffer();
  *((int16u*)sBuf) = 0x0005; //SUBSYS_ID
  sLen += 2;  
  *((int16u*)&sBuf[sLen]) = 0x0005; //CMD_ID
  sLen += 2; 
  sBuf[sLen] = 0; //flag
  sLen += 1;
  sBuf[sLen] = 0x00; //Status
  sLen += 1;
  emberAfOtaStorageDriverReadCallback(*(int32u *)&rBuf[0], *(int32u *)&rBuf[4], &sBuf[sLen]);
  sLen += *(int32u *)&rBuf[4];
  SerialPortDataSending(sLen);
}


//*********************************************************************
// MessageSentCallback
//*********************************************************************
enum
{
  SENGLED_SEND,
  SENGLED_SEND_PLUS,
  SENGLED_ZCL_SEND,
  SENGLED_BRODCAST_SEND,
  SENGLED_BRODCAST_PLUS_SEND
};
/** @brief Message Sent
 *
 * This function is called by the application framework from the message sent
 * handler, when it is informed by the stack regarding the message sent status.
 * All of the values passed to the emberMessageSentHandler are passed on to this
 * callback. This provides an opportunity for the application to verify that its
 * message has been sent successfully and take the appropriate action. This
 * callback should return a boolean value of TRUE or FALSE. A value of TRUE
 * indicates that the message sent notification has been handled and should not
 * be handled by the application framework.
 *
 * @param type   Ver.: always
 * @param indexOrDestination   Ver.: always
 * @param apsFrame   Ver.: always
 * @param msgLen   Ver.: always
 * @param message   Ver.: always
 * @param status   Ver.: always
 */
boolean emberAfMessageSentCallback(EmberOutgoingMessageType type,
                                   int16u indexOrDestination,
                                   EmberApsFrame* apsFrame,
                                   int16u msgLen,
                                   int8u* message,
                                   EmberStatus status)
{
  int8u responsType;
  int16u sysId = 0xffff, comId = 0xffff, information;
  
  
  if (EMBER_ZDO_PROFILE_ID == apsFrame->profileId)
  {
    if (LEAVE_REQUEST == apsFrame->clusterId) //Remove ZIGB Lamp
    { sysId = 3; comId = 131; responsType = SENGLED_ZCL_SEND;}  
    else if (BIND_REQUEST  == apsFrame->clusterId) //Binding ZIGB device
    { 
      if (status == EMBER_SUCCESS)
      {
        bindingEnable = TRUE;        
        bindingStep++;
        if (bindingStep > CLUSTER_METERING)
        { 
          bindingStep = CLUSTER_ON_OFF;
          SengledSendResponse(4, 6, status, desNodeId);
          emberEventControlSetInactive(BindingEventControl);
        }
        else
        { emberEventControlSetDelayMS(BindingEventControl, 500);}
        return FALSE;
      }
      
      sysId = 4; comId = 6; responsType = SENGLED_SEND;      
    }  
  }
  else if (HA_PROFILE_ID == apsFrame->profileId)
  {
    if (0 == (message[0]&ZCL_CLUSTER_SPECIFIC_COMMAND))
    {
      if (ZCL_CONFIGURE_REPORTING_COMMAND_ID == message[2]) //Config ZIGB device Report infomation
      { 
        if (status == EMBER_SUCCESS)
        { return FALSE;}
        sysId = 4; comId = 7; responsType = SENGLED_SEND;
      }
      else if (ZCL_READ_ATTRIBUTES_COMMAND_ID == message[2]) //Get ZIGB Device Attribute
      { 
        if (status == EMBER_SUCCESS)
        { return FALSE;}
        
        sysId = 4; comId = 8; responsType = SENGLED_SEND_PLUS;
        information = GetInformation(apsFrame->clusterId, *((int16u*)&message[3]));
      }
      else if (ZCL_WRITE_ATTRIBUTES_COMMAND_ID == message[2]) //Set ZIGB Device Attribute
      { 
        if (type == EMBER_OUTGOING_BROADCAST)
        { sysId = 4; comId = 11; responsType = SENGLED_BRODCAST_PLUS_SEND;}
        else
        { sysId = 4; comId = 10; responsType = SENGLED_SEND_PLUS;}
        if ((apsFrame->clusterId==ZCL_LEVEL_CONTROL_CLUSTER_ID) && (*((int16u*)&message[3])==ZCL_ON_LEVEL_ATTRIBUTE_ID))
        { information = INFORMATION_ONLEVEL;}
      }
    }    
    else if (ZCL_IDENTIFY_CLUSTER_ID == apsFrame->clusterId) //Identify ZIGB Lamp
    { sysId = 1; comId = 125; responsType = SENGLED_ZCL_SEND;}
    else if (ZCL_COLOR_CONTROL_CLUSTER_ID == apsFrame->clusterId) //Set ZIGB Lamp Color Temprature
    { 
      if (type == EMBER_OUTGOING_BROADCAST)
      { sysId = 1; comId = 140; responsType = SENGLED_BRODCAST_SEND;}
      else if (type == EMBER_OUTGOING_MULTICAST)
      { sysId = 1; comId = 143; responsType = SENGLED_BRODCAST_SEND;}
      else
      { sysId = 1; comId = 126; responsType = SENGLED_ZCL_SEND;}
    }
    else if (ZCL_LEVEL_CONTROL_CLUSTER_ID == apsFrame->clusterId) //Set ZIGB Lamp Brightness
    { 
      if (type == EMBER_OUTGOING_BROADCAST)
      { sysId = 1; comId = 139; responsType = SENGLED_BRODCAST_SEND;}
      else if (type == EMBER_OUTGOING_MULTICAST)
      { sysId = 1; comId = 142; responsType = SENGLED_BRODCAST_SEND;}
      else
      { sysId = 1; comId = 127; responsType = SENGLED_ZCL_SEND;}
    }
    else if (ZCL_ON_OFF_CLUSTER_ID == apsFrame->clusterId) //ZIGB Lamp On Off
    { 
      if (type == EMBER_OUTGOING_BROADCAST)
      { sysId = 1; comId = 138; responsType = SENGLED_BRODCAST_SEND;}
      else if (type == EMBER_OUTGOING_MULTICAST)
      { sysId = 1; comId = 141; responsType = SENGLED_BRODCAST_SEND;}
      else
      { sysId = 1; comId = 128; responsType = SENGLED_ZCL_SEND;}
    }
    else if (ZCL_BASIC_CLUSTER_ID == apsFrame->clusterId) //Reset ZIGB device to the factory
    { sysId = 4; comId = 5; responsType = SENGLED_SEND;}
    else if (ZCL_GROUPS_CLUSTER_ID == apsFrame->clusterId) //Reset ZIGB device to the factory
    { 
      if (message[2] == ZCL_ADD_GROUP_COMMAND_ID)
      { sysId = 1; comId = 144; responsType = SENGLED_ZCL_SEND;}
      else if (message[2] == ZCL_REMOVE_GROUP_COMMAND_ID)
      { sysId = 1; comId = 145; responsType = SENGLED_ZCL_SEND;}  
    }
  }  

  if (SENGLED_SEND == responsType) 
  { SengledSendResponse(sysId, comId, status, indexOrDestination);}
  else if (SENGLED_SEND_PLUS == responsType)
  { SengledSendPlusResponse(sysId, comId, status, indexOrDestination, information);}
  else if (SENGLED_ZCL_SEND == responsType)
  { SengledSendZclResponse(sysId, comId, status, indexOrDestination, GetCurrentMacPtr);}
  else if (SENGLED_BRODCAST_SEND == responsType)
  { SengledSendBoradcastResponse(sysId, comId, status);}
  else if (SENGLED_BRODCAST_PLUS_SEND == responsType)
  { SengledSendBoradcastPlusResponse(sysId, comId, status, information);}
  
  return FALSE;
}
//*********************************************************************
// Data Parse
//*********************************************************************
void DataParse(int8u *rBuf, int8u len)
{
  int16u cmd1, cmd2;    

  cmd1 = *((int16u*)rBuf);
  rBuf += 2;
  cmd2 = *((int16u*)rBuf);
  rBuf += 2;

  if (cmd1 == 1)
  {
    if (cmd2 == 125)
    { processIdentifyReq(rBuf, len);}
    else if (cmd2 == 126)
    { processColorTempratureReq(rBuf, len);}
    else if (cmd2 == 127)
    { processLevelReq(rBuf, len);}
    else if (cmd2 == 128)
    { processOnoffReq(rBuf, len);}
    else if (cmd2 == 138)
    { processAllOnoffReq(rBuf, len);}
    else if (cmd2 == 139)
    { processAllLevelReq(rBuf, len);}
    else if (cmd2 == 140)
    { processAllColorTempratureReq(rBuf, len);}
    else if (cmd2 == 141)
    { processGroupOnoffReq(rBuf, len);}
    else if (cmd2 == 142)
    { processGroupLevelReq(rBuf, len);}
    else if (cmd2 == 143)
    { processGroupColorTempratureReq(rBuf, len);}
    else if (cmd2 == 144)
    { processGroupAddReq(rBuf, len);}
    else if (cmd2 == 145)
    { processGroupRemoveReq(rBuf, len);}
      
  }
  else if (cmd1 == 3)
  {
    if (cmd2 == 130)
    { ProcessCreateNetworkReq(rBuf, len);}
    else if (cmd2 == 131)
    { ProcessRemoveDeviceReq(rBuf, len);}
  }
  else if (cmd1 == 4)
  {
    if (cmd2 == 1)
    { ProcessDeleteNetworkReq(rBuf, len);}
    else if (cmd2 == 2)
    { ProcessChangeChannelReq(rBuf, len);}
    else if (cmd2 == 3)
    { ProcessSetPermitJoiningTimeReq(rBuf, len);}
    else if (cmd2 == 5)
    { ProcessResetToTheFactoryReq(rBuf, len);}
    else if (cmd2 == 6)
    { ProcessBindingReq(rBuf, len);}
    else if (cmd2 == 7)
    { ProcessConfigReportReq(rBuf, len);}
    else if (cmd2 == 8)
    { processGetDeviceAttributeReq(rBuf, len);}
    else if (cmd2 == 10)
    { processSetDeviceAttributeReq(rBuf, len);}
    else if (cmd2 == 11)
    { processAllSetDeviceAttributeReq(rBuf, len);}
  }
  else if (cmd1 == 5)
  {
    if (cmd2 == 1)
    { NoticeZIGBDeviceNewOTAFile(rBuf, len);}
    else if (cmd2 == 2)
    { GetOtaFileData(rBuf, len);}
    else if (cmd2 == 4)
    { processOtaUploaderReq(rBuf, len);}
    else if (cmd2 == 5)
    { processGetOtaDataReq(rBuf, len);}
  }
}



// eof ciConsole.c


