//

// This callback file is created for your convenience. You may add application
// code to this file. If you regenerate this file over a previous version, the
// previous version will be overwritten and any code you have added will be
// lost.

#include "app/framework/include/af.h"

extern EmberApsFrame globalApsFrame;
extern int8u appZclBuffer[EMBER_AF_MAXIMUM_SEND_PAYLOAD_LENGTH];
extern int16u appZclBufferLen;
static boolean failed_mark = 0;


void rgbCalibrCommand(void);
void diagMeTestCommand(void);



PGM_P emberCommandRGBCalibrCommandArguments[] = {
	"RGB Type",
	"RGB pwm percentage", 
	NULL
};

EmberCommandEntry emberAfCustomCommands[] = {	
	emberCommandEntryActionWithDetails("RGB_Calibr", rgbCalibrCommand, "vuw", "RGB factory calibration ", emberCommandRGBCalibrCommandArguments),
	emberCommandEntryActionWithDetails("Diag_Me", diagMeTestCommand, "", "Diagnose code itself ", NULL),

	emberCommandEntryTerminator()

};

					  

void sengledBufferSetup(int8u frameType, int16u clusterId, int8u commandId) 
{
  int8u index = 0;
  emAfApsFrameClusterIdSetup(clusterId);
  appZclBuffer[index++] = (frameType
                           | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER
                           | ZCL_MANUFACTURER_SPECIFIC_MASK);
  

  appZclBuffer[index++] = (int8u)0x1160;
  appZclBuffer[index++] = (int8u)(0x1160 >> 8);

  appZclBuffer[index++] = emberAfNextSequence();
  appZclBuffer[index++] = commandId;  
  appZclBufferLen = index;
}

void rgbCalibrCommand(void)
{
	int16u nodeId = (int16u)emberUnsignedCommandArgument(0);
	int8u type = (int8u)emberUnsignedCommandArgument(1);
	int32u rgb_percentage_integer = (int32u)emberUnsignedCommandArgument(2);
	int8u status;

	//emberAfGuaranteedPrintln("RGB Start,Addr: 0x%2x, Type:0x%x, RGB_Percentage: 0x%2x",nodeId,type,rgb_percentage_integer);

	//ZCL_DISABLE_DEFAULT_RESPONSE_MASK, 
	sengledBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND \
					| ZCL_MANUFACTURER_SPECIFIC_MASK \
					| ZCL_FRAME_CONTROL_CLIENT_TO_SERVER,
					ZCL_RGB_CALIRATION_CLUSTER_ID, 
					ZCL_RGB_CALIBRATION_COMMAND_ID);
	

	zclBufferAddByte(type);  
	zclBufferAddInt32(rgb_percentage_integer);  

	emAfApsFrameEndpointSetup(1, 1);  
	if (nodeId == EMBER_BROADCAST_ADDRESS
		|| nodeId == EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS
		|| nodeId == EMBER_SLEEPY_BROADCAST_ADDRESS) 
	{
		emberAfSendBroadcast(nodeId,
							&globalApsFrame,
							appZclBufferLen,
							appZclBuffer);
	}
	else
	{
		status = emberAfSendUnicast(EMBER_OUTGOING_DIRECT,
								nodeId,
								&globalApsFrame,
								appZclBufferLen,
								appZclBuffer);		
	}

}

void diagMeTestCommand(void)
{
	int16u nodeId = EMBER_SLEEPY_BROADCAST_ADDRESS; // broadcast
	int8u status = 0xff;
	int8u *string = "1234567890abcdefghijklmnopqlsduvwxyz";
	emberAfGuaranteedPrintln("Prepare diag...");
	sengledBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND \
					| ZCL_MANUFACTURER_SPECIFIC_MASK \
					| ZCL_FRAME_CONTROL_CLIENT_TO_SERVER,
					ZCL_DIAG_CODE_CLUSTER_ID, 
					ZCL_DIAG_CODE_BROADCAST_COMMAND_ID);

	zclBufferAddBuffer(string,35);
	
	emAfApsFrameEndpointSetup(1, EMBER_BROADCAST_ENDPOINT);  

	status = emberAfSendBroadcast(nodeId,
					&globalApsFrame,
					appZclBufferLen,
					appZclBuffer);
	
	if (status == EMBER_SUCCESS)
	{
		emberAfGuaranteedPrintln("Prepare Finished and Send!");
	}
	else 
	{
		emberAfGuaranteedPrintln("status is: 0x%x",status);
	}
}


boolean emberAfSengledMobileScenseClusterMobileControlCallback(int16u controlType,
															int16u controlData,
															int16u transitionTime)
{
    //0x0001：onoff键ID；
    //0x0002：暖色温键ID；
    //0x0003：冷色温键ID；
    //0x0004：亮度增加键ID；
    //0x0005：亮度降低键ID；
    //0x0006：营业模式键ID；
    //0x0007：日间模式键ID；
    //0x0008：准备模式键ID；
    //0x0009：夜间模式键ID；
    //0x000a：遥控器保留；
  /*
    switch (controlType)
    {
    case 0x0001:
    break;  
    case 0x0002:
    break;  
    case 0x0003:
      break;
    case 0x0004:
      break;
    case 0x0005:
      break;
    case 0x0006:
      break;
    case 0x0007:
      break;
    case 0x0008:
      break;
    case 0x0009:
      break;
    case 0x000a:
      break;
    default:
      break;
    }
  */
  emberAfGuaranteedPrintln("Remote control: %2x",controlType);          // Robin add for debug	
    return TRUE;
}


