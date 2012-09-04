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
#include "core_internal.h"
#include "core_hal.h"
#include "core_manager.h"
#include "core_protocol.h"
#include "core_util.h"
#include "hba_inter.h"
#include "core_error.h"

extern MV_VOID prot_process_cmpl_req(pl_root *root, MV_Request *req);
MV_VOID mv_set_sas_addr(MV_PVOID root_p, MV_PVOID phy_p, MV_PU8 sas_addr)
{
	pl_root *root = (pl_root *)root_p;
	domain_phy *phy = (domain_phy *)phy_p;
	MV_U64 val64;

	MV_CopyMemory(&val64.value, sas_addr, 8);

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME4);
	WRITE_PORT_CONFIG_DATA(root, phy, val64.parts.high);

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME3);
	WRITE_PORT_CONFIG_DATA(root, phy, val64.parts.low);
}

MV_VOID mv_set_dev_info(MV_PVOID root_p, MV_PVOID phy_p)
{
	pl_root *root = (pl_root *)root_p;
	domain_phy *phy = (domain_phy *)phy_p;
	MV_U32 reg;

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME5);
	reg = READ_PORT_CONFIG_DATA(root, phy);
	reg &= 0xffffff00;
	reg |= phy->asic_id;
	WRITE_PORT_CONFIG_DATA(root, phy, reg);

	reg = (SAS_END_DEV<<4) + ((PORT_DEV_STP_INIT|PORT_DEV_SMP_INIT|PORT_DEV_SSP_INIT)<<8);
 	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME0);
	WRITE_PORT_CONFIG_DATA(root, phy, reg);
}

MV_VOID mv_reset_phy(MV_PVOID root_p, MV_U8 logic_phy_map, MV_BOOLEAN hard_reset)
{
	pl_root *root = (pl_root *)root_p;
	domain_phy *phy;
	MV_U32 reg;
	MV_U32 i;
       MV_U32 phy_irq_mask = 0;
	MV_U8 init = MV_FALSE;
	MV_U32 phyrdy_wait_time = 0;

       core_disable_ints(root->core);
	for (i = 0; i < root->phy_num; i++) {
		if (!(logic_phy_map & MV_BIT(i)))
			continue;

		phy = &root->phy[i];

		if (init == MV_FALSE) {
			phy_irq_mask = phy->phy_irq_mask;
			init = MV_TRUE;
		}
		MV_ASSERT(phy_irq_mask == phy->phy_irq_mask);

                 phy->phy_irq_mask &= ~(IRQ_PHY_RDY_CHNG_MASK | IRQ_PHY_RDY_CHNG_1_TO_0);
                /* disable interrupt */
                WRITE_PORT_IRQ_MASK(root, phy, phy->phy_irq_mask);

		WRITE_PORT_VSR_ADDR(root, phy, VSR_IRQ_MASK);
		reg = READ_PORT_VSR_DATA(root, phy);
		reg &= ~VSR_IRQ_PHY_TIMEOUT;
		WRITE_PORT_VSR_DATA(root, phy, reg);

		core_sleep_millisecond(root->core, 10);

		/* reset */
		reg = READ_PORT_PHY_CONTROL(root, phy);
		if (hard_reset)
			reg |= SCTRL_PHY_HARD_RESET_SEQ;
		else
			reg |= SCTRL_STP_LINK_LAYER_RESET;

		if (((core_extension *)(root->core))->revision_id != VANIR_C2_REV)
			reg |= (SCTRL_STP_LINK_LAYER_RESET | SCTRL_SSP_LINK_LAYER_RESET);
		
		WRITE_PORT_PHY_CONTROL(root, phy, reg);
	}

	/* Polling for phy ready after OOB */
	phyrdy_wait_time = 100;
	while ((phyrdy_wait_time>0) && logic_phy_map) {
		for (i = 0; i < root->phy_num; i++) {
			if (!(logic_phy_map & MV_BIT(i)))
				continue;
			phy = &root->phy[i];

			if (mv_is_phy_ready(root, phy) || 
				(phyrdy_wait_time == 1))
			{
				WRITE_PORT_IRQ_STAT(root, phy, \
					(IRQ_PHY_RDY_CHNG_MASK | IRQ_PHY_RDY_CHNG_1_TO_0));

				WRITE_PORT_VSR_ADDR(root, phy, VSR_IRQ_STATUS);
				WRITE_PORT_VSR_DATA(root, phy, VSR_IRQ_PHY_TIMEOUT);

				/* enable phy interrupt */
				phy->phy_irq_mask = phy_irq_mask;
				WRITE_PORT_IRQ_MASK(root, phy, phy->phy_irq_mask);
				logic_phy_map &= ~(MV_BIT(i));
			}
		}
		core_sleep_millisecond(root->core, 10);
		phyrdy_wait_time--;
	}
        core_enable_ints(root->core);
}

