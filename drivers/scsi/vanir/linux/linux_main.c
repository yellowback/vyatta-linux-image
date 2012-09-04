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
#include "linux_main.h"
#include "hba_mod.h"
#include "linux_iface.h"
#include "hba_timer.h"
#include "csmisas.h"

static const struct pci_device_id mv_pci_ids[] = {
	{PCI_DEVICE(VENDOR_ID, DEVICE_ID_9480)},
	{PCI_DEVICE(VENDOR_ID_EXT,DEVICE_ID_9480)},
	{PCI_DEVICE(VENDOR_ID_EXT, DEVICE_ID_9485)},
	{PCI_DEVICE(VENDOR_ID_EXT, DEVICE_ID_9440)},
	{PCI_DEVICE(VENDOR_ID_EXT, DEVICE_ID_9445)},
	{PCI_DEVICE(VENDOR_ID_EXT,DEVICE_ID_948F)},
	{0}
};


/*
 *  cmd line parameters
 */
static int mv_msi_enable;
module_param(mv_msi_enable, int, 0);
MODULE_PARM_DESC(mv_msi_enable, " Enable MSI Support for Marvell \
	controllers (default=0)");

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12))

/* notifier block to get notified on system shutdown/halt/reboot/down */
static int mv_linux_halt(struct notifier_block *nb, unsigned long event,
			 void *buf)
{
	switch (event) {
	case SYS_RESTART:
	case SYS_HALT:
	case SYS_POWER_OFF:
		MV_DPRINT(("%s assert!\n",__func__));
              	mv_hba_stop(NULL);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block mv_linux_notifier = {
	mv_linux_halt, NULL, 0
};
#endif /*#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12))*/


static int mv_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	unsigned int ret = PCIBIOS_SUCCESSFUL;
	int err = 0;

	ret = pci_enable_device(dev);
	if (ret) {
		MV_PRINT("%s : enable device failed.\n", mv_product_name);
		return ret;
	}

	ret = pci_request_regions(dev, mv_driver_name);
	if (ret)
		goto err_req_region;

	if ( !pci_set_dma_mask(dev, DMA_64BIT_MASK) ) {
		ret = pci_set_consistent_dma_mask(dev, DMA_64BIT_MASK);
		if (ret) {
			ret = pci_set_consistent_dma_mask(dev,
							  DMA_32BIT_MASK);
			if (ret)
				goto err_dma_mask;
		}
	} else {
		ret = pci_set_dma_mask(dev, DMA_32BIT_MASK);
		if (ret)
			goto err_dma_mask;
		ret = pci_set_consistent_dma_mask(dev, DMA_32BIT_MASK);
		if (ret)
			goto err_dma_mask;
	}

	pci_set_master(dev);

	MV_PRINT("Marvell Storage Controller is found, using IRQ %d, driver version %s.\n",
	       dev->irq, mv_version_linux);

	MV_PRINT("Marvell Linux driver %s, driver version %s.\n",
	      mv_driver_name, mv_version_linux);

	MV_DPRINT(("Start mv_hba_init.\n"));

	ret = mv_hba_init(dev, MV_MAX_IO);
	if (ret) {
		MV_DPRINT(( "Error no %d.\n", ret));
		ret = -ENOMEM;
		goto err_dma_mask;
	}

	MV_DPRINT(("Start mv_hba_start.\n"));

	if (mv_hba_start(dev)) {
		ret = -ENODEV;
		goto err_mod_start;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
		if (__mv_get_adapter_count() == 1) {
			register_reboot_notifier(&mv_linux_notifier);
		}
#endif

	MV_DPRINT(("Finished mv_probe.\n"));

	return 0;
err_mod_start:
	err++;
	mv_hba_stop(dev);
	mv_hba_release(dev);
err_dma_mask:
	err++;
	pci_release_regions(dev);
err_req_region:
	err++;
	pci_disable_device(dev);

	MV_PRINT("%s : error counter %d.\n", mv_product_name, err);
	return ret;
}

static void __devexit mv_remove(struct pci_dev *dev)
{
	mv_hba_stop(dev);
	mv_hba_release(dev);
	pci_release_regions(dev);
	pci_disable_device(dev);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12))
	if (__mv_get_adapter_count() == 0) {
		unregister_reboot_notifier(&mv_linux_notifier);
	}
#endif

	MV_PRINT("%s : Marvell %s linux driver removed !\n", mv_product_name, mv_product_name);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12))
static void mv_shutdown(struct pci_dev * pdev)
{
	mv_hba_stop(NULL);
}
#endif

void core_disable_ints(void * ext);
void core_enable_ints(void * ext);
MV_BOOLEAN core_check_int(void *ext);

static irqreturn_t mv_hba_int_handler(void *dev_id)
{
	irqreturn_t retval = MV_FALSE;
	struct hba_extension *hba = (struct hba_extension *) dev_id;

	core_disable_ints(hba->desc->child->extension);

	if (!hba->msi_enabled) {
		retval = core_check_int(hba->desc->child->extension);
		if (!retval) {
			core_enable_ints(hba->desc->child->extension);
			return IRQ_RETVAL(retval);
		}
	} else {
		retval = MV_TRUE;
	}
	tasklet_schedule(&hba->desc->hba_desc->mv_tasklet);

	return IRQ_RETVAL(retval);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
 irqreturn_t mv_intr_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	return mv_hba_int_handler(dev_id);
}
#else
 irqreturn_t mv_intr_handler(int irq, void *dev_id)
{
	return mv_hba_int_handler(dev_id);
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19) */

#ifdef CONFIG_PM
int core_suspend (void *ext);
int core_resume (void *ext);
static int mv_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct hba_extension *ext;
	struct mv_mod_desc *core_mod,*hba_mod = pci_get_drvdata(pdev);
	struct mv_adp_desc *ioc = hba_mod->hba_desc;

	BUG_ON(!ioc);
	MV_PRINT("start  mv_suspend.\n");

	ext = (struct hba_extension *)hba_mod->extension;
	core_mod = __get_lowest_module(ioc);
	core_suspend(core_mod->extension);
	free_irq(ioc->dev->irq,ext);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11))
	pci_save_state(pdev);
#else
	pci_save_state(pdev,ioc->pci_config_space);
#endif
	pci_disable_device(pdev);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11))
	pci_set_power_state(pdev,pci_choose_state(pdev,state));
#else
	pci_set_power_state(pdev,state);
