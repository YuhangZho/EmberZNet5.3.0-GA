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

void attribute_report(EmberAfClusterId clusterId, EmberAfAttributeId attributeId, EmberAfAttributeType type, int8u mask);

void send_zcl_cmd_unicast(int16u nodeId);


