// *******************************************************************
// * 707-rf-test-case.c
// *
// *
// * Copyright 2015 by Sengled Corporation. All rights reserved.              
// *******************************************************************
#include "app/framework/include/af.h"
#include "707-rf-test-case.h"
#include "app/framework/plugin/counters/counters.h"

#define APP_ZCL_BUFFER_SIZE  EMBER_AF_MAXIMUM_SEND_PAYLOAD_LENGTH

extern EmberApsFrame globalApsFrame;
extern int16u appZclBufferLen;
extern int8u appZclBuffer[APP_ZCL_BUFFER_SIZE];

extern int16u get_addr_list[64];
extern int8u addr_list_ptr;

EmberEventControl ReadAttrTestControl;

extern  int16u mfgSpecificId;

volatile static int16u testCounter = 0;
#define MAX_TEST_CNT  1000

void ReadAttrTestFunction(void) 
{
    int16u tmp;
    int16u clusterId = 0x0000;
    int16u attrId = 0x0006;
    int16u commandId = ZCL_READ_ATTRIBUTES_COMMAND_ID;
    int8u addr_to_read_cnt = addr_list_ptr - 1; // to avoid use addr_list_ptr++
    int16u dest_addr_to_read = 0;  
    if(addr_list_ptr == 0)
    {
        emberAfGuaranteedPrintln("Start ReadAttrTestFunction: ERROR! addr_list_ptr is 0x%2X, get_addr_list is 0x%2X\n", addr_list_ptr, get_addr_list[addr_to_read_cnt]);
        emberEventControlSetInactive(ReadAttrTestControl);
        return;
    }
    else
    {
        dest_addr_to_read  = get_addr_list[addr_to_read_cnt];   
    }
    
    zclBufferSetup(ZCL_PROFILE_WIDE_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER,
	             clusterId,
	             commandId);

    zclBufferAddWord(attrId);

    emAfApsFrameEndpointSetup(1, 1);

    int8u status = emberAfSendUnicast(EMBER_OUTGOING_DIRECT,
                                dest_addr_to_read,
                                &globalApsFrame,
                                appZclBufferLen,
                                appZclBuffer);

    if (status != EMBER_SUCCESS) {
        emberAfGuaranteedPrintln("Error: CLI Send failed, status: 0x%X", status);
    }
/*
	if(testCounter != MAX_TEST_CNT)
	{
	    testCounter++;
	    emberEventControlSetDelayMS(ReadAttrTestControl, 150);  
	}
	else
	{
	    testCounter = 0;
		emberEventControlSetInactive(ReadAttrTestControl);	
	}
*/    
    emberEventControlSetDelayMS(ReadAttrTestControl, 150);  
}


void SengledTestClusterCommand(int16u dest) 
{
    int16u tmp;
    int16u clusterId = 0xFC03;
    int16u attrId = 0x0000;
    int16u commandId = ZCL_SENGLED_TEST_COMMAND_ID;
    int8u addr_to_read_cnt = addr_list_ptr - 1; // to avoid use addr_list_ptr++
    int16u dest_addr_to_read = 0;  
	
    if(dest == 0)
    {
        emberAfGuaranteedPrintln("Start SengledTestClusterCommand: ERROR! addr_list_ptr is 0x%2X, get_addr_list is 0x%2X\n", addr_list_ptr, get_addr_list[addr_to_read_cnt]);
        return;
    }
    else
    {
        dest_addr_to_read  = get_addr_list[addr_to_read_cnt];   
    }

    
    int16u mfg_code = mfgSpecificId;
    mfgSpecificId = EMBER_AF_MANUFACTURER_CODE; // sengled manufactured ID : 0x1160    
    
    zclBufferSetup(ZCL_CLUSTER_SPECIFIC_COMMAND | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER | ZCL_MANUFACTURER_SPECIFIC_MASK,
	             clusterId,
	             commandId);
    mfgSpecificId = mfg_code;
    
    zclBufferAddWord(attrId);
	zclBufferAddWord(0x55aa);  // tempData 

    emAfApsFrameEndpointSetup(1, 1);

    int8u status = emberAfSendUnicast(EMBER_OUTGOING_DIRECT,
                                dest,
                                &globalApsFrame,
                                appZclBufferLen,
                                appZclBuffer);

    if (status != EMBER_SUCCESS) {
        emberAfGuaranteedPrintln("Error: CLI Send failed, status: 0x%X", status);
    } 
}


boolean emberAfSengledTestClusterServerCallback(int16u tempData)
{
	
	return TRUE;
}

