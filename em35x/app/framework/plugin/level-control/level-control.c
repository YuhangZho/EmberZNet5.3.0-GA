// *******************************************************************
// * level-control.c
// *
// *
// * Copyright 2014 Silicon Laboratories, Inc.                              *80*
// *******************************************************************

// this file contains all the common includes for clusters in the util
#include "app/framework/include/af.h"

// clusters specific header
#include "level-control.h"

#ifdef EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER
  #include "app/framework/plugin/zll-level-control-server/zll-level-control-server.h"
#endif //EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER

#if (EMBER_AF_PLUGIN_LEVEL_CONTROL_RATE == 0)
  #define FASTEST_TRANSITION_TIME_MS 0
#else
  #define FASTEST_TRANSITION_TIME_MS (MILLISECOND_TICKS_PER_SECOND / EMBER_AF_PLUGIN_LEVEL_CONTROL_RATE)
#endif

#ifdef EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER
  #define MIN_LEVEL EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER_MINIMUM_LEVEL
  #define MAX_LEVEL EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER_MAXIMUM_LEVEL
#else
  #define MIN_LEVEL EMBER_AF_PLUGIN_LEVEL_CONTROL_MINIMUM_LEVEL
  #define MAX_LEVEL EMBER_AF_PLUGIN_LEVEL_CONTROL_MAXIMUM_LEVEL
#endif

#define INVALID_STORED_LEVEL 0xFF

typedef struct {
  int8u commandId;
  int8u moveToLevel;
  boolean increasing;
  boolean useOnLevel;
  int8u onLevel;
  int8u storedLevel;
  int32u eventDurationMs;
  int32u transitionTimeMs;
  int32u elapsedTimeMs;
} EmberAfLevelControlState;

static EmberAfLevelControlState stateTable[EMBER_AF_LEVEL_CONTROL_CLUSTER_SERVER_ENDPOINT_COUNT];

static EmberAfLevelControlState *getState(int8u endpoint);

static void moveToLevelHandler(int8u commandId,
                               int8u level,
                               int16u transitionTimeDs,
                               int8u storedLevel);
static void moveHandler(int8u commandId, int8u moveMode, int8u rate);
static void stepHandler(int8u commandId,
                        int8u stepMode,
                        int8u stepSize,
                        int16u transitionTimeDs);
static void stopHandler(int8u commandId);

static void setOnOffValue(int8u endpoint, boolean onOff);
static void writeRemainingTime(int8u endpoint, int16u remainingTimeMs);

static void schedule(int8u endpoint, int32u delayMs)
{
  emberAfScheduleServerTickExtended(endpoint,
                                    ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                    delayMs,
                                    EMBER_AF_LONG_POLL,
                                    EMBER_AF_OK_TO_SLEEP);
}

static void deactivate(int8u endpoint)
{
  emberAfDeactivateServerTick(endpoint, ZCL_LEVEL_CONTROL_CLUSTER_ID);
}

static EmberAfLevelControlState *getState(int8u endpoint)
{
  int8u ep = emberAfFindClusterServerEndpointIndex(endpoint,
                                                   ZCL_LEVEL_CONTROL_CLUSTER_ID);
  return (ep == 0xFF ? NULL : &stateTable[ep]);
}

void emberAfLevelControlClusterServerInitCallback(int8u endpoint)
{
}

void emberAfLevelControlClusterServerTickCallback(int8u endpoint)
{
  EmberAfLevelControlState *state = getState(endpoint);
  EmberAfStatus status;
  int8u currentLevel;
  boolean currentOnLevel, OnorOff;        // add by Robin 
  
  if (state == NULL) {
    return;
  }

  state->elapsedTimeMs += state->eventDurationMs;

#ifdef EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER
  if (emberAfPluginZllLevelControlServerIgnoreMoveToLevelMoveStepStop(endpoint,
                                                                      state->commandId)) {
    return;
  }
#endif

  // Read the attribute; print error message and return if it can't be read
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                      ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                      (int8u *)&currentLevel,
                                      sizeof(currentLevel));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);
    writeRemainingTime(endpoint, 0);
    return;
  }

  emberAfLevelControlClusterPrint("Event: move from %d", currentLevel);

  // adjust by the proper amount, either up or down
#if 0  
  if (state->increasing) {
    assert(currentLevel < MAX_LEVEL);
    assert(currentLevel < state->moveToLevel);
    currentLevel++;
  } else {
    assert(MIN_LEVEL < currentLevel);
    assert(state->moveToLevel < currentLevel);
    currentLevel--;
  }
