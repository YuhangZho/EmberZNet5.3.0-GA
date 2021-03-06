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
  if (cmd->direction == ZCL_DIRECTION_SERVER_TO_CLIENT
      && emberAfContainsClient(cmd->apsFrame->destinationEndpoint,
                               cmd->apsFrame->clusterId)) {
    switch (cmd->apsFrame->clusterId) {
    case ZCL_ZLL_COMMISSIONING_CLUSTER_ID:
      return emberAfZllCommissioningClusterClientCommandParse(cmd);
    }
  } else if (cmd->direction == ZCL_DIRECTION_CLIENT_TO_SERVER
             && emberAfContainsServer(cmd->apsFrame->destinationEndpoint,
                                      cmd->apsFrame->clusterId)) {
    switch (cmd->apsFrame->clusterId) {
    case ZCL_IDENTIFY_CLUSTER_ID:
      return emberAfIdentifyClusterServerCommandParse(cmd);
    case ZCL_GROUPS_CLUSTER_ID:
      return emberAfGroupsClusterServerCommandParse(cmd);
    case ZCL_SCENES_CLUSTER_ID:
      return emberAfScenesClusterServerCommandParse(cmd);
    case ZCL_ON_OFF_CLUSTER_ID:
      return emberAfOnOffClusterServerCommandParse(cmd);
    case ZCL_LEVEL_CONTROL_CLUSTER_ID:
      return emberAfLevelControlClusterServerCommandParse(cmd);
    case ZCL_ZLL_COMMISSIONING_CLUSTER_ID:
      return emberAfZllCommissioningClusterServerCommandParse(cmd);
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
    case ZCL_TRIGGER_EFFECT_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int8u effectId;  // Ver.: always
        int8u effectVariant;  // Ver.: always
        // Command is fixed length: 2
        if (cmd->bufLen < payloadOffset + 2) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        effectId = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        effectVariant = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfIdentifyClusterTriggerEffectCallback(effectId,
                                                                 effectVariant);
        break;
      }
    }
  }
  return status(wasHandled, cmd->mfgSpecific);
}

// Cluster: Groups, server
EmberAfStatus emberAfGroupsClusterServerCommandParse(EmberAfClusterCommand *cmd)
{
  boolean wasHandled = FALSE;
  if (!cmd->mfgSpecific) {
    switch (cmd->commandId) {
    case ZCL_ADD_GROUP_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int16u groupId;  // Ver.: always
        int8u* groupName;  // Ver.: always
        // Command is not a fixed length
        if (cmd->bufLen < payloadOffset + 2) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        groupId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        if (cmd->bufLen < payloadOffset + emberAfStringLength(cmd->buffer + payloadOffset) + 1) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        groupName = emberAfGetString(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfGroupsClusterAddGroupCallback(groupId,
                                                          groupName);
        break;
      }
    case ZCL_VIEW_GROUP_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int16u groupId;  // Ver.: always
        // Command is fixed length: 2
        if (cmd->bufLen < payloadOffset + 2) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        groupId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfGroupsClusterViewGroupCallback(groupId);
        break;
      }
    case ZCL_GET_GROUP_MEMBERSHIP_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int8u groupCount;  // Ver.: always
        int8u* groupList;  // Ver.: always
        // Command is fixed length: 1
        if (cmd->bufLen < payloadOffset + 1) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        groupCount = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        groupList = cmd->buffer + payloadOffset;
        wasHandled = emberAfGroupsClusterGetGroupMembershipCallback(groupCount,
                                                                    groupList);
        break;
      }
    case ZCL_REMOVE_GROUP_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int16u groupId;  // Ver.: always
        // Command is fixed length: 2
        if (cmd->bufLen < payloadOffset + 2) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        groupId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfGroupsClusterRemoveGroupCallback(groupId);
        break;
      }
    case ZCL_REMOVE_ALL_GROUPS_COMMAND_ID:
      {
        // Command is fixed length: 0
        wasHandled = emberAfGroupsClusterRemoveAllGroupsCallback();
        break;
      }
    case ZCL_ADD_GROUP_IF_IDENTIFYING_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int16u groupId;  // Ver.: always
        int8u* groupName;  // Ver.: always
        // Command is not a fixed length
        if (cmd->bufLen < payloadOffset + 2) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        groupId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        if (cmd->bufLen < payloadOffset + emberAfStringLength(cmd->buffer + payloadOffset) + 1) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        groupName = emberAfGetString(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfGroupsClusterAddGroupIfIdentifyingCallback(groupId,
                                                                       groupName);
        break;
      }
    }
  }
  return status(wasHandled, cmd->mfgSpecific);
}

