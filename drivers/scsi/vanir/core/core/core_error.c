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
#include "core_internal.h"
#include "core_manager.h"
#include "core_util.h"
#include "core_hal.h"
#include "core_sas.h"
#include "core_sata.h"

#include "core_error.h"
#include "core_util.h"
#include "core_protocol.h"
#include "core_api.h"

#include "core_exp.h"
#include "core_expander.h"

MV_VOID core_req_timeout(MV_PVOID dev_p, MV_PVOID param);
extern MV_Request *sas_make_task_req(pl_root *root, domain_device *dev,
MV_Request *org_req, MV_U8 task_function, MV_ReqCompletion func);
extern MV_VOID core_clean_init_queue_entry(MV_PVOID root_p,
				domain_base *base);

/*
 * timer related functions
 */
MV_VOID mv_add_timer(MV_PVOID core_p, MV_Request *req)
{
	core_extension *core = (core_extension *)core_p;
	domain_base *base;
	MV_U32 seconds;
	struct _error_context *err_ctx;

	base = get_device_by_id(&core->lib_dev, req->Device_Id);
	err_ctx = &base->err_ctx;

	if (err_ctx->timer_id == NO_CURRENT_TIMER) {
		if (req->Time_Out != 0) {
			seconds = req->Time_Out;
		} else {
			seconds = CORE_REQUEST_TIME_OUT_SECONDS;
		}

		if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
			domain_device *dev = (domain_device *)base;
			domain_port *port = (domain_port *)dev->base.port;
			if (IS_HDD(dev)) {
				seconds = MV_MAX(seconds,
					CORE_REQUEST_TIME_OUT_SECONDS);
				if (req->Cdb[0] == SCSI_CMD_INQUIRY) {
					seconds = CORE_INQUIRE_TIME_OUT_SECONDS;
				}

			} else if (IS_OPTICAL(dev)) {
				seconds = 200;
			} else if (IS_TAPE(dev)) {
				seconds = 400;
			}
		}

		err_ctx->timer_id = core_add_timer(
			core, seconds, core_req_timeout, base, req);
	}

	MV_DASSERT(err_ctx->timer_id != NO_CURRENT_TIMER);
	List_AddTail(&req->Queue_Pointer, &err_ctx->sent_req_list);
}

MV_VOID mv_cancel_timer(MV_PVOID core_p, MV_PVOID base_p)
{
	core_extension *core = (core_extension *)core_p;
	domain_base *base = (domain_base *)base_p;
	struct _error_context *err_ctx = &base->err_ctx;

	if (err_ctx->timer_id != NO_CURRENT_TIMER) {
		core_cancel_timer(core, err_ctx->timer_id);
		err_ctx->timer_id = NO_CURRENT_TIMER;
	}
}

MV_VOID mv_renew_timer(MV_PVOID core_p, MV_Request *req)
{
	core_extension *core = (core_extension *)core_p;
	domain_base *base;
	MV_Request *oldest_req;
	struct _error_context *err_ctx;

	base = get_device_by_id(&core->lib_dev, req->Device_Id);
	if (base == NULL) {
		CORE_PRINT(("device %d is gone?", req->Device_Id));
		return;
	}
	err_ctx = &base->err_ctx;

	MV_DASSERT(!List_Empty(&err_ctx->sent_req_list));

	/* check if this is the oldest request */
	oldest_req = (MV_Request *)List_GetFirstEntry(
		&err_ctx->sent_req_list, MV_Request, Queue_Pointer);

	if (oldest_req == req) {
		mv_cancel_timer(core, base);

		if (!List_Empty(&err_ctx->sent_req_list)) {
			oldest_req = LIST_ENTRY(
				(&err_ctx->sent_req_list)->next, MV_Request, Queue_Pointer);

			if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
				domain_device *dev = (domain_device *)base;
				MV_DASSERT(!IS_ATAPI(dev));
				MV_DASSERT(!IS_TAPE(dev));
			}
			err_ctx->timer_id = core_add_timer(
				core, req->Time_Out,core_req_timeout, base, oldest_req);
			MV_DASSERT(err_ctx->timer_id != NO_CURRENT_TIMER);
		} else {
			MV_DASSERT(err_ctx->timer_id == NO_CURRENT_TIMER);
		}
	} else {
		List_Add(&oldest_req->Queue_Pointer, &err_ctx->sent_req_list);
		List_Del(&req->Queue_Pointer);
	}
}

/*
 * error handling common functions
 */
/* reset slot - clear command active and command issue */
MV_VOID prot_reset_slot(pl_root *root, domain_base *base, MV_U16 slot,
	MV_Request *req)
{
	CORE_EH_PRINT(("resetting slot %x, req[%p]\n", slot,req));
	MV_REG_WRITE_DWORD(root->mmio_base,
		COMMON_CMD_ADDR, CMD_PORT_ACTIVE0 + ((slot >> 5) << 2));
	MV_REG_WRITE_DWORD(root->mmio_base,
		COMMON_CMD_DATA, MV_BIT(slot % 32));
	prot_clean_slot(root, base, slot, req);
}

MV_U16 gSMPRetryCount = 0;

