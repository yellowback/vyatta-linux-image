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
#include "com_error.h"
#include "core_error.h"
#include "core_sata.h"
#include "core_type.h"
#include "core_internal.h"
#include "core_protocol.h"
#include "core_device.h"
#include "core_exp.h"
#include "core_sat.h"
#include "core_util.h"
#include "core_manager.h"
#include "core_expander.h"

#include "hba_exp.h"
#include "com_define.h"

extern saved_fis *sata_get_received_fis(pl_root *root,	MV_U8 register_set);
MV_BOOLEAN bad_atapi_device_model(domain_device *device);
MV_VOID scsi_ata_translation_callback(MV_PVOID root_p, MV_Request *req);
void scsi_ata_check_condition(MV_Request *req, MV_U8 sense_key,
	MV_U8 sense_code, MV_U8 sense_qualifier);

/*******************************************************************************
*       ATA PASSTHRU STATUS RETURN DESCRIPTOR                                  *
*     ___________________________________________                              *
*     0 |         DESCRIPTOR CODE (0x09)         |                             *
*     __|________________________________________|                             *
*     1 |   ADDITIONAL DESCRIPTOR LENGTH (0x0C)  |                             *
*     __|________________________________________|                             *
*     2 |             RESERVED             | EXT |                             *
*     __|__________________________________|_____|                             *
*     3 |                 ERROR                  |                             *
*     __|________________________________________|                             *
*     4 |            SECTOR COUNT EXT            |                             *
*     __|________________________________________|                             *
*     5 |              SECTOR COUNT              |                             *
*     __|________________________________________|                             *
*     6 |              LBA LOW EXT               |                             *
*     __|________________________________________|                             *
*     7 |                LBA LOW                 |                             *
*     __|________________________________________|                             *
*     8 |              LBA MID EXT               |                             *
*     __|________________________________________|                             *
*     9 |                LBA MID                 |                             *
*     __|________________________________________|                             *
*     10|              LBA HIGH EXT              |                             *
*     __|________________________________________|                             *
*     11|                LBA HIGH                |                             *
*     __|________________________________________|                             *
*     12|                 DEVICE                 |                             *
*     __|________________________________________|                             *
*     13|                 STATUS                 |                             *
*     __|________________________________________|                             *
*                                                                              *
*******************************************************************************/
MV_U8 ata_return_descriptor[ATA_RETURN_DESCRIPTOR_LENGTH];

MV_VOID sat_make_sense(MV_PVOID root_p, MV_Request *req, MV_U8 status,
        MV_U8 error, MV_U64 *lba)
{
	pl_root *root = root_p;
	core_extension *core = (core_extension *)root->core;

	if ((req->Sense_Info_Buffer_Length == 0)
		|| (req->Sense_Info_Buffer == NULL)) {

		if (status & (ATA_STATUS_DF || ATA_STATUS_ERR))
			req->Scsi_Status = REQ_STATUS_ERROR;
		return;
	}

	if (status & ATA_STATUS_DF)
		scsi_ata_check_condition(req,
			SCSI_SK_HARDWARE_ERROR,
			SCSI_ASC_INTERNAL_TARGET_FAILURE, 0);
	else if (status & ATA_STATUS_ERR) {
		if (error & ATA_ERR_UNC) {
			scsi_ata_check_condition(req,
				SCSI_SK_MEDIUM_ERROR,
				SCSI_ASC_UNRECOVERED_READ_ERROR, 0);
		} else if (error & ATA_ERR_MC) {
			scsi_ata_check_condition(req,
				SCSI_SK_UNIT_ATTENTION,
				SCSI_ASC_MEDIA_CHANGED, 0);
		} else if (error & ATA_ERR_IDNF) {
			scsi_ata_check_condition(req,
				SCSI_SK_ILLEGAL_REQUEST,
				SCSI_ASC_LBA_OUT_OF_RANGE, 0);
		} else if (error & ATA_ERR_MCR) {
			scsi_ata_check_condition(req,
				SCSI_SK_UNIT_ATTENTION,
				SCSI_ASC_OPERATOR_MEDIUM_REMOVAL_REQUEST, 0x01);
		} else if (error & ATA_ERR_NM) {
			scsi_ata_check_condition(req,
				SCSI_SK_NOT_READY,
				SCSI_ASC_MEDIUM_NOT_PRESENT, 0);
		} else if (error & ATA_ERR_ICRC) {
			scsi_ata_check_condition(req,
				SCSI_SK_ABORTED_COMMAND,
				SCSI_ASC_SCSI_PARITY_ERROR, 0x3);
		} else if (error & ATA_ERR_ABRT) {
			scsi_ata_check_condition(req,
				SCSI_SK_ABORTED_COMMAND,
				SCSI_ASC_NO_ASC, 0);
		} else {
			return;
		}
	} else {
		return;
	}

	req->Scsi_Status = REQ_STATUS_HAS_SENSE;
}

MV_BOOLEAN atapi_categorize_cdb(MV_Request *req)
{
	req->Cmd_Flag |= CMD_FLAG_PACKET;

	/* for atapi, we supports all commands, just pass to device */
	if (req->Cdb[0] != SCSI_CMD_MARVELL_SPECIFIC) {
		if((req->Req_Type == REQ_TYPE_CORE)&& (req->Cdb[0]== SCSI_CMD_INQUIRY))
			req->Cmd_Flag &= ~CMD_FLAG_PACKET;
		return MV_TRUE;
	} else {
		MV_DASSERT(req->Cdb[1] == CDB_CORE_MODULE);

		switch (req->Cdb[2]) {
		case CDB_CORE_IDENTIFY:
		case CDB_CORE_SET_UDMA_MODE:
		case CDB_CORE_SET_PIO_MODE:
		case CDB_CORE_SOFT_RESET_1:
		case CDB_CORE_SOFT_RESET_0:
			if ((req->Cdb[2]==CDB_CORE_SOFT_RESET_1)
				|| (req->Cdb[2]==CDB_CORE_SOFT_RESET_0))
				req->Cmd_Flag |= CMD_FLAG_SOFT_RESET;
			return MV_TRUE;

		default:
			return MV_FALSE;

		}
	}
}

MV_BOOLEAN sat_categorize_cdb(pl_root *root, MV_Request *req)
{
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *device = (domain_device *)get_device_by_id(root->lib_dev,
		req->Device_Id);

	if (ctx->type == CORE_CONTEXT_TYPE_ORG_REQ)
		return MV_TRUE;

	/* Clear these flags first. */
	req->Cmd_Flag &= ~CMD_FLAG_PACKET;
	req->Cmd_Flag &= ~CMD_FLAG_NCQ;
	req->Cmd_Flag &= ~CMD_FLAG_48BIT;
	req->Cmd_Flag &= ~CMD_FLAG_SOFT_RESET;

	if (IS_ATAPI(device)) {
		return atapi_categorize_cdb(req);
	}

	if (device->capability & DEVICE_CAPABILITY_48BIT_SUPPORTED)
		req->Cmd_Flag |= CMD_FLAG_48BIT;

	switch (req->Cdb[0]) {
	case SCSI_CMD_READ_6:
	case SCSI_CMD_READ_10:
	case SCSI_CMD_READ_16:
	case SCSI_CMD_WRITE_6:
	case SCSI_CMD_WRITE_10:
	case SCSI_CMD_VERIFY_10:
	case SCSI_CMD_VERIFY_16:
	case SCSI_CMD_WRITE_16:
		/*
		 * CMD_FLAG_DATA_IN
		 * CMD_FLAG_DMA
		 */
		if (SCSI_IS_READ(req->Cdb[0]))
			req->Cmd_Flag |= CMD_FLAG_DATA_IN;

		if (SCSI_IS_READ(req->Cdb[0]) || SCSI_IS_WRITE(req->Cdb[0]))
			req->Cmd_Flag |= CMD_FLAG_DMA;

		if (device->capability & DEVICE_CAPABILITY_NCQ_SUPPORTED) {
			if (!CORE_IS_EH_REQ(ctx) &&
				(device->base.queue_depth > 1) &&
				(SCSI_IS_READ(req->Cdb[0]) ||
				SCSI_IS_WRITE(req->Cdb[0]))) {
					req->Cmd_Flag |= CMD_FLAG_NCQ;
			}
		}

		return MV_TRUE;

	case SCSI_CMD_INQUIRY:
	case SCSI_CMD_READ_CAPACITY_10:
	case SCSI_CMD_READ_CAPACITY_16:
	case SCSI_CMD_START_STOP_UNIT:
	case SCSI_CMD_FORMAT_UNIT:
	case SCSI_CMD_LOG_SENSE:
	case SCSI_CMD_REQUEST_SENSE:
	case SCSI_CMD_SND_DIAG:
	case SCSI_CMD_TEST_UNIT_READY:
	case SCSI_CMD_MODE_SELECT_6:
	case SCSI_CMD_MODE_SELECT_10:
	case SCSI_CMD_SYNCHRONIZE_CACHE_10:
	case SCSI_CMD_REASSIGN_BLOCKS:
	case SCSI_CMD_ATA_PASSTHRU_12:
	case SCSI_CMD_ATA_PASSTHRU_16:
		return MV_TRUE;		

	case SCSI_CMD_MARVELL_SPECIFIC:
	{
		MV_ASSERT(req->Cdb[1] == CDB_CORE_MODULE);

		switch (req->Cdb[2]) {
		case CDB_CORE_IDENTIFY:
		case CDB_CORE_READ_LOG_EXT:
		case CDB_CORE_PM_READ_REG:
		case CDB_CORE_PM_WRITE_REG:
			req->Cmd_Flag |= CMD_FLAG_DATA_IN;
			return MV_TRUE;

		case CDB_CORE_SOFT_RESET_1:
		case CDB_CORE_SOFT_RESET_0:
			req->Cmd_Flag |= CMD_FLAG_SOFT_RESET;
			return MV_TRUE;

		case CDB_CORE_SET_UDMA_MODE:
		case CDB_CORE_SET_PIO_MODE:
		case CDB_CORE_ENABLE_WRITE_CACHE:
		case CDB_CORE_DISABLE_WRITE_CACHE:
		case CDB_CORE_SET_FEATURE_SPINUP:
		case CDB_CORE_ENABLE_SMART:
		case CDB_CORE_DISABLE_SMART:
		case CDB_CORE_SMART_RETURN_STATUS:
		case CDB_CORE_ENABLE_READ_AHEAD:
		case CDB_CORE_DISABLE_READ_AHEAD:
		case CDB_CORE_SHUTDOWN:
		case CDB_CORE_OS_SMART_CMD:
		case CDB_CORE_ATA_SLEEP:
		case CDB_CORE_ATA_IDLE:
		case CDB_CORE_ATA_STANDBY:
		case CDB_CORE_ATA_IDLE_IMMEDIATE:
		case CDB_CORE_ATA_STANDBY_IMMEDIATE:
			return MV_TRUE;

		default:
			return MV_FALSE;
		}
	}
	case APICDB0_PD:
		switch (req->Cdb[1]) {
		case APICDB1_PD_SETSETTING:
			switch (req->Cdb[4]) {
				case APICDB4_PD_SET_WRITE_CACHE_OFF:
				case APICDB4_PD_SET_WRITE_CACHE_ON:
				case APICDB4_PD_SET_SMART_OFF:
				case APICDB4_PD_SET_SMART_ON:
					return MV_TRUE;
				default:
					return MV_FALSE;
			}
			break;
		case APICDB1_PD_GETSTATUS:
			switch (req->Cdb[4]) {
			case APICDB4_PD_SMART_RETURN_STATUS:
				return MV_TRUE;
			default:
				return MV_FALSE;
			}
			break;
		default:
			return MV_FALSE;
		}
		break;
	default:
		CORE_DPRINT(("unknown request: 0x%x.\n", req->Cdb[0]));
		return MV_FALSE;
	}
}

/*************************************************************************
***                SAT - SCSI to ATA Translation Functions             ***
***                                                                    ***
**************************************************************************/

/***************************************************************************
* scsi_ata_check_condition
* Purpose: Send Check Condition when request is illegal
*
***************************************************************************/
void scsi_ata_check_condition(MV_Request *req, MV_U8 sense_key,
	MV_U8 sense_code, MV_U8 sense_qualifier)
{
	 req->Scsi_Status = REQ_STATUS_HAS_SENSE;
	 if (req->Sense_Info_Buffer) {
		 ((MV_PU8)req->Sense_Info_Buffer)[0] = 0x70; /* As SPC-4, set Response Code to 70h, or SCSI layer didn't know to set down error disk */
		((MV_PU8)req->Sense_Info_Buffer)[2] = sense_key;
		/* additional sense length */
		((MV_PU8)req->Sense_Info_Buffer)[7] = 0x0a;
		/* additional sense code */
		((MV_PU8)req->Sense_Info_Buffer)[12] = sense_code;
                /* additional sense code qualifier*/
                ((MV_PU8)req->Sense_Info_Buffer)[13] = sense_qualifier;
	 }
}

/***************************************************************************
* scsi_ata_fill_lba_cdb6
* Purpose: Fill LBA Cdb (6) fields with the proper req->Cdb bytes
*
***************************************************************************/
void scsi_ata_fill_lba_cdb6(MV_Request *req, ata_taskfile *taskfile)
{
	taskfile->lba_low = req->Cdb[3];
	taskfile->lba_mid = req->Cdb[2];
	taskfile->lba_high = req->Cdb[1] & 0x1F;
	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
}

/***************************************************************************
* scsi_ata_fill_lba_cdb10
* Purpose: Fill LBA Cdb (10) fields with the proper req->Cdb bytes
*
***************************************************************************/
void scsi_ata_fill_lba_cdb10(MV_Request *req, ata_taskfile *taskfile)
{
	taskfile->lba_low = req->Cdb[5];
	taskfile->lba_mid = req->Cdb[4];
	taskfile->lba_high = req->Cdb[3];
	taskfile->lba_low_exp = req->Cdb[2];
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
}

/***************************************************************************
* scsi_ata_fill_lba_cdb16
* Purpose: Fill LBA Cdb (16) fields with the proper req->Cdb bytes
*
***************************************************************************/
void scsi_ata_fill_lba_cdb16(MV_Request *req, ata_taskfile *taskfile)
{
	taskfile->lba_low = req->Cdb[9];
	taskfile->lba_mid = req->Cdb[8];
	taskfile->lba_high = req->Cdb[7];
	taskfile->lba_low_exp = req->Cdb[6];
	taskfile->lba_mid_exp = req->Cdb[5];
	taskfile->lba_high_exp = req->Cdb[4];
}

/***************************************************************************
* scsi_ata_fill_data_field
* Purpose: Fill data field with bytes that are swaped within a word
*
***************************************************************************/
MV_VOID scsi_ata_fill_data_field(MV_U8 *des, MV_U8 *src, MV_U32 len)
{
	MV_U32 i;
	MV_U32 max_len = len >> 1;

	for (i = 0; i < max_len; i++) {
		((MV_U16 *)des)[i] = MV_SWAP_16(((MV_U16 *)src)[i]);
		((MV_U16 *)des)[i] = MV_LE16_TO_CPU(((MV_U16 *)des)[i]);
	}

	if (len & 1) {
		/* ATA data is WORD accessible */
		/* Length should never be odd  */
		MV_ASSERT(MV_FALSE);
		des[(i << 1)+ 1] = 0;
	}
}

/***************************************************************************
* scsi_ata_fill_format_taskfile
* Purpose: Format Unit cmd translation
*
***************************************************************************/
void scsi_ata_fill_format_taskfile(domain_device *device, MV_U8 ata_cmd,
	MV_U32 lba_count, MV_U32 lba_low, MV_U32 lba_high,
        ata_taskfile *taskfile)
{
	taskfile->command = ata_cmd;
	taskfile->features = 0;
	taskfile->feature_exp = 0;

	if(ata_cmd == ATA_CMD_WRITE_DMA || ata_cmd == ATA_CMD_VERIFY) {
		taskfile->sector_count = (MV_U8)(lba_count & 0xFF);
		taskfile->lba_low = (MV_U8)(lba_low&0xFF);
		taskfile->lba_mid = (MV_U8)(lba_low>>8) & 0xFF;
		taskfile->lba_high = (MV_U8)(lba_low>>16) & 0xFF;
		taskfile->device = ((MV_U8)(lba_low>>24) & 0xF ) | 0x40;

                taskfile->sector_count_exp = 0;
		taskfile->lba_low_exp = 0;
		taskfile->lba_mid_exp = 0;
		taskfile->lba_high_exp = 0;

	} else if (ata_cmd == ATA_CMD_WRITE_DMA_EXT ||
		ata_cmd == ATA_CMD_VERIFY_EXT) {
		taskfile->sector_count = (MV_U8)(lba_count & 0xFF);
		taskfile->sector_count_exp = (MV_U8)((lba_count >> 8) & 0xFF);
		taskfile->lba_low = (MV_U8)(lba_low&0xFF);
		taskfile->lba_mid = (MV_U8)(lba_low>>8) & 0xFF;
		taskfile->lba_high = (MV_U8)(lba_low>>16) & 0xFF;
		taskfile->lba_low_exp = (MV_U8)(lba_low>>24) & 0xFF;
		taskfile->lba_mid_exp = (MV_U8)(lba_high&0xFF);
		taskfile->lba_high_exp = (MV_U8)((lba_high>>8)&0xFF);
		taskfile->device = 0x40;
	}
	device->total_formatted_lba_count += lba_count;
}

MV_Request *sat_get_org_req(MV_Request *req)
{
        core_context *ctx = (core_context *)req->Context[MODULE_CORE];

        if (ctx->type != CORE_CONTEXT_TYPE_ORG_REQ)
                return NULL;

        return ctx->u.org.org_req;
}

MV_Request *sat_clear_org_req(MV_Request *req)
{
        core_context *ctx = (core_context *)req->Context[MODULE_CORE];
        MV_Request *org_req = ctx->u.org.org_req;

        MV_ASSERT(ctx->type == CORE_CONTEXT_TYPE_ORG_REQ);

        ctx->type = CORE_CONTEXT_TYPE_NONE;
        ctx->u.org.org_req = NULL;

        return org_req;
}

