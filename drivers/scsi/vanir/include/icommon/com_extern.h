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
#ifndef  __MV_COM_EXTERN_H__
#define  __MV_COM_EXTERN_H__

#include "com_define.h"
/* Target device type */
#define TARGET_TYPE_LD                          	0
#define TARGET_TYPE_FREE_PD                 	1

#define DISK_TYPE_RAID					0
#define DISK_TYPE_SATA					1
#define DISK_TYPE_SAS					2

// Giving TargetID and LUN, returns it Type and DeviceID.  If returned Type or DeviceID is 0xFF, not found.
typedef struct    _TargetLunType
{
 MV_U8            AdapterID;
 MV_U8            TargetID;
 MV_U8            Lun;
 MV_U8            Type;            // TARGET_TYPE_LD or TARGET_TYPE_FREE_PD
 MV_U16           DeviceID;        // LD ID or PD ID depends on Type
 MV_U8            Reserved[34];
}TargetLunType, * PTargetLunType;

typedef struct	_OS_disk_info
{
	MV_U8 		ataper_id;	/* ataper disk locates */
	MV_U8 		disk_type;	/* RAID disk, SATA disk or SAS disk */
	MV_U16		device_id;	/* contain target id and lun */
	MV_U16		queue_depth;	/* queue depth support this disk */
}OS_disk_info, *POS_disk_info;

#endif
