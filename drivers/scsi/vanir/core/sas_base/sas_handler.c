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
#include "com_error.h"

#include "core_util.h"
#include "core_error.h"
#include "core_sat.h"
#include "core_console.h"
#include "core_expander.h"

#define IS_A_SMP_REQ(pReq) \
	((pReq->Cdb[0]==SCSI_CMD_MARVELL_SPECIFIC)&& \
	 (pReq->Cdb[1]==CDB_CORE_MODULE)&& \
	 (pReq->Cdb[2]==CDB_CORE_SMP))

extern MV_VOID prot_process_cmpl_req(pl_root *root, MV_Request *req);
extern MV_VOID ssp_ata_intl_req_callback(MV_PVOID root_p, MV_Request *req);
extern MV_U8 ssp_ata_parse_smart_return_status(domain_device *device,
	MV_Request *org_req, MV_Request *new_req);
extern MV_Request *smp_make_discover_req(domain_expander *exp,
	MV_Request *vir_req, MV_U8 phy_id, MV_PVOID callback);
extern MV_VOID smp_req_discover_callback(MV_PVOID root_p, MV_Request *req, MV_PVOID exp_p);
extern MV_VOID smp_req_reset_sata_phy_callback(MV_PVOID root_p, MV_Request *req, MV_PVOID exp_p);
extern MV_VOID smp_req_clear_aff_callback(MV_PVOID root_p, MV_Request *req, MV_PVOID exp_p);
extern MV_Request *smp_make_config_route_req(domain_expander *exp,
	MV_Request *vir_req, MV_U16 route_index, MV_U8 phy_id, MV_U64 *sas_addr,
	MV_PVOID callback);
extern MV_VOID smp_req_config_route_callback(MV_PVOID root_p, MV_Request *req, MV_PVOID exp_p);
extern MV_Request *smp_make_phy_crtl_by_id_req(MV_PVOID exp_p,
	MV_U8 phy_id, MV_U8 operation, MV_PVOID callback);
extern MV_Request *smp_make_report_phy_sata_by_id_req(MV_PVOID exp_p,
	MV_U8 phy_id, MV_PVOID callback);
extern domain_base *exp_search_phy(domain_expander *exp, MV_U8 phy_id);
MV_VOID ssp_ata_fill_inquiry_device(domain_device *dev, MV_Request *req);
MV_VOID ssp_mode_page_rmw_callback(MV_PVOID root_p, MV_Request *req);
extern MV_VOID smp_physical_req_callback(MV_PVOID root_p, MV_Request *req);

MV_U8 ssp_verify_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	core_extension *core = (core_extension *)root->core;
	domain_device *dev = (domain_device *)dev_p;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_QUEUE_COMMAND_RESULT result = MV_QUEUE_COMMAND_RESULT_PASSED;
	MV_Request *new_req;
	MV_U8 ret;
	HD_SMART_Status *status;

	if (req->Cdb[0] == APICDB0_ADAPTER){
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	if (req->Cdb[0] == SCSI_CMD_START_STOP_UNIT)
		req->Time_Out = 60;
	
	if ((req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC) &&
		(req->Cdb[1] == CDB_CORE_MODULE)) {

		switch (req->Cdb[2]) {
		case CDB_CORE_TASK_MGMT:
			break;

		case CDB_CORE_RESET_DEVICE:
			mv_reset_phy(root, req->Cdb[3], MV_FALSE);
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;

		case CDB_CORE_RESET_PORT:
			mv_reset_phy(root, req->Cdb[3], MV_TRUE);
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;

		case CDB_CORE_ENABLE_SMART:
			if (!(dev->capability &
				DEVICE_CAPABILITY_SMART_SUPPORTED))
				CORE_DPRINT(("Device %d does not "\
				"support SMART, but returning "\
				"REQ_STATUS_SUCCESS anyway\n",\
				dev->base.id));
			else
				dev->setting |= DEVICE_SETTING_SMART_ENABLED;
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;

		case CDB_CORE_DISABLE_SMART:
			dev->setting &= ~DEVICE_SETTING_SMART_ENABLED;
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;

		case CDB_CORE_ENABLE_WRITE_CACHE:
			new_req = sas_make_mode_sense_req(dev,
			ssp_mode_page_rmw_callback);

			if (new_req == NULL)
				return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

			sas_replace_org_req(root, req, new_req);
			return MV_QUEUE_COMMAND_RESULT_REPLACED;

		case CDB_CORE_DISABLE_WRITE_CACHE:
			new_req = sas_make_mode_sense_req(dev,
			ssp_mode_page_rmw_callback);

			if (new_req == NULL)
				return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
			sas_replace_org_req(root, req, new_req);
			return MV_QUEUE_COMMAND_RESULT_REPLACED;

		case CDB_CORE_ENABLE_READ_AHEAD:
			new_req = sas_make_mode_sense_req(dev,
			ssp_mode_page_rmw_callback);
			if (new_req == NULL)
				return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

			sas_replace_org_req(root, req, new_req);
			return MV_QUEUE_COMMAND_RESULT_REPLACED;

		case CDB_CORE_DISABLE_READ_AHEAD:
			new_req = sas_make_mode_sense_req(dev,
			ssp_mode_page_rmw_callback);

			if (new_req == NULL)
				return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

			sas_replace_org_req(root, req, new_req);
			return MV_QUEUE_COMMAND_RESULT_REPLACED;

		case CDB_CORE_SMART_RETURN_STATUS:
			if (!(dev->capability &
				DEVICE_CAPABILITY_SMART_SUPPORTED)) {
				req->Scsi_Status = REQ_STATUS_SUCCESS;
				status = core_map_data_buffer(req);
				if (status != NULL)
					status->SmartThresholdExceeded = MV_FALSE;
				core_unmap_data_buffer(req);
				return MV_QUEUE_COMMAND_RESULT_FINISHED;
			}

			/* Page=2F(Informal Exceptions Log) */
			new_req = sas_make_log_sense_req(dev,
			0x2F,
			ssp_ata_intl_req_callback);

			if (new_req == NULL)
				return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

			sas_replace_org_req(root, req, new_req);
			return MV_QUEUE_COMMAND_RESULT_REPLACED;
		case CDB_CORE_OS_SMART_CMD:
                        if (req->Cdb[3] == ATA_CMD_IDENTIFY_ATA) {

                                ssp_ata_fill_inquiry_device(dev, req);
                                req->Scsi_Status = REQ_STATUS_SUCCESS;
			        return MV_QUEUE_COMMAND_RESULT_FINISHED;
                        }

                        if (!(dev->capability &
                                DEVICE_CAPABILITY_SMART_SUPPORTED)) {

			        req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
			        return MV_QUEUE_COMMAND_RESULT_FINISHED;
                        }

                        switch (req->Cdb[4]) {
                        case ATA_CMD_ENABLE_SMART:
                                if (!(dev->capability &
                                        DEVICE_CAPABILITY_SMART_SUPPORTED)) {

                                        CORE_DPRINT(("Device %d does "\
                                                "not support SMART, but "\
                                                "returning REQ_STATUS_SUCCESS"\
                                                " anyway\n", dev->base.id));
                                } else if (!(dev->setting &
                                        DEVICE_SETTING_SMART_ENABLED)) {

                                        dev->setting |=
                                                DEVICE_SETTING_SMART_ENABLED;
                                }

                                req->Scsi_Status = REQ_STATUS_SUCCESS;
                                return MV_QUEUE_COMMAND_RESULT_FINISHED;

                        case ATA_CMD_DISABLE_SMART:
                                dev->setting &= ~DEVICE_SETTING_SMART_ENABLED;

                                req->Scsi_Status = REQ_STATUS_SUCCESS;
                                return MV_QUEUE_COMMAND_RESULT_FINISHED;

                        case ATA_CMD_SMART_RETURN_STATUS:
                                if (!(dev->capability &
                                        DEVICE_CAPABILITY_SMART_SUPPORTED)) {

                                        req->Scsi_Status = REQ_STATUS_SUCCESS;
                                        status = (HD_SMART_Status *)
                                                core_map_data_buffer(req);
                                        if (status != NULL)
                                                status->SmartThresholdExceeded
                                                        = MV_FALSE;
                                        core_unmap_data_buffer(req);
                                        return MV_QUEUE_COMMAND_RESULT_FINISHED;
                                }

                                /* Page=2F(Informal Exceptions Log) */
                                new_req = sas_make_log_sense_req(dev,
                                                0x2F,
                                                ssp_ata_intl_req_callback);

                                ret = MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
                                if (new_req == NULL)
                                        return ret;

                                sas_replace_org_req(root, req, new_req);
                                return MV_QUEUE_COMMAND_RESULT_REPLACED;

                        case ATA_CMD_SMART_READ_DATA:
                        case ATA_CMD_SMART_ENABLE_ATTRIBUTE_AUTOSAVE:
                        case ATA_CMD_SMART_EXECUTE_OFFLINE:
                        case ATA_CMD_SMART_READ_LOG:
                        case ATA_CMD_SMART_WRITE_LOG:

                                req->Scsi_Status = REQ_STATUS_SUCCESS;
                                return MV_QUEUE_COMMAND_RESULT_FINISHED;
                        }

                        break;
		case CDB_CORE_SSP_VIRTUAL_PHY_RESET:
			new_req = smp_make_phy_control_req(dev, req->Cdb[3],
				smp_physical_req_callback);
			if (!new_req) return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

			((core_context *)new_req->Context[MODULE_CORE])->u.org.org_req = req;
			core_queue_eh_req(root, new_req);
			return MV_QUEUE_COMMAND_RESULT_REPLACED;
		default:
			req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}

	}

	switch (req->Cdb[0]) {
	case APICDB0_PASS_THRU_CMD_SCSI:
	case APICDB0_PASS_THRU_CMD_ATA:
		result = core_pass_thru_send_command(root->core, req);
		break;
	case APICDB0_PD:
		switch (req->Cdb[1]) {
		case APICDB1_PD_SETSETTING:
			switch (req->Cdb[4]) {
			case APICDB4_PD_SET_WRITE_CACHE_OFF:
	                        if (!(dev->setting &
	                                DEVICE_SETTING_WRITECACHE_ENABLED)) {

	                                req->Scsi_Status = REQ_STATUS_SUCCESS;
	                                return MV_QUEUE_COMMAND_RESULT_FINISHED;
	                        }

	                        new_req = sas_make_mode_sense_req(dev,
	                                ssp_mode_page_rmw_callback);

	                        if (new_req == NULL)
	                                return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

	                        sas_replace_org_req(root, req, new_req);
	                        return MV_QUEUE_COMMAND_RESULT_REPLACED;

	                case APICDB4_PD_SET_WRITE_CACHE_ON:
	                        if (dev->setting & DEVICE_SETTING_WRITECACHE_ENABLED) {
	                                req->Scsi_Status = REQ_STATUS_SUCCESS;
	                                return MV_QUEUE_COMMAND_RESULT_FINISHED;
	                        }

	                        new_req = sas_make_mode_sense_req(dev,
	                                ssp_mode_page_rmw_callback);

	                        if (new_req == NULL)
	                                return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

	                        sas_replace_org_req(root, req, new_req);
	                        return MV_QUEUE_COMMAND_RESULT_REPLACED;

	                case APICDB4_PD_SET_SMART_OFF:
	                        if (dev->setting & DEVICE_SETTING_SMART_ENABLED) {
	                                dev->setting &= ~DEVICE_SETTING_SMART_ENABLED;
	                        }

	                        req->Scsi_Status = REQ_STATUS_SUCCESS;
	                        return MV_QUEUE_COMMAND_RESULT_FINISHED;

	                case APICDB4_PD_SET_SMART_ON:
	                        if (!(dev->capability &
	                                DEVICE_CAPABILITY_SMART_SUPPORTED)) {

	                                CORE_DPRINT(("Device %d does not "\
	                                        "support SMART, but returning "\
	                                        "REQ_STATUS_SUCCESS anyway\n",\
	                                        dev->base.id));

	                        } else if (!(dev->setting &
	                                DEVICE_SETTING_SMART_ENABLED)) {

	                                dev->setting &= ~DEVICE_SETTING_SMART_ENABLED;
	                        }

	                        req->Scsi_Status = REQ_STATUS_SUCCESS;
	                        return MV_QUEUE_COMMAND_RESULT_FINISHED;

	                default:
	                        CORE_DPRINT(("Unsupported API PD "\
	                                "Setting %d\n", req->Cdb[4]));

	                        if (req->Sense_Info_Buffer != NULL &&
	                                req->Sense_Info_Buffer_Length != 0)

	                                ((MV_PU8)req->Sense_Info_Buffer)[0]
	                                        = ERR_INVALID_PARAMETER;

	                        req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
	                        result = MV_QUEUE_COMMAND_RESULT_FINISHED;
	                        break;
	                }
	                break;

	        case APICDB1_PD_GETSTATUS:
	                switch (req->Cdb[4]) {
	                case APICDB4_PD_SMART_RETURN_STATUS:
	                        if (!(dev->capability &
	                                DEVICE_CAPABILITY_SMART_SUPPORTED)) {

	                                req->Scsi_Status = REQ_STATUS_SUCCESS;
	                                status = core_map_data_buffer(req);
	                                if (status != NULL)
	                                        status->SmartThresholdExceeded = MV_FALSE;
	                                core_unmap_data_buffer(req);
	                                return MV_QUEUE_COMMAND_RESULT_FINISHED;
	                        }

	                        /* Page=2F(Informal Exceptions Log) */
	                        new_req = sas_make_log_sense_req(dev, 0x2F,
	                                        ssp_ata_intl_req_callback);

	                        if (new_req == NULL)
	                                return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

	                        sas_replace_org_req(root, req, new_req);
	                        return MV_QUEUE_COMMAND_RESULT_REPLACED;

	                default:
	                        CORE_DPRINT(("Unsupported API PD "\
	                                "Get Status %d\n", req->Cdb[4]));

	                        if (req->Sense_Info_Buffer != NULL &&
	                                req->Sense_Info_Buffer_Length != 0)

	                                ((MV_PU8)req->Sense_Info_Buffer)[0]
	                                        = ERR_INVALID_PARAMETER;

	                        req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
	                        result = MV_QUEUE_COMMAND_RESULT_FINISHED;
	                        break;
	                }
	        }
       break;

	default:
		result = MV_QUEUE_COMMAND_RESULT_PASSED;
	}

	return result;
}

