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
#include "core_resource.h"
#include "core_manager.h"

#include "com_u64.h"
#include "com_dbg.h"

#include "core_sas.h"
#include "core_error.h"
#include "core_expander.h"
#include "core_console.h"
#include "core_util.h"

void core_get_supported_dev (
	IN MV_U16 max_io,	/* max io count */
	OUT MV_PU16 hd_count, /* HDD count */
	OUT MV_PU8 exp_count, /* expander count */
	OUT MV_PU8 pm_count, /* pm count */
	OUT MV_PU8 enc_count /* enclosure count */
	)
{
	*hd_count = CORE_MAX_DEVICE_SUPPORTED;

	*pm_count = CORE_MAX_PM_SUPPORTED;
	if (max_io == 1) {
		*exp_count = CORE_MIN_EXPANDER_SUPPORTED;
		*enc_count = CORE_MIN_ENC_SUPPORTED;
	} else {
		*exp_count = CORE_MAX_EXPANDER_SUPPORTED;
		*enc_count = CORE_MAX_ENC_SUPPORTED;
	}
}

void core_get_supported_pl_counts(
	IN MV_U16 max_io,	/* max io count */
	OUT MV_PU16 slot_count, /* hardware I/O slot count */
	OUT MV_PU16 delv_q_size, /* deliver queue size */
	OUT MV_PU16 cmpl_q_size, /* completion queue size */
	OUT MV_PU16 received_fis_count /* max received FISes we can save. Depends on register set */
	)
{
	if (max_io == 1) {
		*slot_count = 1;
		*delv_q_size = *slot_count + 1;
		*received_fis_count = 1;
	} else {
		*slot_count = CORE_MAX_REQUEST_NUMBER;
		*delv_q_size = *slot_count;
		*received_fis_count = MAX_REGISTER_SET_PER_IO_CHIP;
	}
	*cmpl_q_size = *slot_count + 1 + 10;
}

void xor_get_supported_pl_counts(
	IN MV_U16 max_io,	/* max io count */
	OUT MV_PU16 slot_count, /* hardware I/O slot count */
	OUT MV_PU16 delv_q_size, /* deliver queue size */
	OUT MV_PU16 cmpl_q_size /* completion queue size */
	)
{
	if (max_io == 1) {
		*slot_count = 1;
		*delv_q_size = *slot_count + 1;
	} else {
		*slot_count = XOR_MAX_SLOT_NUMBER;
		*delv_q_size = *slot_count;
	}

	*cmpl_q_size = *slot_count + 1;
}

void core_get_supported_pal_counts(
	IN MV_U16 max_io,	/* max io count */
	OUT MV_PU16 intl_req_count, /* internal request count, used for */
		/* a. initialization(InternalReqCount) b. sub req for large request support(SubReqCount) */
	OUT MV_PU16 req_sg_entry_count, /* cached memory sg entry count for internal requests */
	OUT MV_PU16 hw_sg_entry_count, /* dma sg scratch buffer entry count */
	OUT MV_PU16 hw_sg_buf_count, /* dma sg scratch buffer count */
	OUT MV_PU16 scratch_buf_count, /* dma internal req scratch buffer, used for */
		/* a.identify(SATAScratchCount), b. SES (SESControlCount), c. SMP(SMPScratchCount) */
	OUT MV_PU16 context_count, /* core request context for both internal/external req */
	OUT MV_PU16 event_count /* hardware event count */
   )
{
	MV_U16 slot_count, delv_q_size, cmpl_q_size, received_fis_count;
	core_get_supported_pl_counts(max_io, &slot_count, &delv_q_size, &cmpl_q_size, &received_fis_count);

	if (max_io == 1) {
		*intl_req_count = CORE_MIN_INTERNAL_REQ_COUNT;
		*req_sg_entry_count = CORE_MIN_REQ_SG_ENTRY_COUNT;
		*hw_sg_entry_count = CORE_MIN_HW_SG_ENTRY_COUNT;
		*hw_sg_buf_count = CORE_MIN_HW_SG_BUFFER_COUNT;
		*scratch_buf_count = CORE_MIN_SCRATCH_BUFFER_COUNT;
		*event_count = CORE_MIN_HW_EVENT_COUNT;
	} else {
		/* internal req count = init req count + sub req for large request support */
		*intl_req_count = CORE_MAX_INTERNAL_REQ_COUNT;
		*req_sg_entry_count = CORE_MAX_REQ_SG_ENTRY_COUNT;

		*hw_sg_entry_count = CORE_MAX_HW_SG_ENTRY_COUNT;
		*hw_sg_buf_count = CORE_MAX_HW_SG_BUFFER_COUNT;
		*scratch_buf_count = CORE_MAX_SCRATCH_BUFFER_COUNT; /* SATA + SES + SMP */
		*event_count = CORE_MAX_HW_EVENT_COUNT;
	}

	*context_count = *intl_req_count + slot_count * MAX_NUMBER_IO_CHIP;
}


#define INTERNAL_REQ_TOTAL_SIZE(sg_entry_n) \
	(sizeof(MV_Request) + sizeof(MV_SG_Entry) * sg_entry_n + sizeof(struct _sense_data))

