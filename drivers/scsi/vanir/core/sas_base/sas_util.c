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
#include "core_sas.h"
#include "core_internal.h"
#include "core_manager.h"
#include "com_api.h"
#include "com_u64.h"

#include "core_util.h"
#include "core_error.h"
#include "core_sat.h"

MV_VOID sas_replace_org_req(MV_PVOID root_p,
	MV_Request *org_req,
	MV_Request *new_req)
{
	pl_root *root = (pl_root *) root_p;
	core_context *org_ctx;
	core_context *new_ctx;

	if (org_req == NULL || new_req == NULL) {
		MV_ASSERT(MV_FALSE);
		return;
	}
	org_ctx = (core_context*)org_req->Context[MODULE_CORE];
	new_ctx = (core_context*)new_req->Context[MODULE_CORE];

	new_ctx->req_type = org_ctx->req_type;
	new_req->Time_Out = org_req->Time_Out;
	new_req->Org_Req = org_req;
	core_append_request(root, new_req);
}

MV_Request *sas_get_org_req(MV_Request *req)
{
	core_context *ctx;

	if (req == NULL) {
		MV_ASSERT(MV_FALSE);
		return NULL;
	}

	return (req->Org_Req);
}

MV_Request *sas_clear_org_req(MV_Request *req)
{
	core_context *ctx;
	MV_Request *org_req;

	if (req == NULL) {
		MV_ASSERT(MV_FALSE);
		return NULL;
	}

	org_req = req->Org_Req;
	req->Org_Req = NULL;

	return (org_req);
}

MV_Request
*sas_make_mode_sense_req(MV_PVOID dev_p,
        MV_ReqCompletion cmpltn)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	MV_Request *req;
	MV_U8 length = MAX_MODE_PAGE_LENGTH;      /* Mode Page Cache */

	req = get_intl_req_resource(root, length);
	if (req == NULL)
		return NULL;

	req->Cdb[0] = SCSI_CMD_MODE_SENSE_6;
	req->Cdb[1] = 0x08; /* disable block descriptors */
	req->Cdb[2] = 0x08; /* PC = 0, Page Code = 0x08(caching) */
	req->Cdb[3] = 0;    /* Subpage Code = 0 */
	req->Cdb[4] = length;

	req->Device_Id = dev->base.id;
	req->Cmd_Flag = CMD_FLAG_DATA_IN;
	req->Completion = cmpltn;

	return req;
}

MV_Request
*sas_make_marvell_specific_req(MV_PVOID dev_p,
        MV_U8 cmd,
        MV_ReqCompletion cmpltn)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	MV_Request *req;

	req = get_intl_req_resource(root, 0);
	if (req == NULL)
		return NULL;

	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = cmd;

	req->Device_Id = dev->base.id;
	req->Cmd_Flag = 0;
	req->Completion = cmpltn;

	return req;
}

MV_Request *sas_make_log_sense_req(MV_PVOID dev_p,
	MV_U8 page,
	MV_ReqCompletion cmpltn)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	MV_Request *req;
	MV_U16 buf_size = MAX_MODE_LOG_LENGTH;

	req = get_intl_req_resource(root, buf_size);
	if (req == NULL)
		return NULL;

	req->Cdb[0] = SCSI_CMD_LOG_SENSE;
	req->Cdb[1] = 0;                        /* SP=0 PPC=0 */
	req->Cdb[2] = 0x40 | (page & 0x3F);     /* PC=01b */
	req->Cdb[7] = (buf_size >> 8) & 0xFF;
	req->Cdb[8] = buf_size & 0xFF;
	req->Cdb[9] = 0;                       /* Control */

	req->Device_Id = dev->base.id;
	req->Cmd_Flag = CMD_FLAG_DATA_IN;
	req->Completion = cmpltn;

	return req;
}

MV_Request *sas_make_sync_cache_req(MV_PVOID dev_p,
	MV_ReqCompletion cmpltn)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	MV_Request *req;

	req = get_intl_req_resource(root, 0);
	if (req == NULL)
		return NULL;

	req->Cdb[0] = SCSI_CMD_SYNCHRONIZE_CACHE_10;

	req->Device_Id = dev->base.id;
	req->Cmd_Flag = 0;
	req->Completion = cmpltn;

	return req;
}


