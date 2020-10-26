//#include "app/framework/include/af.h"
//#include "app/framework/util/attribute-storage.h"
#include PLATFORM_HEADER //compiler/micro specifics, types
#include "stack/include/ember.h"
#include "stack/include/mfglib.h"
#include "hal/hal.h"
#include "app/util/serial/serial.h"
#include "app/util/serial/cli.h"


int8u halEepromInit(void);

boolean TheFlashIsRight(void)
{
  if (EEPROM_SUCCESS == halEepromInit())
    return TRUE;
  else
    return FALSE;  
}
