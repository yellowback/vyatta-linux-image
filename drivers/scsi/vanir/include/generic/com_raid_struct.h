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
#ifndef __MV_COM_RAID_STRUCT_H__
#define __MV_COM_RAID_STRUCT_H__

#include "com_define.h"
#include "com_pd_struct.h"

#pragma pack(8)
#define MAX_SPARE_PD_SUPPORTED_API				8
#define MAX_BLOCK_SUPPORTED_API                 512

#define MAX_BGA_RATE                            0xFA
#define MAX_MEDIAPATROL_RATE                    0xFF

#define CACHE_WRITEBACK_ENABLE                  0
#define CACHE_WRITETHRU_ENABLE                  1
#define CACHE_ADAPTIVE_ENABLE                   2
#define CACHE_AUTO_ENABLE                       MV_BIT(3)
#define CACHE_WRITE_POLICY_FILTER               (CACHE_WRITEBACK_ENABLE | \
       CACHE_WRITETHRU_ENABLE | \
       CACHE_ADAPTIVE_ENABLE | \
       CACHE_AUTO_ENABLE)
#define CACHE_LOOKAHEAD_ENABLE                  MV_BIT(2)


/* Definition for MV_LD_State */
#define LD_BGA_NONE                             0
#define LD_BGA_REBUILD                          MV_BIT(0)
#define LD_BGA_CONSISTENCY_FIX                  MV_BIT(1)
#define LD_BGA_CONSISTENCY_CHECK                MV_BIT(2)
#define LD_BGA_INIT_QUICK                       MV_BIT(3)
#define LD_BGA_INIT_BACK                        MV_BIT(4)
#define LD_BGA_MIGRATION                        MV_BIT(5)
#define LD_BGA_INIT_FORE                        MV_BIT(6)
#define LD_BGA_COPYBACK                         MV_BIT(7)
#define LD_BGA_DEFECT_FIXING                    MV_BIT(8)        /* Don't need save to DDF */
#define LD_BGA_MEDIA_PATROL                     MV_BIT(9)
#define LD_BGA_MIGRATION_EXT                    MV_BIT(10)       /* Free -> RAID0 */

#define LD_BGA_STATE_NONE                       0
#define LD_BGA_STATE_RUNNING                    1
#define LD_BGA_STATE_ABORTED                    2
#define LD_BGA_STATE_PAUSED                     3
#define LD_BGA_STATE_AUTOPAUSED                 4
#define LD_BGA_STATE_INTERRUPTED                5
#define LD_BGA_STATE_DDF_PENDING                MV_BIT(7)

#define LD_MODE_FREE                            0x66
#define LD_MODE_RAID0                           0x0
#define LD_MODE_RAID1                           0x1
#define LD_MODE_RAID5                           0x5
#define LD_MODE_RAID6                           0x6
#define LD_MODE_JBOD                            0x0f
#define LD_MODE_RAID10                          0x10
#define LD_MODE_RAID1E                          0x11
#define LD_MODE_RAID50                          0x50
#define LD_MODE_RAID60                          0x60
#define LD_MODE_RAID_CROSS                      0x0a
#define LD_MODE_RAID_HDD_MIRROR		            0x0b
#define LD_MODE_UNKNOWN                         0xFF

#define	LD_IS_HYPER_HDD(raid_mode)	((raid_mode == LD_MODE_RAID_CROSS) || (raid_mode == LD_MODE_RAID_HDD_MIRROR))

#define REBUILD_VD 1
#define REBUILD_DG 2

typedef struct _Rebuild_Param
{
    MV_U16     ID;
    MV_U8      RebuildType;
    MV_U8      PDCount;
    MV_U8      Reserved1[4];
    MV_U16     PDIDs[MAX_HD_SUPPORTED_API];
    MV_U8      Reserved2[64];
} Rebuild_Param, * PRebuild_Param;

typedef struct  _HD_RAID_STATUS
{
 MV_U16            HDID;
 MV_U16            DGID;
 MV_U8             Status;         /* HD_STATUS_SPARE */
 MV_U8             Reserved[3];
}HD_RAID_Status, *PHD_RAID_Status;

typedef struct  _BSL{
 MV_U64            LBA;            /* Bad sector LBA for the HD. */

 MV_U32            Count;          /* How many serial bad sectors */
 MV_BOOLEAN        Flag;           /* Fake bad sector or not. */
 MV_U8             Reserved[3];
}BSL,*PBSL;

typedef struct _BLOCK_INFO
{
 MV_U16            ID;
 MV_U16            HDID;           /* ID in the HD_Info */
 MV_U16            Flags;          /* Refer to BLOCK_XXX definition */
 MV_U16            LDID;           /* Belong to which LD */

 MV_U8             Status;         /* Refer to BLOCK_STATUS_XXX*/
 MV_U8             Reserved;
 MV_U16            BlockSize;      /* in bytes. if 0, BlockSize is 512 */
 MV_U32            ReservedSpaceForMigration; /* Space reserved for migration */

 MV_U64            StartLBA;       /* unit: 512 bytes */
 MV_U64            Size;           /* In unit of BlockSize between API and driver, including ReservedSpaceForMigration */
}Block_Info, *PBlock_Info;

