// IAS Zone Plugin

// Client Operation:
//   1. Look for ZDO device announce notification.
//   2. Perform ZDO match descriptor on device.
//   3. If supports IAS Zone Server, Add that server to our known list.
//     Write CIE Address.
//   4. Read CIE address, verify it is ours.  This is done mostly because
//     the test case requires it.
//   5. Read the IAS Zone Server attributes.
//     Record in table.
//   6. When we get an enroll request, give them our (only) zone ID.
//   7. When we get a notification, read their attributes.

// Improvements that could be made:
//   Add support for multiple endpoints on server.  Most often this is a
//   legacy security system retrofitted with a single ZigBee radio.  Therefore
//   each sensor is on a different endpoint.  Right now our client only
//   handles a single endpoint per node.
//
//   Integration with Poll Control.  When the device boots we should configure
//   its polling to make it possible to read/write its attributes.
//
//   Update the emberAfIasZoneClientKnownServers list when we know a server
//   un-enrolls. Right now, we don't have any way to tell when we don't need
//   to keep track of a server anymore, i.e., when it un-enrolls. Therefore,
//   we could potentially keep adding servers to our known list, and run out
//   of room to add more. Fortunately, we have two things working for us:
//     1. Servers will most likely stay around in a network. It is unlikely
//        that an IAS Zone Client in production will have to handle 254
//        different servers.
//     2. If a server un-enrolls and then enrolls again, it will get the same
//        Zone ID and have a spot in the list, since we store servers by
//        long address.

#include "af.h"
#include "ias-zone-client.h"

//-----------------------------------------------------------------------------
// Globals

IasZoneDevice emberAfIasZoneClientKnownServers[EMBER_AF_PLUGIN_IAS_ZONE_CLIENT_MAX_DEVICES];

typedef enum {
  IAS_ZONE_CLIENT_STATE_NONE,
  IAS_ZONE_CLIENT_STATE_DISCOVER_ENDPOINT,
  IAS_ZONE_CLIENT_STATE_SET_CIE_ADDRESS,
  IAS_ZONE_CLIENT_STATE_READ_CIE_ADDRESS,
  IAS_ZONE_CLIENT_STATE_READ_ATTRIBUTES,
} IasZoneClientState;

static IasZoneClientState iasZoneClientState = IAS_ZONE_CLIENT_STATE_NONE;
static int8u currentIndex = NO_INDEX;
static int8u myEndpoint = 0;

EmberEventControl emberAfPluginIasZoneClientStateMachineEventControl;

//-----------------------------------------------------------------------------
// Forward Declarations

void readIasZoneServerAttributes(EmberNodeId nodeId);
static void iasClientSaveCommand(void);
static void iasClientLoadCommand(void);

//-----------------------------------------------------------------------------
// Functions

void emberAfIasZoneClusterClientInitCallback(int8u endpoint)
{
  emAfClearServers();
  myEndpoint = endpoint;
  iasClientLoadCommand();
}

void emAfClearServers(void)
{
  MEMSET(emberAfIasZoneClientKnownServers, 0xFF,
         sizeof(IasZoneDevice)
         * EMBER_AF_PLUGIN_IAS_ZONE_CLIENT_MAX_DEVICES);
}

static void clearState(void)
{
  currentIndex = 0;
  iasZoneClientState = IAS_ZONE_CLIENT_STATE_NONE;
}

static void setServerZoneStatus(int8u serverIndex, int16u zoneStatus)
{
  emberAfIasZoneClientKnownServers[serverIndex].zoneStatus = zoneStatus;
  iasClientSaveCommand();
}

static void setServerIeee(int8u serverIndex, int8u* ieeeAddress)
{
  MEMCOPY(emberAfIasZoneClientKnownServers[serverIndex].ieeeAddress, ieeeAddress, EUI64_SIZE);
  iasClientSaveCommand();
}