MV_U8 ssp_ata_parse_log_sense_threshold_exceed(MV_PU8 data_buf, MV_U32 length)
{
	if (data_buf == NULL || length < 9) {
		MV_DASSERT(MV_FALSE);
		return MV_FALSE;
	}

	if (data_buf[8] == SCSI_ASC_FAILURE_PREDICTION_THRESHOLD_EXCEEDED)
		return MV_TRUE;
	else
		return MV_FALSE;
}

MV_VOID ssp_ata_parse_smart_return_status(domain_device *dev,
	MV_Request *org_req, MV_Request *new_req)
{
	pl_root *root = dev->base.root;
	core_extension *core = (core_extension *)root->core;
	HD_SMART_Status *status;
	MV_PVOID new_buf_ptr;
	MV_U8 ret;

	if (org_req == NULL || new_req == NULL) {
		MV_ASSERT(MV_FALSE);
		return;
	}

	new_buf_ptr = core_map_data_buffer(new_req);
	ret = ssp_ata_parse_log_sense_threshold_exceed(
	new_buf_ptr,
	new_req->Data_Transfer_Length);

	status = core_map_data_buffer(org_req);

	if (status != NULL)
		status->SmartThresholdExceeded = ret;

	if (ret == MV_TRUE)
		core_generate_event(core, EVT_ID_HD_SMART_THRESHOLD_OVER,
		dev->base.id, SEVERITY_WARNING, 0, NULL ,0);

	core_unmap_data_buffer(org_req);
	core_unmap_data_buffer(new_req);
}

MV_Request *sas_make_inquiry_req(MV_PVOID root_p, MV_PVOID dev_p,
	MV_BOOLEAN EVPD, MV_U8 page, MV_ReqCompletion completion)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *dev = (domain_device *)dev_p;
	PMV_Request req;

	/* allocate resource */
	req = get_intl_req_resource(root, 0x60);
	if (req == NULL)
		return NULL;

	/* Prepare inquiry */
	if (!EVPD) {
		req->Cdb[0] = SCSI_CMD_INQUIRY;
		req->Cdb[1] = 0;
		req->Cdb[4] = 0x60;
	} else {
		req->Cdb[0] = SCSI_CMD_INQUIRY;
		req->Cdb[1] = 0x01; /* EVPD inquiry */
		req->Cdb[2] = page;
		req->Cdb[4] = 0x60;
	}
	req->Device_Id = dev->base.id;
	req->Cmd_Flag = CMD_FLAG_DATA_IN;
	req->Completion = completion;
	return req;
}

MV_Request *sas_make_report_lun_req(MV_PVOID root_p, MV_PVOID dev_p,
	MV_ReqCompletion completion)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *dev = (domain_device *)dev_p;
	PMV_Request req;
	MV_U16 buf_size = 0x400;
	
	/* allocate resource */
	req = get_intl_req_resource(root, buf_size);
	if (req == NULL)
		return NULL;
	
	req->Cdb[0] = SCSI_CMD_REPORT_LUN;
	req->Cdb[2] = 0;
	/*Cdb[6..9] means the allocation length, and it equals to buf_size */
	req->Cdb[8] = 0x4;
	req->Cdb[9] = 0x00;

	req->Device_Id = dev->base.id;
	req->Cmd_Flag = CMD_FLAG_DATA_IN;
	req->Completion = completion;
	return req;
}

MV_Request *
ssp_make_virtual_phy_reset_req(MV_PVOID dev_p,
	MV_U8 operation, MV_PVOID callback)
{
	domain_device *dev = (domain_device *)dev_p;
	pl_root *root = dev->base.root;
	MV_Request *req = get_intl_req_resource(root, 0);

	if (req == NULL) return NULL;

	MV_ASSERT(dev->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER);

	req->Device_Id = dev->base.id;
	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_SSP_VIRTUAL_PHY_RESET;
	req->Cdb[3] = operation;
	req->Completion = callback;

	return req;
}


