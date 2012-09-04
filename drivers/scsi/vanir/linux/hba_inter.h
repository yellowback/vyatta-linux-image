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
#ifndef HBA_INTERNAL_H
#define HBA_INTERNAL_H
#include "hba_header.h"
#include "com_tag.h"

typedef struct _Timer_Request
{
	List_Head Queue_Pointer;
	MV_PVOID Context1;
	MV_PVOID Context2;
	MV_PVOID Reserved0;
	MV_VOID (*Routine) (MV_PVOID, MV_PVOID);
	MV_BOOLEAN Valid;
	MV_U8 Reserved1[7];
	MV_U64 Time_Stamp;
} Timer_Request, *PTimer_Request;


typedef struct _Timer_Module
{
	PTimer_Request *Running_Requests;
	MV_U16 Timer_Request_Number;
	MV_U8 Reserved0[6];
	Tag_Stack Tag_Pool;
	MV_U64 Time_Stamp;
} Timer_Module, *PTimer_Module;

enum hba_info_flags {
	MVF_MSI		= (1U << 0),	/* MSI is enabled */
};

/* Per logical unit */
struct mv_lu
{
	MV_U16 id;
	MV_U16 lun;
	MV_U16 reserved0[2];	
	MV_U16 target_id;	
	struct scsi_device	*sdev;		/* attached SCSI device */
};

typedef struct hba_extension
{
	/* self-descriptor */
	struct mv_mod_desc *desc;

	void    *req_pool;
	/* Device extention */
	MV_PVOID Device_Extension;
	/* System resource */
	MV_LPVOID Base_Address[MAX_BASE_ADDRESS];
	MV_U32 State;
	MV_BOOLEAN Is_Dump;		/* Is OS during hibernation or crash dump? */
	MV_U32 Io_Count;			/* Outstanding requests count */
	MV_U32	hba_flags;
	
	/* Adapter information */
	MV_U8 Adapter_Bus_Number;
	MV_U8 Adapter_Device_Number;
	MV_U16 Vendor_Id;
	MV_U16 Device_Id;
	MV_U8 Revision_Id;
	MV_U8 RunAsNonRAID;
	MV_BOOLEAN msi_enabled;
	MV_U8 reserved;
	MV_U16 RaidMode;
	MV_U16 Sub_Vendor_Id;
	MV_U16 Sub_System_Id;

	MV_U8 pcie_max_lnk_spd;	/* PCIe Max Supported Link Speed */
	MV_U8 pcie_max_bus_wdth;	/* PCIe Max Supported Bus Width */
	MV_U8 pcie_neg_lnk_spd;		/* PCIe Negotiated Link Speed */
	MV_U8 pcie_neg_bus_wdth;	/* PCIe Negotiated Bus Width */
	
	MV_U8 reserved1[2];
	MV_U32 MvAdapterSignature;

	/* Timer module */
	Timer_Module TimerModule;
	MV_PVOID			uncached_virtual;
	MV_PHYSICAL_ADDR	uncached_physical;
	MV_U32				uncached_quota;
	MV_U32				scsiport_allocated_uncached;
	MV_U16				Max_Io;
	MV_U16				waiting_cb_cnt;

	List_Head Waiting_Request;	/* MV_Request waiting queue */

	MV_U32 max_sg_count;
	mempool_t * mv_mempool;
	
	 kmem_cache_t  *mv_request_cache;
	 kmem_cache_t  *mv_request_sg_cache;
	
	char cache_name[CACHE_NAME_LEN];
	char sg_name[CACHE_NAME_LEN];
	
	List_Head Stored_Events;
	List_Head Free_Events;
	MV_U32	SequenceNumber;
	MV_U8 Num_Stored_Events;
	MV_U8 Reserved2[3];

	struct mv_lu mv_unit[MV_MAX_TARGET_NUMBER];

	MV_U8 FlashBad;
	MV_U8 FlashErase;
	MV_U8 Ioctl_Io_Count;
	MV_U8 first_scan;

	MV_PVOID		pNextExtension;
	MV_VOID 		(*pNextFunction)(MV_PVOID , PMV_Request);
	MV_PVOID p_raid_feature;

}HBA_Extension, *PHBA_Extension;

#define DRIVER_STATUS_IDLE      1    /* The first status */
#define DRIVER_STATUS_STARTING  2    /* Begin to start all modules */
#define DRIVER_STATUS_STARTED   3    /* All modules are all settled. */
#define DRIVER_STATUS_SHUTDOWN   4   /* All modules shutdown. */

#endif /* HBA_INTERNAL_H */
