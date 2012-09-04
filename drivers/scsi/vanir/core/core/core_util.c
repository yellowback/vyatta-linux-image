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
#include "core_util.h"
#include "core_internal.h"
#include "core_manager.h"

#include "core_type.h"
#include "core_error.h"
#include "hba_exp.h"
#include "core_spi.h"

int core_suspend(void *ext)
{
	AdapterInfo AI;
	core_extension *core_ext = (core_extension *)ext;

	core_disable_ints(core_ext);
	return 0;
}

int core_resume(void *ext)
{
	int ret;
	MV_U8 i;
	AdapterInfo AI;
	core_extension *core_ext = (core_extension *)ext;
	pl_root * root = NULL;

	if(core_reset_controller(core_ext) == MV_FALSE)
		return -1;
	core_enable_ints(core_ext);

	for(i = core_ext->chip_info->start_host; i < (core_ext->chip_info->start_host + core_ext->chip_info->n_host); i++){
		root = &core_ext->roots[i];
		io_chip_init_registers(root);
		root->last_cmpl_q = 0xfff;
		root->last_delv_q = 0xfff;
		root->running_num = 0;
	}
       return 0;
}

MV_LPVOID get_core_mmio(pl_root *root)
{
	core_extension *core = (core_extension *)root->core;
	return core->mmio_base;
}

MV_VOID core_queue_init_entry(pl_root *root, MV_PVOID base_ext, MV_BOOLEAN start_init)
{
	core_extension *core = (core_extension *)root->core;
	struct _domain_base *base = (struct _domain_base *)base_ext;
	struct _error_context *err_ctx = &base->err_ctx;
	MV_U32 count = Counted_List_GetCount(&core->init_queue, MV_TRUE);
	MV_U8 i;
	struct _domain_base *item;

	if (count != Counted_List_GetCount(&core->init_queue, MV_FALSE)) {
		CORE_DPRINT(("init_queue count  is conflict %d[%d]\n",count,Counted_List_GetCount(&core->init_queue, MV_FALSE)));
	}
	MV_DASSERT(count == Counted_List_GetCount(&core->init_queue, MV_FALSE));
	LIST_FOR_EACH_ENTRY_TYPE(item , &core->init_queue, struct _domain_base,
		init_queue_pointer) {
		if (item == base) {
			CORE_DPRINT(("find same base at init queue %p.\n",base));
			return;
		} 
	}

	if (MV_TRUE == start_init) {
		core->init_queue_count++;
		base->port->init_count++;
		err_ctx->retry_count = 0;
		Counted_List_AddTail(&base->init_queue_pointer, &core->init_queue);
	} else {
		/* for dump mode and hotplug, we need totally finish one entry first */
		Counted_List_Add(&base->init_queue_pointer, &core->init_queue);
	}
}

MV_U8 enable_spin_up(MV_PVOID hba){
	core_extension *core = (core_extension *)HBA_GetModuleExtension(hba, MODULE_CORE);
	if (core->spin_up_group != 0) {
		core_notify_device_hotplug(&core->roots[core->chip_info->start_host],
						MV_TRUE,VIRTUAL_DEVICE_ID, MV_FALSE);
		return MV_TRUE;
	} else
		return MV_FALSE;

}
MV_U32 core_spin_up_get_cached_memory_quota(MV_U16 max_io)
{
	MV_U32 size = 0;

	size=sizeof(struct device_spin_up)*MV_MAX_TARGET_NUMBER;

	return size;
}