MV_U32 core_get_cached_memory_quota(MV_U16 max_io)
{
	MV_U32 size = 0;

	MV_U16 slot_count, delv_q_size, cmpl_q_size, received_fis_count;
	MV_U16 intl_req_count, hw_sg_buf_count, scratch_buf_count, context_count;
	MV_U16 req_sg_entry_count, hw_sg_entry_count, event_count;
	MV_U8 exp_count, pm_count, enc_count;
	MV_U16 hd_count;

	core_get_supported_pl_counts(max_io, &slot_count, &delv_q_size, &cmpl_q_size, &received_fis_count);
	core_get_supported_pal_counts(max_io, &intl_req_count, &req_sg_entry_count,
		&hw_sg_entry_count, &hw_sg_buf_count, &scratch_buf_count, &context_count,
		&event_count);
	core_get_supported_dev(max_io, &hd_count, &exp_count, &pm_count, &enc_count);
	size += ROUNDING(sizeof(core_extension), 8);
	/*
	 * The following are the pure cached memory allocation.
	 */
	/* memory for running_req to store the running requests on slot */
	size += ROUNDING(sizeof(PMV_Request) * slot_count * MAX_NUMBER_IO_CHIP, 8);

	/* memory for internal reqs */
	size += ROUNDING(intl_req_count * INTERNAL_REQ_TOTAL_SIZE(req_sg_entry_count), 8);
	/* request context for both external and internal requests */
	size += ROUNDING(context_count * sizeof(core_context), 8);
	/* tag pools */
	size += ROUNDING(slot_count * MAX_NUMBER_IO_CHIP * sizeof(MV_U16), 8);
	size += ROUNDING(MAX_NUMBER_IO_CHIP * sizeof(MV_U16), 8);

	/*
	 * The following are the wrapper for dma memory allocation.
	 */
	size += ROUNDING(sizeof(hw_buf_wrapper) * scratch_buf_count, 8);
	size += ROUNDING(sizeof(hw_buf_wrapper) * hw_sg_buf_count, 8);
	/* it's used to store command table address. command table is not contiguent. */
	size += ROUNDING(sizeof(hw_buf_wrapper) * slot_count * MAX_NUMBER_IO_CHIP, 8);
	size += ROUNDING(sizeof(event_record) * event_count, 8);
	size += ROUNDING(sizeof(saved_fis) * received_fis_count * MAX_NUMBER_IO_CHIP, 8);

	/* get device data structure memory */
	size += ROUNDING(sizeof(domain_device) * hd_count, 8);
	size += ROUNDING(sizeof(domain_expander) * exp_count, 8);
	size += ROUNDING(sizeof(domain_pm) * pm_count, 8);
	size += ROUNDING(sizeof(domain_enclosure) * enc_count, 8);

	return size;
}

MV_U32 core_get_dma_memory_quota(MV_U16 max_io)
{
	MV_U32 size = 0;

	MV_U16 scratch_buf_count, intl_req_count;
	MV_U16 req_sg_entry_count, hw_sg_entry_count, event_count;
	MV_U16 slot_count, hw_sg_buf_count, delv_q_size, cmpl_q_size, received_fis_count;
	MV_U16 context_count;

	core_get_supported_pl_counts(max_io, &slot_count, &delv_q_size, &cmpl_q_size, &received_fis_count);
	core_get_supported_pal_counts(max_io, &intl_req_count, &req_sg_entry_count,
		&hw_sg_entry_count, &hw_sg_buf_count, &scratch_buf_count, &context_count,
		&event_count);

	/*
	 * For dma memory, need extra bytes for ASIC alignment requirement.
	 * delivery queue/completion queue base address must be 64 byte aligned.
	 */
	size = 64 + sizeof(mv_command_header) * slot_count * MAX_NUMBER_IO_CHIP;	/* command list*/

	size += 128 + sizeof(mv_command_table) * slot_count * MAX_NUMBER_IO_CHIP;	/* Command Table */

	/* received FIS */
	if (max_io > 1) {
		size += (256 + MAX_RX_FIS_POOL_SIZE) * MAX_NUMBER_IO_CHIP;
	} else {
		size += 256 + MIN_RX_FIS_POOL_SIZE;
	}

	size += (64 + sizeof(MV_U32) * delv_q_size) * MAX_NUMBER_IO_CHIP;
	size += (64 + sizeof(MV_U32) * cmpl_q_size) * MAX_NUMBER_IO_CHIP;

	/* scratch buffer for initialization/SES/SMP */
	size += 8 + SCRATCH_BUFFER_SIZE * scratch_buf_count;

	/* buffer for dma SG tables */
	size += 8 + CORE_HW_SG_ENTRY_SIZE * hw_sg_entry_count * hw_sg_buf_count;

	if( max_io != 1 ) {
		size += 8 + TRASH_BUCKET_SIZE;
	}

	return size;
}

MV_BOOLEAN lib_rsrc_allocate_device(lib_resource_mgr *rsrc, MV_U16 max_io)
{
	MV_U16 hd_count;
	MV_U8 exp_count, pm_count, enc_count;
	MV_U32 item_size;
	int i;

	core_get_supported_dev(max_io, &hd_count, &exp_count, &pm_count, &enc_count);

	/* allocate memory for hd */
	if (hd_count > 0) {
		item_size = sizeof(domain_device) * hd_count;
		rsrc->hds = (domain_device *)lib_rsrc_malloc_cached(rsrc, item_size);
		if (rsrc->hds == NULL) return MV_FALSE;
		for (i = 0; i < hd_count-1; i++){
			rsrc->hds[i].base.queue_pointer.next =
				(List_Head *)&rsrc->hds[i+1];
		}
		rsrc->hds[hd_count-1].base.queue_pointer.next = NULL;
	}
	rsrc->hd_count = hd_count;

	/* allocate memory for expander */
	if (exp_count > 0) {
		item_size = sizeof(domain_expander) * exp_count;
		rsrc->expanders = (domain_expander *)lib_rsrc_malloc_cached(rsrc, item_size);
		if (rsrc->expanders == NULL) return MV_FALSE;
		for (i = 0; i < exp_count-1; i++){
			rsrc->expanders[i].base.queue_pointer.next =
				(List_Head *)&rsrc->expanders[i+1];
		}
		rsrc->expanders[exp_count-1].base.queue_pointer.next = NULL;
	}
	rsrc->exp_count = exp_count;

	/* allocate memory for pm */
	if (pm_count > 0) {
		item_size = sizeof(domain_pm) * pm_count;
		rsrc->pms = (domain_pm *)lib_rsrc_malloc_cached(rsrc, item_size);
		if (rsrc->pms == NULL) return MV_FALSE;
		for (i = 0; i < pm_count-1; i++){
			rsrc->pms[i].base.queue_pointer.next =
				(List_Head *)&rsrc->pms[i+1];
		}
		rsrc->pms[pm_count-1].base.queue_pointer.next = NULL;
	}
	rsrc->pm_count = pm_count;

	/* allocate memory for enclosure */
	if (enc_count > 0) {
		item_size = sizeof(domain_enclosure) * enc_count;
		rsrc->enclosures = (domain_enclosure *)lib_rsrc_malloc_cached(rsrc, item_size);
		if (rsrc->enclosures == NULL) return MV_FALSE;
		for (i = 0; i < enc_count-1; i++){
			rsrc->enclosures[i].base.queue_pointer.next =
				(List_Head *)&rsrc->enclosures[i+1];
		}
		rsrc->enclosures[enc_count-1].base.queue_pointer.next = NULL;
	}
	rsrc->enc_count = enc_count;

	return MV_TRUE;
}


