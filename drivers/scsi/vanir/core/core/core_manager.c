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
#include "com_dbg.h"
#include "com_util.h"
#include "com_u64.h"

#include "core_hal.h"
#include "core_manager.h"
#include "core_init.h"
#include "core_device.h"
#include "core_sata.h"
#include "core_sas.h"
#include "core_resource.h"
#include "core_alarm.h"
#include "core_rmw_flash.h"
#include "core_error.h"
#include "core_util.h"
#include "com_api.h"
#include "core_console.h"

#include "hba_exp.h"
#include "core_exp.h"

core_extension *gCore = NULL;
MV_VOID core_handle_waiting_queue(core_extension *core);

MV_VOID core_push_queues(MV_PVOID core_p);

extern MV_VOID core_handle_error_queue(core_extension *core);
extern void core_handle_event_queue(core_extension *core);
extern MV_VOID core_clean_waiting_queue(core_extension *core);
extern MV_VOID core_clean_error_queue(core_extension *core);
extern void core_clean_event_queue(core_extension *core);

MV_U32 Core_ModuleGetResourceQuota(enum Resource_Type type, MV_U16 max_io)
{
	MV_U32 size = 0;

	if (type == RESOURCE_CACHED_MEMORY) {
		size += core_get_cached_memory_quota(max_io);
		size += core_rmw_flash_get_cached_memory_quota(max_io);
		size += core_spin_up_get_cached_memory_quota(max_io);
		size += 1024; /* memory align. */
	} else if (type == RESOURCE_UNCACHED_MEMORY) {
		size += core_get_dma_memory_quota(max_io);
		size += core_rmw_flash_get_dma_memory_quota(max_io);
	}

	return size;
}

MV_VOID init_virtual_device(pl_root *root, lib_resource_mgr *rsrc, lib_device_mgr *lib_dev)
{
	domain_device *dev = get_device_obj(root, rsrc);
	command_handler *handler;
	MV_U16 old_id = dev->base.id;
	MV_U16 new_id = VIRTUAL_DEVICE_ID;
	MV_BOOLEAN ret;

	handler = core_get_handler(root, HANDLER_API);
	
	if (old_id != new_id) {
		ret = change_device_map(lib_dev, &dev->base, old_id, new_id);
		MV_ASSERT(ret == MV_TRUE);
                dev->base.id = new_id;
	}

	set_up_new_device(root, NULL, dev, handler);
	set_up_new_base(root, NULL, &dev->base,
		handler,
		BASE_TYPE_DOMAIN_VIRTUAL,
		sizeof(domain_device));

	dev->base.TargetID = dev->base.id;
	root->lib_dev->target_id_map[dev->base.TargetID] = dev->base.id;
}

resource_func_tbl rsrc_func = {os_malloc_mem, NULL, NULL};

static const struct mvs_chip_info mvs_chips[] = {
	{ DEVICE_ID_9440, 1, 0, 4, 0x800, 64, 64,  11}, 
	{ DEVICE_ID_9445, 1, 0, 4, 0x800, 64, 64,  11}, 
	{ DEVICE_ID_9480, 2, 0, 4, 0x800, 64, 64,  11}, 
	{ DEVICE_ID_9580, 2, 0, 4, 0x800, 64, 64,  11},
	{ DEVICE_ID_9588, 2, 0, 4, 0x800, 64, 64,  11},
	{ DEVICE_ID_9485, 2, 0, 4, 0x800, 64, 64,  11}, 
	{ DEVICE_ID_9480, 1, 1, 4, 0x800, 64, 64,  11},
	{ DEVICE_ID_948F, 2, 0, 4, 0x800, 64, 64,  11},
	{ 0 }
};
#define MVS_CHIP_NUM  sizeof(mvs_chips)/(sizeof(mvs_chips[0]))

MV_U32 core_get_chip_info(void *core_p)
{
	int i = 0;
	core_extension * core = (core_extension *)core_p;
	HBA_Info_Page hba_info_param;
	MV_U8 prod_id[8] = {'V', 'A', '6', '4', '0', '0', 'm', 0x00};

	for ( i = 0; i < MVS_CHIP_NUM; i++) {
		if (core->device_id == mvs_chips[i].chip_id) {
			core->chip_info = &mvs_chips[i]; 
			if (IS_VANIR(core)) {
				if(mv_nvram_init_param(
					HBA_GetModuleExtension(core, MODULE_HBA),
					&hba_info_param)
					) {
					if (memcmp(hba_info_param.Product_ID, prod_id, 8) == 0) {
						if (core->chip_info->n_host != 1) {
							continue;
						}
					}
					if(hba_info_param.DSpinUpGroup==0xff ||
						hba_info_param.DSpinUpTimer == 0x00 ||
						hba_info_param.DSpinUpTimer == 0xff){
						
						hba_info_param.DSpinUpGroup=0;
						}
					core->spin_up_group = hba_info_param.DSpinUpGroup;
					if (core->spin_up_group)
						core->spin_up_time = hba_info_param.DSpinUpTimer;
				}
				CORE_DPRINT(("chip id %X.\n",core->device_id));
			}
			return 0;
		}
	}

	CORE_DPRINT(("Unknow chip id %X.\n",core->device_id));
	return -1;
}


