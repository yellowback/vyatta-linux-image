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
#include "hba_header.h"
#include "linux_main.h"
#include "linux_iface.h"
#include "hba_mod.h"
#include "hba_timer.h"
#include "hba_api.h"

static MV_LIST_HEAD(mv_online_adapter_list);

int __mv_get_adapter_count(void)
{
	struct mv_adp_desc *p;
	int i = 0;
	LIST_FOR_EACH_ENTRY(p, &mv_online_adapter_list, hba_entry)
	i++;

	return i;
}

struct mv_adp_desc *__dev_to_desc(struct pci_dev *dev)
{
	struct mv_adp_desc *p;

	LIST_FOR_EACH_ENTRY(p, &mv_online_adapter_list, hba_entry)
	if (p->dev == dev)
		return p;
	return NULL;
}

MV_PVOID *mv_get_hba_extension(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *p;

	LIST_FOR_EACH_ENTRY(p, &hba_desc->online_module_list, mod_entry)
		if (MODULE_HBA == p->module_id)
			return p->extension;
	return NULL;
}

static inline void __mv_release_hba(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *mod_desc, *p;

	LIST_FOR_EACH_ENTRY_SAFE(mod_desc,
				p,
				&hba_desc->online_module_list,
				mod_entry) {
		List_Del(&mod_desc->mod_entry);
		hba_mem_free(mod_desc,sizeof(struct mv_mod_desc),MV_FALSE);
	}

	List_Del(&hba_desc->hba_entry);
	hba_mem_free(hba_desc,sizeof(struct mv_adp_desc),MV_FALSE);
}

static struct mv_adp_desc *mv_hba_init_modmm(struct pci_dev *dev)
{
	struct mv_adp_desc *hba_desc;

	hba_desc = hba_mem_alloc(sizeof(struct mv_adp_desc),MV_FALSE);
	if (NULL == hba_desc) {
		MV_PRINT("Unable to get memory at hba init.\n");
		return NULL;
	}
	memset(hba_desc, 0, sizeof(struct mv_adp_desc));
	hba_desc->dev = dev;
	MV_LIST_HEAD_INIT(&hba_desc->online_module_list);
	List_Add(&hba_desc->hba_entry, &mv_online_adapter_list);

	return hba_desc;
}

static void mv_hba_release_modmm(struct pci_dev *dev)
{
	struct mv_adp_desc *hba_desc;

	hba_desc = __dev_to_desc(dev);

	if (hba_desc)
		__mv_release_hba(hba_desc);
	else
		MV_PRINT("Weired! dev %p unassociated with any desc.\n", dev);
}

static inline struct mv_mod_desc *__alloc_mod_desc(void)
{
	struct mv_mod_desc *mod_desc;

	mod_desc = hba_mem_alloc(sizeof(struct mv_mod_desc),MV_FALSE);
	if (mod_desc)
		memset(mod_desc, 0, sizeof(struct mv_mod_desc));
	return mod_desc;
}

struct mv_module_ops *mv_hba_register_module(void)
{
	static struct mv_module_ops hba_module_interface = {
		.module_id              = MODULE_HBA,
		.get_res_desc           = HBA_ModuleGetResourceQuota,
		.module_initialize      = HBA_ModuleInitialize,
		.module_start           = HBA_ModuleStart,
		.module_stop            = HBA_ModuleShutdown,
		.module_notification    = HBA_ModuleNotification,
		.module_sendrequest     = HBA_ModuleSendRequest,
	};

	return &hba_module_interface;
}


static int register_online_modules(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *mod_desc, *prev;
	struct mv_module_ops *ops;

	/*
	 * iterate through online_module_list manually , from the lowest(CORE)
	 * to the highest layer (HBA)
	 */
	hba_desc->running_mod_num = 0;

	ops = mv_core_register_module();
	if (NULL == ops) {
		MV_PRINT("No core no life.\n");
		return -1;
	}
	mod_desc = __alloc_mod_desc();
	if (NULL == mod_desc)
		goto disaster;

	mod_desc->hba_desc	= hba_desc;
	mod_desc->ops		= ops;
	mod_desc->status	= MV_MOD_REGISTERED;
	mod_desc->module_id = MODULE_CORE;
	mod_desc->child 	= NULL;
	List_Add(&mod_desc->mod_entry, &hba_desc->online_module_list);
	hba_desc->running_mod_num++;

	prev = mod_desc;
	mod_desc = __alloc_mod_desc();
	if (NULL == mod_desc)
		goto disaster;

	mod_desc->ops = mv_hba_register_module();
	if (NULL == mod_desc->ops) {
		MV_PRINT("No HBA no life.\n");
		return -1;
	}

	mod_desc->hba_desc	= hba_desc;
	mod_desc->status	= MV_MOD_REGISTERED;
	mod_desc->module_id = MODULE_HBA;
	mod_desc->child 	= prev;
	mod_desc->parent	= NULL;
	prev->parent		= mod_desc;
	List_Add(&mod_desc->mod_entry, &hba_desc->online_module_list);
	hba_desc->running_mod_num++;

	return 0;
disaster:
	return -1;

}