MV_BOOLEAN lib_rsrc_allocate(lib_resource_mgr *rsrc, MV_U16 max_io)
{
	MV_U16 intl_req_count, hw_sg_buf_count, scratch_buf_count, context_count;
	MV_U16 req_sg_entry_count, hw_sg_entry_count, event_count;

	MV_U32 item_size;
	MV_PU8 vir;

	MV_Request *req = NULL;
	hw_buf_wrapper *wrapper = NULL;
	core_context *context = NULL;
	event_record *event = NULL;
	struct device_spin_up *device_su=NULL;
	MV_U16 i;

	MV_BOOLEAN ret;

	core_get_supported_pal_counts(max_io, &intl_req_count, &req_sg_entry_count,
		&hw_sg_entry_count, &hw_sg_buf_count, &scratch_buf_count, &context_count,
		&event_count);

	/* allocate memory for internal request including sg entry and sense */
	item_size = INTERNAL_REQ_TOTAL_SIZE(req_sg_entry_count) * intl_req_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	rsrc->intl_req_pool = NULL;
	for (i = 0; i < intl_req_count; i++) {
		req = (MV_Request *)vir;
		vir += sizeof(MV_Request);

		req->SG_Table.Entry_Ptr = (PMV_SG_Entry)vir;
		req->SG_Table.Max_Entry_Count = req_sg_entry_count;
		vir += sizeof(MV_SG_Entry) * req_sg_entry_count;

		req->Sense_Info_Buffer = vir;
		req->Sense_Info_Buffer_Length = sizeof(sense_data);
		vir += sizeof(sense_data);

		req->Queue_Pointer.next = (List_Head *)rsrc->intl_req_pool;
		rsrc->intl_req_pool = req;
	}
	rsrc->intl_req_count = intl_req_count;

	item_size = sizeof(core_context) * context_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	rsrc->context_pool = NULL;
	for (i = 0; i < context_count; i++) {
		context = (core_context *)vir;
		context->type = CORE_CONTEXT_TYPE_NONE;
		context->next = rsrc->context_pool;
		rsrc->context_pool = context;
		vir += sizeof(core_context);
	}
	rsrc->context_count = context_count;

	item_size = sizeof(hw_buf_wrapper) * scratch_buf_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	rsrc->scratch_buf_pool = NULL;
	for (i = 0; i < scratch_buf_count; i++) {
		wrapper = (hw_buf_wrapper *)vir;
		wrapper->next = rsrc->scratch_buf_pool;
		rsrc->scratch_buf_pool = wrapper;
		vir += sizeof(hw_buf_wrapper);
	}
	rsrc->scratch_buf_count = scratch_buf_count;

	item_size = sizeof(hw_buf_wrapper) * hw_sg_buf_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	rsrc->hw_sg_buf_pool = NULL;
	for (i = 0; i < hw_sg_buf_count; i++) {
		wrapper = (hw_buf_wrapper *)vir;
		wrapper->next = rsrc->hw_sg_buf_pool;
		rsrc->hw_sg_buf_pool = wrapper;
		vir += sizeof(hw_buf_wrapper);
	}
	rsrc->hw_sg_buf_count = hw_sg_buf_count;
	item_size = sizeof(event_record) * event_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	rsrc->event_pool = NULL;
	for (i = 0; i < event_count; i++) {
		event = (event_record *)vir;
		event->queue_pointer.next = (List_Head *)rsrc->event_pool;
		rsrc->event_pool = event;
		vir += sizeof(event_record);
	}
	rsrc->event_count = (MV_U8)event_count;

	item_size = sizeof(struct device_spin_up) * CORE_MAX_DEVICE_SUPPORTED;
		vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
		if (vir == NULL) return MV_FALSE;
		rsrc->spin_up_device_pool = NULL;
		for (i = 0; i < CORE_MAX_DEVICE_SUPPORTED; i++) {
			device_su = (struct device_spin_up *)vir;
			device_su->list.next = (List_Head *)rsrc->spin_up_device_pool;
			rsrc->spin_up_device_pool = device_su;
			vir += sizeof(struct device_spin_up);
		}
		rsrc->device_count = (MV_U16)CORE_MAX_DEVICE_SUPPORTED;

	ret = lib_rsrc_allocate_device(rsrc, max_io);
	if (ret==MV_FALSE) return MV_FALSE;

	return MV_TRUE;
}

MV_BOOLEAN ses_state_machine(MV_PVOID enc_p);