MV_VOID staggered_spin_up_handler(domain_base * base, MV_PVOID tmp)
{
	domain_device *device;
	core_extension *core = (core_extension *)base->root->core;
	MV_U8 num=0,renew_timer=0;
	MV_ULONG flags;
	struct device_spin_up *su=NULL;
	if((base == NULL) || (core == NULL)){
		return;
		}
	CORE_DPRINT(("Staggered spin up ...\n"));
	device = (domain_device *)base;
	OSSW_SPIN_LOCK_IRQSAVE_SPIN_UP((void *)core,flags);
	while(!List_Empty(&core->device_spin_up_list)){
		if((num >= core->spin_up_group) || (core->state != CORE_STATE_STARTED)){
			renew_timer=1;
			break;
		}
		su = (struct device_spin_up *)List_GetFirstEntry(
			&core->device_spin_up_list, struct device_spin_up,list);

		if(su){
			if(su->roots==NULL){
				CORE_DPRINT(("Got a NULL device\n"));
				continue;
			}else{
				if(base->type == BASE_TYPE_DOMAIN_DEVICE){
					domain_device *tmp_device=(domain_device *)su->base;
					tmp_device->state=DEVICE_STATE_RESET_DONE;
				}
				core_queue_init_entry(su->roots, su->base, MV_TRUE);
			}
			num++;
			free_spin_up_device_buf(su->roots->lib_rsrc,su);
		}
		
	}
	if(List_Empty(&core->device_spin_up_list)){
		if(core->device_spin_up_timer != NO_CURRENT_TIMER)
			core_cancel_timer(core, core->device_spin_up_timer);
		core->device_spin_up_timer =NO_CURRENT_TIMER;
		}else{
			
			if(core->device_spin_up_timer ==NO_CURRENT_TIMER){
				core->device_spin_up_timer=core_add_timer(core, core->spin_up_time, (MV_VOID (*) (MV_PVOID, MV_PVOID))staggered_spin_up_handler, base, NULL);
			}else{
				if(renew_timer){
					core->device_spin_up_timer=core_add_timer(core, core->spin_up_time, (MV_VOID (*) (MV_PVOID, MV_PVOID))staggered_spin_up_handler, base, NULL);
				}
			}
		}
	OSSW_SPIN_UNLOCK_IRQRESTORE_SPIN_UP(core,flags);

}

MV_VOID core_init_entry_done(pl_root *root, domain_port *port,
	domain_base *base)
{
	core_extension *core = (core_extension *)root->core;
	domain_device *dev;
	domain_expander *exp;
	domain_enclosure *enc;
	MV_ULONG flags;

	if(core->init_queue_count){
		core->init_queue_count--;
	} else {
		CORE_DPRINT(("core->init_queue_count is zero.\n"));
	}

        if(port->init_count) {
		port->init_count--;
        } else {
        }

        /* if init failure, base == NULL */
	if (base != NULL) {
		switch (base->type) {
		case BASE_TYPE_DOMAIN_DEVICE:
			dev = (domain_device *)base;
			MV_ASSERT(dev->status & DEVICE_STATUS_FUNCTIONAL);
			if ((core->state == CORE_STATE_STARTED)){
				core_notify_device_hotplug(root, MV_TRUE,
					base->id, MV_TRUE);
				if(core->spin_up_group){
					OSSW_SPIN_LOCK_IRQSAVE_SPIN_UP(core,flags);
					if(dev->status&DEVICE_STATUS_SPIN_UP){
						dev->status &=~DEVICE_STATUS_SPIN_UP;
					}
					OSSW_SPIN_UNLOCK_IRQRESTORE_SPIN_UP(core,flags);
				}
			}
			break;
		case BASE_TYPE_DOMAIN_ENCLOSURE:
			enc = (domain_enclosure *)base;
			if (enc->enc_flag & ENC_FLAG_FIRST_INIT) {
				enc->enc_flag &= ~ENC_FLAG_FIRST_INIT;
				if (core->state == CORE_STATE_STARTED)
					core_notify_device_hotplug(root, MV_TRUE,
						base->id, MV_TRUE);
			}
			break;
		case BASE_TYPE_DOMAIN_PM:
			port->pm = (domain_pm *)base;
			break;
		case BASE_TYPE_DOMAIN_EXPANDER:
			exp = (domain_expander *)base;
			if ((core->state == CORE_STATE_STARTED) &&
				(exp->enclosure) &&
				(exp->enclosure->enc_flag & ENC_FLAG_NEED_REINIT)) {
				exp->enclosure->state = ENCLOSURE_INQUIRY_DONE;
				exp->enclosure->enc_flag &= ~ENC_FLAG_NEED_REINIT;
				core_queue_init_entry(root,
					&exp->enclosure->base, MV_TRUE);
			}
			if (exp->need_report_plugin) {
				core_generate_event(root->core,
					EVT_ID_EXPANDER_PLUG_IN, exp->base.id,
					SEVERITY_INFO, 0, NULL, 0);
				exp->need_report_plugin = MV_FALSE;
			}
			break;
		case BASE_TYPE_DOMAIN_I2C:
			break;
		case BASE_TYPE_DOMAIN_PORT:
			break;
		default:
			MV_DASSERT(MV_FALSE);
			break;
		}
	}
}

