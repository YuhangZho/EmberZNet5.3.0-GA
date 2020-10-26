// *****************************************************************************
// * white-list.c
// *
// * Routines for white list management including adding and removing white list 
// * table entires.
// *
// *****************************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "app/util/zigbee-framework/zigbee-device-common.h"


boolean emberAfPluginTrustCenterJoinDecisionMakeJoinDecisionCallback(EmberNodeId nodeId,
                                                                     EmberEUI64 nodeEUI64);


//------------------------------------------------------------------------------
// Forward Declaration

EmberEventControl emberAfPluginTrustCenterJoinDecisionDecideEventControl;
static EmberNodeId nodeIdBuffer;
static EmberEUI64 nodeEui64Buffer;

#define TCDecide emberAfPluginTrustCenterJoinDecisionDecideEventControl
//------------------------------------------------------------------------------

void emberAfPluginTrustCenterJoinDecisionDecideEventHandler(void)
{
  emberEventControlSetInactive(TCDecide);
  emberAfCorePrintln("Removing Device");  
  EmberStatus statusLR = emberLeaveRequest(nodeIdBuffer,
                                           nodeEui64Buffer,
                                           EMBER_ZIGBEE_LEAVE_AND_REMOVE_CHILDREN,
                                           EMBER_APS_OPTION_NONE);  
         
  emberAfCorePrintln("emberLeaveRequest Status: 0x%x", statusLR);
  emberAfCorePrintln("");
}

/** @brief Trust Center Join
 *
 * This callback is called from within the application framework's
 * implementation of emberTrustCenterJoinHandler or
 * ezspTrustCenterJoinHandler. This callback provides the same arguments
 * passed to the TrustCenterJoinHandler. For more information about the
 * TrustCenterJoinHandler please see documentation included in
 * stack/include/trust-center.h.
 *
 * @param newNodeId   Ver.: always
 * @param newNodeEui64   Ver.: always
 * @param parentOfNewNode   Ver.: always
 * @param status   Ver.: always
 * @param decision   Ver.: always
 */
void emberAfTrustCenterJoinCallback(EmberNodeId newNodeId,
                                    EmberEUI64 newNodeEui64,
                                    EmberNodeId parentOfNewNode,
                                    EmberDeviceUpdate status,
                                    EmberJoinDecision decision)
{
  if(status != EMBER_DEVICE_LEFT)
  {
    if (decision == EMBER_USE_PRECONFIGURED_KEY || decision == EMBER_SEND_KEY_IN_THE_CLEAR)
    {
      if (!emberAfPluginTrustCenterJoinDecisionMakeJoinDecisionCallback(newNodeId, newNodeEui64))
      {
        emberAfCorePrintln("Setting Event delay 1 QS");   
        nodeIdBuffer = newNodeId;
        MEMCOPY(nodeEui64Buffer, newNodeEui64, EUI64_SIZE);
        emberEventControlSetDelayQS(TCDecide, 1);
      }   
    }
  }
}