extern MV_VOID core_init_handlers(core_extension *core);
MV_BOOLEAN core_init_cached_memory(core_extension *core,
	lib_resource_mgr *rsrc, MV_U16 max_io)
{
	pl_root *root;

	MV_U16 i;
	MV_BOOLEAN ret;

	MV_U16 intl_req_count, hw_sg_buf_count, scratch_buf_count, context_count;
	MV_U16 req_sg_entry_count, hw_sg_entry_count, event_count;

	core_get_supported_pal_counts(max_io, &intl_req_count, &req_sg_entry_count,
		&hw_sg_entry_count, &hw_sg_buf_count, &scratch_buf_count, &context_count,
		&event_count);

	core->max_io = max_io;
	if ( max_io==1 )
		core->is_dump = MV_TRUE;
	else
		core->is_dump = MV_FALSE;
	core->hw_sg_entry_count = hw_sg_entry_count;
	core->init_queue_count = 0;
	core->state = CORE_STATE_IDLE;

	MV_COUNTED_LIST_HEAD_INIT(&core->init_queue);
	MV_COUNTED_LIST_HEAD_INIT(&core->error_queue);
	MV_COUNTED_LIST_HEAD_INIT(&core->waiting_queue);
	MV_COUNTED_LIST_HEAD_INIT(&core->high_priority_queue);
	MV_LIST_HEAD_INIT(&core->complete_queue);
	MV_LIST_HEAD_INIT(&core->event_queue);
	MV_LIST_HEAD_INIT(&core->device_spin_up_list);
	core->device_spin_up_timer=NO_CURRENT_TIMER;
	core_init_handlers(core);

	/* setup PL variables */
	for(i = core->chip_info->start_host; i < (core->chip_info->start_host + core->chip_info->n_host); i++) {
		root = &core->roots[i];

		ret = prot_init_pl(root,
			max_io,
			core,
			(MV_LPVOID)((MV_PU8)core->mmio_base +
				(MV_IO_CHIP_REGISTER_BASE + (i * MV_IO_CHIP_REGISTER_RANGE))),
			&core->lib_dev,
			&core->lib_rsrc
			);
		if (MV_FALSE == ret) return ret;
	}

	/* resource lib */
	ret = lib_rsrc_allocate(rsrc, max_io);
	if (MV_FALSE == ret) return ret;

	return MV_TRUE;
}

MV_BOOLEAN core_init_dma_memory(core_extension * core,
	lib_resource_mgr *lib_rsrc, MV_U16 max_io)
{
	MV_PVOID mem_v=NULL;
	MV_PHYSICAL_ADDR mem_p;
	hw_buf_wrapper *wrapper = NULL;
	MV_U32 item_size, tmp_count, tmp_size, allocated_count;
	lib_resource_mgr *rsrc = lib_rsrc;

	MV_U16 i, j;

	MV_U16 slot_count, delv_q_size, cmpl_q_size, received_fis_count;
	MV_U16 intl_req_count, hw_sg_buf_count, scratch_buf_count, context_count;
	MV_U16 req_sg_entry_count, hw_sg_entry_count, event_count;

	core_get_supported_pl_counts(max_io, &slot_count, &delv_q_size, &cmpl_q_size, &received_fis_count);
	core_get_supported_pal_counts(max_io, &intl_req_count, &req_sg_entry_count,
		&hw_sg_entry_count, &hw_sg_buf_count, &scratch_buf_count, &context_count,
		&event_count);
	/*
	* HW binds the list with Command List Base Address register
	*  so no cache-tune permitted.
	*/
	item_size = sizeof(mv_command_header) * slot_count;
	for(i = core->chip_info->start_host; i < (core->chip_info->start_host + core->chip_info->n_host); i++) {
		mem_v = lib_rsrc_malloc_dma(rsrc, item_size, 64, &mem_p);
		if (mem_v == NULL) return MV_FALSE;
		core->roots[i].cmd_list = mem_v;
		core->roots[i].cmd_list_dma = mem_p;
	}

	/* assign delivery queue (64 byte align) */
	item_size = sizeof(MV_U32) * delv_q_size;
	for(i = core->chip_info->start_host; i < (core->chip_info->start_host + core->chip_info->n_host); i++) {
		mem_v = lib_rsrc_malloc_dma(rsrc, item_size, 64, &mem_p);
		if (mem_v == NULL) return MV_FALSE;
		core->roots[i].delv_q = mem_v;
		core->roots[i].delv_q_dma = mem_p;
	}

	/* assign completion queue (64 byte align) */
	item_size = sizeof(MV_U32) * cmpl_q_size;
	for(i = core->chip_info->start_host; i < (core->chip_info->start_host + core->chip_info->n_host); i++) {
		mem_v = lib_rsrc_malloc_dma(rsrc, item_size, 64, &mem_p);
		if (mem_v == NULL) return MV_FALSE;
		core->roots[i].cmpl_wp = mem_v;
		core->roots[i].cmpl_wp_dma = mem_p;
		core->roots[i].cmpl_q = (MV_PU32)((MV_PU8)mem_v + sizeof(MV_U32));
		core->roots[i].cmpl_q_dma = U64_ADD_U32(mem_p, sizeof(MV_U32));
	}

	/* assign dma memory for received FIS (256 byte align) */
	if (core->is_dump) {
		item_size = MIN_RX_FIS_POOL_SIZE;
	} else {
		item_size = MAX_RX_FIS_POOL_SIZE;
	}
	for(i = core->chip_info->start_host; i < (core->chip_info->start_host + core->chip_info->n_host); i++) {
		/* for hibernation, the FIS memory is shared */
		if (i==0 || !core->is_dump) {
		mem_v = lib_rsrc_malloc_dma(rsrc, item_size, 256, &mem_p);
		if (mem_v == NULL) return MV_FALSE;
		}
		core->roots[i].rx_fis = mem_v;
		core->roots[i].rx_fis_dma = mem_p;
	}
	
	sprintf(core->ct_name,"%s","mv_ct_pool_");
	core->ct_pool= ossw_pci_pool_create(core->ct_name, core, sizeof(mv_command_table), 64 ,0);
	if (core->ct_pool == NULL)
		return MV_FALSE;
	
	item_size = hw_sg_entry_count * CORE_HW_SG_ENTRY_SIZE;
	sprintf(core->sg_name,"%s","mv_sg_pool_");
	core->sg_pool = ossw_pci_pool_create(core->sg_name, core, item_size, 16, 0);
	if (core->sg_pool == NULL)
		return MV_FALSE;

	/* assign the scratch buffer resource (8 byte align) */
	item_size = SCRATCH_BUFFER_SIZE;
	allocated_count = 0;
	tmp_count = scratch_buf_count;
	wrapper = rsrc->scratch_buf_pool;
	do {
		tmp_size = item_size * tmp_count;
		mem_v = lib_rsrc_malloc_dma(rsrc, tmp_size, 8, &mem_p);
		if (mem_v != NULL) {
			for (j = 0; j < tmp_count; j++) {
				MV_ASSERT(wrapper != NULL);
				wrapper->vir = mem_v;
				wrapper->phy = mem_p;
				wrapper = wrapper->next;
				mem_v = (MV_PU8)mem_v + item_size;
				mem_p = U64_ADD_U32(mem_p, item_size);
				allocated_count++;
				if (allocated_count == scratch_buf_count) {
					break;
				}
			}
		} else {
			tmp_count >>= 1;
		}
	} while ((tmp_count != 0) && (allocated_count < scratch_buf_count));
	if (allocated_count < scratch_buf_count) return MV_FALSE;
	MV_ASSERT(wrapper == NULL);

	if(!core->is_dump) {
		mem_v = lib_rsrc_malloc_dma(rsrc, TRASH_BUCKET_SIZE, 8, &mem_p);
		if (mem_v != NULL) {
			core->trash_bucket_dma = mem_p;
		} else  {
			return MV_FALSE;
		}
	}

	return MV_TRUE;
}

