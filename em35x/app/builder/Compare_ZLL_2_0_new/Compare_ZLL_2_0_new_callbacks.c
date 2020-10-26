//

// This callback file is created for your convenience. You may add application
// code to this file. If you regenerate this file over a previous version, the
// previous version will be overwritten and any code you have added will be
// lost.

#include "app/framework/include/af.h"


/** @brief ZLL Commissioning Cluster Scan Response
 *
 * 
 *
 * @param transaction   Ver.: always
 * @param rssiCorrection   Ver.: always
 * @param zigbeeInformation   Ver.: always
 * @param zllInformation   Ver.: always
 * @param keyBitmask   Ver.: always
 * @param responseId   Ver.: always
 * @param extendedPanId   Ver.: always
 * @param networkUpdateId   Ver.: always
 * @param logicalChannel   Ver.: always
 * @param panId   Ver.: always
 * @param networkAddress   Ver.: always
 * @param numberOfSubDevices   Ver.: always
 * @param totalGroupIds   Ver.: always
 * @param endpointId   Ver.: always
 * @param profileId   Ver.: always
 * @param deviceId   Ver.: always
 * @param version   Ver.: always
 * @param groupIdCount   Ver.: always
 */
boolean emberAfZllCommissioningClusterScanResponseCallback(int32u transaction,
                                                           int8u rssiCorrection,
                                                           int8u zigbeeInformation,
                                                           int8u zllInformation,
                                                           int16u keyBitmask,
                                                           int32u responseId,
                                                           int8u* extendedPanId,
                                                           int8u networkUpdateId,
                                                           int8u logicalChannel,
                                                           int16u panId,
                                                           int16u networkAddress,
                                                           int8u numberOfSubDevices,
                                                           int8u totalGroupIds,
                                                           int8u endpointId,
                                                           int16u profileId,
                                                           int16u deviceId,
                                                           int8u version,
                                                           int8u groupIdCount)
{
  return FALSE;
}

/** @brief ZLL Commissioning Cluster Device Information Response
 *
 * 
 *
 * @param transaction   Ver.: always
 * @param numberOfSubDevices   Ver.: always
 * @param startIndex   Ver.: always
 * @param deviceInformationRecordCount   Ver.: always
 * @param deviceInformationRecordList   Ver.: always
 */
boolean emberAfZllCommissioningClusterDeviceInformationResponseCallback(int32u transaction,
                                                                        int8u numberOfSubDevices,
                                                                        int8u startIndex,
                                                                        int8u deviceInformationRecordCount,
                                                                        int8u* deviceInformationRecordList)
{
  return FALSE;
}

/** @brief ZLL Commissioning Cluster Network Start Response
 *
 * 
 *
 * @param transaction   Ver.: always
 * @param status   Ver.: always
 * @param extendedPanId   Ver.: always
 * @param networkUpdateId   Ver.: always
 * @param logicalChannel   Ver.: always
 * @param panId   Ver.: always
 */
boolean emberAfZllCommissioningClusterNetworkStartResponseCallback(int32u transaction,
                                                                   int8u status,
                                                                   int8u* extendedPanId,
                                                                   int8u networkUpdateId,
                                                                   int8u logicalChannel,
                                                                   int16u panId)
{
  return FALSE;
}

/** @brief ZLL Commissioning Cluster Network Join Router Response
 *
 * 
 *
 * @param transaction   Ver.: always
 * @param status   Ver.: always
 */
boolean emberAfZllCommissioningClusterNetworkJoinRouterResponseCallback(int32u transaction,
                                                                        int8u status)
{
  return FALSE;
}

/** @brief ZLL Commissioning Cluster Network Join End Device Response
 *
 * 
 *
 * @param transaction   Ver.: always
 * @param status   Ver.: always
 */
boolean emberAfZllCommissioningClusterNetworkJoinEndDeviceResponseCallback(int32u transaction,
                                                                           int8u status)
{
  return FALSE;
}

/** @brief ZLL Commissioning Cluster Endpoint Information
 *
 * 
 *
 * @param ieeeAddress   Ver.: always
 * @param networkAddress   Ver.: always
 * @param endpointId   Ver.: always
 * @param profileId   Ver.: always
 * @param deviceId   Ver.: always
 * @param version   Ver.: always
 */
boolean emberAfZllCommissioningClusterEndpointInformationCallback(int8u* ieeeAddress,
                                                                  int16u networkAddress,
                                                                  int8u endpointId,
                                                                  int16u profileId,
                                                                  int16u deviceId,
                                                                  int8u version)
{
  return FALSE;
}

/** @brief ZLL Commissioning Cluster Get Group Identifiers Response
 *
 * 
 *
 * @param total   Ver.: always
 * @param startIndex   Ver.: always
 * @param count   Ver.: always
 * @param groupInformationRecordList   Ver.: always
 */
boolean emberAfZllCommissioningClusterGetGroupIdentifiersResponseCallback(int8u total,
                                                                          int8u startIndex,
                                                                          int8u count,
                                                                          int8u* groupInformationRecordList)
{
  return FALSE;
}

/** @brief ZLL Commissioning Cluster Get Endpoint List Response
 *
 * 
 *
 * @param total   Ver.: always
 * @param startIndex   Ver.: always
 * @param count   Ver.: always
 * @param endpointInformationRecordList   Ver.: always
 */
boolean emberAfZllCommissioningClusterGetEndpointListResponseCallback(int8u total,
                                                                      int8u startIndex,
                                                                      int8u count,
                                                                      int8u* endpointInformationRecordList)
{
  return FALSE;
}