extern MV_VOID core_push_queues(MV_PVOID core_p);
MV_VOID
sas_wait_for_spinup(MV_PVOID dev, MV_PVOID  tmp)
{
	domain_device* device = (domain_device *)dev;
	core_queue_init_entry(device->base.root, &device->base, MV_FALSE);
	core_push_queues(device->base.root->core);
}
/* handle init request error */
#define SSP_INIT_REQUEST_RETRY_COUNT         100
#define SMP_INIT_REQUEST_RETRY_COUNT         10
#define OTHER_INIT_REQUEST_RETRY_COUNT       10
void core_handle_init_error(pl_root *root, domain_base *base, MV_Request *req)
{
	domain_port *port = base->port;
	domain_device *dev;
	domain_expander *exp;
	domain_expander *tmp_exp;
	domain_enclosure *enc;
	struct _error_context *err_ctx = &base->err_ctx;
	core_context *ctx = NULL;
	MV_U16 retry_count;
	MV_U8 i;

	retry_count = OTHER_INIT_REQUEST_RETRY_COUNT;
	if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
		dev = (domain_device *)base;
		if (IS_SSP(dev)) {
			retry_count = SSP_INIT_REQUEST_RETRY_COUNT;
		}
	}
	else if (base->type == BASE_TYPE_DOMAIN_EXPANDER) {
		retry_count = SMP_INIT_REQUEST_RETRY_COUNT;
		if (gSMPRetryCount < err_ctx->retry_count) {
			gSMPRetryCount = err_ctx->retry_count;
		}
		MV_ASSERT(gSMPRetryCount <= SMP_INIT_REQUEST_RETRY_COUNT);
	}

	if (((core_extension *)(root->core))->revision_id != VANIR_C2_REV) {
		ctx = req->Context[MODULE_CORE];
		if (ctx->error_info & EH_INFO_WD_TO_RETRY) {   
			pal_abort_port_running_req(root, port);

			core_sleep_millisecond(root->core, 10);

			for (i = 0; i < root->phy_num; i++) {
				if (root->phy[i].port == port) {
					WRITE_PORT_PHY_CONTROL(root,
						(&root->phy[i]),
						(SCTRL_STP_LINK_LAYER_RESET | SCTRL_SSP_LINK_LAYER_RESET));
				}
			}

			core_sleep_millisecond(root->core, 50);

			LIST_FOR_EACH_ENTRY_TYPE(tmp_exp, &port->expander_list,
				domain_expander, base.queue_pointer) {
				pal_clean_expander_outstanding_req(root, tmp_exp);
			}

		}
	}


	/* make the init stage error handling as simple as possible
	 * don't mess up init state machine with runtime error handling state machine
	 * if it's media error, just retry several times
	 * if it's timeout, just give up */
	MV_DASSERT(req->Scsi_Status != REQ_STATUS_SUCCESS);

	err_ctx->retry_count++;

	if (req->Scsi_Status == REQ_STATUS_TIMEOUT) {
		if (err_ctx->retry_count < retry_count ) {
			err_ctx->retry_count = retry_count;
		}
	} 
	{
		switch (base->type) {
		case BASE_TYPE_DOMAIN_DEVICE:
			dev = (domain_device *)base;
			if (IS_STP(dev)) {
				CORE_DPRINT(("STP device %d status %x has error_count %d, req status %X.\n", base->id, dev->state, err_ctx->retry_count, req->Scsi_Status));
			}
			break;
		default:
			break;
		}
	}

	if ((req->Scsi_Status == REQ_STATUS_NO_DEVICE)
		|| (err_ctx->retry_count > retry_count)) {
		switch (base->type) {
		case BASE_TYPE_DOMAIN_DEVICE:
			dev = (domain_device *)base;
			pal_set_down_error_disk(root, dev, MV_TRUE);
			break;
		case BASE_TYPE_DOMAIN_PM:
			pal_set_down_port(root, port);
			break;
		case BASE_TYPE_DOMAIN_EXPANDER:
			exp = (domain_expander *)base;
                        if (exp->has_been_setdown == MV_TRUE) {
                                MV_DASSERT(MV_FALSE);
                                return;
                        }
			exp->has_been_setdown = MV_TRUE;
			pal_set_down_expander(root, exp);
			break;
		case BASE_TYPE_DOMAIN_ENCLOSURE:
			enc = (domain_enclosure *)base;
			if(req->Scsi_Status == REQ_STATUS_NO_DEVICE){
				pal_set_down_enclosure(root, enc);
			}else{
				enc->state = ENCLOSURE_INIT_DONE;
				core_init_entry_done(root, port, base);
				return;
			}
			break;
		case BASE_TYPE_DOMAIN_PORT:
                case BASE_TYPE_DOMAIN_VIRTUAL:
		default:
			MV_ASSERT(MV_FALSE);
			break;
		}

		core_init_entry_done(root, port, NULL);
	} else {
		if((req->Scsi_Status == REQ_STATUS_HAS_SENSE)){
			MV_U8 sense_key;
			MV_U16 timer_id;
			sense_key = ((MV_U8 *)req->Sense_Info_Buffer)[2] & 0x0f;
			if(sense_key == SCSI_SK_NOT_READY){
				timer_id = core_add_timer(root->core, 1, sas_wait_for_spinup, base, NULL);
				MV_ASSERT(timer_id != NO_CURRENT_TIMER);
				return;
			}
		}
		core_sleep_millisecond(root->core, 10);

		core_queue_init_entry(root, base, MV_FALSE);
	}
}