MV_VOID ssp_ata_host_string2ata(MV_U16 *source, MV_U16 *target,
	MV_U32 words_count)
{
	MV_U32 i;
	for (i=0; i < words_count; i++) {
		target[i] = (source[i] >> 8) | ((source[i] & 0xff) << 8);
		target[i] = MV_LE16_TO_CPU(target[i]);
	}
}


MV_VOID ssp_ata_fill_inquiry_device(domain_device *dev, MV_Request *req)
{
        ata_identify_data *dst = (ata_identify_data *)core_map_data_buffer(req);
        MV_U16  tmp;
        MV_LBA max_lba;

        MV_ZeroMemory(dst, sizeof(ata_identify_data));

        tmp = 0x8000;
        ssp_ata_host_string2ata(&tmp, &dst->general_config, 1);

        /* serial number */
        ssp_ata_host_string2ata((MV_PU16)dev->serial_number,
                (MV_PU16)dst->serial_number, 10);

        /* firmware */
        ssp_ata_host_string2ata((MV_PU16)dev->firmware_revision,
                (MV_PU16)dst->firmware_revision, 4);

        /* model */
        ssp_ata_host_string2ata((MV_PU16)dev->model_number,
                (MV_PU16)dst->model_number, 20);

        tmp = 0;
        if (dev->capability & DEVICE_CAPABILITY_SMART_SUPPORTED)
                tmp |= MV_BIT(0);

        ssp_ata_host_string2ata(&tmp,
                &dst->command_set_supported[0], 1);

        tmp = MV_BIT(10);
        ssp_ata_host_string2ata(&tmp,
                &dst->command_set_supported[1], 1);

        tmp = 0;
        if (dev->setting & DEVICE_SETTING_SMART_ENABLED)
                tmp |= MV_BIT(0);
        if (dev->setting & DEVICE_SETTING_WRITECACHE_ENABLED)
                tmp |= MV_BIT(5);
        if (dev->setting & DEVICE_SETTING_READ_LOOK_AHEAD)
                tmp |= MV_BIT(6);

        ssp_ata_host_string2ata(&tmp,
                &dst->command_set_enabled[0], 1);

        /* max lba */
        max_lba = U64_ADD_U32(dev->max_lba, 1);
        tmp = (MV_U16)(max_lba.parts.low & 0xFFFF);
        ssp_ata_host_string2ata(&tmp,
                &dst->max_lba[0], 1);
        ssp_ata_host_string2ata(&tmp,
                &dst->user_addressable_sectors[0], 1);

        tmp = (MV_U16)((max_lba.parts.low >> 16) & 0xFFFF);
        ssp_ata_host_string2ata(&tmp,
                &dst->max_lba[1], 1);
        ssp_ata_host_string2ata(&tmp,
                &dst->user_addressable_sectors[1], 1);

        tmp = (MV_U16)(max_lba.parts.high & 0xFFFF);
        ssp_ata_host_string2ata(&tmp,
                &dst->max_lba[2], 1);
        tmp = (MV_U16)((max_lba.parts.high >> 16) & 0xFFFF);
        ssp_ata_host_string2ata(&tmp,
                &dst->max_lba[3], 1);

        core_unmap_data_buffer(req);
}