#endif

	return 0;
}

static int mv_resume (struct pci_dev *pdev)
{
	int ret;
	struct hba_extension *ext;
	struct mv_mod_desc *core_mod,*hba_mod = pci_get_drvdata(pdev);
	struct mv_adp_desc *ioc = hba_mod->hba_desc;

	ext = (struct hba_extension *)hba_mod->extension;
	core_mod = __get_lowest_module(ioc);
	MV_PRINT("start  mv_resume.\n");

	pci_set_power_state(pdev, PCI_D0);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11))
	pci_enable_wake(pdev, PCI_D0, 0);
	pci_restore_state(pdev);
#else
	pci_restore_state(pdev,ioc->pci_config_space);
#endif
	pci_set_master(pdev);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19)
	ret = request_irq(ioc->dev->irq, mv_intr_handler, IRQF_SHARED,
	                  mv_driver_name, ext);
#else
	ret = request_irq(ioc->dev->irq, mv_intr_handler, SA_SHIRQ,
		mv_driver_name, ext);
#endif
	if (ret < 0) {
	        MV_PRINT("request IRQ failed.\n");
	        return -1;
	}
	if (core_resume(core_mod->extension)) {
		MV_PRINT("mv_resume_core failed.\n");
		return -1;
	}

	return 0;
}
#endif


static struct pci_driver mv_pci_driver = {
	.name     = mv_driver_name,
	.id_table = mv_pci_ids,
	.probe    = mv_probe,
	.remove   = __devexit_p(mv_remove),
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12))
	.shutdown = mv_shutdown,
#endif
#ifdef CONFIG_PM
	.resume = mv_resume,
	.suspend = mv_suspend,
#endif
};

int hba_req_cache_create(MV_PVOID hba_ext)
{
	PHBA_Extension phba = (PHBA_Extension)hba_ext;
	struct mv_adp_desc   *hba_desc=phba->desc->hba_desc;
	sprintf(phba->cache_name,"%s%d%d","mv_request_",hba_desc->Device_Id, hba_desc->id);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,22))
	phba->mv_request_cache = kmem_cache_create(phba->cache_name,sizeof(struct _MV_Request),
			0, SLAB_HWCACHE_ALIGN, NULL);
#else
	phba->mv_request_cache = kmem_cache_create(phba->cache_name,sizeof(struct _MV_Request),
			0, SLAB_HWCACHE_ALIGN, NULL,NULL);
#endif
	if(phba->mv_request_cache == NULL)
		return -ENOMEM;
	sprintf(phba->sg_name,"%s%d%d","mv_sgtable_",hba_desc->Device_Id, hba_desc->id);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,22))
	phba->mv_request_sg_cache = kmem_cache_create(phba->sg_name,
		phba->max_sg_count  * sizeof(MV_SG_Entry),
			0,SLAB_HWCACHE_ALIGN, NULL);
#else
	phba->mv_request_sg_cache = kmem_cache_create(phba->sg_name,
		phba->max_sg_count  * sizeof(MV_SG_Entry),
			0,SLAB_HWCACHE_ALIGN, NULL,NULL);
#endif
	if(phba->mv_request_sg_cache == NULL) {
		kmem_cache_destroy(phba->mv_request_cache);
		return -ENOMEM;
	}

	phba->mv_mempool= mempool_create(2, mempool_alloc_slab,mempool_free_slab,
		phba->mv_request_sg_cache);
	if(!phba->mv_mempool){
		kmem_cache_destroy(phba->mv_request_cache);
		kmem_cache_destroy(phba->mv_request_sg_cache);
		return -ENOMEM;
	}
	return 0;
}

PMV_Request hba_req_cache_alloc(MV_PVOID hba_ext)
{
	int max_sg = 0;
	MV_SG_Entry * sgtable= NULL;
	struct _MV_Request * req = NULL;
	PHBA_Extension phba = (PHBA_Extension)hba_ext;

	max_sg = phba->max_sg_count;
	req = kmem_cache_alloc(phba->mv_request_cache, GFP_ATOMIC);
	if (!req) {
		MV_DPRINT(("cache alloc req failed.\n"));
		return NULL;
	}
	memset(req,0x00,sizeof(struct _MV_Request));
	sgtable =  mempool_alloc(phba->mv_mempool, GFP_ATOMIC);

	if (sgtable) {
		memset(sgtable, 0x00,sizeof(max_sg  * sizeof(MV_SG_Entry)));
		req->SG_Table.Entry_Ptr= sgtable;
		req->SG_Table.Max_Entry_Count = max_sg;
	} else {
		kmem_cache_free(phba->mv_request_cache,req);
		return NULL;
	}
	MV_ZeroMvRequest(req);
	return req;
}

void hba_req_cache_free(MV_PVOID hba_ext,PMV_Request req)
{
	PHBA_Extension phba = (PHBA_Extension)hba_ext;
	mempool_free((void *)req->SG_Table.Entry_Ptr,phba->mv_mempool);
	kmem_cache_free(phba->mv_request_cache,req);
}

void  hba_req_cache_destroy(MV_PVOID hba_ext)
{
	PHBA_Extension phba = (PHBA_Extension)hba_ext;
	mempool_destroy(phba->mv_mempool);
	kmem_cache_destroy(phba->mv_request_cache);
	kmem_cache_destroy(phba->mv_request_sg_cache);
}

