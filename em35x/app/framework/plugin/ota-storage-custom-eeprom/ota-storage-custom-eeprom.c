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

#include "app/framework/plugin/eeprom/eeprom.h"

#include "ota-storage-custom-eeprom.h"


//------------------------------------------------------------------------------
// NOTE:  The magic number here is the "Ember" magic number.
//   It is not the same as the OTA file magic number.
//   It is used solely to verify the validity of the 
//   meta-data stored ahead of the OTA file.
boolean emAfOtaStorageCheckDownloadMetaData(void)
{
  int8u magicNumberExpected[] = { MAGIC_NUMBER , VERSION_NUMBER };
  int8u magicNumberActual[MAGIC_NUMBER_SIZE + VERSION_NUMBER_SIZE];

  emberAfPluginEepromRead(IMAGE_INFO_START + MAGIC_NUMBER_OFFSET,
                 magicNumberActual,
                 MAGIC_NUMBER_SIZE + VERSION_NUMBER_SIZE);
  if (0 != MEMCOMPARE(magicNumberExpected, 
                      magicNumberActual, 
                      MAGIC_NUMBER_SIZE + VERSION_NUMBER_SIZE)) {
    debugPrint("Magic Number or version for download meta-data is invalid");
    debugFlush();
    return FALSE;
  }

  return TRUE;
}

int32u emAfOtaStorageReadInt32uFromEeprom(int32u realOffset)
{
  int8u value[4];
  emberAfPluginEepromRead(realOffset, value, 4);
  return (value[0]
          + ((int32u)value[1] << 8)
          + ((int32u)value[2] << 16)
          + ((int32u)value[3] << 24));
}

void emAfOtaStorageWriteInt32uToEeprom(int32u value, int32u realOffset)
{
  int32u oldValue = emAfOtaStorageReadInt32uFromEeprom(realOffset);
  if (oldValue != value) {
    int8u data[4];
    data[0] = value;
    data[1] = (int8u)(value >> 8);
    data[2] = (int8u)(value >> 16);
    data[3] = (int8u)(value >> 24);

    emberAfPluginEepromWrite(realOffset, data, 4);
  }
}

void emAfOtaStorageEepromInit(void)
{
  // Older drivers do not have an EEPROM info structure that we can reference
  // so we must just assume they are okay.  
  if (emberAfPluginEepromInfo() != NULL) {
    // OTA code must match the capabilities of the part.  This code
    // assumes that a page erase prior to writing data is NOT required.
    assert((emberAfPluginEepromInfo()->capabilitiesMask
            & EEPROM_CAPABILITIES_PAGE_ERASE_REQD)
           == 0);
  }
}

boolean emberAfOtaStorageDriverInitCallback(void)
{
  // Older drivers do not have an EEPROM info structure that we can reference
  // so we must just assume they are okay.  
  if (emberAfPluginEepromInfo() != NULL) {
    assert(emberAfPluginEepromInfo()->partSize >= OTA_EEPROM_SIZE);
  }
  emAfOtaStorageEepromInit();

  return TRUE;
}

boolean emberAfOtaStorageDriverReadCallback(int32u offset, 
                                            int32u length,
                                            int8u* returnData)
{
  int8u status;
  int32u realOffset = IMAGE_INFO_START + OTA_HEADER_INDEX + offset;

  status = emberAfPluginEepromRead(realOffset, returnData, (int16u)length);
  otaPrintln("OTA read status: %x", status);
  
  if (status != EEPROM_SUCCESS) {
    return FALSE;
  }
  
  return TRUE;
}
boolean emberAfOtaStorageDriverWriteCallback(const int8u* dataToWrite,
                                             int32u offset, 
                                             int32u length)
{
  int8u status;
  int32u realOffset = IMAGE_INFO_START + OTA_HEADER_INDEX + offset;

  status = emberAfPluginEepromWrite(realOffset, dataToWrite, (int16u)length);
  otaPrintln("OTA write status: %x", status);
  
  if (status != EEPROM_SUCCESS) {
    return FALSE;
  }
  
  return TRUE;
}

