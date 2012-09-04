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
#ifndef _LINUX_MAIN_H
#define _LINUX_MAIN_H

#include "hba_header.h"

#define __mv_get_ext_from_host(phost) \
          (((HBA_Extension **) (phost)->hostdata)[0])

/* for communication with OS/SCSI mid layer only */
enum {
	MV_MAX_REQUEST_DEPTH		 = MAX_REQUEST_NUMBER_PERFORMANCE - 2,
	MV_MAX_IO                = MAX_REQUEST_NUMBER_PERFORMANCE,
	MV_MAX_REQUEST_PER_LUN   = MAX_REQUEST_PER_LUN_PERFORMANCE,
	MV_MAX_SG_ENTRY          = SG_ALL,
	MV_MAX_IOCTL_REQUEST = 30,
	MV_SHT_USE_CLUSTERING    = ENABLE_CLUSTERING,
	MV_SHT_EMULATED          = 0,
	MV_SHT_THIS_ID           = -1,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
#define mv_scmd_host(cmd)    cmd->device->host
#define mv_scmd_channel(cmd) cmd->device->channel
#define mv_scmd_target(cmd)  cmd->device->id
#define mv_scmd_lun(cmd)     cmd->device->lun
#else
#define mv_scmd_host(cmd)    cmd->host
#define mv_scmd_channel(cmd) cmd->channel
#define mv_scmd_target(cmd)  cmd->target
#define mv_scmd_lun(cmd)     cmd->lun
#endif

#define LO_BUSADDR(x) ((MV_U32)(x))
#define HI_BUSADDR(x) (MV_U32)(sizeof(BUS_ADDRESS)>4? (u64)(x) >> 32 : 0)

struct _MV_SCP {
	MV_U16           mapped;
	MV_U16           map_atomic;
	BUS_ADDRESS bus_address;
};

#define MV_SCp(cmd) ((struct _MV_SCP *)(&((struct scsi_cmnd *)cmd)->SCp))

#ifndef scsi_to_pci_dma_dir
#define scsi_to_pci_dma_dir(scsi_dir) ((int)(scsi_dir))
#endif

void  hba_req_cache_destroy(MV_PVOID hba_ext);
int hba_req_cache_create(MV_PVOID hba_ext);
PMV_Request hba_req_cache_alloc(MV_PVOID hba_ext);
void hba_req_cache_free(MV_PVOID hba_ext,PMV_Request req) ;
void mv_complete_request(struct hba_extension *phba,
				struct scsi_cmnd *scmd,
				PMV_Request pReq);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
int __hba_wait_for_atomic_timeout(atomic_t *atomic, unsigned long timeout);
#endif

extern void * kbuf_array[512];
extern unsigned char mvcdb[512][16];
extern int mv_new_ioctl(struct scsi_device *dev, int cmd, void __user *arg);
void hba_send_shutdown_req(MV_PVOID);

#endif /*_LINUX_MAIN_H*/

