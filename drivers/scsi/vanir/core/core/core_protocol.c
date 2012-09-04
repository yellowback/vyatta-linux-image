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
#include "core_resource.h"
#include "core_internal.h"
#include "core_util.h"
#include "core_manager.h"
#include "com_util.h"

#include "core_error.h"
#include "core_expander.h"

extern MV_PVOID sata_store_received_fis(pl_root *root, MV_U8 register_set, MV_U32 flag);

MV_BOOLEAN prot_init_pl(pl_root *root, MV_U16 max_io,
	MV_PVOID core, MV_LPVOID mmio,
	lib_device_mgr *lib_dev, lib_resource_mgr *rsrc)
{
	MV_U16 slot_count, delv_q_size, cmpl_q_size, received_fis_count;
	MV_U32 item_size;
	MV_PU8 vir;
	MV_U32 i;
	domain_port *port;
	domain_phy *phy;
	core_extension* core_ext = (core_extension*)core;
	core_get_supported_pl_counts(max_io, &slot_count, &delv_q_size, &cmpl_q_size, &received_fis_count);

	root->mmio_base = mmio;

	root->lib_dev = lib_dev;
	root->lib_rsrc = rsrc;
	root->core = (MV_PVOID)core;

	root->phy_num = core_ext->chip_info->n_phy;
	root->slot_count_support = slot_count;

	root->delv_q_size = delv_q_size;
	root->cmpl_q_size = cmpl_q_size;
	root->last_delv_q = 0xfff;
	root->last_cmpl_q = 0xfff;

	/* allocate memory for running_req */
	item_size = sizeof(PMV_Request) * slot_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	root->running_req = (PMV_Request *)vir;
	root->running_num = 0;

	/* allocate memory for saved fis */
	item_size = sizeof(saved_fis) * received_fis_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	root->saved_fis_area = (saved_fis *)vir;

	/* allocate memory for command table wrapper */
	/* this is so far used as a array, not a pool */
	item_size = sizeof(hw_buf_wrapper) * slot_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	root->cmd_table_wrapper = (hw_buf_wrapper*)vir;

	/* slot pool */
	item_size = sizeof(MV_U16) * slot_count;
	vir = (MV_PU8)lib_rsrc_malloc_cached(rsrc, item_size);
	if (vir == NULL) return MV_FALSE;
	root->slot_pool.Stack = (MV_PU16)vir;
	root->slot_pool.Size = slot_count;
	Tag_Init_FIFO(&root->slot_pool, root->slot_pool.Size);

	for (i = 0; i < core_ext->chip_info->n_phy; i++) {
		phy = &(root->phy[i]);
		phy->id = (MV_U8)i;
		phy->asic_id = (MV_U8)i;
              phy->root = root;
	}

	for (i = 0; i < core_ext->chip_info->n_phy; i++) {
		extern void init_port(pl_root *root, domain_port *port);
		port = &(root->ports[i]);

		port->base.id = (MV_U8)i;
		port->type = PORT_TYPE_SAS;

		port->phy_num = 0;
		port->phy_map = 0;
		port->asic_phy_map = 0;
		port->curr_phy_map = 0;
		port->phy = NULL;

		init_port(root, port);
	}

	return MV_TRUE;
}

