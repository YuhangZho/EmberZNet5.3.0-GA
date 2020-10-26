#include "app/framework/include/af.h"
#include "app/framework/util/attribute-storage.h"

#ifdef EMBER_AF_PLUGIN_SCENES
  #include "app/framework/plugin/scenes/scenes.h"
#endif //EMBER_AF_PLUGIN_SCENES

#ifdef EMBER_AF_PLUGIN_ON_OFF
  #include "app/framework/plugin/on-off/on-off.h"
#endif //EMBER_AF_PLUGIN_ON_OFF

#ifdef EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER
  #include "app/framework/plugin/zll-level-control-server/zll-level-control-server.h"
#endif //EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER


/******************************************************************************
                    Includes section
******************************************************************************/
#include "Z02_Gateway_App.h"
#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "app/util/zigbee-framework/zigbee-device-common.h"
#include <string.h>     // ""


enum {
  CMD1_CONTROL_TYPE = 1,
  CMD1_NETWORK_TYPE = 3,
  CMD1_ZIGBEE_TYPE  = 4,
  CMD1_OTA_TYPE     = 5  
};

enum {
  CMD2_IDENTIFY_TYEP       = 125,
  CMD2_COLOR_TEMP_TYEP     = 126,  
  CMD2_LEVEL_TYEP          = 127, 
  CMD2_ONOFF_TYEP          = 128, 
  CMD2_RGBCOLOR_TYPE       = 129, 
  CMD2_STEP_WITH_ON_OFF_TYPE = 0x82, // 130
  // !!!!!! ALLERT!!!!!!!
  // FOR DataParse function
  CMD2_NEW_COLOR_TEMP_TYEP = 145,
  CMD2_MOBILE_SCENE_TYPE   = 0xA0, // zc to wifi 
  
};

enum {
  CMD2_CREATE_NETWORK_TYPE = 130,
  CMD2_REMOVE_DEVICE_TYPE  = 131  
};

enum {
  CMD2_DELETE_NETWORK_TYPE    = 1,
  CMD2_CHANGE_CHANNEL_TYPE    = 2,
  CMD2_SET_PERMIT_TIME_TYPE   = 3,
  CMD2_NEW_DEVICE_REPORT_TYPE = 4,
  CMD2_RESET_TO_FACTORY_TYPE  = 5,
  CMD2_BINDING_TYPE           = 6,
  CMD2_CONFIG_REPORT_TYPE     = 7,
  CMD2_GET_ATTRIBUTE_TYPE     = 8,
  CMD2_REPORTING_TYPE         = 9,
  CMD2_SET_ATTRIBUTE_TYPE     = 10,
  CMD2_NEW_BINDING_TYPE       = 13
};

enum{
  CMD2_NEW_OTA_FILE_TYPE     = 1,
  CMD2_OTA_FINISH_TYPE       = 3, 
  CMD2_OTA_UPLOADER_TYPE     = 4,  
  CMD2_GET_OTA_DATA_TYPE     = 5,  
  CMD2_SET_QUERY_POLICY_TYPE = 8,  
  CMD2_SET_MIN_BLOCK_TYPE    = 9,
  CMD2_IMAGE_NOTIFY_TYPE     = 10 
};

#define HUB_FIRMWARE_VERSION (0x00000103ul)
#define HUB_HARDWARE_VERSION_1 0x01 
#define HUB_HARDWARE_VERSION_2 0x02 


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

int8u ColorTempratureToPercentage(int16u ct);

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