static void clearServerIeee(int8u serverIndex)
{
  MEMSET(emberAfIasZoneClientKnownServers[serverIndex].ieeeAddress,
         0xFF,
         sizeof(IasZoneDevice));
  iasClientSaveCommand();
}

static void setServerNodeId(int8u serverIndex, EmberNodeId nodeId)
{
  emberAfIasZoneClientKnownServers[serverIndex].nodeId = nodeId;
}

static void clearServerNodeId(int8u serverIndex)
{
  emberAfIasZoneClientKnownServers[serverIndex].nodeId = EMBER_NULL_NODE_ID;
}

static void setServerZoneState(int8u serverIndex, int8u zoneState)
{
  emberAfIasZoneClientKnownServers[serverIndex].zoneState = zoneState;
  iasClientSaveCommand();
}

static void setServerEndpoint(int8u serverIndex, int8u endpoint)
{
  emberAfIasZoneClientKnownServers[serverIndex].endpoint = endpoint;
  iasClientSaveCommand();
}

static void setServerZoneType(int8u serverIndex, int16u zoneType)
{
  emberAfIasZoneClientKnownServers[serverIndex].zoneType = zoneType;
  iasClientSaveCommand();
}

static void setServerZoneId(int8u serverIndex, int16u zoneId)
{
  emberAfIasZoneClientKnownServers[serverIndex].zoneId = zoneId;
  iasClientSaveCommand();
}

static void setCurrentIndex(int8u serverIndex)
{
  currentIndex = serverIndex;
  iasClientSaveCommand();
}

static void setIasZoneClientState(int8u clientState)
{
  iasZoneClientState = clientState;
  iasClientSaveCommand();
}

static void iasClientSaveCommand(void)
{
#if defined(EZSP_HOST) && !defined(EMBER_TEST) && defined(UNIX_HOST)
  FILE *fp;
  int16u i, j;

  // save zone server list
  fp = fopen("iaszone.txt", "w");

  for (i = 0; i < EMBER_AF_PLUGIN_IAS_ZONE_CLIENT_MAX_DEVICES; i++) {
    if (emberAfIasZoneClientKnownServers[i].zoneId != 0xFF) {
      fprintf(fp, "%x %x %x %x %x ", emberAfIasZoneClientKnownServers[i].zoneId,
              emberAfIasZoneClientKnownServers[i].zoneStatus,
              emberAfIasZoneClientKnownServers[i].zoneState,
              emberAfIasZoneClientKnownServers[i].endpoint,
              emberAfIasZoneClientKnownServers[i].zoneType);
      for (j = 0; j < 8; j++) {
        fprintf(fp, "%x ", emberAfIasZoneClientKnownServers[i].ieeeAddress[j]);
      }
    }
  }
  // Write something to mark the end of the file.
  fprintf(fp, "ff");
  assert(fclose(fp) == 0);
#endif //#if defined(EZSP_HOST) && !defined(EMBER_TEST) && defined(UNIX_HOST)
}

static void iasClientLoadCommand(void)
{
#if defined(EZSP_HOST) && !defined(EMBER_TEST) && defined(UNIX_HOST)
  FILE *fp;
  int16u i, j;

  unsigned int data1, data2, data3, data4, data5;

  fp = fopen("iaszone.txt", "r");

  if (!fp) {
    return;
  }

  for (i = 0; i < EMBER_AF_PLUGIN_IAS_ZONE_CLIENT_MAX_DEVICES; i++) {
    if (feof(fp) == 0) {
      break;
    }
    fscanf(fp, "%x ", &data1);
    if (data1 == 0xff) {
      break;
    }
    fscanf(fp,
           "%x %x %x %x ",
           &data2,
           &data3,
           &data4,
           &data5);

    emberAfIasZoneClientKnownServers[i].zoneId = (int8u) data1;
    emberAfIasZoneClientKnownServers[i].zoneStatus = (int16u) data2;
    emberAfIasZoneClientKnownServers[i].zoneState = (int8u) data3;
    emberAfIasZoneClientKnownServers[i].endpoint = (int8u) data4;
    emberAfIasZoneClientKnownServers[i].zoneType = (int16u) data5;

    for (j = 0; j < 8; j++) {
      fscanf(fp, "%x ", &data1);
      emberAfIasZoneClientKnownServers[i].ieeeAddress[j] = (int8u) data1;
    }
  }
  assert(fclose(fp) == 0);
#endif // #if defined(EZSP_HOST) && !defined(EMBER_TEST) && defined(UNIX_HOST)
}

