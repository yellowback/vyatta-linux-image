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

#include "core_sas.h"
#include "core_hal.h"
#include "core_error.h"
#include "core_util.h"
#include "core_expander.h"

MV_Request *sas_make_task_req(pl_root *root, domain_device *dev,
	MV_Request *org_req, MV_U8 task_function, MV_ReqCompletion func);
static void ssp_error_handling_callback(pl_root *root, MV_Request *req);
MV_VOID ssp_wait_hard_reset(MV_PVOID dev_p, MV_PVOID tmp);

/*
 * two cases will call this function
 * 1. ssp_error_handler: original error request comes here.
 * 2. ssp_error_handling_callback:
 *    a. it's the error handling(eh) request either success or fail
 *    b. retried req will come here only if fails.
 *       if success, ssp_error_handling_callback has returned it.
 */
static MV_BOOLEAN ssp_timeout_state_machine(pl_root *root, 
        domain_device *dev, MV_BOOLEAN success)
{
	struct _error_context *err_ctx = &(dev->base.err_ctx);
	domain_port *port = (domain_port *)dev->base.port;
	domain_expander *tmp_exp;
	MV_Request *eh_req = NULL;
	MV_Request *org_req = err_ctx->error_req;
	MV_U8 i;

	MV_ASSERT(err_ctx->eh_type == EH_TYPE_TIMEOUT);
	MV_ASSERT(org_req != NULL);

	CORE_EH_PRINT(("device %d state %d success 0x%x\n",\
		dev->base.id, err_ctx->eh_state, success));

	/* for all following state, if eh request returns successfully,
	 * retry the original request
	 * if retry req success, already returned org by ssp_error_handling_callback */
	if (success == MV_TRUE) {
		if ((org_req->Org_Req_Scmd) 
			&& ((struct scsi_cmnd *)org_req->Org_Req_Scmd)->allowed
		) {
			eh_req = core_eh_retry_org_req(
					root, org_req, 
	                                (MV_ReqCompletion)ssp_error_handling_callback);
			if (eh_req == NULL) return MV_FALSE;
			core_queue_eh_req(root, eh_req);
		} else
			core_complete_error_req(root, org_req, REQ_STATUS_SUCCESS);
		
		return MV_TRUE;
	}

	switch (err_ctx->eh_state) {
	case SAS_EH_TIMEOUT_STATE_NONE:
		err_ctx->timeout_count++;

		if (((core_extension *)(root->core))->revision_id != VANIR_C2_REV) {
			pal_abort_port_running_req(root, port);

			core_sleep_millisecond(root->core, 10);

			for (i = 0; i < dev->base.root->phy_num; i++) {
				if (dev->base.root->phy[i].port == port) {
					WRITE_PORT_PHY_CONTROL(root,
						(&dev->base.root->phy[i]),
						(SCTRL_STP_LINK_LAYER_RESET | SCTRL_SSP_LINK_LAYER_RESET));
				}
			}

			core_sleep_millisecond(root->core, 50);

			LIST_FOR_EACH_ENTRY_TYPE(tmp_exp, &port->expander_list,
					domain_expander, base.queue_pointer) {
					pal_clean_expander_outstanding_req(root, tmp_exp);
			}
		}

		eh_req = sas_make_task_req(
			root, dev, org_req, TMF_ABORT_TASK,
			(MV_ReqCompletion)ssp_error_handling_callback);
		if (eh_req == NULL) return MV_FALSE;
		err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_ABORT_REQUEST;
		CORE_EH_PRINT(("device %d sending abort TMF.\n",\
			dev->base.id));
		break;
	case SAS_EH_TIMEOUT_STATE_ABORT_REQUEST:
		/* case =
		 * a. task management request comes back but failed
		 * b. retried req which is failed */

		eh_req = sas_make_task_req(
			root, dev, org_req, TMF_LOGICAL_UNIT_RESET,
			(MV_ReqCompletion)ssp_error_handling_callback);
		if (eh_req == NULL) return MV_FALSE;
		err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_LU_RESET;
		CORE_EH_PRINT(("device %d sending logical unit reset TMF.\n",\
			dev->base.id));
		break;

	case SAS_EH_TIMEOUT_STATE_LU_RESET:
		/* case =
		 * a. task management request finished but failed
		 * b. retried req which is failed */

		pal_abort_device_running_req(root, &dev->base);
		if (IS_BEHIND_EXP(dev)) {
			eh_req = ssp_make_virtual_phy_reset_req(dev,
				LINK_RESET, ssp_error_handling_callback);

		} else {
			eh_req = core_make_device_reset_req(
				root, port, dev,
				(MV_ReqCompletion)ssp_error_handling_callback);
		}
		if (eh_req == NULL) return MV_FALSE;
		err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_DEVICE_RESET;
		CORE_EH_PRINT(("device %d sending device reset.\n",\
			dev->base.id));
		break;

	case SAS_EH_TIMEOUT_STATE_DEVICE_RESET:
		/* case =
		 * a. device reset request is failed
		 * b. retried req which is failed */

		if (IS_BEHIND_EXP(dev)) {
			eh_req = ssp_make_virtual_phy_reset_req(dev,
				HARD_RESET, ssp_error_handling_callback);
		} else {
			eh_req = core_make_port_reset_req(
				root, port, dev,
				(MV_ReqCompletion)ssp_error_handling_callback);
		}
		if (eh_req == NULL) return MV_FALSE;
		err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_PORT_RESET;
		CORE_EH_PRINT(("device %d sending port reset.\n",\
			dev->base.id));
		break;

	case SAS_EH_TIMEOUT_STATE_PORT_RESET:
		/* case =
		 * a. port reset request failed
		 * b. retried req which is failed
		 * c. if > MAX_TIMEOUT_ALLOWED, jump to set down*/
		CORE_EH_PRINT(("device %d: complete req with ERROR\n",dev->base.id));
		dev->status |= DEVICE_STATUS_FROZEN;
		core_complete_error_req(root, org_req, REQ_STATUS_ERROR);

		return MV_TRUE;
	default:
		MV_ASSERT(MV_FALSE);
	}

	MV_ASSERT(eh_req != NULL);
	core_queue_eh_req(root, eh_req);
	return MV_TRUE;
}

