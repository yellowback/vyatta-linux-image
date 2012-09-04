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
#include "mv_config.h"
#include "core_type.h"
#include "core_manager.h"
#include "core_internal.h"
#include "core_util.h"
#include "hba_inter.h"

#include "core_spi.h"


MV_U8
core_init_rmw_flash_memory(MV_PVOID core_p,
        lib_resource_mgr *rsrc, MV_U16 max_io)
{
	core_extension  *core = (core_extension *)core_p;
	HBA_Extension   *hba = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);
	AdapterInfo     *adapter;
	MV_U32          item_size;
	MV_PU8          vir;

	item_size = sizeof(AdapterInfo);
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	core->lib_flash.adapter_info = (MV_PVOID)vir;
	adapter = (AdapterInfo *)vir;

	adapter->bar[FLASH_BAR_NUMBER] = hba->Base_Address[FLASH_BAR_NUMBER];
	if (-1 == OdinSPI_Init(adapter)) {
	        core->lib_flash.buf_ptr = NULL;
	        core->lib_flash.buf_size = 0;
	        core->lib_flash.success = MV_FALSE;
	        CORE_DPRINT(("Flash init failed\n"));
	        return MV_FALSE;
	}
	item_size = adapter->FlashSectSize;

	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL)
		return MV_FALSE;
	core->lib_flash.buf_ptr = vir;
	core->lib_flash.buf_size = item_size;
	core->lib_flash.success = MV_TRUE;
       return MV_TRUE;
}

MV_U32
core_rmw_flash_get_cached_memory_quota(MV_U16 max_io)
{
	MV_U32          size = 0;

	size += sizeof(AdapterInfo);
	size += FLASH_SECTOR_SIZE;
	return size;
}

MV_U32
core_rmw_flash_get_dma_memory_quota(MV_U16 max_io)
{
        return 0;
}

MV_U8
core_rmw_read_flash(MV_PVOID core_p,
        MV_U32 addr,
        MV_PU8 data,
        MV_U32 size)
{
        core_extension  *core = (core_extension *)core_p;
        HBA_Extension   *hba = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);
        AdapterInfo	*adapter_info;
        MV_PU8          vir;

        adapter_info = (AdapterInfo *)core->lib_flash.adapter_info;
        if (core->lib_flash.success == MV_FALSE) {
                if (-1 == OdinSPI_Init(adapter_info)) {
                        CORE_DPRINT(("Flash init failed\n"));
                        return MV_FALSE;
                }
                vir = (MV_PU8)lib_rsrc_malloc_cached(&core->lib_rsrc,
                                adapter_info->FlashSectSize);
                if (vir == NULL) return MV_FALSE;
                core->lib_flash.buf_ptr = vir;
                core->lib_flash.buf_size = adapter_info->FlashSectSize;
                core->lib_flash.success = MV_TRUE;
        }

	OdinSPI_ReadBuf(adapter_info, addr, data, size);
       return MV_TRUE;
}

MV_U8
core_rmw_write_flash(MV_PVOID core_p,
		MV_U32 addr,
		MV_PU8 data,
		MV_U32 size)
{
	MV_U32 data_tmp_addr, buf_tmp_addr;
	MV_U32 data_tmp_size, buf_tmp_size;
	MV_U32 end, data_offset, buf_offset;
	MV_PU8 tmp_buf;
	core_extension  *core = (core_extension *)core_p;
	HBA_Extension   *hba = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);
	AdapterInfo	*adapter_info;
	MV_PU8          vir;

	adapter_info = (AdapterInfo *)core->lib_flash.adapter_info;
	if (core->lib_flash.success == MV_FALSE) {
		if (-1 == OdinSPI_Init(adapter_info)) {
			CORE_DPRINT(("Flash init failed\n"));
			return MV_FALSE;
		}
		vir = (MV_PU8)lib_rsrc_malloc_cached(&core->lib_rsrc,
		adapter_info->FlashSectSize);
		if (vir == NULL)
			return MV_FALSE;
		core->lib_flash.buf_ptr = vir;
		core->lib_flash.buf_size = adapter_info->FlashSectSize;
		core->lib_flash.success = MV_TRUE;
	}
	tmp_buf = core->lib_flash.buf_ptr;
	if ((data == NULL) || (tmp_buf == NULL))
		return MV_FALSE;

	data_tmp_addr = addr;
	data_tmp_size = 0;
	data_offset = 0;
	end = addr + size;

	buf_tmp_size = adapter_info->FlashSectSize;
	while (data_tmp_addr + data_tmp_size < end) {
		data_tmp_addr += data_tmp_size;
		buf_tmp_addr = (data_tmp_addr / buf_tmp_size) * buf_tmp_size;

		data_tmp_size = MV_MIN(buf_tmp_addr + buf_tmp_size, end)
		- data_tmp_addr;

		data_offset = data_tmp_addr - addr;
		if (data_offset == 0)
			buf_offset = data_tmp_addr - buf_tmp_addr;
		else
			buf_offset = 0;

		OdinSPI_ReadBuf(adapter_info, buf_tmp_addr,
		tmp_buf, buf_tmp_size);

		MV_CopyMemory(&tmp_buf[buf_offset],
		&data[data_offset], data_tmp_size);

		if (OdinSPI_SectErase(adapter_info, data_tmp_addr)
			== -1) {
			CORE_DPRINT(("Flash Erase Failed\n"));
			return MV_FALSE;
		}

		OdinSPI_WriteBuf(adapter_info, buf_tmp_addr,
		tmp_buf, buf_tmp_size);
	}

	MV_DASSERT(data_tmp_addr + data_tmp_size == end);
	return MV_TRUE;
}

