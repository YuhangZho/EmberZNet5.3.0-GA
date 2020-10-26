/*
 * File: at45db021d.c
 * Description: SPI Interface to Atmel AT45DB021D Serial Flash Memory
 * containing 264kBytes of memory.
 *
 * This file provides an interface to the AT45DB021D flash memory to allow
 * writing, reading and status polling.
 *
 * The write function uses command 82 to write data to the flash buffer
 * which then erases the page and writes the memory.
 *
 * The read function uses command D2 to read directly from memory without
 * using the buffer.
 *
 * The Ember remote storage code operates using 128 byte blocks of data. This
 * interface will write two 128 byte blocks to each remote page utilizing
 * 256 of the 264 bytes available per page. This format effectively uses
 * 256kBytes of memory.
 *
 * Copyright 2010 by Ember Corporation. All rights reserved.                *80*
 *
 */

/*
 * When EEPROM_USES_SHUTDOWN_CONTROL is defined in the board header, 
 * logic is enabled in the EEPROM driver which drives PB7 high upon EEPROM 
 * initialization.  In Ember reference designs, PB7 acts as an EEPROM enable 
 * pin and therefore must be driven high in order to use the EEPROM.  
 * This option is intended to be enabled when running app-bootloader on 
 * designs based on current Ember reference designs.
 */

#include PLATFORM_HEADER
#include "hal/micro/bootloader-eeprom.h"
#include "bootloader-common.h"
#include "bootloader-serial.h"
#include "hal/micro/micro.h"
#include BOARD_HEADER
#include "hal/micro/cortexm3/memmap.h"

#define THIS_DRIVER_VERSION (0x0109)

#if !(BAT_VERSION == THIS_DRIVER_VERSION)
  #error External Flash Driver must be updated to support new API requirements
#endif


//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~ Generic SPI Routines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
static int8u halSpiReadWrite(int8u txData)
{
  int8u rxData;

  SC2_DATA = txData;
  while( (SC2_SPISTAT&SC_SPITXIDLE) != SC_SPITXIDLE) {} //wait to finish
  if ((SC2_SPISTAT&SC_SPIRXVAL) != SC_SPIRXVAL)
    rxData = 0xff;
  else
    rxData = SC2_DATA;

  return rxData;
}

static void halSpiWrite(int8u txData)
{
  SC2_DATA = txData;
  while( (SC2_SPISTAT&SC_SPITXIDLE) != SC_SPITXIDLE) {} //wait to finish
  (void) SC2_DATA;
}

static int8u halSpiRead(void)
{
  return halSpiReadWrite(0xFF);
}

//
// ~~~~~~~~~~~~~~~~~~~~~~~~ Device Specific Interface ~~~~~~~~~~~~~~~~~~~~~~~~~~
//

#define DEVICE_SIZE       (256ul * 1024ul)   // 256 kBytes

#define DEVICE_PAGE_SZ     (4ul * 1024ul)
#define DEVICE_PAGE_MASK   (4ul * 1024ul-1)
#define DEVICE_WORD_SIZE   (1)

#define AT_MANUFACTURER_ID 0xc2

// Command List  
#define MX25L8006_WREN          (0x06)        ///< Write Enable  
#define MX25L8006_WRDI          (0x04)        ///< Write Disable  
#define MX25L8006_RDID          (0x9F)        ///< Read ident- ification  
#define MX25L8006_RDSR          (0x05)        ///< Read status register  
#define MX25L8006_WRSR          (0x01)        ///< Write status register  
#define MX25L8006_READ          (0x03)        ///< Read data  
#define MX25L8006_FAST_READ     (0x0B)        ///< Fast read data  
#define MX25L8006_SE            (0x20)        ///< Sector Erase  
#define MX25L8006_BE            (0xD8)        ///< Block Erase  
#define MX25L8006_CE            (0x60)        ///< Chip Erase  
#define MX25L8006_PP            (0x02)        ///< Page Program  
#define MX25L8006_DP            (0xB9)        ///< Deep Power Down  
#define MX25L8006_RDP           (0xAB)        ///< Release from Deep Power - down  
#define MX25L8006_RES           (0xAB)        ///< Read Electornic ID  
#define MX25L8006_REMS          (0x90)        ///< Read Electronic Manufacturer & Device ID  
#define MX25L8006_RDSCUR        (0x2B)  
#define MX25L8006_WRSCUR        (0x2F) 