static int8u findIasZoneServerByIeee(int8u* ieeeAddress)
{
  int8u i;
  for (i = 0; i < EMBER_AF_PLUGIN_IAS_ZONE_CLIENT_MAX_DEVICES; i++) {
    if (0 == MEMCOMPARE(ieeeAddress,
                        emberAfIasZoneClientKnownServers[i].ieeeAddress,
                        EUI64_SIZE)) {
      return i;
    }
  }
  return NO_INDEX;
}

static int8u findIasZoneServerByNodeId(EmberNodeId nodeId)
{
  int8u i;
  for (i = 0; i < EMBER_AF_PLUGIN_IAS_ZONE_CLIENT_MAX_DEVICES; i++) {
    if (nodeId == emberAfIasZoneClientKnownServers[i].nodeId) {
      return i;
    }
  }

  // If we didn't find the node ID in the table, see if the stack knows about
  // it.
  EmberEUI64 eui64;
  if (emberLookupEui64ByNodeId(nodeId, eui64) == EMBER_SUCCESS) {
    i = findIasZoneServerByIeee(eui64);
    if (i != NO_INDEX) {
      setServerNodeId(i, nodeId);
    }
  }

  return i;
}

boolean emberAfIasZoneClusterZoneStatusChangeNotificationCallback(int16u zoneStatus,
                                                               int8u extendedStatus,
                                                               int8u zoneId,
                                                               int16u delay)
{
  int8u serverIndex = findIasZoneServerByNodeId(emberAfCurrentCommand()->source);
  int8u status = EMBER_ZCL_STATUS_NOT_FOUND; 
  
  if (serverIndex != NO_INDEX) {
    status = EMBER_ZCL_STATUS_SUCCESS;
    setServerZoneStatus(serverIndex, zoneStatus);

    emberAfIasZoneClusterPrintln("Zone %d status change, 0x%2X from 0x%2X",
                                 zoneId,
                                 zoneStatus,
                                 emberAfCurrentCommand()->source);

    // The Test case calls for readding attributes after status change.
    //   that is silly for the production device.
    // readIasZoneServerAttributes(emberAfCurrentCommand()->source);
  }
  emberAfSendDefaultResponse(emberAfCurrentCommand(), status);
  
#ifdef HEIMAN_SUPPORT
// BugNum: COOR-H-180417-01
  int16u nodeId = emberAfCurrentCommand()->source;

  sengled_send_notification_to_gateway(ZCL_IAS_ZONE_CLUSTER_ID, 
											  ZCL_ZONE_STATUS_ATTRIBUTE_ID ,
											  nodeId,
											  zoneStatus,
											  zoneId,
											  delay);
#endif

  return TRUE;
}

boolean emberAfIasZoneClusterZoneEnrollRequestCallback(int16u zoneType,
                                                    int16u manufacturerCode)
{
  EmberAfIasEnrollResponseCode responseCode = EMBER_ZCL_IAS_ENROLL_RESPONSE_CODE_NO_ENROLL_PERMIT;
  int8u zoneId = UNKNOWN_ZONE_ID;
  int8u serverIndex = findIasZoneServerByNodeId(emberAfCurrentCommand()->source);
  EmberStatus status;

  if (serverIndex != NO_INDEX) {
    responseCode = EMBER_ZCL_IAS_ENROLL_RESPONSE_CODE_SUCCESS;
    zoneId = serverIndex;
    setServerZoneId(serverIndex, zoneId);
  }
  emberAfFillCommandIasZoneClusterZoneEnrollResponse(responseCode,
                                                     zoneId);
  // Need to send this command with our source EUI because the server will
  // check our EUI64 against his CIE Address to see if we're his CIE.
  emberAfGetCommandApsFrame()->options |= EMBER_APS_OPTION_SOURCE_EUI64;
  status = emberAfSendResponse();
  emberAfCorePrintln("Sent enroll response with responseCode: 0x%X, zoneId: 0x%X, status: 0x%X",
                     responseCode,
                     zoneId,
                     status);
  return TRUE;
}