// Cluster: Scenes, server
EmberAfStatus emberAfScenesClusterServerCommandParse(EmberAfClusterCommand *cmd)
{
  boolean wasHandled = FALSE;
  if (!cmd->mfgSpecific) {
    switch (cmd->commandId) {
    case ZCL_ADD_SCENE_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int16u groupId;  // Ver.: always
        int8u sceneId;  // Ver.: always
        int16u transitionTime;  // Ver.: always
        int8u* sceneName;  // Ver.: always
        int8u* extensionFieldSets;  // Ver.: always
        // Command is not a fixed length
        if (cmd->bufLen < payloadOffset + 2) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        groupId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        if (cmd->bufLen < payloadOffset + 1) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        sceneId = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        if (cmd->bufLen < payloadOffset + 2) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        transitionTime = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        if (cmd->bufLen < payloadOffset + emberAfStringLength(cmd->buffer + payloadOffset) + 1) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        sceneName = emberAfGetString(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += emberAfStringLength(cmd->buffer + payloadOffset) + 1;
        extensionFieldSets = cmd->buffer + payloadOffset;
        wasHandled = emberAfScenesClusterAddSceneCallback(groupId,
                                                          sceneId,
                                                          transitionTime,
                                                          sceneName,
                                                          extensionFieldSets);
        break;
      }
    case ZCL_VIEW_SCENE_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int16u groupId;  // Ver.: always
        int8u sceneId;  // Ver.: always
        // Command is fixed length: 3
        if (cmd->bufLen < payloadOffset + 3) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        groupId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        sceneId = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfScenesClusterViewSceneCallback(groupId,
                                                           sceneId);
        break;
      }
    case ZCL_REMOVE_SCENE_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int16u groupId;  // Ver.: always
        int8u sceneId;  // Ver.: always
        // Command is fixed length: 3
        if (cmd->bufLen < payloadOffset + 3) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        groupId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        sceneId = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfScenesClusterRemoveSceneCallback(groupId,
                                                             sceneId);
        break;
      }
    case ZCL_REMOVE_ALL_SCENES_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int16u groupId;  // Ver.: always
        // Command is fixed length: 2
        if (cmd->bufLen < payloadOffset + 2) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        groupId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfScenesClusterRemoveAllScenesCallback(groupId);
        break;
      }
    case ZCL_STORE_SCENE_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int16u groupId;  // Ver.: always
        int8u sceneId;  // Ver.: always
        // Command is fixed length: 3
        if (cmd->bufLen < payloadOffset + 3) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        groupId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        sceneId = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfScenesClusterStoreSceneCallback(groupId,
                                                            sceneId);
        break;
      }
    case ZCL_RECALL_SCENE_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int16u groupId;  // Ver.: always
        int8u sceneId;  // Ver.: always
        // Command is fixed length: 3
        if (cmd->bufLen < payloadOffset + 3) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        groupId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        sceneId = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfScenesClusterRecallSceneCallback(groupId,
                                                             sceneId);
        break;
      }
    case ZCL_GET_SCENE_MEMBERSHIP_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int16u groupId;  // Ver.: always
        // Command is fixed length: 2
        if (cmd->bufLen < payloadOffset + 2) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        groupId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfScenesClusterGetSceneMembershipCallback(groupId);
        break;
      }
    case ZCL_ENHANCED_ADD_SCENE_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int16u groupId;  // Ver.: always
        int8u sceneId;  // Ver.: always
        int16u transitionTime;  // Ver.: always
        int8u* sceneName;  // Ver.: always
        int8u* extensionFieldSets;  // Ver.: always
        // Command is not a fixed length
        if (cmd->bufLen < payloadOffset + 2) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        groupId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        if (cmd->bufLen < payloadOffset + 1) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        sceneId = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        if (cmd->bufLen < payloadOffset + 2) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        transitionTime = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        if (cmd->bufLen < payloadOffset + emberAfStringLength(cmd->buffer + payloadOffset) + 1) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        sceneName = emberAfGetString(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += emberAfStringLength(cmd->buffer + payloadOffset) + 1;
        extensionFieldSets = cmd->buffer + payloadOffset;
        wasHandled = emberAfScenesClusterEnhancedAddSceneCallback(groupId,
                                                                  sceneId,
                                                                  transitionTime,
                                                                  sceneName,
                                                                  extensionFieldSets);
        break;
      }
    case ZCL_ENHANCED_VIEW_SCENE_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int16u groupId;  // Ver.: always
        int8u sceneId;  // Ver.: always
        // Command is fixed length: 3
        if (cmd->bufLen < payloadOffset + 3) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        groupId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        sceneId = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfScenesClusterEnhancedViewSceneCallback(groupId,
                                                                   sceneId);
        break;
      }
    case ZCL_COPY_SCENE_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int8u mode;  // Ver.: always
        int16u groupIdFrom;  // Ver.: always
        int8u sceneIdFrom;  // Ver.: always
        int16u groupIdTo;  // Ver.: always
        int8u sceneIdTo;  // Ver.: always
        // Command is fixed length: 7
        if (cmd->bufLen < payloadOffset + 7) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        mode = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        groupIdFrom = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        sceneIdFrom = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        groupIdTo = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        sceneIdTo = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfScenesClusterCopySceneCallback(mode,
                                                           groupIdFrom,
                                                           sceneIdFrom,
                                                           groupIdTo,
                                                           sceneIdTo);
        break;
      }
    }
  }
  return status(wasHandled, cmd->mfgSpecific);
}

