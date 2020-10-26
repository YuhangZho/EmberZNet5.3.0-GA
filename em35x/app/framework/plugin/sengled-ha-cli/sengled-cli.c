/*
************************************************************************************************************************
*                                                      **COMMON USE**
*                                                     TrackLight | A19EU
*
*                                  (c) Copyright 2017-**; Sengled, Inc.; 
*                           All rights reserved.  Protected by international copyright laws.
*
*
* File    : sengled-cli.c
* Path   : app/framework/plugin/sengled-ha-cli
* By      : ROBIN
* Version : 0x00000001
*
* Description:
* ---------------
* Function
* (1) Send and Report
*           a. attribute_report     
*           b. send_zcl_cmd_unicast
*
*
* History:
* ---------------
*
*
*
************************************************************************************************************************
*/

#include "app/framework/include/af.h"
#include "sengled-cli.h"
#include "app/framework/util/attribute-storage.h"

extern EmberApsFrame globalApsFrame;
extern int8u appZclBuffer[EMBER_AF_MAXIMUM_SEND_PAYLOAD_LENGTH];
extern int16u appZclBufferLen;

// zcl global report <src endpoint id:1> <cluster id:2> <attribute id:2> <mask:1>
void attribute_report(EmberAfClusterId clusterId, EmberAfAttributeId attributeId, EmberAfAttributeType type, int8u mask)
{
	EmberAfStatus status;
	int8u size;
	int8u data[ATTRIBUTE_LARGEST];
  
	zclBufferSetup(ZCL_PROFILE_WIDE_COMMAND
					| (mask == 0
					? ZCL_FRAME_CONTROL_CLIENT_TO_SERVER
					: ZCL_FRAME_CONTROL_SERVER_TO_CLIENT),
					clusterId,
					ZCL_REPORT_ATTRIBUTES_COMMAND_ID);
	
	zclBufferAddWord(attributeId);
	zclBufferAddByte(type);

	size = (emberAfIsThisDataTypeAStringType(type)
			? emberAfStringLength(data) + 1
			: emberAfGetDataSize(type));

	if(attributeId == ZCL_ON_OFF_ATTRIBUTE_ID && clusterId == ZCL_ON_OFF_CLUSTER_ID) {
		data[0] = 0x00; // when power off, report off;
	}

	if(attributeId == ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID && clusterId == ZCL_SIMPLE_METERING_CLUSTER_ID) {
		data[0] = 0x00;
		data[1] = 0x00;
		data[2] = 0x00;
		data[3] = 0x00;
	}
#if (BIGENDIAN_CPU)
	if (isThisDataTypeSentLittleEndianOTA(type)) {
		emberReverseMemCopy(appZclBuffer + appZclBufferLen, data, size);
	} else {
		MEMCOPY(appZclBuffer + appZclBufferLen, data, size);
	}
#else
	MEMCOPY(appZclBuffer + appZclBufferLen, data, size);
#endif
	appZclBufferLen += size;
	//cliBufferPrint();
}

// ******************************************************
// send <nodeId> <src endpoint 1> <dst endpoint 1>

// ******************************************************

void send_zcl_cmd_unicast(int16u nodeId)
{
	int16u destination = nodeId; 
	int8u srcEndpoint = 0x01;
	int8u dstEndpoint = 0x01;
	int8u *commandName = (int8u *)emberCurrentCommand->name;
	EmberStatus status;
	int8u label;

	emAfApsFrameEndpointSetup(srcEndpoint, dstEndpoint);

	status = emberAfSendUnicast(EMBER_OUTGOING_DIRECT,
								destination,
								&globalApsFrame,
								appZclBufferLen,
								appZclBuffer);
#ifdef ROBIN_DEBUG 
/*
	emberAfCorePrintln("Sengled SendUnicast");
	emberAfCorePrintln("  destination: 0x%2x",destination);
	emberAfCorePrintln("  appZclBufferLen:0x%2x",appZclBufferLen);
	emberAfCorePrintln("  globalApsFrame:");  
	emberAfCorePrintln("    profileId: 0x%2x",globalApsFrame.profileId);  
	emberAfCorePrintln("    clusterId: 0x%2x",globalApsFrame.clusterId);  
	emberAfCorePrintln("    groupId: 0x%2x",globalApsFrame.groupId); 
	emberAfCorePrintln("    options: 0x%2x",globalApsFrame.options);   
*/
#endif

	if (status != EMBER_SUCCESS) {
		emberAfCorePrintln("Error: CLI Send failed, status: 0x%X", status);
	}
}