MV_VOID
ssp_ata_intl_req_callback(MV_PVOID root_p, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	MV_Request *org_req = sas_clear_org_req(req);
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *dev = (domain_device *)get_device_by_id(root->lib_dev,
		req->Device_Id);
	core_extension *core = (core_extension *)root->core;
	MV_U32 length;

	if (org_req == NULL) {
		MV_DASSERT(MV_FALSE);
		return;
	}

        org_req->Scsi_Status = req->Scsi_Status;
        if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
                length = MV_MIN(org_req->Sense_Info_Buffer_Length,
                                req->Sense_Info_Buffer_Length);

                if (length != 0 &&
                        org_req->Sense_Info_Buffer != NULL &&
                        req->Sense_Info_Buffer != NULL)

                        MV_CopyMemory(
                                org_req->Sense_Info_Buffer,
                                req->Sense_Info_Buffer,
                                length);

                if ((org_req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC &&
                        org_req->Cdb[2] == CDB_CORE_OS_SMART_CMD &&
                        org_req->Cdb[3] == ATA_CMD_SMART &&
                        org_req->Cdb[4] == ATA_CMD_SMART_RETURN_STATUS) ||
                        (org_req->Cdb[0] == APICDB0_PD &&
                        org_req->Cdb[1] == APICDB1_PD_GETSTATUS &&
                        org_req->Cdb[4] == APICDB4_PD_SMART_RETURN_STATUS)) {

                        core_generate_event(core, EVT_ID_HD_SMART_POLLING_FAIL,
                                dev->base.id, SEVERITY_WARNING, 0, NULL ,0);
                }

                core_queue_completed_req(core, org_req);
                return;
        }

	switch (org_req->Cdb[0]) {
        case SCSI_CMD_MARVELL_SPECIFIC:
                if (org_req->Cdb[1] != CDB_CORE_MODULE)
                        MV_DASSERT(MV_FALSE);

		switch (org_req->Cdb[2]) {
                case CDB_CORE_ENABLE_WRITE_CACHE:
                        dev->setting |= DEVICE_SETTING_WRITECACHE_ENABLED;
                        break;

                case CDB_CORE_DISABLE_WRITE_CACHE:
                        dev->setting &= ~DEVICE_SETTING_WRITECACHE_ENABLED;
                        break;

                case CDB_CORE_ENABLE_READ_AHEAD:
                        dev->setting |= DEVICE_SETTING_READ_LOOK_AHEAD;
                        break;

                case CDB_CORE_DISABLE_READ_AHEAD:
                        dev->setting &= ~DEVICE_SETTING_READ_LOOK_AHEAD;
                        break;

                case CDB_CORE_SMART_RETURN_STATUS:
                        ssp_ata_parse_smart_return_status(dev, org_req, req);
                        break;
                case CDB_CORE_OS_SMART_CMD:
                        switch (org_req->Cdb[4]) {
                        case ATA_CMD_SMART_RETURN_STATUS:
                                ssp_ata_parse_smart_return_status(dev,
                                        org_req, req);
                                break;

                        default:
                                MV_DASSERT(MV_FALSE);
                                break;
                        }
                        break;
                case CDB_CORE_SHUTDOWN:
                        break;
                default:
                        MV_DASSERT(MV_FALSE);
                        break;
                }
                break;

        case APICDB0_PD:
                switch (org_req->Cdb[1]) {
                case APICDB1_PD_SETSETTING:
                        switch (org_req->Cdb[4]) {
                        case APICDB4_PD_SET_WRITE_CACHE_OFF:
                                dev->setting &=
                                        ~DEVICE_SETTING_WRITECACHE_ENABLED;

                                core_generate_event(core,
                                        EVT_ID_HD_CACHE_MODE_CHANGE,
                                        dev->base.id, SEVERITY_INFO,
                                        0, NULL ,0);
                                break;

                        case APICDB4_PD_SET_WRITE_CACHE_ON:
                                dev->setting |=
                                        DEVICE_SETTING_WRITECACHE_ENABLED;

                                core_generate_event(core,
                                        EVT_ID_HD_CACHE_MODE_CHANGE,
                                        dev->base.id, SEVERITY_INFO,
                                        0, NULL ,0);
                                break;

                        case APICDB4_PD_SET_SMART_OFF:
                        case APICDB4_PD_SET_SMART_ON:
                                break;
                        default:
		                MV_DASSERT(MV_FALSE);
                                return;
                        }
                        break;

                case APICDB1_PD_GETSTATUS:
                        switch (org_req->Cdb[4]) {
                        case APICDB4_PD_SMART_RETURN_STATUS:
                                ssp_ata_parse_smart_return_status(dev,
					org_req, req);
                                break;

                        default:
		                MV_DASSERT(MV_FALSE);
                                return;
                        }
                        break;
                }
                break;

	default:
		MV_DASSERT(MV_FALSE);
                return;
	}

        core_queue_completed_req(core, org_req);
}

MV_VOID
ssp_mode_page_rmw_callback(MV_PVOID root_p, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	MV_Request *org_req, *new_req;
	core_context *org_ctx;
	domain_device *dev;
	MV_U32 length;
	MV_PU8 org_buf_ptr, new_buf_ptr;

	org_req = sas_clear_org_req(req);

	if (org_req == NULL) {
		MV_ASSERT(MV_FALSE);
		return;
	}

	org_ctx = (core_context *)org_req->Context[MODULE_CORE];
	dev = (domain_device *)get_device_by_id(root->lib_dev,
	org_req->Device_Id);

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		length = MV_MIN(req->Sense_Info_Buffer_Length,
		org_req->Sense_Info_Buffer_Length);

		MV_CopyMemory(org_req->Sense_Info_Buffer,
			req->Sense_Info_Buffer,
			length);

		org_req->Scsi_Status = req->Scsi_Status;
		core_queue_completed_req(root->core, org_req);
		return;
	}

	org_buf_ptr = core_map_data_buffer(req);

	MV_DASSERT(org_buf_ptr[3] == 0);
	MV_DASSERT((org_buf_ptr[4] & 0x6F) == 0x08);

	switch (org_req->Cdb[0]) {
	case SCSI_CMD_MARVELL_SPECIFIC:
		if (org_req->Cdb[1] != CDB_CORE_MODULE)
			MV_DASSERT(MV_FALSE);

		switch (org_req->Cdb[2]) {
		case CDB_CORE_ENABLE_WRITE_CACHE:
			if (org_buf_ptr[6 + org_buf_ptr[3]] & MV_BIT(2)) {
				dev->setting |= DEVICE_SETTING_WRITECACHE_ENABLED;
				org_req->Scsi_Status = req->Scsi_Status;
				core_queue_completed_req(root->core, org_req);
				core_unmap_data_buffer(req);
				return;
			}
			break;

		case CDB_CORE_DISABLE_WRITE_CACHE:
			if (!(org_buf_ptr[6 + org_buf_ptr[3]] & MV_BIT(2))) {
				dev->setting &= ~DEVICE_SETTING_WRITECACHE_ENABLED;
				org_req->Scsi_Status = req->Scsi_Status;
				core_queue_completed_req(root->core, org_req);
				core_unmap_data_buffer(req);
				return;
			}
			break;

		case CDB_CORE_ENABLE_READ_AHEAD:
			if (!(org_buf_ptr[16 + org_buf_ptr[3]] & MV_BIT(5))) {
				dev->setting |= DEVICE_SETTING_READ_LOOK_AHEAD;
				org_req->Scsi_Status = req->Scsi_Status;
				core_queue_completed_req(root->core, org_req);
				core_unmap_data_buffer(req);
				return;
			}
			break;

		case CDB_CORE_DISABLE_READ_AHEAD:
			if (org_buf_ptr[16 + org_buf_ptr[3]] & MV_BIT(5)) {
				dev->setting &= ~DEVICE_SETTING_READ_LOOK_AHEAD;
				org_req->Scsi_Status = req->Scsi_Status;
				core_queue_completed_req(root->core, org_req);
				core_unmap_data_buffer(req);
				return;
			}
			break;

		default:
			MV_DASSERT(MV_FALSE);
			break;
		}
		break;

	case APICDB0_PD:
		switch (org_req->Cdb[1]) {
		case APICDB1_PD_SETSETTING:
			switch (org_req->Cdb[4]) {
			case APICDB4_PD_SET_WRITE_CACHE_OFF:
				if (!(org_buf_ptr[6 + org_buf_ptr[3]] & MV_BIT(2))) {
					dev->setting &= ~DEVICE_SETTING_WRITECACHE_ENABLED;
					org_req->Scsi_Status = req->Scsi_Status;
					core_queue_completed_req(root->core, org_req);
					core_unmap_data_buffer(req);
					return;
				}
				break;

			case APICDB4_PD_SET_WRITE_CACHE_ON:
				if (org_buf_ptr[6 + org_buf_ptr[3]] & MV_BIT(2)) {
					dev->setting |= DEVICE_SETTING_WRITECACHE_ENABLED;
					org_req->Scsi_Status = req->Scsi_Status;
					core_queue_completed_req(root->core, org_req);
					core_unmap_data_buffer(req);
					return;
				}
				break;
			default:
				MV_DASSERT(MV_FALSE);
				break;
			}
			break;
		default:
			MV_DASSERT(MV_FALSE);
			break;
		}
		break;

	default:
		MV_DASSERT(MV_FALSE);
		break;
	}

	new_req = get_intl_req_resource(root, 24);

	if (new_req == NULL) {
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)org_req->Sense_Info_Buffer)[0] = ERR_NO_RESOURCE;

		org_req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
		core_unmap_data_buffer(req);
		core_queue_completed_req(root->core, org_req);
		return;
	}

	new_req->Cdb[0] = SCSI_CMD_MODE_SELECT_6;
	new_req->Cdb[1] = 0x11;
	new_req->Cdb[4] = 24;

	new_req->Device_Id = dev->base.id;
	new_req->Cmd_Flag = 0;
	new_req->Completion = ssp_ata_intl_req_callback;

	new_buf_ptr = core_map_data_buffer(new_req);

	MV_CopyMemory(&new_buf_ptr[4], &(org_buf_ptr[4 + org_buf_ptr[3]]), 20);

        /* Mode Data Length = 0 */
        /* When using the MODE SELECT command, this field is reserved */
	new_buf_ptr[0] = 0;
	new_buf_ptr[1] = 0;
	new_buf_ptr[2] = 0;
	new_buf_ptr[3] = 0;
	new_buf_ptr[4] &= 0x7F;

	switch (org_req->Cdb[0]) {
	case SCSI_CMD_MARVELL_SPECIFIC:
		if (org_req->Cdb[1] != CDB_CORE_MODULE)
		        MV_DASSERT(MV_FALSE);

		switch (org_req->Cdb[2]) {
		case CDB_CORE_ENABLE_WRITE_CACHE:
			new_buf_ptr[6] |= MV_BIT(2);
			break;

		case CDB_CORE_DISABLE_WRITE_CACHE:
			new_buf_ptr[6] &= ~MV_BIT(2);
			break;

		case CDB_CORE_ENABLE_READ_AHEAD:
			new_buf_ptr[16] &= ~MV_BIT(5);
			break;

		case CDB_CORE_DISABLE_READ_AHEAD:
			new_buf_ptr[16] |= MV_BIT(5);
			break;

		default:
			MV_DASSERT(MV_FALSE);
			break;
		}
		break;

	case APICDB0_PD:
		switch (org_req->Cdb[1]) {
		case APICDB1_PD_SETSETTING:
			switch (org_req->Cdb[4]) {
			case APICDB4_PD_SET_WRITE_CACHE_OFF:
				new_buf_ptr[6] &= ~MV_BIT(2);
				break;
			case APICDB4_PD_SET_WRITE_CACHE_ON:
				new_buf_ptr[6] |= MV_BIT(2);
				break;
			default:
				MV_DASSERT(MV_FALSE);
				break;
			}
			break;
		default:
			MV_DASSERT(MV_FALSE);
			break;
		}
		break;

		default:
		MV_DASSERT(MV_FALSE);
		break;
	}

	core_unmap_data_buffer(req);
	core_unmap_data_buffer(new_req);

	sas_replace_org_req(root, org_req, new_req);
}