MV_VOID hal_clear_srs_irq(MV_PVOID root_p, MV_U32 set, MV_BOOLEAN clear_all)
{
	pl_root *root = (pl_root *)root_p;
	MV_U32 reg;

	if (clear_all == MV_TRUE) {
		reg = READ_SRS_IRQ_STAT(root, 0);
		if (reg) {
			WRITE_SRS_IRQ_STAT(root, 0, reg);
		}
		reg = READ_SRS_IRQ_STAT(root, 32);
		if (reg) {
			WRITE_SRS_IRQ_STAT(root, 32, reg);
		}
	} else {
		reg = READ_SRS_IRQ_STAT(root, set);
		if (reg & MV_BIT(set % 32)) {
			CORE_DPRINT(("register set 0x%x was stopped.\n", set));
			WRITE_SRS_IRQ_STAT(root, set, MV_BIT(set % 32));
		}
	}
}

MV_VOID hal_enable_register_set(MV_PVOID root_p, MV_PVOID base_p)
{
	pl_root *root = (pl_root *)root_p;
	domain_base *base = (domain_base *)base_p;
	domain_device *dev;
	MV_U32 reg;

	if (base->type == BASE_TYPE_DOMAIN_DEVICE) {
		dev = (domain_device *)base;
		if (IS_STP_OR_SATA(dev) && (dev->register_set != NO_REGISTER_SET)) {
			hal_clear_srs_irq(root, dev->register_set, MV_FALSE);
		}
	}
	
	reg = MV_REG_READ_DWORD(root->mmio_base, COMMON_IRQ_STAT);
	reg |= INT_CMD_ISSUE_STOPPED;
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_IRQ_STAT, reg);

	reg = MV_REG_READ_DWORD(root->mmio_base, COMMON_CONTROL);
	reg |= 0xff00;
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CONTROL, reg);
}

MV_VOID hal_disable_io_chip(MV_PVOID root_p)
{
	pl_root *root = (pl_root *)root_p;
	MV_LPVOID mmio = root->mmio_base;
	MV_U32 tmp;

        /* disable CMD/CMPL_Q/RESP mode */
	tmp = MV_REG_READ_DWORD(mmio, COMMON_CONTROL);
	tmp &= ~CONTROL_EN_CMD_ISSUE;
	tmp &= ~CONTROL_RSPNS_RCV_EN;
        /* tmp &= ~CONTROL_CMD_CMPLT_IRQ_MD; */
	MV_REG_WRITE_DWORD(mmio, COMMON_CONTROL, tmp);

        /* disable interrupt */
	tmp = 0;
	MV_REG_WRITE_DWORD(mmio, COMMON_IRQ_MASK, tmp);
}

void core_disable_ints(void *ext)
{
    core_extension *core = (core_extension *)ext;
    
    MV_REG_WRITE_DWORD(core->mmio_base, CPU_MAIN_IRQ_MASK_REG, 0);
}