static void generate_sg_table(struct hba_extension *phba,
			      struct scsi_cmnd *scmd,
			      PMV_SG_Table sg_table)
{
	struct scatterlist *sg;
	unsigned int sg_count = 0;
	unsigned int length;
	dma_addr_t busaddr = 0;
	int i;

	if (mv_rq_bf_l(scmd) > (mv_scmd_host(scmd)->max_sectors << 9)) {
		MV_DPRINT(( "ERROR: request length exceeds "
		"the maximum alowed value.\n"));
	}

	if (0 == mv_rq_bf_l(scmd))
		return ;

	if (mv_use_sg(scmd)) {
		sg = (struct scatterlist *) mv_rq_bf(scmd);
		if (MV_SCp(scmd)->mapped == 0){
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 23)
			sg_count = scsi_dma_map(scmd);
#else
			sg_count = pci_map_sg(phba->desc->hba_desc->dev,sg,mv_use_sg(scmd),
			scsi_to_pci_dma_dir(scmd->sc_data_direction));
#endif
			if (sg_count != mv_use_sg(scmd)) {
				MV_PRINT("WARNING sg_count(%d) != scmd->use_sg(%d)\n",
				(unsigned int) sg_count, mv_use_sg(scmd));
			}
			MV_SCp(scmd)->mapped = 1;
		}

		for (i = 0; i < sg_count; i++) {
			busaddr = sg_dma_address(&sg[i]);
			length = sg_dma_len(&sg[i]);

			sgdt_append_pctx(sg_table,LO_BUSADDR(busaddr), HI_BUSADDR(busaddr),
			length,  sg + i);
		}
	} else {
		if (MV_SCp(scmd)->mapped == 0) {
			busaddr = dma_map_single(&phba->desc->hba_desc->dev->dev,mv_rq_bf(scmd),mv_rq_bf_l(scmd),
			scsi_to_pci_dma_dir(scmd->sc_data_direction));
			MV_SCp(scmd)->bus_address = busaddr;
			MV_SCp(scmd)->mapped = 1;
		}
		sgdt_append_vp(sg_table,mv_rq_bf(scmd), mv_rq_bf_l(scmd),
		LO_BUSADDR(busaddr),   HI_BUSADDR(busaddr));
	}
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#define MV_EEDPFLAGS_INC_PRI_REFTAG        (0x8000)
#define MV_EEDPFLAGS_INC_SEC_REFTAG        (0x4000)
#define MV_EEDPFLAGS_INC_PRI_APPTAG        (0x2000)
#define MV_EEDPFLAGS_INC_SEC_APPTAG        (0x1000)

#define MV_EEDPFLAGS_CHECK_REFTAG          (0x0400)
#define MV_EEDPFLAGS_CHECK_APPTAG          (0x0200)
#define MV_EEDPFLAGS_CHECK_GUARD           (0x0100)

#define MV_EEDPFLAGS_PASSTHRU_REFTAG       (0x0008)

#define MV_EEDPFLAGS_MASK_OP               (0x0007)
#define MV_EEDPFLAGS_NOOP_OP               (0x0000)
#define MV_EEDPFLAGS_CHECK_OP              (0x0001)
#define MV_EEDPFLAGS_STRIP_OP              (0x0002)
#define MV_EEDPFLAGS_CHECK_REMOVE_OP       (0x0003)
#define MV_EEDPFLAGS_INSERT_OP             (0x0004)
#define MV_EEDPFLAGS_REPLACE_OP            (0x0006)
#define MV_EEDPFLAGS_CHECK_REGEN_OP        (0x0007)
/**
 * mv_setup_eedp - setup MV request for EEDP transfer
 * @scmd: pointer to scsi command object
 * @pReq: pointer to the SCSI_IO reqest message frame
 *
 * Supporting protection type 1 and 3.
 *
 * Returns nothing
 */
static void
mv_setup_eedp(struct scsi_cmnd *scmd, PMV_Request pReq)
{
	MV_U16 eedp_flags;
	unsigned char prot_op = scsi_get_prot_op(scmd);
	unsigned char prot_type = scsi_get_prot_type(scmd);

	if (prot_type == SCSI_PROT_DIF_TYPE0 ||
	   prot_type == SCSI_PROT_DIF_TYPE2 ||
	   prot_op == SCSI_PROT_NORMAL)
		return;
	
	if (prot_op ==  SCSI_PROT_READ_STRIP)
		eedp_flags = MV_EEDPFLAGS_CHECK_REMOVE_OP;
	else if (prot_op ==  SCSI_PROT_WRITE_INSERT)
		eedp_flags = MV_EEDPFLAGS_INSERT_OP;
	else
		return;

	switch (prot_type) {
	case SCSI_PROT_DIF_TYPE1:

		/*
		* enable ref/guard checking
		* auto increment ref tag
		*/
		pReq->EEDPFlags = eedp_flags |
		    MV_EEDPFLAGS_INC_PRI_REFTAG |
		    MV_EEDPFLAGS_CHECK_REFTAG |
		    MV_EEDPFLAGS_CHECK_GUARD;
		break;

	case SCSI_PROT_DIF_TYPE3:

		/*
		* enable guard checking
		*/
		pReq->EEDPFlags = eedp_flags |
		    MV_EEDPFLAGS_CHECK_GUARD;

		break;
	}
}
/**
 * mv_eedp_error_handling - return sense code for EEDP errors
 * @scmd: pointer to scsi command object
 * @ioc_status: ioc status
 *
 * Returns nothing
 */
static void
mv_eedp_error_handling(struct scsi_cmnd *scmd, MV_U8 req_status)
{
	return;
}
#endif

void mv_complete_request(struct hba_extension *phba,
				struct scsi_cmnd *scmd,
				PMV_Request pReq)
{
	PMV_Sense_Data  senseBuffer = (PMV_Sense_Data)pReq->Sense_Info_Buffer;

	if (mv_rq_bf_l(scmd)) {
		if (MV_SCp(scmd)->mapped) {
			if (mv_use_sg(scmd)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 23)
				scsi_dma_unmap(scmd);
#else
				pci_unmap_sg(phba->desc->hba_desc->dev,
					     mv_rq_bf(scmd),
					     mv_use_sg(scmd),
					 	scsi_to_pci_dma_dir(scmd->sc_data_direction));
#endif
			} else {
				dma_unmap_single(&phba->desc->hba_desc->dev->dev,
						 MV_SCp(scmd)->bus_address,
						 mv_rq_bf_l(scmd),
				     	scsi_to_pci_dma_dir(scmd->sc_data_direction));
			}
		}
	}

	switch (pReq->Scsi_Status) {
	case REQ_STATUS_SUCCESS:
		scmd->result = 0x00;
		break;
	case REQ_STATUS_MEDIA_ERROR:
		scmd->result = (DID_BAD_TARGET << 16);
		break;
	case REQ_STATUS_BUSY:
		scmd->result = (DID_BUS_BUSY << 16);
		break;
	case REQ_STATUS_NO_DEVICE:
		scmd->result = (DID_NO_CONNECT << 16);
		break;
	case REQ_STATUS_HAS_SENSE:
		scmd->result  = (DRIVER_SENSE << 24) | (DID_OK << 16) |
			SAM_STAT_CHECK_CONDITION;

		if (scmd->cmnd[0]== 0x85 || scmd->cmnd[0]== 0xa1)
			break;

		if (((MV_PU8) senseBuffer)[0] >= 0x72) {
			MV_DPRINT(("MV Sense: response %x SK %s  "
		       		  "ASC %x ASCQ %x.\n\n", ((MV_PU8) senseBuffer)[0],
		       		MV_DumpSenseKey(((MV_PU8) senseBuffer)[1]),
		       		((MV_PU8) senseBuffer)[2],((MV_PU8) senseBuffer)[3]));
		} else {
			MV_DPRINT(("MV Sense: response %x SK %s "
		       		  "ASC %x ASCQ %x.\n\n", ((MV_PU8) senseBuffer)[0],
		       		MV_DumpSenseKey(((MV_PU8) senseBuffer)[2]),
		       		((MV_PU8) senseBuffer)[12],((MV_PU8) senseBuffer)[13]));
		}
		break;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
	case REQ_STATUS_DIF_GUARD_ERROR:
	case REQ_STATUS_DIF_REF_TAG_ERROR:
	case REQ_STATUS_DIF_APP_TAG_ERROR:
		mv_eedp_error_handling(scmd, pReq->Scsi_Status);
		break;
#endif
	default:
		scmd->result = DRIVER_INVALID << 24 | DID_ABORT << 16;
		break;
	}
	if(scmd && scmd->scsi_done) {
		scmd->scsi_done(scmd);
	} else
		MV_DPRINT(("scmd %p no scsi_done.\n",scmd));
}