MV_VOID ssp_prepare_task_management_command(pl_root *root,
	domain_device *dev, mv_command_header *cmd_header,
	mv_command_table *cmd_table,  MV_Request *req)
{
	MV_U64 val64;
	MV_U16 tag;
	MV_U32 ch_dword_0 = 0;

	/*
	 * CDB[3..4] = tag
	 * CDB[5] = task function
	 * CDB[6..13] = lun
	 */

	tag = (MV_U16)(req->Cdb[4]<<8) + (MV_U16)req->Cdb[3];
	cmd_header->frame_len = MV_CPU_TO_LE16(
		(((sizeof(ssp_task_iu) + sizeof(ssp_frame_header)) / 4) & CH_FRAME_LEN_MASK)
		| (1<<CH_MAX_SIMULTANEOUS_CONNECTIONS_SHIFT));
	cmd_header->tag |= req->Tag << root->max_cmd_slot_width;

	MV_CopyMemory(&cmd_table->table.ssp_cmd_table.data.task.lun, &req->Cdb[6], 8);
	cmd_table->table.ssp_cmd_table.data.task.tag = MV_CPU_TO_BE16(tag);
	cmd_table->table.ssp_cmd_table.data.task.task_function = req->Cdb[5];

        ((MV_PU16)cmd_table->table.ssp_cmd_table.data.task.reserved1)[0] = 0;
        cmd_table->table.ssp_cmd_table.data.task.reserved2 = 0;
        MV_ZeroMemory(cmd_table->table.ssp_cmd_table.data.task.reserved3, 14);

	ch_dword_0 |= FRAME_TYPE_TASK << CH_SSP_FRAME_TYPE_SHIFT;
	cmd_header->ctrl_nprd |= MV_CPU_TO_LE32(ch_dword_0);

	cmd_table->open_address_frame.frame_control = (ADDRESS_OPEN_FRAME << OF_FRAME_TYPE_SHIFT |\
		PROTOCOL_SSP << OF_PROT_TYPE_SHIFT | OF_MODE_INITIATOR << OF_MODE_SHIFT);
        ((MV_PU8)&cmd_table->open_address_frame)[1] = dev->negotiated_link_rate;
	*(MV_U16 *)(cmd_table->open_address_frame.connect_tag) =
		MV_CPU_TO_BE16(dev->base.id + 1);
	U64_ASSIGN(val64, MV_CPU_TO_BE64(dev->sas_addr));
	MV_CopyMemory(cmd_table->open_address_frame.dest_sas_addr, &val64, 8);
}

MV_VOID ssp_prepare_io_command(pl_root *root, domain_device *dev,
	mv_command_header *cmd_header, mv_command_table *cmd_table,
	MV_Request *req)
{
	MV_U64 val64;
	domain_enclosure *enc = NULL;
	MV_BOOLEAN is_enclosure = MV_FALSE;
	MV_U32 ch_dword_0 = 0;
	domain_base *base = (domain_base*)dev;
	
	mv_int_to_reqlun(base->LUN, req->lun);
	if(dev->base.type == BASE_TYPE_DOMAIN_ENCLOSURE){
		is_enclosure = MV_TRUE;
		enc = (domain_enclosure *)dev;
	}

	if(!is_enclosure) {
		if (dev->setting & DEVICE_SETTING_PI_ENABLED)
		{
			
			MV_ZeroMemory(&cmd_table->table.ssp_cmd_table.data.command.pir, sizeof(protect_info_record));
	              cmd_table->table.ssp_cmd_table.data.command.pir.USR_DT_SZ  =
				MV_CPU_TO_LE16((dev->sector_size - 8)/4);
			cmd_header->data_xfer_len +=
				MV_CPU_TO_LE32((cmd_header->data_xfer_len/
					(dev->sector_size-8))
					*8);

			switch(req->Cdb[0])
			{
			/*READ*/
			case SCSI_CMD_READ_10:
			case SCSI_CMD_READ_12:
			case SCSI_CMD_READ_16:
					cmd_table->table.ssp_cmd_table.data.command.pir.T10_CHK_EN = 1;
					cmd_table->table.ssp_cmd_table.data.command.pir.T10_CHK_MSK = 0xf3;
				
			/*VERIFY*/
			case SCSI_CMD_VERIFY_10:
			case SCSI_CMD_VERIFY_12:
			case SCSI_CMD_VERIFY_16:
			/*WRITE*/
			case SCSI_CMD_WRITE_10:
			case SCSI_CMD_WRITE_12:
			case SCSI_CMD_WRITE_16:
			/*WRITE&VERIFY*/
			case SCSI_CMD_WRITE_VERIFY_10:
			case SCSI_CMD_WRITE_VERIFY_12:
			case SCSI_CMD_WRITE_VERIFY_16:
			/*WRITE SAME*/
			case SCSI_CMD_WRITE_SAME_10:
			case SCSI_CMD_WRITE_SAME_16:
			/*XDWRITE*/
			case SCSI_CMD_XDWRITE_10:
			/*XPWRITE*/
			case SCSI_CMD_XPWRITE_10:
			/*XDREAD*/
			case SCSI_CMD_XDREAD_10:
			/*XDWRITEREAD*/
			case SCSI_CMD_XDWRITEREAD_10:
				if ((req->Cdb[0] & 0xf0) != 0x80) {
					cmd_table->table.ssp_cmd_table.data.command.pir.LBRT_GEN_VAL =
						MV_CPU_TO_LE32(req->Cdb[5] +
							(req->Cdb[4]<<8) +
							(req->Cdb[3]<<16) +
							(req->Cdb[2]<<24));
				} else {
					cmd_table->table.ssp_cmd_table.data.command.pir.LBRT_GEN_VAL =
						MV_CPU_TO_LE32(req->Cdb[9] +
							(req->Cdb[8]<<8) +
							(req->Cdb[7]<<16) +
							(req->Cdb[6]<<24));
				}
				cmd_table->table.ssp_cmd_table.data.command.pir.LBRT_CHK_VAL = 
					cmd_table->table.ssp_cmd_table.data.command.pir.LBRT_GEN_VAL;
				cmd_table->table.ssp_cmd_table.data.command.pir.INCR_LBRT = 1;
				cmd_table->table.ssp_cmd_table.data.command.pir.CHK_DSBL_MD = 1;
				
				if (req->EEDPFlags & 0x3)
					cmd_table->table.ssp_cmd_table.data.command.pir.T10_RMV_EN = 1;
				else if (req->EEDPFlags & 0x4)
					cmd_table->table.ssp_cmd_table.data.command.pir.T10_INSRT_EN = 1;
			
				cmd_header->pir_fmt = MV_CPU_TO_LE16(0xA000);
				ch_dword_0 |= CH_SSP_VERIFY_DATA_LEN;
					
			case SCSI_CMD_FORMAT_UNIT:
				ch_dword_0 |= CH_PI_PRESENT;
				break;
			default:
				ch_dword_0 &= (~ CH_SSP_VERIFY_DATA_LEN);
				break;
			}
		}
	}
	cmd_header->frame_len =
		MV_CPU_TO_LE16((((sizeof(ssp_command_iu) + sizeof(ssp_frame_header)+3)/4) & CH_FRAME_LEN_MASK)
		| (1<<CH_MAX_SIMULTANEOUS_CONNECTIONS_SHIFT));
	cmd_header->tag |= req->Tag << root->max_cmd_slot_width;
	ch_dword_0 |= FRAME_TYPE_COMMAND << CH_SSP_FRAME_TYPE_SHIFT;
	cmd_header->ctrl_nprd |= MV_CPU_TO_LE32(ch_dword_0);

	MV_ZeroMemory(&cmd_table->table.ssp_cmd_table.data.command.command_iu,
		sizeof(ssp_command_iu));
	MV_CopyMemory(&cmd_table->table.ssp_cmd_table.data.command.command_iu.cdb[0],
		&req->Cdb[0], 16);	
	MV_CopyMemory(&cmd_table->table.ssp_cmd_table.data.command.command_iu.lun[0],
		&req->lun[0], 8);

	cmd_table->open_address_frame.frame_control = (ADDRESS_OPEN_FRAME << OF_FRAME_TYPE_SHIFT |\
		PROTOCOL_SSP << OF_PROT_TYPE_SHIFT | OF_MODE_INITIATOR << OF_MODE_SHIFT);

	if(is_enclosure)
		((MV_PU8)&cmd_table->open_address_frame)[1] = enc->negotiated_link_rate;
	else
		((MV_PU8)&cmd_table->open_address_frame)[1] = dev->negotiated_link_rate;

	*(MV_U16 *)(cmd_table->open_address_frame.connect_tag) = MV_CPU_TO_BE16(dev->base.id + 1);

	if(is_enclosure)
		U64_ASSIGN(val64, MV_CPU_TO_BE64(enc->sas_addr));
	else
		U64_ASSIGN(val64, MV_CPU_TO_BE64(dev->sas_addr));

	MV_CopyMemory(cmd_table->open_address_frame.dest_sas_addr, &val64, 8);
}