MV_VOID core_req_timeout(MV_PVOID dev_p, MV_PVOID req_p)
{

	domain_base *base = NULL;
	MV_Request *err_req = NULL;
	pl_root *root = NULL;
	struct _error_context *err_ctx = NULL;
	core_context *ctx = NULL;
	MV_Request *first_req = NULL;
	MV_U32 tmp=0;
	domain_device *dev = NULL;

	MV_DASSERT(dev_p!= NULL);
	MV_DASSERT(req_p!= NULL);

	base = (domain_base *)dev_p;
	err_req = (MV_Request *)req_p;
	root = (pl_root *)base->root;
	err_ctx = &base->err_ctx;
	ctx = err_req->Context[MODULE_CORE];

	if(IS_ATA_PASSTHRU_CMD(err_req,ATA_CMD_SEC_ERASE_UNIT)){
		CORE_EH_PRINT(("The command of security erase is running...\n"));
		err_ctx->timer_id = core_add_timer(root->core, 60,core_req_timeout, base, err_req);
		return;
	}
	MV_REG_WRITE_DWORD(root->mmio_base,
		COMMON_CMD_ADDR, CMD_PORT_ACTIVE0 + ((ctx->slot >> 5) << 2));
	tmp = MV_REG_READ_DWORD(root->mmio_base, COMMON_CMD_DATA);

	CORE_EH_PRINT(("device %d request %p [0x%x] time out "\
		"on slot(0x%x) while active=0x%x.\n", \
		base->id, err_req, err_req->Cdb[0],
		ctx->slot, tmp));
	MV_DumpRequest(err_req, MV_FALSE);
	
	err_ctx->timer_id = NO_CURRENT_TIMER;
	if (!List_Empty(&err_ctx->sent_req_list)) {
		first_req = LIST_ENTRY(
			(&err_ctx->sent_req_list)->next, MV_Request, Queue_Pointer);
		MV_ASSERT(first_req == err_req);
	} else {
		CORE_DPRINT(("no outstanding req on dev %d?\n", base->id));
		MV_DASSERT(MV_FALSE);
		return;
	}

	io_chip_handle_cmpl_queue_int(root);

	if (List_Empty(&err_ctx->sent_req_list)) { 
		CORE_PRINT(("sent_req_list empty\n"));
		CORE_PRINT(("device %d missed one interrupt for req %p.\n", \
			base->id, err_req));
		goto end_point;
	} else {
		first_req = LIST_ENTRY(
			(&err_ctx->sent_req_list)->next, MV_Request, Queue_Pointer);
		if (first_req != err_req) {
			CORE_PRINT(("device %d missed one interrupt for req %p.\n", \
				base->id, err_req));
			goto end_point;
		}
	}

	if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
		dev = (domain_device *)base;
		if (dev->state == DEVICE_STATE_INIT_DONE) {
			core_generate_event(root->core, EVT_ID_HD_TIMEOUT, base->id,
				SEVERITY_WARNING, 0, NULL,0);
                }
        }

	MV_ASSERT(base->id == err_req->Device_Id);
	MV_ASSERT(base == get_device_by_id(root->lib_dev, err_req->Device_Id));

	/* reset the error req slot */
	prot_reset_slot(root, base, ctx->slot, err_req);

	MV_ASSERT(err_req->Queue_Pointer.next == NULL);
	MV_ASSERT(err_req->Queue_Pointer.prev == NULL);

	if (CORE_IS_EH_REQ(ctx) || CORE_IS_INIT_REQ(ctx)) {
		if (CORE_IS_EH_REQ(ctx)) {
                        err_req->Scsi_Status = REQ_STATUS_TIMEOUT;
		} else {
			core_extension *core=(core_extension *)root->core;
			if(core->spin_up_group){
				if(base->type == BASE_TYPE_DOMAIN_DEVICE){
					dev = (domain_device *)base;
					if(dev->state != DEVICE_STATE_INIT_DONE){
						dev->state = DEVICE_STATE_INIT_DONE;
						dev->status |= DEVICE_STATUS_NO_DEVICE;
						}
					}
			}
			err_req->Scsi_Status = REQ_STATUS_TIMEOUT;
		}
		core_queue_completed_req(root->core, err_req);
	} else {
		err_req->Scsi_Status = REQ_STATUS_TIMEOUT;
		CORE_PRINT(("device %d prepare to do EH for timeout request %p.\n",base->id, err_req));
		core_queue_error_req(root, err_req, MV_TRUE);
	}

end_point:
	core_push_queues(root->core);
}
MV_VOID core_clean_error_queue(core_extension *core)
{
	MV_Request *req;
	domain_base *base;
	pl_root *root;
	
	while (!Counted_List_Empty(&core->error_queue)) {
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->error_queue, MV_Request, Queue_Pointer);
		base = (domain_base *)get_device_by_id(&core->lib_dev, req->Device_Id);
		root = base->root;
		
		req->Scsi_Status = REQ_STATUS_ERROR;
		core_complete_error_req(root, req, req->Scsi_Status);
	}
}
MV_VOID core_handle_error_queue(core_extension *core)
{
	MV_Request *req;
	core_context *ctx;
	domain_base *base;
	domain_device *device;
	MV_BOOLEAN ret = MV_FALSE;
	MV_Request *old_req = NULL;
	domain_port *port;
	pl_root *root;
	MV_U8 i;
	MV_U16 k, running_count = 0;
	MV_U8 j;
	
	for ( j=core->chip_info->start_host; j<(core->chip_info->start_host + core->chip_info->n_host); j++) {
		root = &core->roots[j];
		for (k = 0; k < root->slot_count_support; k++) {
			req = root->running_req[k];
			if (req == NULL) continue;
			running_count++;
		}
	}
	if (running_count)
		return;

	while (!Counted_List_Empty(&core->error_queue)) {
		req = (MV_Request *)Counted_List_GetFirstEntry(
			&core->error_queue, MV_Request, Queue_Pointer);
		if (req == old_req) {
			/* has gone through the list */
			Counted_List_Add(&req->Queue_Pointer, &core->error_queue);
			return;
		}

		ctx = (core_context *)req->Context[MODULE_CORE];
		base = (domain_base *)get_device_by_id(&core->lib_dev, req->Device_Id);
		port = base->port;
		root = base->root;

		for (i = 0; i < root->phy_num; i++) {
			port = &root->ports[i];
			if (port_has_init_req(root, port)) {
				Counted_List_Add(&req->Queue_Pointer, &core->error_queue);
				return;
			}
		}
		
		if(port_is_running_error_handling(root, base->port)) {
				Counted_List_AddTail(&req->Queue_Pointer, &core->error_queue);
				if (old_req == NULL) old_req = req;
				continue;
		}
		
		if ((base->err_ctx.state == BASE_STATE_ERROR_HANDLED) 
			|| (base->port->base.err_ctx.state == BASE_STATE_ERROR_HANDLED)){
			core_push_running_request_back(root, req);
			if (--base->err_ctx.error_count == 0)
				base->err_ctx.state = BASE_STATE_NONE;
			if (!port_has_error_req(root, base->port))
				base->port->base.err_ctx.state = BASE_STATE_NONE;
			continue;
		}

		if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
			device = (domain_device *)base;
            
			if (IS_SSP(device) && (base->outstanding_req >= 1) ) {
				Counted_List_AddTail(&req->Queue_Pointer, &core->error_queue);
				if (old_req == NULL) old_req = req;
				continue;
			}
		}

		if (base->handler->error_handler != NULL) {
			base->err_ctx.state = BASE_STATE_ERROR_HANDLING;
			ret = base->handler->error_handler(base, req);

			if (ret == MV_FALSE) {
				base->err_ctx.state = BASE_STATE_NONE;
				Counted_List_AddTail(&req->Queue_Pointer, &core->error_queue);
				if (old_req == NULL) old_req = req;
			}
		} else {
			MV_ASSERT(base->err_ctx.state == BASE_STATE_NONE);
			if (req->Scsi_Status == REQ_STATUS_TIMEOUT) {
				req->Scsi_Status = REQ_STATUS_ERROR;
			}
			core_complete_error_req(root, req, req->Scsi_Status);
		}
	}
}

