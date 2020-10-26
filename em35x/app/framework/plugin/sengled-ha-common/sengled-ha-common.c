// *******************************************************************
// * sengled-ha-common.c
// *
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *******************************************************************
#include "app/framework/include/af.h"
#include "sengled-ha-common.h"
#include "app/framework/plugin/sengled-hardware-A19EUE27/sengled-adc.h"
#include "app/framework/plugin/counters/counters.h"
#include "sengled-ha-token.h"

boolean doubleClickEnable = FALSE;
int16u dimmerWaveFormCounter;
int8u powerUpFirstTime;

EmberEventControl dimmerDetectControl;
EmberEventControl diagnosticsEventControl;


void GetTheApsNumType(void)
{ 
  int8u num;

  halCommonGetToken(&num, TOKEN_THE_APSNUM_TYPE);
  SengledSetSequenceNumber(num+1);
}
void SetTheApsNumType(int8u num)
{ 
  halCommonSetToken(TOKEN_THE_APSNUM_TYPE, &num);
}


void diagnosticsEventFunction(void) 
{
  int16u tmp;

  tmp = emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_SUCCESS]+emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_FAILED]+emberCounters[EMBER_COUNTER_APS_DATA_TX_UNICAST_RETRY];
  if (tmp != 0)
  { 
    tmp = emberCounters[EMBER_COUNTER_MAC_TX_UNICAST_RETRY]/tmp;
    
    emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                               ZCL_DIAGNOSTICS_CLUSTER_ID,
                               ZCL_AVERAGE_MAC_RETRY_PER_APS_MSG_SENT_ATTRIBUTE_ID,
                               (int8u *)&tmp,
                               ZCL_INT16U_ATTRIBUTE_TYPE);
  }  
  
  emberEventControlSetDelayQS(diagnosticsEventControl, 240);  //60s
}

//*******************************************************************************
// 1-10: level: 10% -100%
// 11-20: cct: 10% -100%
// 99: firmware version
//*******************************************************************************
int8u firmwareVersion[] = "Z01_A19EUE27_V023_170224";
boolean emberProcessCommandSendled(int8u *input, int8u sizeOrPort)
{
  int8u dat;
  
  while (EMBER_SUCCESS == emberSerialReadByte(sizeOrPort, &dat))
  {
    if ((1 <= dat) && (dat <= 10)) 
    {
      SetPwmLevel(TICS_PER_PERIOD-600*dat, DIMMING_CTRL_PIN, FALSE);
      emberSerialWriteData(APP_SERIAL, &dat, 1);
    }
    else if ((11 <= dat) && (dat <= 20))
    { 
      SetPwmLevel((dat-10)*600, COLOR_TEMP_CTRL_PIN, FALSE);
      emberSerialWriteData(APP_SERIAL, &dat, 1);
    }
    else if (99 == dat)
    { emberSerialWriteData(APP_SERIAL, firmwareVersion, sizeof(firmwareVersion));}
  }

  return FALSE;
}


void DimmerDetectFunction(void)
{
  emberEventControlSetInactive(dimmerDetectControl);

  if(dimmerWaveFormCounter <= DIMMER_CONNECTED_LINE)
  {
    // there is no dimmer connetced, so we also enable doubelClick
	SetCloseCircuit();
	doubleClickEnable = TRUE;
#ifdef ROBIN_DEBUG
	emberAfGuaranteedPrintln("no dimmer connetced"); // Robin add for debug
#endif

  }
  dimmerWaveFormCounter = 0;

  //start from a fresh state just in case
  BUTTON1_INTCFG = 0;              //disable BUTTON1 triggering
  INT_CFGCLR = BUTTON1_INT_EN_BIT; //clear BUTTON1 top level int enable
  INT_GPIOFLAG = BUTTON1_FLAG_BIT; //clear stale BUTTON1 interrupt
  INT_MISS = BUTTON1_MISS_BIT;     //clear stale missed BUTTON1 interrupt
}

void emberAfMainInitCallback(void)
{
  emberAfPluginPwmControlInitCallback();
}

void sengledAfMainInit(void)
{
  // open dimmer when power on
  SetOpenCircuit();	
#ifndef COLOR_SELF_CHANGE 
  // request to join network when power on 
  JoinNetworkRequest();
#endif

  // Adc Init
  AdcControlInit();

  // avoid APS sequence number init when power off and power on suddenly;
  GetTheApsNumType();
  
  // start to detct dimmer, by defalut dimmer is open, and if dimmer is open,
  // doubleClick should be disable;
  dimmerWaveFormCounter = 0;
  doubleClickEnable = FALSE; 
  emberEventControlSetDelayMS(dimmerDetectControl, 1200);

  halCommonGetToken(&powerUpFirstTime, TOKEN_THE_FIRST_POWERUP);
#ifdef ROBIN_DEBUG	
	  emberAfGuaranteedPrintln("_____INITPFirstTime:%x",powerUpFirstTime); // Robin add for debug
#endif	
  // confirm PB7 output pwm for color temperature
  halGpioConfig(PORTB_PIN(7), GPIOCFG_OUT_ALT);
  InitLedStatus(FALSE);
}

