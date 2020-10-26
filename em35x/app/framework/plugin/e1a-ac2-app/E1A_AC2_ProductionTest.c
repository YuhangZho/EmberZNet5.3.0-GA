
#include "E1A_AC2_Public_Head.h"



//*******************************************************************************
// 1-10: level: 10% -100%
// 11-20: cct: 10% -100%
// 99: firmware version
//*******************************************************************************
int8u firmwareVersion[] = "E1A_AC2-v1.0.3";
boolean emberProcessCommandSendled(int8u *input, int8u sizeOrPort)
{
  int8u dat;
  
  while (EMBER_SUCCESS == emberSerialReadByte(sizeOrPort, &dat))
  {
    if ((1 <= dat) && (dat <= 10)) 
    {
      powerCount = POWER_COUNT_MAX;
      SetPwmLevel(TICS_PER_PERIOD-600*dat, DIMMING_CTRL_PIN);
      emberSerialWriteData(APP_SERIAL, &dat, 1);
    }
    else if ((11 <= dat) && (dat <= 20))
    { 
      powerCount = POWER_COUNT_MAX;
      SetPwmLevel((dat-10)*600, COLOR_TEMP_CTRL_PIN);
      emberSerialWriteData(APP_SERIAL, &dat, 1);
    }
    else if (99 == dat)
    { emberSerialWriteData(APP_SERIAL, firmwareVersion, sizeof(firmwareVersion));}
  }

  return FALSE;
}