void sat_copy_request(MV_Request *req1, MV_Request *req2)
{
	core_context    *ctx1;
        core_context    *ctx2;

        if (req1 == NULL || req2 == NULL) {
                MV_ASSERT(MV_FALSE);
                return;
        }

	ctx1 = (core_context *)req1->Context[MODULE_CORE];
        ctx2 = (core_context *)req2->Context[MODULE_CORE];

        if (ctx1 == NULL || ctx2 == NULL) {
                MV_ASSERT(MV_FALSE);
                return;
        }

        ctx1->error_info        = ctx2->error_info;
        ctx1->handler           = ctx2->handler;
        ctx1->req_type          = ctx2->req_type;
		ctx1->req_flag			= ctx2->req_flag;

        MV_CopyMemory(req1->Cdb, req2->Cdb, sizeof(req1->Cdb));

        req1->Cmd_Flag          = req2->Cmd_Flag;
        req1->Device_Id         = req2->Device_Id;
        req1->LBA               = req2->LBA;
        req1->Org_Req           = req2->Org_Req;
        req1->Req_Flag          = req2->Req_Flag;
        req1->Sector_Count      = req2->Sector_Count;
        req1->Tag               = req2->Tag;
        req1->Time_Out          = req2->Time_Out;

        req1->pRaid_Request     = NULL;
        req1->Scratch_Buffer    = NULL;
}

/*
   sat_replicate_req

   Creates a new copy of the request for use in translation. Original request
   is perserved through the orq_req pointer in the context.
*/
MV_Request *sat_replicate_req(pl_root *root,
			MV_Request *req,
			MV_U32 new_buff_size,
			MV_ReqCompletion completion)
{
	domain_device   *device;
	core_context    *curr_ctx;
	core_context    *new_ctx;
	MV_Request      *new_req;
	MV_U32          buff_size;
	MV_PVOID        org_buf_ptr, new_buf_ptr;

	curr_ctx = (core_context *)req->Context[MODULE_CORE];
	device = (domain_device *)get_device_by_id(root->lib_dev,
	req->Device_Id);

	/* Previously allocated already */
	if (curr_ctx->type == CORE_CONTEXT_TYPE_ORG_REQ) {
		MV_ASSERT(MV_FALSE);
		return req;
	}

	org_buf_ptr = core_map_data_buffer(req);
	if (org_buf_ptr == NULL && new_buff_size != 0) {
		CORE_DPRINT(("Request wanted data buffer size of %d "\
		"but has NULL data buffer\n", new_buff_size));
		CORE_DPRINT(("CDB %02x %02x %02x %02x %02x %02x\n", req->Cdb[0],\
		req->Cdb[1], req->Cdb[2], req->Cdb[3], req->Cdb[4],\
		req->Cdb[5]));
		new_buff_size = 0;
	}

	MV_DASSERT(new_buff_size <= SCRATCH_BUFFER_SIZE);

	new_req = get_intl_req_resource(root, new_buff_size);

	if (new_req == NULL) {
		core_unmap_data_buffer(req);
		return NULL;
	}

	sat_copy_request(new_req, req);

	MV_LIST_HEAD_INIT(&new_req->Queue_Pointer);

	if (new_buff_size != 0) {
		new_buf_ptr = core_map_data_buffer(new_req);
		buff_size = MV_MIN(new_req->Data_Transfer_Length,
		req->Data_Transfer_Length);
		MV_CopyMemory(new_buf_ptr, org_buf_ptr, buff_size);
		core_unmap_data_buffer(new_req);
	}

	new_req->Completion = completion;
	new_ctx = (core_context *)new_req->Context[MODULE_CORE];
	new_ctx->type = CORE_CONTEXT_TYPE_ORG_REQ;
	new_ctx->u.org.org_req = req;

	core_unmap_data_buffer(req);
	return new_req;
}

MV_U8 scsi_ata_format_unit_translation(domain_device *device, MV_Request *req);
/***************************************************************************
* scsi_ata_format_unit_callback
* Purpose: Format Unit cmd translation
*
***************************************************************************/
MV_U8 scsi_ata_format_unit_callback(MV_PVOID root_p,
        MV_Request *org_req, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *device = (domain_device *)get_device_by_id(
	root->lib_dev, req->Device_Id);
	MV_U32 total_lba_count = device->max_lba.parts.low + 1;

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		org_req->Scsi_Status = req->Scsi_Status;
		return MV_TRUE;
	}

	switch (device->state) {
	case DEVICE_STATE_FORMAT_WRITE:
		if (total_lba_count ==
			device->total_formatted_lba_count) {

			device->state = DEVICE_STATE_FORMAT_VERIFY;
			device->total_formatted_lba_count = 0;
		}
		break;
	case DEVICE_STATE_FORMAT_VERIFY:
		if (total_lba_count == device->total_formatted_lba_count) {
			device->state = DEVICE_STATE_FORMAT_DONE;
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			return MV_TRUE;
		}
		break;
	case DEVICE_STATE_FORMAT_DONE:
	default:
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		return MV_TRUE;
	}

	return MV_FALSE;
}

void scsi_ata_format_unit_fill_taskfile(domain_device *device, MV_Request *req,
	ata_taskfile *taskfile)
{
	MV_U8 ata_cmd = 0;
	MV_U32 lba_count = 0;
	MV_U32 total_lba_count;
	MV_Request *org_req = sat_get_org_req(req);
	core_context *org_ctx = (core_context *)org_req->Context[MODULE_CORE];

	if (org_ctx->req_state != FORMAT_UNIT_STARTED) {
		device->state = DEVICE_STATE_FORMAT_WRITE;
		org_ctx->req_state = FORMAT_UNIT_STARTED;
	}

	if (device->capability & DEVICE_CAPABILITY_48BIT_SUPPORTED)
		ata_cmd = (device->state == DEVICE_STATE_FORMAT_WRITE) ?
		ATA_CMD_WRITE_DMA_EXT : ATA_CMD_VERIFY_EXT;
	else
		ata_cmd = (device->state == DEVICE_STATE_FORMAT_WRITE) ?
		ATA_CMD_WRITE_DMA : ATA_CMD_VERIFY;

	total_lba_count = device->max_lba.parts.low + 1;

	if (LBA_BLOCK_COUNT < (total_lba_count - device->total_formatted_lba_count))
		lba_count = LBA_BLOCK_COUNT;
	else
		lba_count = total_lba_count - device->total_formatted_lba_count;

	scsi_ata_fill_format_taskfile(device, ata_cmd, 0, 0, lba_count, taskfile);
	device->total_formatted_lba_count += lba_count;
}