#ifdef EMBER_SOC

boolean emberAfMainStartCallback(int* returnCode,
                                 int argc,
                                 char** argv)
{
	return FALSE;
}
#endif

void emberAfMainTickCallback(void)
{  
  AdcControlFunction();
  emberEventControlSetDelayQS(diagnosticsEventControl, 240);  //60s
}

void emberAfPluginBasicResetToFactoryDefaultsCallback(int8u endpoint)
{
}


/** @brief Message Sent
 *
 * This function is called by the application framework from the message sent
 * handler, when it is informed by the stack regarding the message sent status.
 * All of the values passed to the emberMessageSentHandler are passed on to this
 * callback. This provides an opportunity for the application to verify that its
 * message has been sent successfully and take the appropriate action. This
 * callback should return a boolean value of TRUE or FALSE. A value of TRUE
 * indicates that the message sent notification has been handled and should not
 * be handled by the application framework.
 *
 * @param type   Ver.: always
 * @param indexOrDestination   Ver.: always
 * @param apsFrame   Ver.: always
 * @param msgLen   Ver.: always
 * @param message   Ver.: always
 * @param status   Ver.: always
 */
boolean emberAfMessageSentCallback(EmberOutgoingMessageType type,
                                   int16u indexOrDestination,
                                   EmberApsFrame* apsFrame,
                                   int16u msgLen,
                                   int8u* message,
                                   EmberStatus status)
{
  return FALSE;
}

/** @brief Pre Message Received
 *
 * This callback is the first in the Application Framework's message processing
 * chain. The Application Framework calls it when a message has been received
 * over the air but has not yet been parsed by the ZCL command-handling code. If
 * you wish to parse some messages that are completely outside the ZCL
 * specification or are not handled by the Application Framework's command
 * handling code, you should intercept them for parsing in this callback. 
     
 *   This callback returns a Boolean value indicating whether or not the message
 * has been handled. If the callback returns a value of TRUE, then the
 * Application Framework assumes that the message has been handled and it does
 * nothing else with it. If the callback returns a value of FALSE, then the
 * application framework continues to process the message as it would with any
 * incoming message.
        Note: 	This callback receives a pointer to an
 * incoming message struct. This struct allows the application framework to
 * provide a unified interface between both Host devices, which receive their
 * message through the ezspIncomingMessageHandler, and SoC devices, which
 * receive their message through emberIncomingMessageHandler.
 *
 * @param incomingMessage   Ver.: always
 */
boolean emberAfPreMessageReceivedCallback(EmberAfIncomingMessage* incomingMessage)
{
  return FALSE;
}


/** @brief Pre Attribute Change
 *
 * This function is called by the application framework before it changes an
 * attribute value. The value passed into this callback is the value to which
 * the attribute is to be set by the framework.
 *
 * @param endpoint   Ver.: always
 * @param clusterId   Ver.: always
 * @param attributeId   Ver.: always
 * @param mask   Ver.: always
 * @param manufacturerCode   Ver.: always
 * @param type   Ver.: always
 * @param size   Ver.: always
 * @param value   Ver.: always
 */
void emberAfPreAttributeChangeCallback(int8u endpoint,
                                       EmberAfClusterId clusterId,
                                       EmberAfAttributeId attributeId,
                                       int8u mask,
                                       int16u manufacturerCode,
                                       int8u type,
                                       int8u size,
                                       int8u* value)
{
}

/** @brief Pre Command Received
 *
 * This callback is the second in the Application Framework's message processing
 * chain. At this point in the processing of incoming over-the-air messages, the
 * application has determined that the incoming message is a ZCL command. It
 * parses enough of the message to populate an EmberAfClusterCommand struct. The
 * Application Framework defines this struct value in a local scope to the
 * command processing but also makes it available through a global pointer
 * called emberAfCurrentCommand, in app/framework/util/util.c. When command
 * processing is complete, this pointer is cleared.
 *
 * @param cmd   Ver.: always
 */
boolean emberAfPreCommandReceivedCallback(EmberAfClusterCommand* cmd)
{
  return FALSE;
}

/** @brief Pre ZDO Message Received
 *
 * This function passes the application an incoming ZDO message and gives the
 * appictation the opportunity to handle it. By default, this callback returns
 * FALSE indicating that the incoming ZDO message has not been handled and
 * should be handled by the Application Framework.
 *
 * @param emberNodeId   Ver.: always
 * @param apsFrame   Ver.: always
 * @param message   Ver.: always
 * @param length   Ver.: always
 */
boolean emberAfPreZDOMessageReceivedCallback(EmberNodeId emberNodeId,
                                             EmberApsFrame* apsFrame,
                                             int8u* message,
                                             int16u length)
{
  return FALSE;
}


