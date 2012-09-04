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
#ifndef _CORE_RMW_FLASH
#define _CORE_RMW_FLASH
#include "mv_config.h"

/*******************************************************************************
* Structs and defines                                                          *
*******************************************************************************/
enum {
	FLASH_SECTOR_SIZE       = 0x10000,
};
typedef struct _lib_rmw_flash {
	MV_PVOID        adapter_info;
	MV_U8           success;
	MV_U8           reserved[3];
	MV_U32          buf_size;
	MV_PU8          buf_ptr;
} lib_rmw_flash;

/*******************************************************************************
* Function Prototypes                                                          *
*******************************************************************************/
MV_U8 core_init_rmw_flash_memory(MV_PVOID core_p,
        lib_resource_mgr *rsrc, MV_U16 max_io);
MV_U32 core_rmw_flash_get_cached_memory_quota(MV_U16 max_io);
MV_U32 core_rmw_flash_get_dma_memory_quota(MV_U16 max_io);
MV_U8 core_rmw_read_flash(MV_PVOID core_p, MV_U32 addr,
        MV_PU8 data, MV_U32 size);
MV_U8 core_rmw_write_flash(MV_PVOID core_p, MV_U32 addr,
        MV_PU8 data, MV_U32 size);

#endif