/***************************************************************************
* scsi_ata_format_unit_translation
* Purpose: Format Unit cmd translation
*
***************************************************************************/
MV_U8 scsi_ata_format_unit_translation(domain_device *device, MV_Request *req)
{
	pl_root *root = device->base.root;
	MV_Request *new_req;

	MV_U8 defect_list = 0;
	MV_U8 cmp_list = 0;
	MV_U8 fmt_data = 0;

	defect_list = req->Cdb[1] & 0x7;
	cmp_list = req->Cdb[1] & 0x8;

	if (defect_list != 0 || defect_list != 0x6 ||
	(cmp_list != 0 && fmt_data != 0) || req->Cdb[2] != 0 ||
	req->Cdb[3] != 0 || req->Cdb[4] != 0 ) {
		scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
				SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	new_req = sat_replicate_req(root, req, 0,
		scsi_ata_translation_callback);

	if (new_req == NULL)
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

	core_append_request(root, new_req);
	return MV_QUEUE_COMMAND_RESULT_REPLACED;

}

/***************************************************************************
* scsi_ata_inquiry_callback
* Purpose: Handles the Inquiry data after ATA_CMD_IDENTIFY is sent
*
***************************************************************************/
MV_VOID sata_parse_identify_data(domain_device *device,
                                 ata_identify_data *identify_data);
MV_U64 sata_create_wwn(ata_identify_data *identify_data);

MV_VOID scsi_ata_inquiry_callback(MV_PVOID root_p,
        MV_Request *org_req, MV_Request *req)
{
	MV_U8 MV_INQUIRY_VPD_PAGE0_DATA[7] =
		{0x00, 0x00, 0x00, 0x03, 0x00, 0x80, 0x83};
	MV_U8 MV_INQUIRY_VPD_PAGE83_DATA[16] =
		{0x00, 0x83, 0x00, 0x0C, 0x01, 0x02, 0x00, 0x08,
		0x00, 0x50, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00};

	pl_root *root = (pl_root *)root_p;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *device = (domain_device *)get_device_by_id(root->lib_dev,
		req->Device_Id);

	ata_identify_data *id_data;
	MV_U32 i;
	MV_U32 tmp_len=0;
	MV_PU8 org_buf_ptr;

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		org_req->Scsi_Status = req->Scsi_Status;
		return;
	}

	org_buf_ptr = (MV_PU8)core_map_data_buffer(org_req);
	id_data = (ata_identify_data *)core_map_data_buffer(req);

	if (org_req->Cdb[1] & CDB_INQUIRY_EVPD) {
		switch(org_req->Cdb[2]) {
		case 0x00:
			if (req->Device_Id == VIRTUAL_DEVICE_ID) {
			tmp_len = MV_MIN(org_req->Data_Transfer_Length,
				VPD_PAGE0_VIRTUALD_SIZE);
			MV_CopyMemory(org_buf_ptr,
				MV_INQUIRY_VPD_PAGE0_VIRTUALD_DATA,
				tmp_len);
			} else {
				tmp_len = MV_MIN(org_req->Data_Transfer_Length,
					sizeof(MV_INQUIRY_VPD_PAGE0_DATA));
				MV_CopyMemory(org_buf_ptr,
					MV_INQUIRY_VPD_PAGE0_DATA,
					tmp_len);
			}
			break;
		case 0x80:
			/* 4-23 bytes hold the serial number of the device.*/
			if (req->Device_Id == VIRTUAL_DEVICE_ID) {
				tmp_len = MV_MIN(org_req->Data_Transfer_Length,
					VPD_PAGE80_VIRTUALD_SIZE);
				MV_CopyMemory(org_buf_ptr,
					MV_INQUIRY_VPD_PAGE80_VIRTUALD_DATA,
					tmp_len);
			} else {
				scsi_ata_fill_data_field(
				&device->serial_number[0],
				&id_data->serial_number[0],
				sizeof(device->serial_number));

				if (org_req->Data_Transfer_Length >= 4) {
				tmp_len = MV_MIN(
					(org_req->Data_Transfer_Length
					- 4),
					20);
				org_buf_ptr[1] = 0x80;
				org_buf_ptr[3] = (MV_U8)tmp_len;
				scsi_ata_fill_data_field(
					&org_buf_ptr[4],
					&id_data->serial_number[0],
					tmp_len);
				tmp_len +=4;
				}
			}
			break;
		case 0x83:
			if (req->Device_Id == VIRTUAL_DEVICE_ID) {
				tmp_len = MV_MIN(org_req->Data_Transfer_Length,
					VPD_PAGE83_VIRTUALD_SIZE);
				/*Logical Unit should have unique name*/
				MV_INQUIRY_VPD_PAGE83_VIRTUALD_DATA[15] = (MV_U8)(req->Device_Id >> 8);
				MV_INQUIRY_VPD_PAGE83_VIRTUALD_DATA[16] = (MV_U8)req->Device_Id;
				MV_CopyMemory(org_buf_ptr,
				MV_INQUIRY_VPD_PAGE83_VIRTUALD_DATA,
				tmp_len);
			} else {
				tmp_len = MV_MIN(org_req->Data_Transfer_Length,
					sizeof(MV_INQUIRY_VPD_PAGE83_DATA));
				MV_INQUIRY_VPD_PAGE83_DATA[14] = (MV_U8)((req->Device_Id >> 8) & 0xFF);
				MV_INQUIRY_VPD_PAGE83_DATA[15] = (MV_U8)(req->Device_Id & 0xFF);
				MV_CopyMemory(org_buf_ptr,
					MV_INQUIRY_VPD_PAGE83_DATA,
					tmp_len);
			}
			break;
		default:
			scsi_ata_check_condition(org_req,
			SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ASC_INVALID_FEILD_IN_CDB, 0);

			core_unmap_data_buffer(org_req);
			core_unmap_data_buffer(req);
			return;
		}

		req->Scsi_Status = REQ_STATUS_SUCCESS;
		org_req->Data_Transfer_Length = tmp_len;

	} else {
		/* Standard inquiry */
		if (org_req->Cdb[2] != 0) {
			/*
			 * PAGE CODE field must be zero when EVPD is zero for a
			 * valid request. sense key as ILLEGAL REQUEST and
			 * additional sense code as INVALID FIELD IN CDB
			 */
			scsi_ata_check_condition(org_req, SCSI_SK_ILLEGAL_REQUEST,
				SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
			core_unmap_data_buffer(org_req);
			core_unmap_data_buffer(req);
			return;
		}

		if (req->Device_Id == VIRTUAL_DEVICE_ID) {
			tmp_len =  MV_MIN(org_req->Data_Transfer_Length,
				VIRTUALD_INQUIRY_DATA_SIZE);
			MV_CopyMemory(org_buf_ptr,
				MV_INQUIRY_VIRTUALD_DATA,
				tmp_len);
			org_req->Data_Transfer_Length = tmp_len;
			core_unmap_data_buffer(org_req);
			core_unmap_data_buffer(req);
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			return;
		}

		sata_parse_identify_data(device, id_data);
		tmp_len =  MV_MIN(org_req->Data_Transfer_Length,
			STANDARD_INQUIRY_LENGTH);
		org_req->Data_Transfer_Length = tmp_len;

		if (tmp_len >= 8) {
			org_buf_ptr[0] = (IS_ATAPI(device)) ? 0x5 : 0;
			org_buf_ptr[1] = (IS_ATAPI(device)) ? MV_BIT(7) : 0;
			org_buf_ptr[2] = 0x05;      /* claim conformance to SPC-3 */
			org_buf_ptr[3] = 0x02;      /* set RESPONSE DATA FORMAT to 2 */
			org_buf_ptr[4] = (MV_U8)tmp_len - 5;
			org_buf_ptr[6] = 0x0;
			org_buf_ptr[7] = 0x13;      /* Tagged Queuing */
		}

		if (tmp_len >= 16)
			MV_CopyMemory(&org_buf_ptr[8], "ATA     ", 8);
		if (tmp_len >= 32)
			scsi_ata_fill_data_field(&org_buf_ptr[16],
				&id_data->model_number[0], 16);

		if (tmp_len >= 36)
			scsi_ata_fill_data_field(&org_buf_ptr[32],
				&id_data->firmware_revision[0], 4);

		if (tmp_len >= 42)
			MV_CopyMemory(&org_buf_ptr[36], "MVSATA", 6);

		/*
		* 0x00A0 SAM 5
		* 0x0460 SPC 4
		* 0x04C0 SBC 3
		* 0x1EC0 SAT 2
		*/
		if (tmp_len >= 66) {
			org_buf_ptr[58] = 0x00;
			org_buf_ptr[59] = 0xA0;
			org_buf_ptr[60] = 0x04;
			org_buf_ptr[61] = 0x60;
			org_buf_ptr[62] = 0x04;
			org_buf_ptr[63] = 0xC0;
			org_buf_ptr[64] = 0x1E;
			org_buf_ptr[65] = 0xC0;
		}

		req->Scsi_Status = REQ_STATUS_SUCCESS;
	}

	core_unmap_data_buffer(req);
	core_unmap_data_buffer(org_req);
}

/***************************************************************************
* scsi_ata_inquiry_translation
* Purpose: Translate Inquiry SCSI command and send ATA_CMD_IDENTIFY
*
***************************************************************************/
void scsi_ata_inquiry_fill_taskfile(MV_Request *req, ata_taskfile *taskfile)
{
	taskfile->command = ATA_CMD_IDENTIFY_ATA;
	taskfile->features = 0;
	taskfile->sector_count = 0;
	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;
	taskfile->device = 0;
	taskfile->control = 0;
	taskfile->feature_exp = 0;
	taskfile->sector_count_exp = 0;
	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
}

/***************************************************************************
* scsi_ata_inquiry_translation
* Purpose: Translate Inquiry SCSI command and send ATA_CMD_IDENTIFY
*
***************************************************************************/
MV_U8 scsi_ata_inquiry_translation(domain_device *device, MV_Request *req)
{
	MV_Request *new_req;
	pl_root *root = device->base.root;

	if (((req->Cdb[1] & 0x1) == 0) && (req->Cdb[2] != 0)) {
		scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
		SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	new_req = sat_replicate_req(root, req, 0x200,
	scsi_ata_translation_callback);

	if (new_req == NULL)
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

	core_append_request(root, new_req);
	return MV_QUEUE_COMMAND_RESULT_REPLACED;
}

/***************************************************************************
* scsi_ata_read_write_translate_cdb
* Purpose: Translate read/write SCSI CDB
*
***************************************************************************/
void scsi_ata_read_write_fill_taskfile(MV_Request *req, MV_U8 tag,
	ata_taskfile * taskfile)
{
	MV_U8 temp_sc;
	MV_U8 temp_sc_exp;

	switch(req->Cdb[0]) {
	case SCSI_CMD_READ_6:
	case SCSI_CMD_WRITE_6:
		temp_sc = req->Cdb[4];
		temp_sc_exp = 0;
		scsi_ata_fill_lba_cdb6(req, taskfile);
		break;
	case SCSI_CMD_READ_10:
	case SCSI_CMD_WRITE_10:
		temp_sc = req->Cdb[8];
		temp_sc_exp = req->Cdb[7];
		scsi_ata_fill_lba_cdb10(req, taskfile);
		break;
	case SCSI_CMD_READ_16:
	case SCSI_CMD_WRITE_16:
		temp_sc = req->Cdb[13];
		temp_sc_exp = req->Cdb[12];
		scsi_ata_fill_lba_cdb16(req, taskfile);
		break;
	default:
		return;
	}

	taskfile->device = MV_BIT(6);
	taskfile->control = 0;

	if (req->Cmd_Flag & CMD_FLAG_NCQ) {
		taskfile->features = temp_sc;
		taskfile->feature_exp = temp_sc_exp;
		taskfile->sector_count = tag << 3;
		taskfile->sector_count_exp = 0;

		if (req->Cdb[0] == SCSI_CMD_READ_6 ||
			req->Cdb[0] == SCSI_CMD_READ_10 ||
			req->Cdb[0] == SCSI_CMD_READ_16)
			taskfile->command = ATA_CMD_READ_FPDMA_QUEUED;
		else
			taskfile->command = ATA_CMD_WRITE_FPDMA_QUEUED;
	} else if (req->Cmd_Flag & CMD_FLAG_48BIT) {
		taskfile->features = 0;
		taskfile->feature_exp = 0;
		taskfile->sector_count = temp_sc;
		taskfile->sector_count_exp = temp_sc_exp;
		if (req->Cdb[0] == SCSI_CMD_READ_6 ||
			req->Cdb[0] == SCSI_CMD_READ_10 ||
			req->Cdb[0] == SCSI_CMD_READ_16)
			taskfile->command = ATA_CMD_READ_DMA_EXT;
		else
			taskfile->command = ATA_CMD_WRITE_DMA_EXT;
	} else {
		taskfile->features = 0;
		taskfile->feature_exp = 0;
		taskfile->sector_count = temp_sc;
		taskfile->sector_count_exp = 0;
		/* ATA6: Device Lowbyte = LBA 27:24.*/
		if ( (req->Cdb[0] == SCSI_CMD_READ_16 || req->Cdb[0] == SCSI_CMD_WRITE_16) ) {
			taskfile->device |= (req->Cdb[6] & 0xF); 
			MV_DASSERT((req->Cdb[6]&0xF0) == 0);
		} else if ( (req->Cdb[0] == SCSI_CMD_READ_10 || req->Cdb[0] == SCSI_CMD_WRITE_10) ) {
			taskfile->device |= (req->Cdb[2] & 0xF);
			MV_DASSERT((req->Cdb[2]&0xF0) == 0);
		} else {
			/*READ_6, WRITE_6, LBA 27:24 = 0*/
		}

		if (req->Cdb[0] == SCSI_CMD_READ_6 ||
			req->Cdb[0] == SCSI_CMD_READ_10 ||
			req->Cdb[0] == SCSI_CMD_READ_16)
			taskfile->command = ATA_CMD_READ_DMA;
		else
			taskfile->command = ATA_CMD_WRITE_DMA;
	}
}

/***************************************************************************
* scsi_ata_read_capacity_callback
* Purpose: Handles read capacity SCSI command callback data
*
***************************************************************************/
void scsi_ata_read_capacity_callback(MV_PVOID root_p,
	MV_Request *org_req, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *device = (domain_device *)get_device_by_id(root->lib_dev,
							req->Device_Id);
	ata_identify_data *identify_data;
	MV_LBA  max_lba;
	MV_U32  block_length;
	MV_PU32  org_buf_ptr;

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		org_req->Scsi_Status = req->Scsi_Status;
		return;
	}

	identify_data = (ata_identify_data *)core_map_data_buffer(req);
	org_buf_ptr = core_map_data_buffer(org_req);

	/* Disk size */
	if (device->capability & DEVICE_CAPABILITY_48BIT_SUPPORTED) {
	device->max_lba.parts.low =
		MV_LE32_TO_CPU(*((MV_PU32)&identify_data->max_lba[0]));
		device->max_lba.parts.high =
		MV_LE32_TO_CPU(*((MV_PU32)&identify_data->max_lba[2]));
	} else {
		device->max_lba.parts.low =
		MV_LE32_TO_CPU(*((MV_PU32)&identify_data->user_addressable_sectors[0]));
		device->max_lba.parts.high = 0;
	}
	/*
	* The disk size as indicated by the ATA spec is the total addressable
	* sectors on the drive; while Max LBA is the LBA of the last logical
	* block on the drive
	*/
	device->max_lba = U64_SUBTRACT_U32(device->max_lba, 1);

	max_lba = device->max_lba;
	block_length = SECTOR_SIZE;

	if (org_req->Cdb[0] == SCSI_CMD_READ_CAPACITY_10 &&
			org_req->Data_Transfer_Length >= 8) {
		/* check if max_lba exceeds the maximum value for Read Capacity 10 */
		if (max_lba.parts.high != 0){
			org_buf_ptr[0] = MV_CPU_TO_BE32(0xFFFFFFFF);
		}else
			org_buf_ptr[0] = MV_CPU_TO_BE32(max_lba.parts.low);
		org_buf_ptr[1] = MV_CPU_TO_BE32(block_length);
	} else if (org_req->Cdb[0] == SCSI_CMD_READ_CAPACITY_16 &&
		org_req->Data_Transfer_Length >= 12) {
		org_buf_ptr[0] = MV_CPU_TO_BE32(max_lba.parts.high);
		org_buf_ptr[1] = MV_CPU_TO_BE32(max_lba.parts.low);
		org_buf_ptr[2] = MV_CPU_TO_BE32(block_length);
	}

	core_unmap_data_buffer(req);
	core_unmap_data_buffer(org_req);
}

void scsi_ata_read_capacity_fill_taskfile(MV_Request *req,
	ata_taskfile *taskfile)
{
	taskfile->command = ATA_CMD_IDENTIFY_ATA;
	taskfile->features = 0;
	taskfile->sector_count = 0;
	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;
	taskfile->device = 0;
	taskfile->control = 0;
	taskfile->feature_exp = 0;
	taskfile->sector_count_exp = 0;
	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
}

/***************************************************************************
* scsi_ata_read_capacity_translation
* Purpose: Handles read capacity SCSI commands
*
***************************************************************************/
MV_U8 scsi_ata_read_capacity_translation(domain_device *device, MV_Request *req)
{
	MV_Request *new_req;
	pl_root *root = device->base.root;

	if ((req->Cdb[2] | req->Cdb[3] | req->Cdb[4] | req->Cdb[5]) != 0) {
		scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	new_req = sat_replicate_req(root, req, 0x200,
		scsi_ata_translation_callback);

	if (new_req == NULL)
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

	core_append_request(root, new_req);
	return MV_QUEUE_COMMAND_RESULT_REPLACED;
}

/***************************************************************************
* scsi_ata_verify_translation
* Purpose: Handles verify SCSI commands
*
***************************************************************************/
void scsi_ata_verify_fill_taskfile(MV_Request *req, ata_taskfile *taskfile)
{
	MV_U8 tempSC, tempSC_exp;

	switch(req->Cdb[0]) {
	case SCSI_CMD_VERIFY_10:
		tempSC = req->Cdb[8];
		tempSC_exp = req->Cdb[7];
		scsi_ata_fill_lba_cdb10(req, taskfile);
		break;
	case SCSI_CMD_VERIFY_16:
		tempSC = req->Cdb[13];
		tempSC_exp = req->Cdb[12];
		scsi_ata_fill_lba_cdb16(req, taskfile);
		break;
	default:
		return;
	}

	taskfile->device = MV_BIT(6);

	if (req->Cmd_Flag & CMD_FLAG_48BIT) {
		taskfile->sector_count = tempSC;
		taskfile->sector_count_exp = tempSC_exp;
		taskfile->command = ATA_CMD_VERIFY_EXT;
	} else {
		taskfile->sector_count = tempSC;
                taskfile->sector_count_exp = 0;
		/* ATA6: Device Lowbyte = LBA 27:24.*/
		if (req->Cdb[0] == SCSI_CMD_VERIFY_16) {
			taskfile->device |= (req->Cdb[6] & 0xF);
			MV_DASSERT((req->Cdb[6]&0xF0) == 0);
		}
		else {
			taskfile->device |= (req->Cdb[2] & 0xF) ;
			MV_DASSERT((req->Cdb[2]&0xF0) == 0);
		}
		taskfile->command = ATA_CMD_VERIFY;
	}
	taskfile->features = 0;
	taskfile->control = 0;
	taskfile->feature_exp = 0;
}


void scsi_ata_sync_cache_fill_taskfile(domain_device *device, MV_Request *req,
	ata_taskfile *taskfile)
{
	taskfile->device = MV_BIT(6);
	switch (req->Cdb[0]){
	case SCSI_CMD_SYNCHRONIZE_CACHE_10:
		if (device->capability & DEVICE_CAPABILITY_48BIT_SUPPORTED)
			taskfile->command = ATA_CMD_FLUSH_EXT;
		else
			taskfile->command = ATA_CMD_FLUSH;
		break;
	case SCSI_CMD_SYNCHRONIZE_CACHE_16:
		taskfile->command = ATA_CMD_FLUSH_EXT;
		break;
	default:
		scsi_ata_check_condition( req, SCSI_SK_ILLEGAL_REQUEST,
				SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
		return;
	}
	taskfile->features = 0;
	taskfile->sector_count = 0;
	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;
	taskfile->device = 0;
	taskfile->control = 0;
	taskfile->feature_exp = 0;
	taskfile->sector_count_exp = 0;
	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
}

/***************************************************************************
* scsi_ata_sync_cache_translation
* Purpose: Syncronize cache 10/16
*
***************************************************************************/
MV_U8 scsi_ata_sync_cache_translation(domain_device *device, MV_Request *req)
{
	MV_Request *new_req;
	pl_root *root = device->base.root;

	/* for SSD without support write_cache */
	if (!(device->capability & DEVICE_CAPABILITY_WRITECACHE_SUPPORTED)) {
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	if (device->capability & DEVICE_CAPABILITY_48BIT_SUPPORTED)
		req->Cmd_Flag |= CMD_FLAG_48BIT;

	if ((req->Cdb[1] & 0x2) !=  0) req->Scsi_Status = REQ_STATUS_SUCCESS;

	new_req = sat_replicate_req(root, req, 0,
		scsi_ata_translation_callback);

	if (new_req == NULL)
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

	core_append_request(root, new_req);
	return MV_QUEUE_COMMAND_RESULT_REPLACED;
}

/***************************************************************************
* scsi_ata_send_diag_callback
* Purpose: handles callback
*
***************************************************************************/
MV_U8 scsi_ata_send_diag_callback(MV_PVOID root_p,
	MV_Request *org_req, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	core_context *org_ctx = (core_context *)org_req->Context[MODULE_CORE];
	domain_device *device = (domain_device *)get_device_by_id(
	root->lib_dev, req->Device_Id);

	if ((req->Scsi_Status != REQ_STATUS_SUCCESS) ||
		((req->Cdb[1] & 0x04) && (!((device->capability &
		DEVICE_CAPABILITY_SMART_SELF_TEST_SUPPORTED) &&
	(device->setting & DEVICE_SETTING_SMART_ENABLED))))) {
		switch(req->Cdb[1] & 0xE0) {
		case FOREGROUND_SHORT_SELF_TEST:
		case FOREGROUND_EXTENDED_SELF_TEST:
			scsi_ata_check_condition(org_req, SCSI_SK_HARDWARE_ERROR,
				        SCSI_ASC_LOGICAL_UNIT_FAILURE, 0x03);
			return MV_TRUE;
		default:
			org_req->Scsi_Status = req->Scsi_Status;
			return MV_TRUE;
		}
		return MV_TRUE;
	}

	switch (org_ctx->req_state) {
	case SEND_DIAGNOSTIC_SMART:
	case SEND_DIAGNOSTIC_VERIFY_MAX:
		req->Scsi_Status = REQ_STATUS_SUCCESS;
		return MV_TRUE;

	case SEND_DIAGNOSTIC_VERIFY_0:
	case SEND_DIAGNOSTIC_VERIFY_MID:
		break;
	default:
		MV_ASSERT(MV_FALSE);
		return MV_TRUE;
	}

	core_append_request(root, org_req);
	return MV_FALSE;
}

MV_U8 scsi_ata_send_diag_verify_fill_req(domain_device *device,
                                               MV_Request *req,
                                               MV_U64 lba)
{
	if (device->capability & DEVICE_CAPABILITY_48BIT_SUPPORTED) {
		req->Cdb[0] = SCSI_CMD_VERIFY_16;
		req->Cdb[2] = (MV_U8)(lba.parts.high >> 24) & 0xFF; /* LBA */
		req->Cdb[3] = (MV_U8)(lba.parts.high >> 16) & 0xFF;
		req->Cdb[4] = (MV_U8)(lba.parts.high >> 8) & 0xFF;
		req->Cdb[5] = (MV_U8)lba.parts.high & 0xFF;
		req->Cdb[6] = (MV_U8)(lba.parts.low >> 24) & 0xFF;
		req->Cdb[7] = (MV_U8)(lba.parts.low >> 16) & 0xFF;
		req->Cdb[8] = (MV_U8)(lba.parts.low >> 8) & 0xFF;
		req->Cdb[9] = (MV_U8)lba.parts.low & 0xFF;
		req->Cdb[10] = 0; /* Sector Count */
		req->Cdb[11] = 0;
		req->Cdb[12] = 0;
		req->Cdb[13] = 1;
	} else {
		req->Cdb[0] = SCSI_CMD_VERIFY_10;
		req->Cdb[2] = (MV_U8)(lba.parts.low >> 24) & 0xFF;
		req->Cdb[3] = (MV_U8)(lba.parts.low >> 16) & 0xFF;
		req->Cdb[4] = (MV_U8)(lba.parts.low >> 8) & 0xFF;
		req->Cdb[5] = (MV_U8)lba.parts.low & 0xFF;
		req->Cdb[7] = 0; /* Sector Count */
		req->Cdb[8] = 1;
	}

	return MV_TRUE;
}

void scsi_ata_send_diag_fill_taskfile(domain_device *device, MV_Request *req,
	ata_taskfile *taskfile)
{
	pl_root *root = device->base.root;

	if ((req->Cdb[1] & 0x04) && (device->capability &
		DEVICE_CAPABILITY_SMART_SELF_TEST_SUPPORTED) &&
		(device->setting & DEVICE_SETTING_SMART_ENABLED)) {
		taskfile->lba_low = 129;
	} else {
		switch ((req->Cdb[1] & 0xE0) >> 5) {
		case BACKGROUND_SHORT_SELF_TEST:
			taskfile->lba_low = 1;
			break;
		case BACKGROUND_EXTENDED_SELF_TEST:
			taskfile->lba_low = 2;
			break;
		case ABORT_BACKGROUND_SELF_TEST:
			taskfile->lba_low = 127;
			break;
		case FOREGROUND_SHORT_SELF_TEST:
			taskfile->lba_low = 129;
			break;
		case FOREGROUND_EXTENDED_SELF_TEST:
			taskfile->lba_low = 130;
			break;
		default:
			break;
		}
	}

	taskfile->device = MV_BIT(6);
	taskfile->lba_mid = 0x4F;
	taskfile->lba_high = 0xC2;
	taskfile->features = ATA_CMD_SMART_EXECUTE_OFFLINE;
	taskfile->command = ATA_CMD_SMART;

	taskfile->sector_count = 0;
	taskfile->control = 0;
	taskfile->feature_exp = 0;
	taskfile->sector_count_exp = 0;
	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
}

/***************************************************************************
* scsi_ata_send_diag_translation
* Purpose: Sends Diagnostic cmd
*
***************************************************************************/
MV_U8 scsi_ata_send_diag_translation(domain_device *device, MV_Request *req)
{
	MV_Request *new_req;
	pl_root *root = device->base.root;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_LBA lba;

	/* CDB[1] checks if DevOffL and UnitOffL bits are set
	  * CDB[3] & CDB[4] checks for parameter list length equal to 0.*/
	if (((req->Cdb[1] & 0x03) != 0) || (req->Cdb[3] != 0) || (req->Cdb[4] != 0)) {
		scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
				SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
	req->Time_Out = 60;
	switch (ctx->req_state) {
	case SEND_DIAGNOSTIC_START:
		if (req->Cdb[1] & 0x04) {
			if (!((device->capability &
				DEVICE_CAPABILITY_SMART_SELF_TEST_SUPPORTED) &&
				(device->setting & DEVICE_SETTING_SMART_ENABLED))) {

				/* Send 3 ATA_CMD_VERIFY commands and report
				   back the result:

				        1) Sector Count = 1, LBA = 0
				        2) Sector Count = 1, LBA = max_lba
				        3) Sector Count = 1, LBA = x,
				                {x e Z | 0 < x < max_lba}

				   If all 3 passes, then return success
				   If any 3 fails, then return logical unit
				   failure
				*/
				new_req = sat_replicate_req(root, req, 0,
				                scsi_ata_translation_callback);

				if (new_req == NULL)
					return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

				ctx->req_state = SEND_DIAGNOSTIC_VERIFY_0;
				lba.parts.low = 0;
				lba.parts.high = 0;
				scsi_ata_send_diag_verify_fill_req(device, req, lba);

				core_append_request(root, new_req);
				return MV_QUEUE_COMMAND_RESULT_REPLACED;
			}
		} else {
			if (device->capability &
				DEVICE_CAPABILITY_SMART_SELF_TEST_SUPPORTED) {
				if (!(device->setting &
					DEVICE_SETTING_SMART_ENABLED)) {
					scsi_ata_check_condition(
					req,
					SCSI_SK_ABORTED_COMMAND,
					SCSI_ASC_CONFIGURATION_FAILURE,
					0x0B);
					return MV_QUEUE_COMMAND_RESULT_FINISHED;
				}
			} else {
				scsi_ata_check_condition(req,
				SCSI_SK_ILLEGAL_REQUEST,
				SCSI_ASC_INVALID_FEILD_IN_CDB,
				0);
				return MV_QUEUE_COMMAND_RESULT_FINISHED;
			}
		}

		ctx->req_state = SEND_DIAGNOSTIC_SMART;
		new_req = sat_replicate_req(root, req, 0,
		                scsi_ata_translation_callback);

		if (new_req == NULL)
			return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

		core_append_request(root, new_req);
		return MV_QUEUE_COMMAND_RESULT_REPLACED;

	case SEND_DIAGNOSTIC_VERIFY_0:
		ctx->req_state = SEND_DIAGNOSTIC_VERIFY_MID;
		new_req = sat_replicate_req(root, req, 0,
		        scsi_ata_translation_callback);

		if (new_req == NULL)
			return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

		lba = U64_DIVIDE_U32(device->max_lba, 2);
		scsi_ata_send_diag_verify_fill_req(device, req, lba);
		core_append_request(root, new_req);
		return MV_QUEUE_COMMAND_RESULT_REPLACED;

	case SEND_DIAGNOSTIC_VERIFY_MID:
		ctx->req_state = SEND_DIAGNOSTIC_VERIFY_MAX;
		new_req = sat_replicate_req(root, req, 0,
		        scsi_ata_translation_callback);

		if (new_req == NULL)
			return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

		lba = device->max_lba;
		scsi_ata_send_diag_verify_fill_req(device, req, lba);
		core_append_request(root, new_req);
		return MV_QUEUE_COMMAND_RESULT_REPLACED;

	case SEND_DIAGNOSTIC_SMART:
	case SEND_DIAGNOSTIC_VERIFY_MAX:
		MV_ASSERT(MV_FALSE);
		req->Scsi_Status = REQ_STATUS_ERROR;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;

	default:
		scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

}

MV_U8 scsi_ata_start_stop_callback(MV_PVOID root_p,
        MV_Request *org_req, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	core_context *ctx = (core_context *) req->Context[MODULE_CORE];
	core_context *org_ctx = (core_context *)org_req->Context[MODULE_CORE];
	domain_device *device = (domain_device *)get_device_by_id(root->lib_dev,
	org_req->Device_Id);

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		switch (ctx->req_state) {
			case START_STOP_IDLE_IMMEDIATE_SYNC:
			case START_STOP_STANDBY_IMMEDIATE_SYNC:
			case START_STOP_STANDBY_SYNC:
				scsi_ata_check_condition(org_req,
					SCSI_SK_ABORTED_COMMAND,
					SCSI_ASC_CMD_SEQUENCE_ERROR, 0);
				break;
			case START_STOP_EJECT:
				scsi_ata_check_condition(org_req,
					SCSI_SK_ABORTED_COMMAND,
					SCSI_ASC_MEDIA_LOAD_EJECT_FAILURE, 0);
				break;
			default:
				break;
		}
		return MV_TRUE;
	}

	switch (org_ctx->req_state) {
	case START_STOP_IDLE_IMMEDIATE_SYNC:
	case START_STOP_STANDBY_IMMEDIATE_SYNC:
	case START_STOP_STANDBY_SYNC:
		core_append_request(root, org_req);
		return MV_FALSE;

	case START_STOP_ACTIVE:
		return MV_TRUE;

	case START_STOP_STANDBY_IMMEDIATE:
	case START_STOP_STANDBY:
		return MV_TRUE;

	case START_STOP_IDLE_IMMEDIATE:
	case START_STOP_EJECT:
		return MV_TRUE;

	default:
		MV_ASSERT(MV_FALSE);
		return MV_TRUE;
	}
}

void scsi_ata_start_stop_fill_taskfile(domain_device *device, MV_Request *req,
	ata_taskfile *taskfile)
{
	MV_Request *org_req = sat_get_org_req(req);
	core_context *org_ctx = (core_context *)org_req->Context[MODULE_CORE];

	switch (org_ctx->req_state) {
	case START_STOP_IDLE_IMMEDIATE_SYNC:
	case START_STOP_STANDBY_IMMEDIATE_SYNC:
	case START_STOP_STANDBY_SYNC:
		taskfile->device = MV_BIT(6);
		taskfile->lba_low = 0;
		taskfile->lba_mid = 0;
		taskfile->lba_high = 0;
		taskfile->sector_count = 0;

		taskfile->lba_low_exp = 0;
		taskfile->lba_mid_exp = 0;
		taskfile->lba_high_exp = 0;
		taskfile->sector_count_exp = 0;

		taskfile->features = 0;
		taskfile->feature_exp = 0;
		taskfile->control = 0;

	if (device->capability & DEVICE_CAPABILITY_48BIT_SUPPORTED)
		taskfile->command = ATA_CMD_FLUSH_EXT;
	else
		taskfile->command = ATA_CMD_FLUSH;
		break;
	case START_STOP_ACTIVE:
		taskfile->device = MV_BIT(6);
		taskfile->lba_low = 1; /* {x e Z | 0 < x < max_lba} */
		taskfile->lba_mid = 0;
		taskfile->lba_high = 0;
		taskfile->sector_count = 1;

		taskfile->lba_low_exp = 0;
		taskfile->lba_mid_exp = 0;
		taskfile->lba_high_exp = 0;
		taskfile->sector_count_exp = 0;

		taskfile->features = 0;
		taskfile->feature_exp = 0;
		taskfile->control = 0;

	if (device->capability & DEVICE_CAPABILITY_48BIT_SUPPORTED)
		taskfile->command = ATA_CMD_VERIFY_EXT;
	else
		taskfile->command = ATA_CMD_VERIFY;

		break;
	case START_STOP_EJECT:
		taskfile->command = ATA_CMD_MEDIA_EJECT;
		break;
	default:
		break;
	}
}

/***************************************************************************
* scsi_ata_start_stop_translation
* Purpose: Start and Stop cmd translation
*
***************************************************************************/
MV_U8 scsi_ata_start_stop_translation(domain_device *device, MV_Request *req)
{
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	pl_root *root = device->base.root;
	MV_Request *new_req=NULL;
	MV_U8 power_condition = (req->Cdb[4] & 0xF0) >> 4;

	switch (ctx->req_state) {
	case START_STOP_START:
		req->Time_Out = 60;
		switch (power_condition) {
		case 0x0:
			switch ((req->Cdb[4]) & 0x03) {
			case 0x00:
				if ((req->Cdb[4]) & 0x04)
					goto scsi_ata_start_stop_standby_immediate;
				else if(!(device->capability & DEVICE_CAPABILITY_WRITECACHE_SUPPORTED))
					goto scsi_ata_start_stop_standby_immediate;
				else
					ctx->req_state = START_STOP_STANDBY_IMMEDIATE_SYNC;
				break;
			case 0x01:
				ctx->req_state = START_STOP_ACTIVE;
				break;
			case 0x02:
				ctx->req_state = START_STOP_EJECT;
				break;
			case 0x03:
				scsi_ata_check_condition(req,
					SCSI_SK_ILLEGAL_REQUEST,
					SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
				return MV_QUEUE_COMMAND_RESULT_FINISHED;
			}
			break;
		case 0x1:
			ctx->req_state = START_STOP_ACTIVE;
			break;
		case 0x2:
			if ((req->Cdb[4]) & 0x04)
				goto scsi_ata_start_stop_idle_immediate;
			else
				ctx->req_state = START_STOP_IDLE_IMMEDIATE_SYNC;
			break;
		case 0x3:
			if ((req->Cdb[4]) & 0x04)
				goto scsi_ata_start_stop_standby_immediate;
			else
				ctx->req_state = START_STOP_STANDBY_IMMEDIATE_SYNC;
			break;
		case 0xB:
			if ((req->Cdb[4]) & 0x04)
				goto scsi_ata_start_stop_standby;
			else
				ctx->req_state = START_STOP_STANDBY_SYNC;
			break;
		default:
			scsi_ata_check_condition(req,
				SCSI_SK_ILLEGAL_REQUEST,
				SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}

		new_req = sat_replicate_req(root, req, 0,
			scsi_ata_translation_callback);

		if (new_req == NULL)
			return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

		core_append_request(root, new_req);
		return MV_QUEUE_COMMAND_RESULT_REPLACED;

	case START_STOP_IDLE_IMMEDIATE_SYNC:
scsi_ata_start_stop_idle_immediate:
		ctx->req_state = START_STOP_IDLE_IMMEDIATE;
		new_req = sat_replicate_req(root, req, 0,
			        scsi_ata_translation_callback);

		if (new_req == NULL)
			return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

		new_req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
		new_req->Cdb[1] = CDB_CORE_MODULE;
		new_req->Cdb[2] = CDB_CORE_ATA_IDLE_IMMEDIATE;

		core_append_request(root, new_req);
			return MV_QUEUE_COMMAND_RESULT_REPLACED;

	case START_STOP_STANDBY_IMMEDIATE_SYNC:
scsi_ata_start_stop_standby_immediate:
		ctx->req_state = START_STOP_STANDBY_IMMEDIATE;
		new_req = sat_replicate_req(root, req, 0,
		scsi_ata_translation_callback);

		if (new_req == NULL)
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

		new_req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
		new_req->Cdb[1] = CDB_CORE_MODULE;
		new_req->Cdb[2] = CDB_CORE_ATA_STANDBY_IMMEDIATE;

		core_append_request(root, new_req);
		return MV_QUEUE_COMMAND_RESULT_REPLACED;

	case START_STOP_STANDBY_SYNC:
scsi_ata_start_stop_standby:
		ctx->req_state = START_STOP_STANDBY;
		new_req = sat_replicate_req(root, req, 0,
			scsi_ata_translation_callback);

		if (new_req == NULL)
			return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

		new_req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
		new_req->Cdb[1] = CDB_CORE_MODULE;
		new_req->Cdb[2] = CDB_CORE_ATA_STANDBY;

		core_append_request(root, new_req);
		return MV_QUEUE_COMMAND_RESULT_REPLACED;

	default:
		MV_ASSERT(MV_FALSE);
		req->Scsi_Status = REQ_STATUS_ERROR;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}
}

/***************************************************************************
* scsi_ata_test_unit_ready_callback
* Purpose: Test Unit Ready cmd translation
*
***************************************************************************/
void scsi_ata_test_unit_ready_callback(MV_PVOID root_p,
	MV_Request *org_req, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *device = (domain_device *)get_device_by_id(
	root->lib_dev, org_req->Device_Id);
	saved_fis *fis;
	MV_U32 reg;
	MV_U8 sector_count_reg;

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		scsi_ata_check_condition(org_req, SCSI_SK_NOT_READY,
		        SCSI_ASC_LOGICAL_UNIT_NOT_RESP_TO_SEL, 0);
		return;
	}

	fis = (saved_fis *)ctx->received_fis;
	if (fis == NULL) MV_ASSERT(MV_FALSE);
	reg = fis->dw4;
	sector_count_reg = (MV_U8)(reg & 0xff);

	/*
	ATA 8, Section 7.8

	Sector Count result value -
	00h - device is in Standby mode.
	40h - device is in NV Cache Power Mode and the spindle is
	      spun down or spinning down
	41h - device is in NV Cache Power Mode and the spindle is
	      spun up or spinning up
	80h - device is in Idle mode.
	FFh - device is in Active mode or Idle mode.
	*/
	switch (sector_count_reg) {
	case 0x00:
	/*
	 * Device is in standby mode and thus the device is not ready.
	 * We should return check status to indicate to the applications
	 * that they need to send down a START STOP UNIT command to start
	 * the device.
	 */
		scsi_ata_check_condition(org_req, SCSI_SK_NOT_READY,
			SCSI_ASC_LUN_NOT_READY, 0x02);
		return;
	case 0x80:
	case 0x40:
	case 0x41:
	case 0xFF:
		break;
	default:
		CORE_DPRINT(("Unsupported sector count code 0x%x returned "\
			"from drive\n", sector_count_reg));
		break;
	}

	req->Scsi_Status = REQ_STATUS_SUCCESS;
}

void scsi_ata_test_unit_ready_fill_taskfile(MV_Request *req,
        ata_taskfile *taskfile)
{
	taskfile->command = ATA_CMD_PM_CHECK;
	taskfile->device = MV_BIT(6);

	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;
	taskfile->sector_count = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->features = 0;
	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

/***************************************************************************
* scsi_ata_test_unit_ready_translation
* Purpose: Test Unit Ready cmd translation
*
***************************************************************************/
MV_U8 scsi_ata_test_unit_ready_translation(domain_device *device,
	MV_Request *req)
{
	MV_Request *new_req;
	pl_root *root = device->base.root;

	if (device->state == DEVICE_STATE_FORMAT_WRITE ||
		device->state == DEVICE_STATE_FORMAT_VERIFY) {
		scsi_ata_check_condition(req, SCSI_SK_NOT_READY,
			SCSI_ASC_LUN_NOT_READY, 0x04);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	new_req = sat_replicate_req(root, req, 0,
		scsi_ata_translation_callback);

	if (new_req == NULL) return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

	((core_context *)new_req->Context[MODULE_CORE])->req_flag |=
		CORE_REQ_FLAG_NEED_D2H_FIS;

	core_append_request(root, new_req);
	return MV_QUEUE_COMMAND_RESULT_REPLACED;
}

void scsi_ata_request_sense_callback(MV_PVOID root_p,
        MV_Request *org_req, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *device = (domain_device *)get_device_by_id(
	root->lib_dev, org_req->Device_Id);
	saved_fis *fis;
	MV_PU8 dest;
	MV_U32 reg;
	MV_U8 lba_mid, lba_high;

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) return;

	fis = (saved_fis *)ctx->received_fis;
	if (fis == NULL) MV_ASSERT(MV_FALSE);
	reg = fis->dw2;
	lba_mid = (MV_U8)((reg >> 8) & 0xff);
	lba_high = (MV_U8)((reg >> 16) & 0xff);

	dest = (MV_PU8)core_map_data_buffer(org_req);

	/* 4Fh if threshold not exceeded, F4h if threshold exceeded.
	C2h if threshold not exceeded, 2Ch if threshold exceeded.
	MRIE is set to 6h
	DExcpt bit is set to 0
	*/
	if (lba_mid == 0xF4 || lba_high == 0x2C) {
		scsi_ata_check_condition(org_req,
		SCSI_ASC_FAILURE_PREDICTION_THRESHOLD_EXCEEDED,
		SCSI_ASCQ_HIF_GENERAL_HD_FAILURE, 0);
	} else {
		req->Scsi_Status = REQ_STATUS_SUCCESS;
	}

	if (dest != NULL &&
		org_req->Data_Transfer_Length >= 13) {     /*This added is for DTM*/
		dest[0] = 0x70; /* Current */
		dest[2] = 0x00; /* Sense Key*/
		dest[7] = 0x00; /* additional sense len */
		dest[12] = 0x00;/* additional sense code */
	}

	core_unmap_data_buffer(org_req);
}

void scsi_ata_request_sense_fill_taskfile(MV_Request *req,
        ata_taskfile *taskfile)
{
	taskfile->device = MV_BIT(6);
	taskfile->lba_mid = 0x4F;
	taskfile->lba_high = 0xC2;
	taskfile->command = ATA_CMD_SMART;
	taskfile->features = ATA_CMD_SMART_RETURN_STATUS;

	taskfile->lba_low = 0;
	taskfile->sector_count = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

/***************************************************************************
* scsi_ata_request_sense_translation
* Purpose: Request sense translation
*
***************************************************************************/
MV_U8 scsi_ata_request_sense_translation(domain_device *device,
	MV_Request *req)
{
	MV_Request *new_req;
	pl_root *root = device->base.root;

	if (!(device->setting & DEVICE_SETTING_SMART_ENABLED)) {
		scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
		SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	if (device->state == DEVICE_STATE_FORMAT_WRITE ||
		device->state == DEVICE_STATE_FORMAT_VERIFY) {
		scsi_ata_check_condition(req, SCSI_SK_NOT_READY,
			SCSI_ASC_LUN_NOT_READY, 0x04);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	new_req = sat_replicate_req(root, req, 0,
		scsi_ata_translation_callback);

	if (new_req == NULL) return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

	((core_context *)new_req->Context[MODULE_CORE])->req_flag |=
		CORE_REQ_FLAG_NEED_D2H_FIS;

	core_append_request(root, new_req);
	return MV_QUEUE_COMMAND_RESULT_REPLACED;
}

void scsi_ata_log_sense_callback(MV_PVOID root_p,
        MV_Request *org_req, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *device = (domain_device *)get_device_by_id(
	root->lib_dev, req->Device_Id);
	saved_fis *fis;
	MV_U32 reg;
	MV_U8 lba_mid, lba_high;
	MV_U32 length;
	MV_PU8 buf_ptr;

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) return;

	length = MV_MIN(org_req->Data_Transfer_Length, 11);
	buf_ptr = core_map_data_buffer(org_req);

	if (buf_ptr == NULL || length == 0) {
		core_unmap_data_buffer(org_req);
		return;
	}

	fis = (saved_fis *)ctx->received_fis;
	if (fis == NULL) MV_ASSERT(MV_FALSE);
	reg = fis->dw2;

	lba_mid = (MV_U8)((reg >> 8) & 0xff);
	lba_high = (MV_U8)((reg >> 16) & 0xff);

	buf_ptr[0] = INFORMATIONAL_EXCEPTIONS_LOG_PAGE;
	buf_ptr[1] = 0; /* reserved */
	buf_ptr[2] = 0;
	buf_ptr[3] = 7;
	buf_ptr[4] = 0;
	buf_ptr[5] = 0; /* parameter code = 0*/
	buf_ptr[6] = 0x03; /* DU=0,DS=0,TSD=0,ETC=0,TMC=0,LBIN=1,LP=1 */
	buf_ptr[7] = 0x03; /* parameter length = 3 */
	if (lba_mid == 0xF4 || lba_high == 0x2C) {
		buf_ptr[8] = SCSI_ASC_FAILURE_PREDICTION_THRESHOLD_EXCEEDED;
		buf_ptr[9] = SCSI_ASCQ_HIF_GENERAL_HD_FAILURE;
	} else {
		buf_ptr[8] = 0;
		buf_ptr[9] = 0;
	}
	buf_ptr[10] = 38; /* MOST RECENT TEMPERATURE READING */

	core_unmap_data_buffer(org_req);
}

void scsi_ata_log_sense_fill_taskfile(MV_Request *req,
        ata_taskfile *taskfile)
{
	taskfile->device = MV_BIT(6);
	taskfile->lba_mid = 0x4F;
	taskfile->lba_high = 0xC2;
	taskfile->command = ATA_CMD_SMART;
	taskfile->features = ATA_CMD_SMART_RETURN_STATUS;

	taskfile->lba_low = 0;
	taskfile->sector_count = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

MV_U8 scsi_ata_log_sense_translation(domain_device *device,
	MV_Request *req)
{
	MV_Request *new_req;
	pl_root *root = device->base.root;

	if (!(device->setting & DEVICE_SETTING_SMART_ENABLED)) {
		scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
		SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	if (device->state == DEVICE_STATE_FORMAT_WRITE ||
		device->state == DEVICE_STATE_FORMAT_VERIFY) {
		scsi_ata_check_condition(req, SCSI_SK_NOT_READY,
		SCSI_ASC_LUN_NOT_READY, 0x04);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	new_req = sat_replicate_req(root, req, 0,
	scsi_ata_translation_callback);

	if (new_req == NULL) return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

	((core_context *)new_req->Context[MODULE_CORE])->req_flag |=
		CORE_REQ_FLAG_NEED_D2H_FIS;

	core_append_request(root, new_req);
	return MV_QUEUE_COMMAND_RESULT_REPLACED;
}

MV_U8 scsi_ata_mode_select_callback(MV_PVOID root_p,
        MV_Request *org_req, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *device = (domain_device *)get_device_by_id(
	root->lib_dev, req->Device_Id);
	MV_U8 finished = MV_TRUE;
	MV_PU8 data_buf;

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		org_req->Scsi_Status = req->Scsi_Status;
		return MV_TRUE;
	}

	data_buf = (MV_PU8)core_map_data_buffer(org_req);
	switch (req->Cdb[2]) {
	case CDB_CORE_ENABLE_WRITE_CACHE:
		device->setting |= DEVICE_SETTING_WRITECACHE_ENABLED;
		if (device->setting & DEVICE_SETTING_READ_LOOK_AHEAD) {
			if (data_buf[12] & MV_BIT(5)) {
				req->Cdb[2] = CDB_CORE_DISABLE_READ_AHEAD;
				finished = MV_FALSE;
			}
		} else {
			if (!(data_buf[12] & MV_BIT(5))) {
				req->Cdb[2] = CDB_CORE_ENABLE_READ_AHEAD;
				finished = MV_FALSE;
			}
		}
		break;
	case CDB_CORE_DISABLE_WRITE_CACHE:
		device->setting &= ~DEVICE_SETTING_WRITECACHE_ENABLED;
		if (device->setting & DEVICE_SETTING_READ_LOOK_AHEAD) {
			if (data_buf[12] & MV_BIT(5)) {
				req->Cdb[2] = CDB_CORE_DISABLE_READ_AHEAD;
				finished = MV_FALSE;
			}
		} else {
			if (!(data_buf[12] & MV_BIT(5))) {
				req->Cdb[2] = CDB_CORE_ENABLE_READ_AHEAD;
				finished = MV_FALSE;
			}
		}
		break;
	case CDB_CORE_ENABLE_READ_AHEAD:
		device->setting &= ~DEVICE_SETTING_READ_LOOK_AHEAD;
		break;
	case CDB_CORE_DISABLE_READ_AHEAD:
		device->setting |= DEVICE_SETTING_READ_LOOK_AHEAD;
		break;
	default:
		break;
	}

	core_unmap_data_buffer(org_req);

	if (finished == MV_FALSE)
		return MV_FALSE;

	req->Scsi_Status = REQ_STATUS_SUCCESS;
	return MV_TRUE;
}

void scsi_ata_mode_select_fill_taskfile(MV_Request *req,
        ata_taskfile *taskfile)
{
	taskfile->command = 0;
	taskfile->device = MV_BIT(6);
	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;
	taskfile->sector_count = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->features = 0;
	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

MV_U8 scsi_ata_mode_select_translation(domain_device *device,
        MV_Request *req)
{
	MV_U32 offset;
	MV_PU8 data_buf;
	MV_U32 length;
	MV_Request *new_req;
	pl_root *root = device->base.root;
	MV_U8 func = 0xFF;

	length = req->Data_Transfer_Length;
	data_buf = core_map_data_buffer(req);
	offset = 4 + data_buf[3];

	data_buf = &data_buf[offset];
	switch (data_buf[0] & 0x3f) {
	case CACHE_MODE_PAGE:
		if (data_buf[1] != 0x12) {
			scsi_ata_check_condition(req,
				SCSI_SK_ILLEGAL_REQUEST,
				SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
			core_unmap_data_buffer(req);
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		}

		if (device->setting &
			DEVICE_SETTING_WRITECACHE_ENABLED) {
			if (!(data_buf[2] & MV_BIT(2)))
				func = CDB_CORE_DISABLE_WRITE_CACHE;
		} else {
			if (data_buf[2] & MV_BIT(2))
				func = CDB_CORE_ENABLE_WRITE_CACHE;
		}

		if (device->setting & DEVICE_SETTING_READ_LOOK_AHEAD
			&& func == 0xFF) {
			if (data_buf[12] & MV_BIT(5))
				func = CDB_CORE_DISABLE_READ_AHEAD;
		} else {
			if (!(data_buf[12] & MV_BIT(5)))
				func = CDB_CORE_ENABLE_READ_AHEAD;
		}
		break;
	default:
		scsi_ata_check_condition(req,
		SCSI_SK_ILLEGAL_REQUEST,
		SCSI_ASC_INVALID_FEILD_IN_CDB, 0);

		core_unmap_data_buffer(req);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	if (func != 0xFF) {
		new_req = sat_replicate_req(root, req, 0,
		scsi_ata_translation_callback);

		if (new_req == NULL) {
			core_unmap_data_buffer(req);
			return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
		}

		new_req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
		new_req->Cdb[1] = CDB_CORE_MODULE;
		new_req->Cdb[2] = func;

		core_unmap_data_buffer(req);
		core_append_request(root, new_req);
		return MV_QUEUE_COMMAND_RESULT_REPLACED;
	}

	core_unmap_data_buffer(req);
	req->Scsi_Status = REQ_STATUS_SUCCESS;
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_U8 scsi_ata_reassign_blocks_callback(MV_PVOID root_p,
        MV_Request *org_req, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	core_context *org_ctx = (core_context *)org_req->Context[MODULE_CORE];
	domain_device *device = (domain_device *)get_device_by_id(
	root->lib_dev, req->Device_Id);

	MV_U8 long_lba;
	MV_U8 long_list;
	MV_U32 list_length;
	MV_U32 offset;
	MV_PU8 buf_ptr;

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		if (org_ctx->req_state & REASSIGN_BLOCKS_ERROR) {
			switch (req->Cdb[0]) {
			case SCSI_CMD_VERIFY_10:
			case SCSI_CMD_VERIFY_16:
			case SCSI_CMD_WRITE_10:
			case SCSI_CMD_WRITE_16:
			scsi_ata_check_condition(org_req,
				SCSI_SK_MEDIUM_ERROR,
				SCSI_ASC_UNRECOVERED_READ_ERROR, 0x04);
				return MV_TRUE;
			default:
				MV_DASSERT(MV_FALSE);
				return MV_TRUE;
			}
		} else {
			switch (req->Cdb[0]) {
			case SCSI_CMD_VERIFY_10:
			case SCSI_CMD_VERIFY_16:
				org_ctx->req_state |= REASSIGN_BLOCKS_ERROR;
				goto scsi_ata_reassign_blocks_callback_end;

			case SCSI_CMD_WRITE_10:
			case SCSI_CMD_WRITE_16:
			default:
				MV_ASSERT(MV_FALSE);
				return MV_TRUE;
			}
		}

	}

	buf_ptr = core_map_data_buffer(org_req);
	long_list = org_req->Cdb[1] & 0x01;
	long_lba = org_req->Cdb[1] & 0x02;

	if (long_list)
		list_length = (buf_ptr[0] << 24)
		+ (buf_ptr[1] << 16)
		+ (buf_ptr[2] << 8)
		+ buf_ptr[3];
	else
		list_length = (buf_ptr[2] << 8) + buf_ptr[3];

	if (long_lba)
		offset = (org_ctx->req_state & 0x3FFFFFFF) << 3;
	else
		offset = (org_ctx->req_state & 0x3FFFFFFF) << 2;

	core_unmap_data_buffer(org_req);

	if (org_ctx->req_state & REASSIGN_BLOCKS_ERROR) {
		switch (req->Cdb[0]) {
		case SCSI_CMD_VERIFY_10:
		case SCSI_CMD_VERIFY_16:
			org_ctx->req_state &= ~REASSIGN_BLOCKS_ERROR;
			if (list_length == offset)
				return MV_TRUE;
			else
				goto scsi_ata_reassign_blocks_callback_end;
			break;
		case SCSI_CMD_WRITE_10:
		case SCSI_CMD_WRITE_16:
			goto scsi_ata_reassign_blocks_callback_end;
		default:
			MV_DASSERT(MV_FALSE);
			return MV_TRUE;
		}
	} else {
		switch (req->Cdb[0]) {
		case SCSI_CMD_VERIFY_10:
		case SCSI_CMD_VERIFY_16:
			if (list_length == offset)
				return MV_TRUE;
			else
				goto scsi_ata_reassign_blocks_callback_end;
			break;
		case SCSI_CMD_WRITE_10:
		case SCSI_CMD_WRITE_16:
		default:
			MV_DASSERT(MV_FALSE);
			return MV_TRUE;
		}
	}

	scsi_ata_reassign_blocks_callback_end:
	core_append_request(root, org_req);
	return MV_FALSE;
}

void scsi_ata_reassign_blocks_fill_taskfile(MV_Request *req,
        ata_taskfile *taskfile)
{
	taskfile->command = 0;
	taskfile->device = MV_BIT(6);
	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;
	taskfile->sector_count = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->features = 0;
	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

MV_U8 scsi_ata_reassign_blocks_translation(domain_device *device,
        MV_Request *req)
{
	pl_root *root = device->base.root;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_PU8 org_buf_ptr, new_buf_ptr;
	MV_U32 buf_size = req->Data_Transfer_Length;
	MV_U8 long_list = req->Cdb[1] & 0x01;
	MV_U8 long_lba = req->Cdb[1] & 0x02;
	MV_U32 list_length;
	MV_U32 offset;
	MV_Request *new_req;

	/* defective LBA list containing one or more LBAs */
	if ((!long_lba && buf_size < 8)
		|| (long_lba && buf_size < 12)) {
		scsi_ata_check_condition(req,
		SCSI_SK_ILLEGAL_REQUEST,
		SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	org_buf_ptr = core_map_data_buffer(req);

	if (long_list)
		list_length = (org_buf_ptr[0] << 24)
		+ (org_buf_ptr[1] << 16)
		+ (org_buf_ptr[2] << 8)
		+ org_buf_ptr[3];
	else
		list_length = (org_buf_ptr[2] << 8) + org_buf_ptr[3];

	/*
	The DEFECT LIST LENGTH field does not include the parameter list
	header length and is equal to either:

	four times the number of LBAs, if the LONGLBA bit is set to zero; or
	eight times the number of LBAs, if the LONGLBA bit is set to one.
	*/

	if ((!long_lba && (list_length & 0x03))
		|| (long_lba && (list_length & 0x07))) {

	scsi_ata_check_condition(req,
			SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
		core_unmap_data_buffer(req);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	if (long_lba)
		offset = (ctx->req_state & 0x3FFFFFFF) << 3;
	else
		offset = (ctx->req_state & 0x3FFFFFFF) << 2;

	org_buf_ptr = &org_buf_ptr[4 + offset];

	if (!(ctx->req_state & REASSIGN_BLOCKS_ERROR)) {
		/* if status is okay, then issue verify */
		ctx->req_state++;
		new_req = sat_replicate_req(root, req, 0,
		scsi_ata_translation_callback);

		if (new_req == NULL) {
			core_unmap_data_buffer(req);
			return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
		}

		if (long_lba) {
			new_req->Cdb[0] = SCSI_CMD_VERIFY_16;
			new_req->Cdb[4] = org_buf_ptr[2];
			new_req->Cdb[5] = org_buf_ptr[3];
			new_req->Cdb[6] = org_buf_ptr[4];
			new_req->Cdb[7] = org_buf_ptr[5];
			new_req->Cdb[8] = org_buf_ptr[6];
			new_req->Cdb[9] = org_buf_ptr[7];
			new_req->Cdb[13] = 1;
		} else {
			new_req->Cdb[0] = SCSI_CMD_VERIFY_10;
			new_req->Cdb[2] = org_buf_ptr[0];
			new_req->Cdb[3] = org_buf_ptr[1];
			new_req->Cdb[4] = org_buf_ptr[2];
			new_req->Cdb[5] = org_buf_ptr[3];
			new_req->Cdb[8] = 1;
		}
	} else {
		/* if status is error, then issue write */
		new_req = sat_replicate_req(root, req,
			device->sector_size,
			scsi_ata_translation_callback);

		if (new_req == NULL) {
			core_unmap_data_buffer(req);
			return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
		}

		new_buf_ptr = core_map_data_buffer(new_req);
		MV_ZeroMemory(new_buf_ptr,
		new_req->Data_Transfer_Length);
		core_unmap_data_buffer(new_req);

		if (long_lba) {
			new_req->Cdb[0] = SCSI_CMD_WRITE_16;
			new_req->Cdb[4] = org_buf_ptr[2];
			new_req->Cdb[5] = org_buf_ptr[3];
			new_req->Cdb[6] = org_buf_ptr[4];
			new_req->Cdb[7] = org_buf_ptr[5];
			new_req->Cdb[8] = org_buf_ptr[6];
			new_req->Cdb[9] = org_buf_ptr[7];
			new_req->Cdb[13] = 1;

		} else {
			new_req->Cdb[0] = SCSI_CMD_WRITE_10;
			new_req->Cdb[2] = org_buf_ptr[0];
			new_req->Cdb[3] = org_buf_ptr[1];
			new_req->Cdb[4] = org_buf_ptr[2];
			new_req->Cdb[5] = org_buf_ptr[3];
			new_req->Cdb[8] = 1;
		}
	}

	core_unmap_data_buffer(new_req);
	core_append_request(root, new_req);
	return MV_QUEUE_COMMAND_RESULT_REPLACED;
}

void scsi_ata_soft_reset_fill_taskfile(MV_Request *req,
        ata_taskfile* taskfile)
{
	if (req->Cdb[2] == CDB_CORE_SOFT_RESET_1)
		taskfile->control = MV_BIT(2);
	else
		taskfile->control = 0;

	taskfile->command = 0;
	taskfile->device = MV_BIT(6);
	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;
	taskfile->sector_count = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->features = 0;
	taskfile->feature_exp = 0;
}

void scsi_atapi_core_identify_fill_taskfile(MV_Request *req,
        ata_taskfile* taskfile)
{
	taskfile->command = ATA_CMD_IDENTIFY_ATAPI;
	taskfile->device = MV_BIT(6);
	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;
	taskfile->sector_count = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->features = 0;
	taskfile->feature_exp = 0;
	taskfile->control = 0;
}


void scsi_ata_core_identify_fill_taskfile(MV_Request *req,
        ata_taskfile* taskfile)
{
	taskfile->command = ATA_CMD_IDENTIFY_ATA;
	taskfile->device = MV_BIT(6);
	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;
	taskfile->sector_count = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->features = 0;
	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

void scsi_ata_set_udma_mode_fill_taskfile(domain_device *device,
	MV_Request *req, ata_taskfile* taskfile)
{
	if (!IS_ATAPI(device)) {
		/* Use UDMA mode */
		MV_DASSERT(req->Cdb[4] == MV_FALSE);
	}

	taskfile->command = ATA_CMD_SET_FEATURES;
	taskfile->features = ATA_CMD_SET_TRANSFER_MODE;
	taskfile->device = MV_BIT(6);

	if (req->Cdb[4] == MV_TRUE && !IS_ATAPI(device))
		taskfile->sector_count = 0x20 | req->Cdb[3]; /* MDMA mode */
	else
		taskfile->sector_count = 0x40 | req->Cdb[3]; /* UDMA mode*/

	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

void scsi_ata_set_pio_mode_fill_taskfile(MV_Request *req,
        ata_taskfile* taskfile)
{
	taskfile->command = ATA_CMD_SET_FEATURES;
	taskfile->features = ATA_CMD_SET_TRANSFER_MODE;
	taskfile->sector_count = 0x08 | req->Cdb[3];
	taskfile->device = MV_BIT(6);

	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

void scsi_ata_write_cache_fill_taskfile(MV_Request *req,
        ata_taskfile* taskfile)
{
	taskfile->command = ATA_CMD_SET_FEATURES;
	taskfile->device = MV_BIT(6);

	if (req->Cdb[2] == CDB_CORE_ENABLE_WRITE_CACHE)
		taskfile->features = ATA_CMD_ENABLE_WRITE_CACHE;
	else
		taskfile->features = ATA_CMD_DISABLE_WRITE_CACHE;

	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;
	taskfile->sector_count = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

void scsi_ata_pois_to_spinup_fill_taskfile(MV_Request *req,
        ata_taskfile* taskfile)
{
	taskfile->command = ATA_CMD_SET_FEATURES;
	taskfile->device = MV_BIT(6);

	if (req->Cdb[2] == CDB_CORE_SET_FEATURE_SPINUP)
		taskfile->features = ATA_CMD_SET_POIS_SPINUP;

	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;
	taskfile->sector_count = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

void scsi_ata_enable_smart_fill_taskfile(MV_Request *req,
        ata_taskfile *taskfile)
{
	taskfile->command = ATA_CMD_SMART;
	taskfile->device = MV_BIT(6);

	if (req->Cdb[2] == CDB_CORE_ENABLE_SMART)
		taskfile->features = ATA_CMD_ENABLE_SMART;
	else
		taskfile->features = ATA_CMD_DISABLE_SMART;

	taskfile->lba_low = 0;
	taskfile->lba_mid = 0x4F;
	taskfile->lba_high = 0xC2;
	taskfile->sector_count = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

MV_U8 scsi_ata_enable_smart_translation(domain_device *device,
        MV_Request *req)
{
	MV_Request *new_req;
	pl_root *root = device->base.root;

	new_req = sat_replicate_req(root, req, 0,
		scsi_ata_translation_callback);

	if (new_req == NULL)
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

	core_append_request(root, new_req);
	return MV_QUEUE_COMMAND_RESULT_REPLACED;
}

void scsi_ata_smart_return_status_callback(pl_root *root,
        MV_Request *org_req, MV_Request *req)
{
	HD_SMART_Status *status;
	MV_U32 reg, lba_mid, lba_high;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *device = (domain_device *)get_device_by_id(
	root->lib_dev, org_req->Device_Id);
	saved_fis *fis;

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) return;

	status = (HD_SMART_Status *)core_map_data_buffer(org_req);
	if (status == NULL
		|| org_req->Data_Transfer_Length == 0) {

		core_unmap_data_buffer(org_req);
		return;
	}

	fis = (saved_fis *)ctx->received_fis;
	if (fis == NULL) MV_ASSERT(MV_FALSE);
	reg = fis->dw2;

	lba_mid = (MV_U8)((reg >> 8) & 0xff);
	lba_high = (MV_U8)((reg >> 16) & 0xff);

	if (lba_mid == 0xF4 && lba_high == 0x2C) {
		status->SmartThresholdExceeded = MV_TRUE;
	}
	else if (lba_mid == 0x4F && lba_high == 0xC2) {
		status->SmartThresholdExceeded = MV_FALSE;
	}
	else {
		status->SmartThresholdExceeded = MV_FALSE;
		CORE_DPRINT(("Invalid register value. LBA Mid:%x LBA High:%x\n",
			lba_mid, lba_high));
	}
	core_unmap_data_buffer(org_req);
}

void scsi_ata_smart_return_status_fill_taskfile(MV_Request *req,
        ata_taskfile *taskfile)
{
	taskfile->command = ATA_CMD_SMART;
	taskfile->features = ATA_CMD_SMART_RETURN_STATUS;
	taskfile->device = MV_BIT(6);

	taskfile->lba_low = 0;
	taskfile->lba_mid = 0x4F;
	taskfile->lba_high = 0xC2;
	taskfile->sector_count = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

MV_U8 scsi_ata_smart_return_status_translation(domain_device *device,
        MV_Request *req)
{
	MV_Request *new_req;
	pl_root *root = device->base.root;

	new_req = sat_replicate_req(root, req, 0,
	scsi_ata_translation_callback);

	if (new_req == NULL) return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

	((core_context *)new_req->Context[MODULE_CORE])->req_flag |=
		CORE_REQ_FLAG_NEED_D2H_FIS;

	core_append_request(root, new_req);
	return MV_QUEUE_COMMAND_RESULT_REPLACED;
}
#define BAD_SSD_DEVICE_MODEL    "Patriot Inferno 60GB SSD"
extern MV_U8 gRunningMode;
void scsi_ata_shudown_fill_taskfile(domain_device *device, MV_Request *req,
        ata_taskfile *taskfile)
{
	if (device->capability &
	DEVICE_CAPABILITY_48BIT_SUPPORTED)
		taskfile->command = ATA_CMD_FLUSH_EXT;
	else
		taskfile->command = ATA_CMD_FLUSH;
	taskfile->features = 0;
	taskfile->device = MV_BIT(6);

	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;
	taskfile->sector_count = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

void scsi_ata_enable_read_ahead_fill_taskfile(MV_Request *req,
        ata_taskfile *taskfile)
{
	taskfile->command = ATA_CMD_SET_FEATURES;
	taskfile->device = MV_BIT(6);

	if (req->Cdb[2] == CDB_CORE_ENABLE_READ_AHEAD)
		taskfile->features = ATA_CMD_ENABLE_READ_LOOK_AHEAD;
	else
		taskfile->features = ATA_CMD_DISABLE_READ_LOOK_AHEAD;

	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;
	taskfile->sector_count = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

void scsi_ata_read_log_fill_taskfile(MV_Request *req,
        ata_taskfile *taskfile)
{
	taskfile->command = ATA_CMD_READ_LOG_EXT;
	taskfile->device = MV_BIT(6);
	taskfile->features = 0;

	taskfile->lba_low = 0x10;       /* Read page 10 */
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;
	taskfile->sector_count = 1;     /* Read 1 sector */

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

void scsi_ata_pm_read_reg_fill_taskfile(MV_Request *req,
        ata_taskfile *taskfile)
{
	taskfile->command = ATA_CMD_PM_READ;
	taskfile->features = req->Cdb[4];
	taskfile->device = req->Cdb[3];

	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;
	taskfile->sector_count = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

void scsi_ata_pm_write_reg_fill_taskfile(MV_Request *req,
        ata_taskfile *taskfile)
{
	taskfile->command = ATA_CMD_PM_WRITE;
	taskfile->features = req->Cdb[4];
	taskfile->device = req->Cdb[3];

	taskfile->lba_low = req->Cdb[5];
	taskfile->lba_mid = req->Cdb[6];
	taskfile->lba_high = req->Cdb[7];
	taskfile->sector_count = req->Cdb[8];

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

void scsi_ata_os_smart_cmd_callback(MV_PVOID root_p,
        MV_Request *org_req, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	core_extension *core = (core_extension *)root->core;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *device;
	saved_fis *fis;
	MV_U32 reg;
	MV_U8 lba_high, lba_mid, status_reg, err_reg;

	device = (domain_device *)get_device_by_id(root->lib_dev,
		req->Device_Id);
	req->Data_Buffer = NULL;
	req->Data_Transfer_Length = 0;
	ctx->buf_wrapper = NULL;
	SGTable_Init(&req->SG_Table, 0);

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) return;

	/* return identify device data as is */
	if (org_req->Cdb[3] == ATA_CMD_IDENTIFY_ATA) return;

	fis = (saved_fis *)ctx->received_fis;
	if (fis == NULL) MV_ASSERT(MV_FALSE);
	reg = fis->dw2;

	lba_mid = (MV_U8)((reg >> 8) & 0xff);
	lba_high = (MV_U8)((reg >> 16) & 0xff);

	switch (org_req->Cdb[4]) {
	case ATA_CMD_SMART_READ_DATA:
	case ATA_CMD_SMART_READ_ATTRIBUTE_THRESHOLDS:
	case ATA_CMD_SMART_ENABLE_ATTRIBUTE_AUTOSAVE:
	case ATA_CMD_SMART_SAVE_ATTRIBUTE_VALUES:
	case ATA_CMD_SMART_READ_LOG:
	case ATA_CMD_SMART_WRITE_LOG:
	case ATA_CMD_ENABLE_SMART:
	case ATA_CMD_DISABLE_SMART:
		break;

	case ATA_CMD_SMART_EXECUTE_OFFLINE:
		if (lba_mid == 0xF4 && lba_high == 0x2C) {
			scsi_ata_check_condition(org_req, 
				SCSI_SK_ABORTED_COMMAND, SCSI_ASC_NO_ASC, 0);
			org_req->Cdb[6] = 0xF4;
			org_req->Cdb[7] = 0x2C;
		}
		break;
	case ATA_CMD_SMART_RETURN_STATUS:
		if (lba_mid == 0xF4 && lba_high == 0x2C) {
			core_generate_event(core,
				EVT_ID_HD_SMART_THRESHOLD_OVER,
				device->base.id, SEVERITY_INFO,
				0, NULL ,0);
			org_req->Cdb[6] = 0xF4;
			org_req->Cdb[7] = 0x2C;
		}
		break;
	default:
		MV_DASSERT(MV_FALSE);
		break;
	}
}

void scsi_ata_os_smart_cmd_fill_taskfile(MV_Request *req,
        ata_taskfile *taskfile)
{
	taskfile->command = req->Cdb[3];
	taskfile->features = req->Cdb[4];
	taskfile->device = MV_BIT(6);

	taskfile->lba_low = req->Cdb[5];
	taskfile->lba_mid  = req->Cdb[6];
	taskfile->lba_high = req->Cdb[7];
	taskfile->sector_count = req->Cdb[8];

	taskfile->lba_low_exp = req->Cdb[9];
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

MV_U8 scsi_ata_os_smart_cmd_translation(domain_device *device,
        MV_Request *req)
{
	MV_Request *new_req;
	pl_root *root = device->base.root;

	if (req->Cdb[3] != ATA_CMD_SMART &&
		req->Cdb[3] != ATA_CMD_IDENTIFY_ATA) {
		scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	if (req->Cdb[3] == ATA_CMD_SMART &&
		(req->Cdb[4] < ATA_CMD_SMART_READ_DATA ||
		req->Cdb[4] > ATA_CMD_SMART_RETURN_STATUS ||
		req->Cdb[4] == 0xD7)) {
		scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
			SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	new_req = sat_replicate_req(root, req, 0,
		scsi_ata_translation_callback);

	if (new_req == NULL) return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

	new_req->Data_Buffer = req->Data_Buffer;
	new_req->Data_Transfer_Length = req->Data_Transfer_Length;
	((core_context *)new_req->Context[MODULE_CORE])->buf_wrapper
		= ((core_context *)req->Context[MODULE_CORE])->buf_wrapper;
	((core_context *)new_req->Context[MODULE_CORE])->req_flag |=
		CORE_REQ_FLAG_NEED_D2H_FIS;

	if (req->Data_Transfer_Length > 0) {
		MV_CopyPartialSGTable(
			&new_req->SG_Table,
			&req->SG_Table,
			0, /* offset */
			req->SG_Table.Byte_Count /* size */
		);
	}

	if (req->Cdb[3] == ATA_CMD_IDENTIFY_ATA) {
		new_req->Cdb[0] = SCSI_CMD_INQUIRY;
		MV_ZeroMemory(&new_req->Cdb[1], sizeof(new_req->Cdb) - 1);
	}

	core_append_request(root, new_req);
	return MV_QUEUE_COMMAND_RESULT_REPLACED;
}

void
scsi_ata_fill_taskfile_cmd(MV_Request *req,
        ata_taskfile *taskfile, MV_U8 cmd)
{
	taskfile->command = cmd;
	taskfile->features = 0;
	taskfile->device = MV_BIT(6);

	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;
	taskfile->sector_count = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

void scsi_ata_api_request_callback(MV_PVOID root_p,
	MV_Request *org_req, MV_Request *req)
{
	pl_root *root = root_p;
	core_extension *core = (core_extension *)root->core;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *dev = (domain_device *)get_device_by_id(root->lib_dev,
	req->Device_Id);
	saved_fis *fis;

	HD_SMART_Status *status;
	MV_U32 reg;
	MV_U8 lba_mid, lba_high;

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		if (org_req->Cdb[1] == APICDB1_PD_GETSTATUS &&
			org_req->Cdb[4] == APICDB4_PD_SMART_RETURN_STATUS)
			core_generate_event(core, EVT_ID_HD_SMART_POLLING_FAIL,
				dev->base.id, SEVERITY_WARNING, 0, NULL ,0);
		return;
	}

	fis = (saved_fis *)ctx->received_fis;
	if (fis == NULL) MV_ASSERT(MV_FALSE);

	switch (org_req->Cdb[1]) {
	case APICDB1_PD_SETSETTING:
		switch (org_req->Cdb[4]) {
		case APICDB4_PD_SET_WRITE_CACHE_OFF:
			dev->setting &= ~DEVICE_SETTING_WRITECACHE_ENABLED;
			core_generate_event(core, EVT_ID_HD_CACHE_MODE_CHANGE,
			dev->base.id, SEVERITY_INFO, 0, NULL ,0);
			break;
		case APICDB4_PD_SET_WRITE_CACHE_ON:
			dev->setting |= DEVICE_SETTING_WRITECACHE_ENABLED;
			core_generate_event(core, EVT_ID_HD_CACHE_MODE_CHANGE,
			dev->base.id, SEVERITY_INFO, 0, NULL ,0);
			break;
		case APICDB4_PD_SET_SMART_OFF:
			dev->setting &= ~DEVICE_SETTING_SMART_ENABLED;
			break;
		case APICDB4_PD_SET_SMART_ON:
			dev->setting |= DEVICE_SETTING_SMART_ENABLED;
			break;
		default:
			MV_ASSERT(MV_FALSE);
			break;
		}
		break;

	case APICDB1_PD_GETSTATUS:
	switch (org_req->Cdb[4]) {
	case APICDB4_PD_SMART_RETURN_STATUS:
		reg = fis->dw2;
		lba_mid = (MV_U8)((reg >> 8) & 0xff);
		lba_high = (MV_U8)((reg >> 16) & 0xff);
		status = (HD_SMART_Status *)core_map_data_buffer(org_req);
		if (lba_mid == 0xF4 && lba_high == 0x2C) {
			core_generate_event(core,
				EVT_ID_HD_SMART_THRESHOLD_OVER,
				dev->base.id, SEVERITY_WARNING,
				0, NULL ,0);
		} else {
			if (status != NULL && org_req->Data_Transfer_Length != 0)
				status->SmartThresholdExceeded = MV_FALSE;
		}
		core_unmap_data_buffer(org_req);
		break;
	default:
		MV_ASSERT(MV_FALSE);
		break;
	}
		break;

	default:
		MV_ASSERT(MV_FALSE);
		break;
	}
}

MV_U8
scsi_ata_api_request_translation(domain_device *dev,
        MV_Request *req, MV_U8 cmd)
{
	MV_Request *new_req;
	pl_root *root = dev->base.root;

	new_req = sat_replicate_req(root, req, req->Data_Transfer_Length,
		scsi_ata_translation_callback);

	if (new_req == NULL)
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

	new_req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	new_req->Cdb[1] = CDB_CORE_MODULE;
	new_req->Cdb[2] = cmd;

	MV_ZeroMemory(&new_req->Cdb[3], sizeof(new_req->Cdb) - 3);
	((core_context *)new_req->Context[MODULE_CORE])->req_flag |=
		CORE_REQ_FLAG_NEED_D2H_FIS;

	core_append_request(root, new_req);
	return MV_QUEUE_COMMAND_RESULT_REPLACED;
}

void scsi_ata_ata_passthru_callback(MV_PVOID root_p,
        MV_Request *org_req, MV_Request *req)
{
	pl_root *root = root_p;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *dev = (domain_device *)get_device_by_id(root->lib_dev,
	req->Device_Id);
	saved_fis *fis;
	MV_U8 protocol, chk_cond;
	MV_U32 reg;
	MV_U32 length;

	req->Data_Buffer = NULL;
	req->Data_Transfer_Length = 0;
	ctx->buf_wrapper = NULL;
	SGTable_Init(&req->SG_Table, 0);

	fis = (saved_fis *)ctx->received_fis;
	if ((req->Scsi_Status != REQ_STATUS_SUCCESS) && (fis == NULL)) return;

	if (fis == NULL) MV_ASSERT(MV_FALSE);
	chk_cond = (org_req->Cdb[2] >> 5) & 0x01;

	switch (org_req->Cdb[0]) {
	case SCSI_CMD_ATA_PASSTHRU_12:
		reg = fis->dw1;
		org_req->Cdb[3] = (MV_U8)((reg >> 24) & 0xff);
		org_req->Cdb[9] = (MV_U8)((reg >> 16) & 0xff);

		reg = fis->dw2;
		org_req->Cdb[5] = (MV_U8)(reg & 0xff);
		org_req->Cdb[6] = (MV_U8)((reg >> 8) & 0xff);
		org_req->Cdb[7] = (MV_U8)((reg >> 16) & 0xff);
		org_req->Cdb[8] = (MV_U8)((reg >> 24) & 0xff);

		reg = fis->dw4;
		org_req->Cdb[4] = (MV_U8)(reg & 0xff);
		org_req->Cdb[11] = (MV_U8)((reg >> 24) & 0xff);

		break;
	case SCSI_CMD_ATA_PASSTHRU_16:
		reg = fis->dw1;
		org_req->Cdb[4] = (MV_U8)((reg >> 24) & 0xff);
		org_req->Cdb[14] = (MV_U8)((reg >> 16) & 0xff);

		reg = fis->dw2;
		org_req->Cdb[8] = (MV_U8)(reg & 0xff);
		org_req->Cdb[10] = (MV_U8)((reg >> 8) & 0xff);
		org_req->Cdb[12] = (MV_U8)((reg >> 16) & 0xff);
		org_req->Cdb[13] = (MV_U8)((reg >> 24) & 0xff);

		reg = fis->dw3;
		org_req->Cdb[7] = (MV_U8)(reg & 0xff);
		org_req->Cdb[9] = (MV_U8)((reg >> 8) & 0xff);
		org_req->Cdb[11] = (MV_U8)((reg >> 16) & 0xff);
		org_req->Cdb[3] = (MV_U8)((reg >> 24) & 0xff);

		reg = fis->dw4;
		org_req->Cdb[6] = (MV_U8)(reg & 0xff);
		org_req->Cdb[5] = (MV_U8)((reg >> 8) & 0xff);
		org_req->Cdb[15] = (MV_U8)((reg >> 24) & 0xff);

		break;
	default:
		MV_ASSERT(MV_FALSE);
		break;
	}

	if (chk_cond) {
		if (org_req->Sense_Info_Buffer) {
			((MV_PU8)org_req->Sense_Info_Buffer)[0] = 0x72;
			((MV_PU8)org_req->Sense_Info_Buffer)[7] = 0x0E;
			if (req->Scsi_Status == REQ_STATUS_SUCCESS) {
				((MV_PU8)org_req->Sense_Info_Buffer)[1] = SCSI_SK_RECOVERED_ERROR;
				((MV_PU8)org_req->Sense_Info_Buffer)[2] = SCSI_ASC_NO_ASC;
				((MV_PU8)org_req->Sense_Info_Buffer)[3] = SCSI_ASCQ_ATA_PASSTHRU_INFO;
				org_req->Scsi_Status = REQ_STATUS_HAS_SENSE;
			}		
		}
		
		/* See ATA PASSTHRU STATUS RETURN DESCRIPTOR on top */
		ata_return_descriptor[0] = 0x09;
		ata_return_descriptor[1] = 0x0C;
		if (org_req->Cdb[0] == SCSI_CMD_ATA_PASSTHRU_16)
			ata_return_descriptor[2] = 0x01;
		else
			ata_return_descriptor[2] = 0;

		reg = fis->dw1;

		ata_return_descriptor[3] = (MV_U8)((reg >> 24) & 0xff);
		ata_return_descriptor[13] = (MV_U8)((reg >> 16) & 0xff);

		reg = fis->dw2;

		ata_return_descriptor[7] = (MV_U8)(reg & 0xff);
		ata_return_descriptor[9] = (MV_U8)((reg >> 8) & 0xff);
		ata_return_descriptor[11] = (MV_U8)((reg >> 16) & 0xff);
		ata_return_descriptor[12] = (MV_U8)((reg >> 24) & 0xff);

		reg = fis->dw4;

		ata_return_descriptor[5] = (MV_U8)(reg & 0xff);
		if ((req->Scsi_Status == REQ_STATUS_SUCCESS)
			&& (req->Cmd_Flag & CMD_FLAG_PIO)
			&& (req->Cmd_Flag & CMD_FLAG_DATA_IN))
			ata_return_descriptor[13] = (MV_U8)((reg >> 24) & 0xff);

		if (req->Cmd_Flag & CMD_FLAG_48BIT) {
			ata_return_descriptor[4] = (MV_U8)((reg >> 8) & 0xff);
			reg = fis->dw3;
			ata_return_descriptor[6] = (MV_U8)(reg & 0xff);
			ata_return_descriptor[8] = (MV_U8)((reg >> 8) & 0xff);
			ata_return_descriptor[10] = (MV_U8)((reg >> 16) & 0xff);
		} else {
			ata_return_descriptor[4] = 0;
			ata_return_descriptor[6] = 0;
			ata_return_descriptor[8] = 0;
			ata_return_descriptor[10] = 0;
		}
		if (org_req->Sense_Info_Buffer_Length > 8)
			MV_CopyMemory(&((MV_PU8)org_req->Sense_Info_Buffer)[8],
				ata_return_descriptor,
				MV_MIN(org_req->Sense_Info_Buffer_Length - 8, 14));
	}
}

void scsi_ata_ata_passthru_12_fill_taskfile(MV_Request *req, MV_U8 tag,
	ata_taskfile *taskfile)
{
	taskfile->command = req->Cdb[9];
	taskfile->features = req->Cdb[3];
	taskfile->device = req->Cdb[8];

	taskfile->lba_low = req->Cdb[5];
	taskfile->lba_mid = req->Cdb[6];
	taskfile->lba_high = req->Cdb[7];
	taskfile->sector_count = req->Cdb[4];

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->feature_exp = 0;
	taskfile->control = req->Cdb[11];
	
	if (req->Cmd_Flag & CMD_FLAG_NCQ) {
	/*For Read/Write FPDMA command by ATA PASS Through command,
	  * replace the NCQ tag with ourselves to keep unique.*/
		taskfile->sector_count = tag << 3;
		taskfile->sector_count_exp = 0; 
	}
}
void scsi_ata_trim_request_fill_taskfile(MV_Request *req, ata_taskfile *taskfile)
{
	MV_ASSERT(!(req->Data_Transfer_Length%0x200));
	taskfile->command = 0x06;
	taskfile->features = 0x01;
	taskfile->device = (MV_U8)MV_BIT(6);
	taskfile->lba_low = 0;
	taskfile->lba_mid = 0;
	taskfile->lba_high = 0;
	taskfile->sector_count =(MV_U8) (req->Data_Transfer_Length/0x200);
	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;

	taskfile->feature_exp = 0;
	taskfile->control = 0;
}

void scsi_ata_ata_passthru_16_fill_taskfile(MV_Request *req, MV_U8 tag,
	ata_taskfile *taskfile)
{
	taskfile->command = req->Cdb[14];
	taskfile->features = req->Cdb[4];
	taskfile->device = req->Cdb[13];

	taskfile->lba_low = req->Cdb[8];
	taskfile->lba_mid = req->Cdb[10];
	taskfile->lba_high = req->Cdb[12];
	taskfile->sector_count = req->Cdb[6];
	taskfile->control = req->Cdb[15];

	if (req->Cdb[1] & 0x01) {
		taskfile->lba_low_exp = req->Cdb[7];
		taskfile->lba_mid_exp = req->Cdb[9];
		taskfile->lba_high_exp = req->Cdb[11];
		taskfile->sector_count_exp = req->Cdb[5];
		taskfile->feature_exp = req->Cdb[3];
	} else {
		taskfile->lba_low_exp = 0;
		taskfile->lba_mid_exp = 0;
		taskfile->lba_high_exp = 0;
		taskfile->sector_count_exp = 0;
		taskfile->feature_exp = 0;
	}
	if (req->Cmd_Flag & CMD_FLAG_NCQ) {
	/*For Read/Write FPDMA command by ATA PASS Through command,
	  * replace the NCQ tag with ourselves to keep unique.*/
		taskfile->sector_count = tag << 3;
		taskfile->sector_count_exp = 0; 
	}

}

/*
   sat_replicate_req

   Creates a new copy of the request for use in translation. Original request
   is perserved through the orq_req pointer in the context.
*/
MV_Request *sat_replicate_req_for_ata_passthru(pl_root *root,
			MV_Request *req,
			MV_U32 new_buff_size,
			MV_ReqCompletion completion)
{
	domain_device   *device;
	core_context    *curr_ctx;
	core_context    *new_ctx;
	MV_Request      *new_req;
	MV_U32          buff_size;
	MV_PVOID        org_buf_ptr, new_buf_ptr;

	curr_ctx = (core_context *)req->Context[MODULE_CORE];
	device = (domain_device *)get_device_by_id(root->lib_dev,
	req->Device_Id);

	if (curr_ctx->type == CORE_CONTEXT_TYPE_ORG_REQ) {
		MV_ASSERT(MV_FALSE);
		return req;
	}

	new_req = get_intl_req_resource(root, new_buff_size);
	if (new_req == NULL) {
		return NULL;
	}

	sat_copy_request(new_req, req);

	MV_LIST_HEAD_INIT(&new_req->Queue_Pointer);

	new_req->Completion = completion;
	new_ctx = (core_context *)new_req->Context[MODULE_CORE];
	new_ctx->type = CORE_CONTEXT_TYPE_ORG_REQ;
	new_ctx->u.org.org_req = req;

	return new_req;
}

MV_U8 scsi_ata_ata_passthru_translation(domain_device *dev,
        MV_Request *req)
{
	MV_Request *new_req;
	core_context *new_ctx;
	pl_root *root = dev->base.root;
	domain_port *port = dev->base.port;
	MV_U8 protocol, t_length, t_dir, byte, multi_rw, command;
	MV_U32 length, tx_length = 0, cmd_flag = req->Cmd_Flag;
	MV_PU8 buf_ptr;
        MV_BOOLEAN data_out = MV_FALSE, data_in = MV_FALSE;

	protocol = (req->Cdb[1] >> 1) & 0x0F;

	switch (protocol) {
	case ATA_PROTOCOL_HARD_RESET:
		new_req = core_make_port_reset_req(root, port, dev,
		        (MV_ReqCompletion)scsi_ata_translation_callback);

		if (new_req == NULL) return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

		new_ctx = (core_context *)new_req->Context[MODULE_CORE];
		new_ctx->type = CORE_CONTEXT_TYPE_ORG_REQ;
		new_ctx->u.org.org_req = req;

		core_append_request(root, new_req);
		return MV_QUEUE_COMMAND_RESULT_REPLACED;

	case ATA_PROTOCOL_SRST:
		return (scsi_ata_api_request_translation(dev, req,
			CDB_CORE_SOFT_RESET_1));
	}

	multi_rw = (req->Cdb[1] >> 5) & 0x7;
	t_length = req->Cdb[2] & 0x3;
	byte = (req->Cdb[2] >> 2) & 0x01;
	t_dir = (req->Cdb[2] >> 3) & 0x1; /* 0: data out, 1: data in*/

	if (req->Cdb[0] == SCSI_CMD_ATA_PASSTHRU_12) {
		command = req->Cdb[9];
		if (command == ATA_CMD_IDENTIFY_ATA && req->Cdb[4] == 0)
			req->Cdb[4] = 1;
	} else {
		command = req->Cdb[14];
		if (req->Cdb[1] & 0x01) 
			cmd_flag |= CMD_FLAG_48BIT;
		else
			cmd_flag &= ~CMD_FLAG_48BIT;
		if (command == ATA_CMD_IDENTIFY_ATA && req->Cdb[6] == 0)
			req->Cdb[6] = 1;
	}

	if (multi_rw != 0 && !IS_ATA_MULTIPLE_READ_WRITE(command))
		goto ata_passthru_translation_error;

	if (t_length != 0) {
		if (t_dir == 0)
			data_out = MV_TRUE;
		else
			data_in = MV_TRUE;
	}

	if (t_length == 0x0) {
		tx_length = 0x0;
	} 
	/* Transfer length defined in Features */
	else if (t_length == 0x1) {
        	if (req->Cdb[0] == SCSI_CMD_ATA_PASSTHRU_12) {
			tx_length = req->Cdb[3];
        	}
       		else {
			/* Extend bit */
            		if (req->Cdb[1] & 1) {
				tx_length = (req->Cdb[3] << 8) | req->Cdb[4];
			}
			else {
				tx_length = req->Cdb[4];
            		}
       		}
	}
	/* Transfer length defined in Sector Count */
	else if (t_length == 0x2) {
        	if (req->Cdb[0] == SCSI_CMD_ATA_PASSTHRU_12) {
			tx_length = req->Cdb[4];
       		}
        	else {
			/* Extend bit */
            		if (req->Cdb[1] & 1) {
				tx_length = (req->Cdb[5] << 8) | req->Cdb[6];
			}
			else {
				tx_length = req->Cdb[6];
            		}
        	}
	}
	/* t_length == 0x3 means use the length defined in the
	  * nexus transaction */
	else {
		tx_length = req->Data_Transfer_Length;
	}
	if (req->Cdb[14] == ATA_CMD_DOWNLOAD_MICROCODE) {
		/* download micro code transfer sector count in sector and lba_low.*/
		tx_length = (req->Cdb[8] << 8) | req->Cdb[6];
	}
	if (byte) {
		tx_length *= dev->sector_size;
	}

	switch (protocol) {
	case ATA_PROTOCOL_NON_DATA:
		if (t_length != 0) goto ata_passthru_translation_error;
		break;

	case ATA_PROTOCOL_PIO_IN:
		if (data_in == MV_FALSE) goto ata_passthru_translation_error;
		cmd_flag |= (CMD_FLAG_PIO | CMD_FLAG_DATA_IN);
		break;

	case ATA_PROTOCOL_PIO_OUT:
		if (data_out == MV_FALSE) goto ata_passthru_translation_error;
		cmd_flag |= (CMD_FLAG_PIO | CMD_FLAG_DATA_OUT);
		break;

	case ATA_PROTOCOL_DMA:
		cmd_flag |= CMD_FLAG_DMA;
		break;
	case ATA_PROTOCOL_DMA_QUEUED:
		cmd_flag |= (CMD_FLAG_DMA | CMD_FLAG_TCQ);
		break;

	case ATA_PROTOCOL_DEVICE_DIAG:
	case ATA_PROTOCOL_DEVICE_RESET:
		break;

	case ATA_PROTOCOL_UDMA_IN:
		if (data_in == MV_FALSE) goto ata_passthru_translation_error;
		cmd_flag |= CMD_FLAG_DMA;
		break;

	case ATA_PROTOCOL_UDMA_OUT:
                if (data_out == MV_FALSE) goto ata_passthru_translation_error;
		cmd_flag |= CMD_FLAG_DMA;
		break;

	case ATA_PROTOCOL_FPDMA:
		cmd_flag |= (CMD_FLAG_DMA | CMD_FLAG_NCQ);
		break;

	case ATA_PROTOCOL_RTN_INFO:
		if ((req->Sense_Info_Buffer)
			&& (req->Sense_Info_Buffer_Length > 8)) {
			
			((MV_PU8)req->Sense_Info_Buffer)[0] = 0x72;
			((MV_PU8)req->Sense_Info_Buffer)[7] = 0x0E;
			
			MV_CopyMemory(&((MV_PU8)req->Sense_Info_Buffer)[8],
				ata_return_descriptor,
				MV_MIN(req->Sense_Info_Buffer_Length - 8, 14));
		}
		req->Scsi_Status = REQ_STATUS_HAS_SENSE;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;

	default:
		CORE_DPRINT(("Invalid ATA PASSTHRU Protocol %d\n", protocol));
		goto ata_passthru_translation_error;
	}

	new_req = sat_replicate_req_for_ata_passthru(root, req, 0,
		scsi_ata_translation_callback);

	if (new_req == NULL) return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;

	new_req->Cmd_Flag = cmd_flag;
	((core_context *)new_req->Context[MODULE_CORE])->buf_wrapper =
		((core_context *)req->Context[MODULE_CORE])->buf_wrapper;
	((core_context *)new_req->Context[MODULE_CORE])->req_flag |=
		CORE_REQ_FLAG_NEED_D2H_FIS;

	switch (req->Cdb[0] == SCSI_CMD_ATA_PASSTHRU_16 ? req->Cdb[14] : req->Cdb[9]) {
	case ATA_CMD_SEC_PASSWORD:
	case ATA_CMD_SEC_UNLOCK:
	case ATA_CMD_SEC_ERASE_UNIT:
	case ATA_CMD_SEC_DISABLE_PASSWORD:
		tx_length = req->Data_Transfer_Length;
		break;
	default:
		break;
	}
	if (tx_length > 0) {
		new_req->Data_Buffer = req->Data_Buffer;
		MV_DASSERT(tx_length <= req->SG_Table.Byte_Count);
		new_req->Data_Transfer_Length = tx_length;
		MV_CopyPartialSGTable(
			&new_req->SG_Table,
			&req->SG_Table,
			0, /* offset */
			tx_length /* size */
			);
	}

	core_append_request(root, new_req);
	return MV_QUEUE_COMMAND_RESULT_REPLACED;

ata_passthru_translation_error:
	scsi_ata_check_condition(req, SCSI_SK_ILLEGAL_REQUEST,
		SCSI_ASC_INVALID_FEILD_IN_CDB, 0);
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

void scsi_ata_atapi_packet_fill_taskfile(domain_device *device,
	MV_Request *req, ata_taskfile *taskfile)
{
	taskfile->features = 0;
	taskfile->feature_exp = 0;

	if (!bad_atapi_device_model(device)) {
		if (req->Cmd_Flag & CMD_FLAG_DMA)
		taskfile->features |= MV_BIT(0);
	} else {
		if (SCSI_IS_READ(req->Cdb[0]))
		taskfile->features |= MV_BIT(0);
	}
	/* Byte count low and byte count high */
	if (req->Data_Transfer_Length > 0xFFFF) {
		taskfile->lba_mid = 0xFF;
		taskfile->lba_high = 0xFF;
	} else {
		taskfile->lba_mid =
		(MV_U8)req->Data_Transfer_Length;
		taskfile->lba_high =
		(MV_U8)(req->Data_Transfer_Length >> 8);
	}

	taskfile->command = ATA_CMD_PACKET;
	taskfile->control = 0;
	taskfile->device = MV_BIT(6);

	taskfile->lba_low = 0;
	taskfile->sector_count = 0;

	taskfile->lba_low_exp = 0;
	taskfile->lba_mid_exp = 0;
	taskfile->lba_high_exp = 0;
	taskfile->sector_count_exp = 0;
}

MV_U8 scsi_ata_translation(pl_root *root, MV_Request *req)
{
	core_extension *core = (core_extension *)root->core;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	domain_device *device = (domain_device *)get_device_by_id(
	root->lib_dev, req->Device_Id);
	MV_U8 ret = MV_QUEUE_COMMAND_RESULT_PASSED;

	if (IS_ATAPI(device))
		return ret;

	if (ctx->type == CORE_CONTEXT_TYPE_ORG_REQ)
		return ret;

	switch (req->Cdb[0]) {
	case SCSI_CMD_INQUIRY:
		ret = scsi_ata_inquiry_translation(device, req);
		break;
	case SCSI_CMD_READ_CAPACITY_10:
	case SCSI_CMD_READ_CAPACITY_16:
		ret = scsi_ata_read_capacity_translation(device, req);
		break;
	case SCSI_CMD_START_STOP_UNIT:
		ret = scsi_ata_start_stop_translation(device, req);
		break;
	case SCSI_CMD_FORMAT_UNIT:
		ret = scsi_ata_format_unit_translation(device, req);
		break;
	case SCSI_CMD_REQUEST_SENSE:
		ret = scsi_ata_request_sense_translation(device, req);
		break;
	case SCSI_CMD_LOG_SENSE:
		ret = scsi_ata_log_sense_translation(device, req);
		break;
	case SCSI_CMD_SND_DIAG:
		ret = scsi_ata_send_diag_translation(device, req);
		break;
	case SCSI_CMD_TEST_UNIT_READY:
		ret = scsi_ata_test_unit_ready_translation(device, req);
		break;
	case SCSI_CMD_MODE_SELECT_6:
	case SCSI_CMD_MODE_SELECT_10:
		ret = scsi_ata_mode_select_translation(device, req);
		break;
	case SCSI_CMD_SYNCHRONIZE_CACHE_10:
		ret = scsi_ata_sync_cache_translation(device, req);
		break;
	case SCSI_CMD_REASSIGN_BLOCKS:
		ret = scsi_ata_reassign_blocks_translation(device, req);
		break;
	case SCSI_CMD_ATA_PASSTHRU_12:
	case SCSI_CMD_ATA_PASSTHRU_16:
		ret = scsi_ata_ata_passthru_translation(device, req);
		break;
	case SCSI_CMD_MARVELL_SPECIFIC:
		if (req->Cdb[1] != CDB_CORE_MODULE)
			MV_DASSERT(MV_FALSE);
		switch (req->Cdb[2]) {
			case CDB_CORE_ENABLE_SMART:
			case CDB_CORE_DISABLE_SMART:
			ret = scsi_ata_enable_smart_translation(device, req);
			break;
			case CDB_CORE_OS_SMART_CMD:
			ret = scsi_ata_os_smart_cmd_translation(device, req);
			break;
			case CDB_CORE_SMART_RETURN_STATUS:
			ret = scsi_ata_smart_return_status_translation(device, req);
			break;
		}
		break;
		case APICDB0_PD:
			switch (req->Cdb[1]) {
			case APICDB1_PD_SETSETTING:
			switch (req->Cdb[4]) {
				case APICDB4_PD_SET_WRITE_CACHE_OFF:
				ret = scsi_ata_api_request_translation(device,
				req, CDB_CORE_DISABLE_WRITE_CACHE);
				break;

			case APICDB4_PD_SET_WRITE_CACHE_ON:
				ret = scsi_ata_api_request_translation(device,
				req, CDB_CORE_ENABLE_WRITE_CACHE);
				break;

			case APICDB4_PD_SET_SMART_OFF:
				ret = scsi_ata_api_request_translation(device,
				req, CDB_CORE_DISABLE_SMART);
				break;

			case APICDB4_PD_SET_SMART_ON:
				ret = scsi_ata_api_request_translation(device,
				req, CDB_CORE_ENABLE_SMART);
				break;

			default:
				CORE_DPRINT(("Unsupported API PD Setting %d\n",\
				req->Cdb[4]));

				if (req->Sense_Info_Buffer != NULL &&
					req->Sense_Info_Buffer_Length != 0)

				((MV_PU8)req->Sense_Info_Buffer)[0]
				= ERR_INVALID_PARAMETER;

				req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
				ret = MV_QUEUE_COMMAND_RESULT_FINISHED;
				break;
			}
			break;
		case APICDB1_PD_GETSTATUS:
			switch (req->Cdb[4]) {
			case APICDB4_PD_SMART_RETURN_STATUS:
				ret = scsi_ata_api_request_translation(device,
				req, CDB_CORE_SMART_RETURN_STATUS);
				break;
			default:
				CORE_DPRINT(("Unsupported API PD "\
				"Get Status %d\n", req->Cdb[4]));

				if (req->Sense_Info_Buffer != NULL &&
					req->Sense_Info_Buffer_Length != 0)

				((MV_PU8)req->Sense_Info_Buffer)[0]
					= ERR_INVALID_PARAMETER;

				req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
				ret = MV_QUEUE_COMMAND_RESULT_FINISHED;
				break;
			}
		}
		break;

	default:
		break;
	}

	return ret;
}

MV_BOOLEAN ata_fill_taskfile(domain_device *device, MV_Request *req,
	MV_U8 tag, ata_taskfile *taskfile)
{
	switch (req->Cdb[0]) {
	case SCSI_CMD_FORMAT_UNIT:
		scsi_ata_format_unit_fill_taskfile(device, req, taskfile);
		break;
	case SCSI_CMD_INQUIRY:
		scsi_ata_inquiry_fill_taskfile(req, taskfile);
		break;
	case SCSI_CMD_READ_6:
	case SCSI_CMD_WRITE_6:
	case SCSI_CMD_READ_10:
	case SCSI_CMD_WRITE_10:
	case SCSI_CMD_READ_16:
	case SCSI_CMD_WRITE_16:
		scsi_ata_read_write_fill_taskfile(req, tag, taskfile);
		break;
	case SCSI_CMD_READ_CAPACITY_10:
	case SCSI_CMD_READ_CAPACITY_16:
		scsi_ata_read_capacity_fill_taskfile(req, taskfile);
		break;
	case SCSI_CMD_VERIFY_10:
	case SCSI_CMD_VERIFY_16:
		scsi_ata_verify_fill_taskfile( req, taskfile );
		break;
	case SCSI_CMD_SYNCHRONIZE_CACHE_10:
	case SCSI_CMD_SYNCHRONIZE_CACHE_16:
		scsi_ata_sync_cache_fill_taskfile(device, req, taskfile);
		break;
	case SCSI_CMD_START_STOP_UNIT:
		scsi_ata_start_stop_fill_taskfile(device, req, taskfile);
		break;
	case SCSI_CMD_TEST_UNIT_READY:
		scsi_ata_test_unit_ready_fill_taskfile(req, taskfile);
		break;
	case SCSI_CMD_REQUEST_SENSE:
		scsi_ata_request_sense_fill_taskfile(req, taskfile);
		break;
	case SCSI_CMD_LOG_SENSE:
		scsi_ata_log_sense_fill_taskfile(req, taskfile);
		break;
	case SCSI_CMD_SND_DIAG:
		scsi_ata_send_diag_fill_taskfile(device, req, taskfile);
		break;
	case SCSI_CMD_MODE_SELECT_6:
	case SCSI_CMD_MODE_SELECT_10:
		scsi_ata_mode_select_fill_taskfile(req, taskfile);
		break;
	case SCSI_CMD_REASSIGN_BLOCKS:
		scsi_ata_reassign_blocks_fill_taskfile(req, taskfile);
		break;
	case SCSI_CMD_MODE_SENSE_6:
	case SCSI_CMD_MODE_SENSE_10:
		break;
	case SCSI_CMD_ATA_PASSTHRU_12:
		scsi_ata_ata_passthru_12_fill_taskfile(req, tag, taskfile);
		break;
	case SCSI_CMD_ATA_PASSTHRU_16:
		scsi_ata_ata_passthru_16_fill_taskfile(req, tag, taskfile);
		break;
	case SCSI_CMD_MARVELL_SPECIFIC:
		if ( req->Cdb[1]!=CDB_CORE_MODULE )
			return MV_FALSE;
		switch (req->Cdb[2]) {
		case CDB_CORE_SOFT_RESET_0:
		case CDB_CORE_SOFT_RESET_1:
			scsi_ata_soft_reset_fill_taskfile(req, taskfile);
			break;
		case CDB_CORE_IDENTIFY:
			scsi_ata_core_identify_fill_taskfile(req, taskfile);
			break;

		case CDB_CORE_SET_UDMA_MODE:
			scsi_ata_set_udma_mode_fill_taskfile(device, req, taskfile);
			break;

		case CDB_CORE_SET_PIO_MODE:
			scsi_ata_set_pio_mode_fill_taskfile(req, taskfile);
			break;

		case CDB_CORE_ENABLE_WRITE_CACHE:
		case CDB_CORE_DISABLE_WRITE_CACHE:
			scsi_ata_write_cache_fill_taskfile(req, taskfile);
			break;
		case CDB_CORE_SET_FEATURE_SPINUP:
			scsi_ata_pois_to_spinup_fill_taskfile(req, taskfile);
			break;
		case CDB_CORE_ENABLE_SMART:
		case CDB_CORE_DISABLE_SMART:
			scsi_ata_enable_smart_fill_taskfile(req, taskfile);
			break;

		case CDB_CORE_SMART_RETURN_STATUS:
			scsi_ata_smart_return_status_fill_taskfile(req, taskfile);
			break;

		case CDB_CORE_SHUTDOWN:
			scsi_ata_shudown_fill_taskfile(device, req, taskfile);
			break;
		case CDB_CORE_ENABLE_READ_AHEAD:
		case CDB_CORE_DISABLE_READ_AHEAD:
			scsi_ata_enable_read_ahead_fill_taskfile(req, taskfile);
			break;

		case CDB_CORE_READ_LOG_EXT:
			scsi_ata_read_log_fill_taskfile(req, taskfile);
			break;

		case CDB_CORE_PM_READ_REG:
			scsi_ata_pm_read_reg_fill_taskfile(req, taskfile);
			break;

		case CDB_CORE_OS_SMART_CMD:
			scsi_ata_os_smart_cmd_fill_taskfile(req, taskfile);
			break;

		case CDB_CORE_PM_WRITE_REG:
			scsi_ata_pm_write_reg_fill_taskfile(req, taskfile);
			break;

		case CDB_CORE_ATA_IDLE:
			scsi_ata_fill_taskfile_cmd(req, taskfile,
			        ATA_CMD_IDLE);
			break;

		case CDB_CORE_ATA_STANDBY:
			scsi_ata_fill_taskfile_cmd(req, taskfile,
			        ATA_CMD_STANDBY);
			break;

		case CDB_CORE_ATA_IDLE_IMMEDIATE:
			scsi_ata_fill_taskfile_cmd(req, taskfile,
			        ATA_CMD_IDLE_IMMEDIATE);
			break;

		case CDB_CORE_ATA_STANDBY_IMMEDIATE:
			scsi_ata_fill_taskfile_cmd(req, taskfile,
			        ATA_CMD_STANDBY_IMMEDIATE);
			break;

		default:
			return MV_FALSE;
		}
		break;
		CORE_DPRINT(("Error: Unknown request: 0x%x.\n", req->Cdb[0]));

	default:
		return MV_FALSE;
	}
	return MV_TRUE;
}

#define MAX_NUM_BAD_ATAPI_DEVICE	8
#define BAD_ATAPI_DEVICE_MODEL_0	"PIONEER DVR-213N"

typedef struct bad_atapi_device_model
{
	MV_U8 model_number[40];
} atapi_device_model;

MV_BOOLEAN bad_atapi_device_model(domain_device *device)
{
	atapi_device_model bad_mode[8];
	MV_U8 i;
	MV_FillMemory(bad_mode[0].model_number, 40 * sizeof(MV_U8), ' ');
	MV_CopyMemory(bad_mode[0].model_number, BAD_ATAPI_DEVICE_MODEL_0,
		sizeof(BAD_ATAPI_DEVICE_MODEL_0));

	for (i=0;i <MAX_NUM_BAD_ATAPI_DEVICE; i++){
		if(MV_CompareMemory(device->model_number, bad_mode[i].model_number,
			40*sizeof(MV_U8)))
			continue;
		else
			break;
	}

	if(i >= MAX_NUM_BAD_ATAPI_DEVICE)
		return MV_FALSE;
	else
		return MV_TRUE;

}

MV_BOOLEAN atapi_fill_taskfile(domain_device *device, MV_Request *req,
	ata_taskfile *taskfile)
{
	switch (req->Cdb[0]) {
	case SCSI_CMD_MARVELL_SPECIFIC:
		if ( req->Cdb[1]!=CDB_CORE_MODULE )
			return MV_FALSE;

	switch (req->Cdb[2]) {
		case CDB_CORE_SOFT_RESET_0:
		case CDB_CORE_SOFT_RESET_1:
		scsi_ata_soft_reset_fill_taskfile(req, taskfile);
		break;

	case CDB_CORE_IDENTIFY:
		scsi_ata_core_identify_fill_taskfile(req, taskfile);
		break;

	case CDB_CORE_SET_UDMA_MODE:
		scsi_ata_set_udma_mode_fill_taskfile(device, req, taskfile);
		break;

	case CDB_CORE_SET_PIO_MODE:
		scsi_ata_set_pio_mode_fill_taskfile(req, taskfile);
		break;

	case CDB_CORE_PM_READ_REG:
		scsi_ata_pm_read_reg_fill_taskfile(req, taskfile);
		break;

	case CDB_CORE_PM_WRITE_REG:
		scsi_ata_pm_write_reg_fill_taskfile(req, taskfile);
		break;
	case CDB_CORE_OS_SMART_CMD:
	default:
		return MV_FALSE;
	}
	break;

	case SCSI_CMD_INQUIRY:
		if(req->Req_Type == REQ_TYPE_CORE)
			scsi_atapi_core_identify_fill_taskfile(req, taskfile);
		else
			scsi_ata_atapi_packet_fill_taskfile(device, req, taskfile);
		break;

	case SCSI_CMD_READ_6:
	case SCSI_CMD_WRITE_6:
	case SCSI_CMD_READ_10:
	case SCSI_CMD_WRITE_10:
	case SCSI_CMD_VERIFY_10:
	case SCSI_CMD_READ_CAPACITY_10:
	case SCSI_CMD_TEST_UNIT_READY:
	case SCSI_CMD_MODE_SENSE_10:
	case SCSI_CMD_MODE_SELECT_10:
	case SCSI_CMD_PREVENT_MEDIUM_REMOVAL:
	case SCSI_CMD_READ_TOC:
	case SCSI_CMD_BLANK:
	case SCSI_CMD_READ_DISC_INFO:
	case SCSI_CMD_START_STOP_UNIT:
	case SCSI_CMD_SYNCHRONIZE_CACHE_10:
	case SCSI_CMD_REQUEST_SENSE:
	default:
		scsi_ata_atapi_packet_fill_taskfile(device, req, taskfile);
		break;
	}

	return MV_TRUE;
}

MV_VOID scsi_ata_translation_callback(MV_PVOID root_p, MV_Request *req)
{
	pl_root         *root = (pl_root *)root_p;
	core_extension  *core = (core_extension *)root->core;
	MV_U8           finished = MV_TRUE;
	MV_Request      *org_req = sat_clear_org_req(req);
	MV_U32          length;

	MV_ASSERT(org_req != NULL);

	switch (org_req->Cdb[0]) {
		case SCSI_CMD_FORMAT_UNIT:
		finished = scsi_ata_format_unit_callback(root,
		                org_req, req);
		break;
	case SCSI_CMD_INQUIRY:
		scsi_ata_inquiry_callback(root, org_req, req);
		break;
	case SCSI_CMD_READ_CAPACITY_10:
	case SCSI_CMD_READ_CAPACITY_16:
		scsi_ata_read_capacity_callback(root, org_req, req);
		break;
	case SCSI_CMD_SND_DIAG:
		finished = scsi_ata_send_diag_callback(root, org_req, req);
		break;
	case SCSI_CMD_START_STOP_UNIT:
		finished = scsi_ata_start_stop_callback(root, org_req, req);
		break;
	case SCSI_CMD_TEST_UNIT_READY:
		scsi_ata_test_unit_ready_callback(root, org_req, req);
		break;
	case SCSI_CMD_REQUEST_SENSE:
		scsi_ata_request_sense_callback(root, org_req, req);
		break;
	case SCSI_CMD_LOG_SENSE:
		scsi_ata_log_sense_callback(root, org_req, req);
		break;
	case SCSI_CMD_MODE_SELECT_6:
	case SCSI_CMD_MODE_SELECT_10:
		scsi_ata_mode_select_callback(root, org_req, req);
		break;
	case SCSI_CMD_REASSIGN_BLOCKS:
		finished = scsi_ata_reassign_blocks_callback(root, org_req, req);
		break;
	case SCSI_CMD_ATA_PASSTHRU_12:
	case SCSI_CMD_ATA_PASSTHRU_16:
		scsi_ata_ata_passthru_callback(root, org_req, req);
		break;
	case SCSI_CMD_SYNCHRONIZE_CACHE_10:
		break;
	case APICDB0_PD:
		scsi_ata_api_request_callback(root, org_req, req);
		break;
	case SCSI_CMD_MARVELL_SPECIFIC:
		if (org_req->Cdb[1] != CDB_CORE_MODULE)
			MV_DASSERT(MV_FALSE);

		switch (org_req->Cdb[2]) {
		case CDB_CORE_ENABLE_SMART:
		case CDB_CORE_DISABLE_SMART:
			break;
		case CDB_CORE_OS_SMART_CMD:
			scsi_ata_os_smart_cmd_callback(root, org_req, req);
			break;
		case CDB_CORE_SMART_RETURN_STATUS:
			scsi_ata_smart_return_status_callback(root, org_req, req);
			break;
		default:
			MV_ASSERT(MV_FALSE);
			break;
		}
		break;
	default:
		MV_ASSERT(MV_FALSE);
		break;
	}

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) 
		core_generate_error_event(core, req);
	
	if (finished) {
		if (org_req->Scsi_Status == REQ_STATUS_PENDING)
			org_req->Scsi_Status = req->Scsi_Status;
		core_queue_completed_req(core, org_req);
	}
}

/*
 * Translate SCSI command to SATA FIS
 * The following SCSI command set is the minimum required.
 *		Standard Inquiry
 *		Read Capacity
 *		Test Unit Ready
 *		Start/Stop Unit
 *		Read 10
 *		Write 10
 *		Request Sense
 *		Mode Sense/Select
 */
MV_VOID scsi_to_sata_fis(MV_Request *req, MV_PVOID fis_pool,
	ata_taskfile *taskfile, MV_U8 pm_port)
{
	sata_fis_reg_h2d *fis = (sata_fis_reg_h2d *)fis_pool;

	fis->fis_type = SATA_FIS_TYPE_REG_H2D;
	fis->cmd_pm = pm_port & 0x0f;
	if (!(req->Cmd_Flag & CMD_FLAG_SOFT_RESET))
		fis->cmd_pm |= H2D_COMMAND_SET_FLAG;
	fis->command = taskfile->command;
	fis->features = taskfile->features;
	fis->device = taskfile->device;
	fis->control = taskfile->control;

	fis->lba_low = taskfile->lba_low;
	fis->lba_mid = taskfile->lba_mid;
	fis->lba_high = taskfile->lba_high;
	fis->sector_count = taskfile->sector_count;
	fis->lba_low_exp = taskfile->lba_low_exp;
	fis->lba_mid_exp = taskfile->lba_mid_exp;
	fis->lba_high_exp = taskfile->lba_high_exp;
	fis->features_exp = taskfile->feature_exp;
	fis->sector_count_exp = taskfile->sector_count_exp;

	fis->ICC = 0x0;
	*((MV_PU32)&fis->reserved2[0]) = 0x0;
}