//#define GetCurrentMacPtr  currentMac  

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
	if (desNetAddress == EMBER_BROADCAST_ADDRESS
		|| desNetAddress == EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS
		|| desNetAddress == EMBER_SLEEPY_BROADCAST_ADDRESS) 
	{
		emAfApsFrameEndpointSetup(1, EMBER_BROADCAST_ENDPOINT); 
		return emberAfSendBroadcast(desNetAddress,
									&globalApsFrame,
									appZclBufferLen,
									appZclBuffer);
	}
	else
	{
		emAfApsFrameEndpointSetup(1, 1); 
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
static EmberStatus SengledBroadcastSend(int16u desNetAddress)
{
  emAfApsFrameEndpointSetup(1, EMBER_BROADCAST_ENDPOINT);  
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

void sengled_cmd_not_support_rsp(int16u desNetAddress)
{
	sLen = SEAT_COMMAND;
	sBuf = GetSendBuffer();
	*((int16u*)sBuf) = 0xffff;
	sLen += 2;	
	*((int16u*)&sBuf[sLen]) = 0xffff;
	sLen += 2;
	sBuf[sLen] = 0xff;
	sLen += 1;
	sBuf[sLen] = 0xff;
	sLen += 1;
	*((int16u*)&sBuf[sLen]) = desNetAddress; //nwkAddr
	sLen += 2;
	SerialPortDataSending(sLen);

}
//*********************************************************************
// Sengled Send Response
//*********************************************************************
void SengledSendResponse(int16u sysId, int16u comId, EmberStatus status, 
								int16u desNetAddress,
								int16u attributeId)
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
void SengledSendPlusResponse(int16u sysId, int16u comId, EmberStatus status, 
									int16u desNetAddress, 
									int16u attributeId)
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

static void SengledSendZclResponse(int16u sysId, int16u comId, EmberStatus status, 
										int16u desNetAddress, 
										int8u* mac)
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
void SengledSendBroadcastResponse(int16u sysId, int16u comId, EmberStatus status,
											int16u desNetAddress, 
											int16u attributeId)

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
void SengledSendBroadcastPlusResponse(int16u sysId, int16u comId, EmberStatus status, 
												int16u desNetAddress, 
												int16u attributeId)
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
	EmberPanId panId;
	panId = emberGetPanId();
	if (panId == 0xffff)
	{
		return;
	}
	
	sLen = SEAT_COMMAND;
	sBuf = GetSendBuffer();
	 *((int16u*)sBuf) = CMD1_NETWORK_TYPE;  //SUBSYS_ID
	sLen += 2;  
	*((int16u*)&sBuf[sLen]) = CMD2_CREATE_NETWORK_TYPE; //CMD_ID
	sLen += 2;   
	sBuf[sLen] = (status==EMBER_SUCCESS)?0:1; //flag
	sLen += 1;
	sBuf[sLen] = status; //Status
	sLen += 1;
	sBuf[sLen] = emberGetRadioChannel(); //Channel ID
	sLen += 1;
	*((int16u*)&sBuf[sLen]) = panId; //Pan ID
	sLen += 2;
  
	SerialPortDataSending(sLen);
}

/** @brief Complete
 *
 * This callback notifies the user that the network creation process has
 * completed successfully.
 *
 * @param network The network that the network creator plugin successfully
 * formed. Ver.: always
 * @param usedSecondaryChannels Whether or not the network creator wants to
 * form a network on the secondary channels Ver.: always
 */
void emberAfPluginNetworkCreatorCompleteCallback(const EmberNetworkParameters *network,
                                                 boolean usedSecondaryChannels)
{	
	sLen = SEAT_COMMAND;
	sBuf = GetSendBuffer();
	 *((int16u*)sBuf) = CMD1_NETWORK_TYPE;  //SUBSYS_ID
	sLen += 2;  
	*((int16u*)&sBuf[sLen]) = CMD2_CREATE_NETWORK_TYPE; //CMD_ID
	sLen += 2;   
	sBuf[sLen] = 0; //flag
	sLen += 1;
	sBuf[sLen] = 0x00; //Status
	sLen += 1;
	sBuf[sLen] = network->radioChannel; //Channel ID
	sLen += 1;
	*((int16u*)&sBuf[sLen]) = network->panId; //Pan ID
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

#ifdef Z3_SUPPORT
	status = emberAfPluginNetworkCreatorStart(1,channelMask); // do form centralized network
#else
	if (status == EMBER_JOINED_NETWORK)
	{
		emberAfPluginNetworkFindFinishedCallback(EMBER_INVALID_CALL);
		return;
	}

	emberScanForUnusedPanId(channelMask,
							EMBER_AF_PLUGIN_NETWORK_FIND_DURATION);  
#endif
 
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
	*((int16u*)sBuf) = CMD1_ZIGBEE_TYPE; //SUBSYS_ID
	sLen += 2;  
	*((int16u*)&sBuf[sLen]) = CMD2_DELETE_NETWORK_TYPE; //CMD_ID
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
	*((int16u*)sBuf) = CMD1_ZIGBEE_TYPE; //SUBSYS_ID
	sLen += 2;  
	*((int16u*)&sBuf[sLen]) = CMD2_CHANGE_CHANNEL_TYPE; //CMD_ID
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
#ifdef Z3_SUPPORT
	EmberStatus status = emberAfPluginNetworkCreatorSecurityOpenNetwork(rBuf[0]);
	//EmberStatus status = emberAfPluginNetworkCreatorSecurityOpenNetwork();
#else
	EmberStatus status = emAfPermitJoin(rBuf[0], TRUE); // broadcast  
#endif

	sLen = SEAT_COMMAND;
	sBuf = GetSendBuffer();
	*((int16u*)sBuf) = CMD1_ZIGBEE_TYPE; //SUBSYS_ID
	sLen += 2;  
	*((int16u*)&sBuf[sLen]) = CMD2_SET_PERMIT_TIME_TYPE; //CMD_ID
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
  *((int16u*)sBuf) = CMD1_ZIGBEE_TYPE; //SUBSYS_ID
  sLen += 2;  
  *((int16u*)&sBuf[sLen]) = CMD2_NEW_DEVICE_REPORT_TYPE; //CMD_ID
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
  { SengledSendResponse(CMD1_ZIGBEE_TYPE, CMD2_RESET_TO_FACTORY_TYPE, status, netAddress, 0xffff);}
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
	{ 
		SengledSendZclResponse(CMD1_NETWORK_TYPE, CMD2_REMOVE_DEVICE_TYPE, status, nodeId, rBuf);
	}
}
//*********************************************************************
// binding
//*********************************************************************
static EmberNodeId desNodeId;
static EmberEUI64  srcNodeEUI64;
static int8u       bindingStep;
static boolean     bindingEnable;
EmberEventControl  BindingEventControl;

#ifdef HEIMAN_SUPPORT
static const int16u bindingClusterTable[] = {
								ZCL_ON_OFF_CLUSTER_ID,
								ZCL_LEVEL_CONTROL_CLUSTER_ID,
								ZCL_COLOR_CONTROL_CLUSTER_ID,
								ZCL_SIMPLE_METERING_CLUSTER_ID,
								0,
								0,
								0,
								0,
								0,
								ZCL_POWER_CONFIG_CLUSTER_ID,    																
};

#else
static const int16u bindingClusterTable[] = {ZCL_ON_OFF_CLUSTER_ID,ZCL_LEVEL_CONTROL_CLUSTER_ID,ZCL_COLOR_CONTROL_CLUSTER_ID,ZCL_SIMPLE_METERING_CLUSTER_ID};
#endif

enum {
	CLUSTER_ON_OFF,
	CLUSTER_LEVEL_CONTROL,
	CLUSTER_COLOR_CONTROL,
	CLUSTER_METERING,
	CLUSTER_NULL,
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
		{
			SengledSendResponse(CMD1_ZIGBEE_TYPE, CMD2_BINDING_TYPE, status, desNodeId, 0xffff);
		}
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
	ATTRIBUTE_CONSUMPTION,
	ATTRIBUTE_RSSI,
	ATTRIBUTE_LQI,
	ATTRIBUTE_AVERAGE_RETRY,
	ATTRIBUTE_ONLEVEL,

#ifdef HEIMAN_SUPPORT
	ATTRIBUTE_BATTERY_PERCENTAGE_REMAINING = 0x0009,


	// IAS zone tpye
	// 0x0000 Starndard CIE
	// 0x000d Motion Sensor
	// 0x0015 Contack switch
	// 0x0028 Fire Sensor
	// 0x002a Water Sesnsor
	// 0x002b Carbon Monoxide Sensor
	// 0x002c Personal Emergency Device
	// 0x002d Vibration/Movement Sensor
	// 0x010f Remote Control
	// ...	  ...
	ATTRIBUTE_IAS_ZONE_TYPE,                     
	ATTRIBUTE_IAS_ZONE_STATUS,                       
#endif  

#ifdef RGB_COLOR_SUPPORT
	ATTRIBUTE_COLOR_CURRENT_X,
	ATTRIBUTE_COLOR_CURRENT_Y,
#endif

	ATTRIBUTE_IDENTIFY_TIME,  // 0x000E
	
	ATTRIBUTE_MANUFACTURE = 0x8000,
	ATTRIBUTE_MODEL,
	ATTRIBUTE_FIRMWARE_VERSION,
	ATTRIBUTE_HARDWARE_VERSION,
	ATTRIBUTE_NETWORK_STATE,
	ATTRIBUTE_CHANNEL,

#ifdef DATA_POLL_SUPPORT
	ATTRIBUTE_DATA_POLL = 0xfffe,
#endif

#ifdef HEIMAN_SUPPORT
	ATTRIBUTE_RESERVERED = 0xffff,
#endif

};

enum {
	COOR_MANUFACTURE = 0x0000,
	COOR_MODEL,
	COOR_FIRMWARE_VERSION,
	COOR_HARDWARE_VERSION,
	COOR_NETWORK_STATE,
	COOR_CHANNEL,
};

void GetConfigReportCADId(int16u attributeId, 
                      int16u* const cluster, int16u* const attribute, int8u* const dataType)
{
	if (attributeId == ATTRIBUTE_ON_OFF)
	{
		*cluster = ZCL_ON_OFF_CLUSTER_ID;
		*attribute = ZCL_ON_OFF_ATTRIBUTE_ID;
		*dataType = ZCL_BOOLEAN_ATTRIBUTE_TYPE;
	}
	else if (attributeId == ATTRIBUTE_LEVEL)
	{
		*cluster = ZCL_LEVEL_CONTROL_CLUSTER_ID;
		*attribute = ZCL_CURRENT_LEVEL_ATTRIBUTE_ID;
		*dataType = ZCL_INT8U_ATTRIBUTE_TYPE;
	}
	else if (attributeId == ATTRIBUTE_COLOR_TEMPRATURE)
	{ 
		*cluster = ZCL_COLOR_CONTROL_CLUSTER_ID;
		*attribute = ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID;
		*dataType = ZCL_INT16U_ATTRIBUTE_TYPE;
	}
	else if (attributeId == ATTRIBUTE_POWER)
	{ 
		*cluster = ZCL_SIMPLE_METERING_CLUSTER_ID;
		*attribute = ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID;
		*dataType = ZCL_INT24S_ATTRIBUTE_TYPE;
	}
	else if (attributeId == ATTRIBUTE_CONSUMPTION)
	{ 
		*cluster = ZCL_SIMPLE_METERING_CLUSTER_ID;
		*attribute = ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID;
		*dataType = ZCL_INT48U_ATTRIBUTE_TYPE;
	}
#ifdef HEIMAN_SUPPORT
	else if (attributeId == ATTRIBUTE_BATTERY_PERCENTAGE_REMAINING)
	{
		*cluster = ZCL_POWER_CONFIG_CLUSTER_ID;
		*attribute = ZCL_BATTERY_PERCENTAGE_REMAINING_ATTRIBUTE_ID;
		*dataType = ZCL_INT8U_ATTRIBUTE_TYPE;
	}
#endif

#ifdef RGB_COLOR_SUPPORT
	else if (attributeId == ATTRIBUTE_COLOR_CURRENT_X)
	{
		*cluster = ZCL_COLOR_CONTROL_CLUSTER_ID;
		*attribute = ZCL_COLOR_CONTROL_CURRENT_X_ATTRIBUTE_ID;
		*dataType = ZCL_INT16U_ATTRIBUTE_TYPE;
	}
	else if (attributeId == ATTRIBUTE_COLOR_CURRENT_Y)
	{
		*cluster = ZCL_COLOR_CONTROL_CLUSTER_ID;
		*attribute = ZCL_COLOR_CONTROL_CURRENT_Y_ATTRIBUTE_ID;
		*dataType = ZCL_INT16U_ATTRIBUTE_TYPE;
	}
#endif
	else if (attributeId == ATTRIBUTE_IDENTIFY_TIME)
	{
		*cluster = ZCL_IDENTIFY_CLUSTER_ID;
		*attribute = ZCL_IDENTIFY_TIME_ATTRIBUTE_ID;
		*dataType = ZCL_INT16U_ATTRIBUTE_TYPE;
	}
}
int16u GetConfigReportAttributeId(int16u cluster, int16u attribute)
{
	if ((ZCL_ON_OFF_CLUSTER_ID == cluster) 
		&& (ZCL_ON_OFF_ATTRIBUTE_ID == attribute))
	{ return ATTRIBUTE_ON_OFF;}
	else if ((ZCL_LEVEL_CONTROL_CLUSTER_ID == cluster) 
		&& (ZCL_CURRENT_LEVEL_ATTRIBUTE_ID == attribute))
	{ return ATTRIBUTE_LEVEL;}    
	else if ((ZCL_COLOR_CONTROL_CLUSTER_ID == cluster) 
		&& (ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID == attribute))
	{ return ATTRIBUTE_COLOR_TEMPRATURE;}
	else if ((ZCL_SIMPLE_METERING_CLUSTER_ID == cluster) 
		&& (ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID == attribute))
	{ return ATTRIBUTE_POWER;}
	else if ((ZCL_SIMPLE_METERING_CLUSTER_ID == cluster) 
		&& (ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID == attribute))
	{ return ATTRIBUTE_CONSUMPTION;}
#ifdef HEIMAN_SUPPORT
	else if ((cluster == ZCL_POWER_CONFIG_CLUSTER_ID)
		&& (attribute == ZCL_BATTERY_PERCENTAGE_REMAINING_ATTRIBUTE_ID))
	{
		return ATTRIBUTE_BATTERY_PERCENTAGE_REMAINING;
	}
#endif

#ifdef RGB_COLOR_SUPPORT
	else if ((cluster == ZCL_COLOR_CONTROL_CLUSTER_ID)
			&& (attribute == ZCL_COLOR_CONTROL_CURRENT_X_ATTRIBUTE_ID))
	{
		return ATTRIBUTE_COLOR_CURRENT_X;
	}
	else if ((cluster == ZCL_COLOR_CONTROL_CLUSTER_ID)
			&& (attribute == ZCL_COLOR_CONTROL_CURRENT_Y_ATTRIBUTE_ID))
	{
		return ATTRIBUTE_COLOR_CURRENT_Y;
	}
#endif
	else if ((cluster == ZCL_IDENTIFY_CLUSTER_ID)
		&& (attribute == ZCL_IDENTIFY_TIME_ATTRIBUTE_ID))
	{
		return ATTRIBUTE_IDENTIFY_TIME;
	}

}
/*
  Config Report Response
*/
boolean emberAfConfigureReportingResponseCallback(EmberAfClusterId clusterId, 
                                                  int8u *buffer, 
                                                  int16u bufLen)
{
	int8u status = emberAfGetInt8u(buffer, 0, bufLen);
	int16u attributeId = emberAfGetInt16u(buffer, 2, bufLen);
  
	if (bufLen < 4)
	{
		status = 0;
	}
  
	SengledSendPlusResponse(CMD1_ZIGBEE_TYPE, 
                          CMD2_CONFIG_REPORT_TYPE, 
                          status, 
                          emberAfCurrentCommand()->source, 
                          GetConfigReportAttributeId(clusterId, attributeId));
  
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

	GetConfigReportCADId(attrId, &clusterId, &attributeId, &dataType);

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
	if (emberAfGetAttributeAnalogOrDiscreteType(dataType) == EMBER_AF_DATA_TYPE_ANALOG) 
	{
		int8u dataSize = emberAfGetDataSize(dataType);    
		CopyZclMemory(&rBuf[8], dataSize);
	}

	int16u netAddress = *((int16u*)rBuf);
	EmberStatus status = SengledSend(netAddress);

	if (EMBER_SUCCESS != status)
	{ 
		SengledSendPlusResponse(CMD1_ZIGBEE_TYPE, CMD2_CONFIG_REPORT_TYPE, 
								status, netAddress, attrId);
	}
}
//*********************************************************************
// Get Device Attribute
//*********************************************************************
#define COORDINATOR_NETWORK_ID 0x0000

int16u GetInformation(int16u cluster, int16u attribute)
{
  int16u information;
  
  if (cluster == ZCL_BASIC_CLUSTER_ID)  //attributeId
  {
    if (attribute == ZCL_MANUFACTURER_NAME_ATTRIBUTE_ID)
    { information = ATTRIBUTE_MANUFACTURE;}
    else if (attribute == ZCL_MODEL_IDENTIFIER_ATTRIBUTE_ID)
    { information = ATTRIBUTE_MODEL;}
    else if (attribute == ZCL_HW_VERSION_ATTRIBUTE_ID)
    { information = ATTRIBUTE_HARDWARE_VERSION;}  
  }
  else if (cluster == ZCL_OTA_BOOTLOAD_CLUSTER_ID)
  {
    if (attribute == ZCL_CURRENT_FILE_VERSION_ATTRIBUTE_ID)
    { information = ATTRIBUTE_FIRMWARE_VERSION;}
  }
  else if (cluster == ZCL_ON_OFF_CLUSTER_ID)
  {
    if (attribute == ZCL_ON_OFF_ATTRIBUTE_ID)
    { information = ATTRIBUTE_ON_OFF;}
  }
  else if (cluster == ZCL_LEVEL_CONTROL_CLUSTER_ID)
  {
    if (attribute == ZCL_CURRENT_LEVEL_ATTRIBUTE_ID)
    { information = ATTRIBUTE_LEVEL;}
  }
  else if (cluster == ZCL_COLOR_CONTROL_CLUSTER_ID)
  {
    if (attribute == ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID)
    { 
		information = ATTRIBUTE_COLOR_TEMPRATURE;
	}
#ifdef RGB_COLOR_SUPPORT
	else if (attribute == ZCL_COLOR_CONTROL_CURRENT_X_ATTRIBUTE_ID)
	{
		information = ATTRIBUTE_COLOR_CURRENT_X;
	}
	else if (attribute == ZCL_COLOR_CONTROL_CURRENT_Y_ATTRIBUTE_ID)
	{
		information = ATTRIBUTE_COLOR_CURRENT_Y;
	}
#endif
  }
  else if (cluster == ZCL_SIMPLE_METERING_CLUSTER_ID)
  {
    if (attribute == ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID)
    { information = ATTRIBUTE_POWER;}
    else if (attribute == ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID)
    { information = ATTRIBUTE_CONSUMPTION;}
  }
  else if (cluster == ZCL_DIAGNOSTICS_CLUSTER_ID)
  {
    if (attribute == ZCL_LAST_MESSAGE_RSSI_ATTRIBUTE_ID)
    { information = ATTRIBUTE_RSSI;}
    else if (attribute == ZCL_LAST_MESSAGE_LQI_ATTRIBUTE_ID)
    { information = ATTRIBUTE_LQI;}
    else if (attribute == ZCL_AVERAGE_MAC_RETRY_PER_APS_MSG_SENT_ATTRIBUTE_ID)
    { information = ATTRIBUTE_AVERAGE_RETRY;}
  }
#ifdef HEIMAN_SUPPORT
	else if (cluster == ZCL_POWER_CONFIG_CLUSTER_ID)
	{
		if (attribute == ZCL_BATTERY_PERCENTAGE_REMAINING_ATTRIBUTE_ID)
		{
			information = ATTRIBUTE_BATTERY_PERCENTAGE_REMAINING;
		}
	}
	else if (cluster == ZCL_IAS_ZONE_CLUSTER_ID)
	{
		if (attribute == ZCL_ZONE_TYPE_ATTRIBUTE_ID)
		{
			information = ATTRIBUTE_IAS_ZONE_TYPE;
		}
		else if(attribute == ZCL_ZONE_STATUS_ATTRIBUTE_ID)
		{
			information = ATTRIBUTE_IAS_ZONE_STATUS;
		}
	}
#endif
	else if (cluster == ZCL_IDENTIFY_CLUSTER_ID)
	{
		if (attribute == ZCL_IDENTIFY_TIME_ATTRIBUTE_ID)
		{
			information = ATTRIBUTE_IDENTIFY_TIME;
		}
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
	*((int16u*)sBuf) = CMD1_ZIGBEE_TYPE; sLen += 2;  //SUBSYS_ID
	*((int16u*)&sBuf[sLen]) = CMD2_GET_ATTRIBUTE_TYPE; sLen += 2; //CMD_ID
	attributeId = (EmberAfAttributeId)emberAfGetInt16u(buffer, bufIndex, bufLen);
	bufIndex += 2;
	status = (EmberAfStatus)emberAfGetInt8u(buffer, bufIndex, bufLen);
	bufIndex++;
	sBuf[sLen++] = (status==EMBER_SUCCESS)?0:1; //flag

	sBuf[sLen++] = status;      //state  
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
			{ 
				sBuf[sLen++] = buffer[bufIndex+i];
			}
		}
    
	}

	emberGetLastHopLqi(&sBuf[sLen++]);
	emberGetLastHopRssi(&sBuf[sLen++]);
  
	SerialPortDataSending(sLen);
  
	emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
	return TRUE;
}