MV_PVOID core_get_handler(pl_root *root, MV_U8 handler_id)
{
	core_extension *core = (core_extension *)root->core;
	return (MV_PVOID)&core->handlers[handler_id];
}

MV_VOID core_append_request(pl_root *root, PMV_Request req)
{
	core_extension *core = (core_extension *)root->core;

	Counted_List_AddTail(&req->Queue_Pointer, &core->waiting_queue);
}

MV_VOID core_push_running_request_back(pl_root *root, PMV_Request req)
{
	core_extension *core = (core_extension *)root->core;

	Counted_List_Add(&req->Queue_Pointer, &core->waiting_queue);
}

MV_VOID core_append_high_priority_request(pl_root *root, PMV_Request req)
{
	core_extension *core = (core_extension *)root->core;

	Counted_List_AddTail(&req->Queue_Pointer, &core->high_priority_queue);
}

void core_append_init_request(pl_root *root, MV_Request *req)
{
	core_extension *core = (core_extension *)root->core;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	
	MV_ASSERT(ctx != NULL);

	ctx->req_type = CORE_REQ_TYPE_INIT;
	req->Time_Out = HBA_CheckIsFlorence(core) ?
		CORE_REQ_FLORENCE_INIT_TIMEOUT : CORE_REQ_INIT_TIMEOUT;
	core_append_request(root, req);
}

void core_queue_eh_req(pl_root *root, MV_Request *req)
{
	core_extension *core = (core_extension *)root->core;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];

	if (ctx->req_type == CORE_REQ_TYPE_NONE) {
	ctx->req_type = CORE_REQ_TYPE_ERROR_HANDLING;
	}
	MV_ASSERT((ctx->req_type == CORE_REQ_TYPE_ERROR_HANDLING)
		|| (ctx->req_type == CORE_REQ_TYPE_RETRY));

	req->Time_Out = 2;
	core_append_high_priority_request(root, req);
}

void core_queue_error_req(pl_root *root, MV_Request *req, MV_BOOLEAN new_error)
{
	core_extension *core = (core_extension *)root->core;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_base *base = (domain_base *)get_device_by_id(root->lib_dev,
		req->Device_Id);

	MV_ASSERT(ctx);
	MV_ASSERT(get_device_by_id(root->lib_dev, req->Device_Id) == base);
	MV_ASSERT(!CORE_IS_INIT_REQ(ctx));

	if (new_error) {
		CORE_EH_PRINT(("dev %d queue original error req %p [0x%x].\n",\
			base->id, req, req->Cdb[0]));
		base->err_ctx.error_count++;

		MV_ASSERT(!CORE_IS_EH_REQ(ctx));
		Counted_List_AddTail(&req->Queue_Pointer, &core->error_queue);
	} else {
		CORE_EH_PRINT(("no resource, push back error req %p [0x%x].\n",\
			req, req->Cdb[0]));
		Counted_List_Add(&req->Queue_Pointer, &core->error_queue);
	}
}