PMV_Request get_intl_req_handler(lib_resource_mgr *rsrc);
void free_intl_req_handler(lib_resource_mgr *rsrc, PMV_Request req);
core_context *get_core_context_handler(lib_resource_mgr *rsrc);
void free_core_context_handler(lib_resource_mgr *rsrc, core_context *context);
hw_buf_wrapper *get_scratch_buf_handler(lib_resource_mgr *rsrc);
void free_scratch_buf_handler(lib_resource_mgr *rsrc, hw_buf_wrapper *wrapper);
hw_buf_wrapper *get_sg_buf_handler(lib_resource_mgr *rsrc);
void free_sg_buf_handler(lib_resource_mgr *rsrc, hw_buf_wrapper *wrapper);
struct device_spin_up *get_device_spin_up_handler(lib_resource_mgr *rsrc);
void free_device_spin_up_handler(lib_resource_mgr *rsrc, struct device_spin_up *device);
event_record *get_event_record_handler(lib_resource_mgr *rsrc);
void free_event_record_handler(lib_resource_mgr *rsrc, event_record *event);
domain_device *get_device_handler(lib_resource_mgr *rsrc);
void free_device_handler(lib_resource_mgr *rsrc, domain_device *device);
domain_expander *get_expander_handler(lib_resource_mgr *rsrc);
void free_expander_handler(lib_resource_mgr *rsrc, domain_expander *exp);
domain_pm *get_pm_handler(lib_resource_mgr *rsrc);
void free_pm_handler(lib_resource_mgr *rsrc, domain_pm *pm);

domain_enclosure *get_enclosure_handler(lib_resource_mgr *rsrc);
void free_enclosure_handler(lib_resource_mgr *rsrc, domain_enclosure *enc);

void lib_rsrc_init(lib_resource_mgr *rsrc, MV_PVOID cached_vir, MV_U32 cached_size,
	MV_PVOID dma_vir, MV_PHYSICAL_ADDR dma_phy, MV_U32 dma_size,
	resource_func_tbl *func, lib_device_mgr *lib_dev)
{
	/* global cached memory buffer */
	MV_ASSERT((((MV_PTR_INTEGER)cached_vir) & (SIZE_OF_POINTER-1)) == 0);
	rsrc->global_cached_vir = cached_vir;
	rsrc->free_cached_vir = cached_vir;
	rsrc->total_cached_size = cached_size;
	rsrc->free_cached_size = cached_size;

	/* global dma memory virtual and physical address */
	MV_ASSERT((((MV_PTR_INTEGER)dma_vir) & (SIZE_OF_POINTER-1)) == 0);
	rsrc->global_dma_vir = dma_vir;
	rsrc->free_dma_vir = dma_vir;
	rsrc->global_dma_phy = dma_phy;
	rsrc->free_dma_phy = dma_phy;
	rsrc->total_dma_size = dma_size;
	rsrc->free_dma_size = dma_size;

	rsrc->func_tbl = *func;
	rsrc->lib_dev = lib_dev;
}

void * lib_rsrc_malloc_cached(lib_resource_mgr *rsrc, MV_U32 size)
{
	void * vir;

	size = ROUNDING(size, SIZE_OF_POINTER);
	if (rsrc->free_cached_size < size) {
		MV_ASSERT(rsrc->func_tbl.malloc != NULL);
		return rsrc->func_tbl.malloc(rsrc->func_tbl.extension,size, RESOURCE_CACHED_MEMORY, SIZE_OF_POINTER, NULL);
	}

	vir = rsrc->free_cached_vir;
	rsrc->free_cached_vir = (MV_PU8)vir + size;
	rsrc->free_cached_size -= size;

	MV_DASSERT((((MV_PTR_INTEGER)vir) & (SIZE_OF_POINTER-1)) == 0);
	return vir;
}

