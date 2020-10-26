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
EmberEventControl dimmerDetectControl;
EmberEventControl searchNetworkTimeControl;
EmberEventControl doubleClickEventControl;
EmberEventControl ezmodeEventControl;
EmberEventControl ledBreathEventControl;
EmberEventControl networkOperationEventControl;
EmberEventControl cctDimmingEventControl;
EmberEventControl powerReportEventControl;
EmberEventControl powerConsumptionReportEventControl;
EmberEventControl onOffStatusWhenPowerOffEventControl;
EmberEventControl diagnosticsEventControl;
EmberEventControl powerOffDetectEventControl;

// Event function forward declarations
void DimmerDetectFunction(void);
void SearchNetworkTimeFunction(void);
void DoubleClickEventFunction(void);
void EzmodeEventFunction(void);
void LedBreathEventFunction(void);
void NetworkOperationEventFunction(void);
void CctDimmingEventFunction(void);
void powerReportFunction(void);
void powerConsumptionReportFunction(void);
void OnOffStatusWhenPowerOffEventFunction(void);
void diagnosticsEventFunction(void);
void powerOffDetectEventFunction(void);

// Event function stubs
void DimmerDetectFunction(void) { }
void SearchNetworkTimeFunction(void) { }
void DoubleClickEventFunction(void) { }
void EzmodeEventFunction(void) { }
void LedBreathEventFunction(void) { }
void NetworkOperationEventFunction(void) { }
void CctDimmingEventFunction(void) { }
void powerReportFunction(void) { }
void powerConsumptionReportFunction(void) { }
void OnOffStatusWhenPowerOffEventFunction(void) { }
void diagnosticsEventFunction(void) { }
void powerOffDetectEventFunction(void) { }

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

/** @brief Color Control Cluster Move Color Temperature
 *
 * 
 *
 * @param moveMode   Ver.: always
 * @param rate   Ver.: always
 * @param colorTemperatureMinimum   Ver.: always
 * @param colorTemperatureMaximum   Ver.: always
 */
boolean emberAfColorControlClusterMoveColorTemperatureCallback(int8u moveMode,
                                                               int16u rate,
                                                               int16u colorTemperatureMinimum,
                                                               int16u colorTemperatureMaximum)
{
  return FALSE;
}

/** @brief Color Control Cluster Move To Color Temperature
 *
 * 
 *
 * @param colorTemperature   Ver.: always
 * @param transitionTime   Ver.: always
 */
boolean emberAfColorControlClusterMoveToColorTemperatureCallback(int16u colorTemperature,
                                                                 int16u transitionTime)
{
  return FALSE;
}

/** @brief Color Control Cluster Step Color Temperatue
 *
 * 
 *
 * @param stepMode   Ver.: always
 * @param stepSize   Ver.: always
 * @param transitionTime   Ver.: always
 * @param colorTemperatureMinimum   Ver.: always
 * @param colorTemperatureMaximum   Ver.: always
 */
boolean emberAfColorControlClusterStepColorTemperatueCallback(int8u stepMode,
                                                              int16u stepSize,
                                                              int16u transitionTime,
                                                              int16u colorTemperatureMinimum,
                                                              int16u colorTemperatureMaximum)
{
  return FALSE;
}

/** @brief Color Control Cluster Stop Move Step
 *
 * 
 *
 */
boolean emberAfColorControlClusterStopMoveStepCallback(void)
{
  return FALSE;
}