#else  //sengled 2015.9.7 wms
  if (state->increasing) {
    if (currentLevel > MAX_LEVEL)
      currentLevel = MAX_LEVEL;
    if (currentLevel < state->moveToLevel)
      currentLevel++;
  } else {
    if (MIN_LEVEL > currentLevel)
      currentLevel = MIN_LEVEL;
    if (state->moveToLevel < currentLevel)
      currentLevel--;
  }
#endif

  emberAfLevelControlClusterPrint(" to %d ", currentLevel);
  emberAfLevelControlClusterPrintln("(diff %c1)",
                                    state->increasing ? '+' : '-');

  status = emberAfWriteServerAttribute(endpoint,
                                       ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                       ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                       (int8u *)&currentLevel,
                                       ZCL_INT8U_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("ERR: writing current level %x", status);
    writeRemainingTime(endpoint, 0);
    return;
  }

  // The level has changed, so the scene is no longer valid.
  if (emberAfContainsServer(endpoint, ZCL_SCENES_CLUSTER_ID)) {
    emberAfScenesClusterMakeInvalidCallback(endpoint);
  }

  // Are we at the requested level?
  if (currentLevel == state->moveToLevel) {
    if (state->commandId == ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID
        || state->commandId == ZCL_MOVE_WITH_ON_OFF_COMMAND_ID
        || state->commandId == ZCL_STEP_WITH_ON_OFF_COMMAND_ID) {
      setOnOffValue(endpoint, (currentLevel != MIN_LEVEL));
      if (currentLevel == MIN_LEVEL && state->useOnLevel) {
        status = emberAfWriteServerAttribute(endpoint,
                                             ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                             ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                             (int8u *)&state->onLevel,
                                             ZCL_INT8U_ATTRIBUTE_TYPE);
        if (status != EMBER_ZCL_STATUS_SUCCESS) {
          emberAfLevelControlClusterPrintln("ERR: writing current level %x",
                                            status);
        }
      }
    } else {
    #ifdef EMBER_ORIGINAL_CODE
      if (state->storedLevel != INVALID_STORED_LEVEL) {
        status = emberAfWriteServerAttribute(endpoint,
                                             ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                             ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                             (int8u *)&state->storedLevel,
                                             ZCL_INT8U_ATTRIBUTE_TYPE);
        if (status != EMBER_ZCL_STATUS_SUCCESS) {
          emberAfLevelControlClusterPrintln("ERR: writing current level %x",
                                          status);
        }
      }
    #else
      status = emberAfReadServerAttribute(endpoint,
                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                      ZCL_ON_LEVEL_ATTRIBUTE_ID,
                                      (int8u *)&currentOnLevel,
                                      sizeof(currentOnLevel));      
      if (status != EMBER_ZCL_STATUS_SUCCESS) {
        emberAfOnOffClusterPrintln("ERR: Read onLevel to judge whether store %x", status);
      }
      
      status = emberAfReadAttribute(endpoint,
                                    ZCL_ON_OFF_CLUSTER_ID,
                                    ZCL_ON_OFF_ATTRIBUTE_ID,
                                    CLUSTER_MASK_SERVER,
                                    (int8u *)&OnorOff,
                                    sizeof(OnorOff),
                                    NULL); // data type


      if (status != EMBER_ZCL_STATUS_SUCCESS) {
        emberAfOnOffClusterPrintln("ERR: Read onoff to judge whether store %x", status);
      }

      if((0xff == currentOnLevel) && (0x00 == OnorOff)) 
      {
        status = emberAfWriteServerAttribute(endpoint,
                                            ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                            ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                            (int8u *)&state->storedLevel,
                                            ZCL_INT8U_ATTRIBUTE_TYPE);
        if (status != EMBER_ZCL_STATUS_SUCCESS) {
          emberAfLevelControlClusterPrintln("ERR: writing current level %x", status);
        }
      }
      else
      {
        if (state->storedLevel != INVALID_STORED_LEVEL) {
          status = emberAfWriteServerAttribute(endpoint,
                                              ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                              ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                              (int8u *)&state->storedLevel,
                                              ZCL_INT8U_ATTRIBUTE_TYPE);
          if (status != EMBER_ZCL_STATUS_SUCCESS) {
            emberAfLevelControlClusterPrintln("ERR: writing current level %x", status);
          }
        }
      }
    #endif
    }
    writeRemainingTime(endpoint, 0);
  }
  else {  
    writeRemainingTime(endpoint,
                       state->transitionTimeMs - state->elapsedTimeMs);
    schedule(endpoint, state->eventDurationMs);
  }
}