void core_enable_ints(void *ext)
{
    core_extension *core = (core_extension *)ext;
    
    MV_REG_WRITE_DWORD(core->mmio_base, CPU_MAIN_IRQ_MASK_REG, core->irq_mask);
}
MV_BOOLEAN core_check_int(void *ext)
{	
	core_extension *core = (core_extension *)ext;
	MV_U32 main_irq;
	pl_root *root;

	main_irq = MV_REG_READ_DWORD(core->mmio_base, CPU_MAIN_INT_CAUSE_REG);
	main_irq = main_irq & core->irq_mask;
	if (main_irq == 0) return(MV_FALSE);
	return MV_TRUE;
}
MV_BOOLEAN core_clear_int(core_extension *core)
{
	MV_U32 main_irq;
	pl_root *root;

	main_irq = MV_REG_READ_DWORD(core->mmio_base, CPU_MAIN_INT_CAUSE_REG);
	main_irq = main_irq & core->irq_mask;
	if (main_irq == 0) return(MV_FALSE);

	core->main_irq |= main_irq;

	if (main_irq & INT_MAP_SAS) {
		if (main_irq & INT_MAP_SASINTA) {
			root = &core->roots[0];
			io_chip_clear_int(root);
		}
		if (main_irq & INT_MAP_SASINTB) {
			root = &core->roots[1];
			io_chip_clear_int(root);
		}
	}

	if (main_irq)
		return MV_TRUE;
	else
		return MV_FALSE;
}

MV_VOID core_handle_int(core_extension *core)
{
	pl_root *root;

	/* handle io chip interrupt */
	if (core->main_irq & INT_MAP_SAS) {
		if (core->main_irq & INT_MAP_SASINTA) {
			root = &core->roots[0];
			io_chip_handle_int(root);
		}
		if (core->main_irq & INT_MAP_SASINTB) {
			root = &core->roots[1];
			io_chip_handle_int(root);
		}

	}
	core->main_irq = 0;
}

MV_BOOLEAN sata_is_register_set_stopped(MV_PVOID root_p, MV_U8 set)
{
	pl_root *root = (pl_root *)root_p;
	MV_U32 reg_set_irq_stat;
	reg_set_irq_stat = READ_SRS_IRQ_STAT(root, set);

	if (reg_set_irq_stat & MV_BIT(set))
		return MV_TRUE;
	else
		return MV_FALSE;
}

MV_U8 sata_get_register_set(MV_PVOID root_p)
{
	pl_root *root = (pl_root *)root_p;
	MV_LPVOID mmio = root->mmio_base;
	MV_U32 tmp, reg;

	int i;
	i = ffc64(root->sata_reg_set);
	if (i >= 32) {
		root->sata_reg_set.parts.high |= MV_BIT(i - 32);
		WRITE_REGISTER_SET_ENABLE(root, i, root->sata_reg_set.parts.high);

		return (MV_U8) i;
	}
	else if (i >= 0) {
		root->sata_reg_set.parts.low |= MV_BIT(i);
		WRITE_REGISTER_SET_ENABLE(root, i, root->sata_reg_set.parts.low);

		return (MV_U8) i;
	}

	return NO_REGISTER_SET;
}

void sata_free_register_set(MV_PVOID root_p, MV_U8 set)
{
	pl_root *root = (pl_root *)root_p;
	MV_LPVOID mmio = root->mmio_base;
	MV_U32 reg, tmp;

	if (set < 32) {
		root->sata_reg_set.parts.low &= ~MV_BIT(set);
		WRITE_REGISTER_SET_ENABLE(root, set, root->sata_reg_set.parts.low);
	}
	else {
		root->sata_reg_set.parts.high &= ~MV_BIT(set - 32);
		WRITE_REGISTER_SET_ENABLE(root, set, root->sata_reg_set.parts.high);
	}
	hal_clear_srs_irq(root, set, MV_FALSE);
}
MV_U8	exp_check_plugging_finished(pl_root *root, domain_port *port);