static void hba_req_callback(MV_PVOID This, PMV_Request pReq)
{
	struct hba_extension *phba = (struct hba_extension *)This;
	struct scsi_cmnd *scmd = (struct scsi_cmnd *)pReq->Org_Req_Scmd;

	mv_complete_request(phba, scmd, pReq);
	phba->Io_Count--;
	hba_req_cache_free(phba,pReq);
}


static int scsi_cmd_to_req_conv(struct hba_extension *phba,
				struct scsi_cmnd *scmd,
				PMV_Request pReq)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
	mv_setup_eedp(scmd, pReq);
#endif
	/*
	 * Set three flags: CMD_FLAG_NON_DATA
	 *                  CMD_FLAG_DATA_IN
	 *                  CMD_FLAG_DMA
	 * currently data in/out all go thru DMA
	 */
	pReq->Cmd_Flag = 0;
	switch (scmd->sc_data_direction) {
	case DMA_NONE:
		break;
	case DMA_FROM_DEVICE:
		pReq->Cmd_Flag |= CMD_FLAG_DATA_IN;
	case DMA_TO_DEVICE:
		pReq->Cmd_Flag |= CMD_FLAG_DMA;
		break;
	case DMA_BIDIRECTIONAL :
		MV_DPRINT(( " unexpected DMA_BIDIRECTIONAL.\n"));
		break;
	default:
		break;
	}

	/* max CDB length set for 32 */
	memset(pReq->Cdb, 0, MAX_CDB_SIZE);
	
#if LINUX_VERSION_CODE >KERNEL_VERSION(2, 6, 27)
	pReq->Time_Out = jiffies_to_msecs(scmd->request->timeout)/1000;
#else
	#if (LINUX_VERSION_CODE ==KERNEL_VERSION(2, 6, 27) && (IS_OPENSUSE_SLED_SLES))
	pReq->Time_Out = jiffies_to_msecs(scmd->request->timeout)/1000;
	#else
	pReq->Time_Out = jiffies_to_msecs(scmd->timeout_per_command)/1000;
	#endif
#endif

	switch (scmd->cmnd[0]) {
	/* per smartctl, it sets SCSI_TIMEOUT_DEFAULT to 6 , but for captive mode, we extends to 60 HZs */
	case SCSI_CMD_ATA_PASSTHRU_16:
		if (scmd->cmnd[14] != ATA_CMD_PM_CHECK)
			pReq->Time_Out = 60;
		pReq->Cmd_Flag = hba_parse_ata_protocol(scmd);
		break;
	case SCSI_CMD_ATA_PASSTHRU_12:
		if (scmd->cmnd[9] != ATA_CMD_PM_CHECK)
			pReq->Time_Out = 60;
		pReq->Cmd_Flag = hba_parse_ata_protocol(scmd);
		break;
	default:
		break;
	}
	memcpy(pReq->Cdb, scmd->cmnd, scmd->cmd_len);

	pReq->Data_Buffer = mv_rq_bf(scmd);
	pReq->Data_Transfer_Length = mv_rq_bf_l(scmd);
	pReq->Sense_Info_Buffer = scmd->sense_buffer;
	pReq->Sense_Info_Buffer_Length = SCSI_SENSE_BUFFERSIZE;

	SGTable_Init(&pReq->SG_Table, 0);
	generate_sg_table(phba, scmd, &pReq->SG_Table);

	MV_SetLBAandSectorCount(pReq);

	pReq->Req_Type      = REQ_TYPE_OS;
	pReq->Org_Req_Scmd       = scmd;
	pReq->Tag           = scmd->tag;
	pReq->Scsi_Status   = REQ_STATUS_PENDING;
	pReq->Completion    = hba_req_callback;
	pReq->Cmd_Initiator = phba;
	pReq->Device_Id     = get_id_by_targetid_lun(phba, mv_scmd_target(scmd),
					    	mv_scmd_lun(scmd));

	return 0;
}

extern void * kbuf_array[512];
static void hba_ioctl_req_callback(MV_PVOID This, PMV_Request pReq)
{
	struct hba_extension *phba = (struct hba_extension *)This;
	struct scsi_cmnd *scmd = (struct scsi_cmnd *)pReq->Org_Req_Scmd;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
/*openSUSE 11.1 SLES 11 SLED 11*/
#if ((LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 27))&&(!IS_OPENSUSE_SLED_SLES))
        /* Return this request to OS. */
	scmd->eh_timeout.expires = jiffies + 1;
	add_timer(&scmd->eh_timeout);
#endif
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19) || LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,9)
	scmd->request->tag = pReq->Scsi_Status;
#else
	scmd->request->rq_status = pReq->Scsi_Status;
#endif
	scmd->request->sense = pReq->Sense_Info_Buffer;
	scmd->request->sense_len = pReq->Sense_Info_Buffer_Length;
	mv_complete_request(phba, scmd, pReq);
	phba->Io_Count--;
	hba_req_cache_free(phba,pReq);
}

