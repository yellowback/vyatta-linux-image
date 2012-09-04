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
#if !defined(CORE_API_H)
#define CORE_API_H

#include "mv_config.h"


/* api related functions */
void Core_Change_LED(
	IN MV_PVOID core,
	IN MV_U16 device_id,
	IN MV_U8 flag);

MV_VOID Core_GetHDInfo(
	IN MV_PVOID core,
	IN MV_U16 HDId,
	OUT MV_PVOID buffer,
        IN MV_BOOLEAN useVariableSize);

MV_BOOLEAN Core_Set_SCSIID(MV_PVOID core_p, MV_PU16 device_id, 
        MV_PU16 scsi_id, MV_U16 entry_size);
MV_BOOLEAN Core_Get_SCSIID(MV_PVOID core_p, MV_PU16 device_id, 
        MV_PU16 scsi_id, MV_U16 entry_size);

MV_U8 core_sgpio_set_led( MV_PVOID extension, MV_U16 device_id, MV_U8 light_type,
        MV_U8 light_behavior, MV_U8 flag);
#define MAX_BIOS_SIZE           192L * 1024
#endif /* CORE_API_H */