MV_U32 pal_abort_device_running_req(pl_root *root, domain_base *base)
{
	MV_U16 i;
	MV_Request *req;
	core_context *ctx;
	MV_U32 abort_count = 0;

	/* clean cmpl queue in case request is already finished */
	io_chip_handle_cmpl_queue_int(root);

	for (i = 0; i < root->slot_count_support; i++) {
		req = root->running_req[i];

		if (req == NULL) continue;
		if (base->id != req->Device_Id) continue;

		/* make sure this device should still exist */
		MV_ASSERT(base == get_device_by_id(root->lib_dev, req->Device_Id));

		ctx = (core_context *)req->Context[MODULE_CORE];
		prot_reset_slot(root, base, i, req);
		MV_DASSERT(ctx->slot == i);

		/* it has been removed from the sent list in prot_reset_slot */
		MV_DASSERT(req->Queue_Pointer.next == NULL);
		MV_DASSERT(req->Queue_Pointer.prev == NULL);
		/* sg_wrapper should have been released in prot_reset_slot*/
		MV_DASSERT(ctx->sg_wrapper == NULL);

		core_push_running_request_back(root, req);
		abort_count++;
	}

	MV_ASSERT(List_Empty(&(base->err_ctx.sent_req_list)));
	CORE_EH_PRINT(("%d requests aborted.\n", abort_count));

	/* clear register set, if any */
	if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
		domain_device *device = (domain_device *)base;

		if (device->register_set != NO_REGISTER_SET) {
			MV_DASSERT(device->base.outstanding_req == 0);
			sata_free_register_set(root, device->register_set);
			device->register_set = NO_REGISTER_SET;
		}
	}

	return abort_count;
}

extern void core_handle_waiting_queue(core_extension *core);
MV_U32 pal_abort_port_running_req(pl_root *root, domain_port *port)
{
	MV_U16 i;
	MV_U32 abort_count = 0;
	domain_base *req_base;
	domain_device *device;
	MV_Request *req;
	core_context *ctx;

	/* clean cmpl queue in case request is already finished */
	CORE_EH_PRINT(("abort: cmpl queue cleaned, pointer 0x%x\n", root->last_cmpl_q));
	io_chip_handle_cmpl_queue_int(root);		

	for (i = 0; i < root->slot_count_support; i++) {
		req = root->running_req[i];

		if (req == NULL) continue;

		req_base = get_device_by_id(root->lib_dev, req->Device_Id);
		MV_ASSERT(req_base != NULL);
		if (req_base->port != port) continue;

		ctx = (core_context *)req->Context[MODULE_CORE];
		prot_reset_slot(root, req_base, i, req);

		if (req_base->type == BASE_TYPE_DOMAIN_DEVICE) {
			domain_device *dev = (domain_device *)req_base;
			if (dev->register_set != NO_REGISTER_SET)
				CORE_EH_PRINT(("slot's reg set is %d\n", dev->register_set));
		}

		MV_DASSERT(ctx->slot == i);
		MV_DASSERT(req->Queue_Pointer.next == NULL);
		MV_DASSERT(req->Queue_Pointer.prev == NULL);
		MV_DASSERT(ctx->sg_wrapper == NULL);

		core_push_running_request_back(root, req);
		abort_count++;
	}

	/* clear register set, if any */
	LIST_FOR_EACH_ENTRY_TYPE(device, &port->device_list, domain_device,
		base.queue_pointer) {
		if (device->register_set != NO_REGISTER_SET) {
			MV_DASSERT(device->base.outstanding_req == 0);
			CORE_EH_PRINT(("freeing reg set %d\n", device->register_set));
			sata_free_register_set(root, device->register_set);
			device->register_set = NO_REGISTER_SET;
		}
	}
	CORE_EH_PRINT(("end of function, reg set enable 0 = 0x%x\n",
		READ_REGISTER_SET_ENABLE(root, 0)));
	CORE_EH_PRINT(("end of function, cmpl queue pointer 0x%x\n", root->last_cmpl_q));

	CORE_EH_PRINT(("%d requests aborted.\n", abort_count));
        return abort_count;
}

void pal_clean_req_callback(pl_root *root, MV_Request *req)
{
	domain_base *base = (domain_base *)get_device_by_id(
		root->lib_dev, req->Device_Id);

	if (base)
		CORE_EH_PRINT(("base id %d clean up done, type %d\n", base->id, base->type));
	else
		MV_ASSERT(MV_FALSE);
}

void pal_clean_expander_outstanding_req(pl_root *root, domain_expander *exp)
{
	domain_device *device;
	MV_Request *req=NULL;
	core_context *ctx;

	LIST_FOR_EACH_ENTRY_TYPE(device, &exp->device_list, domain_device,
		base.exp_queue_pointer) {
		if (IS_SSP(device)) {
			req = sas_make_task_req(root, device, NULL,
				TMF_ABORT_TASK_SET, (MV_ReqCompletion)pal_clean_req_callback);
		} else if (IS_STP(device)) {
			req = smp_make_phy_control_req(device, LINK_RESET,
				pal_clean_req_callback);
		}
		if (!req) {
			CORE_DPRINT(("clean up ran out of reqs\n"));
			return;
		} else {
			CORE_DPRINT(("sending clean up req to device %d\n",
				device->base.id));
		}
		core_queue_eh_req(root, req);
	}
}

MV_U32 pal_complete_device_waiting_queue(pl_root *root, domain_base *base,
	MV_U8 scsi_status, Counted_List_Head *queue)
{
	MV_U32 count;
	MV_Request *req;
	MV_U32 cmpl_count = 0;
	core_extension *core = (core_extension *)root->core;

	/* Return the waiting request related to this device. */
	count = Counted_List_GetCount(queue, MV_FALSE);

	while (count > 0) {
		MV_ASSERT(!Counted_List_Empty(queue));

		req = (PMV_Request)Counted_List_GetFirstEntry(
			queue, MV_Request, Queue_Pointer);

		if (base->id != req->Device_Id) {
			Counted_List_AddTail(&req->Queue_Pointer, queue);
			count--;
			continue;
		}

		MV_ASSERT(base == get_device_by_id(root->lib_dev, req->Device_Id));

		req->Scsi_Status = scsi_status;
		core_queue_completed_req(core, req);
		cmpl_count++;
		count--;
	}

	return cmpl_count;
}