static int scsi_ioctl_cmd_adjust(struct hba_extension *phba,
                                struct scsi_cmnd *scmd,
                                PMV_Request pReq)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
#if ((LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 27))&&(!IS_OPENSUSE_SLED_SLES))
	del_timer(&scmd->eh_timeout);
#endif
#endif
	memcpy(pReq->Cdb,mvcdb[scmd->request->errors - 1],16);
	if( __is_scsi_cmd_rcv_snd_diag(pReq->Cdb[0])) {
		if((pReq->Cdb[1] ==0x01) ||(pReq->Cdb[1]==0x10)) {
			if(pReq->Cdb[0] ==API_SCSI_CMD_RCV_DIAG_RSLT)
				pReq->Cmd_Flag = CMD_FLAG_DATA_IN;
			if(pReq->Cdb[0] ==API_SCSI_CMD_SND_DIAG  )
				pReq->Cmd_Flag &= ~CMD_FLAG_DATA_IN;
			goto bypass;
		}
	}
	pReq->Data_Buffer = kbuf_array[scmd->request->errors - 1];
bypass:
	scmd->request->errors = 0;
	pReq->Sense_Info_Buffer = scmd->request->sense;
	pReq->Sense_Info_Buffer_Length = scmd->request->sense_len;
	pReq->Req_Type = REQ_TYPE_INTERNAL;
	pReq->Org_Req = pReq;
	pReq->Completion =  hba_ioctl_req_callback;
	return 0;
}

static void hba_shutdown_req_cb(MV_PVOID this, PMV_Request req)
{
	struct hba_extension *phba = (struct hba_extension *) this;

	hba_req_cache_free(phba,req);
	phba->Io_Count--;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&phba->desc->hba_desc->hba_sync, 0);
#else
	complete(&phba->desc->hba_desc->cmpl);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
int __hba_wait_for_atomic_timeout(atomic_t *atomic, unsigned long timeout)
{
	unsigned intv = HZ/20;

	while (timeout) {
		if (0 == atomic_read(atomic))
			break;

		if (timeout < intv)
			intv = timeout;
		set_current_state(TASK_INTERRUPTIBLE);
		timeout -= (intv - schedule_timeout(intv));
	}
	return timeout;
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */

void hba_send_shutdown_req(MV_PVOID extension)
{
	unsigned long flags;
	PMV_Request pReq;
	MV_U32 timeout=100;
	struct hba_extension *phba = (struct hba_extension *)extension;

	pReq = hba_req_cache_alloc(phba);
	if (NULL == pReq) {
		MV_PRINT("%s : cannot allocate memory for req.\n",
		       mv_product_name);
		return;
	}

	while (((phba->Io_Count) || (phba->Ioctl_Io_Count)) && timeout) {
		//MV_DPRINT(("have running request, Io_Count = %d,, ioctl_count=%d, wait...\n",
		//	phba->Io_Count,phba->Ioctl_Io_Count));
		msleep(100);
		timeout--;
	}
	//WARN_ON(phba->Io_Count != 0);
	//WARN_ON(phba->Ioctl_Io_Count != 0);
	pReq->Device_Id = VIRTUAL_DEVICE_ID;
	pReq->Cmd_Initiator = phba;
	pReq->Org_Req = pReq;
	pReq->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
	pReq->Completion = hba_shutdown_req_cb;
	{
		MV_DPRINT(("Send SHUTDOWN request to CORE.\n"));
		pReq->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
		pReq->Cdb[1] = CDB_CORE_MODULE;
		pReq->Cdb[2] = CDB_CORE_SHUTDOWN;
		pReq->Req_Type = REQ_TYPE_OS;
	}

	spin_lock_irqsave(&phba->desc->hba_desc->global_lock, flags);
	phba->Io_Count++;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&phba->desc->hba_desc->hba_sync, 1);
#endif
	phba->desc->ops->module_sendrequest(phba->desc->extension, pReq);
	spin_unlock_irqrestore(&phba->desc->hba_desc->global_lock, flags);

	MV_DPRINT(("wait finished send_shutdown_req.\n"));
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	if (0 == __hba_wait_for_atomic_timeout(&phba->desc->hba_desc->hba_sync, 10 * HZ))
		goto err_shutdown_req;
#else
	if(0 == wait_for_completion_timeout(&phba->desc->hba_desc->cmpl, 10 * HZ))
		goto err_shutdown_req;
#endif
	MV_DPRINT(("finished send_shutdown_req.\n"));

	return;
err_shutdown_req:
	MV_PRINT("hba_send_shutdown_req failed.\n");
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 7)
#if ((LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 27))&& (IS_OPENSUSE_SLED_SLES))
static enum blk_eh_timer_return mv_linux_timed_out(struct scsi_cmnd *cmd)
{
	MV_BOOLEAN ret = MV_TRUE;
	return (ret)?BLK_EH_RESET_TIMER:BLK_EH_NOT_HANDLED;

}
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 28)
static enum scsi_eh_timer_return mv_linux_timed_out(struct scsi_cmnd *cmd)
{
	MV_BOOLEAN ret = MV_TRUE;
	return (ret)?EH_RESET_TIMER:EH_NOT_HANDLED;

}
#else
static enum blk_eh_timer_return mv_linux_timed_out(struct scsi_cmnd *cmd)
{
	MV_BOOLEAN ret = MV_TRUE;
	return (ret)?BLK_EH_RESET_TIMER:BLK_EH_NOT_HANDLED;

}
#endif
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 7) */

