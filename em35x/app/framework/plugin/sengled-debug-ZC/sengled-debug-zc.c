// *******************************************************************
// * sengled-ha-common.c
// *
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *******************************************************************
#include "app/framework/include/af.h"
#include "sengled-debug-zc.h"

void emberAfMainInitCallback(void)
{
	emberAfGuaranteedPrintln("Main Init");
}

void sengledAfMainInit(void)
{

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

/**@brief A callback invoked by the EmberZNet stack when the
 * MAC has finished transmitting a raw message.
 *
 * If the application includes this callback, it must define
 * EMBER_APPLICATION_HAS_RAW_HANDLER in its CONFIGURATION_HEADER.
 *
 * @param message  The raw message that was sent.
 * @param status   ::EMBER_SUCCESS if the transmission was successful,
 *                 or ::EMBER_DELIVERY_FAILED if not.
 */
void emberRawTransmitCompleteHandler(EmberMessageBuffer message,
                                     EmberStatus status)
{
	int8u length = emberMessageBufferLength(message);
	
	emberAfGuaranteedPrintln("Mac Data Number: %x",message);
}

/**@brief A callback invoked by the EmberZNet stack to filter out incoming
 * application MAC passthrough messages.  If this returns TRUE for a message
 * the complete message will be passed to emberMacPassthroughMessageHandler()
 * with a type of EMBER_MAC_PASSTHROUGH_APPLICATION.
 *
 * Note that this callback may be invoked in ISR context and should execute as
 * quickly as possible.
 *
 * Note that this callback may be called more than once per incoming message.  
 * Therefore the callback code should not depend on being called only once,
 * and should return the same value each time it is called with a given header.
 *
 * If the application includes this callback, it must define
 * EMBER_APPLICATION_HAS_MAC_PASSTHROUGH_FILTER_HANDLER in its
 * CONFIGURATION_HEADER.
 *
 * @param macHeader        A pointer to the initial portion of the
 *     incoming MAC header.  This contains the MAC frame control and
 *     addressing fields.  Subsequent MAC fields, and the MAC payload,
 *     may not be present.
 * @return TRUE if the messages is an application MAC passthrough message.
 */

boolean emberMacPassthroughFilterHandler(int8u *macHeader)
{	
	emberAfGuaranteedPrintln("Mac Header Length: %x",*macHeader);
	emberAfGuaranteedPrintln("Mac Node Addr: %2x",*(macHeader+6));
	
	return TRUE;
}