MV_VOID ssp_prepare_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmd_header_p, MV_PVOID cmd_table_p,
	MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *dev = (domain_device *)dev_p;
	mv_command_header *cmd_header = (mv_command_header *)cmd_header_p;
	mv_command_table *cmd_table = (mv_command_table *)cmd_table_p;

	cmd_header->ctrl_nprd |= MV_CPU_TO_LE32(CH_SSP_TP_RETRY);
	cmd_header->max_rsp_frame_len = MV_CPU_TO_LE16(MAX_RESPONSE_FRAME_LENGTH >> 2);

	if (IS_A_TSK_REQ(req)) {
		ssp_prepare_task_management_command(
			root, dev, cmd_header, cmd_table, req);
	} else {
		ssp_prepare_io_command(
			root, dev, cmd_header, cmd_table, req);
	}
}

MV_VOID ssp_send_command(MV_PVOID root_p, MV_PVOID dev_p, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *dev = (domain_device *)dev_p;
	domain_port *port = dev->base.port;
	core_context *ctx = req->Context[MODULE_CORE];

	MV_U8 phy_map;
	MV_U32 entry_nm;

	entry_nm = prot_get_delv_q_entry(root);

	if (CORE_IS_EH_REQ(ctx))
		phy_map = port->asic_phy_map;
	else
		phy_map = core_wideport_load_balance_asic_phy_map(port, dev);

	if (IS_A_TSK_REQ(req)) {
		root->delv_q[entry_nm] = MV_CPU_TO_LE32(TXQ_MODE_I | ctx->slot | \
			phy_map  << TXQ_PHY_SHIFT | TXQ_CMD_SSP |\
		 				1 << TXQ_PRIORITY_SHIFT);
	} else {
		root->delv_q[entry_nm] = MV_CPU_TO_LE32(TXQ_MODE_I | ctx->slot | \
			phy_map  << TXQ_PHY_SHIFT | TXQ_CMD_SSP);
	}

	prot_write_delv_q_entry(root, entry_nm);
}

/* this includes error handling for SMP, STP and SSP*/
MV_VOID sas_process_command_error(pl_root *root, domain_base *base,
	mv_command_table *cmd_table, err_info_record *err_info, MV_Request *req)
{
	domain_port *port = base->port;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_U32 error = MV_LE32_TO_CPU(err_info->err_info_field_1);

	MV_ASSERT(base == get_device_by_id(root->lib_dev, req->Device_Id));

	if (error & CMD_ISS_STPD){
		ctx->error_info |= EH_INFO_CMD_ISS_STPD;
		base->cmd_issue_stopped = MV_TRUE;
	}

	if (error & NO_DEST_ERR){
		req->Scsi_Status = REQ_STATUS_NO_DEVICE;
		/* suppose CMD_ISS_STPD is not set. otherwise need start issue somewhere */
		MV_ASSERT(!(ctx->error_info & EH_INFO_NEED_RETRY));
		return;
	}

#define SAS_TIMEOUT_REASON (OPEN_TMOUT_ERR| NAK_ERR | ACK_NAK_TO | RD_DATA_OFFST_ERR |\
		XFR_RDY_OFFST_ERR | UNEXP_XFER_RDY_ERR | DATA_OVR_UNDR_FLW_ERR | WD_TMR_TO_ERR |\
		SYNC_ERR | TX_STOPPED_EARLY | CNCTN_RT_NT_SPRTD_ERR)

        if (error & SAS_TIMEOUT_REASON) {
			if (IS_A_SMP_REQ(req)) {
				req->Scsi_Status = REQ_STATUS_ERROR;
			} else {
				req->Scsi_Status = REQ_STATUS_TIMEOUT;
			}

			if (error & (WD_TMR_TO_ERR | TX_STOPPED_EARLY))
				ctx->error_info |= EH_INFO_WD_TO_RETRY;

	        ctx->error_info |= EH_INFO_NEED_RETRY;
		return;
	}

	if (error & TFILE_ERR) {
		/* SSP or STP requests */
		MV_DASSERT(!IS_A_SMP_REQ(req));
		sata_handle_taskfile_error(root, req);
		return;
	}

	if (error & RESP_BFFR_OFLW) {
		CORE_EH_PRINT(("Response Buffer Overflow Condition meet.\n"));
	}
	MV_ASSERT(((MV_U64 *)err_info)->value != 0);

	CORE_EH_PRINT(("attention: req %p[0x%x] unknown error 0x%08x 0x%08x! "\
		"treat as media error.\n",\
		req, req->Cdb[0],
		((MV_U64 *)err_info)->parts.high, \
		((MV_U64 *)err_info)->parts.low));

	req->Scsi_Status = REQ_STATUS_ERROR;
	ctx->error_info |= EH_INFO_NEED_RETRY;
	return;
}

typedef struct mv_scsi_sense_hdr {
        MV_U8 response_code;       /* permit: 0x0, 0x70, 0x71, 0x72, 0x73 */
        MV_U8 sense_key;
        MV_U8 asc;
        MV_U8 ascq;
}*MV_PSB_HDR,MV_SB_HDR;

MV_BOOLEAN ssp_sense_valid(MV_PSB_HDR sshdr)
{
	if (!sshdr)
		return MV_FALSE;

	return (sshdr->response_code & 0x70) == 0x70 ;
}

MV_BOOLEAN ssp_normalize_sense(MV_PU8 sense_buffer, MV_U32 sb_len, MV_PSB_HDR sshdr)
{
	if (!sense_buffer || !sb_len)
		return MV_FALSE;

	MV_ZeroMemory(sshdr,sizeof(*sshdr));

	sshdr->response_code = (sense_buffer[0] & 0x7f);

	if (!ssp_sense_valid(sshdr))
		return MV_FALSE;

	if (sshdr->response_code >= 0x72) {
                if (sb_len > 1)
                        sshdr->sense_key = (sense_buffer[1] & 0xf);
                if (sb_len > 2)
                        sshdr->asc = sense_buffer[2];
                if (sb_len > 3)
                        sshdr->ascq = sense_buffer[3];
        } else {
                if (sb_len > 2)
                        sshdr->sense_key = (sense_buffer[2] & 0xf);
                if (sb_len > 7) {
                        sb_len = (sb_len < (MV_U32)(sense_buffer[7] + 8)) ?
                                         sb_len : (MV_U32)(sense_buffer[7] + 8);
                        if (sb_len > 12)
                                sshdr->asc = sense_buffer[12];
                        if (sb_len > 13)
                                sshdr->ascq = sense_buffer[13];
                }
        }
	return MV_TRUE;
}

/* if return MV_TRUE, means we want to retry this request */
MV_BOOLEAN parse_sense_error(MV_PU8 sense_buffer, MV_U32 sb_len)
{
	MV_SB_HDR sshdr;

	if(!ssp_normalize_sense(sense_buffer, sb_len, &sshdr))
		return MV_FALSE;

	/* Mode Parameters changed */
	if (sshdr.sense_key == SCSI_SK_UNIT_ATTENTION &&
		sshdr.asc == 0x2a && sshdr.ascq == 0x01)
		return MV_TRUE;

	return MV_FALSE;
}