static void writeRemainingTime(int8u endpoint, int16u remainingTimeMs)
{
#ifdef ZCL_USING_LEVEL_CONTROL_CLUSTER_LEVEL_CONTROL_REMAINING_TIME_ATTRIBUTE
  int16u remainingTimeDs = remainingTimeMs / 100;
  EmberAfStatus status = emberAfWriteServerAttribute(endpoint,
                                                   ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                                   ZCL_LEVEL_CONTROL_REMAINING_TIME_ATTRIBUTE_ID,
                                                   (int8u *)&remainingTimeDs,
                                                   sizeof(remainingTimeDs));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("ERR: writing remaining time %x", status);
  }
#endif
}

static void setOnOffValue(int8u endpoint, boolean onOff)
{
  if (emberAfContainsServer(endpoint, ZCL_ON_OFF_CLUSTER_ID)) {
    emberAfLevelControlClusterPrintln("Setting on/off to %p due to level change",
                                      onOff ? "ON" : "OFF");
    emberAfOnOffClusterSetValueCallback(endpoint,
                         (onOff ? ZCL_ON_COMMAND_ID : ZCL_OFF_COMMAND_ID),
                         TRUE);
  }
}

boolean emberAfLevelControlClusterMoveToLevelCallback(int8u level,
                                                      int16u transitionTime)
{
  emberAfLevelControlClusterPrintln("%pMOVE_TO_LEVEL %x %2x",
                                    "RX level-control:",
                                    level,
                                    transitionTime);
  moveToLevelHandler(ZCL_MOVE_TO_LEVEL_COMMAND_ID,
                     level,
                     transitionTime,
                     INVALID_STORED_LEVEL); // Don't revert to the stored level
  return TRUE;
}

boolean emberAfLevelControlClusterMoveToLevelWithOnOffCallback(int8u level,
                                                               int16u transitionTime)
{
  emberAfLevelControlClusterPrintln("%pMOVE_TO_LEVEL_WITH_ON_OFF %x %2x",
                                    "RX level-control:",
                                    level,
                                    transitionTime);
  moveToLevelHandler(ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID,
                     level,
                     transitionTime,
                     INVALID_STORED_LEVEL); // Don't revert to the stored level
  return TRUE;
}

boolean emberAfLevelControlClusterMoveCallback(int8u moveMode, int8u rate)
{
  emberAfLevelControlClusterPrintln("%pMOVE %x %x",
                                    "RX level-control:",
                                    moveMode,
                                    rate);
  moveHandler(ZCL_MOVE_COMMAND_ID, moveMode, rate);
  return TRUE;
}

boolean emberAfLevelControlClusterMoveWithOnOffCallback(int8u moveMode, int8u rate)
{
  emberAfLevelControlClusterPrintln("%pMOVE_WITH_ON_OFF %x %x",
                                    "RX level-control:",
                                    moveMode,
                                    rate);
  moveHandler(ZCL_MOVE_WITH_ON_OFF_COMMAND_ID, moveMode, rate);
  return TRUE;
}

boolean emberAfLevelControlClusterStepCallback(int8u stepMode,
                                               int8u stepSize,
                                               int16u transitionTime)
{
  emberAfLevelControlClusterPrintln("%pSTEP %x %x %2x",
                                    "RX level-control:",
                                    stepMode,
                                    stepSize,
                                    transitionTime);
  stepHandler(ZCL_STEP_COMMAND_ID, stepMode, stepSize, transitionTime);
  return TRUE;
}

boolean emberAfLevelControlClusterStepWithOnOffCallback(int8u stepMode,
                                                        int8u stepSize,
                                                        int16u transitionTime)
{
  emberAfLevelControlClusterPrintln("%pSTEP_WITH_ON_OFF %x %x %2x",
                                    "RX level-control:",
                                    stepMode,
                                    stepSize,
                                    transitionTime);
  stepHandler(ZCL_STEP_WITH_ON_OFF_COMMAND_ID,
              stepMode,
              stepSize,
              transitionTime);
  return TRUE;
}