static void __release_consistent_mem(struct mv_mod_res *mod_res,
				     struct pci_dev *dev)
{
	dma_addr_t       dma_addr;
	MV_PHYSICAL_ADDR phy_addr;

	phy_addr = mod_res->bus_addr;
	dma_addr = (dma_addr_t) (phy_addr.parts.low |
				 ((u64) phy_addr.parts.high << 32));
	pci_free_consistent(dev,
			    mod_res->size,
			    mod_res->virt_addr,
			    dma_addr);
}

int __alloc_consistent_mem(struct mv_mod_res *mod_res,
				  struct pci_dev *dev)
{
	unsigned long size;
	dma_addr_t    dma_addr;
	BUS_ADDRESS   bus_addr;
	MV_PHYSICAL_ADDR phy_addr;

	size = mod_res->size;
	size = ROUNDING(size, 8);
	mod_res->virt_addr = (MV_PVOID) pci_alloc_consistent(dev,
							     size,
							     &dma_addr);
	if (NULL == mod_res->virt_addr) {
		MV_DPRINT(("unable to alloc 0x%lx consistent mem.\n",
		       size));
		return -1;
	}
	memset(mod_res->virt_addr, 0, size);
	bus_addr            = (BUS_ADDRESS) dma_addr;
	phy_addr.parts.low  = LO_BUSADDR(bus_addr);
	phy_addr.parts.high = HI_BUSADDR(bus_addr);
	mod_res->bus_addr   = phy_addr;

	return 0;
}



static void __release_resource(struct mv_adp_desc *hba_desc,
			       struct mv_mod_desc *mod_desc)
{
	struct mv_mod_res *mod_res, *tmp;

	LIST_FOR_EACH_ENTRY_SAFE(mod_res,
				tmp,
				&mod_desc->res_list,
				res_entry) {
		switch (mod_res->type) {
		case RESOURCE_UNCACHED_MEMORY :
			__release_consistent_mem(mod_res, hba_desc->dev);
			break;
		case RESOURCE_CACHED_MEMORY :
			hba_mem_free(mod_res->virt_addr,mod_res->size,MV_FALSE);
			break;
		default:
			MV_DPRINT(("res type %d unknown.\n",
			       mod_res->type));
			break;
		}
		List_Del(&mod_res->res_entry);
		hba_mem_free(mod_res,sizeof(struct mv_mod_res),MV_FALSE);
	}
}

static void __release_module_resource(struct mv_mod_desc *mod_desc)
{
	__release_resource(mod_desc->hba_desc, mod_desc);
}

static int __alloc_module_resource(struct mv_mod_desc *mod_desc,
				   unsigned int max_io)
{
	struct mv_mod_res *mod_res = NULL;
	unsigned int size = 0;

	/*
	 * alloc only cached mem at this stage, uncached mem will be alloc'ed
	 * during mod init.
	 */
	MV_LIST_HEAD_INIT(&mod_desc->res_list);
	mod_res = hba_mem_alloc(sizeof(struct mv_mod_res),MV_FALSE);
	if (NULL == mod_res)
		return -1;
	memset(mod_res, 0, sizeof(sizeof(struct mv_mod_res)));
	mod_desc->res_entry = 1;

	size = mod_desc->ops->get_res_desc(RESOURCE_CACHED_MEMORY, max_io);
	if (size) {
		mod_res->virt_addr = hba_mem_alloc(size,MV_FALSE);
		if (NULL == mod_res->virt_addr) {
			hba_mem_free(mod_res,sizeof(struct mv_mod_res),MV_FALSE);
			MV_DASSERT(MV_FALSE);
			return -1;
		}
		memset(mod_res->virt_addr, 0, size);
		mod_res->type                = RESOURCE_CACHED_MEMORY;
		mod_res->size                = size;
		mod_desc->extension          = mod_res->virt_addr;
		mod_desc->extension_size     = size;
		List_Add(&mod_res->res_entry, &mod_desc->res_list);
	}
	MV_DPRINT(("show module id[%d] cached size[0x%x], addr[0x%p].\n",mod_desc->module_id,size,mod_res->virt_addr));

	return 0;
}

