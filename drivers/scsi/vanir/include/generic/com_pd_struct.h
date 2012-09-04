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
#ifndef __MV_COM_PD_STRUCT_H__
#define __MV_COM_PD_STRUCT_H__

#include "com_physical_link.h"

//PD
#define MAX_HD_SUPPORTED_API                    128
#define MAX_BLOCK_PER_HD_SUPPORTED_API          16

// Macros used for error injection. 
// Error status that should be returned upon encountering the specified LBA
#define REQ_STATUS_MEDIA_ERROR                  0x2
#define REQ_STATUS_HAS_SENSE                    0x7
#define REQ_STATUS_ERROR                        0x8

#define PD_INIT_STATUS_OK						0
#define PD_INIT_STATUS_ERROR					MV_BIT(0) // for HD_Info InitStatus, set if there was ever a failure during init

// Definition used for old driver.
#define HD_TYPE_SATA                            MV_BIT(0)
#define HD_TYPE_PATA                            MV_BIT(1)
#define HD_TYPE_SAS                             MV_BIT(2)
#define HD_TYPE_ATAPI                           MV_BIT(3)
#define HD_TYPE_TAPE                            MV_BIT(4)
#define HD_TYPE_SES                             MV_BIT(5)

// Definition used for PD interface type as defined in DDF spec
#define PD_INTERFACE_TYPE_UNKNOWN               0x0000
#define PD_INTERFACE_TYPE_SCSI                  0x1000
#define PD_INTERFACE_TYPE_SAS                   0x2000
#define PD_INTERFACE_TYPE_SATA                  0x3000
#define PD_INTERFACE_TYPE_FC                    0x4000

// PD's Protocol/Connection type (used by new driver)
#define DC_ATA                                  MV_BIT(0)
#define DC_SCSI                                 MV_BIT(1)
#define DC_SERIAL                               MV_BIT(2)
#define DC_PARALLEL                             MV_BIT(3)
#define DC_ATAPI                                MV_BIT(4)  // used by core driver to prepare FIS
#define DC_SGPIO                                MV_BIT(5)

// PD's Device type defined in SCSI-III specification (used by new driver)
#define DT_DIRECT_ACCESS_BLOCK                  0x00
#define DT_SEQ_ACCESS                           0x01
#define DT_PRINTER                              0x02
#define DT_PROCESSOR                            0x03
#define DT_WRITE_ONCE                           0x04
#define DT_CD_DVD                               0x05
#define DT_OPTICAL_MEMORY                       0x07
#define DT_MEDIA_CHANGER                        0x08
#define DT_STORAGE_ARRAY_CTRL                   0x0C
#define DT_ENCLOSURE                            0x0D
// The following are defined by Marvell
#define DT_EXPANDER                             0x20
#define DT_PM                                   0x21

#define HD_FEATURE_NCQ                          MV_BIT(0)
#define HD_FEATURE_TCQ                          MV_BIT(1)
#define HD_FEATURE_1_5G                         MV_BIT(2)
#define HD_FEATURE_3G                           MV_BIT(3)
#define HD_FEATURE_WRITE_CACHE                  MV_BIT(4)
#define HD_FEATURE_48BITS                       MV_BIT(5)
#define HD_FEATURE_SMART                        MV_BIT(6)
#define HD_FEATURE_6G                           MV_BIT(7)
#define HD_FEATURE_CRYPTO                       MV_BIT(8)
#define HD_FEATURE_TRIM                       MV_BIT(9)

#define HD_SPEED_1_5G                           1  
#define HD_SPEED_3G                             2
#define HD_SPEED_6G                             3

#define HD_AES_CRYPTO_DISK                      MV_BIT(0)
#define HD_AES_KEY_MATCHED                      MV_BIT(1)

#define HD_WIPE_MDD                             0
#define HD_WIPE_FORCE                           1

#define HD_DMA_NONE                             0
#define HD_DMA_1                                1
#define HD_DMA_2                                2
#define HD_DMA_3                                3
#define HD_DMA_4                                4
#define HD_DMA_5                                5
#define HD_DMA_6                                6
#define HD_DMA_7                                7
#define HD_DMA_8                                8
#define HD_DMA_9                                9

