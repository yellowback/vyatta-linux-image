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
#ifndef __CORE_RESOURCE_H
#define __CORE_RESOURCE_H

#include "mv_config.h"
#include "core_type.h"
#include "core_protocol.h"

struct _core_extension;

/*
 * resource management for sas/sata part
 */
MV_U32 core_get_cached_memory_quota(MV_U16 max_io);
MV_U32 core_get_dma_memory_quota(MV_U16 max_io);
MV_BOOLEAN core_init_cached_memory(struct _core_extension *core, 
	lib_resource_mgr *lib_rsrc, MV_U16 max_io);
MV_BOOLEAN core_init_dma_memory(struct _core_extension * core,
	lib_resource_mgr *lib_rsrc, MV_U16 max_io);

/*
 * resource management interface
 */
void core_get_supported_dev(MV_U16 max_io, MV_PU16 hd_count, MV_PU8 exp_count,
	MV_PU8 pm_count, MV_PU8 enc_count);

void core_get_supported_pl_counts(MV_U16 max_io, MV_PU16 slot_count,
	MV_PU16 delv_q_size, MV_PU16 cmpl_q_size, MV_PU16 received_fis_count);

void xor_get_supported_pl_counts(MV_U16 max_io, MV_PU16 slot_count,
	MV_PU16 delv_q_size, MV_PU16 cmpl_q_size);

void core_get_supported_pal_counts(MV_U16 max_io, MV_PU16 intl_req_count,
	MV_PU16 req_sg_entry_count, MV_PU16 hw_sg_entry_count, MV_PU16 hw_sg_buf_count,
	MV_PU16 scratch_buf_count, MV_PU16 context_count,
	MV_PU16 event_count);

enum resource_type {
	CORE_RESOURCE_TYPE_INTERNAL_REQ = 1,
	CORE_RESOURCE_TYPE_CONTEXT,
	CORE_RESOURCE_TYPE_SCRATCH_BUFFER,
	CORE_RESOURCE_TYPE_SG_BUFFER,
	CORE_RESOURCE_TYPE_EVENT_RECORD,

	CORE_RESOURCE_TYPE_DOMAIN_DEVICE,
	CORE_RESOURCE_TYPE_DOMAIN_EXPANDER,
	CORE_RESOURCE_TYPE_DOMAIN_PM,
	CORE_RESOURCE_TYPE_DOMAIN_ENCLOSURE,
	CORE_RESOURCE_TYPE_SPIN_UP_DEVICE,
};

typedef struct _resource_func_tbl {
	/* dynamic memory malloc free function */
	void *(*malloc)(void *extension, MV_U32 size, MV_U8 mem_type, MV_U16 alignment, MV_PHYSICAL_ADDR *phy);
	void (*free)(const void * obj);
	void * extension;
} resource_func_tbl;

struct _lib_resource_mgr {
	/*
	 * global cached/dma memory management variables
	 */
	/* global cached memory buffer */
	MV_PVOID global_cached_vir;
	MV_PVOID free_cached_vir;
	MV_U32 total_cached_size;
	MV_U32 free_cached_size;

	/* global dma memory virtual and physical address */
	MV_PVOID global_dma_vir;
	MV_PHYSICAL_ADDR global_dma_phy;
	MV_PVOID free_dma_vir;
	MV_PHYSICAL_ADDR free_dma_phy;
	MV_U32 total_dma_size;
	MV_U32 free_dma_size;

	resource_func_tbl	func_tbl;
	lib_device_mgr          *lib_dev;

	/*
	 * allocated cached and dma memory
	 */
	MV_U16                 intl_req_count;
	MV_U16                 context_count;
	MV_U16                 scratch_buf_count;
	MV_U16                 hw_sg_buf_count;
	MV_U8                  event_count;

	/* resource pool for cached memory */
	MV_Request             *intl_req_pool;	/* internal requests pool */
	core_context           *context_pool; /* core context pool */

	/* resource pool for dma memory */
	hw_buf_wrapper         *scratch_buf_pool; /* scratch buffer */
	hw_buf_wrapper         *hw_sg_buf_pool; /* dma sg buffer for ASIC access */
	event_record           *event_pool; /* event record pool */
	struct device_spin_up	*spin_up_device_pool;
	
	/*
	 * allocated device data structure
	 */
	MV_U16                  hd_count; /* how many domain_device available */
	MV_U8                  exp_count;
	MV_U8                  pm_count;
	MV_U8                  enc_count;
	MV_U8			reserved0;
	MV_U16                  device_count;

