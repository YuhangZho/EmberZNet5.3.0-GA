// *****************************************************************************
// * sengled-cli.h
// *
// * This code provides support for sengled self defined command
// *
// * Copyright 2016 by Sengled Corporation. All rights reserved.              
// *****************************************************************************

#define CLUSTER_IS_SERVER 0x01
#define CLUSTER_IS_CLIENT 0x00
#define COORDINATOR_NODE_ID 0x0000

void sengledGlobalReportCommand(EmberAfClusterId clusterId, EmberAfAttributeId attributeId, int8u mask);

void emAfSengledSendCommand(int16u nodeId);