MV_VOID Core_ModuleInitialize(MV_PVOID module, MV_U32 size, MV_U16 max_io)
{
	core_extension *core = NULL;
	lib_resource_mgr *rsrc = NULL;
	lib_device_mgr *lib_dev = NULL;
	Assigned_Uncached_Memory dma;
	MV_PU8 global_cached_vir = NULL;
	MV_U32 global_cached_size;
	MV_BOOLEAN ret;
	MV_U32 uncache_size;
	void * mod_desc;
	Controller_Infor controller;
	core = (core_extension*)module;

	/* zero cached memory */
	mod_desc = core->desc;
	MV_ZeroMemory(core, size);
	core->desc = mod_desc;
	core->bh_enabled = 1;
	gCore = core;
	MV_ASSERT(((MV_PTR_INTEGER)core & (SIZE_OF_POINTER - 1)) == 0);
	rsrc = &core->lib_rsrc;
	lib_dev = &core->lib_dev;
	global_cached_vir = (MV_PU8)core + sizeof(core_extension);
	global_cached_size = size - sizeof(core_extension);
	/* set up controller information */
	HBA_GetControllerInfor(core, (PController_Infor)&controller);

	core->vendor_id = controller.Vendor_Id;
	core->device_id = controller.Device_Id;
	core->revision_id = controller.Revision_Id;
	core->mmio_base = controller.Base_Address[MV_PCI_BAR];
	core->io_base   = controller.Base_Address[MV_PCI_BAR_IO];
	core->run_as_none_raid = controller.run_as_none_raid;

	if (core_get_chip_info(core)) {
		return;
	}

	/* get dma memory */
	MV_ZeroMemory(&dma, sizeof(Assigned_Uncached_Memory));
	rsrc_func.extension = (void *)core;

	lib_rsrc_init(rsrc, global_cached_vir, global_cached_size,
		dma.Virtual_Address, dma.Physical_Address, dma.Byte_Size,
		&rsrc_func, lib_dev);

	ret = core_init_cached_memory(core, rsrc, max_io);
	if (MV_FALSE == ret)
		goto err_alloc_core_mem;

	ret = core_init_dma_memory(core, rsrc, max_io);
	if (MV_FALSE == ret)
		goto err_alloc_core_mem;

	init_target_id_map(core->lib_dev.target_id_map, (sizeof(MV_U16)*MV_MAX_TARGET_NUMBER));
	init_virtual_device(&core->roots[core->chip_info->start_host], rsrc, lib_dev);

	ret = core_init_rmw_flash_memory(core, rsrc, max_io);
	if (MV_FALSE == ret)
		goto err_alloc_core_mem;

	return;

err_alloc_core_mem:
	alloc_uncached_failed(HBA_GetModuleExtension( core, MODULE_HBA ));
	return;
}

void core_io_chip_init_done(core_extension *core, MV_BOOLEAN single)
{
	sgpio_initialize(core);
	core_alarm_init(core);
}

void core_handle_init_queue(core_extension *core, MV_BOOLEAN single)
{
	struct _domain_base *base = NULL;
	MV_BOOLEAN ret;

	MV_DASSERT(Counted_List_GetCount(&core->init_queue, MV_TRUE) == Counted_List_GetCount(&core->init_queue, MV_FALSE));
	if (single) {
		if (Counted_List_GetCount(&core->init_queue, MV_FALSE) < core->init_queue_count) {
			MV_DASSERT(Counted_List_GetCount(&core->init_queue, MV_FALSE) == core->init_queue_count-1);
			return;
		}
	}

	if ((core->state == CORE_STATE_IDLE) && !(core->is_dump)) {
		CORE_DPRINT(("pushing queue when core state is idle!\n"));
			return;
	}

	while (!Counted_List_Empty(&core->init_queue)) {
		base = Counted_List_GetFirstEntry(&core->init_queue,
			struct _domain_base, init_queue_pointer);

		ret = base->handler->init_handler(base);

		if (ret == MV_FALSE) {
			CORE_DPRINT(("init %p fail.\n", base));
			Counted_List_Add(&base->init_queue_pointer, &core->init_queue);
			return;
		}

		if (single) {
		        break;
		}
	}

	if (core->state == CORE_STATE_STARTING) {
		if (core->init_queue_count == 0) {
			if (core_check_outstanding_req(core) != 0)
				return;

			MV_ASSERT(Counted_List_Empty(&core->init_queue));
			core->state = CORE_STATE_STARTED;

			core_io_chip_init_done(core, single);

			CORE_DPRINT((" finished init, notify upper layer. \n" ));
			core_start_cmpl_notify(core);
			return;
		}
	}
}

MV_VOID core_clean_init_queue_entry(MV_PVOID root_p, domain_base *base)
{
	pl_root *root = (pl_root *)root_p;
	core_extension *core = (core_extension *)root->core;
	domain_base *tmp_base, *found_base = NULL;

	LIST_FOR_EACH_ENTRY_TYPE(tmp_base, &core->init_queue, domain_base,
		init_queue_pointer) {
		if (tmp_base == base) {
			found_base = base;
			break;
		}
	}

	if (found_base) {
		core_init_entry_done(root, base->port, NULL);
		Counted_List_Del(&found_base->init_queue_pointer, &core->init_queue);
	}
}