void dump_scsi_cmd(const char * prefix, struct scsi_cmnd * scmd)
{
	int i = 0;
	MV_PRINT("%s dump cdb[",prefix);
	for(i = 0; i < 16; i++)
		MV_PRINT("0x%02x ", scmd->cmnd[i]);
	MV_PRINT("]\n");
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
static int mv_linux_queue_command_lck(struct scsi_cmnd *scmd,
				  void (*done) (struct scsi_cmnd *))
#else
static int mv_linux_queue_command(struct scsi_cmnd *scmd,
				  void (*done) (struct scsi_cmnd *))
#endif	
{
	struct Scsi_Host *host = mv_scmd_host(scmd);
	struct hba_extension *hba = *((struct hba_extension * *) host->hostdata);
	PMV_Request req;
	unsigned long flags;

	if (done == NULL) {
		MV_PRINT( ": in queuecommand, done function can't be NULL\n");
		return 0;
    	}

	scmd->result = 0;
 	scmd->scsi_done = done;
	MV_SCp(scmd)->bus_address = 0;
	MV_SCp(scmd)->mapped = 0;
	MV_SCp(scmd)->map_atomic = 0;

	if (mv_scmd_channel(scmd)) {
		scmd->result = DID_BAD_TARGET << 16;
		goto done;
	}

	/*
	 * Get mv_request resource and translate the scsi_cmnd request
	 * to mv_request.
	 */
	req = hba_req_cache_alloc(hba);

	if (req == NULL) {
		return SCSI_MLQUEUE_HOST_BUSY;
	}

	if (scsi_cmd_to_req_conv(hba, scmd, req)) {
		MV_DPRINT(( "ERROR - Translation from OS Request failed.\n"));
		hba_req_callback(hba, req);
		return 0;
	}

	if(scmd->request->errors)
		scsi_ioctl_cmd_adjust(hba,scmd,req);

	spin_unlock_irq(host->host_lock);
	spin_lock_bh(&hba->desc->hba_desc->global_lock);

	hba->Io_Count++;

	MV_ASSERT(hba->State == DRIVER_STATUS_STARTED);
	
	hba->desc->ops->module_sendrequest(hba->desc->extension, req);
	
	{
		MV_PVOID core = (MV_PVOID)HBA_GetModuleExtension(hba, MODULE_CORE);
		core_push_queues(core);
	}

	spin_unlock_bh(&hba->desc->hba_desc->global_lock);
	spin_lock_irq(host->host_lock);
	return 0;
done:
        scmd->scsi_done(scmd);
        return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
static DEF_SCSI_QCMD(mv_linux_queue_command)
#endif

static int mv_linux_reset (struct scsi_cmnd *scmd)
{
	MV_PRINT("__MV__ reset handler %p.\n", scmd);
	return FAILED;
}

struct mv_lu *mv_get_avaiable_device(struct hba_extension *hba, MV_U16  target_id, MV_U16 lun)
{
	MV_U16 id=0;
	struct mv_lu * lu=NULL;
	for (id =0; id < MV_MAX_TARGET_NUMBER; id++) {
		lu = &hba->mv_unit[id];
		if (lu && (lu->sdev == NULL) && (lu->lun == 0xFFFF)&& (lu->target_id == 0xFFFF)) {
			return lu;
		}
	}
	MV_PRINT("invalid target id %d, lun %d.\n",target_id, lun);
	return NULL;
}

static int mv_scsi_slave_alloc(struct scsi_device *sdev)
{
	struct hba_extension *hba = *((struct hba_extension * *) sdev->host->hostdata);
	struct mv_lu * lu = mv_get_avaiable_device (hba, sdev->id, sdev->lun);
	MV_U16 base_id;
	if(lu == NULL)
		return -1;

	lu->sdev = sdev;
	lu->lun = sdev->lun;
	lu->target_id = sdev->id;
	base_id = get_id_by_targetid_lun(hba, sdev->id, sdev->lun);
	if (base_id == 0xFFFF) {
		MV_DPRINT(("device %d-%d is not exist.\n", sdev->id, sdev->lun));
		return -1;
	}
	sdev->scsi_level=SCSI_SPC_2;
	return 0;
}

struct mv_lu *mv_get_device_by_target_lun(struct hba_extension *hba, MV_U16  target_id, MV_U16 lun)
{
	MV_U16 id=0;
	struct mv_lu * lu=NULL;
	for (id =0; id < MV_MAX_TARGET_NUMBER; id++) {
		lu = &hba->mv_unit[id];
		if (lu && lu->sdev && (lu->lun == lun) && (lu->target_id == target_id)) {
			return lu;
		}
	}
	return NULL;
}

static void mv_scsi_slave_destroy(struct scsi_device *sdev)
{
	struct hba_extension *hba = *((struct hba_extension * *) sdev->host->hostdata);
	struct mv_lu *lu = mv_get_device_by_target_lun (hba, sdev->id, sdev->lun);
	if(lu == NULL)
		return;
	lu->sdev = NULL;
	lu->lun = 0xFFFF;
	lu->target_id = 0xFFFF;
	return;
}

static void hba_send_ioctl_cb(MV_PVOID this, PMV_Request req)
{
	struct hba_extension *phba = (struct hba_extension *) this;

	hba_req_cache_free(phba,req);
	phba->Io_Count--;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&phba->desc->hba_desc->hba_sync, 0);
#else
	complete(&phba->desc->hba_desc->cmpl);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
}

void hba_send_internal_ioctl(struct scsi_device *sdev, MV_PVOID extension, MV_PVOID buffer, MV_U8 cdb1)
{
	unsigned long flags;
	PMV_Request pReq;
	struct hba_extension *phba = (struct hba_extension *)extension;

	pReq = hba_req_cache_alloc(phba);
	if (NULL == pReq) {
		MV_PRINT("%s : cannot allocate memory for req.\n",
		       mv_product_name);
		return;
	}

	if ((phba->Io_Count) || (phba->Ioctl_Io_Count)) {
		msleep(100);
	}
	WARN_ON(phba->Ioctl_Io_Count != 0);
	pReq->Device_Id = VIRTUAL_DEVICE_ID;
	pReq->Cmd_Initiator = phba;
	pReq->Org_Req = pReq;
	pReq->Data_Buffer = buffer;
	pReq->Data_Transfer_Length = sizeof(OS_disk_info);
	pReq->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
	pReq->Completion = hba_send_ioctl_cb;
	pReq->Cdb[0] = APICDB0_IOCONTROL;
	pReq->Cdb[1] = cdb1;
	pReq->Cdb[2] = sdev->id;
	pReq->Cdb[3] = sdev->lun;
	pReq->Req_Type = REQ_TYPE_INTERNAL;

	init_completion(&phba->desc->hba_desc->cmpl);
	spin_lock_irqsave(&phba->desc->hba_desc->global_lock, flags);
	phba->Io_Count++;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&phba->desc->hba_desc->hba_sync, 1);
#endif
	phba->desc->ops->module_sendrequest(phba->desc->extension, pReq);
	spin_unlock_irqrestore(&phba->desc->hba_desc->global_lock, flags);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	if (0 == __hba_wait_for_atomic_timeout(&phba->desc->hba_desc->hba_sync, 10 * HZ))
		goto err_get_hdinfo_req;
#else
	if(0 == wait_for_completion_timeout(&phba->desc->hba_desc->cmpl, 10 * HZ))
		goto err_get_hdinfo_req;
#endif
	MV_DPRINT(("finished io control.\n"));
	return;

err_get_hdinfo_req:
	MV_PRINT("io control req failed.\n");
}