MV_VOID ssp_process_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmpl_q_p, MV_PVOID cmd_table_p,
	MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *dev = (domain_device *)dev_p;
	 core_extension *core = (core_extension *)root->core;
	MV_U32 cmpl_q = *(MV_PU32)cmpl_q_p;
	mv_command_table *cmd_table = (mv_command_table *)cmd_table_p;
	err_info_record *err_info;
	ssp_response_iu *resp_iu;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_U32 sense_length;
	MV_SB_HDR sshdr;
	domain_device *device;
	MV_Sense_Data *sense_data;

	err_info = prot_get_command_error_info(cmd_table, &cmpl_q);
	if (err_info) {
		CORE_EH_PRINT(("dev %d.\n", req->Device_Id));
		sas_process_command_error(
			root, &dev->base, cmd_table, err_info, req);
		return;
	}

	if ((req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC) &&
		(req->Cdb[1] == CDB_CORE_MODULE)) {
		switch (req->Cdb[2])
		{
		}
	}

	resp_iu = &cmd_table->status_buff.data.ssp_resp;
	/* if response frame is not transferred, success */
	/* if response frame is transfered and status is GOOD, success */
	if (!(cmpl_q & RXQ_RSPNS_XFRD) || (cmpl_q & RXQ_RSPNS_XFRD && \
		(resp_iu->status==SCSI_STATUS_GOOD))) {
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		return;
	}

	sense_length = MV_BE32_TO_CPU(resp_iu->sense_data_len);
	if( sense_length && (resp_iu->data_pres & SENSE_ONLY) &&
		ssp_normalize_sense(resp_iu->data, sense_length, &sshdr)) {
			CORE_EH_PRINT(("dev %d req 0x%x status 0x%x sense length %d " \
				"sense(key 0x%x, asc 0x%x ascq 0x%x).\n", \
			        dev->base.id,req->Cdb[0], resp_iu->status, sense_length, \
			sshdr.sense_key, sshdr.asc, sshdr.ascq));

	}

	switch (resp_iu->status) {
	case SCSI_STATUS_CHECK_CONDITION:
		if ((req->Cdb[0] == SCSI_CMD_REPORT_LUN) && (IS_ENCLOSURE(dev))) {
			MV_U8 *buf = core_map_data_buffer(req);

			if (buf != NULL) {
				MV_ZeroMemory(buf, req->Data_Transfer_Length);
				buf[3] = 0x08;
				req->Scsi_Status = REQ_STATUS_SUCCESS;
	                	core_unmap_data_buffer(req);
				return;
	        	} else {
	       		core_unmap_data_buffer(req);
	       	}
		}else{
			if (req->Cdb[0] == SCSI_CMD_START_STOP_UNIT) {
				HBA_SleepMillisecond(NULL,500);
			}
			sense_data = (MV_Sense_Data *)&resp_iu->data[0];
			if(sense_data->SenseKey == UNIT_ATTENTION ){
				if((sense_data->AdditionalSenseCode== 0x3F) && (sense_data->AdditionalSenseCodeQualifier ==0x0e)){
					CORE_PRINT(("REPORT LUN DATA Changed\n"));
					device =(domain_device *) get_device_by_id(root->lib_dev,
						get_id_by_targetid_lun(core, get_device_targetid(root->lib_dev, req->Device_Id), 0));
					if(device == NULL)
						break;
					if(device->state == DEVICE_STATE_INIT_DONE){
						device->state = DEVICE_STATE_REPORT_LUN;	
                                			core_queue_init_entry(root, &device->base, MV_FALSE);
              				}
				}
			}	
		}

		if (parse_sense_error(resp_iu->data, sense_length)) {
			req->Scsi_Status = REQ_STATUS_ERROR;
			ctx->error_info |= EH_INFO_NEED_RETRY;
		} else {
			sense_length = MV_MIN(req->Sense_Info_Buffer_Length,
				sense_length);
			if (sense_length != 0) {
				req->Scsi_Status = REQ_STATUS_HAS_SENSE;

				MV_CopyMemory(req->Sense_Info_Buffer,
					resp_iu->data, sense_length);
			} else {
				req->Scsi_Status = REQ_STATUS_ERROR;
			}
		}
		break;

	case SCSI_STATUS_FULL:
		if (dev->base.outstanding_req > 1) {
			dev->base.queue_depth = dev->base.outstanding_req - 1;
		} else {
			dev->base.queue_depth = 1;
		}

		CORE_DPRINT(("set queue depth to %d.\n", dev->base.queue_depth));
		req->Scsi_Status = REQ_STATUS_ERROR;
		ctx->error_info |= EH_INFO_NEED_RETRY;
		break;

	default:
		req->Scsi_Status = REQ_STATUS_ERROR;
		break;
	}

        if (req->Scsi_Status != REQ_STATUS_SUCCESS)
                core_generate_error_event(root->core, req);
}

extern MV_VOID sata_prepare_command_header(MV_Request *req,
	mv_command_header *cmd_header, mv_command_table *cmd_table,
	MV_U8 tag, MV_U8 pm_port);
extern MV_VOID sata_prepare_command_table(MV_Request *req,
	mv_command_table *cmd_table, ata_taskfile *taskfile, MV_U8 pm_port);

MV_VOID stp_prepare_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmd_header_p, MV_PVOID cmd_table_p, MV_Request *req)
{
	domain_device *device = (domain_device *)dev_p;
	mv_command_header *cmd_header = (mv_command_header *)cmd_header_p;
	mv_command_table *cmd_table = (mv_command_table *)cmd_table_p;
	ata_taskfile taskfile;
	MV_BOOLEAN ret;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_U8 tag;
	MV_U64 val64;

	if (req->Cmd_Flag & CMD_FLAG_NCQ) {
		sata_get_ncq_tag(device, req);
		tag = ctx->ncq_tag;
	} else {
		tag = req->Tag;
	}

	ret = ata_fill_taskfile(device, req, tag, &taskfile);

	if (!ret) {
		MV_DASSERT(MV_FALSE);
	}

	sata_prepare_command_header(req, cmd_header, cmd_table, tag,
		device->pm_port);
	sata_prepare_command_table(req, cmd_table, &taskfile, device->pm_port);

	cmd_table->open_address_frame.frame_control = (ADDRESS_OPEN_FRAME << OF_FRAME_TYPE_SHIFT |\
		PROTOCOL_STP << OF_PROT_TYPE_SHIFT | OF_MODE_INITIATOR << OF_MODE_SHIFT);

        ((MV_PU8)&cmd_table->open_address_frame)[1] = device->negotiated_link_rate;
	*(MV_U16 *)cmd_table->open_address_frame.connect_tag =
		MV_CPU_TO_BE16(device->base.id+1);
	U64_ASSIGN(val64, MV_CPU_TO_BE64(device->sas_addr));
	MV_CopyMemory(cmd_table->open_address_frame.dest_sas_addr, &val64, 8);
}

MV_VOID stp_process_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmpl_q_p, MV_PVOID cmd_table_p,
	MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *dev = (domain_device *)dev_p;
	MV_U32 cmpl_q = *(MV_PU32)cmpl_q_p;
	mv_command_table *cmd_table = (mv_command_table *)cmd_table_p;
	err_info_record *err_info;

	err_info = prot_get_command_error_info(cmd_table, &cmpl_q);
	if (err_info) {
		CORE_EH_PRINT(("dev %d, req %p has error info.\n", req->Device_Id, req));
		MV_DumpRequest(req, 0);		
		sas_process_command_error(
			root, &dev->base, cmd_table, err_info, req);
		return;
	}

	req->Scsi_Status = REQ_STATUS_SUCCESS;
	return;
}