static void mv_release_module_resource(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *mod_desc;

	LIST_FOR_EACH_ENTRY(mod_desc, &hba_desc->online_module_list,
			    mod_entry) {
		if (mod_desc->status == MV_MOD_INITED) {
			__release_module_resource(mod_desc);
			mod_desc->status = MV_MOD_REGISTERED;
		}
	}
}

static int mv_alloc_module_resource(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *mod_desc;
	int ret;

	LIST_FOR_EACH_ENTRY(mod_desc, &hba_desc->online_module_list,
			    mod_entry) {
		ret = __alloc_module_resource(mod_desc, hba_desc->max_io);
		if (ret)
			goto err_out;
		mod_desc->status = MV_MOD_INITED;
		__ext_to_gen(mod_desc->extension)->desc = mod_desc;
	}
	return 0;

err_out:
	MV_DPRINT(("error %d allocating resource for mod %d.\n",
	       ret, mod_desc->module_id));
	LIST_FOR_EACH_ENTRY(mod_desc, &hba_desc->online_module_list,
			    mod_entry) {
		if (mod_desc->status == MV_MOD_INITED) {
			__release_module_resource(mod_desc);
			mod_desc->status = MV_MOD_REGISTERED;
		}
	}
	return -1;
}

struct mv_mod_desc * __get_lowest_module(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *p;

	p = LIST_ENTRY(hba_desc->online_module_list.next,
		       struct mv_mod_desc,
		       mod_entry);

	WARN_ON(NULL == p);
	while (p) {
		if (NULL == p->child)
			break;
		p = p->child;
	}
	return p;
}

struct mv_mod_desc * __get_highest_module(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *p;

	p = LIST_ENTRY(hba_desc->online_module_list.next,
		       struct mv_mod_desc,
		       mod_entry);

	WARN_ON(NULL == p);
	while (p) {
		if (NULL == p->parent)
			break;
		p = p->parent;
	}
	return p;
}

static void __map_pci_addr(struct pci_dev *dev, MV_PVOID *addr_array)
{
	int i;
	resource_size_t addr;
	resource_size_t range;

	for (i = 0; i < MAX_BASE_ADDRESS; i++) {
		addr  = pci_resource_start(dev, i);
		range = pci_resource_len(dev, i);

		if (pci_resource_flags(dev, i) & IORESOURCE_MEM){
			addr_array[i] =(MV_PVOID) ioremap(addr, (unsigned long)range);
		}
		else{
			addr_array[i] = (MV_PVOID)((unsigned long)addr);
		}
		MV_DPRINT(( "%s : BAR %d : %p.\n", mv_product_name,
		       i, addr_array[i]));
	}
}

static void __unmap_pci_addr(struct pci_dev *dev, MV_PVOID *addr_array)
{
	int i;

	for (i = 0; i < MAX_BASE_ADDRESS; i++)
		if (pci_resource_flags(dev, i) & IORESOURCE_MEM)
                        iounmap(addr_array[i]);
}

int __mv_is_mod_all_started(struct mv_adp_desc *adp_desc)
{
	struct mv_mod_desc *mod_desc;

	mod_desc = __get_lowest_module(adp_desc);

	while (mod_desc) {
		if (MV_MOD_STARTED != mod_desc->status)
			return 0;

		mod_desc = mod_desc->parent;
	}
	return 1;
}