#pragma pack()

#define CONSISTENCYCHECK_ONLY                   0
#define CONSISTENCYCHECK_FIX                    1

#define INIT_QUICK                              0
#define INIT_FULLFOREGROUND                     1
#define INIT_FULLBACKGROUND                     2
#define INIT_NONE                               3

#define INIT_QUICK_WITHOUT_EVENT                0xf

#define BGA_CONTROL_START                       0
#define BGA_CONTROL_RESTART                     1
#define BGA_CONTROL_PAUSE                       2
#define BGA_CONTROL_RESUME                      3
#define BGA_CONTROL_ABORT                       4
#define BGA_CONTROL_COMPLETE                    5
#define BGA_CONTROL_IN_PROCESS                  6
#define BGA_CONTROL_TERMINATE_IMMEDIATE         7
#define BGA_CONTROL_AUTO_PAUSE                  8
#define BGA_CONTROL_CONTINUE					9

#define ROUNDING_SCHEME_NONE                    0     /* no rounding */
#define ROUNDING_SCHEME_1GB                     1     /* 1 GB rounding */
#define ROUNDING_SCHEME_10GB                    2     /* 10 GB rounding */


#define HD_STATUS_FREE                          MV_BIT(0)
#define HD_STATUS_ASSIGNED                      MV_BIT(1)
#define HD_STATUS_SPARE                         MV_BIT(2)
#define HD_STATUS_OFFLINE                       MV_BIT(3)
#define HD_STATUS_SMARTCHECKING                 MV_BIT(4)
#define HD_STATUS_MP                            MV_BIT(5)
#define HD_STATUS_DEDICATED_SPARE               MV_BIT(6)
#define HD_STATUS_FOREIGN						MV_BIT(7)

#define HD_BGA_STATE_NONE                       LD_BGA_STATE_NONE
#define HD_BGA_STATE_RUNNING                    LD_BGA_STATE_RUNNING
#define HD_BGA_STATE_ABORTED                    LD_BGA_STATE_ABORTED
#define HD_BGA_STATE_PAUSED                     LD_BGA_STATE_PAUSED
#define HD_BGA_STATE_AUTOPAUSED                 LD_BGA_STATE_AUTOPAUSED
#define HD_BGA_STATE_PART_COMPLETE				6

#define HD_BGA_TYPE_NONE                        0
#define HD_BGA_TYPE_MP                          1
#define HD_BGA_TYPE_DATASCRUB                   2

#define PD_TYPE_PD_IN_VD                        MV_BIT(1)
#define GLOBAL_SPARE_DISK                       MV_BIT(2)
#define DEDICATED_SPARE_DISK                    MV_BIT(3)
#define PD_TYPE_FOREIGN							MV_BIT(4)
#define PD_TYPE_ENCLOSURE						MV_BIT(7)
#define PD_TYPE_SSD								MV_BIT(6)


#define PD_DDF_VALID                            MV_BIT(0)
#define PD_DISK_VALID                           MV_BIT(1)
#define PD_DDF_CLEAN                            MV_BIT(2)
#define PD_NEED_UPDATE                          MV_BIT(3)
#define PD_MBR_VALID                            MV_BIT(4)
#define PD_NEED_FLUSH                           MV_BIT(5)
#define PD_CLEAR_MBR                            MV_BIT(6)
#define PD_RCT_NEED_UPDATE                      MV_BIT(7)
#define PD_INIT_DDF_PROCESSING					MV_BIT(8)
#define PD_NEED_UPDATE_DELETE_LD				MV_BIT(9)
#define PD_NEED_SPIN_DOWN                       MV_BIT(10)

#define PD_STATE_ONLINE                         MV_BIT(0)
#define PD_STATE_FAILED                         MV_BIT(1)
#define PD_STATE_REBUILDING                     MV_BIT(2)
#define PD_STATE_TRANSITION                     MV_BIT(3)
#define PD_STATE_SMART_ERROR                    MV_BIT(4)
#define PD_STATE_READ_ERROR                     MV_BIT(5)
#define PD_STATE_MISSING                        MV_BIT(6)
#define PD_STATE_SPIN_DOWN						MV_BIT(7)

#define HD_STATUS_SETONLINE                     0
#define HD_STATUS_SETOFFLINE                    1
#define HD_STATUS_FORCESETOFFLINE				2
#define HD_STATUS_SETFREE						3
#define HD_STATUS_INVALID                       0xFF

#define BLOCK_INVALID                           0
#define BLOCK_VALID                             MV_BIT(0)
#define BLOCK_ASSIGNED                          MV_BIT(1)
#define BLOCK_FLAG_REBUILDING                   MV_BIT(2)
#define BLOCK_FLAG_TEMP_ASSIGN                  MV_BIT(3

#endif
