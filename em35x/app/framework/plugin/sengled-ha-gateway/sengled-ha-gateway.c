// *****************************************************************************
// * sengled-cli.c
// *
// * This code provides support for sengled self defined command
// *
// * Copyright 2016 by Sengled Corporation. All rights reserved.              
// *****************************************************************************
#include "app/framework/include/af.h"
#include "sengled-ha-gateway.h"
#include "app/framework/util/attribute-storage.h"

extern EmberApsFrame globalApsFrame;
extern int8u appZclBuffer[EMBER_AF_MAXIMUM_SEND_PAYLOAD_LENGTH];
extern int16u appZclBufferLen;

// zcl global report <src endpoint id:1> <cluster id:2> <attribute id:2> <mask:1>
void sengledGlobalReportCommand(EmberAfClusterId clusterId, EmberAfAttributeId attributeId, int8u mask)
{
  EmberAfStatus status;
  EmberAfAttributeType type;
  int8u size;
  int8u data[ATTRIBUTE_LARGEST];

  type = ZCL_BOOLEAN_ATTRIBUTE_TYPE;
  
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

  if(attributeId == ZCL_ON_OFF_ATTRIBUTE_ID && clusterId == ZCL_ON_OFF_CLUSTER_ID)
  {
    data[0] = 0x00; // when power off, report off;
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

void emAfSengledSendCommand(int16u nodeId)
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
  emberAfCorePrintln("Sengled SendUnicast");
  emberAfCorePrintln("  destination: 0x%2x",destination);
  emberAfCorePrintln("  appZclBufferLen:0x%2x",appZclBufferLen);
  emberAfCorePrintln("  globalApsFrame:");  
  emberAfCorePrintln("    profileId: 0x%2x",globalApsFrame.profileId);  
  emberAfCorePrintln("    clusterId: 0x%2x",globalApsFrame.clusterId);  
  emberAfCorePrintln("    groupId: 0x%2x",globalApsFrame.groupId); 
  emberAfCorePrintln("    options: 0x%2x",globalApsFrame.options);   
#endif

  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("Error: CLI Send failed, status: 0x%X", status);
  }
}



