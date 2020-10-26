// Copyright 2013 Silicon Laboratories, Inc.

#include "app/framework/include/af.h"
#include "app/util/common/form-and-join.h"
#include "app/framework/plugin/ezmode-commissioning/ez-mode.h"


#define LED BOARDLED1
#define PJOIN_DURATION_S 60

boolean fobidden_ota_image_response = FALSE;

// Event control struct declarations
EmberEventControl buttonEventControl;

static int8u PGM happyTune[] = {
  NOTE_B4, 1,
  0,       1,
  NOTE_B5, 1,
  0,       0
};
static int8u PGM sadTune[] = {
  NOTE_B5, 1,
  0,       1,
  NOTE_B4, 5,
  0,       0
};
static int8u PGM waitTune[] = {
  NOTE_B4, 1,
  0,       0
};

static void pjoin(void)
{
  if (emberPermitJoining(PJOIN_DURATION_S) == EMBER_SUCCESS) {
    halPlayTune_P(happyTune, TRUE);
  } else {
    halPlayTune_P(sadTune, TRUE);
  }
}

void buttonEventHandler(void)
{
  // On a press-and-hold button event, form a network if we don't have one or
  // permit joining if we do.  If we form, we'll automatically permit joining
  // when we come up.
  emberEventControlSetInactive(buttonEventControl);
  if (halButtonState(BUTTON0) == BUTTON_PRESSED) {
    if (emberNetworkState() == EMBER_NO_NETWORK) {
      halPlayTune_P(((emberFormAndJoinIsScanning()
                      || emberAfFindUnusedPanIdAndForm() == EMBER_SUCCESS)
                     ? waitTune
                     : sadTune),
                    TRUE);
    } else {
      pjoin();
    }
  } else if (halButtonState(BUTTON1) == BUTTON_PRESSED) {
    emberAfEzmodeServerCommission(emberAfPrimaryEndpoint());
  }
}

/** @brief Stack Status
 *
 * This function is called by the application framework from the stack status
 * handler.  This callbacks provides applications an opportunity to be
 * notified of changes to the stack status and take appropriate action.  The
 * application should return TRUE if the status has been handled and should
 * not be handled by the application framework.
 *
 * @param status   Ver.: always
 */
boolean emberAfStackStatusCallback(EmberStatus status)
{
  // Whenever the network comes up, permit joining.  If we go down, let the
  // user know, although this shouldn't happen.
  if (status == EMBER_NETWORK_UP) {
    pjoin();
  } else if (status == EMBER_NETWORK_DOWN) {
    halPlayTune_P(sadTune, TRUE);
  }
  return FALSE;
}

/** @brief emberAfHalButtonIsrCallback
 *
 *
 */
// Hal Button ISR Callback
// This callback is called by the framework whenever a button is pressed on the
// device. This callback is called within ISR context.
void emberAfHalButtonIsrCallback(int8u button, int8u state)
{
  if ((button == BUTTON0 || button == BUTTON1) && state == BUTTON_PRESSED) {
    emberEventControlSetActive(buttonEventControl);
  }
}

/** @brief Broadcast Sent
 *
 * This function is called when a new MTORR broadcast has been successfully
 * sent by the concentrator plugin.
 *
 */
void emberAfPluginConcentratorBroadcastSentCallback(void)
{
}

/** @brief Client Complete
 *
 * This function is called by the EZ-Mode Commissioning plugin when client
 * commissioning completes.
 *
 * @param bindingIndex The binding index that was created or
 * ::EMBER_NULL_BINDING if an error occurred.  Ver.: always
 */
void emberAfPluginEzmodeCommissioningClientCompleteCallback(int8u bindingIndex)
{
}

/** @brief Finished
 *
 * This callback is fired when the network-find plugin is finished with the
 * forming or joining process.  The result of the operation will be returned
 * in the status parameter.
 *
 * @param status   Ver.: always
 */
