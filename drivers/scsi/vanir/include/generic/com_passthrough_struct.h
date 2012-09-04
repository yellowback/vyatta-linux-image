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
#ifndef __MV_COM_PASS_THROUGH_H__
#define __MV_COM_PASS_THROUGH_H__
#include "com_define.h"
#define PASSTHROUGH_ECC_WRITE	1
#define PASSTHROUGH_ECC_SIZE	2

#define SECTOR_LENGTH                           512
#define SECTOR_WRITE                            0
#define SECTOR_READ                             1

#define MAX_PASS_THRU_DATA_BUFFER_SIZE (SECTOR_LENGTH+128)

#pragma pack(8)

typedef struct {
 // We put Data_Buffer[] at the very beginning of this structure because SCSI commander did so.
 MV_U8    Data_Buffer[MAX_PASS_THRU_DATA_BUFFER_SIZE];  // set by driver if read, by application if write
 MV_U8    Reserved1[128];
 MV_U32   Data_Length; // set by driver if read, by application if write 
 MV_U16   DevId;       // PD ID (used by application only)
 MV_U8    CDB_Type;    // define a CDB type for each CDB category (used by application only)
 MV_U8    Reserved2;
 MV_U32   lba;
 MV_U8    Reserved3[64];
} PassThrough_Config, * PPassThorugh_Config;

typedef struct {
 // The data structure is used in conjunction with APICDB0_PASS_THRU_CMD_SCSI_16 (see com_api.h)
 // when CDB[16] is required.
 MV_U8    CDB[16];	// CDB is embedded in data buffer
 MV_U32    buf[1];	// actually input/output data buffer
} PassThrough_Config_16, * PPassThorugh_Config_16;

typedef struct _Pass_Through_Cmd
{
	MV_U8 	cdb[16];
	MV_U16 	data_length;
	MV_U8	Reserved[2];	// pad 2 bytes for DWORD alignment.
	MV_U8 	data[1];
}Pass_Through_Cmd,*PPass_Through_Cmd;

#pragma pack()

#endif