// Atmel Op Codes for SPI mode 0 or 3
#define AT_OP_RD_MFG_ID 0x9F //read manufacturer ID
#define AT_OP_RD_STATUS_REG 0xD7   // status register read
#define AT_OP_RD_MEM_PG 0xD2     // memory page read
#define AT_OP_WR_TO_BUF_TO_MEM_W_ERASE 0x82   // write into buffer, then to memory, erase first
#define AT_OP_RD_MEM_TO_BUF 0x53   // memory to buffer read


#define CHIP_RDY 0x80   // ready bit

#define EEPROM_CS_ACTIVE()   do { GPIO_PACLR |= 0x0008; } while (0) // CS pin, activate chip; PA3
#define EEPROM_CS_INACTIVE() do { GPIO_PASET |= 0x0008; } while (0) // CS pin, deactivate chip; PA3

// could be optionally added
#define EEPROM_WP_ON()  do { ; } while (0)  // WP pin, write protection on
#define EEPROM_WP_OFF() do { ; } while (0)  // WP pin, write protection off


volatile union  
{  
  int32u Flash24Bit_Address;  
  struct  
  {  
    int8u First_Byte_Address;  
    int8u Second_Byte_Address;  
    int8u Third_Byte_Address;  
    int8u Fourth_Byte_Address;          ///< 没有使用  
  }Address_For_Byte;  
}Flash_Add;


/** 
 *  @file       写 (WEL) 使能位. 
 *  @return     NULL 
 *  @author     NULL 
 */  
void Write_Enable(void)  
{  
  EEPROM_CS_ACTIVE();  
  halSpiWrite(MX25L8006_WREN);  
  EEPROM_CS_INACTIVE();  
} 

/** 
 *  @file       写禁止 (WRID) 指令,写使能锁存 (WEL) 位复位 
 *              WEL位复位是有以下几种情况： 
 *              1.芯片上电时 
 *              2.写禁止（WRDI）的指令完成后 
 *              3.写状态寄存器 (WRSR) 的指令完成后 
 *              4.页程序 (PP) 的指令完成之后 
 *              5.扇区擦除 (SE) 的指令完成之后 
 *              6.块擦除 (BE) 的指令完成之后 
 *              7.芯片擦除（CE）的指令完成. 
 * 
 *  @return     NULL 
 *  @author     NULL 
 */  
void Write_Disable(void)  
{  
  EEPROM_CS_ACTIVE();                       ///< 片选有效  
  halSpiWrite(MX25L8006_WRDI);  
  EEPROM_CS_INACTIVE();  
}

boolean Read_Identification(void)  
{
  int8u mfgId;
  
  EEPROM_CS_ACTIVE();  
  halSpiWrite(MX25L8006_RDID);
  mfgId = halSpiRead();
  EEPROM_CS_INACTIVE();
  
  return (mfgId==AT_MANUFACTURER_ID)? EEPROM_SUCCESS : EEPROM_ERR_INVALID_CHIP; 
} 

#ifdef EEPROM_USES_SHUTDOWN_CONTROL
  // Define PB7 as the EEPROM_ENABLE_PIN
  #define EEPROM_ENABLE_PIN PORTB_PIN(7)
  #define SET_EEPROM_ENABLE_PIN() GPIO_PBSET = PB7
  #define CLR_EEPROM_ENABLE_PIN() GPIO_PBCLR = PB7
#endif //EEPROM_USES_SHUTDOWN_CONTROL

// Initialization constants.  For more detail on the resulting waveforms,
// see the EM35x datasheet.
#define SPI_ORD_MSB_FIRST (0<<SC_SPIORD_BIT) // Send the MSB first
#define SPI_ORD_LSB_FIRST (1<<SC_SPIORD_BIT) // Send the LSB first

#define SPI_PHA_FIRST_EDGE (0<<SC_SPIPHA_BIT)  // Sample on first edge
#define SPI_PHA_SECOND_EDGE (1<<SC_SPIPHA_BIT) // Sample on second edge

#define SPI_POL_RISING_LEAD  (0<<SC_SPIPOL_BIT) // Leading edge is rising
#define SPI_POL_FALLING_LEAD (1<<SC_SPIPOL_BIT) // Leading edge is falling

// configure for fastest (12MHz) line rate.
// rate = 24MHz / (2 * (LIN + 1) * (2^EXP))
#if defined(CORTEXM3_EM350) || defined(CORTEXM3_STM32W108)
  // limited to 6MHz
  #define SPI_LIN  (1)  // 6Mhz
  #define SPI_EXP  (0)
#else
  #define SPI_LIN  (0)    // 12Mhz - FOR EM35x
  #define SPI_EXP  (0)
#endif

