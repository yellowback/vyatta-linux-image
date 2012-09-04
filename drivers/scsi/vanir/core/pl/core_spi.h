// ============================================================================
//
//     Copyright (c) Marvell Corporation 2000-2015  -  All rights reserved
//
//  This computer program contains confidential and proprietary information,
//  and  may NOT  be reproduced or transmitted, in whole or in part,  in any
//  form,  or by any means electronic, mechanical, photo-optical, or  other-
//  wise, and  may NOT  be translated  into  another  language  without  the
//  express written permission from Marvell Corporation.
//
// ============================================================================
// =      C O M P A N Y      P R O P R I E T A R Y      M A T E R I A L       =
// ============================================================================
#if !defined( _CORE_SPI_H_ )
#define _CORE_SPI_H_

#include "core_header.h"

#   define SPI_DBG( _X_ )       MV_DPRINT( _X_ )
#   define EEPROM_DBG( _X_ )    MV_DPRINT( _X_ )

#define AT25F2048               0x0101
#define AT25DF041A               0x0102
#define AT25DF021               0x0103
#define PM25LD010               0x2110

#define MX25L2005               0x0201
#define MX25L4005               0x0202
#define MX25L8005               0x0203
#define W25X40					0x0301
#define EN25F20					0x0401
#define SST25VF040B					0x0501


#define SPI_CTRL_REG            0xc0
#define SPI_CMD_REG             0xc4
#define SPI_DATA_REG            0xc8f

#define SPI_INS_WREN            0
#define SPI_INS_WRDI            1
#define SPI_INS_RDSR            2
#define SPI_INS_WRSR            3
#define SPI_INS_READ            4
#define SPI_INS_PROG            5
#define SPI_INS_SERASE          6
#define SPI_INS_CERASE          7
#define SPI_INS_RDID            8
#define SPI_INS_PTSEC            9
#define SPI_INS_UPTSEC            10
#define SPI_INS_RDPT            11

#define DEFAULT_SPI_FLASH_SIZE          256L * 1024
#define DEFAULT_SPI_FLASH_SECT_SIZE     64L * 1024

typedef struct _AdapterInfo AdapterInfo;
struct _AdapterInfo
{
    MV_U32      flags;
    MV_U16      devId;
    MV_U32      classCode;
    MV_U8       revId;
    MV_U8       bus;
    MV_U8       devFunc;
    void*       bar[6];
    MV_U32      ExpRomBaseAddr;
    MV_U8       version;
    MV_U8       subVersion;

    MV_U16      FlashID;
    MV_U32      FlashSize; 
    MV_U32      FlashSectSize;
};

typedef union 
{
    struct 
    {
        MV_U32 low;
        MV_U32 high;
    } parts;
    MV_U8       b[8];
    MV_U16      w[4];
    MV_U32      d[2];
} SAS_ADDRESS, *PSAS_ADDRESS;

#define MAX_BOOTDEVICE_SUPPORTED        8

#pragma pack(1)
typedef struct _HBA_Info_Main
{

	MV_U8 signature[4];	//offset 0x0h,4bytes,structure signature
	 
	MV_U16 image_size;	//offset 0x4h,2bytes,512bytes per indecate romsize

	MV_U16 option_rom_checksum;	//offset 0x6h,2 bytes,16BIT CHECKSUM OPTIONROM caculation

	MV_U8  major;	//offset 0x8h,1 byte,BIOS major version

	MV_U8  minor;	//offset 0x9h,1byte,BIOS minor version

	MV_U8  oem_num;	//offset 0xah,1byte,OEM number

	MV_U8  build_version;	//offset 0xbh,1byte,build number

	MV_U32  hba_flag;	/* offset 0xch,4 bytes,
 				     HBA flags:  refers to HBA_FLAG_XX
 					bit 0   --- HBA_FLAG_BBS_ENABLE
					bit 1   --- HBA_FLAG_SUPPORT_SMART
					bit 2   --- HBA_FLAG_ODD_BOOT
					bit 3   --- HBA_FLAG_INT13_ENABLE
					bit 4   --- HBA_FLAG_ERROR_STOP
					bit 5   --- HBA_FLAG_ERROR_PASS
					bit 6   --- HBA_FLAG_SILENT_MODE_ENABLE
				*/

	MV_U16 boot_order[MAX_BOOTDEVICE_SUPPORTED];	//offset 0x10h,2bytes*Max_bootdevice_supported,BootOrder[0] = 3,means the first bootable device is device 3

	MV_U8 bootablecount;	//bootable device count
	
	MV_U8  serialnum[20];	//offset 0x20h,20bytes,serial number

	MV_U8  chip_revision;	//offset 0x34h,1 byte,chip revision

	SAS_ADDRESS   sas_address[MAX_NUMBER_IO_CHIP*MAX_PORT_PER_PL];	//offset ,8bytes*MAX_PHYSICAL_PORT_NUMBER

	MV_U8  phy_rate[MAX_NUMBER_IO_CHIP*MAX_PORT_PER_PL];	//offset ,1byte*MAX_PHYSICAL_PORT_NUMBER,0:  1.5 GB/s, 1: 3.0 GB/s

    	MV_U32  bootdev_wwn[8]; //this is initial as 0xFFFF,FFFF

	MV_U8  reserve[97];

	MV_U8  checksum;
}HBA_Info_Main, *pHBA_Info_Main;
#pragma pack()

int
OdinSPI_SectErase
(
    AdapterInfo *pAI,
    MV_U32      addr
);
int
OdinSPI_ChipErase
(
    AdapterInfo *pAI
);
int
OdinSPI_Write
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U32      Data
);
int
OdinSPI_WriteBuf
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U8       *Data,
    MV_U32      Count
);
int
OdinSPI_Read
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U8       *Data,
    MV_U8       Size
);
int
OdinSPI_ReadBuf
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U8       *Data,
    MV_U32      Count
);
int
OdinSPI_RMWBuf
(
    AdapterInfo *pAI,
    MV_U32      Addr,
    MV_U8       *Data,
    MV_U32      Count,
    MV_U8       *Buf
);
int
OdinSPI_Init
(
    AdapterInfo *pAI
);
int
LoadHBAInfo
(
    AdapterInfo *pAI,
    HBA_Info_Main *pHBAInfo
);

int
OdinSPI_Write_SST
(
    AdapterInfo *pAI,
    MV_I32      Addr,
    MV_U8*      Data, 
    MV_U32      Count
);


#endif