static int mv_scsi_slave_configure(struct scsi_device *sdev)
{
	struct hba_extension *hba = *((struct hba_extension * *) sdev->host->hostdata);
	OS_disk_info disk_info;
	int dev_qth = 1;

	MV_ZeroMemory(&disk_info, sizeof(OS_disk_info));
	if (hba->RunAsNonRAID) {
		hba_send_internal_ioctl(sdev, hba, &disk_info, APICDB1_GET_OS_DISK_INFO);
		if (disk_info.queue_depth)
			dev_qth = disk_info.queue_depth;

		if (disk_info.disk_type != DISK_TYPE_SATA)
			sdev->allow_restart = 1;
	} else
		dev_qth = MV_MAX_REQUEST_PER_LUN;

	if (sdev->tagged_supported)
		scsi_adjust_queue_depth(sdev, scsi_get_tag_type(sdev),
					dev_qth);
	else {
		scsi_adjust_queue_depth(sdev, 0, 1);
		dev_qth = 1;
	}
	
	return 0;
}

#if  LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 10)

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32))
static int mv_change_queue_depth(struct scsi_device *sdev, int new_depth, int reason)
#else
static int mv_change_queue_depth(struct scsi_device *sdev, int new_depth)
#endif
{
	struct hba_extension *hba = *((struct hba_extension * *) sdev->host->hostdata);
	OS_disk_info disk_info;
	int res = 0, dev_qth = 1;

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32))
	if (reason != SCSI_QDEPTH_DEFAULT)
		return -EOPNOTSUPP;
#endif

	MV_ZeroMemory(&disk_info, sizeof(OS_disk_info));
	if (hba->RunAsNonRAID) {
		hba_send_internal_ioctl(sdev, hba, &disk_info, APICDB1_GET_OS_DISK_INFO);
		if (disk_info.queue_depth)
			dev_qth = disk_info.queue_depth;
	} else
		dev_qth = MV_MAX_REQUEST_PER_LUN;
	if(new_depth > MAX_REQUEST_PER_LUN_PERFORMANCE)
		return dev_qth;
	res=new_depth;
	if (sdev->tagged_supported) {
		scsi_adjust_queue_depth(sdev, scsi_get_tag_type(sdev), res);
	} else {
		scsi_adjust_queue_depth(sdev, 0, 1);
		res = 1;
	}

	return res;
}

static int mv_change_queue_type(struct scsi_device *scsi_dev, int qt)
{
	if (!scsi_dev->tagged_supported)
		return 0;

	scsi_deactivate_tcq(scsi_dev, 1);

	scsi_set_tag_type(scsi_dev, qt);
	scsi_activate_tcq(scsi_dev, scsi_dev->queue_depth);

	return qt;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
struct class_device_attribute *mvs_host_attrs[];
#else
struct device_attribute *mvs_host_attrs[];
#endif

static struct scsi_host_template mv_driver_template = {
	.module                      =  THIS_MODULE,
	.name                        =  "Marvell Storage Controller",
	.proc_name                   =  mv_driver_name,
	.proc_info                   =  mv_linux_proc_info,
	.queuecommand                =  mv_linux_queue_command,
	.eh_host_reset_handler       =  mv_linux_reset,
	.slave_alloc                = mv_scsi_slave_alloc,
	.slave_configure         = mv_scsi_slave_configure,
	.slave_destroy            = mv_scsi_slave_destroy,
#if  LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 10)
	.change_queue_depth	= mv_change_queue_depth,
	.change_queue_type 	= mv_change_queue_type,
#endif
#if  LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 7) && \
	LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 16)
	.eh_timed_out                =  mv_linux_timed_out,
#endif
	.can_queue                   =  MV_MAX_REQUEST_DEPTH,
	.this_id                     =  MV_SHT_THIS_ID,
	.max_sectors                 =  (MV_MAX_TRANSFER_SIZE >> 9),
	.sg_tablesize                =  MV_MAX_SG_ENTRY,
	.cmd_per_lun                 =  MV_MAX_REQUEST_PER_LUN,
	.use_clustering              =  MV_SHT_USE_CLUSTERING,
	.emulated                    =  MV_SHT_EMULATED,
	.ioctl                       =  mv_new_ioctl,
	.shost_attrs		= mvs_host_attrs,
};

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 15)
static struct scsi_transport_template mv_transport_template = {
	.eh_timed_out   =  mv_linux_timed_out,
};
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 16) */

void HBA_ModuleStart(MV_PVOID extension)
{
	struct Scsi_Host *host = NULL;
	struct hba_extension *hba;
	struct mv_adp_desc *hba_desc;
	int ret;

	hba = (struct hba_extension *) extension;
	hba_desc = hba->desc->hba_desc;
	host = scsi_host_alloc(&mv_driver_template, sizeof(void *));
	if (NULL == host) {
		MV_PRINT("%s : Unable to allocate a scsi host.\n",
		       mv_product_name);
		goto err_out;
	}

	*((MV_PVOID *) host->hostdata) = extension;
	hba_desc->hba_host = host;
	host->irq          = hba_desc->dev->irq;
	host->max_id       = MV_MAX_TARGET_NUMBER;
	host->max_lun      = MV_MAX_LUN_NUMBER;
	host->max_channel  = 0;
	host->max_cmd_len  = 16;
	
	if ((hba->Max_Io) && (hba->Max_Io < MV_MAX_REQUEST_DEPTH)){
		host->can_queue = 1;
	}

	MV_DPRINT(("HBA queue command = %d.\n", host->can_queue));

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 15)
	host->transportt   = &mv_transport_template;
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 16) */

	hba->msi_enabled = mv_msi_enable;

	if (hba->msi_enabled)
		pci_enable_msi(hba->desc->hba_desc->dev);

	MV_DPRINT(( "start install request_irq.\n"));
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19)
	ret = request_irq(hba_desc->dev->irq, mv_intr_handler, IRQF_SHARED,
			  mv_driver_name, hba);