MV_VOID Core_ModuleStart(MV_PVOID This)
{
	core_extension *core = (core_extension *)This;
	unsigned long flags;
	MV_U8 i;
	Controller_Infor controller;

	HBA_GetControllerInfor(core, (PController_Infor)&controller);

	if(core_reset_controller(core) == MV_FALSE){
		CORE_DPRINT(("Reset hba failed.\n"));
		return;
	}

	controller_init(core);

	OSSW_SPIN_LOCK_IRQSAVE(core, flags);

	for (i=core->chip_info->start_host; i<(core->chip_info->start_host + core->chip_info->n_host); i++){
		core->roots[i].base_phy_num = i * core->chip_info->n_phy;
		io_chip_init(&core->roots[i]);
	}
	core->state = CORE_STATE_STARTING;
	core_push_queues(core);

	OSSW_SPIN_UNLOCK_IRQRESTORE(core, flags);
}

extern MV_VOID core_sgpio_led_off_timeout(domain_base * base, MV_PVOID tmp);
extern MV_U8 core_sgpio_set_led( MV_PVOID extension, MV_U16 device_id, MV_U8 light_type,
        MV_U8 light_behavior, MV_U8 flag);
MV_QUEUE_COMMAND_RESULT core_send_request(core_extension *core, MV_Request *req)
{
	core_context *ctx;
	pl_root *root;
	MV_QUEUE_COMMAND_RESULT result;
	struct _domain_base *base;

	if (req->Context[MODULE_CORE] == NULL)
		req->Context[MODULE_CORE] = get_core_context(&core->lib_rsrc);
	ctx = req->Context[MODULE_CORE];
	if (ctx == NULL)
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

	base = (struct _domain_base *)get_device_by_id(&core->lib_dev, req->Device_Id);
	if (base == NULL) {
		req->Scsi_Status = REQ_STATUS_NO_DEVICE;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	root = base->root;
	ctx->handler = base->handler;

	result = prot_send_request(root, base, req);

	if (result == MV_QUEUE_COMMAND_RESULT_SENT)
		base->outstanding_req++;
	else if ((result == MV_QUEUE_COMMAND_RESULT_FULL)
		|| (result == MV_QUEUE_COMMAND_RESULT_NO_RESOURCE)) {
		if ((req->Context[MODULE_CORE] != NULL)
			&& (req->Req_Type != REQ_TYPE_CORE)) {
			free_core_context(root->lib_rsrc, req->Context[MODULE_CORE]);
			req->Context[MODULE_CORE] = NULL;
		}
	}

 	if ((core->device_id == DEVICE_ID_6480) || (core->device_id == DEVICE_ID_6485)
                        ||(core->device_id == 0x8180) || IS_VANIR(core)
			||(core->device_id == DEVICE_ID_948F)) {
		if((core->state == CORE_STATE_STARTED)
			&& (base->type == BASE_TYPE_DOMAIN_DEVICE)
			&& (IS_SGPIO(((domain_device *)base)))){
			if(((domain_device *)base)->active_led_off_timer == NO_CURRENT_TIMER){
				core_sgpio_set_led(core,base->id,0,0,LED_FLAG_ACT);
			}else{
				core_cancel_timer(core, ((domain_device *)base)->active_led_off_timer);
				((domain_device *)base)->active_led_off_timer = NO_CURRENT_TIMER;
			}
			((domain_device *)base)->active_led_off_timer = core_add_timer(core, 1, (MV_VOID (*) (MV_PVOID, MV_PVOID))core_sgpio_led_off_timeout, base, NULL);
		}
	}

	return result;
}
MV_VOID core_clean_waiting_queue(core_extension *core)
{
	MV_Request *req;
	
	while (!Counted_List_Empty(&core->waiting_queue)) {
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->waiting_queue, MV_Request, Queue_Pointer);
		req->Scsi_Status = REQ_STATUS_ERROR;
		core_queue_completed_req(core, req);
	}

	while (!Counted_List_Empty(&core->high_priority_queue)) {
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->high_priority_queue, MV_Request, Queue_Pointer);
		req->Scsi_Status = REQ_STATUS_ERROR;
		core_queue_completed_req(core, req);
	}

}
MV_VOID io_chip_handle_waiting_queue(core_extension *core, Counted_List_Head *queue)
{
	MV_Request *req;
	MV_QUEUE_COMMAND_RESULT result;
	List_Head tmp_queue;
        domain_base *base = NULL;

	MV_LIST_HEAD_INIT(&tmp_queue);

	while (!Counted_List_Empty(queue)) {
		req = (MV_Request *)Counted_List_GetFirstEntry(
			queue, MV_Request, Queue_Pointer);
		result = core_send_request(core, req);

		switch ( result )
		{
		case MV_QUEUE_COMMAND_RESULT_FINISHED:
			core_queue_completed_req(core, req);
			break;

		case MV_QUEUE_COMMAND_RESULT_FULL:
			List_Add(&req->Queue_Pointer, &tmp_queue);
			base = (domain_base *)get_device_by_id(&core->lib_dev, req->Device_Id);
			MV_DASSERT(base != NULL);
			base->blocked = MV_TRUE;
			break;

		case MV_QUEUE_COMMAND_RESULT_BLOCKED:
			List_Add(&req->Queue_Pointer, &tmp_queue);
			break;

		case MV_QUEUE_COMMAND_RESULT_NO_RESOURCE:
			Counted_List_Add(&req->Queue_Pointer, queue);
			goto ending;

		case MV_QUEUE_COMMAND_RESULT_SENT:
			mv_add_timer(core, req);
			break;

		case MV_QUEUE_COMMAND_RESULT_REPLACED:
			break;

		default:
			MV_ASSERT(MV_FALSE);
		}
	}

ending:
	while (!List_Empty(&tmp_queue)) {
		req = (MV_Request *)List_GetFirstEntry(
			&tmp_queue, MV_Request, Queue_Pointer);
			base = (domain_base *)get_device_by_id(&core->lib_dev, req->Device_Id);
			MV_DASSERT(base != NULL);
			base->blocked = MV_FALSE;
		Counted_List_Add(&req->Queue_Pointer, queue);
	}
}