void GetClusterAndAttribute(int16u information, int16u *cluster, int16u *attribute)
{
  if (information == ATTRIBUTE_MANUFACTURE)
  {
    *cluster = ZCL_BASIC_CLUSTER_ID;
    *attribute = ZCL_MANUFACTURER_NAME_ATTRIBUTE_ID;
  }
  else if (information == ATTRIBUTE_MODEL)
  {
    *cluster = ZCL_BASIC_CLUSTER_ID;
    *attribute = ZCL_MODEL_IDENTIFIER_ATTRIBUTE_ID;
  }
  else if (information == ATTRIBUTE_FIRMWARE_VERSION)
  {
    *cluster = ZCL_OTA_BOOTLOAD_CLUSTER_ID;
    *attribute = ZCL_CURRENT_FILE_VERSION_ATTRIBUTE_ID;
  }
  else if (information == ATTRIBUTE_HARDWARE_VERSION)
  {
    *cluster = ZCL_BASIC_CLUSTER_ID;
    *attribute = ZCL_HW_VERSION_ATTRIBUTE_ID;
  }
  else if (information == ATTRIBUTE_ON_OFF)
  {
    *cluster = ZCL_ON_OFF_CLUSTER_ID;
    *attribute = ZCL_ON_OFF_ATTRIBUTE_ID;
  }
  else if (information == ATTRIBUTE_LEVEL)
  {
    *cluster = ZCL_LEVEL_CONTROL_CLUSTER_ID;
    *attribute = ZCL_CURRENT_LEVEL_ATTRIBUTE_ID;
  }
  else if (information == ATTRIBUTE_COLOR_TEMPRATURE)
  {
    *cluster = ZCL_COLOR_CONTROL_CLUSTER_ID;
    *attribute = ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID;
  }
  else if (information == ATTRIBUTE_POWER)
  {
    *cluster = ZCL_SIMPLE_METERING_CLUSTER_ID;
    *attribute = ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID;
  }
  else if (information == ATTRIBUTE_CONSUMPTION)
  {
    *cluster = ZCL_SIMPLE_METERING_CLUSTER_ID;
    *attribute = ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID;
  }
  else if (information == ATTRIBUTE_RSSI)
  {
    *cluster = ZCL_DIAGNOSTICS_CLUSTER_ID;
    *attribute = ZCL_LAST_MESSAGE_RSSI_ATTRIBUTE_ID;
  }
  else if (information == ATTRIBUTE_RSSI)
  {
    *cluster = ZCL_DIAGNOSTICS_CLUSTER_ID;
    *attribute = ZCL_LAST_MESSAGE_RSSI_ATTRIBUTE_ID;
  }
  else if (information == ATTRIBUTE_LQI)
  {
    *cluster = ZCL_DIAGNOSTICS_CLUSTER_ID;
    *attribute = ZCL_LAST_MESSAGE_LQI_ATTRIBUTE_ID;
  }
  else if (information == ATTRIBUTE_AVERAGE_RETRY)
  {
    *cluster = ZCL_DIAGNOSTICS_CLUSTER_ID;
    *attribute = ZCL_AVERAGE_MAC_RETRY_PER_APS_MSG_SENT_ATTRIBUTE_ID;
  }
#ifdef HEIMAN_SUPPORT
	else if(information == ATTRIBUTE_IAS_ZONE_TYPE)
	{
		*cluster = ZCL_IAS_ZONE_CLUSTER_ID;
		*attribute = ZCL_ZONE_TYPE_ATTRIBUTE_ID;
	}
	else if(information == ATTRIBUTE_IAS_ZONE_STATUS)
	{
		*cluster = ZCL_IAS_ZONE_CLUSTER_ID;
		*attribute = ZCL_ZONE_STATUS_ATTRIBUTE_ID;
	}
	else if (information == ATTRIBUTE_BATTERY_PERCENTAGE_REMAINING)
	{
		*cluster = ZCL_POWER_CONFIG_CLUSTER_ID;
		*attribute = ZCL_BATTERY_PERCENTAGE_REMAINING_ATTRIBUTE_ID;
	}
#endif

#ifdef RGB_COLOR_SUPPORT
	else if (information == ATTRIBUTE_COLOR_CURRENT_X)
	{
		*cluster = ZCL_COLOR_CONTROL_CLUSTER_ID;
		*attribute = ZCL_COLOR_CONTROL_CURRENT_X_ATTRIBUTE_ID;
	}
	else if (information == ATTRIBUTE_COLOR_CURRENT_Y)
	{
		*cluster = ZCL_COLOR_CONTROL_CLUSTER_ID;
		*attribute = ZCL_COLOR_CONTROL_CURRENT_Y_ATTRIBUTE_ID;
	}
#endif
	else if (information == ATTRIBUTE_IDENTIFY_TIME)
	{
		*cluster = ZCL_IDENTIFY_CLUSTER_ID;
		*attribute = ZCL_IDENTIFY_TIME_ATTRIBUTE_ID;		
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
		*((int16u*)sBuf) = CMD1_ZIGBEE_TYPE; sLen += 2;  //SUBSYS_ID
		*((int16u*)&sBuf[sLen]) = CMD2_GET_ATTRIBUTE_TYPE; sLen += 2; //CMD_ID
		sBuf[sLen] = 0x00; sLen += 1;     //Flag
		sBuf[sLen] = 0x00; sLen += 1;     //state
		*((int16u*)&sBuf[sLen]) = COORDINATOR_NETWORK_ID; sLen += 2;  //desNwkAddr
		*((int16u*)&sBuf[sLen]) = information; sLen += 2; //information
		if (information == COOR_MANUFACTURE) //informationValue
		{
			int8u len = strlen(manufacturer);
			sBuf[sLen] = len; sLen += 1;
			for (i=0; i<len; i++)
			{ 
				sBuf[sLen] = manufacturer[i]; sLen += 1;
			}      
		}
		else if (information == COOR_MODEL)
		{
			int8u len = strlen(model);
			sBuf[sLen] = len; sLen += 1;
			for (i=0; i<len; i++)
			{ 
				sBuf[sLen] = model[i]; sLen += 1;
			}      
		}
		else if (information == COOR_FIRMWARE_VERSION)
		{
			*((int32u*)&sBuf[sLen]) = HUB_FIRMWARE_VERSION; sLen += 4;
		}
		else if (information == COOR_HARDWARE_VERSION)
		{ 
			halGpioConfig(PORTC_PIN(1), GPIOCFG_IN_PUD);
  
      		if (GPIO_PCIN & PC1_MASK) // Z02 ?
			{ 
				sBuf[sLen] = HUB_HARDWARE_VERSION_2;
			}
			else
			{ 
				sBuf[sLen] = HUB_HARDWARE_VERSION_1;
			}
			sLen += 1;
		}
    	else if (information == COOR_NETWORK_STATE)
    	{ 
    		sBuf[sLen] = emberAfNetworkState(); sLen += 1;
		}
    	else if (information == COOR_CHANNEL)
    	{ 
    		sBuf[sLen] = emberGetRadioChannel(); sLen += 1;
		}
    
		SerialPortDataSending(sLen);
	}
	else
	{
		int16u clusterId;
		int16u attributeId;
    
		GetClusterAndAttribute(information, &clusterId, &attributeId);
    
		if (information == ATTRIBUTE_FIRMWARE_VERSION)
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
		{ 
			SengledSendPlusResponse(CMD1_ZIGBEE_TYPE, CMD2_GET_ATTRIBUTE_TYPE, status, netAddress, information);
		}
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
	int16u identify_time = *((int16u*)&rBuf[4]) / 10; // gateway 1 means 100ms, identify time unit is second,we need transform;

	if (information == ATTRIBUTE_ONLEVEL )
	{
		clusterId = ZCL_LEVEL_CONTROL_CLUSTER_ID;
		attributeId = ZCL_ON_LEVEL_ATTRIBUTE_ID;
		type = ZCL_INT8U_ATTRIBUTE_TYPE;
	}
	else if(information == ATTRIBUTE_IDENTIFY_TIME)
	{
		clusterId = ZCL_IDENTIFY_CLUSTER_ID;
		attributeId = ZCL_IDENTIFY_TIME_ATTRIBUTE_ID;
		type = ZCL_INT16U_ATTRIBUTE_TYPE;

	}
  
	zclBufferSetup(ZCL_PROFILE_WIDE_COMMAND|ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK,
                 clusterId,
                 ZCL_WRITE_ATTRIBUTES_COMMAND_ID);
	zclBufferAddWord(attributeId);
	zclBufferAddByte(type);
	if (information == ATTRIBUTE_ONLEVEL)
	{
		zclBufferAddByte(rBuf[4]);
	}
	else if (information == ATTRIBUTE_IDENTIFY_TIME)
	{
		zclBufferAddWord(identify_time);
	}

	int16u netAddress = *((int16u*)rBuf);
	EmberStatus status = SengledSend(netAddress);

	if (EMBER_SUCCESS != status)
	{ 
		SengledSendPlusResponse(CMD1_ZIGBEE_TYPE, CMD2_SET_ATTRIBUTE_TYPE, status, netAddress, information);
	}
}

//*********************************************************************
// Report Attributes
//*********************************************************************
int8u ColorTempratureToPercentage(int16u ct)
{
	// 153 -370  6500 - 2700k
	if (ct <= COLOR_TEMPRATURE_MIN)
	{ 
		return COLOR_FULL_PERCENTAGE;
	}
	else if (COLOR_TEMPRATURE_MAX <= ct)
	{ 
		return 0;
	}
	else
	{ 
		return COLOR_FULL_PERCENTAGE - ((ct-COLOR_TEMPRATURE_MIN)*COLOR_FULL_PERCENTAGE)/COLOR_TEMPRATURE_D_VALUE;
	}
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
	*((int16u*)sBuf) = CMD1_ZIGBEE_TYPE; sLen += 2;  //SUBSYS_ID
	*((int16u*)&sBuf[sLen]) = CMD2_REPORTING_TYPE; sLen += 2; //CMD_ID
	sBuf[sLen] = 0x00; sLen += 1;     //Flag
	sBuf[sLen] = 0x00; sLen += 1;     //state  
	*((int16u*)&sBuf[sLen]) = emberAfCurrentCommand()->source; sLen += 2;  //desNwkAddr
  
	attributeId = (EmberAfAttributeId)emberAfGetInt16u(buffer, bufIndex, bufLen); //messageId,message

	bufIndex += 3; // attribute id (2 bytes) + attribute type(1 byte), the following bytes are the attribute value
	
	if (clusterId == ZCL_ON_OFF_CLUSTER_ID)
	{
		if (attributeId == ZCL_ON_OFF_ATTRIBUTE_ID)
		{ 
			*((int16u*)&sBuf[sLen]) = ATTRIBUTE_ON_OFF; sLen += 2;
			sBuf[sLen] = buffer[bufIndex];sLen += 1;
			for (i=1; i<8; i++)
			{ 
				sBuf[sLen] = 0;sLen += 1;
			}
		}  
	}
	else if (clusterId == ZCL_IDENTIFY_CLUSTER_ID)
	{
		*((int16u*)&sBuf[sLen]) = ATTRIBUTE_IDENTIFY_TIME; sLen += 2; //attributeId
		sBuf[sLen] = buffer[bufIndex];bufIndex++;sLen += 1;
		sBuf[sLen] = buffer[bufIndex];bufIndex++;sLen += 1;
		for (i=2; i<8; i++)
		{ 
			sBuf[sLen] = 0;sLen += 1;
		}	
	}
#ifdef HEIMAN_SUPPORT
	else if(clusterId == ZCL_POWER_CONFIG_CLUSTER_ID)
	{
		if (attributeId == ZCL_BATTERY_PERCENTAGE_REMAINING_ATTRIBUTE_ID)
		{ 
			*((int16u*)&sBuf[sLen]) = ATTRIBUTE_BATTERY_PERCENTAGE_REMAINING; sLen += 2;
			sBuf[sLen] = buffer[bufIndex];sLen += 1;
			for (i=1; i<8; i++)
			{ 
				sBuf[sLen] = 0;sLen += 1;
			}
		}	
	}
#endif
	else if (clusterId == ZCL_LEVEL_CONTROL_CLUSTER_ID)
	{
		if (attributeId == ZCL_CURRENT_LEVEL_ATTRIBUTE_ID)
		{ 
			*((int16u*)&sBuf[sLen]) = ATTRIBUTE_LEVEL; sLen += 2;
			sBuf[sLen] = buffer[bufIndex];sLen += 1;
			for (i=1; i<8; i++)
			{ 
				sBuf[sLen] = 0;sLen += 1;
			}
		}
	}
	else if (clusterId == ZCL_COLOR_CONTROL_CLUSTER_ID)
	{
		if (attributeId == ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID)
		{ 
			*((int16u*)&sBuf[sLen]) = ATTRIBUTE_COLOR_TEMPRATURE; sLen += 2; //attributeId
			//sBuf[sLen] = ColorTempratureToPercentage(*((int16u*)&buffer[bufIndex]));sLen += 1;
			sBuf[sLen] = buffer[bufIndex];bufIndex++;sLen += 1;
			sBuf[sLen] = buffer[bufIndex];bufIndex++;sLen += 1;
			for (i=2; i<8; i++)
			{ 
				sBuf[sLen] = 0;sLen += 1;
			}
		}
#ifdef RGB_COLOR_SUPPORT
		else if (attributeId == ZCL_COLOR_CONTROL_CURRENT_X_ATTRIBUTE_ID ||
			attributeId == ZCL_COLOR_CONTROL_CURRENT_Y_ATTRIBUTE_ID)
		{
			 
			*((int16u*)&sBuf[sLen]) = 9+attributeId; 
			sLen += 2; //attributeId
			sBuf[sLen++] = buffer[bufIndex++];
			sBuf[sLen++] = buffer[bufIndex++];
			for (i=2;i<8;i++)
			{
				sBuf[sLen++] = 0;
			}

/*			
			if (bufIndex >= bufLen)
			{
				*((int16u*)&sBuf[sLen]) = 0xffff; 
				sLen += 2; //attributeId
				sBuf[sLen++] = 0xff;
				sBuf[sLen++] = 0xff;
			}
			else
			{
				// one report with two attribute: currentX,currentY
				attributeId = (EmberAfAttributeId)emberAfGetInt16u(buffer, bufIndex, bufLen); //messageId,message
				bufIndex += 3;

				if (attributeId == ZCL_COLOR_CONTROL_CURRENT_X_ATTRIBUTE_ID)
				{
					// if attribute 0x0004 come first by accident, we need to exchange the two attribute data position
					*((int16u*)&sBuf[sLen]) = 9+attributeId+1;
					*((int16u*)&sBuf[sLen-4]) = 9+attributeId;					
					sLen += 2; //attributeId					
					sBuf[sLen] = sBuf[sLen-4];
					sLen++;
					sBuf[sLen] = sBuf[sLen-4];
					sLen++;
					sBuf[sLen++] = buffer[bufIndex++];
					sBuf[sLen++] = buffer[bufIndex++];
				}

				*((int16u*)&sBuf[sLen]) = 9+attributeId;
				sLen += 2; //attributeId
				sBuf[sLen++] = buffer[bufIndex++];
				sBuf[sLen++] = buffer[bufIndex++];
			}
*/
		}
#endif
	}
	else if (clusterId == ZCL_SIMPLE_METERING_CLUSTER_ID)
	{
		if (attributeId == ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID)
		{ 
			*((int16u*)&sBuf[sLen]) = ATTRIBUTE_CONSUMPTION; sLen += 2; //attributeId
			for (i=0; i<6; i++)
			{ 
				sBuf[sLen+i] = buffer[bufIndex+i];
			}
			for (i=6; i<8; i++)
			{ 
				sBuf[sLen+i] = 0;
			}
			sLen += 8;
    	}
		else if (attributeId == ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID)
		{ 
			*((int16u*)&sBuf[sLen]) = ATTRIBUTE_POWER; sLen += 2; //attributeId
			for (i=0; i<3; i++)
			{ 
				sBuf[sLen+i] = buffer[bufIndex+i];
			}
			for (i=3; i<8; i++)
			{
				sBuf[sLen+i] = 0;
			}
			sLen += 8;
		}
	}
#ifdef HEIMAN_SUPPORT
	else if (clusterId == ZCL_IAS_ZONE_CLUSTER_ID)
	{
		if (attributeId == ZCL_ZONE_TYPE_ATTRIBUTE_ID)
		{ 
			*((int16u*)&sBuf[sLen]) = ATTRIBUTE_IAS_ZONE_TYPE; sLen += 2; //attributeId
			sBuf[sLen] = buffer[bufIndex];bufIndex++;sLen += 1;
			sBuf[sLen] = buffer[bufIndex];bufIndex++;sLen += 1;
			for (i=2; i<8; i++)
			{ 
				sBuf[sLen] = 0;sLen += 1;
			}
		}
		else if (attributeId == ZCL_ZONE_STATUS_ATTRIBUTE_ID)
		{ 
			*((int16u*)&sBuf[sLen]) = ATTRIBUTE_IAS_ZONE_STATUS; sLen += 2; //attributeId
			sBuf[sLen] = buffer[bufIndex];bufIndex++;sLen += 1;
			sBuf[sLen] = buffer[bufIndex];bufIndex++;sLen += 1;
			for (i=2; i<8; i++)
			{ 
				sBuf[sLen] = 0;sLen += 1;
			}
		}
	}
	else
	{
		*((int16u*)&sBuf[sLen]) = ATTRIBUTE_RESERVERED; sLen += 2;
		for (i=0; i<8; i++)
		{
			sBuf[sLen] = 0xff;sLen += 1;
		}	
	}
#endif
		emberGetLastHopLqi(&sBuf[sLen]);sLen += 1;
		emberGetLastHopRssi(&sBuf[sLen]);sLen += 1;
		sBuf[sLen] = emberAfCurrentCommand()->seqNum;sLen += 1;
  
		SerialPortDataSending(sLen);
  
		emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
		return TRUE;
}

#ifdef HEIMAN_SUPPORT
void sengled_send_notification_to_gateway(EmberAfClusterId clusterId,
													EmberAfAttributeId attributeId,
													EmberNodeId nodeId,	
													int16u zone_status,
													int8u addition_info_8,
													int16u addition_info_16)
{
	int8u i;
	
	sLen = SEAT_COMMAND;
	sBuf = GetSendBuffer();
	*((int16u*)&sBuf[sLen]) = CMD1_ZIGBEE_TYPE; 
	sLen += 2;  //SUBSYS_ID
	
	*((int16u*)&sBuf[sLen]) = CMD2_REPORTING_TYPE; 
	sLen += 2; //CMD_ID
	
	sBuf[sLen] = 0x00; sLen += 1;	  //Flag
	
	sBuf[sLen] = 0x00; sLen += 1;	  //state  
	
	*((int16u*)&sBuf[sLen]) = nodeId; sLen += 2;
	
	if(clusterId == ZCL_IAS_ZONE_CLUSTER_ID && attributeId == ZCL_ZONE_STATUS_ATTRIBUTE_ID)
	{
		*((int16u*)&sBuf[sLen]) = ATTRIBUTE_IAS_ZONE_STATUS;  //attributeId
		sLen += 2; 
		
		*((int16u*)&sBuf[sLen]) = zone_status;
		sLen += 2;

		*((int16u*)&sBuf[sLen]) = addition_info_8;   // zone id
		sLen++;

		*((int16u*)&sBuf[sLen]) = addition_info_16;  // delay
		sLen += 2;
		
		for (i=5; i<8; i++)
		{ sBuf[sLen] = 0;sLen += 1;}

	}

	
	emberGetLastHopLqi(&sBuf[sLen]);sLen += 1;
	emberGetLastHopRssi(&sBuf[sLen]);sLen += 1;
	sBuf[sLen] = emberAfCurrentCommand()->seqNum;sLen += 1;
	
	SerialPortDataSending(sLen);
}
#endif
//********************************************************************
/*
  New Binding
*/
boolean emberAfPreZDOMessageReceivedCallback(EmberNodeId emberNodeId,
                                             EmberApsFrame* apsFrame,
                                             int8u* message,
                                             int16u length)
{
	if (apsFrame->clusterId == BIND_RESPONSE) 
	{
		SengledSendResponse(CMD1_ZIGBEE_TYPE, 13, message[1], emberNodeId, 0xffff);
	}
	return FALSE;
}
static void ProcessNewBindingReq(int8u *rBuf, int8u len)
{
	EmberNodeId desNodeId;
	EmberEUI64  srcNodeEUI64;

	bindingStep = CLUSTER_NULL;
  
	desNodeId = *((int16u*)rBuf);
	CharMacToHexMac(&rBuf[2], srcNodeEUI64);

	EmberStatus status = emberBindRequest(desNodeId, // who gets the bind req
                                        srcNodeEUI64, // source eui IN the binding
                                        1,
                                        bindingClusterTable[*(int16u*)(&rBuf[20])],       
                                        UNICAST_BINDING, // binding type
                                        emberGetEui64(),    // destination eui IN the binding
                                        0,               // groupId for new binding
                                        1,
                                        EMBER_AF_DEFAULT_APS_OPTIONS);

	if (status != EMBER_SUCCESS)
	{ 
		SengledSendResponse(CMD1_ZIGBEE_TYPE, CMD2_NEW_BINDING_TYPE, status, desNodeId, 0xffff);
	}
}
//*********************************************************************
// Onoff
//*********************************************************************
static void processOnoffReq(int8u *rBuf, int8u len)
{
	int8u command;

	SaveCurrentMac(rBuf); //Save Current Mac
  
	if (rBuf[18] == 0) 
	{ 
		command = ZCL_OFF_COMMAND_ID;
	}
	else if (rBuf[18] == 1) 
	{ 
		command = ZCL_ON_COMMAND_ID;
	}
	else  //Invalid data
	{ 
		return;
	}
	zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
					ZCL_ON_OFF_CLUSTER_ID, 
					command);
	int16u netAddress = *((int16u*)&rBuf[19]);
	EmberStatus status = SengledSend(netAddress);
  
	if (EMBER_SUCCESS != status)
	{ 
		SengledSendZclResponse(CMD1_CONTROL_TYPE, CMD2_ONOFF_TYEP, status, netAddress, rBuf);
	}
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
	{ 
		SengledSendZclResponse(CMD1_CONTROL_TYPE, CMD2_LEVEL_TYEP, status, netAddress, rBuf);
	}
}

