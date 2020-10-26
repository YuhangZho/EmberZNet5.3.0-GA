#include PLATFORM_HEADER
#include "app/framework/include/af.h"
#include "stack/include/ember.h"
#include "stack/include/mfglib.h"
#include "hal/hal.h"
#include "app/util/serial/serial.h"
#include "app/util/serial/cli.h"

extern EmberApsFrame globalApsFrame;
extern int8u appZclBuffer[EMBER_AF_MAXIMUM_SEND_PAYLOAD_LENGTH];
extern int16u appZclBufferLen;

extern int16u get_addr_list[64];

#pragma align sendBuff
int8u sendBuff[128];

#define MAC_PAYLOAD_SIZE 0x74

typedef enum {
  NODE_ADDR  = 0x00,
  EXT_ADDR   = 0x01,
  BROAD_ADDR = 0x02,
} ADDR_MODE;

// DUT to tester: short addr to short addr, unicast, no ACK
// Packete Length = 0x10
// frame control = 0x8841
// sequence = 0x37
// dst PAN ID = 0x8a8a
// dst addr = 0x0000
// src addr = 0x3344
// mac payload = 0x00 0x01 0x02 0x03 0x04
// CRC = 0xXXXX
int8u mac_data_01_a[] = {
	0x41, 0x88, 0x37, 0x8a, 0x8a, 0x00, 0x00, 0x44, 0x33,
	0x04, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00
};

void makePacketCommand(void);
void makePacketCommandData3(void);

PGM_P emberCommandMakepacketCommandArguments[] = {
	"RGB Type",
	"RGB pwm percentage", 
	NULL
};

EmberCommandEntry emberAfCustomCommands[] = {	
	emberCommandEntryActionWithDetails("MakePackets", makePacketCommand, "vuuuu", "Fill the test case packet", emberCommandMakepacketCommandArguments),
	emberCommandEntryActionWithDetails("Data03", makePacketCommandData3, "vuvuuu", "Fill the test case packet", emberCommandMakepacketCommandArguments),
	emberCommandEntryTerminator()

};

void emberMacPassthroughMessageHandler(EmberMacPassthroughType messageType,
                                       EmberMessageBuffer message)
{

}

// last use transmit buffer
// _macTransmitBuffer[0] :  packet len
// _macTransmitBuffer[1] :  frame control(low byte)
// _macTransmitBuffer[2] :  frame control(high byte)
// _macTransmitBuffer[3] :  seq num
extern int8u _macTransmitBuffer[80];