MV_VOID core_handle_waiting_queue(core_extension *core)
{
	/* io chip queue: high priority and low priority */
	io_chip_handle_waiting_queue(core, &core->high_priority_queue);
	io_chip_handle_waiting_queue(core, &core->waiting_queue);
}

MV_VOID core_abort_all_running_requests(MV_PVOID core_p)
{
	MV_U8 j;
	MV_U32 i;
	pl_root *root;
	domain_port *port;
	domain_base *base;
	PMV_Request req = NULL;
	core_extension *core = (core_extension *)core_p;
	PHBA_Extension pHBA = (PHBA_Extension)HBA_GetModuleExtension(core, MODULE_HBA);

	core_clean_event_queue(core);
	core_clean_error_queue(core);
	core_clean_waiting_queue(core);
	
	for(j = core->chip_info->start_host; j < (core->chip_info->start_host + core->chip_info->n_host); j++) {
		root = &core->roots[j];
		for (i = 0; i < root->slot_count_support; i++) {
			req = root->running_req[i];
			if (req == NULL) continue;
			
			base = get_device_by_id(root->lib_dev, req->Device_Id);
			MV_ASSERT(base != NULL);
			prot_reset_slot(root, base, i, req);
			if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
				domain_device *device = (domain_device *)base;

				if (device->register_set != NO_REGISTER_SET) {
					MV_DASSERT(device->base.outstanding_req == 0);
					sata_free_register_set(root, device->register_set);
					device->register_set = NO_REGISTER_SET;
				}
			}
			req->Scsi_Status = REQ_STATUS_ERROR;
			core_queue_completed_req(core, req);
		}
	}
	
	core_complete_requests(core);
	
}

MV_VOID Core_ModuleSendRequest(MV_PVOID core_p, PMV_Request req)
{
	core_extension *core = (core_extension *)core_p;

	MV_DASSERT(req->Context[MODULE_CORE] == NULL);
	Counted_List_AddTail(&req->Queue_Pointer, &core->waiting_queue);
}

MV_VOID Core_ModuleMonitor(MV_PVOID core_p)
{
}

MV_VOID Core_ModuleNotification(MV_PVOID core_p, enum Module_Event event,
	struct mod_notif_param *param)
{
}

MV_VOID Core_ModuleReset(MV_PVOID core_p)
{
}

MV_VOID Core_ModuleShutdown(MV_PVOID core_p)
{
	core_extension *core = (core_extension *)core_p;
	MV_U8 i;
	pl_root *root;

	MV_ASSERT(Counted_List_Empty(&core->waiting_queue));
	MV_ASSERT(Counted_List_Empty(&core->high_priority_queue));
	MV_ASSERT(Counted_List_Empty(&core->error_queue));

	for (i = core->chip_info->start_host; i<(core->chip_info->start_host + core->chip_info->n_host); i++) {
		root = &core->roots[i];
		hal_disable_io_chip(root);
	}

	ossw_pci_pool_destroy(core->sg_pool);
	ossw_pci_pool_destroy(core->ct_pool);

}

MV_VOID core_set_chip_options(MV_PVOID ext)
{
	pl_root *root = (pl_root *)ext;
	core_extension *core = (core_extension *)root->core;
	root->max_register_set = core->chip_info->srs_sz;
	root->unassoc_fis_offset = core->chip_info->fis_offs;
	root->max_cmd_slot_width = core->chip_info->slot_width;

	if (root->max_register_set > 64) {
	        MV_DASSERT(MV_FALSE);
	} else if (root->max_register_set == 64) {
		root->sata_reg_set.value = 0;

	} else {
		root->sata_reg_set.value = (1ULL << (root->max_register_set + 1)) - 1;
		root->sata_reg_set.value = ~root->sata_reg_set.value;
	}
}