void emberAfPluginNetworkFindFinishedCallback(EmberStatus status)
{
}

/** @brief Get Radio Power For Channel
 *
 * This callback is called by the framework when it is setting the radio power
 * during the discovery process. The framework will set the radio power
 * depending on what is returned by this callback.
 *
 * @param channel   Ver.: always
 */
int8s emberAfPluginNetworkFindGetRadioPowerForChannelCallback(int8u channel)
{
  return EMBER_AF_PLUGIN_NETWORK_FIND_RADIO_TX_POWER;
}

/** @brief Join
 *
 * This callback is called by the plugin when a joinable network has been
 * found.  If the application returns TRUE, the plugin will attempt to join
 * the network.  Otherwise, the plugin will ignore the network and continue
 * searching.  Applications can use this callback to implement a network
 * blacklist.
 *
 * @param networkFound   Ver.: always
 * @param lqi   Ver.: always
 * @param rssi   Ver.: always
 */
boolean emberAfPluginNetworkFindJoinCallback(EmberZigbeeNetwork *networkFound,
                                             int8u lqi,
                                             int8s rssi)
{
  return TRUE;
}

/** @brief Configured
 *
 * This callback is called by the Reporting plugin whenever a reporting entry
 * is configured, including when entries are deleted or updated.  The
 * application can use this callback for scheduling readings or measurements
 * based on the minimum and maximum reporting interval for the entry.  The
 * application should return EMBER_ZCL_STATUS_SUCCESS if it can support the
 * configuration or an error status otherwise.  Note: attribute reporting is
 * required for many clusters and attributes, so rejecting a reporting
 * configuration may violate ZigBee specifications.
 *
 * @param entry   Ver.: always
 */
EmberAfStatus emberAfPluginReportingConfiguredCallback(const EmberAfPluginReportingEntry * entry)
{
  return EMBER_ZCL_STATUS_SUCCESS;
}


/** @brief Trust Center Join
 *
 * This callback is called from within the application framework's
 * implementation of emberTrustCenterJoinHandler or ezspTrustCenterJoinHandler.
 * This callback provides the same arguments passed to the
 * TrustCenterJoinHandler. For more information about the TrustCenterJoinHandler
 * please see documentation included in stack/include/trust-center.h.
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
	emberAfGuaranteedPrintln("newNodeId: %2x, parentofNewNode: %2x", newNodeId,parentOfNewNode);
	emberAfGuaranteedPrintln("EmberEUI64: %x,%x,%x,%x,%x,%x,%x,%x,",
							newNodeEui64[7],
							newNodeEui64[6],
							newNodeEui64[5],
							newNodeEui64[4],
							newNodeEui64[3],
							newNodeEui64[2],
							newNodeEui64[1],
							newNodeEui64[0]);
	
	emberAfGuaranteedPrintln("	EmberDeviceUpdate: %x", status);
	emberAfGuaranteedPrintln("	EmberJoinDecision: %x", decision);
	
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
	if(apsFrame->clusterId == 0x0036 && apsFrame->profileId == 0x0000)
	{
		static int16u SequenceNumber=0,SequenceNumber_back=0,emberNodeId_back=0;
		emberAfCopyInt16u(message,0,SequenceNumber);
		if(emberNodeId==emberAfGetNodeId())
			return TRUE;
		else if((emberNodeId==emberNodeId_back)&&(SequenceNumber_back==SequenceNumber))
			return TRUE;
		else
		{
			emberNodeId_back = emberNodeId;
			SequenceNumber_back = SequenceNumber;
			emberAfGuaranteedPrintln("emberAfPreZDOMessageReceivedCallback: PermitJoin!: sender 0x%2x, SN 0x%2x,",
									 emberNodeId,
									 SequenceNumber);  

			emberAfPermitJoin(0, TRUE); // Broadcast permit join?		  
		}
	}
	 
	return TRUE; 
}