#define HD_PIO_NONE                             0
#define HD_PIO_1                                1
#define HD_PIO_2                                2
#define HD_PIO_3                                3
#define HD_PIO_4                                4
#define HD_PIO_5                                5

#define HD_XCQ_OFF                              0
#define HD_NCQ_ON                               1
#define HD_TCQ_ON                               2

#define HD_SSD_TYPE_NOT_SSD                     0
#define HD_SSD_TYPE_UNKNOWN_SSD                 1

#pragma pack(8)

typedef struct _HD_Info
{
 Link_Entity     Link;             /* Including self DevID & DevType */
 MV_U8           AdapterID;
 MV_U8           InitStatus;       /* Refer to PD_INIT_STATUS_XXX */
 MV_U8           HDType;           /* HD_Type_xxx, replaced by new driver with ConnectionType & DeviceType */
 MV_U8           PIOMode;          /* Max PIO mode */
 MV_U8           MDMAMode;         /* Max MDMA mode */
 MV_U8           UDMAMode;         /* Max UDMA mode */
 MV_U8           ConnectionType;   /* DC_XXX, ConnectionType & DeviceType in new driver to replace HDType above */
 MV_U8           DeviceType;       /* DT_XXX */

 MV_U32          FeatureSupport;   /* Support 1.5G, 3G, TCQ, NCQ, and etc, MV_BIT related */
 MV_U8           Model[40];
 MV_U8           SerialNo[20];
 MV_U8           FWVersion[8];
 MV_U64          Size;             /* In unit of BlockSize between API and driver */
 MV_U8           WWN[8];           /* ATA/ATAPI-8 has such definitions for the identify buffer */
 MV_U8           CurrentPIOMode;   /* Current PIO mode */
 MV_U8           CurrentMDMAMode;  /* Current MDMA mode */
 MV_U8           CurrentUDMAMode;  /* Current UDMA mode */
 MV_U8           ElementIdx;       /* corresponding element index in enclosure */
 MV_U32          BlockSize;        /* Bytes in one sector/block, if 0, set it to be 512 */

 MV_U8           ActivityLEDStatus;
 MV_U8           LocateLEDStatus;
 MV_U8           ErrorLEDStatus;
 MV_U8           SesDeviceType;	   /* ENC_ELEMENTTYPE_DEVICE or ENC_ELEMENTTYPE_ARRAYDEVICE */
 MV_U32		 sata_signature;
 MV_U8           Reserved4[8];
 MV_U8		 HD_SSD_Type;
 MV_U8		 Reserved1[63];
}HD_Info, *PHD_Info;

typedef struct _HD_MBR_Info
{
 MV_U8           HDCount;
 MV_U8           Reserved[7];
 MV_U16          HDIDs[MAX_HD_SUPPORTED_API];    
 MV_BOOLEAN      hasMBR[MAX_HD_SUPPORTED_API];
} HD_MBR_Info, *PHD_MBR_Info;


typedef struct _HD_FreeSpaceInfo
{
 MV_U16          ID;               /* ID should be unique*/
 MV_U8           AdapterID;
 MV_U8           reserved1;
 MV_U16          BlockSize;        /* Bytes in one sector/block, if 0, set it to be 512 */
 MV_U8           reserved2;
 MV_BOOLEAN      isFixed;

 MV_U64          Size;             /* In unit of BlockSize between API and driver */
}HD_FreeSpaceInfo, *PHD_FreeSpaceInfo;

typedef struct _HD_Block_Info
{
 MV_U16          ID;               /* ID in the HD_Info*/
 MV_U8           Type;             /* Refer to DEVICE_TYPE_xxx */
 MV_U8           BlkCount;         /* The valid entry count of BlockIDs */
                                   /* This field is added for supporting large block count */
                                   /* If BlkCount==0, "0x00FF" means invalid entry of BlockIDs */
 MV_U8           Reserved1[4];

 /* Free is 0xff */
 MV_U16          BlockIDs[MAX_BLOCK_PER_HD_SUPPORTED_API];  
}HD_Block_Info, *PHD_Block_Info;