/*
 * it's called either by the ssp_error_handler
 * or ssp_error_handling_callback
 * refer to sas_timeout_state_machine
 */
static MV_BOOLEAN ssp_media_error_state_machine(pl_root *root,
	domain_device *dev, MV_BOOLEAN success)
{
	struct _error_context *err_ctx = &(dev->base.err_ctx);
	MV_Request *org_req = err_ctx->error_req;
	MV_Request *eh_req = NULL;

	MV_ASSERT(err_ctx->eh_type == EH_TYPE_MEDIA_ERROR);
	MV_ASSERT(org_req != NULL);

	CORE_EH_PRINT(("device %d state %d success 0x%x\n",\
		dev->base.id, err_ctx->eh_state, success));

	switch (err_ctx->eh_state) {
	case SAS_EH_MEDIA_STATE_NONE:
		MV_ASSERT(success == MV_FALSE);
		eh_req = core_eh_retry_org_req(
				root, org_req, (MV_ReqCompletion)ssp_error_handling_callback);
		if (eh_req == NULL) return MV_FALSE;
		core_queue_eh_req(root, eh_req);
		err_ctx->eh_state = SAS_EH_MEDIA_STATE_RETRY;
		break;
	case SAS_EH_MEDIA_STATE_RETRY:
		MV_ASSERT(success == MV_FALSE);
		core_complete_error_req(root, org_req, REQ_STATUS_ERROR);
		break;

	default:
		MV_ASSERT(MV_FALSE);
		break;
	}

	return MV_TRUE;
}

/*
 * in two cases code will come here
 * 1. the original error req is called using handler when error is first hit
 * 2. if in state machine lack of resource, 
 *    the original error request got pushed to the queue again.
 *    see the comments in the function.
 */