MV_QUEUE_COMMAND_RESULT prot_send_request(pl_root *root, struct _domain_base *base,
	MV_Request *req)
{
	mv_command_header *cmd_list = (mv_command_header *)root->cmd_list;
	mv_command_header *cmd_header;
	core_context *ctx = req->Context[MODULE_CORE];
	core_extension * core = (core_extension *)root->core;
	command_handler *handler = (command_handler *)ctx->handler;
	hw_buf_wrapper *table_wrapper = NULL, *sg_wrapper = NULL;
	MV_PHYSICAL_ADDR tmpPhy;
	MV_U16 slot = 0, prd_num = 0;
	MV_QUEUE_COMMAND_RESULT result;
	MV_LPVOID mmio = root->mmio_base;
	domain_phy *phy;
	if (base->outstanding_req >= base->queue_depth) {
		return MV_QUEUE_COMMAND_RESULT_FULL;
	}


	if (base->port != NULL) {
                if (port_has_error_req(root, base->port) || (base->blocked == MV_TRUE) ||
			port_has_init_req(root, base->port)) {
			if (!CORE_IS_EH_REQ(ctx) && !CORE_IS_INIT_REQ(ctx)) {
				return MV_QUEUE_COMMAND_RESULT_FULL;
			}
		}
	}
	
	if (base->cmd_issue_stopped) {
		pal_abort_device_running_req(root, base);
		hal_enable_register_set(root, base);
		base->cmd_issue_stopped = MV_FALSE;
	}
	if ((BASE_TYPE_DOMAIN_DEVICE == base->type) 
		&& (((domain_device *)base)->status & DEVICE_STATUS_FROZEN) && (!CORE_IS_EH_REQ(ctx))) {
		req->Scsi_Status = REQ_STATUS_NO_DEVICE;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	/* verify this commands. Maybe can be finished right away without resource */
	result = handler->verify_command(root, base, req);
	if (result != MV_QUEUE_COMMAND_RESULT_PASSED) {
		return result;
	}
	
	if (Tag_IsEmpty(&root->slot_pool)) {
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
	} else {
		slot = Tag_GetOne(&root->slot_pool);
	}
	ctx->slot = slot;

	/* set up hardware SG table */
	sg_wrapper = get_sg_buf(root->lib_rsrc);
	if (sg_wrapper == NULL) {
		Tag_ReleaseOne(&root->slot_pool, slot);
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
	}

	req->sg_table = ossw_pci_pool_alloc (core->sg_pool, &req->sg_table_phy.value);
	if (req->sg_table == NULL) {
		Tag_ReleaseOne(&root->slot_pool, slot);
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
	} else {
		MV_ZeroMemory(req->sg_table, CORE_HW_SG_ENTRY_SIZE*CORE_MAX_HW_SG_ENTRY_COUNT);
	}
	sg_wrapper->vir = req->sg_table;
	sg_wrapper->phy =  req->sg_table_phy;

	MV_ZeroMemory(sg_wrapper->vir, CORE_HW_SG_ENTRY_SIZE*CORE_MAX_HW_SG_ENTRY_COUNT);
	MV_DASSERT(ctx->sg_wrapper == NULL);
	ctx->sg_wrapper = sg_wrapper;

	/* set up command header */
	cmd_header = &cmd_list[slot];
	table_wrapper = &root->cmd_table_wrapper[slot];

	req->cmd_table = ossw_pci_pool_alloc(core->ct_pool, &req->cmd_table_phy.value);
	if (req->cmd_table == NULL) {
		Tag_ReleaseOne(&root->slot_pool, slot);
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
	} else {
		MV_ZeroMemory(req->cmd_table, sizeof(mv_command_table));
	}
	table_wrapper->vir = req->cmd_table;
	table_wrapper->phy =  req->cmd_table_phy;

	MV_ZeroMemory(cmd_header, sizeof(mv_command_header));
	MV_ZeroMemory(table_wrapper->vir, sizeof(mv_command_table));

	root->running_req[slot] = req;
	root->running_num++;

	prd_num = prot_set_up_sg_table(root, req, sg_wrapper);
	cmd_header->ctrl_nprd = MV_CPU_TO_LE32(prd_num << CH_PRD_TABLE_LEN_SHIFT);
	cmd_header->prd_table_addr = MV_CPU_TO_LE64(sg_wrapper->phy);


	tmpPhy.value = table_wrapper->phy.value + OFFSET_OF(struct _mv_command_table, status_buff);
	cmd_header->status_buff_addr = MV_CPU_TO_LE64(tmpPhy);

	tmpPhy.value = table_wrapper->phy.value + OFFSET_OF(struct _mv_command_table, table);
	cmd_header->table_addr = MV_CPU_TO_LE64(tmpPhy);

	tmpPhy.value = table_wrapper->phy.value + OFFSET_OF(struct _mv_command_table, open_address_frame);
	cmd_header->open_addr_frame_addr  = MV_CPU_TO_LE64(tmpPhy);

	/* cmd_header->tag is different between SSP and STP/SATA
	 * it'll be further handled in prepare_command handler.*/
	cmd_header->tag = MV_CPU_TO_LE16(slot);
	cmd_header->data_xfer_len = MV_CPU_TO_LE32(req->Data_Transfer_Length);

	core_set_cmd_header_selector(cmd_header);
	
	/* splits off to handle protocol-specific tasks */
	handler->prepare_command(root, base, cmd_header, table_wrapper->vir, req);
	handler->send_command(root, base, req);

	return MV_QUEUE_COMMAND_RESULT_SENT;
}

MV_U32 prot_get_delv_q_entry(pl_root *root)
{
	MV_LPVOID mmio = root->mmio_base;
	MV_U32 tmp;


	root->last_delv_q++;
	if (root->last_delv_q >= root->delv_q_size)
		root->last_delv_q = 0;
	tmp = (MV_U32)(root->last_delv_q);
	return tmp;
}

MV_VOID prot_write_delv_q_entry(pl_root *root, MV_U32 entry)
{
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_DELV_Q_WR_PTR, entry);
}