void fillBuffer(int8u* buff, int16u nodeId, int8u length, int16u frame_control, int8u src_addr_mode, int8u dst_addr_mode, int8u max_data_size)
{
  int8u i = 0, p_cnt = 0;
  
  int16u panId = emberAfGetPanId();
  
  EmberEUI64 local_mac={0,0,0,0,0,0,0,0};

  // get local mac addr 
  emberAfGetEui64(local_mac);
  
  // first fill packet length
  buff[i++] = length;
  // frame control 
  buff[i++] = frame_control & 0xff;
  buff[i++] = (frame_control>>8) & 0xff;
  // fill sequence number
  buff[i++] = _macTransmitBuffer[3] + 1;
  // fill PAN ID
  buff[i++] = panId & 0xff;
  buff[i++] = (panId >> 8) & 0xff;
  
  // fill destination addr according to the mode sel
  if (dst_addr_mode == NODE_ADDR)
  {
    buff[i++] = nodeId & 0xff;
	buff[i++] = (nodeId >> 8) & 0xff;
  }
  else if (dst_addr_mode == EXT_ADDR)
  {
    buff[i++] = 0x44;
    buff[i++] = 0x33;
    buff[i++] = 0x22;
    buff[i++] = 0x11;
    buff[i++] = 0x00;
    buff[i++] = 0xee;
    buff[i++] = 0x1f;
    buff[i++] = 0x00;  
  }
  else if (dst_addr_mode == BROAD_ADDR)
  {
    buff[i++] = 0xff;
    buff[i++] = 0xff;
  }

  // fill short addr according to the mode sel
  if (src_addr_mode == NODE_ADDR)
  {
	buff[i++] = 0x00;
	buff[i++] = 0x00;
  }
  else if (src_addr_mode == EXT_ADDR)
  {
    buff[i++] = local_mac[0];
    buff[i++] = local_mac[1];
    buff[i++] = local_mac[2];
    buff[i++] = local_mac[3];
    buff[i++] = local_mac[4];
    buff[i++] = local_mac[5];
    buff[i++] = local_mac[6];
    buff[i++] = local_mac[7];
  }
  else if (src_addr_mode == BROAD_ADDR)
  {
	buff[i++] = 0xff;
	buff[i++] = 0xff;
  }  
  
  // fill the rest mac payload
  if (max_data_size == 0)
  {
    buff[i++] = 0x04;
    for (p_cnt = 1; p_cnt < 5; p_cnt++)
    {
      buff[i++] = 4 - p_cnt;
    }  
  }
  else
  {
    buff[i++] = 0x04;
    for (p_cnt = 1; p_cnt < MAC_PAYLOAD_SIZE; p_cnt++)
    {
      buff[i++] = p_cnt;
    }     
  }
  
  // fill the crc
  buff[i++] = 0x00;
  buff[i++] = 0x00;
}

void makePacketCommand(void)
{
	int16u frame_control = (int16u)emberUnsignedCommandArgument(0);
        int8u packet_len = (int8u)emberUnsignedCommandArgument(1);
        // mode: 
        //     0 -> short addr
        //     1 -> ext addr
        //     2 -> 0xffff
        int8u src_addr_mode = (int8u)emberUnsignedCommandArgument(2);
	int8u dst_addr_mode = (int8u)emberUnsignedCommandArgument(3);
        // 0 -> normal data
        // 1 -> max size data
	int8u fill_max_data_size_bool = (int8u)emberUnsignedCommandArgument(4);
	int8u status = 0;
	int8u index = 0;
	int16u nodeId = get_addr_list[0];
	
	emberAfGuaranteedPrintln("make packet sent to 0x%2x: \r\n   frameControl: 0x%2x, packet_len: %x, src_addr_mode:%x, dst_addr_mode:%x, fill_max_data_size_bool: %x",
		nodeId, frame_control, packet_len, src_addr_mode, dst_addr_mode, fill_max_data_size_bool);
 
        //emberAfGuaranteedPrintln("_macTransmitBuffer: ");
        //for(index = 0; index < 0x80; index++)
        //{
        //  emberAfGuaranteedPrintln("%x ", _macTransmitBuffer[index]);
        //}
           
        mfglibStart(NULL);
        mfglibSetChannel(11);
        mfglibSetPower(0, 8);
        fillBuffer((int8u*) &sendBuff, nodeId, packet_len, frame_control, 
                   src_addr_mode, dst_addr_mode, fill_max_data_size_bool);
        
        status = mfglibSendPacket(sendBuff, 0);
        emberAfGuaranteedPrintln("send status is %x",status);
        mfglibEnd();
}