MV_BOOLEAN Core_InterruptCheckIRQ(MV_PVOID This)
{
	core_extension *core = (core_extension *)This;
	return core_clear_int(core);
}

void Core_InterruptHandleIRQ(MV_PVOID This)
{
	core_extension *core = (core_extension *)This;
	core_handle_int(core);
	core_complete_requests(core);
	core_push_queues(core);
}

MV_BOOLEAN Core_InterruptServiceRoutine(MV_PVOID This)
{
	core_extension *core = (core_extension *)This;
	pl_root *root;
	MV_BOOLEAN ret = MV_TRUE;

	if(!core->bh_enabled)
    		ret = core_clear_int(core);

	if (ret == MV_TRUE) {
		core_disable_ints(core);
		core_handle_int(core);
		core_complete_requests(core);
		core_push_queues(core);
		core_enable_ints(core);
	}

    return ret;
}

MV_VOID core_queue_completed_req(MV_PVOID ext, MV_Request *req)
{
	core_extension *core = (core_extension *)ext;
	List_AddTail(&req->Queue_Pointer, &core->complete_queue);
}

MV_VOID generic_post_callback(core_extension *core, MV_Request *req)
{
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];

	if (req->Req_Type != REQ_TYPE_CORE) {
		if (ctx != NULL) {
			MV_DASSERT(ctx->buf_wrapper == NULL);
			free_core_context(&core->lib_rsrc, ctx);
			req->Context[MODULE_CORE] = NULL;
		} else {
			MV_DASSERT(req->Scsi_Status == REQ_STATUS_NO_DEVICE);
		}
	} else {
		intl_req_release_resource(&core->lib_rsrc, req);
	}
}


MV_VOID core_return_finished_req(core_extension *core, MV_Request *req)
{
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];

	if (req->Req_Type != REQ_TYPE_CORE) {
		generic_post_callback(core, req);
	}
	
	if ((IS_ATA_PASSTHRU_CMD(req,SCSI_CMD_MARVELL_VENDOR_UNIQUE))  
		&& (!IS_VU_CMD(req,MARVELL_VU_CMD_ASYNC_NOTIFY))) {
		domain_device *device  = NULL;
		device = (domain_device*)get_device_by_id(&core->lib_dev,req->Device_Id ); 
		if(device){
			device->status &= ~DEVICE_STATUS_WAIT_ASYNC;
			device->status |= DEVICE_STATUS_FUNCTIONAL;
		}
	}

	if(req->Completion != NULL)
		req->Completion(req->Cmd_Initiator, req);

	if (req->Req_Type == REQ_TYPE_CORE) {
		generic_post_callback(core, req);
	}
}

MV_VOID io_chip_complete_requests(core_extension *core)
{
	MV_Request *req;

	while (!List_Empty(&core->complete_queue)) {
		req = (MV_Request *)List_GetFirstEntry(&core->complete_queue,
			MV_Request, Queue_Pointer);
		MV_DASSERT(req);

		if (req->Scsi_Status != REQ_STATUS_SUCCESS && \
			req->Scsi_Status != REQ_STATUS_NO_DEVICE &&\
			!(req->Scsi_Status == REQ_STATUS_HAS_SENSE && \
				(req->Cdb[0] == SCSI_CMD_ATA_PASSTHRU_16 || req->Cdb[0] == SCSI_CMD_ATA_PASSTHRU_12))\
			&& !((req->Scsi_Status == REQ_STATUS_ERROR_WITH_SENSE) && (req->Cdb[0] == APICDB0_PD)&&
				(req->Cdb[1] == APICDB1_PD_GETHD_INFO)&&(core->device_id == 0x8180))) {
			CORE_EH_PRINT(("req %p [0x%x] status 0x%x\n",
				req, req->Cdb[0], req->Scsi_Status));
		}
		core_return_finished_req(core, req);
	}
}

MV_VOID core_complete_requests(MV_PVOID core_p)
{
	core_extension *core = (core_extension *)core_p;

        io_chip_complete_requests(core);
}

MV_VOID core_push_queues(MV_PVOID core_p)
{
	core_extension *core = (core_extension *)core_p;
	MV_U32 queue_count1;

	if (!List_Empty(&core->event_queue))
		core_handle_event_queue(core);

	if (!Counted_List_Empty(&core->error_queue))
		core_handle_error_queue(core);

	if ((core->state == CORE_STATE_STARTED)) {
		core_handle_init_queue(core, MV_FALSE);
	} else if (core->is_dump) {
		/* for dump mode, do one by one to save memory */
		/* for hotplug, do init one by one because we need remap device id */
		core_handle_init_queue(core, MV_TRUE);
	} else {
		core_handle_init_queue(core, MV_FALSE);
	}

	do{
		core_handle_waiting_queue(core);
		queue_count1 = Counted_List_GetCount(&core->waiting_queue, MV_FALSE);
		core_complete_requests(core);
	}while(Counted_List_GetCount(&core->waiting_queue, MV_FALSE) != queue_count1);

}

