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
#ifndef __MV_COM_ARRAY_STRUCT_H__
#define __MV_COM_ARRAY_STRUCT_H__

#include "com_raid_struct.h"
#include "com_vd_struct.h"

#define MAX_DG_SUPPORTED_API                    64

#define DG_STATUS_FUNCTIONAL                    0
#define DG_STATUS_PD_MISSING                    1
#define DG_STATUS_PD_OFFLINE                    2

#pragma pack(8)

typedef struct _CreateOrModify_DG_Param
{
	MV_U16     DGID;                        // <=> InstanceID
	MV_U8      Reserved1[2];
	MV_U8      Name[LD_MAX_NAME_LENGTH];    // <=> ElementName.
	MV_U8      RaidMode;                    // <=> Goal
	MV_U8      SubVDCount;                  // for raid 10, 50, 60
	MV_U8      NumParityDisk;               // for RAID 6
	MV_U8      Reserved2[7];
	MV_U16     StripeBlockSize;             // <=> Goal.UserDataStripeDepth (between API and driver, it is # of BlockSize)
	MV_U64     Size;                        // <=> Size (in unit of BlockSize to/from driver)
	MV_U32     BlockSize;                   // returned from driver (in byte)
	MV_U8      InitializationOption;        // refer to INIT_XXXX
	MV_U8      Reserved3[2];
	MV_U8      PDCount;
	MV_U16     PDIDs[MAX_HD_SUPPORTED_API]; // <=> InExtents[]
	MV_U8      Reserved4[128];
} CreateOrModify_DG_Param, * PCreateOrModify_DG_Param;

typedef struct  _CreateOrModify_VD_Param
{
	MV_U16     VDID;                      // <=> DeviceID
	MV_U16     DGID;                      // <=> InPool  (parent pool)
	MV_U8      Name[LD_MAX_NAME_LENGTH];  // <=> ElementName
	MV_U8      RaidMode;                  // <=> Goal
	MV_U8      SubVDCount;                // for raid 10, 50, 60
	MV_U8      NumParityDisk;             // for RAID 6
	MV_U8      DGSlotNum;                 // Specify which DG slot the VD will be created upon. If set to 0xFF, driver will pick a slot with closest specified size.
	MV_U8      Reserved1[6];
	MV_U16     StripeBlockSize;           // <=> Goal.UserDataStripeDepth (between API and driver, it is # of BlockSize)
	MV_U64     Size;                      // <=> Size (in unit of BlockSize to/from driver)

	MV_U8      RoundingScheme;            // refer to ROUNDING_SCHEME_XXX
	MV_U8      CachePolicy;               // refer to CACHEMODE_XXXX
	MV_U8      InitializationOption;      //  refer to INIT_XXXX.
	MV_U8      SectorCoefficient;         // (sector size) 1=>512 (default)
	MV_U32     BlockSize;                 // used to calculate StripeBlockSize.
	MV_U8      Reserved[256];
} CreateOrModify_VD_Param, * PCreateOrModify_VD_Param;

typedef struct _DG_Info
{
	MV_U16     DGID;                      // <=> InstanceID
	MV_U8      Reserved1[2];
	MV_U8      Name[LD_MAX_NAME_LENGTH];  // <=> ElementName
	// other properties proprietary to Marvell
	MV_U8      RaidMode;
	MV_U8      SubVDCount;                // for raid 10, 50, 60
	MV_U8      NumParityDisk;             // for RAID 6
	MV_U8      AdapterID;

	MV_U8      Reserved2[2];
	MV_U16     StripeBlockSize;           // between RAIDAPI and driver, it is # of BlockSize
	MV_U32     BlockSize;                 // disk block size

	MV_U64     TotalManagedSpace;         // <=> TotalManagedSpace (in unit of BlockSize between API and driver)
	MV_U64     RemainingManagedSpace;     // <=> RemainingManagedSpace in unit of BlockSize between API and driver)
	MV_U64     SmallestAvailablePDSize;   // Available size of the smallest PD in this DG in unit of BlockSize between API and driver
	MV_U8      Status;                    // OperationalStatus, refer to DG_STATUS_xxx
	MV_U8      Reserved3[2];
	MV_U8      PDCount;
	MV_U16     PDIDs[MAX_HD_SUPPORTED_API];
	MV_U8      Reserved4[3];
	// VD slots are contiguous space in DG with existing VD on it or can be used to create VD.
	// If VD created on empty slot does not use up all its slot space, the remaing space will be used to create
	// another slot unless total slots number reaches MaxVDPerDG as defined in Adapter_Info. When total slots reaches
	// it maximum number, VD created on the slot has to use up all its space.
	MV_U8      VDSlotCount;						// current number of slots in DG
	MV_U64     VDSlotSize[MAX_LD_SUPPORTED_API]; // size of each slot (including slot with VD)
	MV_U16     VDIDs[MAX_LD_SUPPORTED_API];		 // VD ID corresponding to above slot. Entries with 0xFFFF means empty slot
	MV_U8      Reserved5[3];
	MV_U8      SparePDCount;
	MV_U16     SparePDIDs[MAX_SPARE_PD_SUPPORTED_API];
	MV_U16     BgaType;
	MV_U8      BgaState;
	MV_U8      Reserved6[25];
} DG_Info, *PDG_Info;

// Returning all PDs (and its status) that are not used by any DG on given adapter including spare, offline PD etc.
typedef struct _PD_NotInAny_DG
{
	MV_U8      AdapterID;
	MV_U8      Reserved1[1];
	MV_U16     PDCount;
	MV_U16     PDIDs[MAX_HD_SUPPORTED_API];
	MV_U8      Status[MAX_HD_SUPPORTED_API];   // Status of the corresponding PD in PDIDs above.  Refer to HD_STATUS_XXX.
	MV_U8      Reserved2[252];
} PD_NotInAny_DG, *PPD_NotInAny_DG;

typedef struct _DG_CONFIG
{
	MV_U16            DGID;
	MV_BOOLEAN        WriteCacheOn;            // 1: enable write cache
	MV_U8             Reserved;
	MV_U8             Name[LD_MAX_NAME_LENGTH];  // DG Name
	MV_U8             Reserved1[12];
}DG_Config, *PDG_Config;

#pragma pack()

#endif
