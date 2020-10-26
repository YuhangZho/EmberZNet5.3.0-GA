// *****************************************************************************
// * sengled-hardware-downlight.c
// *
// * This code provides support for managing the hardware for CIA19.
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *****************************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/attribute-storage.h"

#include "sengled-hardware-downlight.h"
#include "app/framework/plugin/sengled-ha-common/sengled-ha-common.h"
#include "app/framework/plugin/sengled-ha-common/sengled-ledControl.h"
#include "app/framework/plugin/sengled-ha-cli/sengled-cli.h"
#include "app/framework/plugin/sengled-ha-common/sengled-ha-token.h"

//extern int16u dimmerWaveFormCounter;
extern int8u joinNetworkFirstTimeFlag;
extern int8u powerUpFirstTime;
extern EmberEventControl powerReportEventControl;

static int8u doubleClickCount = 0;
static boolean doubleClickStart = FALSE;
static boolean doubleClickEnd = TRUE;
int8u blinkTimes = 0;
boolean avoidRebootSuddenBlink = FALSE; 

extern int8u currentLevelForLedBreath;
extern int16u currentColorTptForLedBreath;
static int8u breathTimes = 0;	
static boolean upDown = BREATH_UP;  // by default, it changes from current to 100% level, so it's up;

EmberEventControl  doubleClickEventControl;
EmberEventControl  ledBreathEventControl;
EmberEventControl  networkOperationEventControl;
EmberEventControl  onOffStatusWhenPowerOffEventControl;

void LedBreathEventFunction(void)
{
  int8u currentOnLevel= 0xff;
  emberEventControlSetInactive(ledBreathEventControl);

  if(currentLevelForLedBreath == 255 && upDown == BREATH_UP)
  {
    breathTimes--;
	upDown = BREATH_DOWN;
  } 
  else if(currentLevelForLedBreath == 1 && upDown == BREATH_DOWN) // by default it's up, so it's ok here
  {
    breathTimes--;
	upDown = BREATH_UP;    
  }
	
  if(upDown == BREATH_DOWN)
  {
    currentLevelForLedBreath--;
  }
  else
  {
	currentLevelForLedBreath++;  
  }
 
  //GPIO_PBCLR = BIT(3); 
  SetLedLevel(currentLevelForLedBreath);
  
  if(breathTimes == 0)
  {
    doubleClickCount = 0; // we need to reset count when the breath finish;
    upDown = BREATH_UP;   // we need to init towards when the breath finish;
#ifdef ROBIN_DEBUG
    emberAfGuaranteedPrintln("breathFinish0: %d",currentLevelForLedBreath); // Robin add for debug
#endif
    emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_LEVEL_CONTROL_CLUSTER_ID,
                              ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                              (int8u *)&currentLevelForLedBreath,
                              ZCL_INT8U_ATTRIBUTE_TYPE); 

    emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_LEVEL_CONTROL_CLUSTER_ID,
                              ZCL_ON_LEVEL_ATTRIBUTE_ID,
                              (int8u *)&currentOnLevel,
                              sizeof(currentOnLevel));	
	
	if(currentOnLevel != 0xFF)
	{
	  // when write onLevel, must be careful! Read first if it is 0xff, onlevel is not used
	  emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
					   ZCL_LEVEL_CONTROL_CLUSTER_ID,
					   ZCL_ON_LEVEL_ATTRIBUTE_ID,
					   (int8u *)&currentLevelForLedBreath,
					   ZCL_INT8U_ATTRIBUTE_TYPE);
	}
	return;
  }

  emberEventControlSetDelayMS(ledBreathEventControl, 30); 
}