boolean emberAfLevelControlClusterStopCallback(void)
{
  emberAfLevelControlClusterPrintln("%pSTOP", "RX level-control:");
  stopHandler(ZCL_STOP_COMMAND_ID);
  return TRUE;
}

boolean emberAfLevelControlClusterStopWithOnOffCallback(void)
{
  emberAfLevelControlClusterPrintln("%pSTOP_WITH_ON_OFF", "RX level-control:");
  stopHandler(ZCL_STOP_WITH_ON_OFF_COMMAND_ID);
  return TRUE;
}

static void moveToLevelHandler(int8u commandId,
                               int8u level,
                               int16u transitionTimeDs,
                               int8u storedLevel)
{
  int8u endpoint = emberAfCurrentEndpoint();
  EmberAfLevelControlState *state = getState(endpoint);
  EmberAfStatus status;
  int8u currentLevel;
  int8u actualStepSize;

  if (state == NULL) {
    status = EMBER_ZCL_STATUS_FAILURE;
    goto send_default_response;
  }

  // Cancel any currently active command before fiddling with the state.
  deactivate(endpoint);

  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                      ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                      (int8u *)&currentLevel,
                                      sizeof(currentLevel));

  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);
    goto send_default_response;
  }

  state->commandId = commandId;

  // Move To Level commands cause the device to move from its current level to
  // the specified level at the specified rate.
  if (MAX_LEVEL < level) {
    state->moveToLevel = MAX_LEVEL;
  } else if (level < MIN_LEVEL) {
    state->moveToLevel = MIN_LEVEL;
  } else {
    state->moveToLevel = level;
  }

  // If the level is decreasing, the On/Off attribute is left unchanged.  This
  // logic is to prevent a light from transitioning from off to bright to dim.
  // Instead, a light that is off will stay off until the target level is
  // reached.
  if (currentLevel <= state->moveToLevel) {
    if (commandId == ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID) {
      setOnOffValue(endpoint, (state->moveToLevel != MIN_LEVEL));
    }
    if (currentLevel == state->moveToLevel) {
      status = EMBER_ZCL_STATUS_SUCCESS;
      goto send_default_response;
    }
    state->increasing = TRUE;
    actualStepSize = state->moveToLevel - currentLevel;
  } else {
    state->increasing = FALSE;
    actualStepSize = currentLevel - state->moveToLevel;
  }

  // If the Transition time field takes the value 0xFFFF, then the time taken
  // to move to the new level shall instead be determined by the On/Off
  // Transition Time attribute.  If On/Off Transition Time, which is an
  // optional attribute, is not present, the device shall move to its new level
  // as fast as it is able.
  if (transitionTimeDs == 0xFFFF) {
#ifdef ZCL_USING_LEVEL_CONTROL_CLUSTER_ON_OFF_TRANSITION_TIME_ATTRIBUTE
    status = emberAfReadServerAttribute(endpoint,
                                        ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                        ZCL_ON_OFF_TRANSITION_TIME_ATTRIBUTE_ID,
                                        (int8u *)&transitionTimeDs,
                                        sizeof(transitionTimeDs));
    if (status != EMBER_ZCL_STATUS_SUCCESS) {
      emberAfLevelControlClusterPrintln("ERR: reading on/off transition time %x",
                                        status);
      goto send_default_response;
    }

    // Transition time comes in (or is stored, in the case of On/Off Transition
    // Time) as tenths of a second, but we work in milliseconds.
    state->transitionTimeMs = (transitionTimeDs
                               * MILLISECOND_TICKS_PER_SECOND
                               / 10);
#else //ZCL_USING_LEVEL_CONTROL_CLUSTER_ON_OFF_TRANSITION_TIME_ATTRIBUTE
    // If the Transition Time field is 0xFFFF and On/Off Transition Time,
    // which is an optional attribute, is not present, the device shall move to
    // its new level as fast as it is able.
    state->transitionTimeMs = FASTEST_TRANSITION_TIME_MS;
#endif //ZCL_USING_LEVEL_CONTROL_CLUSTER_ON_OFF_TRANSITION_TIME_ATTRIBUTE
  } else {
    // Transition time comes in (or is stored, in the case of On/Off Transition
    // Time) as tenths of a second, but we work in milliseconds.
    state->transitionTimeMs = (transitionTimeDs
                               * MILLISECOND_TICKS_PER_SECOND
                               / 10);
  }

  // for Z02 gateway wakeup function;
  if(transitionTimeDs == 0x0000)
  {
	  status = emberAfWriteServerAttribute(endpoint,
                                       ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                       ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                       (int8u *)&(state->moveToLevel),
                                       ZCL_INT8U_ATTRIBUTE_TYPE);
	  if (status != EMBER_ZCL_STATUS_SUCCESS) {
	    emberAfLevelControlClusterPrintln("ERR: writing current level %x", status);
	    return;
	  }
  }
  // The duration between events will be the transition time divided by the
  // distance we must move.
  state->eventDurationMs = state->transitionTimeMs / actualStepSize;
  state->elapsedTimeMs = 0;

  // OnLevel is not used for Move commands. 
  state->useOnLevel = FALSE;
  
  state->storedLevel = storedLevel;

  // The setup was successful, so mark the new state as active and return.
  schedule(endpoint, state->eventDurationMs);
  status = EMBER_ZCL_STATUS_SUCCESS;

