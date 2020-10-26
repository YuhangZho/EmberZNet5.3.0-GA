// *****************************************************************************
// * ota-storage-custom-sample.c
// *
// * Zigbee Over-the-air bootload cluster for upgrading firmware and 
// * downloading specific file.
// *
// * THIS IS A TEST IMPLEMENTATION.  It defines a single static, NULL, upgrade 
// * file that contains the upgrade information for a single manufacturer
// * and device ID.  The payload is a real OTA file but dummy data.
// *
// * This can serve as both the storage for the OTA client and the server.
// * The data is stored in RAM and thus is limited by the size of available
// * memory.
// * 
// * Copyright 2010 by Ember Corporation. All rights reserved.              *80*
// *****************************************************************************

#include "app/framework/include/af.h"
#include "callback.h"
#include "app/framework/plugin/ota-common/ota.h"
#include "app/framework/plugin/ota-storage-common/ota-storage.h"

#include "app/framework/util/util.h"
#include "app/framework/util/common.h"

//------------------------------------------------------------------------------
// Globals

// This is used to store both the static SERVER image, and to hold
// the temporary CLIENT image being downloaded.  It can't do both at the same
// time so a client download will wipe out the server image.

//------------------------------------------------------------------------------
extern boolean otaFileDataUpdata;
int8u* otaUpdataBufferPtr;

boolean GetOtaFileState(void);
void SendGetOtaFileCommand(int32u offset, int32u length);

int8u* GetOtaUpdataBuffer(void)
{
  return otaUpdataBufferPtr;
}
//*********************************************************************
// Ota End Indicated
//*********************************************************************
boolean emberAfOtaStorageDriverInitCallback(void)
{
  return GetOtaFileState();
  //return TRUE;
}
boolean emberAfOtaStorageDriverReadCallback(int32u offset, 
                                            int32u length,
                                            int8u* returnData)
{
  otaFileDataUpdata = FALSE;
  otaUpdataBufferPtr = returnData;
  SendGetOtaFileCommand(offset, length);
  while (otaFileDataUpdata == FALSE)
  { 
    halResetWatchdog();
    emberProcessCommand(NULL, APP_SERIAL);
  }
  otaFileDataUpdata = FALSE;
  
  return TRUE;
}

boolean emberAfOtaStorageDriverWriteCallback(const int8u* dataToWrite,
                                             int32u offset, 
                                             int32u length)
{
  //MEMCOPY(storage + offset, dataToWrite, length);
  return TRUE;
}

int32u emberAfOtaStorageDriverRetrieveLastStoredOffsetCallback(void)
{
  return 0xffff;
}

void emberAfOtaStorageDriverDownloadFinishCallback(int32u finalOffset)
{
  
}

void emAfOtaStorageDriverCorruptImage(int16u index)
{
  
}

int16u emAfOtaStorageDriveGetImageSize(void)
{
  return 0xffff;
}

EmberAfOtaStorageStatus emberAfOtaStorageDriverInvalidateImageCallback(void)
{
  return EMBER_AF_OTA_STORAGE_SUCCESS;
}

int32u emberAfOtaStorageDriverMaxDownloadSizeCallback(void)
{
  return 0xffff;
}

void emAfOtaStorageDriverInfoPrint(void)
{
  otaPrintln("Storage Driver:       OTA Simple Storage RAM");
  otaPrintln("Data Size (bytes):    %d", 0xffff);
}

EmberAfOtaStorageStatus emberAfOtaStorageDriverPrepareToResumeDownloadCallback(void)
{
  return EMBER_AF_OTA_STORAGE_SUCCESS;
}