MV_VOID io_chip_clear_int(pl_root *root)
{
	MV_U32 irq = MV_REG_READ_DWORD(root->mmio_base, COMMON_IRQ_STAT);
	domain_phy *phy;
	MV_U8 i;
	MV_U32 reg, cmpl_wp;
	
	root->comm_irq |= irq;
	
	/* for completion queue interrupt INT_CMD_CMPL_MASK */
	if (root->comm_irq & INT_CMD_CMPL_MASK )
		MV_REG_WRITE_DWORD(root->mmio_base, COMMON_IRQ_STAT, INT_CMD_CMPL_MASK);

	if (root->comm_irq & INT_PRD_BC_ERR ) {
		CORE_EVENT_PRINT(("Find PRD BC error, COMMON_IRQ_STAT: 0x%08x\n", irq));
		MV_REG_WRITE_DWORD(root->mmio_base, COMMON_IRQ_STAT, INT_PRD_BC_ERR);
		MV_REG_READ_DWORD(root->mmio_base, COMMON_IRQ_STAT);
	}

	if (root->comm_irq & (INT_PORT_MASK|INT_PHY_MASK)) {
		/* for port interrupt and PHY interrupt */
		for (i = 0; i < root->phy_num; i++) {
			phy = &root->phy[i];
			if (hal_has_phy_int(root->comm_irq, phy)) {
				/* clear PORT interrupt status */
				reg = READ_PORT_IRQ_STAT(root, phy);
				CORE_EVENT_PRINT(("phy %d READ_PORT_IRQ_STAT: 0x%08x\n", phy->id,reg));
				phy->irq_status |= reg & phy->phy_irq_mask;
				WRITE_PORT_IRQ_STAT(root, phy, reg);
				/* Read back IRQ status register */
				READ_PORT_IRQ_STAT(root, phy);
				/* clear PHY interrupt status */
				WRITE_PORT_VSR_ADDR(root, phy, VSR_IRQ_STATUS);
				reg = READ_PORT_VSR_DATA(root, phy);
				if (reg & VSR_IRQ_PHY_TIMEOUT) {
					CORE_EVENT_PRINT(("READ_PORT_VSR_DATA: 0x%08x\n", reg));
					WRITE_PORT_VSR_DATA(root, phy, VSR_IRQ_PHY_TIMEOUT);
				}

				CORE_EVENT_PRINT(("phy %d irq_status = 0x%x.\n", phy->id, phy->irq_status));
			}
		}
	}
}

MV_VOID io_chip_handle_port_int(pl_root *root)
{
	domain_phy *phy;
	MV_U8 i;
	MV_U32 port_irq;

	for (i=0; i<root->phy_num; i++) {
		phy = &root->phy[i];
		if (!hal_has_phy_int(root->comm_irq, phy))
			continue;

		port_irq = phy->irq_status;
		root->comm_irq = hal_remove_phy_int(root->comm_irq, phy);
		CORE_EVENT_PRINT(("phy %d irq_status %08X.\n", phy->id, port_irq));

		if (port_irq & IRQ_UNASSOC_FIS_RCVD_MASK ||
			port_irq & IRQ_SIG_FIS_RCVD_MASK) {
			if ((phy->type == PORT_TYPE_SATA) && (phy->port))
				sata_port_notify_event(phy->port, port_irq);
			phy->irq_status &=
				~(IRQ_UNASSOC_FIS_RCVD_MASK | IRQ_SIG_FIS_RCVD_MASK);
		}

		if (port_irq & (IRQ_PHY_RDY_CHNG_MASK|IRQ_PHY_RDY_CHNG_1_TO_0)) {
			pal_notify_event(root, i, PL_EVENT_PHY_CHANGE);
		}

		if (port_irq & IRQ_STP_SATA_PHY_DEC_ERR_MASK) {
			CORE_EVENT_PRINT(("PHY decoding error for root %p phy %d\n",\
				root, phy->id));
			phy->irq_status &= ~IRQ_STP_SATA_PHY_DEC_ERR_MASK;
		}

		if (port_irq & IRQ_ASYNC_NTFCN_RCVD_MASK) {
			CORE_EVENT_PRINT(("PHY asynchronous notification for root %p phy %d\n",\
				root, phy->id));
			pal_notify_event(root, i, PL_EVENT_ASYNC_NOTIFY);
			phy->irq_status &= ~IRQ_ASYNC_NTFCN_RCVD_MASK;
		}

		if (port_irq & IRQ_BRDCST_CHNG_RCVD_MASK) {
			pal_notify_event(root, i, PL_EVENT_BROADCAST_CHANGE);
			phy->irq_status &= ~IRQ_BRDCST_CHNG_RCVD_MASK;
		}
	}
}