MV_U8 get_min_negotiated_link_rate(domain_port *port)
{
	pl_root *root = port->base.root;
	MV_U8 i, min_rate, rate = 0;
	MV_U32 tmp;
	domain_phy *phy;

	min_rate = PHY_LINKRATE_6;

	for (i=0; i<root->phy_num; i++) {
		if (port->phy_map & MV_BIT(i)) {
			phy = &root->phy[i];
			tmp = READ_PORT_PHY_CONTROL(root, phy);
			rate = get_phy_link_rate(tmp);
			if (rate < min_rate)
				min_rate = rate;
		}
	}

	return min_rate;
}
MV_VOID update_base_id(pl_root *root,domain_port *port,domain_device *dev,MV_U8 skip){
	core_extension * core = (core_extension *)root->core;

	if(!skip){
		if(core->device_id==DEVICE_ID_6440 || core->device_id==DEVICE_ID_6340 ||
			core->device_id==DEVICE_ID_6480|| core->device_id==DEVICE_ID_6485){
			root->lib_rsrc->lib_dev->device_map[dev->base.id]=NULL;
			dev->base.id=port->phy->id;
			root->lib_rsrc->lib_dev->device_map[dev->base.id]=&dev->base;
		}else if(IS_VANIR(core)){
			root->lib_rsrc->lib_dev->device_map[dev->base.id]=NULL;
			dev->base.id=(MV_U16)(port->phy->id+root->base_phy_num);
			root->lib_rsrc->lib_dev->device_map[dev->base.id]=&dev->base;
		}
	}

	if (dev->base.id != VIRTUAL_DEVICE_ID)		
		dev->base.TargetID = add_target_map(root->lib_dev->target_id_map, dev->base.id, MV_MAX_TARGET_NUMBER);
}
void sas_set_up_new_device(pl_root *root, domain_port *port, domain_device *dev)
{
	set_up_new_device(root, port, dev,
		(command_handler *)
		core_get_handler(root, HANDLER_SSP));

	dev->connection = DC_SCSI | DC_SERIAL |DC_SGPIO;
	dev->dev_type = DT_DIRECT_ACCESS_BLOCK;
	dev->state = DEVICE_STATE_RESET_DONE;
	dev->base.queue_depth = CORE_SAS_DISK_QUEUE_DEPTH;
	dev->base.parent = &port->base;

	dev->sas_addr = port->att_dev_sas_addr;
	dev->negotiated_link_rate = port->link_rate;
	dev->capability |= DEVICE_CAPABILITY_RATE_1_5G;
	if (dev->negotiated_link_rate >= SAS_LINK_RATE_3_0_GBPS)
		dev->capability |= DEVICE_CAPABILITY_RATE_3G ;
	if (dev->negotiated_link_rate >= SAS_LINK_RATE_6_0_GBPS)
		dev->capability |= DEVICE_CAPABILITY_RATE_6G ;

	dev->sgpio_drive_number = (MV_U8)port->base.id;
	dev->active_led_off_timer = NO_CURRENT_TIMER;
}

void sas_init_port(pl_root *root, domain_port *port)
{
	domain_device *device = NULL;
	domain_expander *expander = NULL;
	core_extension *core = (core_extension *)root->core;
	MV_U8 i, phy_id = 0;
	MV_ULONG flags;				
	struct device_spin_up *tmp=NULL;

	MV_ASSERT(port->phy_map);

	if (port->att_dev_info & (PORT_DEV_SMP_TRGT | PORT_DEV_STP_TRGT)) {
		expander = get_expander_obj(root, root->lib_rsrc);
		if (expander == NULL) {
			CORE_DPRINT(("Ran out of expanders. Aborting.\n"));
			return;
		}
		set_up_new_expander(root, port, expander);
		expander->base.parent = &port->base;
		if (core->state == CORE_STATE_STARTED) {
			expander->need_report_plugin = MV_TRUE;
		}
		expander->has_been_setdown = MV_FALSE;

		expander->sas_addr = port->att_dev_sas_addr;
		expander->neg_link_rate = get_min_negotiated_link_rate(port);
		List_AddTail(&expander->base.queue_pointer, &port->expander_list);
		port->expander_count++;

		expander->parent_phy_count = 0;
		for (i=0; i<root->phy_num; i++) {
			if (port->phy_map & MV_BIT(i)) {
				expander->parent_phy_id[phy_id] = i;
				expander->parent_phy_count++;
				phy_id++;
			}
		}

		core_queue_init_entry(root, &expander->base, MV_TRUE);
	} else if (port->att_dev_info & PORT_DEV_SSP_TRGT) {
		device = get_device_obj(root, root->lib_rsrc);
		if (device == NULL) {
			CORE_DPRINT(("no more free device\n"));
			return;
		}
		sas_set_up_new_device(root, port, device);
		update_base_id(root,port,device,MV_FALSE);
		List_AddTail(&device->base.queue_pointer, &port->device_list);
		port->device_count++;
		tmp=get_spin_up_device_buf(root->lib_rsrc);
		if(!tmp || (core->spin_up_group == 0)){
			if(tmp){
				free_spin_up_device_buf(root->lib_rsrc, tmp);
			}
			core_queue_init_entry(root, &device->base, MV_TRUE);
		}else{
			MV_LIST_HEAD_INIT(&tmp->list);
			tmp->roots=root;
			tmp->base=&device->base;
			
			OSSW_SPIN_LOCK_IRQSAVE_SPIN_UP(core,flags);
			List_AddTail(&tmp->list,&core->device_spin_up_list);
			if(core->device_spin_up_timer ==NO_CURRENT_TIMER) {
				core->device_spin_up_timer=core_add_timer(core, 3,
					(MV_VOID (*) (MV_PVOID, MV_PVOID))staggered_spin_up_handler,
					&device->base, NULL);
			}
			OSSW_SPIN_UNLOCK_IRQRESTORE_SPIN_UP(core,flags);
		}
	}
}