void NetworkOperationEventFunction(void)
{
  int8u currentLevel=0;
  int16u currentColorTemperature = 0;
  if(blinkTimes == RESETF_BLINK && avoidRebootSuddenBlink == FALSE)
  {
    avoidRebootSuddenBlink = TRUE;
  }

  if(blinkTimes > 0)
  {
    if(blinkTimes & 0x01)  // need on
    {
      emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_LEVEL_CONTROL_CLUSTER_ID,
                              ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                              (int8u *)&currentLevel,
                              sizeof(int8u));
	  emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_COLOR_CONTROL_CLUSTER_ID,
                              ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
                              (int8u *)&currentColorTemperature,
							  sizeof(int16u));
#ifdef ROBIN_DEBUG	  
	  emberAfGuaranteedPrintln("blink On %d",blinkTimes); // Robin add for debug
#endif
      SetLed(LED_ON, currentLevel, currentColorTemperature);
	}
	else                   // need off
	{ 
	  if(blinkTimes == 2 && avoidRebootSuddenBlink == TRUE)
	  {
	    blinkTimes = 0;
        avoidRebootSuddenBlink =FALSE;		
		SetLed(LED_OFF, DIMMING_MAX_LEVEL, COLOR_YELLOWEST);
		emberEventControlSetDelayMS(networkOperationEventControl, 600);
		return;
	  }
#ifdef ROBIN_DEBUG	  
	  emberAfGuaranteedPrintln("blink Off %d",blinkTimes); // Robin add for debug
#endif
      SetLed(LED_OFF, DIMMING_MAX_LEVEL, COLOR_YELLOWEST);
	}
    blinkTimes--;
	
	emberEventControlSetDelayMS(networkOperationEventControl, 500);
  }
  else
  {
#ifdef ROBIN_DEBUG
    emberAfGuaranteedPrintln("double-network %d",doubleClickCount); // Robin add for debug
#endif
    if(doubleClickCount == REJOIN)
    {
      //emberFindAndRejoinNetwork(1, EMBER_ALL_802_15_4_CHANNELS_MASK);
      emberFindAndRejoinNetworkWithReason(1,  // 1=rejoin with encryption, 0=rejoin without encryption
                                        EMBER_ALL_802_15_4_CHANNELS_MASK,
                                        EMBER_AF_REJOIN_DUE_TO_END_DEVICE_MOVE); // end device rejoin 
	}
	else if(doubleClickCount >= RESETF)
	{
      emberLeaveNetwork();
	  joinNetworkFirstTimeFlag = 0x00;
      halCommonSetToken(TOKEN_THE_FIRST_JOINED_FLAG, &joinNetworkFirstTimeFlag);

	  powerUpFirstTime = 0x00;		  
	  halCommonSetToken(TOKEN_THE_FIRST_POWERUP, &powerUpFirstTime);

	  emberAfResetAttributes(emberAfPrimaryEndpoint());
      emberAfGroupsClusterClearGroupTableCallback(emberAfPrimaryEndpoint());
      emberAfScenesClusterClearSceneTableCallback(emberAfPrimaryEndpoint());
      emberClearBindingTable();
      emberAfClearReportTableCallback();
      halReboot();
	}
	
	doubleClickCount = 0;
	emberEventControlSetInactive(networkOperationEventControl);
  }
}


void DoubleClickEventFunction(void)
{
  int8u currentOnLevel = 0xff;
  emberEventControlSetInactive(doubleClickEventControl);

  if(doubleClickEnd == TRUE)
  {
    doubleClickStart = FALSE;
#ifdef ROBIN_DEBUG	
	emberAfGuaranteedPrintln("ClickCount: %d",doubleClickCount); // Robin add for debug
#endif
	switch(doubleClickCount)
	{
    case BREATH_START:  // do not clear doubleClickCount
        {
		  breathTimes = BREATH_TOTAL_TIMES;
          emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_LEVEL_CONTROL_CLUSTER_ID,
                              ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                              (int8u *)&currentLevelForLedBreath,
                              sizeof(int8u));
	      emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_COLOR_CONTROL_CLUSTER_ID,
                              ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_ATTRIBUTE_ID,
                              (int8u *)&currentColorTptForLedBreath,
                              sizeof(int16u));		  
          emberEventControlSetActive(ledBreathEventControl);
	    }
        break;
    case BREATH_END:
		{
#ifdef ROBIN_DEBUG
	      emberAfGuaranteedPrintln("breathFinish1: %d",currentLevelForLedBreath); // Robin add for debug
#endif
		  // first record currentLevel
		  emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                             ZCL_LEVEL_CONTROL_CLUSTER_ID,
                             ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                             (int8u *)&currentLevelForLedBreath,
                             ZCL_INT8U_ATTRIBUTE_TYPE);

		  emberAfReadServerAttribute(emberAfPrimaryEndpoint(),
									ZCL_LEVEL_CONTROL_CLUSTER_ID,
									ZCL_ON_LEVEL_ATTRIBUTE_ID,
									(int8u *)&currentOnLevel,
									sizeof(currentOnLevel));  

		  if(currentOnLevel != 0xFF)
		  {
		  	// when write onLevel, must be careful! Read first if it is 0xff, onlevel is not used
		    emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                             ZCL_LEVEL_CONTROL_CLUSTER_ID,
                             ZCL_ON_LEVEL_ATTRIBUTE_ID,
                             (int8u *)&currentLevelForLedBreath,
                             ZCL_INT8U_ATTRIBUTE_TYPE);
		  }
		  // clear 
		  // 2016.4.20 
		  //currentLevelForLedBreath = 0x00;
		  //currentColorTptForLedBreath = 0x0000;
		  breathTimes = 0;
		  doubleClickCount = 0;
		  upDown = BREATH_UP;   // we need to init towards when the breath finish;
		  emberEventControlSetInactive(ledBreathEventControl);
    	}
		break;
    case REJOIN:  // do not clear doubleClickCount right now
		{
			blinkTimes = REJOIN_BLINK; 
			emberEventControlSetActive(networkOperationEventControl);

    	}
        break;
    case EZMODE:
        {
		  emberAfEzmodeServerCommission(emberAfPrimaryEndpoint());
          doubleClickCount = 0;
    	}
		break;
	default:  // 2016.4.20
		if(doubleClickCount < RESETF)
		{
			doubleClickCount = 0; // do not record  if useless;
		}
	}
	
    if(doubleClickCount >= RESETF)
    {
      blinkTimes = RESETF_BLINK;
      emberEventControlSetActive(networkOperationEventControl);
	}
  }

}