PMV_Request get_intl_req_resource(pl_root *root, MV_U32 buf_size)
{
	PMV_Request req;
	core_context *ctx;
	hw_buf_wrapper *wrapper = NULL;

	PMV_SG_Table sg_tbl;

	req = get_intl_req(root->lib_rsrc);
	if (req == NULL) return NULL;

	MV_DASSERT(req->Context[MODULE_CORE] == NULL);
	ctx = get_core_context(root->lib_rsrc);
	if (ctx == NULL) {
		free_intl_req(root->lib_rsrc, req);
		return NULL;
	}

	MV_DASSERT(buf_size <= SCRATCH_BUFFER_SIZE);
	if (buf_size != 0) {
		wrapper = get_scratch_buf(root->lib_rsrc);
		if (wrapper == NULL) {
			free_intl_req(root->lib_rsrc, req);
			free_core_context(root->lib_rsrc, ctx);
			return NULL;
		}
		ctx->buf_wrapper = wrapper;
		req->Data_Buffer = wrapper->vir;
	} else {
		ctx->buf_wrapper = NULL;
		req->Data_Buffer = NULL;
	}
	req->Tag = 0xaa;
	req->Context[MODULE_CORE] = ctx;
	req->Cmd_Initiator = root;
	req->Data_Transfer_Length = buf_size;
	req->Scsi_Status = REQ_STATUS_PENDING;
	req->Req_Flag = 0;
	req->Cmd_Flag = 0;
	req->Time_Out = 5;

	/* Make the SG table. */
	sg_tbl = &req->SG_Table;
	SGTable_Init(sg_tbl, 0);
	if (buf_size != 0) {
		SGTable_Append(
			sg_tbl,
			wrapper->phy.parts.low,
			wrapper->phy.parts.high,
			buf_size);
	}

	MV_ZeroMemory(req->Cdb, sizeof(req->Cdb));
	return req;
}

void intl_req_release_resource(lib_resource_mgr *rsrc, PMV_Request req)
{
	core_context *ctx;
	hw_buf_wrapper *wrapper;

	ctx = req->Context[MODULE_CORE];
	MV_ASSERT(ctx != NULL);

	wrapper = ctx->buf_wrapper;
	if (wrapper) {
		free_scratch_buf(rsrc, wrapper);
		ctx->buf_wrapper = NULL;
	}

	if (ctx) {
		free_core_context(rsrc, ctx);
		req->Context[MODULE_CORE] = NULL;
	}

	free_intl_req(rsrc, req);
}

MV_VOID prot_clean_slot(pl_root *root, domain_base *base, MV_U16 slot,
	MV_Request *req)
{
	hw_buf_wrapper *sg_wrapper;
	core_extension *core = (core_extension *)root->core;
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];

	root->running_req[slot] = NULL;
	root->running_num--;

	if (base->outstanding_req) {
		base->outstanding_req--;
	}
	else {
		CORE_DPRINT(("device %d outstanding req is zero??.\n",base->id));
	}

	Tag_ReleaseOne(&root->slot_pool, slot);

	ossw_pci_pool_free(core->ct_pool, req->cmd_table, req->cmd_table_phy.value);
	ossw_pci_pool_free(core->sg_pool, req->sg_table, req->sg_table_phy.value);
	mv_renew_timer(root->core, req);

	/* not every req will have a sg_wrapper (eg, instant requests) */
	sg_wrapper = ctx->sg_wrapper;
	if (sg_wrapper != NULL) {
		free_sg_buf(root->lib_rsrc, sg_wrapper);
		ctx->sg_wrapper = NULL;
	}

	/* release NCQ tag & register set */
	if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
		domain_device *dev = (domain_device *)base;
		if (IS_STP_OR_SATA(dev)) {
			if (req->Cmd_Flag & CMD_FLAG_NCQ) {
				sata_free_ncq_tag(dev, req);
			}

			if (ctx->req_flag & CORE_REQ_FLAG_NEED_D2H_FIS) {
				if (dev->register_set != NO_REGISTER_SET) {
					if ((req->Scsi_Status != REQ_STATUS_SUCCESS)
                                		&& (req->Cmd_Flag & CMD_FLAG_PIO))		        			
	        				ctx->received_fis =
		        				sata_store_received_fis(root,
					        			dev->register_set,
						        		(req->Cmd_Flag & (~CMD_FLAG_PIO)));
                                	else	        			
		        			ctx->received_fis =
			        			sata_store_received_fis(root,
				        			dev->register_set,
					        		req->Cmd_Flag);
				} else {
                                        ctx->received_fis = NULL;
				}
			}
			if (dev->register_set != NO_REGISTER_SET) {
				if (dev->base.outstanding_req == 0) {
					sata_free_register_set(root, dev->register_set);
					dev->register_set = NO_REGISTER_SET;
				}
			}
		}
	} else if (base->type == BASE_TYPE_DOMAIN_PM) {
		domain_pm *pm = (domain_pm *)base;

		if (ctx->req_flag & CORE_REQ_FLAG_NEED_D2H_FIS) {
                        if (pm->register_set != NO_REGISTER_SET) {
			        ctx->received_fis =
				        sata_store_received_fis(root,
					        pm->register_set,
					        req->Cmd_Flag);
                        } else {
                                ctx->received_fis = NULL;
                        }
		}

		if (pm->register_set != NO_REGISTER_SET) {
			if (pm->base.outstanding_req == 0) {
				sata_free_register_set(root, pm->register_set);
				pm->register_set = NO_REGISTER_SET;
			}
		}
	}
}