#ifdef EMBER_AF_PLUGIN_ZLL_LEVEL_CONTROL_SERVER
  if (commandId == ZCL_MOVE_TO_LEVEL_WITH_ON_OFF_COMMAND_ID) {
    emberAfPluginZllLevelControlServerMoveToLevelWithOnOffZllExtensions(emberAfCurrentCommand());
  }
#endif

send_default_response:
if (emberAfCurrentCommand()->apsFrame->clusterId
    == ZCL_LEVEL_CONTROL_CLUSTER_ID)
  emberAfSendImmediateDefaultResponse(status);
}

static void moveHandler(int8u commandId, int8u moveMode, int8u rate)
{
  int8u endpoint = emberAfCurrentEndpoint();
  EmberAfLevelControlState *state = getState(endpoint);
  EmberAfStatus status;
  int8u currentLevel;
  int8u difference;

  if (state == NULL) {
    status = EMBER_ZCL_STATUS_FAILURE;
    goto send_default_response;
  }

  // Cancel any currently active command before fiddling with the state.
  deactivate(endpoint);

  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                      ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                      (int8u *)&currentLevel,
                                      sizeof(currentLevel));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);
    goto send_default_response;
  }

  state->commandId = commandId;

  // Move commands cause the device to move from its current level to either
  // the maximum or minimum level at the specified rate.
  switch (moveMode) {
  case EMBER_ZCL_MOVE_MODE_UP:
    state->increasing = TRUE;
    state->moveToLevel = MAX_LEVEL;
    difference = MAX_LEVEL - currentLevel;
    break;
  case EMBER_ZCL_MOVE_MODE_DOWN:
    state->increasing = FALSE;
    state->moveToLevel = MIN_LEVEL;
    difference = currentLevel - MIN_LEVEL;
    break;
  default:
    status = EMBER_ZCL_STATUS_INVALID_FIELD;
    goto send_default_response;
  }

  // If the level is decreasing, the On/Off attribute is left unchanged.  This
  // logic is to prevent a light from transitioning from off to bright to dim.
  // Instead, a light that is off will stay off until the target level is
  // reached.
  if (currentLevel <= state->moveToLevel) {
    if (commandId == ZCL_MOVE_WITH_ON_OFF_COMMAND_ID) {
      setOnOffValue(endpoint, (state->moveToLevel != MIN_LEVEL));
    }
    if (currentLevel == state->moveToLevel) {
      status = EMBER_ZCL_STATUS_SUCCESS;
      goto send_default_response;
    }
  }

  // If the Rate field is 0xFF, the device should move as fast as it is able.
  // Otherwise, the rate is in units per second.
  if (rate == 0xFF) {
    state->eventDurationMs = FASTEST_TRANSITION_TIME_MS;
  } else {
    state->eventDurationMs = MILLISECOND_TICKS_PER_SECOND / rate;
  }
  state->transitionTimeMs = difference * state->eventDurationMs;
  state->elapsedTimeMs = 0;

  // OnLevel is not used for Move commands.
  state->useOnLevel = FALSE;

  // The setup was successful, so mark the new state as active and return.
  schedule(endpoint, state->eventDurationMs);
  status = EMBER_ZCL_STATUS_SUCCESS;

send_default_response:
  emberAfSendImmediateDefaultResponse(status);
}

