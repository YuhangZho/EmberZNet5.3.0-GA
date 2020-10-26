#include PLATFORM_HEADER //compiler/micro specifics, types
#include "stack/include/ember.h"
#include "stack/include/mfglib.h"
#include "hal/hal.h"
#include "app/util/serial/serial.h"
#include "app/util/serial/cli.h"


// Application serial port
#define APP_SERIAL 1

#ifndef MFG_ROBIN

char getCharFromByte(int8u mByteCount)
{
  char baseString[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                      'A', 'B', 'C', 'D', 'E', 'F'} ;
  return baseString[mByteCount];
}
void getMacFromByteToString(int8u macByte[], char* macString)
{
  int8u i;
  for(i = 0; i < 8; i++)
  {
    *macString++ = getCharFromByte(macByte[i]/16) ;
    *macString++ = getCharFromByte(macByte[i]%16) ;
  }
  *macString++ = '\0';
}

//---below fun is added reading token parameter attribute by dar on 2014.11.3---------
//------------------------------------------------------------------------------------
// func:   process read token parameter TOKEN_MFG_EUI_64 -00 command
// *rBuf:  receive buffer
// len:    receive data length
// author: dong airong
// Date:   2014.11.4
//------------------------------------------------------------------------------------
void msProcessGetTokenPara_Eui64_Req(void)
{

  EmberEUI64 TEST_EUI ={00};
  char macContext[17];
  int8u i,temp;
  halCommonGetMfgToken(&TEST_EUI,TOKEN_MFG_CUSTOM_EUI_64);
  
  i = 0;
  while(i < EUI64_SIZE/2)
  {
    temp = TEST_EUI[i];
    TEST_EUI[i] = TEST_EUI[EUI64_SIZE-i-1];
    TEST_EUI[EUI64_SIZE-i-1] = temp;
    i++;
  }

  getMacFromByteToString(TEST_EUI, macContext);

  emberSerialPrintf(APP_SERIAL, "%s\r\nreadmac done!\r\n\r\n", macContext);

}



int8u getHex(int8u *ch)
{
  int8u val = 0;
  int8u tmp;
  
  if ((ch[0]>='0') && (ch[0]<='9'))
  { 
    tmp = ch[0]-'0';  
  }
  else if ((ch[0]>='a') && (ch[0]<='f'))
  {
    tmp = ch[0]-'a'+10;
  }
  else if ((ch[0]>='A') && (ch[0]<='F'))
  {
    tmp = ch[0]-'A'+10;    
  }

  val = tmp<<4;

  if ((ch[1]>='0') && (ch[1]<='9'))
  { 
    tmp = ch[1]-'0';  
  }
  else if ((ch[1]>='a') && (ch[1]<='f'))
  {
    tmp = ch[1]-'a'+10;
  }
  else if ((ch[1]>='A') && (ch[1]<='F'))
  {
    tmp = ch[1]-'A'+10;    
  }

  val += tmp;

  return val;  
}

//------------------------------------------------------------------------------------
// func:   process Set token parameter TOKEN_MFG_EUI_64 -0a command
// *rBuf:  receive buffer
// len:    receive data length
// author: dong airong
// Date:   2014.11.4
//------------------------------------------------------------------------------------
void msProcessSetTokenPara_Eui64_Req(void)
{

  int8u i;
  int8u temp; 

  int8u TEST_EUI[8];
/*
//  char macOrder[8];
  char macContext[16];

 // emberSerialReadLine(APP_SERIAL, macOrder, 7);
 // emberSerialReadLine(APP_SERIAL, macContext, 17);  
 cliGetStringFromArgument(1, macContext, 16);

  // test input for MAC can only write once!
  for(i =0; i < 16; i++)
  {
    if((macContext[i] >= '0' && macContext[i] <= '9')  || 
        (macContext[i] >= 'a' && macContext[i] <= 'f') ||
        (macContext[i] >= 'A' && macContext[i] <= 'F'))
    {
      // do nothing;
    }
    else
    {
      emberSerialPrintf(APP_SERIAL, "setmac Input Error!Please input again!\r\n\r\n");
      return;
    }
  }

  
  for(i = 0; i < 8; i++ )
  { 
    TEST_EUI[i] = getHex(&macContext[i*2]);
  }
  //cliGetStringFromArgument(1, TEST_EUI, 16); 
  i = 0;
  while(i < 4)
  {
    temp = TEST_EUI[i];
    TEST_EUI[i] = TEST_EUI[EUI64_SIZE-i-1];
    TEST_EUI[EUI64_SIZE-i-1] = temp;
    i++;
  }
*/  
  TEST_EUI[0] = 0x00;
  TEST_EUI[1] = 0x1F;
  TEST_EUI[2] = 0xEE;
  TEST_EUI[3] = 0x00;
  TEST_EUI[4] = 0x00;
  TEST_EUI[5] = 0x00;
  TEST_EUI[6] = 0x1E;
  TEST_EUI[7] = 0x49;

  halCommonSetMfgToken(TOKEN_MFG_CUSTOM_EUI_64, &TEST_EUI);
  
  emberSerialPrintf(APP_SERIAL, "setmac done!\r\n\r\n");

}
#endif