void sas_init_target_device(pl_root *root, domain_port *port, domain_device *dev, MV_U16 targetid, MV_U16 lun)
{
	domain_device *device = NULL;
	domain_expander *expander = NULL;
	core_extension *core = (core_extension *)root->core;
	MV_U8 i, phy_id = 0;
		device = get_device_obj(root, root->lib_rsrc);
		if (device == NULL) {
			CORE_DPRINT(("no more free device\n"));
			return;
		}
		sas_set_up_new_device(root, port, device);
		update_base_id(root,port,device,MV_TRUE);
		remove_target_map(root->lib_dev->target_id_map, device->base.TargetID, MV_MAX_TARGET_NUMBER);
		device->base.LUN = lun;
		device->base.TargetID = targetid;
		device->sas_addr=dev->sas_addr;
		device->base.parent=dev->base.parent;
		CORE_DPRINT(("Init target device Target  ID %d ,  lun  %d\n",device->base.TargetID,device->base.LUN));
		List_AddTail(&device->base.queue_pointer, &port->device_list);
		port->device_count++;
		core_queue_init_entry(root, &device->base, MV_TRUE);
	
}

extern MV_BOOLEAN sata_port_store_sig(domain_port *port, MV_BOOLEAN from_sig_reg);
void sata_init_port(pl_root *root, domain_port *port)
{
	domain_pm *pm;
	domain_phy *phy = port->phy;

	CORE_DPRINT(("port %p\n", port));
	port->state = PORT_SATA_STATE_POWER_ON;

	pm = get_pm_obj(root, root->lib_rsrc);
	if (pm == NULL) {
		CORE_DPRINT(("no more free pm\n"));
		return;
	}
	set_up_new_pm(root, port, pm);
	phy->sata_signature = 0xFFFFFFFF;
	port->state = PORT_SATA_STATE_POWER_ON;
	core_queue_init_entry(root, &port->base, MV_TRUE);
}

void map_phy_id(pl_root *root);
void io_chip_init(MV_PVOID root_p)
{
	pl_root *root = (pl_root *)root_p;
	core_extension *core = (core_extension *)root->core;
	domain_phy *phy = NULL;
	domain_port *port = NULL;
	MV_U8 i, tmp_phy_map = 0;

	map_phy_id(root);
	io_chip_init_registers(root);

	core_sleep_millisecond(core, 100);

	/* get phy information */
	for (i = 0; i < root->phy_num; i++) {
		phy = &root->phy[i];
		update_phy_info(root, phy);
		update_port_phy_map(root, phy);
	}

	for (i = 0; i < root->phy_num; i++) {
		port = &root->ports[i];
		if (port->type & PORT_TYPE_SATA)
			tmp_phy_map |= port->phy_map;
	}

	mv_reset_phy(root, tmp_phy_map, MV_TRUE);

	core_sleep_millisecond(root->core, 100);

	/* to init port, discover first level devices */
	for (i = 0; i < root->phy_num; i++) {
		port = &root->ports[i];
		if (port->phy_num == 0) continue;

		if (port->type & PORT_TYPE_SAS) {
			sas_init_port(root, port);
		} else {
			sata_init_port(root, port);
		}
	}
}

void write_wide_port_register(pl_root *root, domain_port *port)
{
	MV_U8 i;
	MV_U32 reg;
	domain_phy *phy;

	/* for all phy participated, need update CONFIG_WIDE_PORT register */
	for (i = 0; i < root->phy_num; i++) {
		if (port->phy_map & MV_BIT(i)) {
			phy = &root->phy[i];
			WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_WIDE_PORT);
			reg = READ_PORT_CONFIG_DATA(root, phy);
			reg &= ~WIDE_PORT_PHY_MASK;
			reg |= port->asic_phy_map;
			WRITE_PORT_CONFIG_DATA(root, phy, reg);
		}
	}
}

void init_port(pl_root *root, domain_port *port)
{

	if (port->phy_num) {
		CORE_DPRINT(("port %p has phy_num %d, phy_map %X, att_dev_sas_addr %016llx.\n",port,port->phy_num,port->phy_map,port->att_dev_sas_addr.value));
	}

	port->phy_num = 0;
	port->phy_map = 0;
	port->asic_phy_map = 0;
	port->phy = NULL;
	port->att_dev_sas_addr.value =0;

	MV_ASSERT(port->phy_num == 0);
	MV_ASSERT(port->phy_map == 0);
	MV_ASSERT(port->asic_phy_map == 0);
	MV_ASSERT(port->phy == NULL);
	MV_ASSERT(U64_COMPARE_U32(port->att_dev_sas_addr, 0) == 0);

	port->base.type = BASE_TYPE_DOMAIN_PORT;
	port->base.port = port;
	port->base.root = root;

	MV_LIST_HEAD_INIT(&port->device_list);
	MV_LIST_HEAD_INIT(&port->expander_list);
	MV_LIST_HEAD_INIT(&port->current_tier);
	MV_LIST_HEAD_INIT(&port->next_tier);

	port->device_count = 0;
	port->expander_count = 0;
	port->init_count = 0;
}