void core_complete_error_req(pl_root *root, MV_Request *req, MV_U8 status)
{
	core_extension *core = (core_extension *)root->core;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_base *base = (domain_base *)get_device_by_id(root->lib_dev,
		req->Device_Id);
	struct _error_context *err_ctx = &base->err_ctx;

	CORE_DPRINT(("returns %p with status 0x%x.\n", \
		req, status));
	MV_ASSERT(get_device_by_id(root->lib_dev, req->Device_Id) == base);

	MV_ASSERT(ctx->req_type == CORE_REQ_TYPE_NONE);
	if ((req->Scsi_Status == REQ_STATUS_NO_DEVICE) && (err_ctx->eh_type == EH_TYPE_TIMEOUT)) {
		domain_device *device = (domain_device *)base;
		CORE_DPRINT(("Find device %d is bad disk\n",base->id));
		device->status = DEVICE_STATUS_NO_DEVICE;
	}
	
	err_ctx->eh_type = EH_TYPE_NONE;
	err_ctx->eh_state = EH_STATE_NONE;
	err_ctx->error_count--;
	err_ctx->error_req = NULL;
	if ((status == REQ_STATUS_SUCCESS)
		&& (err_ctx->state == BASE_STATE_ERROR_HANDLING)) {
		err_ctx->state = BASE_STATE_ERROR_HANDLED;
	} else
		err_ctx->state = BASE_STATE_NONE;

	if (err_ctx->eh_timer != NO_CURRENT_TIMER) {
		core_cancel_timer(core, err_ctx->eh_timer);
		err_ctx->eh_timer = NO_CURRENT_TIMER;
	}

	MV_ASSERT(status != REQ_STATUS_TIMEOUT);
	req->Scsi_Status = status;

	if ((req->Scsi_Status == REQ_STATUS_SUCCESS) 
		&& (req->Sense_Info_Buffer_Length != 0)
		&& (req->Sense_Info_Buffer != NULL)) {
 		((MV_PU8)req->Sense_Info_Buffer)[0] = 0; 
		((MV_PU8)req->Sense_Info_Buffer)[2] = 0;
		((MV_PU8)req->Sense_Info_Buffer)[7] = 0;
		((MV_PU8)req->Sense_Info_Buffer)[12] = 0;
		((MV_PU8)req->Sense_Info_Buffer)[13] = 0;
	}

	core_queue_completed_req(core, req);
}
void
core_notify_device_hotplug(pl_root *root, MV_BOOLEAN plugin,
	MV_U16 dev_id, MV_U8 generate_event)
{
	core_extension *core = (core_extension *)root->core;

	struct mod_notif_param param;
	MV_PVOID upper_layer;
	MV_VOID (*notify_func)(MV_PVOID, enum Module_Event, struct mod_notif_param *);
	domain_base *base;
	MV_U16 report_id = 0xffff;

	if (core->state != CORE_STATE_STARTED) return;

	base = get_device_by_id(root->lib_dev, dev_id);
	if (base == NULL) {
		MV_ASSERT(MV_FALSE);
		return;
		goto core_notify_device_hotplug_done;
	}

	if (plugin == MV_TRUE) {
		report_id = base->id;

		if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
			if (generate_event == MV_TRUE)
				core_generate_event(core, EVT_ID_HD_ONLINE,
					base->id, SEVERITY_INFO, 0, NULL ,0);
		} else {
			if(base->type != BASE_TYPE_DOMAIN_ENCLOSURE)
				if (generate_event == MV_TRUE)
					core_generate_event(core, EVT_ID_HD_ONLINE,
						base->id, SEVERITY_INFO, 0, NULL ,0);
		}
	} else {
		report_id = base->id;

		if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
			if (generate_event == MV_TRUE)
				core_generate_event(core, EVT_ID_HD_OFFLINE,
					base->id, SEVERITY_WARNING, 0, NULL ,0);
		} else {
			if(base->type != BASE_TYPE_DOMAIN_ENCLOSURE)
				if (generate_event == MV_TRUE)
					core_generate_event(core, EVT_ID_HD_OFFLINE,
						base->id, SEVERITY_WARNING, 0, NULL ,0);
		}
	}

core_notify_device_hotplug_done:

	CORE_PRINT(("%s device %d.\n",
		plugin? "plugin" : "unplug",
		report_id));

	HBA_GetUpperModuleNotificationFunction(core, &upper_layer, &notify_func);

	param.lo = base->TargetID;
	param.hi = base->LUN;
    	param.param_count = 0;

	notify_func(upper_layer,
		plugin ? EVENT_DEVICE_ARRIVAL : EVENT_DEVICE_REMOVAL,
		&param);
}

MV_U16 core_add_timer(MV_PVOID core, MV_U32 seconds,
	MV_VOID (*routine) (MV_PVOID, MV_PVOID),
	MV_PVOID context1, MV_PVOID context2)
{
	MV_U16 timer_id =
		Timer_AddRequest(core, seconds*2, routine, context1, context2);
	MV_DASSERT(timer_id != NO_CURRENT_TIMER);
	return timer_id;
}

MV_VOID core_cancel_timer(MV_PVOID core, MV_U16 timer)
{
	Timer_CancelRequest(core, timer);
}

