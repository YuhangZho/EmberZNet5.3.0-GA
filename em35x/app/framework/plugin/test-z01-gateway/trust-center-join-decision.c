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


void NewDeviceJoinReport(EmberNodeId nodeId, EmberEUI64 nodeEUI64);


//------------------------------------------------------------------------------
// Forward Declaration

//------------------------------------------------------------------------------
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
      NewDeviceJoinReport(newNodeId, newNodeEui64);
    }
  }
}