MV_U32 core_complete_device_waiting_queue(pl_root *root, domain_base *base,
	MV_U8 scsi_status)
{
	core_extension *core = (core_extension *)root->core;
	MV_U32 cmpl_count = 0;
	MV_U32 error_count = 0;

	/* push completion queue to clear up the aborted requests */
	core_complete_requests(root->core); 

	cmpl_count = pal_complete_device_waiting_queue(
		root, base, scsi_status, &core->high_priority_queue);

	cmpl_count += pal_complete_device_waiting_queue(
		root, base, scsi_status, &core->waiting_queue);

	error_count = pal_complete_device_waiting_queue(
			root, base, scsi_status, &core->error_queue);
	CORE_EH_PRINT(("completed %d error request.\n", error_count));
	cmpl_count += error_count;

	MV_ASSERT(scsi_status == REQ_STATUS_NO_DEVICE);

	/* push completion queue to clear up the aborted requests */
	core_complete_requests(root->core);

	if (base->err_ctx.error_req) {
		core_complete_error_req(root, base->err_ctx.error_req, scsi_status);
		base->err_ctx.error_req = NULL;
                CORE_EH_PRINT(("completed one outstanding error request.\n"));

                /* complete this request */
                core_complete_requests(root->core);
	}

	base->err_ctx.eh_type = EH_TYPE_NONE;
	base->err_ctx.state = BASE_STATE_NONE;
	base->err_ctx.error_count = 0;

	MV_ASSERT(List_Empty(&base->err_ctx.sent_req_list));

	CORE_EH_PRINT(("%d requests completed with status 0x%x.\n", \
		cmpl_count, scsi_status));
	return cmpl_count;
}

/* after calling this function, domain_expander *exp is not valid any more
 * all the devices under this expander got set down too */
void pal_set_down_expander(pl_root *root, domain_expander *exp)
{
	domain_port *port = exp->base.port;
	MV_BOOLEAN attached;
	domain_base *tmp;
	domain_expander *parent_exp, *tmp_exp;
	domain_device *dev;

        CORE_EH_PRINT(("begin to set down expander %d\n", exp->base.id));
        MV_ASSERT(get_device_by_id(root->lib_dev, exp->base.id) != NULL);
	/* free all devices/expanders attached to this expander */
	while (!List_Empty(&exp->device_list)) {
		dev = List_GetFirstEntry(&exp->device_list, domain_device,
			base.exp_queue_pointer);
		pal_set_down_disk(root, dev, MV_TRUE);
		exp->device_count--;
	}

	while (!List_Empty(&exp->expander_list)) {
		tmp_exp = List_GetFirstEntry(&exp->expander_list, domain_expander,
			base.exp_queue_pointer);
		pal_set_down_expander(root, tmp_exp);
		exp->expander_count--;
	}

	if(exp->enclosure){
		domain_enclosure *associate_enc = exp->enclosure;
		domain_expander *tmp_exp;
		attached = MV_FALSE;
		LIST_FOR_EACH_ENTRY_TYPE(
			tmp_exp, &associate_enc->expander_list, domain_expander, enclosure_queue_pointer){
			if(tmp_exp == exp)
				attached = MV_TRUE;
		}
		if(attached == MV_TRUE){
			List_Del(&exp->enclosure_queue_pointer);
			exp->enclosure = NULL;
		}
		if(List_Empty(&associate_enc->expander_list))
			pal_set_down_enclosure(root,associate_enc);
	}

	pal_abort_device_running_req(root, &exp->base);
	core_complete_device_waiting_queue(root, &exp->base, REQ_STATUS_NO_DEVICE);
        MV_ASSERT(get_device_by_id(root->lib_dev, exp->base.id) != NULL);

	exp->status = 0;
	exp->state = EXP_STATE_IDLE;

	/* check whether it has been attached to the port */
	attached = MV_FALSE;
	LIST_FOR_EACH_ENTRY_TYPE(
		tmp, &port->expander_list, domain_base, queue_pointer) {
		if (tmp == (domain_base *)exp) attached = MV_TRUE;
	}
	/* can be FALSE if called by pal_set_down_port */
	if (attached) {
		List_Del(&exp->base.queue_pointer);
		port->expander_count--;
	}

	/* check whether it has been attached to the other lists */
	attached = MV_FALSE;
	LIST_FOR_EACH_ENTRY_TYPE(
		tmp, &port->current_tier, domain_base, queue_pointer) {
		if (tmp == (domain_base *)exp) attached = MV_TRUE;
	}
	if (attached)
		List_Del(&exp->base.queue_pointer);

	attached = MV_FALSE;
	LIST_FOR_EACH_ENTRY_TYPE(
		tmp, &port->next_tier, domain_base, queue_pointer) {
		if (tmp == (domain_base *)exp) attached = MV_TRUE;
	}
	if (attached)
		List_Del(&exp->base.queue_pointer);

	/* check whether it has been attached to an expander */
	MV_ASSERT(exp->base.parent != NULL);
	if (exp->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER) {
		attached = MV_FALSE;
		parent_exp = (domain_expander *)exp->base.parent;
		LIST_FOR_EACH_ENTRY_TYPE(
			tmp, &parent_exp->expander_list, domain_base, exp_queue_pointer) {
			if (tmp == (domain_base *)exp) attached = MV_TRUE;
		}
		/* attached can be false */
		if (attached) {
			List_Del(&exp->base.exp_queue_pointer);
			parent_exp->expander_count--;
		}
	}

	core_clean_init_queue_entry(root, &exp->base);

	MV_ASSERT(List_Empty(&exp->device_list));
	MV_ASSERT(List_Empty(&exp->expander_list));
	MV_ASSERT(exp->device_count == 0);
	MV_ASSERT(exp->expander_count == 0);

	core_generate_event(root->core, EVT_ID_EXPANDER_PLUG_OUT, exp->base.id,
			SEVERITY_INFO, 0, NULL ,0);
	CORE_EH_PRINT(("set down expander %d is done\n", exp->base.id));

	free_expander_obj(root, root->lib_rsrc, exp);
}