static void process_step_with_on_off_req(int8u *rBuf, int8u len)
{
	SaveCurrentMac(rBuf); //Save Current Mac
  
	zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
					ZCL_LEVEL_CONTROL_CLUSTER_ID, 
					ZCL_STEP_WITH_ON_OFF_COMMAND_ID);
	zclBufferAddByte(rBuf[18]);  // step mode
	zclBufferAddByte(rBuf[19]);  // step size
	zclBufferAddWord(*(int16u *)&rBuf[20]); // transition time
	int16u netAddress = *((int16u*)&rBuf[22]);
	EmberStatus status = SengledSend(netAddress);
  
	if (EMBER_SUCCESS != status)
	{ 
		SengledSendZclResponse(CMD1_CONTROL_TYPE, CMD2_STEP_WITH_ON_OFF_TYPE, status, netAddress, rBuf);
	}	
}

//*********************************************************************
// Color Temprature
//*********************************************************************
static boolean newcolorReqFlag = FALSE;
/*
  Get Color Temprature
*/

int16u PercentageToColorTemprature(int8u ct)
{
	// 153 -370  6500 - 2700k
	if (ct == 0)
	{ 
		return COLOR_TEMPRATURE_MAX;
	}
	else if (COLOR_FULL_PERCENTAGE <= ct)
	{ 
		return COLOR_TEMPRATURE_MIN;
	}
	else
	{
		return COLOR_TEMPRATURE_MAX - (ct*COLOR_TEMPRATURE_D_VALUE)/COLOR_FULL_PERCENTAGE;
	}
}