void * lib_rsrc_malloc_dma(lib_resource_mgr *rsrc, MV_U32 size,
	MV_U16 alignment, MV_PHYSICAL_ADDR *phy)
{
	MV_U32 offset = 0;
	void * vir;

	size = ROUNDING(size, SIZE_OF_POINTER);

	offset = (MV_U32)
		(ROUNDING(rsrc->free_dma_phy.value, alignment) - rsrc->free_dma_phy.value);
	if ((rsrc->free_dma_size + offset) < size) {
		MV_DASSERT(rsrc->func_tbl.malloc != NULL);
		return rsrc->func_tbl.malloc(rsrc->func_tbl.extension, size, RESOURCE_UNCACHED_MEMORY, alignment, phy);
	}

	vir = (MV_PU8)rsrc->free_dma_vir + offset;
	*phy = U64_ADD_U32(rsrc->free_dma_phy, offset);

	rsrc->free_dma_vir = (MV_PU8)vir + size;
	rsrc->free_dma_phy = U64_ADD_U32(*phy, size);
	rsrc->free_dma_size -= offset + size;

	return vir;
}

MV_BOOLEAN core_init_obj(lib_resource_mgr *rsrc, void *obj, MV_U8 obj_type)
{
	switch (obj_type) {
	case CORE_RESOURCE_TYPE_INTERNAL_REQ:
		{
			MV_Request *req = (MV_Request *)obj;
			req->Req_Type = REQ_TYPE_CORE;
			break;
		}
	case CORE_RESOURCE_TYPE_CONTEXT:
		{
			core_context *ctx = (core_context *)obj;
			ctx->req_type = CORE_REQ_TYPE_NONE;
			ctx->error_info = 0;
			ctx->type = CORE_CONTEXT_TYPE_NONE;
			ctx->req_state = 0;
			ctx->req_flag = 0;
			ctx->handler = NULL;

			MV_DASSERT(ctx->buf_wrapper == NULL);
			MV_DASSERT(ctx->sg_wrapper == NULL);

			break;
		}
	case CORE_RESOURCE_TYPE_EVENT_RECORD:
		{
			event_record *event = (event_record *)obj;
			event->handle_time = 0x0;
			break;
		}
	case CORE_RESOURCE_TYPE_DOMAIN_DEVICE:
		{
			domain_device *dev = (domain_device *)obj;
                     MV_ASSERT(dev->base.exp_queue_pointer.prev == NULL);
                     MV_ASSERT(dev->base.exp_queue_pointer.next == NULL);
			MV_ZeroMemory(dev, sizeof(domain_device));
			dev->base.id = add_device_map(rsrc->lib_dev, &dev->base);
			if (dev->base.id == MAX_ID) {
				/* ran out of IDs */
				return MV_FALSE;
			}
			break;
		}
	case CORE_RESOURCE_TYPE_DOMAIN_EXPANDER:
		{
			domain_expander *exp = (domain_expander *)obj;
			MV_ZeroMemory(exp, sizeof(domain_expander));
			exp->base.id = add_device_map(rsrc->lib_dev, &exp->base);
			if (exp->base.id == MAX_ID) {
				/* ran out of IDs */
				return MV_FALSE;
			}
			break;
		}
	case CORE_RESOURCE_TYPE_DOMAIN_PM:
		{
			domain_pm *pm = (domain_pm *)obj;
			MV_ZeroMemory(pm, sizeof(domain_pm));
			pm->base.id = add_device_map(rsrc->lib_dev, &pm->base);
			if (pm->base.id == MAX_ID) {
				/* ran out of IDs */
				return MV_FALSE;
			}
			break;
		}
	case CORE_RESOURCE_TYPE_DOMAIN_ENCLOSURE:
		{
			domain_enclosure *enc = (domain_enclosure *)obj;
			MV_ZeroMemory(enc, sizeof(domain_enclosure));
			enc->base.id = add_device_map(rsrc->lib_dev, &enc->base);
			if (enc->base.id == MAX_ID) {
				/* ran out of IDs */
				return MV_FALSE;
			}
			break;
		}
	case CORE_RESOURCE_TYPE_SPIN_UP_DEVICE:
		{
			struct device_spin_up *su=(struct device_spin_up *)obj;
			MV_ZeroMemory(su,sizeof(struct device_spin_up));
		}
		
		break;
	default:
		break;
	}

	return MV_TRUE;
}

void * core_malloc(MV_PVOID root_p, lib_resource_mgr *rsrc, MV_U8 obj_type)
{
	void * obj = NULL;
	pl_root *root = (pl_root *)root_p;
	
	if (root_p) {		
		core_extension *core = (core_extension *)root->core;
		if (core == NULL) {
			MV_ASSERT(MV_FALSE);
			return NULL;
		}		
	}
		switch (obj_type) {
		case CORE_RESOURCE_TYPE_INTERNAL_REQ:
			obj = get_intl_req_handler(rsrc);
			break;
		case CORE_RESOURCE_TYPE_CONTEXT:
			obj = get_core_context_handler(rsrc);
			break;
		case CORE_RESOURCE_TYPE_SCRATCH_BUFFER:
			obj = get_scratch_buf_handler(rsrc);
			break;
		case CORE_RESOURCE_TYPE_SG_BUFFER:
			obj = get_sg_buf_handler(rsrc);
			break;
		case CORE_RESOURCE_TYPE_EVENT_RECORD:
			obj = get_event_record_handler(rsrc);
			break;
		case CORE_RESOURCE_TYPE_DOMAIN_DEVICE:
			obj = get_device_handler(rsrc);
			break;
		case CORE_RESOURCE_TYPE_DOMAIN_EXPANDER:
			obj = get_expander_handler(rsrc);
			break;
		case CORE_RESOURCE_TYPE_DOMAIN_PM:
			obj = get_pm_handler(rsrc);
			break;
		case CORE_RESOURCE_TYPE_DOMAIN_ENCLOSURE:
			obj = get_enclosure_handler(rsrc);
			break;
		case CORE_RESOURCE_TYPE_SPIN_UP_DEVICE:
			obj =  get_device_spin_up_handler(rsrc);
			break;
		default:
			MV_ASSERT(MV_FALSE);
			break;
		}

	if (obj != NULL) {
		if (core_init_obj(rsrc, obj, obj_type) == MV_FALSE) {
			core_free(root_p, rsrc, obj, obj_type);
			obj = NULL;
		}
	}

	return obj;
}