MV_U8 smp_translate_reset_sata_phy_req(pl_root *root,
	domain_expander *exp, MV_Request *req)
{
	MV_Request *new_req = NULL;
	core_context *ctx, *phy_ctx;
        domain_device *tmp_dev;

	ctx = req->Context[MODULE_CORE];
	if (ctx->type != CORE_CONTEXT_TYPE_RESET_SATA_PHY) MV_ASSERT(MV_FALSE);
	ctx->u.smp_reset_sata_phy.req_remaining = 0;
        req->Scsi_Status = REQ_STATUS_SUCCESS;

	for ( ; ctx->u.smp_reset_sata_phy.curr_dev_count < 
                ctx->u.smp_reset_sata_phy.total_dev_count;
		ctx->u.smp_reset_sata_phy.curr_dev_count++) {

		tmp_dev = List_GetFirstEntry(&exp->device_list, domain_device, 
                        base.exp_queue_pointer);
                if (IS_STP(tmp_dev)) {
                        new_req = smp_make_phy_crtl_by_id_req(exp, 
			        tmp_dev->parent_phy_id,
			        LINK_RESET, 
					smp_physical_req_callback);
                } else {
                        List_AddTail(&tmp_dev->base.exp_queue_pointer, &exp->device_list);
                        continue;
                }

                if (new_req) {
                        List_AddTail(&tmp_dev->base.exp_queue_pointer, &exp->device_list);

	                phy_ctx = (core_context *)new_req->Context[MODULE_CORE];
	                phy_ctx->type = CORE_CONTEXT_TYPE_ORG_REQ; 
	                phy_ctx->u.org.org_req = req;

	                if (CORE_IS_INIT_REQ(ctx))
		                core_append_init_request(root, new_req);
	                else if (CORE_IS_EH_REQ(ctx))
		                core_queue_eh_req(root, new_req);
	                else
		                core_append_request(root, new_req);

	                ctx->u.smp_reset_sata_phy.req_remaining++;
                        
                } else {
                        List_Add(&tmp_dev->base.exp_queue_pointer, &exp->device_list);

	                if (ctx->u.smp_reset_sata_phy.req_remaining != 0) {
		                return MV_QUEUE_COMMAND_RESULT_REPLACED;
	                } else {
		                return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
	                }
                }
        }
	
	if (ctx->u.smp_reset_sata_phy.req_remaining > 0)
		return MV_QUEUE_COMMAND_RESULT_REPLACED;
	
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_U8 smp_translate_clear_aff_all_req(pl_root *root,
	domain_expander *exp, MV_Request *req)
{
	MV_Request *new_req = NULL;
	core_context *ctx, *phy_ctx;
	domain_base *base;
	domain_device *tmp_dev;
	MV_U8 state;

	ctx = req->Context[MODULE_CORE];
	MV_ASSERT(ctx->type == CORE_CONTEXT_TYPE_CLEAR_AFFILIATION);
	ctx->u.smp_clear_aff.req_remaining = 0;
	state = ctx->u.smp_clear_aff.state;
        req->Scsi_Status = REQ_STATUS_SUCCESS;

        for ( ; ctx->u.smp_clear_aff.curr_dev_count < 
                ctx->u.smp_clear_aff.total_dev_count;
                ctx->u.smp_clear_aff.curr_dev_count++) {

                switch (state) {
                case CLEAR_AFF_STATE_REPORT_PHY_SATA:

                        tmp_dev = List_GetFirstEntry(&exp->device_list, domain_device, 
                                base.exp_queue_pointer);
                        if (IS_STP(tmp_dev)) {
                                new_req = smp_make_report_phy_sata_by_id_req(exp, 
											tmp_dev->parent_phy_id, 
											smp_physical_req_callback);
                                if (new_req) {
                                        List_AddTail(&tmp_dev->base.exp_queue_pointer,
                                                &exp->device_list);
                                } else {
                                        List_Add(&tmp_dev->base.exp_queue_pointer,
                                                &exp->device_list);
                                }
                        } else {
                                List_AddTail(&tmp_dev->base.exp_queue_pointer,
                                        &exp->device_list);
                                continue;
                        }
	                break;

                case CLEAR_AFF_STATE_CLEAR_AFF:
                        tmp_dev = List_GetFirstEntry(&exp->device_list, domain_device, 
                                base.exp_queue_pointer);
                        if (tmp_dev->status & DEVICE_STATUS_NEED_RESET) {
                                new_req = smp_make_phy_crtl_by_id_req(exp, 
			                tmp_dev->parent_phy_id,
			                HARD_RESET,
			                smp_physical_req_callback);
                        } else if (tmp_dev->status & DEVICE_STATUS_NEED_CLEAR) {
		                new_req = smp_make_phy_crtl_by_id_req(exp, 
			                tmp_dev->parent_phy_id,
			                CLEAR_AFFILIATION,
			                smp_physical_req_callback);
                        } else {
                                List_AddTail(&tmp_dev->base.exp_queue_pointer,
                                        &exp->device_list);
                                continue;
                        }

                        if (new_req) {
                                tmp_dev->status &= ~(DEVICE_STATUS_NEED_RESET | 
                                        DEVICE_STATUS_NEED_CLEAR);
                                List_AddTail(&tmp_dev->base.exp_queue_pointer,
                                        &exp->device_list);
                        } else {
                                List_Add(&tmp_dev->base.exp_queue_pointer,
                                        &exp->device_list);
                        }
	                ctx->u.smp_clear_aff.need_wait = MV_TRUE;
	                break;
                default:
	                MV_ASSERT(MV_FALSE);
	                break;
                }

	        if (new_req) {
		        phy_ctx = (core_context *)new_req->Context[MODULE_CORE];
		        phy_ctx->type = CORE_CONTEXT_TYPE_ORG_REQ; 
		        phy_ctx->u.org.org_req = req;

		        if (CORE_IS_INIT_REQ(ctx))
			        core_append_init_request(root, new_req);
		        else if (CORE_IS_EH_REQ(ctx))
			        core_queue_eh_req(root, new_req);
		        else
			        core_append_request(root, new_req);

		        ctx->u.smp_clear_aff.req_remaining++;
	        } else {
		        if (ctx->u.smp_clear_aff.req_remaining != 0) {
			        return MV_QUEUE_COMMAND_RESULT_REPLACED;
		        } else {
			        return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
		        }
                }
        }

	if (ctx->u.smp_clear_aff.req_remaining > 0)
		return MV_QUEUE_COMMAND_RESULT_REPLACED;
	
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_U8 smp_translate_phy_req(pl_root *root, domain_expander *exp,
	MV_Request *req)
{
	MV_Request *new_req = NULL;
	core_context *ctx;
	smp_virtual_config_route_buffer *buf;
	MV_U8 i, j, phy_id;

	switch (req->Cdb[2]) {
	case CDB_CORE_SMP_VIRTUAL_DISCOVER:
		ctx = req->Context[MODULE_CORE];
		CORE_DPRINT(("discover: phy count = %d\n", exp->phy_count));
		ctx->u.smp_discover.req_remaining = 0;
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		for ( ; ctx->u.smp_discover.current_phy_id < exp->phy_count;
			ctx->u.smp_discover.current_phy_id++) {
			new_req = smp_make_discover_req(exp, req,
				ctx->u.smp_discover.current_phy_id, smp_physical_req_callback);
			if (new_req) {
				core_append_init_request(root, new_req);
				ctx->u.smp_discover.req_remaining++;
			} else {
				if (ctx->u.smp_discover.req_remaining != 0) {
					return MV_QUEUE_COMMAND_RESULT_REPLACED;
				} else {
					return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
				}
			}
		}
		if (ctx->u.smp_discover.req_remaining == 0)
			MV_ASSERT(MV_FALSE);
		return MV_QUEUE_COMMAND_RESULT_REPLACED;

	case CDB_CORE_SMP_VIRTUAL_CONFIG_ROUTE:
		ctx = req->Context[MODULE_CORE];
		buf = (smp_virtual_config_route_buffer *)core_map_data_buffer(req);

		CORE_DPRINT(("smp config: phy %d/%d addr %d/%d.\n",\
			ctx->u.smp_config_route.current_phy, \
			ctx->u.smp_config_route.phy_count,\
			ctx->u.smp_config_route.current_addr, \
			ctx->u.smp_config_route.address_count));
		ctx->u.smp_config_route.req_remaining = 0;
                req->Scsi_Status = REQ_STATUS_SUCCESS;
		for (i=ctx->u.smp_config_route.current_phy;
			i<ctx->u.smp_config_route.phy_count; i++) {
			phy_id = buf->phy_id[i];

			for (j=ctx->u.smp_config_route.current_addr;
				j<ctx->u.smp_config_route.address_count; j++) {

				new_req = smp_make_config_route_req(exp, req,
					exp->route_table[phy_id], phy_id, &buf->sas_addr[j],
					smp_physical_req_callback);
				if (new_req) {
					exp->route_table[phy_id]++;
					core_append_init_request(root, new_req);
					ctx->u.smp_config_route.req_remaining++;
				} else {
					ctx->u.smp_config_route.current_phy = i;
					ctx->u.smp_config_route.current_addr = j;
                                        core_unmap_data_buffer(req);
					if (ctx->u.smp_config_route.req_remaining == 0) {
						return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
					} else {
						return MV_QUEUE_COMMAND_RESULT_REPLACED;
					}
				}
			}
			ctx->u.smp_config_route.current_addr = 0;
		}
		ctx->u.smp_config_route.current_phy = ctx->u.smp_config_route.phy_count;
		ctx->u.smp_config_route.current_addr = ctx->u.smp_config_route.address_count;
                core_unmap_data_buffer(req);
		if (ctx->u.smp_config_route.req_remaining == 0)
			MV_ASSERT(MV_FALSE);
		return MV_QUEUE_COMMAND_RESULT_REPLACED;

	case CDB_CORE_SMP_VIRTUAL_RESET_SATA_PHY:
		return (smp_translate_reset_sata_phy_req(root, exp, req));
	case CDB_CORE_SMP_VIRTUAL_CLEAR_AFFILIATION_ALL:
		return (smp_translate_clear_aff_all_req(root, exp,req));
	default:
		break;
	}

	req->Scsi_Status = REQ_STATUS_NO_DEVICE;
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}	

MV_U8 smp_verify_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_expander *exp = (domain_expander *)dev_p;

	if ((req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC) &&
		(req->Cdb[1] == CDB_CORE_MODULE) &&
		(req->Cdb[2] == CDB_CORE_RESET_PORT)) {
			mv_reset_phy(root, req->Cdb[3], MV_TRUE);
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
	
	if ((req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC) &&
		(req->Cdb[1] == CDB_CORE_MODULE) &&
		(req->Cdb[2] != CDB_CORE_SMP)) {
		
		return (smp_translate_phy_req(root, exp, req));
	} else {
		if (req->Data_Buffer == NULL) {
			CORE_DPRINT(("smp req no data buffer.\n"));
			req->Scsi_Status = REQ_STATUS_ERROR;
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}
	}

	return MV_QUEUE_COMMAND_RESULT_PASSED;
}

MV_U32 smp_get_request_len(MV_U8 function)
{
	MV_U32 req_len = 4;

	switch (function) {
	case REPORT_GENERAL:
	case REPORT_MANUFACTURER_INFORMATION:
		req_len += sizeof(struct SMPRequestGeneralInput) - sizeof(MV_U32);
		break;
	case REPORT_SELF_CONFIGURATION_STATUS:
		req_len += sizeof(struct SMPRequestSelfConfigurationInput) -
			sizeof(MV_U32);
		break;
	case DISCOVER:
	case REPORT_PHY_ERROR_LOG:
	case REPORT_PHY_SATA:
	case REPORT_PHY_EVENT_INFORMATION:
	case REPORT_PHY_BROADCAST_COUNTS:
		req_len += sizeof(struct SMPRequestPhyInput) - sizeof(MV_U32);
		break;
	case REPORT_ROUTE_INFORMATION:
		req_len += sizeof(struct SMPRequestRouteInformationInput) -
			sizeof(MV_U32);
		break;
	case DISCOVER_LIST:
		req_len += sizeof(struct SMPRequestDiscoverList) - sizeof(MV_U32);
		break;
	case REPORT_EXPANDER_ROUTE_TABLE:
		req_len += sizeof(struct SMPRequestReportExpanderRouteTable) -
			sizeof(MV_U32);
		break;
	case CONFIGURE_GENERAL:
		req_len += sizeof(struct SMPRequestConfigureGeneral) - sizeof(MV_U32);
		break;
	case ENABLE_DISABLE_ZONING:
		req_len += sizeof(struct SMPRequestEnableDisableZoning) -
			sizeof(MV_U32);
		break;
	case ZONED_BROADCAST:
		req_len += sizeof(struct SMPRequestZonedBroadcast) - sizeof(MV_U32);
		break;
	case CONFIGURE_ROUTE_INFORMATION:
		req_len += sizeof(struct SMPRequestConfigureRouteInformation) -
			sizeof(MV_U32);
		break;
	case PHY_CONTROL:
		req_len += sizeof(struct SMPRequestPhyControl) - sizeof(MV_U32);
		break;
	case PHY_TEST:
		req_len += sizeof(struct SMPRequestPhyTest) - sizeof(MV_U32);
		break;
	case CONFIGURE_PHY_EVENT_INFORMATION:
		req_len += sizeof(struct SMPRequestConfigurePhyEventInformation) -
			sizeof(MV_U32);
		break;
	}

	return req_len;
}

MV_U32 smp_get_response_len(MV_U8 function)
{
	MV_U32 resp_len = 4;

	switch (function) {
	case REPORT_GENERAL:
		resp_len += sizeof(struct SMPResponseReportGeneral) - sizeof(MV_U32);
		break;
	case REPORT_MANUFACTURER_INFORMATION:
		resp_len += sizeof(struct SMPResponseReportManufacturerInformation) -
			sizeof(MV_U32);
		break;
	case REPORT_SELF_CONFIGURATION_STATUS:
		resp_len += sizeof(struct SMPResponseReportSelfConfigurationStatus) -
			sizeof(MV_U32);
		break;
	case DISCOVER:
		resp_len += sizeof(struct SMPResponseDiscover) - sizeof(MV_U32);
		break;
	case REPORT_PHY_ERROR_LOG:
		resp_len += sizeof(struct SMPResponseReportPhyErrorLog) -
			sizeof(MV_U32);
		break;
	case REPORT_PHY_SATA:
		resp_len += sizeof(struct SMPResponseReportPhySATA) - sizeof(MV_U32);
		break;
	case REPORT_ROUTE_INFORMATION:
		resp_len += sizeof(struct SMPResponseReportRouteInformation) -
			sizeof(MV_U32);
		break;
	case REPORT_PHY_EVENT_INFORMATION:
		resp_len += sizeof(struct SMPResponseReportPhyEventInformation) -
			sizeof(MV_U32);
		break;
	case REPORT_PHY_BROADCAST_COUNTS:
		resp_len += sizeof(struct SMPResponseReportBroadcastCounts) -
			sizeof(MV_U32);
		break;
	case DISCOVER_LIST:
		resp_len += sizeof(struct SMPResponseDiscoverList) - sizeof(MV_U32);
		break;
	case REPORT_EXPANDER_ROUTE_TABLE:
		resp_len += sizeof(struct SMPResponseReportExpanderTable) -
			sizeof(MV_U32);
		break;
	case CONFIGURE_GENERAL:
	case ENABLE_DISABLE_ZONING:
	case ZONED_BROADCAST:
	case CONFIGURE_ROUTE_INFORMATION:
	case PHY_CONTROL:
	case PHY_TEST:
	case CONFIGURE_PHY_EVENT_INFORMATION:
		resp_len += sizeof(struct SMPResponseConfigureFunction) -
			sizeof(MV_U32);
		break;
	}

	return resp_len;
}

MV_VOID smp_prepare_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmd_header_p, MV_PVOID cmd_table_p,
	MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_expander *exp = (domain_expander *)dev_p;
	mv_command_header *cmd_header = (mv_command_header *)cmd_header_p;
	mv_command_table *cmd_table = (mv_command_table *)cmd_table_p;
	MV_PVOID buf_ptr = core_map_data_buffer(req);
	smp_request *smp_req = (smp_request *)buf_ptr;
	MV_U32 req_len;
	MV_U64 val64;

	cmd_header->max_rsp_frame_len = MAX_RESPONSE_FRAME_LENGTH >> 2;
	req_len = smp_get_request_len(smp_req->function);
	cmd_header->frame_len = MV_CPU_TO_LE16(((MV_U16)(req_len+3)/4) | (1<<CH_MAX_SIMULTANEOUS_CONNECTIONS_SHIFT));
	cmd_header->tag |= req->Tag << root->max_cmd_slot_width;

	cmd_table->open_address_frame.frame_control = (ADDRESS_OPEN_FRAME << OF_FRAME_TYPE_SHIFT |\
		PROTOCOL_SMP << OF_PROT_TYPE_SHIFT | OF_MODE_INITIATOR << OF_MODE_SHIFT);
	*(MV_U16 *)(cmd_table->open_address_frame.connect_tag) = MV_CPU_TO_BE16(0xffff);
	((MV_PU8)&cmd_table->open_address_frame)[1] = exp->neg_link_rate;
	U64_ASSIGN(val64, MV_CPU_TO_BE64(exp->sas_addr));
	MV_CopyMemory(cmd_table->open_address_frame.dest_sas_addr,
		&val64, 8);

	MV_CopyMemory(&cmd_table->table.smp_cmd_table, buf_ptr, req_len);
	core_unmap_data_buffer(req);
}

MV_VOID smp_send_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_expander *exp = (domain_expander *)dev_p;
	domain_port *port = exp->base.port;
	core_context *ctx = req->Context[MODULE_CORE];

	MV_U8 phy_map;
	MV_U32 entry_nm;

	entry_nm = prot_get_delv_q_entry(root);
	phy_map = port->asic_phy_map;

	root->delv_q[entry_nm] = MV_CPU_TO_LE32(TXQ_MODE_I | ctx->slot | \
		phy_map << TXQ_PHY_SHIFT | TXQ_CMD_SMP);

	prot_write_delv_q_entry(root, entry_nm);
}