// Cluster: On/off, server
EmberAfStatus emberAfOnOffClusterServerCommandParse(EmberAfClusterCommand *cmd)
{
  boolean wasHandled = FALSE;
  if (!cmd->mfgSpecific) {
    switch (cmd->commandId) {
    case ZCL_OFF_COMMAND_ID:
      {
        // Command is fixed length: 0
        wasHandled = emberAfOnOffClusterOffCallback();
        break;
      }
    case ZCL_ON_COMMAND_ID:
      {
        // Command is fixed length: 0
        wasHandled = emberAfOnOffClusterOnCallback();
        break;
      }
    case ZCL_TOGGLE_COMMAND_ID:
      {
        // Command is fixed length: 0
        wasHandled = emberAfOnOffClusterToggleCallback();
        break;
      }
    case ZCL_OFF_WITH_EFFECT_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int8u effectId;  // Ver.: always
        int8u effectVariant;  // Ver.: always
        // Command is fixed length: 2
        if (cmd->bufLen < payloadOffset + 2) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        effectId = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        effectVariant = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfOnOffClusterOffWithEffectCallback(effectId,
                                                              effectVariant);
        break;
      }
    case ZCL_ON_WITH_RECALL_GLOBAL_SCENE_COMMAND_ID:
      {
        // Command is fixed length: 0
        wasHandled = emberAfOnOffClusterOnWithRecallGlobalSceneCallback();
        break;
      }
    case ZCL_ON_WITH_TIMED_OFF_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int8u onOffControl;  // Ver.: always
        int16u onTime;  // Ver.: always
        int16u offWaitTime;  // Ver.: always
        // Command is fixed length: 5
        if (cmd->bufLen < payloadOffset + 5) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        onOffControl = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        onTime = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        offWaitTime = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfOnOffClusterOnWithTimedOffCallback(onOffControl,
                                                               onTime,
                                                               offWaitTime);
        break;
      }
    }
  }
  return status(wasHandled, cmd->mfgSpecific);
}