#else
	ret = request_irq(hba_desc->dev->irq, mv_intr_handler, SA_SHIRQ,
			  mv_driver_name, hba);
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19) */
	if (ret < 0) {
		MV_PRINT("%s : Error upon requesting IRQ %d.\n",
		       mv_product_name, hba_desc->dev->irq);
		goto  err_request_irq;
	}
	MV_DPRINT(("request_irq has been installed.\n"));

	return ;

err_request_irq:
	scsi_host_put(host);
	hba_desc->hba_host = NULL;
err_out:
	return;
}

#if   LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static ssize_t
mvs_show_driver_version(struct class_device *cdev,  char *buffer)
{
#else
static ssize_t
mvs_show_driver_version(struct device *cdev, struct device_attribute *attr,  char *buffer)
{
#endif
	return snprintf(buffer, PAGE_SIZE, "%s\n", mv_version_linux);
}

#if  LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static CLASS_DEVICE_ATTR(driver_version,
			 S_IRUGO,
			 mvs_show_driver_version,
			 NULL);
#else
static DEVICE_ATTR(driver_version,
			 S_IRUGO,
			 mvs_show_driver_version,
			 NULL);
#endif

#if   LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static ssize_t mvs_show_host_can_queue(struct class_device *cdev, char *buffer)
{
#else
static ssize_t mvs_show_host_can_queue(struct device *cdev, struct device_attribute *attr, char *buffer)
{
#endif
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct hba_extension *hba = *((struct hba_extension * *) shost->hostdata);
	struct mv_adp_desc *hba_desc;
	hba_desc = hba->desc->hba_desc;
	return snprintf(buffer, PAGE_SIZE, "%d\n", hba_desc->hba_host->can_queue);
}

#if   LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static ssize_t
mvs_store_host_can_queue(struct class_device *cdev,  const char *buffer, size_t size)
{
#else
static ssize_t
mvs_store_host_can_queue(struct device *cdev, struct device_attribute *attr, const char *buffer, size_t size)
{
#endif
	struct Scsi_Host *shost = class_to_shost(cdev);
	struct hba_extension *hba = *((struct hba_extension * *) shost->hostdata);
	struct mv_adp_desc *hba_desc;
	int val = 0;

	hba_desc = hba->desc->hba_desc;
	if (buffer == NULL)
		return size;

	if (sscanf(buffer, "%d", &val) != 1)
		return -EINVAL;

	if(val > MAX_REQUEST_NUMBER_PERFORMANCE - 2){
		printk( "can_queue %d exceeds max vaule:%d\n",  val, MV_MAX_REQUEST_DEPTH);
		hba_desc->hba_host->can_queue = MV_MAX_REQUEST_DEPTH;
		return strlen(buffer);
	} else if (val < 1){
		printk( "can_queue legal value is >= 1\n");
		hba_desc->hba_host->can_queue = 1;
		return strlen(buffer);
	} else
		hba_desc->hba_host->can_queue = val;
	printk( "set host can queue to %d \n",  hba_desc->hba_host->can_queue);
	return strlen(buffer);
}

#if  LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static CLASS_DEVICE_ATTR(host_can_queue,
			 S_IRUGO|S_IWUSR,
			 mvs_show_host_can_queue,
			 mvs_store_host_can_queue);
#else
static DEVICE_ATTR(host_can_queue, S_IRUGO|S_IWUSR,
			 mvs_show_host_can_queue,
			 mvs_store_host_can_queue);
#endif

MV_U16 mv_debug_mode = 0; /*CORE_FULL_DEBUG_INFO;*/
#if   LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static ssize_t mvs_show_debug_mode(struct class_device *cdev, char *buffer)
{
#else
static ssize_t mvs_show_debug_mode(struct device *cdev, struct device_attribute *attr, char *buffer)
{
#endif
	return snprintf(buffer, PAGE_SIZE, "0x%x\n", mv_debug_mode);
}

#if   LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static ssize_t
mvs_store_debug_mode(struct class_device *cdev,  const char *buffer, size_t size)
{
#else
static ssize_t
mvs_store_debug_mode(struct device *cdev, struct device_attribute *attr, const char *buffer, size_t size)
{
#endif

	int val = 0;

	if (buffer == NULL)
		return size;

	if (sscanf(buffer, "0x%x", &val) != 1) {
		printk( "Input invalid debug mode, please input hexadecimal number:0x0~0xf.\n");
		return -EINVAL;
	}

	mv_debug_mode = val;
	if(mv_debug_mode > 0xF){
		printk( "Invalid debug mode, close all debug info!\n");
		mv_debug_mode = 0x0;
		return strlen(buffer);
	} else
		printk( "set debug mode to 0x%x\n",  mv_debug_mode);
	return strlen(buffer);
}

#if  LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
static CLASS_DEVICE_ATTR(debug_mode,
			 S_IRUGO|S_IWUSR,
			 mvs_show_debug_mode,
			 mvs_store_debug_mode);
#else
static DEVICE_ATTR(debug_mode, S_IRUGO|S_IWUSR,
			 mvs_show_debug_mode,
			 mvs_store_debug_mode);
#endif

#if  LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
struct class_device_attribute *mvs_host_attrs[] = {
	&class_device_attr_driver_version,
	&class_device_attr_host_can_queue,
	&class_device_attr_debug_mode,
	NULL,
};
#else
struct device_attribute *mvs_host_attrs[] = {
	&dev_attr_driver_version,
	&dev_attr_host_can_queue,
	&dev_attr_debug_mode,
	NULL,
};
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
unsigned int mv_prot_mask =  SHOST_DIF_TYPE1_PROTECTION |SHOST_DIF_TYPE3_PROTECTION;
module_param(mv_prot_mask, uint, 0);
MODULE_PARM_DESC(mv_prot_mask, "host protection mask");
#endif

static int __init sas_hba_init(void)
{
	hba_house_keeper_init();
	return pci_register_driver(&mv_pci_driver);
}

static void __exit sas_hba_exit(void)
{
	pci_unregister_driver(&mv_pci_driver);
	MV_DPRINT(("sas_hba_exit: before hba_house_keeper_exit\n"));
	hba_house_keeper_exit();
}

MODULE_AUTHOR ("Marvell Technolog Group Ltd.");
MODULE_DESCRIPTION ("Marvell SAS hba driver");

MODULE_LICENSE("GPL");

MODULE_VERSION(mv_version_linux);
MODULE_DEVICE_TABLE(pci, mv_pci_ids);
module_init(sas_hba_init);
module_exit(sas_hba_exit);