/*
  Color Temprature
*/
static void processColorTempratureReq(int8u *rBuf, int8u len)
{
	if (rBuf[18] > COLOR_FULL_PERCENTAGE) //Invalid data
	{ 
		return;
	}

	newcolorReqFlag = FALSE;
  
	SaveCurrentMac(rBuf); //Save Current Mac
  
	zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
					ZCL_COLOR_CONTROL_CLUSTER_ID, 
					ZCL_MOVE_TO_COLOR_TEMPERATURE_COMMAND_ID);
  
	zclBufferAddWord(PercentageToColorTemprature(rBuf[18]));  
	zclBufferAddWord(*(int16u *)&rBuf[19]);
	int16u netAddress = *((int16u*)&rBuf[21]);
	EmberStatus status = SengledSend(netAddress);
  
	if (EMBER_SUCCESS != status)
	{
		SengledSendZclResponse(CMD1_CONTROL_TYPE, CMD2_COLOR_TEMP_TYEP, status, netAddress, rBuf);
	}
}

static void processNewColorTempratureReq(int8u *rBuf, int8u len)
{
	newcolorReqFlag = TRUE;
  
	SaveCurrentMac(rBuf); //Save Current Mac
  
	zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
					ZCL_COLOR_CONTROL_CLUSTER_ID, 
					ZCL_MOVE_TO_COLOR_TEMPERATURE_COMMAND_ID);
  
	zclBufferAddWord(*(int16u *)&rBuf[18]);  
	zclBufferAddWord(*(int16u *)&rBuf[20]);
	int16u netAddress = *((int16u*)&rBuf[22]);
	EmberStatus status = SengledSend(netAddress);
  
	if (EMBER_SUCCESS != status)
	{ 
		SengledSendZclResponse(CMD1_CONTROL_TYPE, CMD2_NEW_COLOR_TEMP_TYEP, status, netAddress, rBuf);
	}
}
//*********************************************************************
// Identify
//*********************************************************************
static void processIdentifyReq(int8u *rBuf, int8u len)
{
	int16u time = 0x0000;  //defined unit of 100ms;
	SaveCurrentMac(rBuf); //Save Current Mac

	zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
                 	ZCL_IDENTIFY_CLUSTER_ID, 
                 	ZCL_IDENTIFY_COMMAND_ID);

	time = *((int16u*)&rBuf[18]);
	time = time / 10;  // 1 means 100ms,  ms-> s
	zclBufferAddWord(time); 
	int16u netAddress = *((int16u*)&rBuf[20]);
	EmberStatus status = SengledSend(netAddress);
  
	if (EMBER_SUCCESS != status)
	{ 
		SengledSendZclResponse(CMD1_CONTROL_TYPE, CMD2_IDENTIFY_TYEP, status, netAddress, rBuf);
	}
}

static void processRGBColorReq(int8u *rBuf, int8u len)
{ 
	int16u netAddress;
	EmberStatus status;

	SaveCurrentMac(rBuf); //Save Current Mac
  
	zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
                 	ZCL_COLOR_CONTROL_CLUSTER_ID, 
                 	ZCL_MOVE_TO_COLOR_COMMAND_ID);
  
	zclBufferAddWord(*(int16u *)&rBuf[18]);  // current x
	zclBufferAddWord(*(int16u *)&rBuf[20]);  // current y
	zclBufferAddWord(*(int16u *)&rBuf[22]);  // transition time
	
	netAddress = (*(int16u *)&rBuf[24]);
	status = SengledSend(netAddress);
  
	if (EMBER_SUCCESS != status)
	{
		SengledSendZclResponse(CMD1_CONTROL_TYPE, CMD2_RGBCOLOR_TYPE, status, netAddress, rBuf);
	}
}