// Cluster: Level Control, server
EmberAfStatus emberAfLevelControlClusterServerCommandParse(EmberAfClusterCommand *cmd)
{
  boolean wasHandled = FALSE;
  if (!cmd->mfgSpecific) {
    switch (cmd->commandId) {
    case ZCL_MOVE_TO_LEVEL_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int8u level;  // Ver.: always
        int16u transitionTime;  // Ver.: always
        // Command is fixed length: 3
        if (cmd->bufLen < payloadOffset + 3) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        level = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        transitionTime = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfLevelControlClusterMoveToLevelCallback(level,
                                                                   transitionTime);
        break;
      }
    case ZCL_MOVE_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int8u moveMode;  // Ver.: always
        int8u rate;  // Ver.: always
        // Command is fixed length: 2
        if (cmd->bufLen < payloadOffset + 2) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        moveMode = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        rate = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfLevelControlClusterMoveCallback(moveMode,
                                                            rate);
        break;
      }
    case ZCL_STEP_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int8u stepMode;  // Ver.: always
        int8u stepSize;  // Ver.: always
        int16u transitionTime;  // Ver.: always
        // Command is fixed length: 4
        if (cmd->bufLen < payloadOffset + 4) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        stepMode = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        stepSize = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        transitionTime = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfLevelControlClusterStepCallback(stepMode,
                                                            stepSize,
                                                            transitionTime);
        break;
      }
    case ZCL_STOP_COMMAND_ID:
      {
        // Command is fixed length: 0
        wasHandled = emberAfLevelControlClusterStopCallback();
        break;
      }
    case ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int8u level;  // Ver.: always
        int16u transitionTime;  // Ver.: always
        // Command is fixed length: 3
        if (cmd->bufLen < payloadOffset + 3) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        level = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        transitionTime = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfLevelControlClusterMoveToLevelWithOnOffCallback(level,
                                                                            transitionTime);
        break;
      }
    case ZCL_MOVE_WITH_ON_OFF_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int8u moveMode;  // Ver.: always
        int8u rate;  // Ver.: always
        // Command is fixed length: 2
        if (cmd->bufLen < payloadOffset + 2) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        moveMode = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        rate = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfLevelControlClusterMoveWithOnOffCallback(moveMode,
                                                                     rate);
        break;
      }
    case ZCL_STEP_WITH_ON_OFF_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int8u stepMode;  // Ver.: always
        int8u stepSize;  // Ver.: always
        int16u transitionTime;  // Ver.: always
        // Command is fixed length: 4
        if (cmd->bufLen < payloadOffset + 4) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        stepMode = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        stepSize = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        transitionTime = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfLevelControlClusterStepWithOnOffCallback(stepMode,
                                                                     stepSize,
                                                                     transitionTime);
        break;
      }
    case ZCL_STOP_WITH_ON_OFF_COMMAND_ID:
      {
        // Command is fixed length: 0
        wasHandled = emberAfLevelControlClusterStopWithOnOffCallback();
        break;
      }
    }
  }
  return status(wasHandled, cmd->mfgSpecific);
}