int8u halEepromInit(void)
{
  // GPIO assignments
  // PA0: SC2MOSI
  // PA1: SC2MISO
  // PA2: SC2SCLK
  // PA3: SC2 chip select

  // Set EEPROM_ENABLE_PIN high as part of EEPROM init
  #ifdef EEPROM_USES_SHUTDOWN_CONTROL
    halGpioConfig(EEPROM_ENABLE_PIN, GPIOCFG_OUT);
    SET_EEPROM_ENABLE_PIN();
  #endif //EEPROM_USES_SHUTDOWN_CONTROL

  //-----SC2 SPI Master GPIO configuration
  #if defined(CORTEXM3_EM350) || defined(CORTEXM3_STM32W108)
    // requires special mode for CLK
    GPIO_PACFGL =   (GPIOCFG_OUT_ALT  << PA0_CFG_BIT)|  // PA0 MOSI
                    (GPIOCFG_IN       << PA1_CFG_BIT)|  // PA1 MISO
                    (0xb              << PA2_CFG_BIT)|  // PA2 SCLK
                    (GPIOCFG_OUT      << PA3_CFG_BIT);  // PA3 nSSEL
  #else
    GPIO_PACFGL =   (GPIOCFG_OUT_ALT  << PA0_CFG_BIT)|  // PA0 MOSI
                    (GPIOCFG_IN       << PA1_CFG_BIT)|  // PA1 MISO
                    (GPIOCFG_OUT_ALT  << PA2_CFG_BIT)|  // PA2 SCLK
                    (GPIOCFG_OUT      << PA3_CFG_BIT);  // PA3 nSSEL
  #endif

  SC2_RATELIN = SPI_LIN;
  SC2_RATEEXP = SPI_EXP;
  SC2_SPICFG  =  0;
  SC2_SPICFG =  (1 << SC_SPIMST_BIT)|  // 4; master control bit
                (SPI_ORD_MSB_FIRST | SPI_PHA_FIRST_EDGE | SPI_POL_RISING_LEAD);
  SC2_MODE   =  SC2_MODE_SPI;
  #if defined(CORTEXM3_EM350) || defined(CORTEXM3_STM32W108)
    // required to enable high speed SCLK
    SC2_TWICTRL2 |= SC_TWIACK_BIT;
  #endif

  //The datasheet describes timing parameters for powerup.  To be
  //the safest and impose no restrictions on further operations after
  //powerup/init, delay worst case of 20ms.  (I'd much rather worry about
  //time and power consumption than potentially unstable behavior).
  //The Atmel AT45DB021D datasheet says that 20ms is the max "Power-Up Device
  //Delay before Write Allowed", and 1ms is the delay "required before the
  //device can be selected in order to perform a read operation."
  halCommonDelayMicroseconds(20000);
  
  //Make sure this driver is talking to the correct chip
  return Read_Identification();
}

static const HalEepromInformationType partInfo = {
  EEPROM_INFO_VERSION,
  0,  // no specific capabilities
  0,  // page erase time (not suported or needed in this driver)
  0,  // part erase time (not suported or needed in this driver)
  DEVICE_PAGE_SZ,  // page size
  DEVICE_SIZE,  // device size
  "MX25L2006E",
  DEVICE_WORD_SIZE // word size in bytes
};
/** 
 *  @file       读取 flash 状态寄存器状态 RDSR 指令是读状态寄存器。读状态寄存器 
 *          可以在任何时候读取 (即使在编程/擦除/写状态寄存器的条件),建议在发送一 
 *          个指令之前,如 编程/擦除/写状态寄存器操作时,检查一下 (WIP) 位. 
 *              状态寄存器位定义如下： 
 *                  1.WIP bit(bit 0):写进行位 (WIP),这是个不确定的位,表示该设备是 
 *          否处于忙碌状态(编程/擦除/写状态寄存器操作),当这个位为 1 时,表示当前处 
 *          于忙碌状态,如果这个位为 0 ,表示当前处于等待状态. 
 * 
 *                  2.WEL bit(bit 1):写使能锁存器（WEL）位,表示该设备是否设置内部 
 *          写使能锁存,当 (WEL)位设置为1，这意味着内部写使能锁存器置，该装置可以 
 *          接受的编程/擦除/写状态寄存器指令.当WEL位设置为0，这意味着没有内部写使 
 *          能锁存器设备将不接受编程/擦除/写状态寄存器指令. 
 * 
 *                  3.BP2, BP1, BP0 bits(bit 2,3,4): 块保护位,定义保护区域 
 * 
 *                  4.SRWD bit(bit 7):状态寄存器写禁止（SRWD）位, 
 *  @return     NULL 
 *  @author     NULL 
 */  