/* Set disk status to offline, we still have the data structure and link to it. */
void pal_set_disk_offline(pl_root *root, domain_device *dev, MV_BOOLEAN notify_os)
{
	MV_U8 state = dev->state;

       MV_ASSERT(get_device_by_id(root->lib_dev, dev->base.id) != NULL);
	pal_abort_device_running_req(root, &dev->base);
	core_complete_device_waiting_queue(root, &dev->base, REQ_STATUS_NO_DEVICE);
       MV_ASSERT(get_device_by_id(root->lib_dev, dev->base.id) != NULL);

	dev->status = DEVICE_STATUS_NO_DEVICE;
	dev->state = DEVICE_STATE_INIT_DONE;

	core_clean_init_queue_entry(root, &dev->base);

	if (dev->register_set != NO_REGISTER_SET) {
		CORE_EH_PRINT(("free device %d register set %d\n", \
                        dev->base.id, dev->register_set));
		MV_DASSERT(dev->base.outstanding_req == 0);
		sata_free_register_set(root, dev->register_set);
		dev->register_set = NO_REGISTER_SET;
	}
	
        if(notify_os) {
		core_notify_device_hotplug(root, MV_FALSE, dev->base.id,
			state == DEVICE_STATE_INIT_DONE);
        }

	Core_Change_LED(root->core, dev->base.id,LED_FLAG_OFF_ALL);

	CORE_EH_PRINT(("set down disk %d\n", dev->base.id));

}

void pal_set_down_error_disk(pl_root *root, domain_device *dev, MV_BOOLEAN notify_os)
{
        /* if it's direct attached disk, remove data structure either */
        if (!IS_BEHIND_EXP(dev)) {
                pal_set_down_disk(root, dev, notify_os);
        } else {
                pal_set_disk_offline(root, dev, notify_os);
        }

}

void pal_set_down_multi_lun_disk(pl_root *root, domain_device *dev, MV_BOOLEAN notify_os)
{
	domain_port *port = dev->base.port;
	MV_BOOLEAN attached;
	domain_base *tmp;
	domain_expander *exp;
	domain_pm *pm;
	MV_U32 i;
	
	pal_set_disk_offline(root, dev, notify_os);
	/* check whether it has been attached to the port */
	attached = MV_FALSE;
	LIST_FOR_EACH_ENTRY_TYPE(
		tmp, &port->device_list, domain_base, queue_pointer) {
		if (tmp == (domain_base *)dev) attached = MV_TRUE;
	}
	/* can be false if called by pal_set_down_port */
	if (attached) {
		List_Del(&dev->base.queue_pointer);
		port->device_count--;
	}

	/* check whether it has been attached to the expander */
	MV_ASSERT(dev->base.parent != NULL);
	if (dev->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER) {
		attached = MV_FALSE;
		exp = (domain_expander *)dev->base.parent;
		LIST_FOR_EACH_ENTRY_TYPE(
			tmp, &exp->device_list, domain_base, exp_queue_pointer) {
			if (tmp == (domain_base *)dev) attached = MV_TRUE;
		}
		/* if set down by pal_set_down_expander, maybe not attached anymore */
		if (attached) {
			List_Del(&dev->base.exp_queue_pointer);
			exp->device_count--;
		}
	}

	/* check whether it is attached to the PM */
	if (dev->base.parent->type == BASE_TYPE_DOMAIN_PM) {
		attached = MV_FALSE;
		pm = (domain_pm *)dev->base.parent;
		for (i = 0; i < CORE_MAX_DEVICE_PER_PM; i++) {
			if (pm->devices[i] == dev) {
				attached = MV_TRUE;
				pm->devices[i] = NULL;
			}
		}
		MV_ASSERT(attached == MV_TRUE);
	}


	free_device_obj(root, root->lib_rsrc, dev);
}

/* after calling this function, domain_device *dev is not valid any more
 * no matter direct attached or under expander
 */
void pal_set_down_disk(pl_root *root, domain_device *dev, MV_BOOLEAN notify_os)
{
	domain_port *port = dev->base.port;
	MV_BOOLEAN attached;
	domain_base *tmp;
	domain_expander *exp;
	domain_pm *pm;
	MV_U32 i;

	core_extension *core = root->core;
	lib_device_mgr *lib_dev = &core->lib_dev;
	domain_device *dev_multi_lun;
         if((dev->base.multi_lun == MV_TRUE)&& (dev->base.LUN==0)){
		CORE_EH_PRINT(("set down multi lun disk %d\n", dev->base.id));	
	        	for(i=0; i<MAX_ID; i++) {
			domain_base *base;
			base = lib_dev->device_map[i];
			if (base && ( base->TargetID== dev->base.TargetID) && ( base->LUN!=0)){
				dev_multi_lun = (domain_device *)base;
                                   pal_set_down_multi_lun_disk(root, dev_multi_lun, notify_os);
			}				
		} 
         }else if(dev->base.LUN > 0){
         	pal_set_down_multi_lun_disk(root, dev, notify_os);
		return;
         }		

	pal_set_disk_offline(root, dev, notify_os);
	/* check whether it has been attached to the port */
	attached = MV_FALSE;
	LIST_FOR_EACH_ENTRY_TYPE(
		tmp, &port->device_list, domain_base, queue_pointer) {
		if (tmp == (domain_base *)dev) attached = MV_TRUE;
	}
	/* can be false if called by pal_set_down_port */
	if (attached) {
		List_Del(&dev->base.queue_pointer);
		port->device_count--;
	}

	/* check whether it has been attached to the expander */
	MV_ASSERT(dev->base.parent != NULL);
	if (dev->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER) {
		attached = MV_FALSE;
		exp = (domain_expander *)dev->base.parent;
		LIST_FOR_EACH_ENTRY_TYPE(
			tmp, &exp->device_list, domain_base, exp_queue_pointer) {
			if (tmp == (domain_base *)dev) attached = MV_TRUE;
		}
		/* if set down by pal_set_down_expander, maybe not attached anymore */
		if (attached) {
			List_Del(&dev->base.exp_queue_pointer);
			exp->device_count--;
		}
	}

	/* check whether it is attached to the PM */
	if (dev->base.parent->type == BASE_TYPE_DOMAIN_PM) {
		attached = MV_FALSE;
		pm = (domain_pm *)dev->base.parent;
		for (i = 0; i < CORE_MAX_DEVICE_PER_PM; i++) {
			if (pm->devices[i] == dev) {
				attached = MV_TRUE;
				pm->devices[i] = NULL;
			}
		}
		MV_ASSERT(attached == MV_TRUE);
	}

	remove_target_map(root->lib_dev->target_id_map, dev->base.TargetID, MV_MAX_TARGET_NUMBER);
	free_device_obj(root, root->lib_rsrc, dev);
}