domain_port * get_port(pl_root *root, domain_phy *phy)
{
	domain_port *port;

	/* wide port will search the existing ports first.
	 * if cannot find one, will come here */
	port = &root->ports[phy->id];
	init_port(root, port);
	CORE_DPRINT(("phy %d on port %d.\n", phy->id, port->base.id));
	return port;
}

void update_port_phy_map(pl_root *root, domain_phy *phy)
{
	domain_port *port;
	MV_U32 i;
	port = phy->port;

	/*
	 * the possible case code comes here,
	 * phy is coming,
	 * phy is changed, sas to sata, wide port to narrow, different attached sas
	 * phy is removed,
	 * for easy approach, deform the port and regenerate the port
	 */
	if (port != NULL) {
		/* deform the wide port */
		MV_DASSERT(port->phy_num >= 1);

		if (port->phy_num >= 1) {
			port->phy_num--;
			port->phy_map &= ~MV_BIT(phy->id);
			port->asic_phy_map &= ~MV_BIT(phy->asic_id);
		}	

		phy->port = NULL;
		if (port->phy_num == 0) {
			MV_ASSERT(port->phy_map == 0);
			MV_ASSERT(port->asic_phy_map == 0);
			port->phy = NULL;
			port->dev_info = 0;
			port->att_dev_info = 0;
			MV_ZeroMemory(&port->att_dev_sas_addr, sizeof(port->att_dev_sas_addr));
			MV_ZeroMemory(&port->dev_sas_addr, sizeof(port->dev_sas_addr));
		} else {
			/* update phy and link_rate */
			port->phy = NULL;
			port->link_rate = SAS_LINK_RATE_6_0_GBPS;
			for (i = 0; i < root->phy_num; i++) {
				domain_phy *tmp_phy;
				tmp_phy = &root->phy[i];
				if (port->phy_map & MV_BIT(tmp_phy->id)) {
					if (port->phy == NULL) {
						port->phy = tmp_phy;
					}
					port->link_rate =
						MV_MIN(port->link_rate,
						get_phy_link_rate(tmp_phy->phy_status));
				}
			}
			write_wide_port_register(root, port);
			port = NULL;
		}
	}

	if (0 == (phy->phy_status & SCTRL_PHY_READY_MASK))
		return;

	/* treat it as a new phy, if it's sas, try to find a wide port */
	if (phy->type & PORT_TYPE_SAS) {
		if (!(phy->att_dev_info &
			(PORT_DEV_SMP_TRGT | PORT_DEV_SSP_TRGT | PORT_DEV_STP_TRGT)))
			return;

		for (i = 0; i < root->phy_num; i++) {
			domain_port *tmp_port;
			tmp_port = &root->ports[i];
			if ((tmp_port->phy_num > 0)
				&& (tmp_port->type & PORT_TYPE_SAS)
				&& (U64_COMPARE_U64(
					tmp_port->att_dev_sas_addr, phy->att_dev_sas_addr)==0)
				&& (U64_COMPARE_U64(
					tmp_port->dev_sas_addr, phy->dev_sas_addr)==0))
			{
				/* find the wide port */
				phy->port = tmp_port;
				tmp_port->phy_num++;
				tmp_port->phy_map |= MV_BIT(phy->id);
				tmp_port->asic_phy_map |= MV_BIT(phy->asic_id);
				
				MV_ASSERT(tmp_port->phy != NULL);

				tmp_port->link_rate =
					MV_MIN(tmp_port->link_rate,
					get_phy_link_rate(phy->phy_status));
				write_wide_port_register(root, tmp_port);
				return;
			}

		}
	}

	if (port == NULL) {
		port = get_port(root, phy);
	}
	MV_ASSERT(port != NULL);
	phy->port = port;	
	port->type = phy->type;
	MV_ASSERT(port->phy_num == 0);
	port->phy_num = 1;
	port->phy_map = MV_BIT(phy->id);
	port->asic_phy_map = MV_BIT(phy->asic_id);
	port->phy = &root->phy[phy->id];
	port->link_rate = get_phy_link_rate(phy->phy_status);
	port->dev_info = phy->dev_info;
	MV_CopyMemory(&port->dev_sas_addr, &phy->dev_sas_addr,
		sizeof(phy->dev_sas_addr));
	port->att_dev_info = phy->att_dev_info;
	MV_CopyMemory(&port->att_dev_sas_addr, &phy->att_dev_sas_addr,
		sizeof(phy->att_dev_sas_addr));

	write_wide_port_register(root, port);
}