MV_BOOLEAN ssp_error_handler(MV_PVOID dev_p, MV_Request *req)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	struct _error_context *err_ctx = &dev->base.err_ctx;

	MV_ASSERT(dev->base.type == BASE_TYPE_DOMAIN_DEVICE);
	MV_ASSERT(IS_SSP(dev));

	CORE_EH_PRINT(("device %d eh_type %d eh_state %d status 0x%x req %p Cdb[0x%x]\n",\
                dev->base.id, err_ctx->eh_type, err_ctx->eh_state, 
                err_ctx->scsi_status, req, req->Cdb[0]));

	/* process_command can only return five status
	 * REQ_STATUS_SUCCESS, XXX_HAS_SENSE, XXX_ERROR, XXX_TIMEOUT, XXX_NO_DEVICE */
	if (err_ctx->eh_type == EH_TYPE_NONE) {
		MV_ASSERT((req->Scsi_Status == REQ_STATUS_ERROR)
			|| (req->Scsi_Status == REQ_STATUS_HAS_SENSE)
			|| (req->Scsi_Status == REQ_STATUS_TIMEOUT));
		MV_ASSERT((err_ctx->error_req == NULL)
                        || (err_ctx->error_req == req)); 
		err_ctx->error_req = req;

		if (req->Scsi_Status == REQ_STATUS_TIMEOUT) {
			err_ctx->eh_type = EH_TYPE_TIMEOUT;
			err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_NONE;
                     return ssp_timeout_state_machine(root, dev, MV_FALSE);
		} else {
			err_ctx->eh_type = EH_TYPE_MEDIA_ERROR;
			err_ctx->eh_state = SAS_EH_MEDIA_STATE_NONE;
			return ssp_media_error_state_machine(root, dev, MV_FALSE);
		}
	} else {
		/* code comes here if ssp_error_handling_callback 
                 * cannot continue because short of resource
		 * no device should be handled already 
                 * but the req is the original error request not eh req
                 * because the eh req has released already 
                 * the eh req scsi status is saved in err_ctx->scsi_status */
              MV_ASSERT(req == err_ctx->error_req);

		MV_ASSERT((err_ctx->scsi_status == REQ_STATUS_ERROR)
			|| (err_ctx->scsi_status == REQ_STATUS_HAS_SENSE)
			|| (err_ctx->scsi_status == REQ_STATUS_SUCCESS)
			|| (err_ctx->scsi_status == REQ_STATUS_TIMEOUT));

		MV_ASSERT((err_ctx->eh_type == EH_TYPE_MEDIA_ERROR)
			||(err_ctx->eh_type == EH_TYPE_TIMEOUT));

		if (err_ctx->eh_type == EH_TYPE_TIMEOUT) {
                        return ssp_timeout_state_machine(root, dev, 
                                (err_ctx->scsi_status==REQ_STATUS_SUCCESS?MV_TRUE:MV_FALSE));
		} else {
			return ssp_media_error_state_machine(root, dev, 
                                (err_ctx->scsi_status==REQ_STATUS_SUCCESS?MV_TRUE:MV_FALSE));
		}
	}
}