//*********************************************************************
// OTA
//*********************************************************************
enum{
	OTA_FILE_TRANSMIT_UNFINISHED = 0,
	OTA_FILE_TRANSMIT_FINISHED
};

enum{
	OTA_UPLOADER_FINISH = 0,
	OTA_NEW_OTA_FILE,
	OTA_ERREO_RETRY,
};

static int8u otaEventType;
static int8u otaState;

//#define SENGLED_OTA_TEST

#ifdef SENGLED_OTA_TEST
static boolean otaFileState = TRUE;
#else
static boolean otaFileState = FALSE;
#endif
boolean emberAfOtaStorageDriverWriteCallback(const int8u* dataToWrite,
                                             int32u offset, 
                                             int32u length);
boolean emberAfOtaStorageDriverReadCallback(int32u offset, 
                                            int32u length,
                                            int8u* returnData);
EmberAfOtaStorageStatus emberAfOtaStorageDeleteImageCallback(void);

// Event control struct declarations
EmberEventControl OtaEventControl;

int32u otaOffset = 0;

// Event function stubs
void OtaEventFunction(void) 
{
	emberEventControlSetInactive(OtaEventControl);

	if (otaEventType == OTA_UPLOADER_FINISH)
	{
		otaFileState = TRUE;
		emberAfOtaStorageInitCallback();
	}
	else if (otaEventType == OTA_NEW_OTA_FILE)
	{
		sLen = SEAT_COMMAND;
		sBuf = GetSendBuffer();
		*((int16u*)sBuf) = CMD1_OTA_TYPE; //SUBSYS_ID
		sLen += 2;  
		*((int16u*)&sBuf[sLen]) = CMD2_NEW_OTA_FILE_TYPE; //CMD_ID
		sLen += 2; 
		sBuf[sLen] = (otaState==1)?1:0; //flag
		sLen += 1;
		sBuf[sLen] = 0; //Status
		sLen += 1;
		SerialPortDataSending(sLen);
	}
	else if (otaEventType == OTA_ERREO_RETRY)
	{
		otaFileState = FALSE;
		sLen = SEAT_COMMAND;
		sBuf = GetSendBuffer();
		*((int16u*)sBuf) = CMD1_OTA_TYPE; //SUBSYS_ID
 		sLen += 2;  
		*((int16u*)&sBuf[sLen]) = CMD2_OTA_UPLOADER_TYPE; //CMD_ID
		sLen += 2; 
		sBuf[sLen] = 1; //flag
		sLen += 1;
		sBuf[sLen] = 1; //Status
		sLen += 1;
		*((int32u*)&sBuf[sLen]) = 0; //otaOffset
		sLen += 4; 
		SerialPortDataSending(sLen);
	}
}

boolean GetOtaFileState(void)
{
	return otaFileState;
}

void NoticeZIGBDeviceNewOTAFile(int8u *rBuf, int8u len)
{
	otaState = emberAfOtaStorageForceDeleteImage(); 

	otaOffset = 0;
	otaEventType = OTA_NEW_OTA_FILE;

	otaFileState = FALSE;
	emberAfOtaStorageInitCallback();

	emberEventControlSetDelayMS(OtaEventControl, 8000);
}
void SengledOtaEndIndicated(EmberNodeId source, const EmberAfOtaImageId* imageId)
{
	sLen = SEAT_COMMAND;
	sBuf = GetSendBuffer();
	*((int16u*)sBuf) = CMD1_OTA_TYPE; //SUBSYS_ID
	sLen += 2;  
	*((int16u*)&sBuf[sLen]) = CMD2_OTA_FINISH_TYPE; //CMD_ID
	sLen += 2; 
	*((int16u*)&sBuf[sLen]) = source; 
	sLen += 2;
	*((int16u*)&sBuf[sLen]) = imageId->manufacturerId; 
	sLen += 2;
	*((int16u*)&sBuf[sLen]) = imageId->imageTypeId; 
	sLen += 2;
	*((int32u*)&sBuf[sLen]) = imageId->firmwareVersion; 
	sLen += 4;
	SerialPortDataSending(sLen);
}
static void processOtaUploaderReq(int8u *rBuf, int8u len)
{
	boolean flag;  
	static int8u count = 0;
	int32u tmp = *(int32u *)&rBuf[1];

	if (otaOffset == tmp)
	{ 
		count++;
		flag = emberAfOtaStorageDriverWriteCallback(&rBuf[9], tmp, *(int32u *)&rBuf[5]);
    
		if (flag == TRUE)
		{ 
			count = 0;
			otaOffset += *(int32u *)&rBuf[5];
		}
		else if (count >= 50)
		{ 
			count = 0;
			otaOffset = 0;
			otaState = emberAfOtaStorageDeleteImageCallback();
			otaEventType = OTA_ERREO_RETRY;
			emberEventControlSetDelayMS(OtaEventControl, 8000);
			return;
		}
	}
	else
	{ 
		flag = FALSE;
	}
  
	if ((rBuf[0] == OTA_FILE_TRANSMIT_FINISHED)
		&& (flag == TRUE))
	{ 
		otaEventType = OTA_UPLOADER_FINISH;
		emberEventControlSetDelayMS(OtaEventControl, 2000);
	}

	sLen = SEAT_COMMAND;
	sBuf = GetSendBuffer();
	*((int16u*)sBuf) = CMD1_OTA_TYPE; //SUBSYS_ID
	sLen += 2;  
	*((int16u*)&sBuf[sLen]) = CMD2_OTA_UPLOADER_TYPE; //CMD_ID
	sLen += 2; 
	sBuf[sLen] = (flag==TRUE)?0:1; //flag
	sLen += 1;
 	sBuf[sLen] = (flag==TRUE)?0:1; //Status
	sLen += 1;
	*((int32u*)&sBuf[sLen]) = otaOffset; //otaOffset
	sLen += 4; 
	SerialPortDataSending(sLen);
}
static void processGetOtaDataReq(int8u *rBuf, int8u len)
{
	sLen = SEAT_COMMAND;
	sBuf = GetSendBuffer();
	*((int16u*)sBuf) = CMD1_OTA_TYPE; //SUBSYS_ID
	sLen += 2;  
	*((int16u*)&sBuf[sLen]) = CMD2_GET_OTA_DATA_TYPE; //CMD_ID
	sLen += 2; 
	sBuf[sLen] = 0; //flag
	sLen += 1;
	sBuf[sLen] = 0x00; //Status
	sLen += 1;
	*((int32u*)&sBuf[sLen]) = *((int32u*)&rBuf[0]); 
	sLen += 4;
	*((int32u*)&sBuf[sLen]) = *((int32u*)&rBuf[4]); 
	sLen += 4; 
	emberAfOtaStorageDriverReadCallback(*(int32u *)&rBuf[0], *(int32u *)&rBuf[4], &sBuf[sLen]);
	sLen += *(int32u *)&rBuf[4];
	SerialPortDataSending(sLen);
}
static void processSetQueryPolicyReq(int8u *rBuf, int8u len)
{
	emAfOtaServerSetQueryPolicy(rBuf[0]);
  
	sLen = SEAT_COMMAND;
	sBuf = GetSendBuffer();
	*((int16u*)sBuf) = CMD1_OTA_TYPE; //SUBSYS_ID
	sLen += 2;  
	*((int16u*)&sBuf[sLen]) = CMD2_SET_QUERY_POLICY_TYPE; //CMD_ID
	sLen += 2; 
	sBuf[sLen] = 0; //flag
	sLen += 1;
	sBuf[sLen] = 0x00; //Status
	sLen += 1;
	sBuf[sLen] = rBuf[0]; //QueryPolicy
	sLen += 1;
	SerialPortDataSending(sLen);
}  
static void processSetMinBlockReq(int8u *rBuf, int8u len)
{
	emAfOtaServerPolicySetMinBlockRequestPeriod(*((int16u*)&rBuf[0]));
  
	sLen = SEAT_COMMAND;
	sBuf = GetSendBuffer();
	*((int16u*)sBuf) = CMD1_OTA_TYPE; //SUBSYS_ID
	sLen += 2;  
	*((int16u*)&sBuf[sLen]) = CMD2_SET_MIN_BLOCK_TYPE; //CMD_ID
	sLen += 2; 
	sBuf[sLen] = 0; //flag
	sLen += 1;
	sBuf[sLen] = 0x00; //Status
	sLen += 1;
	*((int16u*)&sBuf[sLen]) = *((int16u*)&rBuf[0]); // MinBlockTime
	sLen += 2;
	SerialPortDataSending(sLen);
}

EmberNodeId otaNetAddress = 0xffff;
static void processImageNotifyReq(int8u *rBuf, int8u len)
{
	zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_SERVER_TO_CLIENT | ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
					ZCL_OTA_BOOTLOAD_CLUSTER_ID, 
					ZCL_IMAGE_NOTIFY_COMMAND_ID);
	zclBufferAddByte(0);
	zclBufferAddByte(100);

	otaNetAddress = *((int16u*)&rBuf[0]);  
	EmberStatus status = SengledSend(otaNetAddress);
  

	sLen = SEAT_COMMAND;
	sBuf = GetSendBuffer();
	*((int16u*)sBuf) = CMD1_OTA_TYPE; //SUBSYS_ID
	sLen += 2;  
	*((int16u*)&sBuf[sLen]) = CMD2_IMAGE_NOTIFY_TYPE; //CMD_ID
	sLen += 2;
	sBuf[sLen] = 0x00; //flag
	sLen += 1;
	sBuf[sLen] = 0x00; //State
	sLen += 1;  
	*((int16u*)&sBuf[sLen]) = otaNetAddress; //nwkAddr
	sLen += 2;
  
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