int8u Read_Status_Register(void)  
{  
  int8u data; 
  
  Write_Enable();  
  EEPROM_CS_ACTIVE();                       ///< 使能 Flash 芯片  
  halSpiWrite(MX25L8006_RDSR);          ///< 发送读取状态寄存器命令  
  data = halSpiRead();  
  EEPROM_CS_INACTIVE();                       ///< 关闭 Flash 芯片  
  return data;  
} 

const HalEepromInformationType *halEepromInfo(void)
{
  return &partInfo;
}

int32u halEepromSize(void)
{
  return halEepromInfo()->partSize;
}

boolean halEepromBusy(void)
{
  // This driver doesn't support busy detection
  return (Read_Status_Register()&0x01)?TRUE:FALSE;
}

void halEepromShutdown(void)
{
  EEPROM_CS_ACTIVE();  
  halSpiWrite(MX25L8006_DP);   ///< 发送 Deep Power Down 命令  
  EEPROM_CS_INACTIVE(); 
}
/** 
 *  @file           扇区擦除. 
 *  @param Address  传递要擦除扇区的地址 24 位. 
 *  @return         NULL. 
 *  @author         NULL. 
 */  
void Sector_Erase(int32u Address)  
{  
  Write_Enable();  
  Flash_Add.Flash24Bit_Address = Address;  
  EEPROM_CS_ACTIVE();  
  halSpiWrite(MX25L8006_SE);                                            ///< 发送 Sector Erase 命令  
  halSpiWrite(Flash_Add.Address_For_Byte.Third_Byte_Address);           ///< 发送高 8 位  
  halSpiWrite(Flash_Add.Address_For_Byte.Second_Byte_Address);          ///< 发送中 8 位  
  halSpiWrite(Flash_Add.Address_For_Byte.First_Byte_Address);           ///< 发送低 8 位  
  EEPROM_CS_INACTIVE();  
  while (halEepromBusy()) ;  
}  

/** 
 *  @file           高速读取 Flash Rom 数据. 
 *  @param Address  传递要读取的数据的地址 24 位. 
 *  @param *Buf     传递存放数据的 unsigned chr 数组指针. 
 *  @param Length   传递需要读取的数据个数. 
 *  @return         NULL. 
 *  @author         NULL. 
 */  
int8u halMX25L2006EReadBytes(int32u Address,int8u *Buf,int16u Length)  
{  
  int16u i;  
    
  Flash_Add.Flash24Bit_Address = Address;  
  EEPROM_CS_ACTIVE();  
  halSpiWrite(MX25L8006_FAST_READ);                                     ///< 发送读数据命令  
  halSpiWrite(Flash_Add.Address_For_Byte.Third_Byte_Address);           ///< 发送高 8 位  
  halSpiWrite(Flash_Add.Address_For_Byte.Second_Byte_Address);          ///< 发送中 8 位  
  halSpiWrite(Flash_Add.Address_For_Byte.First_Byte_Address);           ///< 发送低 8 位  
  for (i=0; i<Length; i++)  
  {  
    *Buf = halSpiRead(); 
    Buf++;  
  }  
  EEPROM_CS_INACTIVE();  

  return EEPROM_SUCCESS;
}  
/** 
 *  @file           高速读取 Flash Rom 数据. 
 *  @param Address  传递要写入的地址 24 位. 
 *  @param *Buf     传递需要写入的数据的 unsigned chr 数组指针. 
 *  @param Length   传递需要写入的数据个数. 
 *  @return         NULL. 
 *  @author         NULL. 
 */  

int8u halMX25L2006EWriteBytes(int32u Address,const int8u *Buf,int16u Length)  
{  
  int16u i;
  
  Write_Enable();
  Flash_Add.Flash24Bit_Address = Address;  
  EEPROM_CS_ACTIVE();  
  halSpiWrite(MX25L8006_PP);                                        ///< 发送读数据命令  
  halSpiWrite(Flash_Add.Address_For_Byte.Third_Byte_Address);       ///< 发送高 8 位  
  halSpiWrite(Flash_Add.Address_For_Byte.Second_Byte_Address);      ///< 发送中 8 位  
  halSpiWrite(Flash_Add.Address_For_Byte.First_Byte_Address);       ///< 发送低 8 位  
  for(i=0;i<Length;i++)  
  {  
    halSpiWrite(*Buf);  
    Buf++;  
  }  
  EEPROM_CS_INACTIVE();

  while (halEepromBusy()) ;

  return EEPROM_SUCCESS;
}  