static void stepHandler(int8u commandId,
                        int8u stepMode,
                        int8u stepSize,
                        int16u transitionTimeDs)
{
  int8u endpoint = emberAfCurrentEndpoint();
  EmberAfLevelControlState *state = getState(endpoint);
  EmberAfStatus status;
  int8u currentLevel;
  int8u actualStepSize = stepSize;

  if (state == NULL) {
    status = EMBER_ZCL_STATUS_FAILURE;
    goto send_default_response;
  }

  // Cancel any currently active command before fiddling with the state.
  deactivate(endpoint);

  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                      ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                      (int8u *)&currentLevel,
                                      sizeof(currentLevel));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);
    goto send_default_response;
  }

  state->commandId = commandId;

  // Step commands cause the device to move from its current level to a new
  // level over the specified transition time.
  switch (stepMode) {
  case EMBER_ZCL_STEP_MODE_UP:
    state->increasing = TRUE;
    if (MAX_LEVEL - currentLevel < stepSize) {
      state->moveToLevel = MAX_LEVEL;
      actualStepSize = (MAX_LEVEL - currentLevel);
    } else {
      state->moveToLevel = currentLevel + stepSize;
    }
    break;
  case EMBER_ZCL_STEP_MODE_DOWN:
    state->increasing = FALSE;
    if (currentLevel - MIN_LEVEL < stepSize) {
      state->moveToLevel = MIN_LEVEL;
      actualStepSize = (currentLevel - MIN_LEVEL);
    } else {
      state->moveToLevel = currentLevel - stepSize;
    }
    break;
  default:
    status = EMBER_ZCL_STATUS_INVALID_FIELD;
    goto send_default_response;
  }

  // If the level is decreasing, the On/Off attribute is left unchanged.  This
  // logic is to prevent a light from transitioning from off to bright to dim.
  // Instead, a light that is off will stay off until the target level is
  // reached.
  if (currentLevel <= state->moveToLevel) {
    if (commandId == ZCL_STEP_WITH_ON_OFF_COMMAND_ID) {
      setOnOffValue(endpoint, (state->moveToLevel != MIN_LEVEL));
    }
    if (currentLevel == state->moveToLevel) {
      status = EMBER_ZCL_STATUS_SUCCESS;
      goto send_default_response;
    }
  }

  // If the Transition Time field is 0xFFFF, the device should move as fast as
  // it is able.
  if (transitionTimeDs == 0xFFFF) {
    state->transitionTimeMs = FASTEST_TRANSITION_TIME_MS;
  } else {
    // Transition time comes in as tenths of a second, but we work in
    // milliseconds.
    state->transitionTimeMs = (transitionTimeDs
                               * MILLISECOND_TICKS_PER_SECOND
                               / 10);
    // If the new level was pegged at the minimum level, the transition time
    // shall be proportionally reduced.  This is done after the conversion to
    // milliseconds to reduce rounding errors in integer division.
    if (stepSize != actualStepSize) {
      state->transitionTimeMs = (state->transitionTimeMs
                                 * actualStepSize
                                 / stepSize);
    }
  }

  // The duration between events will be the transition time divided by the
  // distance we must move.
  state->eventDurationMs = state->transitionTimeMs / actualStepSize;
  state->elapsedTimeMs = 0;

  // OnLevel is not used for Step commands.
  state->useOnLevel = FALSE;

  // The setup was successful, so mark the new state as active and return.
  schedule(endpoint, state->eventDurationMs);
  status = EMBER_ZCL_STATUS_SUCCESS;

send_default_response:
  emberAfSendImmediateDefaultResponse(status);
}

static void stopHandler(int8u commandId)
{
  int8u endpoint = emberAfCurrentEndpoint();
  EmberAfLevelControlState *state = getState(endpoint);
  EmberAfStatus status;

  if (state == NULL) {
    status = EMBER_ZCL_STATUS_FAILURE;
    goto send_default_response;
  }

  // Cancel any currently active command.
  deactivate(endpoint);
  writeRemainingTime(endpoint, 0);
  status = EMBER_ZCL_STATUS_SUCCESS;

send_default_response:
  emberAfSendImmediateDefaultResponse(status);
}

// Follows 07-5123-04 (ZigBee Cluster Library doc), section 3.10.2.1.1.
// Quotes are from table 3.46.
void emberAfOnOffClusterLevelControlEffectCallback(int8u endpoint,
                                                   boolean newValue)
{
  int8u temporaryCurrentLevelCache;
  int16u currentOnOffTransitionTime;
  int8u currentOnLevel;
  int8u minimumLevelAllowedForTheDevice = MIN_LEVEL;
  EmberAfStatus status;

  // "Temporarilty store CurrentLevel."
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                      ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                      (int8u *)&temporaryCurrentLevelCache,
                                      sizeof(temporaryCurrentLevelCache));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);
    return;
  }


  // Read the OnLevel attribute.