boolean emberAfMessageSentCallback(EmberOutgoingMessageType type,
                                   int16u indexOrDestination,
                                   EmberApsFrame* apsFrame,
                                   int16u msgLen,
                                   int8u* message,
                                   EmberStatus status)
{
	int8u messageLen = 0;
	int8u responsType = 0xff;
	int16u sysId = 0xffff, comId = 0xffff, information;  
	int8u zcl_cmd_id = 0xff;

#if 0  
	if (EMBER_ZDO_PROFILE_ID == apsFrame->profileId)
	{
		if (LEAVE_REQUEST == apsFrame->clusterId) //Remove ZIGB Lamp
		{ 
			sysId = CMD1_NETWORK_TYPE; 
			comId = CMD2_REMOVE_DEVICE_TYPE; 
			responsType = SENGLED_ZCL_SEND;
		}  
		else if (BIND_REQUEST  == apsFrame->clusterId) //Binding ZIGB device
		{ 
			if (bindingStep == CLUSTER_NULL)
			{
				if (status != EMBER_SUCCESS)
				{ 
					SengledSendResponse(CMD1_ZIGBEE_TYPE, CMD2_NEW_BINDING_TYPE, status, indexOrDestination, 0xffff);
				}
				return FALSE;
			}
			else 
			{
				if (status == EMBER_SUCCESS)
				{
					bindingEnable = TRUE;        
					bindingStep++;
					if (bindingStep > CLUSTER_METERING)
					{ 
						bindingStep = CLUSTER_ON_OFF;
						SengledSendResponse(CMD1_ZIGBEE_TYPE, CMD2_BINDING_TYPE, status, desNodeId, 0xffff);
						emberEventControlSetInactive(BindingEventControl);
					}
					else
					{
						emberEventControlSetDelayMS(BindingEventControl, 500);
					}
					return FALSE;
				}
        		sysId = CMD1_ZIGBEE_TYPE; 
				comId = CMD2_BINDING_TYPE; 
				responsType = SENGLED_SEND;  
			}
		}
	}
  	else if (HA_PROFILE_ID == apsFrame->profileId)
	{
		if (message[0]&ZCL_MANUFACTURER_SPECIFIC_MASK) //Frame control -> Manufacturer specific
		{ 
			messageLen = 2;
		}
		else
		{
			messageLen = 0;
		}
    
		if (0 == (message[0]&ZCL_CLUSTER_SPECIFIC_COMMAND)) //Frame control -> Frame type
		{ 
			if (ZCL_CONFIGURE_REPORTING_COMMAND_ID == message[messageLen+2]) //Config ZIGB device Report infomation
			{ 
				if (status == EMBER_SUCCESS)
				{
					return FALSE;
				}
				sysId = CMD1_ZIGBEE_TYPE; 
				comId = CMD2_CONFIG_REPORT_TYPE; 
				responsType = SENGLED_SEND_PLUS;
				information = GetConfigReportAttributeId(apsFrame->clusterId, *((int16u*)&message[messageLen+4]));
			}
			else if (ZCL_READ_ATTRIBUTES_COMMAND_ID == message[messageLen+2]) //Get ZIGB Device Attribute
			{ 
				if (status == EMBER_SUCCESS)
				{
					return FALSE;
				}
				sysId = CMD1_ZIGBEE_TYPE; 
				comId = CMD2_GET_ATTRIBUTE_TYPE; 
				responsType = SENGLED_SEND_PLUS;
				information = GetInformation(apsFrame->clusterId, *((int16u*)&message[messageLen+3]));
			}
			else if (ZCL_WRITE_ATTRIBUTES_COMMAND_ID == message[messageLen+2]) //Set ZIGB Device Attribute
			{ 
				sysId = CMD1_ZIGBEE_TYPE; 
				comId = CMD2_SET_ATTRIBUTE_TYPE; 
				responsType = SENGLED_SEND_PLUS;
				if ((apsFrame->clusterId==ZCL_LEVEL_CONTROL_CLUSTER_ID) && (*((int16u*)&message[messageLen+3])==ZCL_ON_LEVEL_ATTRIBUTE_ID))
				{
					information = ATTRIBUTE_ONLEVEL;
				}
			}
		}    
		else if (ZCL_IDENTIFY_CLUSTER_ID == apsFrame->clusterId) //Identify ZIGB Lamp
		{
			sysId = CMD1_CONTROL_TYPE; 
			comId = CMD2_IDENTIFY_TYEP; 
			responsType = SENGLED_ZCL_SEND;
		}
		else if (ZCL_COLOR_CONTROL_CLUSTER_ID == apsFrame->clusterId) //Set ZIGB Lamp Color Temprature
		{ 
			if (type == EMBER_OUTGOING_BROADCAST)
			{
				sysId = CMD1_CONTROL_TYPE; 
				comId = 140; 
				responsType = SENGLED_BRODCAST_SEND;
			}
  			else if (type == EMBER_OUTGOING_MULTICAST)
			{
				sysId = CMD1_CONTROL_TYPE; 
				comId = 143; 
				responsType = SENGLED_BRODCAST_SEND;
			}
			else
			{
				if (newcolorReqFlag == TRUE)
				{
					sysId = CMD1_CONTROL_TYPE; 
					comId = CMD2_NEW_COLOR_TEMP_TYEP; 
					responsType = SENGLED_ZCL_SEND;
				}
				else
				{
					sysId = CMD1_CONTROL_TYPE; 
					comId = CMD2_COLOR_TEMP_TYEP; 
					responsType = SENGLED_ZCL_SEND;
				}	
			}  
		}
		else if (ZCL_LEVEL_CONTROL_CLUSTER_ID == apsFrame->clusterId) //Set ZIGB Lamp Brightness
		{ 
			if (type == EMBER_OUTGOING_BROADCAST)
			{
				sysId = CMD1_CONTROL_TYPE; 
				comId = 139; 
				responsType = SENGLED_BRODCAST_SEND;
			}
			else if (type == EMBER_OUTGOING_MULTICAST)
			{
				sysId = CMD1_CONTROL_TYPE; 
				comId = 142; 
				responsType = SENGLED_BRODCAST_SEND;
			}
			else
			{
				sysId = CMD1_CONTROL_TYPE; 
				comId = CMD2_LEVEL_TYEP; 
				responsType = SENGLED_ZCL_SEND;
			}
		}
		else if (ZCL_ON_OFF_CLUSTER_ID == apsFrame->clusterId) //ZIGB Lamp On Off
		{ 
			if (type == EMBER_OUTGOING_BROADCAST)
			{
				sysId = CMD1_CONTROL_TYPE; 
				comId = 138; 
				responsType = SENGLED_BRODCAST_SEND;
			}
			else if (type == EMBER_OUTGOING_MULTICAST)
			{
				sysId = CMD1_CONTROL_TYPE; 
				comId = 141; 
				responsType = SENGLED_BRODCAST_SEND;
			}
			else
			{
				sysId = CMD1_CONTROL_TYPE; 
				comId = CMD2_ONOFF_TYEP; 
				responsType = SENGLED_ZCL_SEND;
			}
		}
		else if (ZCL_BASIC_CLUSTER_ID == apsFrame->clusterId) //Reset ZIGB device to the factory
		{
			sysId = CMD1_ZIGBEE_TYPE; 
			comId = CMD2_RESET_TO_FACTORY_TYPE; 
			responsType = SENGLED_SEND;
		}    
  	}  
#endif

	if (HA_PROFILE_ID == apsFrame->profileId)
	{
		if (message[0]&ZCL_MANUFACTURER_SPECIFIC_MASK) //Frame control -> Manufacturer specific
		{ 
			messageLen = 2;
		}
		else
		{
			messageLen = 0;
		}
		
		if (0 == (message[0]&ZCL_CLUSTER_SPECIFIC_COMMAND)) //Frame control -> Frame type
		{ 
			if (ZCL_CONFIGURE_REPORTING_COMMAND_ID == message[messageLen+2]) //Config ZIGB device Report infomation
			{ 
				if (status == EMBER_SUCCESS)
				{
					return FALSE;
				}

				information = GetConfigReportAttributeId(apsFrame->clusterId, *((int16u*)&message[messageLen+4]));
				SengledSendPlusResponse(CMD1_ZIGBEE_TYPE, CMD2_CONFIG_REPORT_TYPE, 
										status, indexOrDestination, information);
			}
			else if (ZCL_READ_ATTRIBUTES_COMMAND_ID == message[messageLen+2]) //Get ZIGB Device Attribute
			{ 
				if (status == EMBER_SUCCESS)
				{
					return FALSE;
				}

				information = GetInformation(apsFrame->clusterId, *((int16u*)&message[messageLen+3]));
				SengledSendPlusResponse(CMD1_ZIGBEE_TYPE, CMD2_GET_ATTRIBUTE_TYPE, 
										status, indexOrDestination, information);
			}
			else if (ZCL_WRITE_ATTRIBUTES_COMMAND_ID == message[messageLen+2]) //Set ZIGB Device Attribute
			{ 
				if ((apsFrame->clusterId==ZCL_LEVEL_CONTROL_CLUSTER_ID) && (*((int16u*)&message[messageLen+3])==ZCL_ON_LEVEL_ATTRIBUTE_ID))
				{
					information = ATTRIBUTE_ONLEVEL;
				}
				
				SengledSendPlusResponse(CMD1_ZIGBEE_TYPE, CMD2_SET_ATTRIBUTE_TYPE, 
										status, indexOrDestination, information);
			}		
			return  FALSE;	
		}  			
	}
	
	switch(apsFrame->clusterId)
	{
	case ZCL_BASIC_CLUSTER_ID:
	{	
		SengledSendResponse(CMD1_ZIGBEE_TYPE, 
							CMD2_RESET_TO_FACTORY_TYPE, 
							status, 
							indexOrDestination, 
							0xffff);
		break;
	}
	
	case ZCL_IDENTIFY_CLUSTER_ID:
	{	
		SengledSendZclResponse(CMD1_CONTROL_TYPE, 
								CMD2_IDENTIFY_TYEP, 
								status, 
								indexOrDestination, 
								currentMac);		
		break;
	}
				
	case ZCL_ON_OFF_CLUSTER_ID: // broadcast or multicast don't need check
	{	
		SengledSendZclResponse(CMD1_CONTROL_TYPE, 
								CMD2_ONOFF_TYEP, 
								status, 
								indexOrDestination, 
								currentMac);		
		break;
	}
		
	case ZCL_LEVEL_CONTROL_CLUSTER_ID:
	{
		if (msgLen < EMBER_AF_ZCL_OVERHEAD) {
//    		return EMBER_ERR_FATAL;
		} else if (message[0] & ZCL_MANUFACTURER_SPECIFIC_MASK) {
			if (msgLen < EMBER_AF_ZCL_MANUFACTURER_SPECIFIC_OVERHEAD) {
//				return EMBER_ERR_FATAL;
			}
			zcl_cmd_id = message[4];
		} else {
			zcl_cmd_id = message[2];
		}
		
		if(zcl_cmd_id == ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID)
		{
			SengledSendZclResponse(CMD1_CONTROL_TYPE, 
									CMD2_LEVEL_TYEP, 
									status, 
									indexOrDestination, 
									currentMac);
		}
		else if (zcl_cmd_id == ZCL_STEP_WITH_ON_OFF_COMMAND_ID)
		{
			SengledSendZclResponse(CMD1_CONTROL_TYPE, 
									CMD2_STEP_WITH_ON_OFF_TYPE, 
									status, 
									indexOrDestination, 
									currentMac);
		}
		break;
	}

	case ZCL_COLOR_CONTROL_CLUSTER_ID:
	{
		if (msgLen < EMBER_AF_ZCL_OVERHEAD) {
//    		return EMBER_ERR_FATAL;
		} else if (message[0] & ZCL_MANUFACTURER_SPECIFIC_MASK) {
			if (msgLen < EMBER_AF_ZCL_MANUFACTURER_SPECIFIC_OVERHEAD) {
//				return EMBER_ERR_FATAL;
			}
			zcl_cmd_id = message[4];
		} else {
			zcl_cmd_id = message[2];
		}

		if(zcl_cmd_id == ZCL_MOVE_TO_COLOR_TEMPERATURE_COMMAND_ID)
		{
			SengledSendZclResponse(CMD1_CONTROL_TYPE,
									(newcolorReqFlag == TRUE) ? CMD2_NEW_COLOR_TEMP_TYEP : CMD2_COLOR_TEMP_TYEP, 
									status, 
									indexOrDestination, 
									currentMac);		
		} 
		else if (zcl_cmd_id == ZCL_MOVE_TO_COLOR_COMMAND_ID)
		{
			SengledSendZclResponse(CMD1_CONTROL_TYPE,
									CMD2_RGBCOLOR_TYPE,
									status, 
									indexOrDestination, 
									currentMac);

		}
		break;
	}
	case BIND_REQUEST:
	{
		if (bindingStep == CLUSTER_NULL)
		{
			if (status != EMBER_SUCCESS)
			{ 
				SengledSendResponse(CMD1_ZIGBEE_TYPE, 
									CMD2_NEW_BINDING_TYPE, 
									status, 
									indexOrDestination, 
									0xffff);
			}
			return FALSE;
		}
		else 
		{
			if (status == EMBER_SUCCESS)
			{
				bindingEnable = TRUE;        
				bindingStep++;
				if (bindingStep > CLUSTER_METERING)
				{ 
					bindingStep = CLUSTER_ON_OFF;
					SengledSendResponse(CMD1_ZIGBEE_TYPE, 
										CMD2_BINDING_TYPE, 
										status, 
										desNodeId, 
										0xffff);
					emberEventControlSetInactive(BindingEventControl);
				}
				else
				{
					emberEventControlSetDelayMS(BindingEventControl, 500);
				}
				return FALSE;
			}

			SengledSendResponse(CMD1_ZIGBEE_TYPE, 
								CMD2_BINDING_TYPE , 
								status, 
								indexOrDestination, 
								0xffff);
		}		
		break;
	}
		
	case LEAVE_REQUEST:
	{
		SengledSendZclResponse(CMD1_NETWORK_TYPE, 
								CMD2_REMOVE_DEVICE_TYPE, 
								status, 
								indexOrDestination, 
								currentMac);			
		break;
	}
	default:
	{
#ifdef DEBUG_SENGLED
		sengled_cmd_not_support_rsp(indexOrDestination);
#endif
	}
	}
	
	return FALSE;
}
//*********************************************************************
// Data Parse
//*********************************************************************

