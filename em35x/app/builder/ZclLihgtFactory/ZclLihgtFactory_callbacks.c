//

// This callback file is created for your convenience. You may add application
// code to this file. If you regenerate this file over a previous version, the
// previous version will be overwritten and any code you have added will be
// lost.

#include "app/framework/include/af.h"


/** @brief Mobile Scense Cluster Mobile Control
 *
 * 
 *
 * @param controlType   Ver.: always
 * @param controlData   Ver.: always
 * @param transitionTime   Ver.: always
 */
boolean emberAfSengledMobileScenseClusterMobileControlCallback(int16u controlType,
                                                               int16u controlData,
                                                               int16u transitionTime)
{
  return FALSE;
}

boolean emberAfSengledRgbTestClusterRGB_PWMControlCallback(int8u RGB,
                                                           int16u PWMData)
{
	emberAfGuaranteedPrintln("RGB Receive, Type: 0x%x, Percentage: 0x%2x",RGB, PWMData);
	
	return FALSE;
}