typedef struct _HD_CONFIG
{
 MV_BOOLEAN        WriteCacheOn; // 1: enable write cache 
 MV_BOOLEAN        SMARTOn;      // 1: enable S.M.A.R.T
 MV_BOOLEAN        Online;       // 1: to set HD online
 MV_U8             DriveSpeed;   // For SATA & SAS.  HD_SPEED_1_5G, HD_SPEED_3G etc
 MV_U8             crypto;
 MV_U8             Reserved[1];
 MV_U16            HDID;   
}HD_Config, *PHD_Config;

typedef struct  _HD_SMART_STATUS
{
 MV_BOOLEAN        SmartThresholdExceeded;        
 MV_U8             Reserved1;
 MV_U16            HDID;
 MV_U8             Reserved2[4];
}HD_SMART_Status, *PHD_SMART_Status;

typedef struct _HD_BGA_STATUS
{
 MV_U16            HDID;
 MV_U16            Percentage;      /* xx% */
 MV_U8             Bga;             /* Refer to HD_BGA_TYPE_xxx */
 MV_U8             Status;          /* not used */
 MV_U8             BgaStatus;       /* Refer to HD_BGA_STATE_xxx */
 MV_U8             Reserved[57];  
}HD_BGA_Status, *PHD_BGA_Status;

/* 
 * RCT entry flag 
 */
/* request type related */
#define EH_READ_VERIFY_REQ_ERROR                MV_BIT(0) /* Read or Read Verify request is failed. */
#define EH_WRITE_REQ_ERROR                      MV_BIT(1) /* Write request is failed */
/* error type related */
#define EH_MEDIA_ERROR                          MV_BIT(3) /* Media Error or timeout */
#define EH_LOGICAL_ERROR                        MV_BIT(4) /* Logical Error because of BGA activity. */
/* other flag */
#define EH_FIX_FAILURE                          MV_BIT(5) /* Ever tried to fix this error but failed */
/* extra flag */
#define EH_TEMPORARY_ERROR                      MV_BIT(7) /* Temporary error. Used when BGA rebuild write mark target disk up. */


typedef struct _RCT_Record{
 MV_LBA  lba;
 MV_U32  sec;                       //sector count
 MV_U8   flag;
 MV_U8   rev[3];
}RCT_Record, *PRCT_Record;

#pragma pack()

//PM
#define MAX_PM_SUPPORTED_API                    8

#pragma pack(8)

typedef  struct _PM_Info{
 Link_Entity       Link;           /* Including self DevID & DevType */
 MV_U8             AdapterID;
 MV_U8             ProductRevision;
 MV_U8             PMSpecRevision; /* 10 means 1.0, 11 means 1.1 */
 MV_U8             NumberOfPorts;
 MV_U16            VendorId;
 MV_U16            DeviceId;
 MV_U8             Reserved1[8];
}PM_Info, *PPM_Info;

#pragma pack()

#define MAX_EXPANDER_SUPPORTED_API              16

#define EXP_SSP                                 MV_BIT(0)
#define EXP_STP                                 MV_BIT(1)
#define EXP_SMP                                 MV_BIT(2)

#pragma pack(8)

typedef struct _Exp_Info
{
 Link_Entity       Link;            /* Including self DevID & DevType */
 MV_U8             AdapterID;
 MV_BOOLEAN        Configuring;      
 MV_BOOLEAN        RouteTableConfigurable;
 MV_U8             PhyCount;
 MV_U16            ExpChangeCount;
 MV_U16            MaxRouteIndexes;
 MV_U8             VendorID[8+1];
 MV_U8             ProductID[16+1];
 MV_U8             ProductRev[4+1];
 MV_U8             ComponentVendorID[8+1];
 MV_U16            ComponentID;
 MV_U8             ComponentRevisionID;
 MV_U8             Reserved1[17];
}Exp_Info, * PExp_Info;

#pragma pack()

#endif
