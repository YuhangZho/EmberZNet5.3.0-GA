// This file is generated by Ember Desktop.  Please do not edit manually.
//
//

// This is a set of generated functions that parse the
// the incomming message, and call appropriate command handler.



#include PLATFORM_HEADER
#ifdef EZSP_HOST
// Includes needed for ember related functions for the EZSP host
#include "stack/include/error.h"
#include "stack/include/ember-types.h"
#include "app/util/ezsp/ezsp-protocol.h"
#include "app/util/ezsp/ezsp.h"
#include "app/util/ezsp/ezsp-utils.h"
#include "app/util/ezsp/serial-interface.h"
#else
// Includes needed for ember related functions for the EM250
#include "stack/include/ember.h"
#endif // EZSP_HOST

#include "app/framework/util/util.h"
#include "af-structs.h"
#include "call-command-handler.h"
#include "command-id.h"
#include "callback.h"

static EmberAfStatus status(boolean wasHandled, boolean mfgSpecific)
{
  if (wasHandled) {
    return EMBER_ZCL_STATUS_SUCCESS;
  } else if (mfgSpecific) {
    return EMBER_ZCL_STATUS_UNSUP_MANUF_CLUSTER_COMMAND;
  } else {
    return EMBER_ZCL_STATUS_UNSUP_CLUSTER_COMMAND;
  }
}

// Main command parsing controller.
EmberAfStatus emberAfClusterSpecificCommandParse(EmberAfClusterCommand *cmd)
{
  if (cmd->direction == ZCL_DIRECTION_CLIENT_TO_SERVER
      && emberAfContainsServer(cmd->apsFrame->destinationEndpoint,
                               cmd->apsFrame->clusterId)) {
    switch (cmd->apsFrame->clusterId) {
    case ZCL_IDENTIFY_CLUSTER_ID:
      return emberAfIdentifyClusterServerCommandParse(cmd);
    }
  }
  return status(FALSE, cmd->mfgSpecific);
}

// Cluster: Identify, server
EmberAfStatus emberAfIdentifyClusterServerCommandParse(EmberAfClusterCommand *cmd)
{
  boolean wasHandled = FALSE;
  if (!cmd->mfgSpecific) {
    switch (cmd->commandId) {
    case ZCL_IDENTIFY_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int16u identifyTime;  // Ver.: always
        // Command is fixed length: 2
        if (cmd->bufLen < payloadOffset + 2) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        identifyTime = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfIdentifyClusterIdentifyCallback(identifyTime);
        break;
      }
    case ZCL_IDENTIFY_QUERY_COMMAND_ID:
      {
        // Command is fixed length: 0
        wasHandled = emberAfIdentifyClusterIdentifyQueryCallback();
        break;
      }
    }
  }
  return status(wasHandled, cmd->mfgSpecific);
}