MV_VOID sata_handle_non_spcfc_ncq_err(pl_root *root, MV_U8 register_set)
{
	domain_device *device;
	MV_Request *req;
	core_context *ctx;
	struct _error_context *err_ctx;
	MV_U16 i;
	MV_U32 params[MAX_EVENT_PARAMS];

	device = get_device_by_register_set(root, register_set);
	if (device) {
		io_chip_handle_cmpl_queue_int(root);

		err_ctx = &device->base.err_ctx;
		mv_cancel_timer(root->core, &device->base);

		if (!List_Empty(&err_ctx->sent_req_list)) {
			req = LIST_ENTRY(
				(&err_ctx->sent_req_list)->next, MV_Request, Queue_Pointer);
			ctx = (core_context *)req->Context[MODULE_CORE];

			if (sata_is_register_set_stopped(root, device->register_set)) {
				CORE_EH_PRINT(("Register set %d is disabled\n", device->register_set));
				device->base.cmd_issue_stopped = MV_TRUE;
			}

			/* reset that command slot */
			prot_reset_slot(root, &device->base, ctx->slot, req);

			if (CORE_IS_EH_REQ(ctx) || CORE_IS_INIT_REQ(ctx)) {
				/* Set timeout for EH and INIT req let EH Handler handle request */
				req->Scsi_Status = REQ_STATUS_TIMEOUT; 
				core_queue_completed_req(root->core, req);
			} else {
				/* treat it as media error */
				req->Scsi_Status = REQ_STATUS_ERROR;
				core_queue_error_req(root, req, MV_TRUE);
			}
		}
	}
}

MV_VOID io_chip_handle_non_spcfc_ncq_err(pl_root *root)
{
	MV_U32 err_0, err_1;
	MV_U8 i;

	err_0 = MV_REG_READ_DWORD(root->mmio_base, COMMON_NON_SPEC_NCQ_ERR0);
	err_1 = MV_REG_READ_DWORD(root->mmio_base, COMMON_NON_SPEC_NCQ_ERR1);

	for (i=0; i<32; i++) {
		if (err_0 & MV_BIT(i)) {
			sata_handle_non_spcfc_ncq_err(root, i);
		}

		if (err_1 & MV_BIT(i)) {
			sata_handle_non_spcfc_ncq_err(root, (i+32));
		}
	}

	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_NON_SPEC_NCQ_ERR0, err_0);
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_NON_SPEC_NCQ_ERR1, err_1);
}

