//

// This callback file is created for your convenience. You may add application
// code to this file. If you regenerate this file over a previous version, the
// previous version will be overwritten and any code you have added will be
// lost.

#include "app/framework/include/af.h"

// Custom event stubs. Custom events will be run along with all other events in
// the application framework. They should be managed using the Ember Event API
// documented in stack/include/events.h

// Event control struct declarations
EmberEventControl emberAfPluginIdentifyFeedbackProvideFeedbackEventControl;
EmberEventControl buttonEventControl;
EmberEventControl JoinTheNetworkEventControl;
EmberEventControl ActivePowerDelayEventControl;
EmberEventControl WritePowerAttributeEventControl;
EmberEventControl PowerConsumptionEventControl;

// Event function forward declarations
void emberAfPluginIdentifyFeedbackProvideFeedbackEventHandler(void);
void buttonEventHandler(void);
void JoinTheNetworkEventFunction(void);
void ActivePowerDelayEventFunction(void);
void WritePowerAttributeEventFunction(void);
void PowerConsumptionEventFunction(void);

// Event function stubs
void emberAfPluginIdentifyFeedbackProvideFeedbackEventHandler(void) { }
void buttonEventHandler(void) { }
void JoinTheNetworkEventFunction(void) { }
void ActivePowerDelayEventFunction(void) { }
void WritePowerAttributeEventFunction(void) { }
void PowerConsumptionEventFunction(void) { }

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

/** @brief Reset To Factory Defaults
 *
 * This function is called by the Basic server plugin when a request to reset to
 * factory defaults is received.  The plugin will reset attributes managed by
 * the framework to their default values.  The application should preform any
 * other necessary reset-related operations in this callback, including
 * resetting any externally-stored attributes.
 *
 * @param endpoint   Ver.: always
 */
void emberAfPluginBasicResetToFactoryDefaultsCallback(int8u endpoint)
{
}