static void __mv_save_hba_configuration(struct mv_adp_desc *hba_desc, void *hba_ext)
{
	u8 i;
	PHBA_Extension phba = (PHBA_Extension)hba_ext;
	phba->Vendor_Id = hba_desc->Vendor_Id;
	phba->Device_Id = hba_desc->Device_Id ;
	phba->Revision_Id = hba_desc->Revision_Id;
	phba->Sub_Vendor_Id = hba_desc->Sub_Vendor_Id;
	phba->Sub_System_Id = hba_desc->Sub_System_Id;
	phba->pcie_max_lnk_spd = hba_desc->pcie_max_lnk_spd;
	phba->pcie_max_bus_wdth = hba_desc->pcie_max_bus_wdth;
	phba->pcie_neg_lnk_spd = hba_desc->pcie_neg_lnk_spd;
	phba->pcie_neg_bus_wdth = hba_desc->pcie_neg_bus_wdth;
	for (i = 0;i < MAX_BASE_ADDRESS; i++)
		phba->Base_Address[i] = hba_desc->Base_Address[i];

	MV_DPRINT(( "HBA device id 0x%x, RunAsNonRAID:%x.\n", phba->Device_Id, phba->RunAsNonRAID));

}

static void __hba_module_stop(struct mv_adp_desc *hba_desc)
{
	struct mv_mod_desc *mod_desc;

	mod_desc = __get_highest_module(hba_desc);
	if (NULL == mod_desc)
		return;

	while (mod_desc) {
		if (MV_MOD_STARTED == mod_desc->status) {
			mod_desc->ops->module_stop(mod_desc->extension);
			mod_desc->status = MV_MOD_INITED;
		}
		mod_desc = mod_desc->child;
	}
}

struct hba_extension *__mv_get_ext_from_adp_id(int id)
{
	struct mv_adp_desc *p;

	LIST_FOR_EACH_ENTRY(p, &mv_online_adapter_list, hba_entry)
		if (p->id == id)
			return __get_highest_module(p)->extension;

	return NULL;
}

extern MV_U8 enable_spin_up(MV_PVOID hba);

int mv_hba_start(struct pci_dev *dev)
{
	struct mv_adp_desc *hba_desc;
	struct mv_mod_desc *mod_desc;
	struct hba_extension *hba;
	unsigned long flags;
	hba_desc = __dev_to_desc(dev);

	if(NULL == (mod_desc = __get_highest_module(hba_desc)))
		return -1;

	mod_desc->ops->module_start(mod_desc->extension);
	hba = (struct hba_extension *)mod_desc->extension;
	if(hba_desc->hba_host == NULL){
		MV_DPRINT(("Start highest module failed.\n"));
		return	-1;
	}

	HBA_GetNextModuleSendFunction(hba, &hba->pNextExtension, &hba->pNextFunction);
	RAID_Feature_SetSendFunction(hba->p_raid_feature, hba, hba->pNextExtension, hba->pNextFunction);

	ossw_add_timer(&hba->desc->hba_desc->hba_timer,
		TIMER_INTERVAL_OS, (void (*)(unsigned long))Timer_CheckRequest,(unsigned long)hba);

	mod_desc = __get_lowest_module(hba_desc);
	if (NULL == mod_desc)
		return -1;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&hba_desc->hba_sync, 1);
#endif

	mod_desc->ops->module_start(mod_desc->extension);

	hba->desc->status = MV_MOD_STARTED;
	HBA_ModuleStarted(hba);
	hba_house_keeper_run();

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	if (0 == __hba_wait_for_atomic_timeout(&hba_desc->hba_sync, 1000 * HZ))
		goto err_wait_cmpl;
#else
	if (0 == wait_for_completion_timeout(&hba_desc->cmpl, 1000 * HZ))
		goto err_wait_cmpl;
#endif

	if (scsi_add_host(hba_desc->hba_host, &hba_desc->dev->dev))
		goto err_wait_cmpl;
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
	if (mv_prot_mask) {
			scsi_host_set_prot(hba_desc->hba_host, mv_prot_mask);
	}
#endif

	if(!enable_spin_up(hba)) {
		MV_PRINT("Start scsi_scan_host.\n");
		scsi_scan_host(hba_desc->hba_host);
		hba->first_scan = 0;
	}

	MV_DPRINT(("Finished Driver Initialization.\n"));

	return 0;

err_wait_cmpl:
	MV_PRINT("Timeout waiting for module start.\n");
	free_irq(hba_desc->dev->irq, hba);

	return -1;
}

void mv_hba_stop(struct pci_dev *dev)
{
	struct mv_adp_desc *hba_desc;
	MV_DPRINT(("mv_hba_stop: before hba_house_keeper_exit\n"));
	hba_house_keeper_exit();

	if (dev) {
		hba_desc = __dev_to_desc(dev);
		__hba_module_stop(hba_desc);
	} else {
		list_for_each_entry(hba_desc, &mv_online_adapter_list, hba_entry)
			__hba_module_stop(hba_desc);
	}
}