//
// ~~~~~~~~~~~~~~~~~~~~~~~~~ Standard EEPROM Interface ~~~~~~~~~~~~~~~~~~~~~~~~~
//

// halEepromRead
// address: the address in EEPROM to start reading
// data: write the data here
// len: number of bytes to read
//
// return: result of halAT45DB021DReadBytes() call(s) or EEPROM_ERR_ADDR
//
int8u halEepromRead(int32u address, int8u *data, int16u totalLength)
{
  int32u nextPageAddr;
  int16u len;
  int8u status;

  if( address > DEVICE_SIZE || (address + totalLength) > DEVICE_SIZE)
    return EEPROM_ERR_ADDR;

  if( address & DEVICE_PAGE_MASK) {
    // handle unaligned first block
    nextPageAddr = (address & (~DEVICE_PAGE_MASK)) + DEVICE_PAGE_SZ;
    if((address + totalLength) < nextPageAddr){
      // fits all within first block
      len = totalLength;
    } else {
      len = (int16u) (nextPageAddr - address);
    }
  } else {
    len = (totalLength>DEVICE_PAGE_SZ)? DEVICE_PAGE_SZ : totalLength;
  }
  while(totalLength) {
    if( (status=halMX25L2006EReadBytes(address, data, len)) != EEPROM_SUCCESS) {
      return status;
    }
    totalLength -= len;
    address += len;
    data += len;
    len = (totalLength>DEVICE_PAGE_SZ)? DEVICE_PAGE_SZ : totalLength;
  }
  return EEPROM_SUCCESS;
}

// halEepromWrite
// address: the address in EEPROM to start writing
// data: pointer to the data to write
// len: number of bytes to write
//
// return: result of halAT45DB021DWriteBytes() call(s) or EEPROM_ERR_ADDR
//
int8u halEepromWrite(int32u address, const int8u *data, int16u totalLength)
{
  int32u nextPageAddr;
  int16u len;
  int8u status;

  if( address > DEVICE_SIZE || (address + totalLength) > DEVICE_SIZE)
    return EEPROM_ERR_ADDR;

  if( address & DEVICE_PAGE_MASK) {
    // handle unaligned first block
    nextPageAddr = (address & (~DEVICE_PAGE_MASK)) + DEVICE_PAGE_SZ;
    if((address + totalLength) < nextPageAddr){
      // fits all within first block
      len = totalLength;
    } else {
      len = (int16u) (nextPageAddr - address);
    }
  } else {
    len = (totalLength>DEVICE_PAGE_SZ)? DEVICE_PAGE_SZ : totalLength;
  }
  while(totalLength) {
    if( (status=halMX25L2006EWriteBytes(address, data, len)) != EEPROM_SUCCESS) {
      return status;
    }
    totalLength -= len;
    address += len;
    data += len;
    len = (totalLength>DEVICE_PAGE_SZ)? DEVICE_PAGE_SZ : totalLength;
  }
  return EEPROM_SUCCESS;
}
static boolean verifyAddressRange(int32u address, int16u length)
{
  int32u endAddr = address+length;
  int32u storageSize = halEepromSize();

  // Make sure both the start and end addresses are within range
  // for the internal storage size specified
  if((address >= storageSize) || 
     ((address + length) > storageSize) ||
     ((address + length) < address)) {
    return FALSE;
  }

  // Extra check to make sure we never overwrite the bootloader
  //  note: assumes the AAT is at the beginning of a flash page
  if(endAddr < (int32u)halBootloaderAddressTable.appAddressTable) {
    return FALSE;
  }

  // must be within range
  return TRUE;
}

int8u halEepromErase(int32u address, int32u totalLength)
{
  // Make sure the length and start address are multiples of the flash page size
  if(totalLength & (~DEVICE_PAGE_MASK)) {
    return EEPROM_ERR_PG_SZ;
  } 
  // Make sure the address is on the start of a page boundary
  if(address & (~DEVICE_PAGE_MASK)) {
    return EEPROM_ERR_PG_BOUNDARY;
  }
  // Make sure that the totalLength is within the internal storage region
  if( !verifyAddressRange(address, totalLength) ) {
    return EEPROM_ERR_ADDR;
  }
  totalLength += address&DEVICE_PAGE_MASK;

  // Erase all of the requested pages
  while(totalLength) {
    Sector_Erase(address&(~DEVICE_PAGE_MASK));
    address -= DEVICE_PAGE_SZ;
    totalLength -= DEVICE_PAGE_SZ;
  }

  return EEPROM_SUCCESS;
}