int32u emberAfOtaStorageDriverRetrieveLastStoredOffsetCallback(void)
{
  int32u offset;

  if (!emAfOtaStorageCheckDownloadMetaData()) {
    return 0;
  }

  offset = emAfOtaStorageReadInt32uFromEeprom(IMAGE_INFO_START
                                              + SAVED_DOWNLOAD_OFFSET_INDEX);
  if (offset == 0xFFFFFFFFL) {
    return 0;
  }
  return offset;
}
void emAfStorageEepromUpdateDownloadOffset(int32u offset, boolean finalOffset)
{
  int32u oldDownloadOffset = 
    emberAfOtaStorageDriverRetrieveLastStoredOffsetCallback();

  if (finalOffset
      || offset == 0
      || (offset > SAVE_RATE
          && (oldDownloadOffset + SAVE_RATE) <= offset)) {
    // The actual offset we are writing TO is the second parameter.
    // The data we are writing (first param) also happens to be an offset but
    // is not a location for the write operation in this context.
    debugFlush();
    debugPrint("Recording download offset: 0x%4X", offset);
    debugFlush();

    emAfOtaStorageWriteInt32uToEeprom(offset, IMAGE_INFO_START + SAVED_DOWNLOAD_OFFSET_INDEX);
    //printImageInfoStartData();
  }
}

void emberAfOtaStorageDriverDownloadFinishCallback(int32u finalOffset)
{
  debugPrint("Noting final download offset 0x%4X", finalOffset);
  emAfStorageEepromUpdateDownloadOffset(finalOffset, 
                                        TRUE);  // final offset?
  emberAfPluginEepromFlushSavedPartialWrites();
  return;
}

EmberAfOtaStorageStatus emberAfOtaStorageDriverInvalidateImageCallback(void)
{
  int8u zeroMagicNumber[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  // Wipe out the magic number in the file and the Header length field.
  // EEPROM driver requires a write of at least 8 bytes in length.
  if (!emberAfOtaStorageDriverWriteCallback(zeroMagicNumber, 
                                            0,      // offset
                                            sizeof(zeroMagicNumber))){    // length
    return EMBER_AF_OTA_STORAGE_ERROR;
  }

  // Set the offset to 0 to indicate on reboot that there is no previous image
  // to resume downloading.
  emberAfOtaStorageDriverDownloadFinishCallback(0);

  return EMBER_AF_OTA_STORAGE_SUCCESS;
}

int32u emberAfOtaStorageDriverMaxDownloadSizeCallback(void)
{
  return (OTA_EEPROM_SIZE - MAX_IMAGE_INFO_AND_OTA_HEADER_SIZE);
}

void emAfOtaStorageDriverInfoPrint(void)
{
  int32u downloadOffset = emberAfOtaStorageDriverRetrieveLastStoredOffsetCallback();

  otaPrintln("Storage Driver:            OTA Simple Storage EEPROM Plugin");
  otaPrintFlush();
  otaPrintln("Read Modify Write Support: " READ_MODIFY_WRITE_SUPPORT_TEXT);
  otaPrintFlush();
  otaPrintln("SOC Bootloading Support:   " SOC_BOOTLOADING_SUPPORT_TEXT);
  otaPrintFlush();
  otaPrintln("Current Download Offset:   0x%4X", downloadOffset);

#if defined(SOC_BOOTLOADING_SUPPORT)
  otaPrintFlush();
  otaPrintln("EBL Start Offset:          0x%4X", emAfGetEblStartOffset());
  otaPrintFlush();
#endif

  otaPrintln("EEPROM Start:              0x%4X", EEPROM_START);
  otaPrintFlush();
  otaPrintln("EEPROM End:                0x%4X", EEPROM_END);
  otaPrintFlush();
  otaPrintln("Image Info Start:          0x%4X", IMAGE_INFO_START);
  otaPrintFlush();
  otaPrintln("Save Rate (bytes)          0x%4X", SAVE_RATE);
  otaPrintFlush();
  otaPrintln("Offset of download offset  0x%4X", IMAGE_INFO_START + SAVED_DOWNLOAD_OFFSET_INDEX);
  otaPrintFlush();
  otaPrintln("Offset of EBL offset:      0x%4X", IMAGE_INFO_START + EBL_START_OFFSET_INDEX);
  otaPrintFlush();
  otaPrintln("Offset of image start:     0x%4X", IMAGE_INFO_START + OTA_HEADER_INDEX);
  otaPrintFlush();

#if defined(DEBUG_PRINT)
  {
    int8u data[DATA_SIZE];

    otaPrintln("\nData at EEPROM Start");
    emberAfPluginEepromRead(EEPROM_START, data, DATA_SIZE);
    emberAfPrintCert(data);  // certs are 48 bytes long
    otaPrintFlush();
  }
  printImageInfoStartData();
#endif
}

EmberAfOtaStorageStatus emberAfOtaStorageDriverPrepareToResumeDownloadCallback(void)
{
  return EMBER_AF_OTA_STORAGE_SUCCESS;
}