MV_VOID io_chip_handle_cmpl_queue_int(MV_PVOID root_p)
{
	pl_root *root = (pl_root *)root_p;
	MV_Request *req=NULL;
	core_context *ctx=NULL;
	MV_PU32 cmpl_q; /* points to the completion entry zero */
	MV_U32 cmpl_entry;
	MV_U16 cmpl_wp; /* the completion write pointer */
	MV_U16 i, slot;
	struct _domain_base *base;

	cmpl_q = root->cmpl_q;
	cmpl_wp = (MV_U16) MV_LE32_TO_CPU(*(MV_U32 *)root->cmpl_wp)&0xfff;

	i = root->last_cmpl_q; /* last_cmpl_q is handled. will start from the next. */
	root->last_cmpl_q = cmpl_wp;
	if (i == root->last_cmpl_q)	/* no new entry */
		return;

	if (root->last_cmpl_q == 0xfff) /* write pointer is not updated yet */
		return;

	while (i != root->last_cmpl_q) {
		i++;
		if (i >= root->cmpl_q_size)
			i = 0;

		cmpl_entry = MV_LE32_TO_CPU(cmpl_q[i]);
		if (cmpl_entry & RXQ_ATTN) {
			continue;
		}

		slot = (MV_U16)(cmpl_entry & 0xfff);
		if(slot >= root->slot_count_support){
			CORE_DPRINT(("finished  slot %x exceed max slot %x.\n",slot, root->slot_count_support));
			continue;
		}
		req = root->running_req[slot];
		if (req == NULL) {
			CORE_EH_PRINT(("attention: "\
				"cannot find corresponding req on slot 0x%x\n", \
				slot));
			continue;
		}
		ctx = req->Context[MODULE_CORE];

		base = (struct _domain_base *)get_device_by_id(
				root->lib_dev, req->Device_Id);
		if (base == NULL) {
			MV_ASSERT(MV_FALSE);
		}

		/*
		 * process_command:
		 * 1. if success, return REQ_STATUS_SUCCESS
		 * 2. if final error like disk is gone, return REQ_STATUS_NO_DEVICE
		 * 3. if want to do error handling,
		 *    set ctx->error_info with EH_INFO_NEED_RETRY
		 *    and also set the Scsi_Status properly to
		 *    a. REQ_STATUS_HAS_SENSE if has sense
		 *    b. for timeout, the status should be REQ_STATUS_TIMEOUT
		 *    c. REQ_STATUS_ERROR for all other error
		 */
		((command_handler *)ctx->handler)->process_command(
			root, base, &cmpl_entry, root->cmd_table_wrapper[slot].vir, req);

		MV_DASSERT((req->Scsi_Status == REQ_STATUS_SUCCESS)
			|| (req->Scsi_Status == REQ_STATUS_TIMEOUT)
			|| (req->Scsi_Status == REQ_STATUS_NO_DEVICE)
			|| (req->Scsi_Status == REQ_STATUS_HAS_SENSE)
			|| (req->Scsi_Status == REQ_STATUS_ERROR)
			|| (req->Scsi_Status == REQ_STATUS_BUSY));

		prot_process_cmpl_req(root, req);
	}
}

MV_VOID io_chip_handle_int(pl_root *root)
{
	if (root->comm_irq & INT_CMD_CMPL) {
		io_chip_handle_cmpl_queue_int(root);
	}

	if (root->comm_irq & (INT_PORT_MASK|INT_PHY_MASK)) {
		io_chip_handle_port_int(root);
	}

	if (root->comm_irq & INT_NON_SPCFC_NCQ_ERR) {
		io_chip_handle_non_spcfc_ncq_err(root);
	}

	root->comm_irq = 0;
	return ;
}