MV_VOID core_generate_error_event(MV_PVOID core_p, MV_Request *req)
{
	core_extension *core = core_p;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *dev = (domain_device *)get_device_by_id(&core->lib_dev,
		req->Device_Id);
	MV_U32 params[MAX_EVENT_PARAMS];
	MV_PU8 sense;
	MV_U8 sense_key;

        if(req->Sense_Info_Buffer == NULL) {
                if (HBA_CheckIsFlorence(core)) {
                        params[0] = req->Cdb[0];
                        params[1] = 0xFF;
                        params[2] = req->Scsi_Status;
                        params[3] = 0;

        	        core_generate_event(core, EVT_ID_HD_SC_ERROR, dev->base.id,
	                        SEVERITY_INFO, 4, params, 0);
                }
		return;
        }
	sense = (MV_PU8)req->Sense_Info_Buffer;
	sense_key = sense[2] & 0x0f;

        if (!HBA_CheckIsFlorence(core)) {
	        if ((sense_key == SCSI_SK_NO_SENSE) ||
	                (sense_key == SCSI_SK_UNIT_ATTENTION) ||
	                (sense_key == SCSI_SK_NOT_READY) ||
	                (sense_key == SCSI_SK_ILLEGAL_REQUEST))
	                return;
        }

	params[0] = req->Cdb[0];
	params[1] = sense[2];
	params[2] = sense[12];
	params[3] = sense[13];

	core_generate_event(core, EVT_ID_HD_SC_ERROR, dev->base.id,
	        SEVERITY_INFO, 4, params, 0x1e);
}

MV_U32 List_Get_Count(List_Head * head)
{
	List_Head *pPos;
	MV_U32 count = 0;

	LIST_FOR_EACH(pPos, head) {
		count++;
	}
	return count;
}

MV_PVOID core_map_data_buffer(MV_Request *req)
{
	sg_map(req);
	return req->Data_Buffer;
}

MV_VOID core_unmap_data_buffer(MV_Request *req)
{
	sg_unmap(req);
}

MV_U8
core_check_duplicate_device(pl_root *root,
	domain_device *dev)
{
	core_extension  *core = (core_extension *)root->core;
	domain_base     *tmp_base;
	domain_device   *tmp_dev;
	MV_U16          dev_id;

	if (U64_COMPARE_U32(dev->WWN, 0) == 0) {
		CORE_PRINT(("Device WWN is 0. Not checking duplicate.\n"));
		return MV_FALSE;
	}

	for (dev_id = 0; dev_id < MAX_ID; dev_id++) {
		if (dev_id == dev->base.id)
			continue;

		tmp_base = get_device_by_id(&core->lib_dev, dev_id);

		if (tmp_base == NULL)
			continue;

		if (tmp_base->type != BASE_TYPE_DOMAIN_DEVICE)
			continue;

		tmp_dev = (domain_device*)tmp_base;

		if (U64_COMPARE_U64(dev->WWN, tmp_dev->WWN) == 0)
			return MV_TRUE;
	}

	return MV_FALSE;
}

MV_U32
core_check_outstanding_req(MV_PVOID core_p)
{
        core_extension *core = (core_extension *)core_p;
        domain_base *base;
        MV_U32 result = 0;
        MV_U16 dev_id;

        for (dev_id = 0; dev_id < MAX_ID; dev_id++) {
                base = get_device_by_id(&core->lib_dev, dev_id);
                if (base == NULL)
                        continue;
                result += base->outstanding_req;
        }

        return result;
}

/*
 * core_wideport_load_balance_asic_phy_map
 *
 * This function returns the asic phy map to use to send a frame to a device.
 * The phy maps returned are of round robin per SAS device, and that SAS
 * device will use the same phy for all commands. STP devices should not
 * use software load balance as hardware will balance the register sets used.
 */
MV_U8
core_wideport_load_balance_asic_phy_map(domain_port *port, domain_device *dev)
{
	MV_U8 phy_map;
	MV_U8 asic_phy_map = port->asic_phy_map;
	domain_base *base = &dev->base;
	if (BASE_TYPE_DOMAIN_DEVICE != base->type)
		return asic_phy_map;

	if ((dev->curr_phy_map & asic_phy_map) != 0) return dev->curr_phy_map;

	if (asic_phy_map == 0) return asic_phy_map;

	phy_map = port->curr_phy_map << 1;
	if (phy_map > asic_phy_map || phy_map == 0)
		phy_map = (MV_U8)MV_BIT(ossw_ffs(asic_phy_map));
	while (phy_map <= asic_phy_map) {
		if (phy_map & asic_phy_map) {
			port->curr_phy_map = phy_map;
			dev->curr_phy_map = phy_map;
			return phy_map;
		}
		phy_map <<= 1;
	}

	MV_DASSERT(MV_FALSE);

	return asic_phy_map;
}