// Cluster: ZLL Commissioning, client
EmberAfStatus emberAfZllCommissioningClusterClientCommandParse(EmberAfClusterCommand *cmd)
{
  boolean wasHandled = FALSE;
  if (!cmd->mfgSpecific) {
    switch (cmd->commandId) {
    case ZCL_SCAN_RESPONSE_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int32u transaction;  // Ver.: always
        int8u rssiCorrection;  // Ver.: always
        int8u zigbeeInformation;  // Ver.: always
        int8u zllInformation;  // Ver.: always
        int16u keyBitmask;  // Ver.: always
        int32u responseId;  // Ver.: always
        int8u* extendedPanId;  // Ver.: always
        int8u networkUpdateId;  // Ver.: always
        int8u logicalChannel;  // Ver.: always
        int16u panId;  // Ver.: always
        int16u networkAddress;  // Ver.: always
        int8u numberOfSubDevices;  // Ver.: always
        int8u totalGroupIds;  // Ver.: always
        int8u endpointId;  // Ver.: always
        int16u profileId;  // Ver.: always
        int16u deviceId;  // Ver.: always
        int8u version;  // Ver.: always
        int8u groupIdCount;  // Ver.: always
        // Command is fixed length: 36
        if (cmd->bufLen < payloadOffset + 36) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        transaction = emberAfGetInt32u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 4;
        rssiCorrection = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        zigbeeInformation = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        zllInformation = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        keyBitmask = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        responseId = emberAfGetInt32u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 4;
        extendedPanId = cmd->buffer + payloadOffset;
        payloadOffset += 8;
        networkUpdateId = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        logicalChannel = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        panId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        networkAddress = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        numberOfSubDevices = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        totalGroupIds = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        endpointId = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        profileId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        deviceId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        version = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        groupIdCount = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfZllCommissioningClusterScanResponseCallback(transaction,
                                                                        rssiCorrection,
                                                                        zigbeeInformation,
                                                                        zllInformation,
                                                                        keyBitmask,
                                                                        responseId,
                                                                        extendedPanId,
                                                                        networkUpdateId,
                                                                        logicalChannel,
                                                                        panId,
                                                                        networkAddress,
                                                                        numberOfSubDevices,
                                                                        totalGroupIds,
                                                                        endpointId,
                                                                        profileId,
                                                                        deviceId,
                                                                        version,
                                                                        groupIdCount);
        break;
      }
    case ZCL_DEVICE_INFORMATION_RESPONSE_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int32u transaction;  // Ver.: always
        int8u numberOfSubDevices;  // Ver.: always
        int8u startIndex;  // Ver.: always
        int8u deviceInformationRecordCount;  // Ver.: always
        int8u* deviceInformationRecordList;  // Ver.: always
        // Command is fixed length: 7
        if (cmd->bufLen < payloadOffset + 7) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        transaction = emberAfGetInt32u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 4;
        numberOfSubDevices = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        startIndex = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        deviceInformationRecordCount = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        deviceInformationRecordList = cmd->buffer + payloadOffset;
        wasHandled = emberAfZllCommissioningClusterDeviceInformationResponseCallback(transaction,
                                                                                     numberOfSubDevices,
                                                                                     startIndex,
                                                                                     deviceInformationRecordCount,
                                                                                     deviceInformationRecordList);
        break;
      }
    case ZCL_NETWORK_START_RESPONSE_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int32u transaction;  // Ver.: always
        int8u status;  // Ver.: always
        int8u* extendedPanId;  // Ver.: always
        int8u networkUpdateId;  // Ver.: always
        int8u logicalChannel;  // Ver.: always
        int16u panId;  // Ver.: always
        // Command is fixed length: 17
        if (cmd->bufLen < payloadOffset + 17) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        transaction = emberAfGetInt32u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 4;
        status = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        extendedPanId = cmd->buffer + payloadOffset;
        payloadOffset += 8;
        networkUpdateId = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        logicalChannel = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        panId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfZllCommissioningClusterNetworkStartResponseCallback(transaction,
                                                                                status,
                                                                                extendedPanId,
                                                                                networkUpdateId,
                                                                                logicalChannel,
                                                                                panId);
        break;
      }
    case ZCL_NETWORK_JOIN_ROUTER_RESPONSE_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int32u transaction;  // Ver.: always
        int8u status;  // Ver.: always
        // Command is fixed length: 5
        if (cmd->bufLen < payloadOffset + 5) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        transaction = emberAfGetInt32u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 4;
        status = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfZllCommissioningClusterNetworkJoinRouterResponseCallback(transaction,
                                                                                     status);
        break;
      }
    case ZCL_NETWORK_JOIN_END_DEVICE_RESPONSE_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int32u transaction;  // Ver.: always
        int8u status;  // Ver.: always
        // Command is fixed length: 5
        if (cmd->bufLen < payloadOffset + 5) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        transaction = emberAfGetInt32u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 4;
        status = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfZllCommissioningClusterNetworkJoinEndDeviceResponseCallback(transaction,
                                                                                        status);
        break;
      }
    case ZCL_ENDPOINT_INFORMATION_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int8u* ieeeAddress;  // Ver.: always
        int16u networkAddress;  // Ver.: always
        int8u endpointId;  // Ver.: always
        int16u profileId;  // Ver.: always
        int16u deviceId;  // Ver.: always
        int8u version;  // Ver.: always
        // Command is fixed length: 16
        if (cmd->bufLen < payloadOffset + 16) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        ieeeAddress = cmd->buffer + payloadOffset;
        payloadOffset += 8;
        networkAddress = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        endpointId = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        profileId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        deviceId = emberAfGetInt16u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 2;
        version = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfZllCommissioningClusterEndpointInformationCallback(ieeeAddress,
                                                                               networkAddress,
                                                                               endpointId,
                                                                               profileId,
                                                                               deviceId,
                                                                               version);
        break;
      }
    case ZCL_GET_GROUP_IDENTIFIERS_RESPONSE_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int8u total;  // Ver.: always
        int8u startIndex;  // Ver.: always
        int8u count;  // Ver.: always
        int8u* groupInformationRecordList;  // Ver.: always
        // Command is fixed length: 3
        if (cmd->bufLen < payloadOffset + 3) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        total = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        startIndex = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        count = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        groupInformationRecordList = cmd->buffer + payloadOffset;
        wasHandled = emberAfZllCommissioningClusterGetGroupIdentifiersResponseCallback(total,
                                                                                       startIndex,
                                                                                       count,
                                                                                       groupInformationRecordList);
        break;
      }
    case ZCL_GET_ENDPOINT_LIST_RESPONSE_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int8u total;  // Ver.: always
        int8u startIndex;  // Ver.: always
        int8u count;  // Ver.: always
        int8u* endpointInformationRecordList;  // Ver.: always
        // Command is fixed length: 3
        if (cmd->bufLen < payloadOffset + 3) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        total = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        startIndex = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        count = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        payloadOffset += 1;
        endpointInformationRecordList = cmd->buffer + payloadOffset;
        wasHandled = emberAfZllCommissioningClusterGetEndpointListResponseCallback(total,
                                                                                   startIndex,
                                                                                   count,
                                                                                   endpointInformationRecordList);
        break;
      }
    }
  }
  return status(wasHandled, cmd->mfgSpecific);
}