void emberAfPluginIasZoneClientStateMachineEventHandler(void)
{
  emberAfIasZoneClusterPrintln("IAS Zone Client Timeout waiting for message response.");
  emberEventControlSetInactive(emberAfPluginIasZoneClientStateMachineEventControl);
  clearState();
}

static int8u addServer(EmberNodeId nodeId, int8u* ieeeAddress)
{
  int8u i = findIasZoneServerByIeee(ieeeAddress);
  if (i != NO_INDEX) {
    return i;
  }

  for (i = 0; i < EMBER_AF_PLUGIN_IAS_ZONE_CLIENT_MAX_DEVICES; i++) {
    const int8u unsetEui64[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    if (0 == MEMCOMPARE(emberAfIasZoneClientKnownServers[i].ieeeAddress,
                        unsetEui64,
                        EUI64_SIZE)) {
      setServerIeee(i, ieeeAddress);
      setServerNodeId(i, nodeId);
      setServerEndpoint(i, UNKNOWN_ENDPOINT);
      return i;
    }
  }
  return NO_INDEX;
}

static void removeServer(int8u* ieeeAddress)
{
  int8u index = findIasZoneServerByIeee(ieeeAddress);
  clearServerIeee(index);
  clearServerNodeId(index);
}

static EmberStatus sendCommand(EmberNodeId destAddress)
{
  emberAfSetCommandEndpoints(myEndpoint, emberAfIasZoneClientKnownServers[currentIndex].endpoint);
  EmberStatus status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, destAddress);
  emberAfIasZoneClusterPrintln("Sent IAS Zone Client Command to 0x%2X (%d -> %d) status: 0x%X",
                               destAddress,
                               myEndpoint,
                               emberAfIasZoneClientKnownServers[currentIndex].endpoint,
                               status);
  if (status != EMBER_SUCCESS) {
    clearState();
  }
  return status;
}

static void setCieAddress(EmberNodeId destAddress)
{
  int8u writeAttributes[] = {
    LOW_BYTE(ZCL_IAS_CIE_ADDRESS_ATTRIBUTE_ID),
    HIGH_BYTE(ZCL_IAS_CIE_ADDRESS_ATTRIBUTE_ID),
    ZCL_IEEE_ADDRESS_ATTRIBUTE_TYPE,
    0, 0, 0, 0, 0, 0, 0, 0,  // ieee (filled in later)
  };
  emberAfGetEui64(&writeAttributes[3]);
  emberAfFillCommandGlobalClientToServerWriteAttributes(ZCL_IAS_ZONE_CLUSTER_ID,
                                                        writeAttributes,
                                                        sizeof(writeAttributes));
  emberAfIasZoneClusterPrintln("Writing CIE Address to IAS Zone Server");
  if (EMBER_SUCCESS == sendCommand(destAddress)) {
    setIasZoneClientState(IAS_ZONE_CLIENT_STATE_SET_CIE_ADDRESS);
  }
}

static void iasZoneClientServiceDiscoveryCallback(const EmberAfServiceDiscoveryResult* result)
{
  if (result->status == EMBER_AF_UNICAST_SERVICE_DISCOVERY_COMPLETE_WITH_RESPONSE
      && result->zdoRequestClusterId == MATCH_DESCRIPTORS_REQUEST) {
    const EmberAfEndpointList* endpointList = (const EmberAfEndpointList *)result->responseData;
    if (endpointList->count > 0) {
      setServerEndpoint(currentIndex, endpointList->list[0]);
      emberAfIasZoneClusterPrintln("Device 0x%2X supports IAS Zone Server",
                                   result->matchAddress);
      setCieAddress(result->matchAddress);
      return;
    }
  }
  clearState();
}