MV_VOID
stp_req_report_phy_sata_callback(MV_PVOID root_p, MV_Request *req, MV_PVOID exp_p)
{
	pl_root *root = (pl_root *)root_p;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_Request *org_req = ctx->u.org.org_req;
	core_context *org_req_ctx = (core_context *)org_req->Context[MODULE_CORE];
	domain_expander *exp = (domain_expander *)exp_p;
	domain_device *device = NULL;
	smp_response *smp_resp = NULL;

	if (exp) {
		if (req->Scsi_Status == REQ_STATUS_SUCCESS) {
			smp_resp = (smp_response *)core_map_data_buffer(req);
			if (smp_resp != NULL) {
				LIST_FOR_EACH_ENTRY_TYPE(device,
				&exp->device_list, domain_device,
				base.exp_queue_pointer) {
					if (device->parent_phy_id ==
						smp_resp->response.ReportPhySATA.PhyIdentifier)
						break;
				}

				if (device == NULL) {
					MV_ASSERT(MV_FALSE);
					return;
				}

				switch (device->state) {
				case DEVICE_STATE_STP_REPORT_PHY:
					if (smp_resp->response.ReportPhySATA.AffiliationValid) {
						CORE_DPRINT(("Find device %d phy %d has affiliation_valid.\n",device->base.id, smp_resp->response.ReportPhySATA.PhyIdentifier));
						org_req_ctx->u.smp_report_phy.affiliation_valid = 1;
					}
					else {
						device->signature =
						smp_resp->response.ReportPhySATA.fis.lba_high << 24 |
						smp_resp->response.ReportPhySATA.fis.lba_mid << 16 |
						smp_resp->response.ReportPhySATA.fis.lba_low << 8 |
						smp_resp->response.ReportPhySATA.fis.sector_count;
					}
					break;
				default:
					break;
				}
			}
			core_unmap_data_buffer(req);
		}
	}

	core_queue_completed_req(root->core, org_req);
}

MV_VOID stp_req_disc_callback(MV_PVOID root_p, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_base *base = get_device_by_id(root->lib_dev, req->Device_Id);
	domain_device *device = NULL;
	core_context *ctx = NULL;

	if (!base)
		return;

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		core_handle_init_error(root, base, req);
		return;
	}

	MV_DASSERT(base->type == BASE_TYPE_DOMAIN_DEVICE);
	device = (domain_device *)base;
	ctx = (core_context *)req->Context[MODULE_CORE];

	switch (device->state) {
	case DEVICE_STATE_STP_RESET_PHY:
		device->state = DEVICE_STATE_STP_REPORT_PHY;
		core_sleep_millisecond(root->core, 100);
		break;
	case DEVICE_STATE_STP_REPORT_PHY:
		if (ctx->u.smp_report_phy.affiliation_valid == 1) {
			device->state = DEVICE_STATE_STP_RESET_PHY;
		}
		else {
			device->state = DEVICE_STATE_RESET_DONE;
		}
		break;
	}

	core_queue_init_entry(root, base, MV_FALSE);
}

MV_Request *stp_make_report_phy_sata_req(domain_device *dev, MV_PVOID callback)
{
	pl_root *root = dev->base.root;
	MV_Request *req = get_intl_req_resource(root, 0);
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];

	if (req == NULL)
		return NULL;

	MV_DASSERT(dev->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER);

	req->Device_Id = dev->base.id;
	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_STP_VIRTUAL_REPORT_SATA_PHY;
	req->Completion = callback;

	ctx->u.smp_report_phy.affiliation_valid = 0;

	return req;
}

MV_Request *stp_make_phy_reset_req(domain_device *device, MV_U8 operation,
	MV_PVOID callback)
{
	pl_root *root = device->base.root;
	MV_Request *req = get_intl_req_resource(root, 0);

	if (req == NULL) {
		return NULL;
	}

	MV_DASSERT(device->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER);

	req->Device_Id = device->base.id;
	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_STP_VIRTUAL_PHY_RESET;
	req->Cdb[3] = operation;
	req->Completion = callback;

	return req;
}

void mv_int_to_reqlun(MV_U16 lun, MV_U8*reqlun)
{
	reqlun[1] = lun & 0xFF;
	reqlun[0] = (lun >> 8) & 0xFF;
	reqlun[2] = 0;
	reqlun[3] = 0;
}