// Cluster: ZLL Commissioning, server
EmberAfStatus emberAfZllCommissioningClusterServerCommandParse(EmberAfClusterCommand *cmd)
{
  boolean wasHandled = FALSE;
  if (!cmd->mfgSpecific) {
    switch (cmd->commandId) {
    case ZCL_GET_GROUP_IDENTIFIERS_REQUEST_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int8u startIndex;  // Ver.: always
        // Command is fixed length: 1
        if (cmd->bufLen < payloadOffset + 1) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        startIndex = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfZllCommissioningClusterGetGroupIdentifiersRequestCallback(startIndex);
        break;
      }
    case ZCL_GET_ENDPOINT_LIST_REQUEST_COMMAND_ID:
      {
        int16u payloadOffset = cmd->payloadStartIndex;
        int8u startIndex;  // Ver.: always
        // Command is fixed length: 1
        if (cmd->bufLen < payloadOffset + 1) return EMBER_ZCL_STATUS_MALFORMED_COMMAND;
        startIndex = emberAfGetInt8u(cmd->buffer, payloadOffset, cmd->bufLen);
        wasHandled = emberAfZllCommissioningClusterGetEndpointListRequestCallback(startIndex);
        break;
      }
    }
  }
  return status(wasHandled, cmd->mfgSpecific);
}