/** @brief External Attribute Write
 *
 * This function is called whenever the Application Framework needs to write an
 * attribute which is not stored within the data structures of the Application
 * Framework itself. One of the new features in Version 2 is the ability to
 * store attributes outside the Framework. This is particularly useful for
 * attributes that do not need to be stored because they can be read off the
 * hardware when they are needed, or are stored in some central location used by
 * many modules within the system. In this case, you can indicate that the
 * attribute is stored externally. When the framework needs to write an external
 * attribute, it makes a call to this callback.
        This callback is very
 * useful for host micros which need to store attributes in persistent memory.
 * Because each host micro (used with an Ember NCP) has its own type of
 * persistent memory storage, the Application Framework does not include the
 * ability to mark attributes as stored in flash the way that it does for Ember
 * SoCs like the EM35x. On a host micro, any attributes that need to be stored
 * in persistent memory should be marked as external and accessed through the
 * external read and write callbacks. Any host code associated with the
 * persistent storage should be implemented within this callback.
        All of
 * the important information about the attribute itself is passed as a pointer
 * to an EmberAfAttributeMetadata struct, which is stored within the application
 * and used to manage the attribute. A complete description of the
 * EmberAfAttributeMetadata struct is provided in
 * app/framework/include/af-types.h.
        This function assumes that the
 * application is able to write the attribute and return immediately. Any
 * attributes that require a state machine for reading and writing are not
 * candidates for externalization at the present time. The Application Framework
 * does not currently include a state machine for reading or writing attributes
 * that must take place across a series of application ticks. Attributes that
 * cannot be written immediately should be stored within the Application
 * Framework and updated occasionally by the application code from within the
 * emberAfMainTickCallback.
        If the application was successfully able to
 * write the attribute, it returns a value of EMBER_ZCL_STATUS_SUCCESS. Any
 * other return value indicates the application was not able to write the
 * attribute.
 *
 * @param endpoint   Ver.: always
 * @param clusterId   Ver.: always
 * @param attributeMetadata   Ver.: always
 * @param manufacturerCode   Ver.: always
 * @param buffer   Ver.: always
 */
EmberAfStatus emberAfExternalAttributeWriteCallback(int8u endpoint,
                                                    EmberAfClusterId clusterId,
                                                    EmberAfAttributeMetadata *attributeMetadata,
                                                    int16u manufacturerCode,
                                                    int8u *buffer)
{
  return EMBER_ZCL_STATUS_FAILURE;
}

/** @brief Post Attribute Change
 *
 * This function is called by the application framework after it changes an
 * attribute value. The value passed into this callback is the value to which
 * the attribute was set by the framework.
 *
 * @param endpoint   Ver.: always
 * @param clusterId   Ver.: always
 * @param attributeId   Ver.: always
 * @param mask   Ver.: always
 * @param manufacturerCode   Ver.: always
 * @param type   Ver.: always
 * @param size   Ver.: always
 * @param value   Ver.: always
 */
void emberAfPostAttributeChangeCallback(int8u endpoint,
                                        EmberAfClusterId clusterId,
                                        EmberAfAttributeId attributeId,
                                        int8u mask,
                                        int16u manufacturerCode,
                                        int8u type,
                                        int8u size,
                                        int8u* value)
{
}

/** @brief External Attribute Read
 *
 * Like emberAfExternalAttributeWriteCallback above, this function is called
 * when the framework needs to read an attribute that is not stored within the
 * Application Framework's data structures.
        All of the important
 * information about the attribute itself is passed as a pointer to an
 * EmberAfAttributeMetadata struct, which is stored within the application and
 * used to manage the attribute. A complete description of the
 * EmberAfAttributeMetadata struct is provided in
 * app/framework/include/af-types.h
        This function assumes that the
 * application is able to read the attribute, write it into the passed buffer,
 * and return immediately. Any attributes that require a state machine for
 * reading and writing are not really candidates for externalization at the
 * present time. The Application Framework does not currently include a state
 * machine for reading or writing attributes that must take place across a
 * series of application ticks. Attributes that cannot be read in a timely
 * manner should be stored within the Application Framework and updated
 * occasionally by the application code from within the
 * emberAfMainTickCallback.
        If the application was successfully able to
 * read the attribute and write it into the passed buffer, it should return a
 * value of EMBER_ZCL_STATUS_SUCCESS. Any other return value indicates the
 * application was not able to read the attribute.
 *
 * @param endpoint   Ver.: always
 * @param clusterId   Ver.: always
 * @param attributeMetadata   Ver.: always
 * @param manufacturerCode   Ver.: always
 * @param buffer   Ver.: always
 */
EmberAfStatus emberAfExternalAttributeReadCallback(int8u endpoint,
                                                   EmberAfClusterId clusterId,
                                                   EmberAfAttributeMetadata *attributeMetadata,
                                                   int16u manufacturerCode,
                                                   int8u *buffer)
{
  return EMBER_ZCL_STATUS_FAILURE;
}