void fillBuffer_data3(int8u* buff, int16u nodeId, int8u length, int16u frame_control, int16u dst_panid, int8u src_addr_mode, int8u dst_addr_mode, int8u max_data_size)
{
  int8u i = 0, p_cnt = 0;
  
  int16u panId = emberAfGetPanId();
  
  EmberEUI64 local_mac={0,0,0,0,0,0,0,0};

  // get local mac addr 
  emberAfGetEui64(local_mac);
  
  // first fill packet length
  buff[i++] = length;
  // frame control 
  buff[i++] = frame_control & 0xff;
  buff[i++] = (frame_control>>8) & 0xff;
  // fill sequence number
  buff[i++] = _macTransmitBuffer[3] + 1;
  // fill PAN ID
  buff[i++] = dst_panid & 0xff;
  buff[i++] = (dst_panid >> 8) & 0xff;
  
  // fill destination addr according to the mode sel
  if (dst_addr_mode == NODE_ADDR)
  {
    buff[i++] = nodeId & 0xff;
	buff[i++] = (nodeId >> 8) & 0xff;
  }
  else if (dst_addr_mode == EXT_ADDR)
  {
    buff[i++] = 0x44;
    buff[i++] = 0x33;
    buff[i++] = 0x22;
    buff[i++] = 0x11;
    buff[i++] = 0x00;
    buff[i++] = 0xee;
    buff[i++] = 0x1f;
    buff[i++] = 0x00;  
  }
  else if (dst_addr_mode == BROAD_ADDR)
  {
    buff[i++] = 0xff;
    buff[i++] = 0xff;
  }

  // set local panid
  buff[i++] = panId & 0xff;
  buff[i++] = (panId >> 8) & 0xff;

  // fill short addr according to the mode sel
  if (src_addr_mode == NODE_ADDR)
  {
	buff[i++] = 0x00;
	buff[i++] = 0x00;
  }
  else if (src_addr_mode == EXT_ADDR)
  {
    buff[i++] = local_mac[0];
    buff[i++] = local_mac[1];
    buff[i++] = local_mac[2];
    buff[i++] = local_mac[3];
    buff[i++] = local_mac[4];
    buff[i++] = local_mac[5];
    buff[i++] = local_mac[6];
    buff[i++] = local_mac[7];
  }
  else if (src_addr_mode == BROAD_ADDR)
  {
	buff[i++] = 0xff;
	buff[i++] = 0xff;
  }  
  
  // fill the rest mac payload
  if (max_data_size == 0)
  {
    buff[i++] = 0x04;
    for (p_cnt = 1; p_cnt < 5; p_cnt++)
    {
      buff[i++] = 4 - p_cnt;
    }  
  }
  else
  {
    buff[i++] = 0x04;
    for (p_cnt = 1; p_cnt < MAC_PAYLOAD_SIZE; p_cnt++)
    {
      buff[i++] = p_cnt;
    }     
  }
  
  // fill the crc
  buff[i++] = 0x00;
  buff[i++] = 0x00;
}

void makePacketCommandData3(void)
{
	int16u frame_control = (int16u)emberUnsignedCommandArgument(0);
    int8u packet_len = (int8u)emberUnsignedCommandArgument(1);
	int16u dst_panid = (int16u)emberUnsignedCommandArgument(2);
    // mode: 
    //     0 -> short addr
    //     1 -> ext addr
    //     2 -> 0xffff
    int8u src_addr_mode = (int8u)emberUnsignedCommandArgument(3);
	int8u dst_addr_mode = (int8u)emberUnsignedCommandArgument(4);
    // 0 -> normal data
    // 1 -> max size data
	int8u fill_max_data_size_bool = (int8u)emberUnsignedCommandArgument(5);
	int8u status = 0;
	int8u index = 0;
	int16u nodeId = get_addr_list[0];
	
	emberAfGuaranteedPrintln("make packet sent to 0x%2x: \r\n   frameControl: 0x%2x, packet_len: %x, src_addr_mode:%x, dst_addr_mode:%x, fill_max_data_size_bool: %x",
		nodeId, frame_control, packet_len, src_addr_mode, dst_addr_mode, fill_max_data_size_bool);
       
    mfglibStart(NULL);
    //mfglibSetChannel(11);
    mfglibSetPower(0, 8);
    fillBuffer_data3((int8u*) &sendBuff, nodeId, packet_len, frame_control, dst_panid,
               src_addr_mode, dst_addr_mode, fill_max_data_size_bool);
    
    status = mfglibSendPacket(sendBuff, 0);
    emberAfGuaranteedPrintln("send status is %x",status);
    mfglibEnd();
}