#ifdef ZCL_USING_LEVEL_CONTROL_CLUSTER_ON_LEVEL_ATTRIBUTE
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                      ZCL_ON_LEVEL_ATTRIBUTE_ID,
                                      (int8u *)&currentOnLevel,
                                      sizeof(currentOnLevel));

  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);
    return;
  }

  if(currentOnLevel == 0xff) // it's a invalid value
  {
  	status = emberAfReadServerAttribute(endpoint,
                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                      ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                      (int8u *)&currentOnLevel,
                                      sizeof(currentOnLevel));	
	if (status != EMBER_ZCL_STATUS_SUCCESS) {
    	emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);
    	return;
  	}
  }
  

#else
  currentOnLevel = 0xFF;
#endif
  
  // Read the OnOffTransitionTime attribute.
#ifdef ZCL_USING_LEVEL_CONTROL_CLUSTER_ON_OFF_TRANSITION_TIME_ATTRIBUTE
  status = emberAfReadServerAttribute(endpoint,
                                      ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                      ZCL_ON_OFF_TRANSITION_TIME_ATTRIBUTE_ID,
                                      (int8u *)&currentOnOffTransitionTime,
                                      sizeof(currentOnOffTransitionTime));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);
    return;
  }
#else
  currentOnOffTransitionTime = 0xFFFF;
#endif

  #if 0
  if (newValue) {
    // If newValue is ZCL_ON_COMMAND_ID...
    // "Set CurrentLevel to minimum level allowed for the device."
    status = emberAfWriteServerAttribute(endpoint,
                                         ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                         ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                         (int8u *)&minimumLevelAllowedForTheDevice,
                                         ZCL_INT8U_ATTRIBUTE_TYPE);
    if (status != EMBER_ZCL_STATUS_SUCCESS) {
      emberAfLevelControlClusterPrintln("ERR: reading current level %x", status);
      return;
    }                                     
    // "Move CurrentLevel to OnLevel, or to the stored level if OnLevel is not
    // defined, over the time period OnOffTransitionTime."
    moveToLevelHandler(ZCL_MOVE_TO_LEVEL_COMMAND_ID,
                       (currentOnLevel == 0xFF
                        ? temporaryCurrentLevelCache
                        : currentOnLevel),
                       currentOnOffTransitionTime,
                       INVALID_STORED_LEVEL); // Don't revert to stored level
  } else {
    // ...else if newValue is ZCL_OFF_COMMAND_ID...
    // "Move CurrentLevel to the minimum level allowed for the device over the
    // time period OnOffTransitionTime."
    moveToLevelHandler(ZCL_MOVE_TO_LEVEL_COMMAND_ID,
                       minimumLevelAllowedForTheDevice,
                       currentOnOffTransitionTime,
                       temporaryCurrentLevelCache);
                       
    // "If OnLevel is not defined, set the CurrentLevel to the stored level."
    // The emberAfLevelControlClusterServerTickCallback implementation handles
    // this.
  }
  #else
  if (newValue) {
    if (currentOnLevel == 0xFF)
    {
      status = emberAfWriteServerAttribute(endpoint,
                                           ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                           ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                           (int8u *)&temporaryCurrentLevelCache,
                                           ZCL_INT8U_ATTRIBUTE_TYPE);
    }
    else
    {
      status = emberAfWriteServerAttribute(endpoint,
                                           ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                           ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                           (int8u *)&currentOnLevel,
                                           ZCL_INT8U_ATTRIBUTE_TYPE);
    }
                       
    if (status != EMBER_ZCL_STATUS_SUCCESS) {
      emberAfLevelControlClusterPrintln("ERR: writing current level %x", status);
      return;
    }
  }  
  
  status = emberAfWriteServerAttribute(endpoint,
                                       ZCL_ON_OFF_CLUSTER_ID,
                                       ZCL_ON_OFF_ATTRIBUTE_ID,
                                       (int8u *)&newValue,
                                       ZCL_BOOLEAN_ATTRIBUTE_TYPE);
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfLevelControlClusterPrintln("ERR: writing onoff %x", status);
    return;
  }
  #endif
}
