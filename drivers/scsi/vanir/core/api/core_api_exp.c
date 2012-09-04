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
#include "com_struct.h"
#include "com_api.h"
#include "com_error.h"
#include "core_console.h"
#include "core_util.h"
#include "core_sas.h"
#include "core_expander.h"

MV_U8 core_get_exp_information(core_extension * core, domain_expander *exp_p,
        PExp_Info exp_info, PMV_Request req);

MV_VOID core_fill_exp_information(core_extension * core, domain_expander *exp,
        PExp_Info exp_info);

MV_PVOID core_generate_exp_hardware_req(pl_root *root, domain_expander * exp,
        PExp_Info exp_info, PMV_Request req, MV_U8 request_type, MV_U8 phy_index);

MV_VOID core_get_exp_information_callback(MV_PVOID root_p, MV_Request *req);

/* return MV_QUEUE_COMMAND_RESULT_XXX */
MV_U8 core_pd_request_get_expander_info(core_extension * core, PMV_Request req)
{
	PExp_Info_Request exp_req;
       PRequestHeader header = NULL;
       PExp_Info exp_info = NULL;

	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	MV_U16 start_id = 0, end_id = 0;
       MV_U8 j;
       List_Head smp_list;
       domain_base *base = NULL;
       domain_expander *exp, *parent = NULL;
       MV_Request *new_req = NULL;
	
	exp_req = (PExp_Info_Request)core_map_data_buffer(req);
	header = &exp_req->header;
       req->Scsi_Status = REQ_STATUS_SUCCESS;

	if (exp_req == NULL ||
		req->Data_Transfer_Length < sizeof(Exp_Info_Request)) {
		
		core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

        if (req->Cmd_Initiator != NULL) {
                if (header->startingIndexOrId >= MAX_ID) {
        		if (req->Sense_Info_Buffer != NULL)
			        ((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_EXP_ID;
                        req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
                        core_unmap_data_buffer(req);
		        return MV_QUEUE_COMMAND_RESULT_FINISHED;
	        }

	        if (header->requestType == REQUEST_BY_RANGE) {
		        start_id = header->startingIndexOrId;
		        end_id = MAX_ID;
	        } else if (header->requestType == REQUEST_BY_ID) {
		        start_id = header->startingIndexOrId;
		        end_id = header->startingIndexOrId + 1;
	        }

	        header->numReturned = 0;
	        header->nextStartingIndex = NO_MORE_DATA;

                ctx->u.api_req.pointer = req->Cmd_Initiator;
                ctx->u.api_req.remaining = 0;
                ctx->u.api_req.start = start_id;
                ctx->u.api_req.end = end_id;
                req->Cmd_Initiator = NULL;
        } else {
                MV_ASSERT(ctx->u.api_req.remaining == 0);
                MV_ASSERT(ctx->u.api_req.pointer != NULL);
                CORE_DPRINT(("expander api, start 0x%x, end 0x%x.\n", \
                        ctx->u.api_req.start, ctx->u.api_req.end));
        }

        MV_LIST_HEAD_INIT(&smp_list);
        while (ctx->u.api_req.start < ctx->u.api_req.end) {
                MV_ASSERT(List_Empty(&smp_list));

                base = get_device_by_id(&core->lib_dev, ctx->u.api_req.start);
                if ((base == NULL) || (base->type != BASE_TYPE_DOMAIN_EXPANDER)) {
                        ctx->u.api_req.start++;
                        continue;
                }

                exp = (domain_expander *)base;
                /* during initialization */
                if (exp->state != EXP_STATE_DONE) {
                        ctx->u.api_req.start++;
                        continue;
	        }
                if ((exp->base.parent != NULL) &&
                        (exp->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER)) {
                        parent = (domain_expander *)exp->base.parent;
                        /* need do discover to the parent expander */
                        if (parent->state != EXP_STATE_DONE) {
                                ctx->u.api_req.start++;
                                continue;
                        }
                }

                exp_info = &exp_req->expInfo[header->numReturned];
                /* for each expander, fill the already known information */
		 core_fill_exp_information(core, exp, exp_info);

                if ((exp->base.parent != NULL) &&
                        (exp->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER)) {
                        for (j = 0; j < exp_info->Link.Self.PhyCnt; j++) {
		                new_req = core_generate_exp_hardware_req(
                                        exp->base.root, exp, exp_info, req, DISCOVER, j);
                                if (new_req) {
                                        List_Add(&new_req->Queue_Pointer , &smp_list);
                                } else {
                                        goto no_resource;
                                }
                        }
                }
                
                new_req = core_generate_exp_hardware_req(
                        exp->base.root, exp, exp_info, req, REPORT_GENERAL, 0);
                
                if (new_req) {
                        List_Add(&new_req->Queue_Pointer, &smp_list);
                } else {
                        goto no_resource;
                }

                while (!List_Empty(&smp_list)) {
                        new_req = List_GetFirstEntry(&smp_list, MV_Request, Queue_Pointer);
                        core_append_request(exp->base.root, new_req);
                        ctx->u.api_req.remaining++;
                }

                ctx->u.api_req.start++;
		header->numReturned++;
		if ((header->requestType == REQUEST_BY_RANGE) &&
			(header->numReturned == header->numRequested)) {
			header->nextStartingIndex = ctx->u.api_req.start;
			break;
		}
	}

        core_unmap_data_buffer(req);

        req->Cmd_Initiator = ctx->u.api_req.pointer;
        if (ctx->u.api_req.remaining == 0) {
                MV_ASSERT(header->numReturned == 0);
                return MV_QUEUE_COMMAND_RESULT_FINISHED;
        } else {
                return MV_QUEUE_COMMAND_RESULT_REPLACED;
        }

no_resource:
        while (!List_Empty(&smp_list)) {
                new_req = List_GetFirstEntry(&smp_list, MV_Request, Queue_Pointer);
                intl_req_release_resource(&core->lib_rsrc, req);
        }
        if (ctx->u.api_req.remaining != 0) {
                return MV_QUEUE_COMMAND_RESULT_REPLACED;
        } else {
                return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
        }
}

MV_VOID core_fill_exp_information(core_extension *core, domain_expander *exp,
        PExp_Info exp_info)
{
	MV_U8 i, phy_id;

        MV_ASSERT(exp->state == EXP_STATE_DONE);
	exp_info->Link.Self.DevID = exp->base.id;

	exp_info->Link.Self.DevType = DEVICE_TYPE_EXPANDER;
	exp_info->Link.Self.PhyCnt = exp->parent_phy_count;
	MV_CopyMemory( exp_info->Link.Self.SAS_Address, &exp->sas_addr.value, 8 );
	if (exp->enclosure)
		exp_info->Link.Self.EnclosureID = exp->enclosure->base.id;
	else
		exp_info->Link.Self.EnclosureID = 0xFFFF;
	exp_info->Link.Parent.EnclosureID = 0xFFFF;
	exp_info->Link.Parent.PhyCnt = exp->parent_phy_count;
	MV_CopyMemory(exp_info->Link.Parent.PhyID, exp->parent_phy_id, 8);

	if (exp->base.parent != NULL) {
		if (exp->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER) {
			exp_info->Link.Parent.DevType = DEVICE_TYPE_EXPANDER;
			exp_info->Link.Parent.DevID = exp->base.parent->id;
			if (((domain_expander *)(exp->base.parent))->enclosure)
				exp_info->Link.Parent.EnclosureID = ((domain_expander *)(exp->base.parent))->enclosure->base.id;
		} else if (exp->base.parent->type == BASE_TYPE_DOMAIN_PORT) {
                        phy_id = 0;
			for (i = 0; i < exp->base.root->phy_num; i++) {
				if (exp->base.port->phy_map & MV_BIT(i)) {
					exp_info->Link.Self.PhyID[phy_id] = 
                                                (MV_U8)(exp->base.root->phy[i].att_dev_info >> 24);
					phy_id++;
				}
			}

			exp_info->Link.Parent.DevType = DEVICE_TYPE_PORT;
			exp_info->Link.Parent.DevID = exp->base.port->base.id;
			for (i = 0; i < exp->base.root->phy_num; i++) {
				if( exp->base.port->phy_map & MV_BIT(i) )
					break;
			}
			MV_CopyMemory(exp_info->Link.Parent.SAS_Address, &exp->base.root->phy[i].dev_sas_addr.value, 8);
			for (i = 0; i < 8; i++)
				exp_info->Link.Parent.PhyID[i] += (MV_U8)exp->base.root->base_phy_num;
		}
	}

	exp_info->RouteTableConfigurable = exp->configurable_route_table;
	exp_info->PhyCount = exp->phy_count;

	exp_info->ComponentID = exp->component_id;
	exp_info->ComponentRevisionID = exp->component_rev_id;
	MV_CopyMemory(exp_info->ComponentVendorID, exp->component_vendor_id, 8);
	MV_CopyMemory(exp_info->VendorID, exp->vendor_id, 8);
	MV_CopyMemory(exp_info->ProductID, exp->product_id, 16);
	MV_CopyMemory(exp_info->ProductRev, exp->product_rev, 4);
}

MV_PVOID core_generate_exp_hardware_req(pl_root *root, domain_expander * exp,
        PExp_Info exp_info, PMV_Request req, MV_U8 request_type, MV_U8 phy_index)
{
	MV_Request *new_req = get_intl_req_resource(root, sizeof(smp_request));
	smp_request *smp_req;
	core_context *ctx = (core_context *)new_req->Context[MODULE_CORE];

        if (new_req == NULL) return NULL;

        if (request_type == DISCOVER) {
                MV_ASSERT(exp->base.parent != NULL);
                MV_ASSERT(exp->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER);
        }

	ctx->u.api_smp.buffer = exp_info;
        ctx->u.api_smp.phy_index = phy_index;

	/* Prepare identify ATA task */
	new_req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	new_req->Cdb[1] = CDB_CORE_MODULE;
	new_req->Cdb[2] = CDB_CORE_SMP;

        /* send discover request to the parent. send report general to itself */
	new_req->Device_Id = (request_type == DISCOVER) ? exp->base.parent->id : exp->base.id;
	new_req->Org_Req = req;
	
	smp_req = (smp_request *)core_map_data_buffer(new_req);
	smp_req->function = request_type;
	if (request_type == DISCOVER) {
                smp_req->request.Discover.IgnoredByte4_7[0] = 0;
                smp_req->request.Discover.IgnoredByte4_7[1] = 0;
                smp_req->request.Discover.IgnoredByte4_7[2] = 0;
                smp_req->request.Discover.IgnoredByte4_7[3] = 0;
                smp_req->request.Discover.ReservedByte8 = 0;
		  smp_req->request.Discover.PhyIdentifier = exp->parent_phy_id[phy_index];
                smp_req->request.Discover.IgnoredByte10 = 0;
                smp_req->request.Discover.ReservedByte11 = 0;
	}
	smp_req->smp_frame_type = SMP_REQUEST_FRAME;
	new_req->Completion = (void(*)(MV_PVOID,PMV_Request))core_get_exp_information_callback;

	/* Send this internal request */
        core_unmap_data_buffer(new_req);
        return new_req;
}

MV_VOID core_get_exp_information_callback(MV_PVOID root_p, MV_Request *req)
{
	pl_root *root = (pl_root *)root_p;
	PMV_Request org_req = (PMV_Request)req->Org_Req;
	smp_response *smp_req = (smp_response *)core_map_data_buffer(req);

	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
        core_context *org_ctx = (core_context *)org_req->Context[MODULE_CORE];
	MV_U16 phy_index = ctx->u.api_smp.phy_index;
	PExp_Info exp_info = ctx->u.api_smp.buffer;
        domain_expander *exp;

	if (smp_req->function == DISCOVER) {
                if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
                        exp_info->Link.Self.PhyID[phy_index] = 0;
                } else {
	                exp_info->Link.Self.PhyID[phy_index] = 
                                smp_req->response.Discover.AttachedPhyIdentifier;
		        if (phy_index == 0) {
			        /* only need to store the SAS address once */
			        MV_CopyMemory(exp_info->Link.Parent.SAS_Address, 
                                        smp_req->response.Discover.SASAddress, 8);
		        }
                }
        } else {
                MV_ASSERT(smp_req->function == REPORT_GENERAL);
                if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
  		        exp_info->Configuring = 0;
                        exp_info->ExpChangeCount = 0;
                } else {
                        exp_info->Configuring = smp_req->response.ReportGeneral.Configuring;
        		MV_CopyMemory(&exp_info->ExpChangeCount, 
                                smp_req->response.ReportGeneral.ExpanderChangeCount, 2);
		        /* convert SAS addr to little-endian */
		        if (exp_info->Link.Parent.SAS_Address[0] != 0x50)
			        mv_swap_bytes(exp_info->Link.Parent.SAS_Address, 8);
		        if (exp_info->Link.Self.SAS_Address[0] != 0x50)
			        mv_swap_bytes(exp_info->Link.Self.SAS_Address, 8);
                }
        }

        core_unmap_data_buffer(req);

        MV_ASSERT(org_ctx->u.api_req.remaining > 0);
        org_ctx->u.api_req.remaining--;

        if (org_ctx->u.api_req.remaining == 0) {
                if (org_ctx->u.api_req.start == org_ctx->u.api_req.end) {
                        req->Cmd_Initiator = ctx->u.api_req.pointer;
                        core_queue_completed_req(root->core, org_req);
                } else {
                        core_append_request(root, org_req);
                }
        }
}

MV_VOID
core_get_enc_information(core_extension *core, domain_enclosure *enc,
	PEnclosure_Info pEncInfo)
{
	domain_expander *exp;
	MV_U8 i = 0;

	pEncInfo->Link.Self.DevID = enc->base.id;
	if ( !(enc->status & ENCLOSURE_STATUS_FUNCTIONAL) ||
		 (enc->state != ENCLOSURE_INIT_DONE))
	{
		pEncInfo->Link.Self.DevType = DEVICE_TYPE_NONE;
		return;
	}

	pEncInfo->Link.Self.EnclosureID = enc->base.id;
	pEncInfo->Link.Self.PhyCnt = 0;
	pEncInfo->Link.Parent.DevType = DEVICE_TYPE_NONE;
	pEncInfo->Link.Parent.DevID = 0xFFFF;
	pEncInfo->Link.Parent.PhyCnt = 0;
	pEncInfo->Link.Parent.EnclosureID = 0xFFFF;
	if (enc->base.type == BASE_TYPE_DOMAIN_I2C) {
		pEncInfo->Link.Self.DevType = DEVICE_TYPE_I2C_ENCLOSURE;
		pEncInfo->Link.Parent.DevID = enc->base.port->base.id;
		pEncInfo->Link.Parent.DevType = DEVICE_TYPE_PORT;
	}
	else
		pEncInfo->Link.Self.DevType = DEVICE_TYPE_ENCLOSURE;

	pEncInfo->Status = ENC_STATUS_OK;

	LIST_FOR_EACH_ENTRY_TYPE(exp, &enc->expander_list, domain_expander, enclosure_queue_pointer)
	{
		pEncInfo->ExpanderIDs[i] = exp->base.id;
		i++;
	}
	pEncInfo->ExpanderCount = i;

	MV_CopyMemory(pEncInfo->LogicalID, enc->enclosure_logical_id, 8);
	MV_CopyMemory(pEncInfo->VendorID, enc->vendor_id, 8);
	MV_CopyMemory(pEncInfo->ProductID, enc->product_id, 16);
	MV_CopyMemory(pEncInfo->RevisionLevel, enc->product_revision, 4);
}

MV_VOID core_get_enc_info( MV_PVOID extension, MV_PVOID buffer)
{
	core_extension *core = (core_extension *)extension;
	MV_U16 start_id = 0, end_id = 0;
	domain_enclosure *enc = NULL;
	PEnclosure_Info pEncInfo;
	PEnclosure_Info_Request pEncReq;
	PRequestHeader header = NULL;
	MV_U16 i;
	domain_base *base = NULL;

	pEncReq = (PEnclosure_Info_Request)buffer;
	header = &pEncReq->header;
	if (header->requestType == REQUEST_BY_RANGE) {
		start_id = header->startingIndexOrId;
		end_id = MAX_ID;
	} else if (header->requestType == REQUEST_BY_ID) {
		start_id = header->startingIndexOrId;
		end_id = header->startingIndexOrId;
	}
	pEncInfo = pEncReq->encInfo;
	header->numReturned = 0;
	header->nextStartingIndex = NO_MORE_DATA;

	for (i = start_id; i <= end_id; i++) {
		base = get_device_by_id(&core->lib_dev, i);
		if ((base!= NULL)&&
			((base->type == BASE_TYPE_DOMAIN_ENCLOSURE)||
			(base->type == BASE_TYPE_DOMAIN_I2C))) {
			enc = (domain_enclosure *)base;
			core_get_enc_information( core, enc, pEncInfo );
		} else {
			pEncInfo->Link.Self.DevID = i;
			pEncInfo->Link.Self.DevType = DEVICE_TYPE_NONE;
		}

		if (pEncInfo->Link.Self.DevType != DEVICE_TYPE_NONE) {
			header->numReturned++;
			pEncInfo++;
			if ((header->requestType == REQUEST_BY_RANGE) &&
				(header->numReturned == header->numRequested)) {
				header->nextStartingIndex = i+1;
				break;
			}
		}
	}
}

MV_U8
core_pd_request_get_enclosure_info(core_extension * core_p, PMV_Request req)
{
	PEnclosure_Info pEncInfo = NULL;
	domain_base *base = NULL;
        MV_PVOID buf_ptr = core_map_data_buffer(req);
	PEnclosure_Info_Request pEncReq = (PEnclosure_Info_Request)buf_ptr;

	if (buf_ptr == NULL ||
		req->Data_Transfer_Length < sizeof(Enclosure_Info_Request)) {

                core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	if (pEncReq->header.requestType == REQUEST_BY_RANGE &&
		pEncReq->header.nextStartingIndex == 0){
		pEncReq->header.startingIndexOrId = 0;
	} else if (pEncReq->header.startingIndexOrId >= MAX_ID) {
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_ENC_ID;
		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
                core_unmap_data_buffer(req);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	core_get_enc_info(core_p, buf_ptr);

	if (pEncReq->header.requestType == REQUEST_BY_ID &&
		pEncReq->header.numReturned == 0) {
		if (req->Sense_Info_Buffer != NULL)
			((MV_PU8)req->Sense_Info_Buffer)[0] = ERR_INVALID_ENC_ID;
		req->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;
	} else
		req->Scsi_Status = REQ_STATUS_SUCCESS;

        core_unmap_data_buffer(req);
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