void (*process_wifi_to_zc_cmd_control[7])(int8u*, int8u) = {
	processIdentifyReq,
	processColorTempratureReq,
	processLevelReq,
	processOnoffReq,
	processRGBColorReq,      
	process_step_with_on_off_req,    // ALLERT!!FOR ALLIGN!!
	processNewColorTempratureReq,
};

void (*process_wifi_to_zc_cmd_network[2])(int8u*, int8u) = {
	ProcessCreateNetworkReq,
	ProcessRemoveDeviceReq
};

void (*process_wifi_to_zc_cmd_zigbee[13])(int8u*, int8u) = {
	ProcessDeleteNetworkReq,
	ProcessChangeChannelReq,
	ProcessSetPermitJoiningTimeReq,
	NULL,
	ProcessResetToTheFactoryReq,
	ProcessBindingReq,
	ProcessConfigReportReq,
	processGetDeviceAttributeReq,
	NULL,
	processSetDeviceAttributeReq,
	NULL,
	NULL,
	ProcessNewBindingReq 
};

void (*process_wifi_to_zc_cmd_ota[10])(int8u*, int8u) = {
	NoticeZIGBDeviceNewOTAFile,
	NULL,
	NULL,
	processOtaUploaderReq,	
	processGetOtaDataReq,
	NULL,
	NULL,
	processSetQueryPolicyReq,
	processSetMinBlockReq,
	processImageNotifyReq
};

void DataParse(int8u *rBuf, int8u len)
{
	int16u cmd1, cmd2;    

	cmd1 = *((int16u*)rBuf); rBuf += 2;
	cmd2 = *((int16u*)rBuf); rBuf += 2;

	// cmd alignment, to save ram space;
	// CMD1_CONTROL_TYPE = 1
	// CMD1_NETWORK_TYPE = 3
	// CMD1_ZIGBEE_TYPE  = 4
	// CMD1_OTA_TYPE     = 5
	switch (cmd1)
	{
	case CMD1_CONTROL_TYPE:
	{
		if (cmd2 == CMD2_NEW_COLOR_TEMP_TYEP)
		{	
			cmd2 = CMD2_STEP_WITH_ON_OFF_TYPE + 1;  //!!ALERT!!! for alignment!!
		}
		cmd2 -= CMD2_IDENTIFY_TYEP;
		process_wifi_to_zc_cmd_control[cmd2](rBuf, len);
	}
	break;
	case CMD1_NETWORK_TYPE:
	{
		cmd2 -= CMD2_CREATE_NETWORK_TYPE;
		process_wifi_to_zc_cmd_network[cmd2](rBuf, len);
	}
	break;
	case CMD1_ZIGBEE_TYPE:
	{
		cmd2 -= 1;
		process_wifi_to_zc_cmd_zigbee[cmd2](rBuf, len);
	}
	break;
	case CMD1_OTA_TYPE:
	{
		cmd2 -= 1;
		process_wifi_to_zc_cmd_ota[cmd2](rBuf, len);
	}
	break;
	default:
	break;
	}
}

//*******************************************************************************
// Main
//*******************************************************************************
/** @brief Get Radio Power For Channel
 *
 * This callback is called by the framework when it is setting the radio power
 * during the discovery process. The framework will set the radio power
 * depending on what is returned by this callback.
 *
 * @param channel   Ver.: always
 */
int8s emberAfPluginNetworkFindGetRadioPowerForChannelCallback(int8u channel)
{
  halGpioConfig(PORTC_PIN(1), GPIOCFG_IN_PUD);
  
  if (channel == 26)
  {
	  return -43;
  }
  else if (channel == 25)
  { 
	  return -5;
  }
  else
  { 
	  return -3;
  }
}

void emberAfMainInitCallback(void)
{
	halGpioConfig(PORTC_PIN(1), GPIOCFG_IN_PUD);
  
	if (GPIO_PCIN & PC1_MASK) // Z02 ?
	{ 
		halGpioConfig(PORTC_PIN(6), GPIOCFG_OUT_ALT);
	}
}

/** @brief Main Tick
 *
 * Whenever main application tick is called, this callback will be called at
 * the end of the main tick execution.
 *
 */
void emberAfMainTickCallback(void)
{
  ;
}

void emberPollHandler(EmberNodeId childId, boolean transmitExpected)
{
#ifdef DATA_POLL_SUPPORT

	int8u i;
	int16u bufIndex = 0;
  
	sLen = SEAT_COMMAND;
	sBuf = GetSendBuffer();
	*((int16u*)sBuf) = CMD1_ZIGBEE_TYPE; 
	sLen += 2;					//SUBSYS_ID
	*((int16u*)&sBuf[sLen]) = CMD2_REPORTING_TYPE; 
	sLen += 2;					//CMD_ID

	sBuf[sLen++] = 0x00;		//Flag
	sBuf[sLen++] = 0x00;		//state  
	
	*((int16u*)&sBuf[sLen]) = childId; 
	sLen += 2;					//desNwkAddr	
	
	*((int16u*)&sBuf[sLen]) = ATTRIBUTE_DATA_POLL; 
	sLen += 2;

	for (i=0; i<8; i++)
	{ 
		sBuf[sLen++] = 0;
	}	

	sBuf[sLen++] = 0xff;
	sBuf[sLen++] = 0xff;
	sBuf[sLen++] = 0xff;
  
	SerialPortDataSending(sLen);
#endif
}


boolean emberAfSengledMobileScenseClusterMobileControlCallback(int16u controlType,
															int16u controlData,
															int16u transitionTime)
{
	int8u i;
	int16u bufIndex = 0;
	EmberAfAttributeId attributeId;
  
	sLen = SEAT_COMMAND;
	sBuf = GetSendBuffer();
	*((int16u*)sBuf) = CMD1_ZIGBEE_TYPE; sLen += 2;  //SUBSYS_ID
	*((int16u*)&sBuf[sLen]) = CMD2_MOBILE_SCENE_TYPE; sLen += 2; //CMD_ID
	sBuf[sLen++] = 0x00;		//Flag
	sBuf[sLen++] = 0x00;		//state  
	*((int16u*)&sBuf[sLen]) = emberAfCurrentCommand()->source; sLen += 2;  

	*((int16u*)&sBuf[sLen]) = controlType; sLen += 2;
	*((int16u*)&sBuf[sLen]) = controlData; sLen += 2;
	*((int16u*)&sBuf[sLen]) = transitionTime; sLen += 2;

	for (i=4; i<8; i++)
	{ 
		sBuf[sLen++] = 0;
	}	
	
	sBuf[sLen++] = 0xff;
	sBuf[sLen++] = 0xff;
	sBuf[sLen++] = 0xff;
  
	SerialPortDataSending(sLen);	
	return TRUE;
}
/** @brief Stack Status
 *
 * This function is called by the application framework from the stack status
 * handler.  This callbacks provides applications an opportunity to be notified
 * of changes to the stack status and take appropriate action.  The return code
 * from this callback is ignored by the framework.  The framework will always
 * process the stack status after the callback returns.
 *
 * @param status   Ver.: always
 */
boolean emberAfStackStatusCallback(EmberStatus status)
{
  return FALSE;
}

/** @brief Join
 *
 * This callback is called by the plugin when a joinable network has been
 * found. If the application returns true, the plugin will attempt to join the
 * network. Otherwise, the plugin will ignore the network and continue
 * searching. Applications can use this callback to implement a network
 * blacklist.
 *
 * @param networkFound   Ver.: always
 * @param lqi   Ver.: always
 * @param rssi   Ver.: always
 */
boolean emberAfPluginNetworkFindJoinCallback(EmberZigbeeNetwork *networkFound,
                                          int8u lqi,
                                          int8s rssi)
{
  return TRUE;
}