static void core_clean_obj(lib_resource_mgr *rsrc, void * obj, MV_U8 obj_type)
{
	switch (obj_type) {
	case CORE_RESOURCE_TYPE_DOMAIN_DEVICE:
		{
			domain_device *dev = (domain_device *)obj;
			if (dev->base.id != MAX_ID) {
				remove_device_map(rsrc->lib_dev, dev->base.id);
			}
			MV_ASSERT(dev->base.err_ctx.timer_id == NO_CURRENT_TIMER);
			break;
		}
	case CORE_RESOURCE_TYPE_DOMAIN_EXPANDER:
		{
			domain_expander *exp = (domain_expander *)obj;
			if (exp->base.id != MAX_ID) {
				remove_device_map(rsrc->lib_dev, exp->base.id);
			}
			break;
		}
	case CORE_RESOURCE_TYPE_DOMAIN_PM:
		{
			domain_pm *pm = (domain_pm *)obj;
			if (pm->base.id != MAX_ID) {
				remove_device_map(rsrc->lib_dev, pm->base.id);
			}
			break;
		}
	case CORE_RESOURCE_TYPE_DOMAIN_ENCLOSURE:
		{
			domain_enclosure *enc = (domain_enclosure *)obj;
			if (enc->base.id != MAX_ID) {
				remove_device_map(rsrc->lib_dev, enc->base.id);
			}
			break;
		}
	default:
		break;
	}
}

void core_free(MV_PVOID root_p, lib_resource_mgr *rsrc, void * obj, MV_U8 obj_type)
{	
	pl_root *root = (pl_root *)root_p;
	
	if (root_p) {		
		core_extension *core = (core_extension *)root->core;
		if (core == NULL) {
			MV_ASSERT(MV_FALSE);
			return;
		}		
	}
	core_clean_obj(rsrc, obj, obj_type);

	if (rsrc->func_tbl.free == NULL) {
		switch (obj_type) {
		case CORE_RESOURCE_TYPE_INTERNAL_REQ:
			free_intl_req_handler(rsrc, (PMV_Request)obj);
			break;
		case CORE_RESOURCE_TYPE_CONTEXT:
			free_core_context_handler(rsrc, (core_context *)obj);
			break;
		case CORE_RESOURCE_TYPE_SCRATCH_BUFFER:
                        free_scratch_buf_handler(rsrc, (hw_buf_wrapper *)obj);
			break;
		case CORE_RESOURCE_TYPE_SG_BUFFER:
                        free_sg_buf_handler(rsrc, (hw_buf_wrapper *)obj);
			break;
		case CORE_RESOURCE_TYPE_EVENT_RECORD:
                        free_event_record_handler(rsrc, (event_record *)obj);
			break;
		case CORE_RESOURCE_TYPE_DOMAIN_DEVICE:
			free_device_handler(rsrc, (domain_device *)obj);
			break;
		case CORE_RESOURCE_TYPE_DOMAIN_EXPANDER:
			free_expander_handler(rsrc, (domain_expander *)obj);
			break;
		case CORE_RESOURCE_TYPE_DOMAIN_PM:
			free_pm_handler(rsrc, (domain_pm *)obj);
			break;
		case CORE_RESOURCE_TYPE_DOMAIN_ENCLOSURE:
			free_enclosure_handler(rsrc, (domain_enclosure *)obj);
			break;
		case CORE_RESOURCE_TYPE_SPIN_UP_DEVICE:
			free_device_spin_up_handler(rsrc,(struct device_spin_up *)obj);
			break;
		default:
			MV_DASSERT(MV_FALSE);
			break;
		}
	}
}

PMV_Request get_intl_req_handler(lib_resource_mgr *rsrc)
{
	PMV_Request req = NULL;
	if (rsrc->intl_req_count > 0) {
		MV_DASSERT(rsrc->intl_req_pool != NULL);
		req = rsrc->intl_req_pool;
		rsrc->intl_req_pool = (PMV_Request)req->Queue_Pointer.next;
		rsrc->intl_req_count--;
	} else {
		MV_DASSERT(rsrc->intl_req_pool == NULL);
	}

	return req;
}

void free_intl_req_handler(lib_resource_mgr *rsrc, PMV_Request req)
{
	rsrc->intl_req_count++;
	req->Queue_Pointer.next = (List_Head *)rsrc->intl_req_pool;
	rsrc->intl_req_pool = req;
}

core_context *get_core_context_handler(lib_resource_mgr *rsrc)
{
	core_context *context = NULL;

	if (rsrc->context_count > 0) {
		MV_DASSERT(rsrc->context_pool != NULL);
		context = rsrc->context_pool;
		rsrc->context_pool = context->next;
		rsrc->context_count--;
	} else {
		MV_DASSERT(rsrc->context_pool == NULL);
	}

	return context;
}

void free_core_context_handler(lib_resource_mgr *rsrc, core_context *context)
{
	MV_DASSERT(context != NULL);
	context->next = rsrc->context_pool;
	rsrc->context_pool = context;
	rsrc->context_count++;
}