static void checkForIasZoneServer(EmberNodeId emberNodeId, int8u* ieeeAddress)
{
  int8u endpointIndex = emberAfIndexFromEndpoint(myEndpoint);
  int16u profileId = emberAfProfileIdFromIndex(endpointIndex);
  int8u serverIndex = addServer(emberNodeId, ieeeAddress);

  if (serverIndex == NO_INDEX) {
    emberAfIasZoneClusterPrintln("Error: Could not add IAS Zone server.");
    return;
  }

  setCurrentIndex(serverIndex);

  if (emberAfIasZoneClientKnownServers[serverIndex].endpoint != UNKNOWN_ENDPOINT) {
    // If a remote endpoint that you have already seen announces itself,
    // write your IEEE in them just in case they left and are rejoining. --agkeesle
    // Bug: EMAPPFWKV2-1078
    setCieAddress(emberNodeId);
    emberAfIasZoneClusterPrintln("Node 0x%2X already known to IAS client", emberNodeId);
    return;
  }

  EmberStatus status = emberAfFindDevicesByProfileAndCluster(emberNodeId,
                                                             profileId,
                                                             ZCL_IAS_ZONE_CLUSTER_ID,
                                                             TRUE,  // server cluster?
                                                             iasZoneClientServiceDiscoveryCallback);

  if (status != EMBER_SUCCESS) {
    emberAfIasZoneClusterPrintln("Error: Failed to initiate service discovery for IAS Zone Server 0x%2X", emberNodeId);
    clearState();
  }
}

void emberAfPluginIasZoneClientZdoCallback(EmberNodeId emberNodeId,
                                                          EmberApsFrame* apsFrame,
                                                          int8u* message,
                                                          int16u length)
{
  emberAfIasZoneClusterPrintln("Incoming ZDO, Cluster: 0x%2X", apsFrame->clusterId);
  if (apsFrame->clusterId == END_DEVICE_ANNOUNCE) {
    checkForIasZoneServer(emberNodeId, &(message[3]));
  }
}

void readIasZoneServerAttributes(EmberNodeId nodeId)
{
  int8u iasZoneAttributeIds[] = {
    LOW_BYTE(ZCL_ZONE_STATE_ATTRIBUTE_ID),
    HIGH_BYTE(ZCL_ZONE_STATE_ATTRIBUTE_ID),

    LOW_BYTE(ZCL_ZONE_TYPE_ATTRIBUTE_ID),
    HIGH_BYTE(ZCL_ZONE_TYPE_ATTRIBUTE_ID),

    LOW_BYTE(ZCL_ZONE_STATUS_ATTRIBUTE_ID),
    HIGH_BYTE(ZCL_ZONE_STATUS_ATTRIBUTE_ID),
  };
  emberAfFillCommandGlobalClientToServerReadAttributes(ZCL_IAS_ZONE_CLUSTER_ID,
                                                       iasZoneAttributeIds,
                                                       sizeof(iasZoneAttributeIds));
  if (EMBER_SUCCESS == sendCommand(nodeId)) {
    setIasZoneClientState(IAS_ZONE_CLIENT_STATE_READ_ATTRIBUTES);
  }
}

void readIasZoneServerCieAddress(EmberNodeId nodeId)
{
  int8u iasZoneAttributeIds[] = {
    LOW_BYTE(ZCL_IAS_CIE_ADDRESS_ATTRIBUTE_ID),
    HIGH_BYTE(ZCL_IAS_CIE_ADDRESS_ATTRIBUTE_ID),
  };
  emberAfFillCommandGlobalClientToServerReadAttributes(ZCL_IAS_ZONE_CLUSTER_ID,
                                                       iasZoneAttributeIds,
                                                       sizeof(iasZoneAttributeIds));
  if (EMBER_SUCCESS == sendCommand(nodeId)) {
    setIasZoneClientState(IAS_ZONE_CLIENT_STATE_READ_CIE_ADDRESS);
  }
}

