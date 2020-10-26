// Copyright 2013 Silicon Laboratories, Inc.

#include "app/framework/include/af.h"
#include "app/framework/util/af-event.h"
#include "idle-sleep.h"

#ifdef EZSP_HOST
  #define MAX_SLEEP_VALUE_MS MAX_TIMER_MILLISECONDS_HOST
#else
  #define MAX_SLEEP_VALUE_MS 0xFFFFFFFFUL
#endif

#ifdef EMBER_AF_PLUGIN_IDLE_SLEEP_STAY_AWAKE_WHEN_NOT_JOINED
  #define STAY_AWAKE_WHEN_NOT_JOINED_DEFAULT TRUE
#else
  #define STAY_AWAKE_WHEN_NOT_JOINED_DEFAULT FALSE
#endif
boolean emAfStayAwakeWhenNotJoined = STAY_AWAKE_WHEN_NOT_JOINED_DEFAULT;

boolean emAfForceEndDeviceToStayAwake = FALSE;

// NO PRINTFS.  This may be called in ISR context.
void emberAfForceEndDeviceToStayAwake(boolean stayAwake)
{
  emAfForceEndDeviceToStayAwake = stayAwake;
}

#ifdef EMBER_AF_PLUGIN_IDLE_SLEEP_USE_BUTTONS
//int32u sleepDurationMS = 0x0FFFFFFF;
void emberAfHalButtonIsrCallback(int8u button, int8u state)
{
	emberAfDebugPrintln("BUTTON_TRIGGER:button:%d,state:%d", button, state);
	boolean on_off_toggle = FALSE;

	if (state == BUTTON_PRESSED) {
		//emberAfForceEndDeviceToStayAwake(button == BUTTON0);	
	}
	else if ((state == BUTTON_RELEASED) && (button == BUTTON0))
	{		
		halPowerUp();
		emberStackPowerUp();
		emberAfEepromNoteInitializedStateCallback(FALSE);
		emberTick();
		
		emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
								ZCL_ON_OFF_CLUSTER_ID,
								ZCL_ON_OFF_ATTRIBUTE_ID,
								(int8u *)&on_off_toggle,
								sizeof(boolean));
		if (on_off_toggle == FALSE)
		{
			on_off_toggle = TRUE;
		}
		else
		{
			on_off_toggle = FALSE;
		}

		emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
								ZCL_ON_OFF_CLUSTER_ID,
								ZCL_ON_OFF_ATTRIBUTE_ID,
								(int8u *)&on_off_toggle,
								ZCL_BOOLEAN_ATTRIBUTE_TYPE);
		
	}
}
#endif

void emAfPrintSleepDuration(int32u sleepDurationMS, int8u eventIndex)
{
  emberAfDebugPrint("sleep ");
  if (sleepDurationMS == MAX_SLEEP_VALUE_MS) {
    emberAfDebugPrintln("forever");
  } else {
    emberAfDebugPrintln("%l ms (until %p Event)",
                        sleepDurationMS,
                        (eventIndex == 0xFF
                         ? "Stack"
                         : emAfEventStrings[eventIndex]));
  }

  // IMPORTANT:  At this point App Framework apps do blocking serial IO.
  // This means that flush macros are no-ops and serial data is printed
  // at the uart's rate.
  // If there is serial data in the output queue prior to sleeping then we
  // may inadvertently wake up unless we force the flush of the data here.
  emberSerialWaitSend(APP_SERIAL);
}

void emAfPrintForceAwakeStatus(void)
{
  if (emAfForceEndDeviceToStayAwake) {
    emberAfDebugPrintln("Force to Stay awake: yes");
  }
}

boolean emAfOkToIdleOrSleep(void)
{
#ifdef ROBIN_DEBUG
//	emberAfGuaranteedPrintln("emAfForceEndDeviceToStayAwake:%x",emAfForceEndDeviceToStayAwake); // Robin add for debug
//	emberAfGuaranteedPrintln("emAfStayAwakeWhenNotJoined:%x",emAfStayAwakeWhenNotJoined); // Robin add for debug
#endif

  if (emAfForceEndDeviceToStayAwake) {
    return FALSE;
  }

  if (emAfStayAwakeWhenNotJoined) {
    boolean awake = FALSE;
    int8u i;
    for (i = 0; !awake && i < EMBER_SUPPORTED_NETWORKS; i++) {
      emberAfPushNetworkIndex(i);
      awake = (emberAfNetworkState() != EMBER_JOINED_NETWORK);
      emberAfPopNetworkIndex();
    }
    if (awake) {
      return FALSE;
    }
  }

  return (emberAfGetCurrentSleepControlCallback() != EMBER_AF_STAY_AWAKE);
}


static EmberAfEventSleepControl defaultSleepControl = EMBER_AF_OK_TO_SLEEP;

EmberAfEventSleepControl emberAfGetCurrentSleepControlCallback(void)
{
  EmberAfEventSleepControl sleepControl = defaultSleepControl;
#ifdef EMBER_AF_GENERATED_EVENT_CONTEXT
  int8u i;
  for (i = 0; i < emAfAppEventContextLength; i++) {
    EmberAfEventContext *context = &emAfAppEventContext[i];
    if (emberEventControlGetActive(*context->eventControl)
        && sleepControl < context->sleepControl) {
      sleepControl = context->sleepControl;
    }
  }
#endif
  return sleepControl;
}

EmberAfEventSleepControl emberAfGetDefaultSleepControlCallback(void)
{
  return defaultSleepControl;
}

void emberAfSetDefaultSleepControlCallback(EmberAfEventSleepControl sleepControl)
{
  defaultSleepControl = sleepControl;
}
