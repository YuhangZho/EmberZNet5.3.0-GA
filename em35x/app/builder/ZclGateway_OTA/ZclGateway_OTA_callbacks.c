//

// This callback file is created for your convenience. You may add application
// code to this file. If you regenerate this file over a previous version, the
// previous version will be overwritten and any code you have added will be
// lost.

#include "app/framework/include/af.h"

int8u fobidden_ota_image_response;

PGM_P emberCommandRGBCalibrCommandArguments[] = {
	"RGB Type",
	"RGB pwm percentage", 
	NULL
};

void rgbCalibrCommand(void)
{
}

void OTAFileDelete(void)
{
	
	int8u status = emberAfOtaStorageForceDeleteImage();

	switch(status)
	{
	case 0:
	emberAfGuaranteedPrintln("OTA_STORAGE_SUCCESS");
		break;
	case 1:
	emberAfGuaranteedPrintln("OTA_STORAGE_ERROR");
		break;
	case 2:
	emberAfGuaranteedPrintln("OTA_STORAGE_RETURN_DATA_TOO_LONG");
		break;
	case 3:
	emberAfGuaranteedPrintln("OTA_STORAGE_PARTIAL_FILE_FOUND");
		break;
	case 4:
	emberAfGuaranteedPrintln("OTA_STORAGE_OPERATION_IN_PROGRESS");
		break;
	default:
		emberAfGuaranteedPrintln("OTA_STORAGE_UNKNOW");		
	}
	
}

EmberCommandEntry emberAfCustomCommands[] = {	
	emberCommandEntryActionWithDetails("RGB_Calibr", rgbCalibrCommand, "vuu", "RGB factory calibration ", emberCommandRGBCalibrCommandArguments),
	emberCommandEntryAction("ota_delete", OTAFileDelete, "", "ota_delete_sengled_test"),
	
	emberCommandEntryTerminator()

};