	domain_device          *hds;
	domain_expander        *expanders;
	domain_pm              *pms;
	domain_enclosure       *enclosures;

};

void * core_malloc(MV_PVOID root_p, lib_resource_mgr *rsrc, MV_U8 obj_type);
void core_free(MV_PVOID root_p, lib_resource_mgr *rsrc, void * obj, MV_U8 obj_type);

void * lib_rsrc_malloc_cached(lib_resource_mgr *rsrc, MV_U32 size);
void * lib_rsrc_malloc_dma(lib_resource_mgr *rsrc, MV_U32 size, 
	MV_U16 alignment, MV_PHYSICAL_ADDR *phy);

void lib_rsrc_init(lib_resource_mgr *rsrc, MV_PVOID cached_vir, MV_U32 cached_size,
	MV_PVOID dma_vir, MV_PHYSICAL_ADDR dma_phy, MV_U32 dma_size,
	resource_func_tbl *func, lib_device_mgr *lib_dev);

#define get_intl_req(rsrc) \
	(PMV_Request)core_malloc(NULL, rsrc, CORE_RESOURCE_TYPE_INTERNAL_REQ)

#define free_intl_req(rsrc, req) \
	core_free(NULL, rsrc, req, CORE_RESOURCE_TYPE_INTERNAL_REQ)

#define get_core_context(rsrc) \
	(core_context *)core_malloc(NULL, rsrc, CORE_RESOURCE_TYPE_CONTEXT)

#define free_core_context(rsrc, context) \
	core_free(NULL, rsrc, context, CORE_RESOURCE_TYPE_CONTEXT)

#define get_scratch_buf(rsrc) \
	(hw_buf_wrapper *)core_malloc(NULL, rsrc, CORE_RESOURCE_TYPE_SCRATCH_BUFFER)

#define free_scratch_buf(rsrc, wrapper) \
	core_free(NULL, rsrc, wrapper, CORE_RESOURCE_TYPE_SCRATCH_BUFFER)

#define get_sg_buf(rsrc) \
	(hw_buf_wrapper *)core_malloc(NULL, rsrc, CORE_RESOURCE_TYPE_SG_BUFFER)

#define free_sg_buf(rsrc, wrapper) \
	core_free(NULL, rsrc, wrapper, CORE_RESOURCE_TYPE_SG_BUFFER)
#define get_spin_up_device_buf(rsrc) \
		(struct device_spin_up *)core_malloc(NULL, rsrc, CORE_RESOURCE_TYPE_SPIN_UP_DEVICE)
#define free_spin_up_device_buf(rsrc, device) \
		core_free(NULL, rsrc, device, CORE_RESOURCE_TYPE_SPIN_UP_DEVICE)
#define get_event_record(rsrc) \
	(event_record *)core_malloc(NULL, rsrc, CORE_RESOURCE_TYPE_EVENT_RECORD)

#define free_event_record(rsrc, event) \
	core_free(NULL, rsrc, event, CORE_RESOURCE_TYPE_EVENT_RECORD)

/* 
 * device object data structure allocation and free 
 */
#define get_device_obj(root_p, rsrc) \
	(domain_device *)core_malloc(root_p, rsrc, CORE_RESOURCE_TYPE_DOMAIN_DEVICE)

#define free_device_obj(root_p, rsrc, dev) \
	core_free(root_p, rsrc, dev, CORE_RESOURCE_TYPE_DOMAIN_DEVICE)

#define get_expander_obj(root_p, rsrc) \
	(domain_expander *)core_malloc(root_p, rsrc, CORE_RESOURCE_TYPE_DOMAIN_EXPANDER)

#define free_expander_obj(root_p, rsrc, exp) \
	core_free(root_p, rsrc, exp, CORE_RESOURCE_TYPE_DOMAIN_EXPANDER)

#define get_pm_obj(root_p, rsrc) \
	(domain_pm *)core_malloc(root_p, rsrc, CORE_RESOURCE_TYPE_DOMAIN_PM)

#define free_pm_obj(root_p, rsrc, pm) \
	core_free(root_p, rsrc, pm, CORE_RESOURCE_TYPE_DOMAIN_PM)

#define get_enclosure_obj(root_p, rsrc) \
	(domain_enclosure *)core_malloc(root_p, rsrc, CORE_RESOURCE_TYPE_DOMAIN_ENCLOSURE)

#define free_enclosure_obj(root_p, rsrc,enc) \
	core_free(root_p, rsrc,enc,CORE_RESOURCE_TYPE_DOMAIN_ENCLOSURE)

#endif /* __CORE_RESOURCE_H */