static void ssp_error_handling_callback(pl_root *root, MV_Request *req)
{
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *dev = (domain_device *)get_device_by_id(root->lib_dev,
		req->Device_Id);
	struct _error_context *err_ctx = &dev->base.err_ctx;
	MV_BOOLEAN ret = MV_TRUE;
	MV_Request *org_req = err_ctx->error_req;

	MV_ASSERT(dev->base.type == BASE_TYPE_DOMAIN_DEVICE);
	MV_ASSERT(IS_SSP(dev));
	MV_ASSERT(CORE_IS_EH_REQ(ctx));
	MV_ASSERT((err_ctx->eh_type == EH_TYPE_MEDIA_ERROR)
		|| (err_ctx->eh_type == EH_TYPE_TIMEOUT));
	MV_ASSERT(org_req != NULL);

	CORE_EH_PRINT(("device %d eh_type %d eh_state %d eh_status 0x%x req %p Cdb[0x%x] status 0x%x\n",\
                dev->base.id, err_ctx->eh_type, err_ctx->eh_state, 
                err_ctx->scsi_status, req, req->Cdb[0], req->Scsi_Status));

	if (req->Scsi_Status == REQ_STATUS_NO_DEVICE) {
		core_complete_error_req(root, org_req, REQ_STATUS_NO_DEVICE);
		goto end_point;
	} else if (req->Scsi_Status == REQ_STATUS_TIMEOUT) {
		if (err_ctx->eh_type == EH_TYPE_MEDIA_ERROR) {
			err_ctx->eh_type = EH_TYPE_TIMEOUT;
			err_ctx->eh_state = SAS_EH_TIMEOUT_STATE_NONE;
		}
		/* continue the state machine */
	} else if (req->Scsi_Status == REQ_STATUS_SUCCESS) {
		if (CORE_IS_EH_RETRY_REQ(ctx)) {
			/* original req retry success, complete state machine*/
			core_complete_error_req(root, org_req, REQ_STATUS_SUCCESS);
			goto end_point;
		}
	} else if (req->Scsi_Status == REQ_STATUS_HAS_SENSE) {
		if (CORE_IS_EH_RETRY_REQ(ctx)) {
			/* copy sense code */
			MV_CopyMemory(org_req->Sense_Info_Buffer,
				req->Sense_Info_Buffer,
				MV_MIN(org_req->Sense_Info_Buffer_Length,
				req->Sense_Info_Buffer_Length));
			core_complete_error_req(root, org_req, REQ_STATUS_HAS_SENSE);
			goto end_point;
		}
	}


	MV_ASSERT((req->Scsi_Status == REQ_STATUS_ERROR)
		|| (req->Scsi_Status == REQ_STATUS_HAS_SENSE)
		|| (req->Scsi_Status == REQ_STATUS_TIMEOUT)
		|| (req->Scsi_Status == REQ_STATUS_SUCCESS));

	if (err_ctx->eh_type == EH_TYPE_MEDIA_ERROR) {
		ret = ssp_media_error_state_machine(root, dev, 
                        (req->Scsi_Status==REQ_STATUS_SUCCESS?MV_TRUE:MV_FALSE));
	} else {
        ret = ssp_timeout_state_machine(root, dev, 
                (req->Scsi_Status==REQ_STATUS_SUCCESS?MV_TRUE:MV_FALSE));
	}

end_point:
	if (ret == MV_FALSE) {
              err_ctx->scsi_status = req->Scsi_Status;
		core_queue_error_req(root, org_req, MV_FALSE);
        }
}

MV_Request *sas_make_task_req(pl_root *root, domain_device *dev,
	MV_Request *org_req, MV_U8 task_function, MV_ReqCompletion func)
{
	core_context *ctx;
	MV_Request *req;
	MV_U16 tag;

        /* allocate resource */
	req = get_intl_req_resource(root, 0);
	if (req == NULL) return NULL;

	if (task_function == TMF_ABORT_TASK_SET) {
		tag = CORE_MAX_REQUEST_NUMBER << root->max_cmd_slot_width |
			CORE_MAX_REQUEST_NUMBER;
	} else {
		ctx = (core_context *)org_req->Context[MODULE_CORE];
		tag = (org_req->Tag<<root->max_cmd_slot_width) | ctx->slot;
	}

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_TASK_MGMT;
	req->Cdb[3] = (MV_U8)(tag&0xff);
	req->Cdb[4] = (MV_U8)(tag>>8);
	req->Cdb[5] = task_function;
	MV_ZeroMemory(&(req->Cdb[6]), 8);
	mv_int_to_reqlun(dev->base.LUN, &(req->Cdb[6]));

	req->Cmd_Flag = 0;

	req->Device_Id = dev->base.id;
	req->Completion = func;

	return req;
}

MV_VOID ssp_wait_hard_reset(MV_PVOID dev_p, MV_PVOID tmp)
{
        domain_device *dev = (domain_device *)dev_p;
        pl_root *root = dev->base.root;
        MV_BOOLEAN ret = MV_FALSE; 
        struct _error_context *err_ctx = &dev->base.err_ctx;

        err_ctx->eh_timer = NO_CURRENT_TIMER;
        if (err_ctx->eh_type == EH_TYPE_TIMEOUT)
        	ret = ssp_timeout_state_machine(root, dev, MV_TRUE);
        else if (err_ctx->eh_type == EH_TYPE_MEDIA_ERROR)
        	ret = ssp_media_error_state_machine(root, dev, MV_TRUE);
        
        if (ret == MV_FALSE) {
                err_ctx->scsi_status = REQ_STATUS_SUCCESS;
                core_queue_error_req(root, err_ctx->error_req, MV_FALSE);
        }
}