void OnOffStatusWhenPowerOffEventFunction(void)
{
  emberEventControlSetInactive(onOffStatusWhenPowerOffEventControl);  

#ifdef ROBIN_DEBUG
		emberAfGuaranteedPrintln("OnOffEvent: OFF"); // Robin add for debug
#endif

  sengledGlobalReportCommand(ZCL_SIMPLE_METERING_CLUSTER_ID,ZCL_INSTANTANEOUS_DEMAND_ATTRIBUTE_ID, 
  							ZCL_INT24S_ATTRIBUTE_TYPE, CLUSTER_IS_SERVER);
  emAfSengledSendCommand(COORDINATOR_NODE_ID);

}

/** @brief Hal Button Isr
 *
 * This callback is called by the framework whenever a button is pressed on the
 * device. This callback is called within ISR context.
 *
 * @param button The button which has changed state, either BUTTON0 or BUTTON1
 * as defined in the appropriate BOARD_HEADER.  Ver.: always
 * @param state The new state of the button referenced by the button parameter,
 * either ::BUTTON_PRESSED if the button has been pressed or ::BUTTON_RELEASED
 * if the button has been released.  Ver.: always
 */
void emberAfHalButtonIsrCallback(int8u button,
                                 int8u state)
{
  boolean onOffStatusWhenPowerOff;
#ifdef ONE_LINE_POWER_AVOID
  static int16u oldTime, newTime;  // 2016.3.23 power on affect another when they all supplied by one power
#endif

  if(button == DOUBLE_CLICK)
  {
    if (state == DOUBLE_CLICK_ON)
    {
#ifdef ROBIN_DEBUG
      emberAfGuaranteedPrintln("DOUBLE_CLICK: ON"); // Robin add for debug
#endif
      if(doubleClickStart == TRUE)
      {
        doubleClickEnd = TRUE;
#ifdef ONE_LINE_POWER_AVOID		
		newTime = halCommonGetInt16uMillisecondTick();
        if (50 > elapsedTimeInt16u(oldTime, newTime))
        {
        	//do nothing to filter signal burr
        }
		else
		{
			doubleClickCount++;
		}
#else
		doubleClickCount++;
#endif		
		onOffStatusWhenPowerOff = 1;
		emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
									ZCL_ON_OFF_CLUSTER_ID,
									ZCL_ON_OFF_ATTRIBUTE_ID,
									(int8u *)&onOffStatusWhenPowerOff,
									ZCL_BOOLEAN_ATTRIBUTE_TYPE);  

		emberEventControlSetDelayMS(doubleClickEventControl,1300);
      }
    }
	else if(state == DOUBLE_CLICK_OFF)
	{
#ifdef ROBIN_DEBUG
	  emberAfGuaranteedPrintln("DOUBLE_CLICK: OFF"); // Robin add for debug
#endif

	  doubleClickStart = TRUE;
	  doubleClickEnd = FALSE;
#ifdef ONE_LINE_POWER_AVOID	  
	  // 2016.3.23 power on affect another when they are all 
	  // connected by one line and supplied by one power
	  oldTime = halCommonGetInt16uMillisecondTick();
#endif	
	  sengledGlobalReportCommand(ZCL_ON_OFF_CLUSTER_ID,ZCL_ON_OFF_ATTRIBUTE_ID, 
								ZCL_BOOLEAN_ATTRIBUTE_TYPE, CLUSTER_IS_SERVER);
	  emAfSengledSendCommand(COORDINATOR_NODE_ID);
	  emberEventControlSetDelayMS(onOffStatusWhenPowerOffEventControl,10);

	  SetTheApsNumType(emberAfGetLastSequenceNumber());
	  powerUpFirstTime = 0x01;		  
	  halCommonSetToken(TOKEN_THE_FIRST_POWERUP, &powerUpFirstTime);

	  //emberEventControlSetActive(onOffStatusWhenPowerOffEventControl);
	}
	return;
  }
/*
  if(button == DIMMER_DETECT)
  { 
    if(state == BUTTON_RELEASED)
    {
      dimmerWaveFormCounter++; 
#ifdef ROBIN_DEBUG
	  emberAfGuaranteedPrintln("dWFCtr: %2x",dimmerWaveFormCounter); // Robin add for debug
#endif
	}
	else if(state == BUTTON_PRESSED)
	{
#ifdef ROBIN_DEBUG
      emberAfGuaranteedPrintln("DIMMER_DETECT: PRESSED"); 
#endif
	}
	return;
  }
*/
}