void mv_hba_release(struct pci_dev *dev)
{
	struct mv_adp_desc *hba_desc;

	hba_desc = __dev_to_desc(dev);
	MV_DPRINT(("mv_hba_release\n"));
	if (hba_desc) {
		__unmap_pci_addr(hba_desc->dev, hba_desc->Base_Address);
		mv_release_module_resource(hba_desc);
		mv_hba_release_modmm(hba_desc->dev);
	}
}

int mv_hba_init(struct pci_dev *dev, MV_U32 max_io)
{
	struct mv_adp_desc *hba_desc;
	struct mv_mod_desc *mod_desc;
	PHBA_Extension phba = NULL ;
	MV_U32 tmp1 = 0, tmp2 = 0;

	int    dbg_ret = 0;
	hba_desc = mv_hba_init_modmm(dev);
	if (NULL == hba_desc)
		goto ext_err_init;
	hba_desc->dev = dev;
	hba_desc->max_io = max_io;
	hba_desc->id     = __mv_get_adapter_count() - 1;

	if (pci_read_config_byte(hba_desc->dev,
				 PCI_REVISION_ID,
				 &hba_desc->Revision_Id)) {
		MV_PRINT("%s : Failed to get hba's revision id.\n",
		       mv_product_name);
		goto ext_err_pci;
	}

	hba_desc->Vendor_Id = dev->vendor;
	hba_desc->Device_Id = dev->device;
	hba_desc->Sub_Vendor_Id = dev->subsystem_vendor;
	hba_desc->Sub_System_Id = dev->subsystem_device;
	MV_DPRINT(("original device id=%04X.\n",hba_desc->Device_Id));

	if (hba_desc->Device_Id == DEVICE_ID_6440) {
		if (hba_desc->Sub_System_Id == DEVICE_ID_6480)
			hba_desc->Device_Id = DEVICE_ID_6480;
	}
	__map_pci_addr(dev, hba_desc->Base_Address);

	pci_read_config_dword(dev, 0x7c, &tmp1);;
	pci_read_config_dword(dev, 0x80, &tmp2);
	
	hba_desc->pcie_max_lnk_spd = (MV_U8)(tmp1 & 0x0F);
	hba_desc->pcie_max_bus_wdth = (MV_U8)((tmp1 >> 4) & 0x3F);
	hba_desc->pcie_neg_lnk_spd = (MV_U8)((tmp2 >> 16) & 0x0F);
	hba_desc->pcie_neg_bus_wdth = (MV_U8)((tmp2 >> 20) & 0x3F);

	spin_lock_init(&hba_desc->global_lock);
	spin_lock_init(&hba_desc->device_spin_up);

	MV_DPRINT(( "HBA ext struct init'ed at %p.\n",hba_desc));

	if (register_online_modules(hba_desc))
		goto ext_err_modmm;

	if (mv_alloc_module_resource(hba_desc))
		goto ext_err_modmm;

	mod_desc = __get_highest_module(hba_desc);
	if (NULL == mod_desc)
		goto ext_err_pci;
	__mv_save_hba_configuration(hba_desc, mod_desc->extension);
	
	phba=(PHBA_Extension)mod_desc->extension;

	phba->RunAsNonRAID = 1;

	hba_desc->RunAsNonRAID = phba->RunAsNonRAID;

#ifdef CONFIG_PM
	mod_desc =  __get_highest_module(hba_desc);
	pci_set_drvdata(dev,mod_desc);
#endif
	mod_desc = __get_lowest_module(hba_desc);
	if (NULL == mod_desc)
		goto ext_err_pci;

	hba_desc->alloc_uncahemem_failed = MV_FALSE;
	while (mod_desc) {
		if (MV_MOD_INITED != mod_desc->status)
			continue;
		mod_desc->ops->module_initialize(mod_desc->extension,
						 mod_desc->extension_size,
						 hba_desc->max_io);
		if (hba_desc->alloc_uncahemem_failed)
			goto ext_err_pci;
		mod_desc = mod_desc->parent;
	}

	return 0;

ext_err_pci:
	++dbg_ret;
	mv_release_module_resource(hba_desc);
ext_err_modmm:
	++dbg_ret;
	mv_hba_release_modmm(dev);
ext_err_init:
        ++dbg_ret;
	return dbg_ret;
}