MV_U8 pal_check_disk_exist(void *ext, MV_U16 device_target_id, MV_U16 device_lun)
{
	core_extension *core = (core_extension *)HBA_GetModuleExtension(ext, MODULE_CORE);
	pl_root *root = NULL;
	domain_base *base = NULL;
	
	base = get_device_by_id(&core->lib_dev, get_id_by_targetid_lun(core, device_target_id, device_lun));
	if (base == NULL)
		return 0;
	
	return 1;
}
MV_U8 pal_set_down_disk_from_upper(void *ext, MV_U16 device_target_id, MV_U16 device_lun)
{
	core_extension *core = (core_extension *)HBA_GetModuleExtension(ext, MODULE_CORE);
	pl_root *root = NULL;
	domain_base *base = NULL;
	
	base = get_device_by_id(&core->lib_dev, get_id_by_targetid_lun(core, device_target_id, device_lun));
	if (base == NULL)
		return -1;
	
	root = (pl_root *)base->root;
	if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
		domain_device *device = (domain_device *)base;
		if(device->state != DEVICE_STATE_INIT_DONE) {
			CORE_DPRINT(("device %d is bad disk \n",base->id));
			return 0;
		}
	}
	
	if (base->type == BASE_TYPE_DOMAIN_DEVICE)
		pal_set_down_disk(root, (domain_device *)base, MV_FALSE);
	else if (base->type == BASE_TYPE_DOMAIN_ENCLOSURE)
		pal_set_down_enclosure(root, (domain_enclosure *)base);
	else
		MV_ASSERT(0);
	CORE_DPRINT(("upper layer notify to set down device %d.\n", device_target_id));
	return 0;
}

void pal_set_down_enclosure(pl_root *root, domain_enclosure *enc)
{
	MV_BOOLEAN attached;
	domain_port *port = enc->base.port;
	domain_expander *tmp_exp;
	domain_device *tmp_dev;
	domain_base *tmp_base;
	MV_U8 dev_id;
	MV_U32 i;

	/*set all associated expander->enclosure = NULL*/
	if(!List_Empty(&enc->expander_list)){
		tmp_exp = List_GetFirstEntry(&enc->expander_list, domain_expander, enclosure_queue_pointer);
		MV_ASSERT(tmp_exp->enclosure == enc);
		tmp_exp->enclosure = NULL;
	}

	/* clean up the enclosure pointer for the attached device */
	for (dev_id = 0; dev_id < MAX_ID; dev_id++) {
		tmp_base = get_device_by_id(root->lib_dev, dev_id);
		if (tmp_base == NULL || tmp_base->type != BASE_TYPE_DOMAIN_DEVICE)
			continue;
		tmp_dev = (domain_device*)tmp_base;
		if (tmp_dev->enclosure == enc)
			tmp_dev->enclosure = NULL;
	}

	pal_abort_device_running_req(root, &enc->base);
	core_complete_device_waiting_queue(root, &enc->base, REQ_STATUS_NO_DEVICE);
       core_clean_init_queue_entry(root, &enc->base);

	enc->status = 0;
	enc->state = ENCLOSURE_INQUIRY_DONE;

	core_notify_device_hotplug(root, MV_FALSE, enc->base.id, MV_TRUE);
	CORE_EH_PRINT(("set down enclosure %d\n", enc->base.id));
	
	remove_target_map(root->lib_dev->target_id_map, enc->base.TargetID, MV_MAX_TARGET_NUMBER);
	free_enclosure_obj(root, root->lib_rsrc, enc);
}

void pal_set_down_pm(pl_root *root, domain_pm *pm, MV_BOOLEAN notify_os)
{
	domain_port *port = pm->base.port;
	MV_U8 i;

	pal_abort_device_running_req(root, &pm->base);
	core_complete_device_waiting_queue(root, &pm->base, REQ_STATUS_NO_DEVICE);

	for (i=0; i<CORE_MAX_DEVICE_PER_PM; i++) {
		if (pm->devices[i])
			pal_set_down_disk(root, pm->devices[i], MV_TRUE);
	}
       core_clean_init_queue_entry(root, &pm->base);

	pm->status = 0;
	pm->state = PM_STATE_RESET_DONE;

	CORE_EH_PRINT(("set down pm %d\n", pm->base.id));

	if (pm->register_set != NO_REGISTER_SET) {
		MV_DASSERT(pm->base.outstanding_req == 0);
		sata_free_register_set(root, pm->register_set);
		pm->register_set = NO_REGISTER_SET;
	}

	MV_ASSERT(port->pm == pm);
	port->pm = NULL;
	free_pm_obj(root, root->lib_rsrc, pm);
}

/* after calling this function, all devices under this port is invalid.
 * dont access them after that. */
void pal_set_down_port(pl_root *root, domain_port *port)
{
	domain_base *base;
	domain_device *dev;
	domain_expander *exp;
	domain_pm   *pm;

	MV_ASSERT(List_Empty(&port->current_tier));
	MV_ASSERT(List_Empty(&port->next_tier));

	/* expanders & their devices */
	while(!List_Empty(&port->expander_list)) {
		exp = (domain_expander *)List_GetFirstEntry(&port->expander_list,
			domain_expander, base.queue_pointer);
		pal_set_down_expander(root, exp);
		port->expander_count--;
	}

	/* direct attached, for those without targets */
	while (!List_Empty(&port->device_list)) {
		base = List_GetFirstEntry(
			&port->device_list, domain_base, queue_pointer);
		MV_ASSERT(base->type == BASE_TYPE_DOMAIN_DEVICE);
		dev = (domain_device *)base;
		pal_set_down_disk(root, dev, MV_TRUE);
		port->device_count--;
	}

	/* set down pm if any */
	pm = port->pm;
	if (pm != NULL) {
		pal_set_down_pm(root, pm, MV_TRUE);
		port->pm = NULL;
	}

	MV_ASSERT(port->device_count == 0);
	MV_ASSERT(port->expander_count == 0);
	MV_ASSERT(List_Empty(&port->expander_list));
	MV_ASSERT(List_Empty(&port->device_list));
	MV_ASSERT(port->pm == NULL);

	CORE_EH_PRINT(("set down port %d\n", port->base.id));
}