MV_VOID smp_process_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_PVOID cmpl_q_p, MV_PVOID cmd_table_p,
	MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	domain_expander *exp = (domain_expander *)dev_p;
	MV_U32 cmpl_q = *(MV_PU32)cmpl_q_p;
	mv_command_table *cmd_table = (mv_command_table *)cmd_table_p;
	err_info_record *err_info;
	smp_response *smp_resp =
		(smp_response *)cmd_table->status_buff.data.smp_resp;
	MV_U32 resp_len;
	MV_PVOID buf_ptr;
	domain_port *port = exp->base.port;
	MV_U8 i;

	err_info = prot_get_command_error_info(cmd_table, &cmpl_q);
	if (err_info) {
		CORE_EH_PRINT(("dev %d has error info.\n", req->Device_Id));
		/* error information record has some fields set. */
		sas_process_command_error(
			root, &exp->base, cmd_table, err_info, req);
		return;
	}

	if (smp_resp->function_result == SMP_FUNCTION_ACCEPTED) {
		req->Scsi_Status = REQ_STATUS_SUCCESS;
                buf_ptr = core_map_data_buffer(req);
		if (buf_ptr != NULL) {
			if (cmd_table->status_buff.data.smp_resp[3] == 0) {
				resp_len = smp_get_response_len(smp_resp->function);
			} else {
				/* get response len from RESPONSE LENGTH field of response data */
				resp_len =4;
				resp_len += cmd_table->status_buff.data.smp_resp[3] * 4;
			}
			resp_len = (resp_len < req->Data_Transfer_Length) ? resp_len :
				req->Data_Transfer_Length;
			MV_CopyMemory(buf_ptr, &cmd_table->status_buff.data.smp_resp,
				resp_len);
		}
		core_unmap_data_buffer(req);
	} else {
                req->Scsi_Status = REQ_STATUS_SUCCESS;
	}
}



