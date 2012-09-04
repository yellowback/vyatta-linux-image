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
#ifndef __MODULE_MANAGE_H__
#define __MODULE_MANAGE_H__

#include "hba_header.h"

/* module management & hba module code */
extern struct mv_module_ops *mv_core_register_module(void);
extern struct mv_module_ops *mv_hba_register_module(void);

/* adapter descriptor */
struct mv_adp_desc {
	List_Head hba_entry;
	List_Head  online_module_list;
	spinlock_t   global_lock;
	spinlock_t   device_spin_up;

	struct timer_list hba_timer;
	struct pci_dev    *dev;
	dev_t   dev_no;
	struct cdev 	cdev;

	struct Scsi_Host	*hba_host;
	struct completion		cmpl;
	struct completion		ioctl_cmpl;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_t				hba_sync;
	atomic_t				hba_ioctl_sync;
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */

	struct tasklet_struct mv_tasklet;
	atomic_t		tasklet_active_count;
	spinlock_t	tasklet_count_lock;

	/* adapter information */
	MV_U8             Adapter_Bus_Number;
	MV_U8             Adapter_Device_Number;
	MV_U8             Revision_Id;
	MV_U8             id;             /* multi-hba support, start from 0 */
	MV_U8             running_mod_num;/* number of up & running modules */
	MV_U8             RunAsNonRAID;	
	MV_U16            RaidMode;
	MV_U16            Vendor_Id;
	MV_U16            Device_Id;
	MV_U16            Sub_System_Id;
	MV_U16            Sub_Vendor_Id;	
	
	MV_U8		pcie_max_lnk_spd;	/* PCIe Max Supported Link Speed */
	MV_U8		pcie_max_bus_wdth;	/* PCIe Max Supported Bus Width */
	MV_U8		pcie_neg_lnk_spd;		/* PCIe Negotiated Link Speed */
	MV_U8		pcie_neg_bus_wdth;	/* PCIe Negotiated Bus Width */
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,11)
	unsigned int   pci_config_space[16];
#endif
	/* System resource */
	MV_PVOID          Base_Address[ROUNDING(MAX_BASE_ADDRESS, 2)];
	MV_U32            max_io;
	MV_BOOLEAN    alloc_uncahemem_failed;

};

int  mv_hba_init(struct pci_dev *dev, MV_U32 max_io);
void mv_hba_release(struct pci_dev *dev);
void mv_hba_stop(struct pci_dev *dev);
int  mv_hba_start(struct pci_dev *dev);
MV_PVOID *mv_get_hba_extension(struct mv_adp_desc *hba_desc);
int __mv_get_adapter_count(void);
void raid_get_hba_page_info( MV_PVOID This);
struct mv_mod_desc * __get_lowest_module(struct mv_adp_desc *hba_desc);
struct mv_mod_desc * __get_highest_module(struct mv_adp_desc *hba_desc);
int __mv_is_mod_all_started(struct mv_adp_desc *adp_desc);
int __alloc_consistent_mem(struct mv_mod_res *mod_res,
				  struct pci_dev *dev);

#endif /* __MODULE_MANAGE_H__ */