/* retry we allocate internal request to do the retry*/
MV_Request *core_eh_retry_org_req(pl_root *root, MV_Request *req,
	MV_ReqCompletion func)
{
	MV_Request *eh_req;
	core_context *ctx;

	/* dont allocate any data buffer, use the original */
	eh_req = get_intl_req_resource(root, 0);
	if (eh_req == NULL) return NULL;
	ctx = eh_req->Context[MODULE_CORE];

	eh_req->Device_Id = req->Device_Id;
	eh_req->Data_Buffer = req->Data_Buffer;
	eh_req->Data_Transfer_Length = req->Data_Transfer_Length;

	MV_CopyMemory(eh_req->Cdb, req->Cdb, MAX_CDB_SIZE);
	eh_req->Cmd_Flag = req->Cmd_Flag;
	eh_req->LBA.value = req->LBA.value;
	eh_req->Sector_Count = req->Sector_Count;

	MV_DASSERT(req->Data_Transfer_Length == req->SG_Table.Byte_Count);
	/* refer to the original SG.
	 * actually cover the whole buffer because offset=0, size=whole size */
	if (req->Data_Transfer_Length > 0) {
		MV_CopyPartialSGTable(
			&eh_req->SG_Table,
			&req->SG_Table,
			0, /* offset */
			req->SG_Table.Byte_Count /* size */
		);
	}
	MV_DASSERT(eh_req->Data_Transfer_Length == eh_req->SG_Table.Byte_Count);

	ctx->type = CORE_CONTEXT_TYPE_ORG_REQ;
	ctx->u.org.org_req = req;
	ctx->req_type = CORE_REQ_TYPE_RETRY;

	eh_req->Scsi_Status = REQ_STATUS_PENDING;
	eh_req->Completion = func;

	return eh_req;
}
MV_BOOLEAN port_is_running_error_handling(pl_root *root, domain_port *port)
{
	domain_base *base;

	LIST_FOR_EACH_ENTRY_TYPE(
		base, &port->device_list, domain_base, queue_pointer) {
		if (base->err_ctx.state == BASE_STATE_ERROR_HANDLING)
			return MV_TRUE;
	}
	

	LIST_FOR_EACH_ENTRY_TYPE(
		base, &port->expander_list, domain_base, queue_pointer) {
		if (base->err_ctx.state == BASE_STATE_ERROR_HANDLING)
			return MV_TRUE;
	}

	return MV_FALSE;
}
MV_BOOLEAN port_has_error_req(pl_root *root, domain_port *port)
{
	domain_base *base;

	LIST_FOR_EACH_ENTRY_TYPE(
		base, &port->device_list, domain_base, queue_pointer) {
		if (base->err_ctx.error_count > 0) return MV_TRUE;
	}

	LIST_FOR_EACH_ENTRY_TYPE(
		base, &port->expander_list, domain_base, queue_pointer) {
		if (base->err_ctx.error_count > 0) return MV_TRUE;
	}

	if (port->pm &&
		(port->pm->base.err_ctx.error_count > 0)) {
		return MV_TRUE;
	}

	return MV_FALSE;
}

MV_BOOLEAN port_has_init_req(pl_root *root, domain_port *port)
{
	if (port->init_count != 0)
		return MV_TRUE;
	else
		return MV_FALSE;
}

MV_BOOLEAN device_has_error_req(pl_root *root, domain_base *base)
{
	if (base->err_ctx.error_count > 0)
		return MV_TRUE;
	else
		return MV_FALSE;
}

MV_Request *core_make_device_reset_req(pl_root *root, domain_port *port,
	domain_device *dev, MV_ReqCompletion func)
{

	MV_Request *req;

	/* allocate resource */
	req = get_intl_req_resource(root, 0);
	if (req == NULL) return NULL;

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_RESET_DEVICE;
	req->Cdb[3] = port->phy_map;
	req->Cmd_Flag = 0;
	req->Device_Id = dev->base.id;
	req->Completion = func;

	return req;
}

MV_Request *core_make_port_reset_req(pl_root *root, domain_port *port,
	MV_PVOID base, MV_ReqCompletion func)
{
	MV_Request *req;

	/* allocate resource */
	req = get_intl_req_resource(root, 0);
	if (req == NULL) return NULL;

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_RESET_PORT;
	req->Cdb[3] = port->phy_map;
	req->Cmd_Flag = 0;
	req->Device_Id = ((domain_base *)base)->id;
	req->Completion = func;

	return req;
}

void pal_abort_running_req_for_async_sdb_fis(pl_root *root, domain_device *dev)
{
	MV_Request *req;
	MV_U8 state = dev->state;

        MV_ASSERT(get_device_by_id(root->lib_dev, dev->base.id) != NULL);
	pal_abort_device_running_req(root, &dev->base);
	core_complete_device_waiting_queue(root, &dev->base, REQ_STATUS_NO_DEVICE);
        MV_ASSERT(get_device_by_id(root->lib_dev, dev->base.id) != NULL);


	dev->status &= ~DEVICE_STATUS_FUNCTIONAL;
	dev->status |= DEVICE_STATUS_WAIT_ASYNC;
	dev->state = DEVICE_STATE_INIT_DONE;

	if (dev->register_set != NO_REGISTER_SET) {
		CORE_EH_PRINT(("free device %d register set %d\n", \
                        dev->base.id, dev->register_set));
		MV_DASSERT(dev->base.outstanding_req == 0);
		sata_free_register_set(root, dev->register_set);
		dev->register_set = NO_REGISTER_SET;
	}

	CORE_PRINT(("abort disk %d running req for async sdb fis.\n", dev->base.id));

}