void emberAfPluginIasZoneClientWriteAttributesResponseCallback(EmberAfClusterId clusterId,
                                                               int8u * buffer,
                                                               int16u bufLen)
{
  if (clusterId == ZCL_IAS_ZONE_CLUSTER_ID
      && iasZoneClientState == IAS_ZONE_CLIENT_STATE_SET_CIE_ADDRESS
      && buffer[0] == EMBER_ZCL_STATUS_SUCCESS) {
    readIasZoneServerCieAddress(emberAfCurrentCommand()->source);
    return;
  }
  return;
}

void emberAfPluginIasZoneClientReadAttributesResponseCallback(EmberAfClusterId clusterId,
                                                              int8u * buffer,
                                                              int16u bufLen)
{
  int8u zoneStatus, zoneType, zoneState;
  if (clusterId == ZCL_IAS_ZONE_CLUSTER_ID
      && (iasZoneClientState == IAS_ZONE_CLIENT_STATE_READ_ATTRIBUTES
          || iasZoneClientState == IAS_ZONE_CLIENT_STATE_READ_CIE_ADDRESS)) {
    int16u i = 0;
    while ((i + 3) <= bufLen) {  // 3 to insure we can read at least the attribute ID
                                 // and the status
      int16u attributeId = buffer[i] + (buffer[i + 1] << 8);
      int8u status = buffer[i + 2];
      i += 3;
      //emberAfIasZoneClusterPrintln("Parsing Attribute 0x%2X, Status: 0x%X", attributeId, status);
      if (status == EMBER_ZCL_STATUS_SUCCESS) {
        if ((i + 1) > bufLen) {
          // Too short, dump the message.
          return;
        }
        i++;  // skip the type of the attribute.  We already know what it should be.
        switch (attributeId) {
          case ZCL_ZONE_STATUS_ATTRIBUTE_ID:
            if ((i + 2) > bufLen) {
              // Too short, dump the message.
              return;
            }
            zoneStatus = (buffer[i] + (buffer[i + 1] << 8));
            setServerZoneStatus(currentIndex, zoneStatus);
            i += 2;
            break;
          case ZCL_ZONE_TYPE_ATTRIBUTE_ID:
            if ((i + 2) > bufLen) {
              // Too short, dump the message.
              return;
            }
            zoneType = (buffer[i] + (buffer[i + 1] << 8));
            setServerZoneType(currentIndex, zoneType);
            i += 2;
            break;
          case ZCL_ZONE_STATE_ATTRIBUTE_ID:
            if ((i + 1) > bufLen) {
              // Too short, dump the message
              return;
            }
            zoneState = buffer[i];
            setServerZoneState(currentIndex, zoneState);
            i++;
            break;
          case ZCL_IAS_CIE_ADDRESS_ATTRIBUTE_ID: {
            int8u myIeee[EUI64_SIZE];
            emberAfGetEui64(myIeee);
            if ((i + 8) > bufLen) {
              // Too short, dump the message
            } else if (0 != MEMCOMPARE(&(buffer[i]), myIeee, EUI64_SIZE)) {
              emberAfIasZoneClusterPrintln("CIE Address not set to mine, removing IAS zone server.");
              removeServer(&(buffer[i]));
              clearState();
            } else {
              readIasZoneServerAttributes(emberAfCurrentCommand()->source);
            }
            return;
          }
        }
      }
    }
    emberAfIasZoneClusterPrintln("Retrieved IAS Zone Server attributes from 0x%2X",
                                 emberAfCurrentCommand()->source);
    clearState();
  }
}