MV_VOID prot_process_cmpl_req(pl_root *root, MV_Request *req)
{
	core_context *ctx = (core_context *)req->Context[MODULE_CORE];
	command_handler *handler = (command_handler *)ctx->handler;
	domain_base *base = (domain_base *)get_device_by_id(root->lib_dev,
		req->Device_Id);
	domain_base *err_req_base;
	domain_port *err_req_port;
	PMV_Request err_req = NULL;
	core_context *err_req_ctx;
	core_extension *core = root->core;
	MV_BOOLEAN push_back_req = FALSE;

	MV_DASSERT(ctx != NULL);

	prot_clean_slot(root, base, ctx->slot, req);

	if ((ctx->error_info & EH_INFO_NEED_RETRY)
		&& !CORE_IS_EH_REQ(ctx)
		&& !CORE_IS_INIT_REQ(ctx)) {
		MV_ASSERT(req->Scsi_Status != REQ_STATUS_SUCCESS);

		if ((core->revision_id != VANIR_C2_REV)
			&& (ctx->error_info & EH_INFO_WD_TO_RETRY) && (!Counted_List_Empty(&core->error_queue))) {
			LIST_FOR_EACH_ENTRY_TYPE(err_req, &core->error_queue, MV_Request, Queue_Pointer) {
				err_req_ctx = err_req->Context[MODULE_CORE];
				if (err_req_ctx->error_info & EH_INFO_WD_TO_RETRY) {
					err_req_base = (domain_base *)get_device_by_id(
											&core->lib_dev,
											err_req->Device_Id
											);
					err_req_port = err_req_base->port;
					if (err_req_port == base->port) {
						push_back_req = MV_TRUE;
						break;
					}
				}
			}
			if (push_back_req) {
				core_push_running_request_back(root, req);
			}
			else { 
				core_queue_error_req(root, req, MV_TRUE);
				core_sleep_millisecond(core, 1000);
			}
		}
		else
			core_queue_error_req(root, req, MV_TRUE);
	} else {
		core_queue_completed_req(root->core, req);
	}
}

err_info_record * prot_get_command_error_info(mv_command_table *cmd_table,
	 MV_PU32 cmpl_q)
{
	MV_U64 tmp;
	err_info_record *err_info;

	if (!(*cmpl_q & RXQ_ERR_RCRD_XFRD))
		return NULL;

	tmp.value = (_MV_U64)(*(_MV_U64 *)(&cmd_table->status_buff.err_info));
	if (U64_COMPARE_U32(tmp, 0) != 0) {
		CORE_EH_PRINT(("error info 0x%016llx.\n", MV_CPU_TO_LE64(tmp)));
		err_info = &cmd_table->status_buff.err_info;
		return err_info;
	} else {
		return NULL;
	}
}


void prot_fill_sense_data(MV_Request *req, MV_U8 sense_key,
	MV_U8 ad_sense_code)
{
	if (req->Sense_Info_Buffer != NULL) {
		((MV_PU8)req->Sense_Info_Buffer)[0] = 0x70;	/* Current */
		((MV_PU8)req->Sense_Info_Buffer)[2] = sense_key;
		/* additional sense length */
		((MV_PU8)req->Sense_Info_Buffer)[7] = 0;
		/* additional sense code */
		((MV_PU8)req->Sense_Info_Buffer)[12] = ad_sense_code;
	}
}