hw_buf_wrapper *get_scratch_buf_handler(lib_resource_mgr *rsrc)
{
	hw_buf_wrapper *wrapper = NULL;
	if (rsrc->scratch_buf_count > 0) {
		MV_DASSERT(rsrc->scratch_buf_pool != NULL);
		wrapper = rsrc->scratch_buf_pool;
		rsrc->scratch_buf_pool = wrapper->next;
		rsrc->scratch_buf_count--;

		wrapper->next = NULL;
	} else {
		MV_DASSERT(rsrc->scratch_buf_pool == NULL);
	}

	return wrapper;
}

void free_scratch_buf_handler(lib_resource_mgr *rsrc, hw_buf_wrapper *wrapper)
{
	MV_DASSERT(wrapper != NULL);
	wrapper->next = rsrc->scratch_buf_pool;
	rsrc->scratch_buf_pool = wrapper;
	rsrc->scratch_buf_count++;
}

hw_buf_wrapper *get_sg_buf_handler(lib_resource_mgr *rsrc)
{
	hw_buf_wrapper *wrapper = NULL;
	if (rsrc->hw_sg_buf_count > 0) {
		MV_DASSERT(rsrc->hw_sg_buf_pool != NULL);
		wrapper = rsrc->hw_sg_buf_pool;
		rsrc->hw_sg_buf_pool = wrapper->next;
		rsrc->hw_sg_buf_count--;

		wrapper->next = NULL;
	} else {
		MV_DASSERT(rsrc->hw_sg_buf_pool == NULL);
	}

	return wrapper;
}

void free_sg_buf_handler(lib_resource_mgr *rsrc, hw_buf_wrapper *wrapper)
{
	MV_DASSERT(wrapper != NULL);
	wrapper->next = rsrc->hw_sg_buf_pool;
	rsrc->hw_sg_buf_pool = wrapper;
	rsrc->hw_sg_buf_count++;
}

struct device_spin_up *get_device_spin_up_handler(lib_resource_mgr *rsrc)
{
	struct device_spin_up *device = NULL;

	if (rsrc->device_count > 0) {
		MV_DASSERT(rsrc->spin_up_device_pool != NULL);
		device = rsrc->spin_up_device_pool;
		rsrc->spin_up_device_pool = (struct device_spin_up *)device->list.next;
		rsrc->device_count--;
	} else {
		MV_DASSERT(rsrc->spin_up_device_pool == NULL);
	}
	return device;
}
void free_device_spin_up_handler(lib_resource_mgr *rsrc, struct device_spin_up *device)
{
	MV_DASSERT(device != NULL);
	device->list.next = (List_Head *)rsrc->spin_up_device_pool;
	rsrc->spin_up_device_pool = device;
	rsrc->device_count++;
}

event_record *get_event_record_handler(lib_resource_mgr *rsrc)
{
	event_record *event = NULL;

	if (rsrc->event_count > 0) {
		MV_DASSERT(rsrc->event_pool != NULL);
		event = rsrc->event_pool;
		rsrc->event_pool = (event_record *)event->queue_pointer.next;
		rsrc->event_count--;
	} else {
		MV_DASSERT(rsrc->event_pool == NULL);
	}

	return event;
}

void free_event_record_handler(lib_resource_mgr *rsrc, event_record *event)
{
	MV_DASSERT(event != NULL);
	event->queue_pointer.next = (List_Head *)rsrc->event_pool;
	rsrc->event_pool = event;
	rsrc->event_count++;
}

domain_device *get_device_handler(lib_resource_mgr *rsrc)
{
	domain_device *dev = NULL;

	if (rsrc->hd_count > 0) {
		dev = rsrc->hds;
		rsrc->hds = (domain_device *)dev->base.queue_pointer.next;
		rsrc->hd_count--;
	} else {
		MV_DASSERT(rsrc->hds == NULL);
	}

	return dev;
}

void free_device_handler(lib_resource_mgr *rsrc, domain_device *device)
{
	device->base.queue_pointer.next = (List_Head *)rsrc->hds;
	rsrc->hds = device;
	rsrc->hd_count++;
}

domain_expander *get_expander_handler(lib_resource_mgr *rsrc)
{
	domain_expander *exp = NULL;

	if (rsrc->exp_count > 0) {
		exp = rsrc->expanders;
		rsrc->expanders = (domain_expander *)exp->base.queue_pointer.next;
		rsrc->exp_count--;
	} else {
		MV_DASSERT(rsrc->expanders == NULL);
	}

	return exp;
}

void free_expander_handler(lib_resource_mgr *rsrc, domain_expander *exp)
{
	exp->base.queue_pointer.next = (List_Head *)rsrc->expanders;
	rsrc->expanders = exp;
	rsrc->exp_count++;
}

domain_pm *get_pm_handler(lib_resource_mgr *rsrc)
{
	domain_pm *pm = NULL;

	if (rsrc->pm_count > 0) {
		pm = rsrc->pms;
		rsrc->pms = (domain_pm *)pm->base.queue_pointer.next;
		rsrc->pm_count--;
	} else {
		MV_DASSERT(rsrc->pms == NULL);
	}
	return pm;
}

void free_pm_handler(lib_resource_mgr *rsrc, domain_pm *pm)
{
	pm->base.queue_pointer.next = (List_Head *)rsrc->pms;
	rsrc->pms = pm;
	rsrc->pm_count++;
}
domain_enclosure *get_enclosure_handler(lib_resource_mgr *rsrc)
{
	domain_enclosure *enc = NULL;

	if (rsrc->enc_count > 0) {
		enc = rsrc->enclosures;
		rsrc->enclosures = (domain_enclosure *)enc->base.queue_pointer.next;
		rsrc->enc_count--;
	} else {
		MV_DASSERT(rsrc->enclosures == NULL);
	}
	return enc;
}
void free_enclosure_handler(lib_resource_mgr *rsrc, domain_enclosure *enc)
{
	enc->base.queue_pointer.next = (List_Head *)rsrc->enclosures;
	rsrc->enclosures= enc;
	rsrc->enc_count++;
}
