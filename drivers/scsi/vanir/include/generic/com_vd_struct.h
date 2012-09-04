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
#ifndef __MV_COM_VD_STRUCT_H__
#define __MV_COM_VD_STRUCT_H__

#include "com_raid_struct.h"

#define MAX_LD_SUPPORTED_API                    32

#define LD_MAX_NAME_LENGTH                      16

#define LD_STATUS_FUNCTIONAL                    0
#define LD_STATUS_DEGRADE                       1
#define LD_STATUS_DELETED                       2
#define LD_STATUS_MISSING                       3 /* LD missing in system. */
#define LD_STATUS_OFFLINE                       4
#define LD_STATUS_PARTIALLYOPTIMAL              5 /* r6 w/ 2 pd, 1 hd drops */
#define LD_STATUS_FOREIGN                       6
#define LD_STATUS_IMPORTABLE		            7
#define LD_STATUS_NOT_IMPORTABLE	            8
#define LD_STATUS_MIGRATION                     9
#define LD_STATUS_REBUILDING			        10
#define LD_STATUS_CONFLICTED		            11
#define LD_STATUS_DEGRADE_PLUGIN		        12
#define LD_STATUS_HYPER_UNINIT    		        13
#define LD_STATUS_INVALID                       0xFF

#ifndef MV_GUID_SIZE
#define MV_GUID_SIZE                            8
#endif /* MV_GUID_SIZE */

#pragma pack(8)

typedef struct _LD_Info
{
 MV_U16            ID;
 MV_U8             Status;         /* Refer to LD_STATUS_xxx */
 MV_U8             BGAStatus;      /* Refer to LD_BGA_STATE_xxx */
 MV_U16            StripeBlockSize;/* between RAIDAPI and driver, it is # of BlockSize */
 MV_U8             RaidMode;            
 MV_U8             HDCount;

 MV_U8             CacheMode;      /* Default is CacheMode_Default, see above */
 MV_U8             LD_GUID[MV_GUID_SIZE];
 MV_U8             SectorCoefficient; /* (sector size) 1=>512 (default), 2=>1024, 4=>2048, 8=>4096 */
 MV_U8             AdapterID;
 MV_U8             BlkCount;         /* The valid entry count of BlockIDs */
                                     /* This field is added for supporting large block count */
                                     /* If BlkCount==0, "0x00FF" means invalid entry of BlockIDs */
 MV_U16            time_hi;
 MV_U16            DGID;       

 MV_U64            Size;           /* In unit of BlockSize between API and driver. */

 MV_U8             Name[LD_MAX_NAME_LENGTH];

 MV_U16            BlockIDs[MAX_HD_SUPPORTED_API];
/* 
 * According to BLOCK ID, to get the related HD ID, then WMRU can 
 * draw the related graph like above.
 */
 MV_U8             SubLDCount;     /* for raid 10, 50,60 */
 MV_U8             NumParityDisk;  /* For RAID 6. */
 MV_U16            time_low;
 MV_U32            BlockSize;      /* in bytes. if 0, BlockSize is 512 */
 
}LD_Info, *PLD_Info;

typedef struct _Create_LD_Param
{
 MV_U8             RaidMode;
 MV_U8             HDCount;
 MV_U8             RoundingScheme; /* please refer to the definitions of  ROUNDING_SCHEME_XXX. */
 MV_U8             SubLDCount;     /* for raid 10,50,60 */
 MV_U16            StripeBlockSize;/* between API and driver, it is # of BlockSize */
 MV_U8             NumParityDisk;  /* For RAID 6. */
 MV_U8             CachePolicy;    /* please refer to the definition of CACHEMODE_XXXX. */

 MV_U8             InitializationOption; /* please refer to the definitions of INIT_XXXX. */
 MV_U8             SectorCoefficient;    /* (sector size) 1=>512 (default), 2=>1024, 4=>2048, 8=>4096 */
 MV_U16            LDID;                 /* ID of the LD to be migrated or expanded */
 MV_U8             SpecifiedBlkSeq;      /* DG slot number, this is only used when DG is enforced */
 MV_U8             HypperWaterMark;
 MV_U8             Reserved2[1];
 MV_U8             ReservedForApp;	/* Reserved for application (CLI) */

 MV_U16            HDIDs[MAX_HD_SUPPORTED_API];    /* 32 */
 MV_U8             Name[LD_MAX_NAME_LENGTH];

 MV_U64            Size;           /* In unit of BlockSize between API and driver. API need to get the BlockSize for PD first and use it to set 'Size' */
} Create_LD_Param, *PCreate_LD_Param;


typedef struct _Hyper_Free_Info
{
 MV_U16            ID;
 MV_U8             Reserved[6];

 MV_U64            HyperFreeSize;
} Hyper_Free_Info, *PHyper_Free_Info;

typedef struct _LD_STATUS
{
 MV_U8            Status;          /* Refer to LD_STATUS_xxx */
 MV_U8            Bga;             /* Refer to LD_BGA_xxx */
 MV_U16           BgaPercentage;   /* xx% */
 MV_U8            BgaState;        /* Refer to LD_BGA_STATE_xxx */
 MV_U8            BgaExt;          /* Not used yet. Extention of Bga. */
 MV_U16           LDID;
} LD_Status, *PLD_Status;

typedef struct    _LD_Config
{
 MV_U8            CacheMode;        /* See definition 4.4.1 CacheMode_xxx */
 MV_U8            HypperWaterMark;        
 MV_BOOLEAN       AutoRebuildOn;    /* 1- AutoRebuild On */
 MV_U8            Status;
 MV_U16           LDID;
 MV_U8            Reserved2[2];

 MV_U8            Name[LD_MAX_NAME_LENGTH];
}LD_Config, * PLD_Config;

#pragma pack()

#endif