MV_U16 prot_set_up_sg_table(MV_PVOID root_p, MV_Request *req, MV_PVOID sg_wrapper_p)
{
	pl_root *root = (pl_root *)root_p;
	core_extension *core = (core_extension *)root->core;
	hw_buf_wrapper *sg_wrapper = (hw_buf_wrapper *)sg_wrapper_p;
	MV_U16 consumed = 0;

	if (req->SG_Table.Valid_Entry_Count > 0) {
		MV_DASSERT(req->Data_Transfer_Length > 0);
		MV_DASSERT(req->SG_Table.Byte_Count == req->Data_Transfer_Length);
		consumed = (MV_U16) core_prepare_hwprd(root->core,
			&req->SG_Table, sg_wrapper->vir);

		if (consumed == 0) {
			CORE_DPRINT( ("Run out of PRD entry.\n") );
			MV_ASSERT( MV_FALSE );
		}

		if (req->Cmd_Flag & CMD_FLAG_DATA_IN) {
			prd_t* prd;
			MV_PHYSICAL_ADDR dma;
			core_extension *core = (core_extension *)root->core;
			MV_U32 dw2 = 0;

			MV_ASSERT(consumed <= (core->hw_sg_entry_count - 2));

			prd = ((prd_t *)sg_wrapper->vir)+consumed;
			prd->baseAddr_low =
				MV_CPU_TO_LE32(core->trash_bucket_dma.parts.low);
			prd->baseAddr_high =
				MV_CPU_TO_LE32(core->trash_bucket_dma.parts.high);
			dw2 = TRASH_BUCKET_SIZE;
			dw2 &= ~PRD_CHAIN_BIT;			
			dw2 |= INTRFC_PCIEA<<PRD_IF_SELECT_SHIFT;

			prd->size = MV_CPU_TO_LE32(dw2);

			dma = U64_ADD_U32(sg_wrapper->phy, (consumed * sizeof(prd_t)));

			prd++;
			prd->baseAddr_low = MV_CPU_TO_LE32(dma.parts.low);
			prd->baseAddr_high = MV_CPU_TO_LE32(dma.parts.high);
			dw2 = 2;
			dw2 |= PRD_CHAIN_BIT;
			dw2 |= INTRFC_PCIEA<<PRD_IF_SELECT_SHIFT;
			prd->size = MV_CPU_TO_LE32(dw2);
			consumed += 2;
		}
	} else {
		MV_DASSERT(req->Data_Transfer_Length == 0);
	}

	return consumed;
}
MV_VOID core_fill_prd(MV_PVOID prd_ctx, MV_U64 bass_addr, MV_U32 size)
{
	prd_context *ctx = (prd_context *)prd_ctx;
	MV_U32 dw2 = 0;

	MV_ASSERT(ctx->avail);
	
	dw2 = size;
	dw2 &= ~PRD_CHAIN_BIT;
	
	ctx->prd->baseAddr_low = MV_CPU_TO_LE32(bass_addr.parts.low);
	ctx->prd->baseAddr_high = MV_CPU_TO_LE32(bass_addr.parts.high);
	dw2 |= INTRFC_PCIEA<<PRD_IF_SELECT_SHIFT;
	ctx->prd->size = MV_CPU_TO_LE32(dw2);
	ctx->prd++;
       MV_ASSERT(ctx->avail > 0);
	ctx->avail--;
	
}

int core_prepare_hwprd(MV_PVOID core, sgd_tbl_t * source, MV_PVOID prd)	
{	
	prd_context ctx;	
	
	ctx.prd = (prd_t *)prd;	
	ctx.avail = ((core_extension *)core)->hw_sg_entry_count;	
	return sgdt_prepare_hwprd((core), (source), (&ctx), 
	core_fill_prd); 
	
}

MV_VOID
core_alarm_enable_register(MV_PVOID core_p)
{
	MV_U32 reg;
	core_extension *core = (core_extension *) core_p;

	reg = MV_REG_READ_DWORD(core->mmio_base, TEST_PIN_OUTPUT_ENABLE);
	MV_REG_WRITE_DWORD(core->mmio_base, TEST_PIN_OUTPUT_ENABLE, reg | TEST_PIN_BUZZER);
}

MV_VOID
core_alarm_set_register(MV_PVOID core_p, MV_U8 value)
{
	MV_U32 reg;
	core_extension *core = (core_extension *) core_p;

	reg = MV_REG_READ_DWORD(core->mmio_base, TEST_PIN_OUTPUT_VALUE);

	if (value == MV_TRUE)
		reg |= TEST_PIN_BUZZER;
	else
		reg &= ~TEST_PIN_BUZZER;
		
	MV_REG_WRITE_DWORD(core->mmio_base, TEST_PIN_OUTPUT_VALUE, reg);
}

