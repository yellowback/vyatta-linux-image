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
/*
 *
 *  Kernel/CLI interface
 *
 */

#ifndef __MV_HBA_LINUX_INTERFACE__
#define __MV_HBA_LINUX_INTERFACE__

#include "hba_header.h"
#include "com_ioctl.h"
#include "com_adapter_struct.h"

/*Request Structure.*/
#define SENSE_INFO_BUFFER_SIZE		32
#define MAX_COMMAND_SIZE		16

/* DMA read write command */
#define ATA_CMD_READ_DMA			0xC8	/* 24 bit DMA read */
#define ATA_CMD_READ_DMA_QUEUED			0xC7	/* 24 bit TCQ DMA read */
#define ATA_CMD_READ_DMA_EXT			0x25	/* 48 bit DMA read */
#define ATA_CMD_READ_DMA_QUEUED_EXT		0x26	/* 48 bit TCQ DMA read */
#define ATA_CMD_READ_FPDMA_QUEUED		0x60	/* NCQ DMA read: SATA only.* Always 48 bit */

#define ATA_CMD_WRITE_DMA			0xCA	
#define ATA_CMD_WRITE_DMA_QUEUED		0xCC
#define ATA_CMD_WRITE_DMA_EXT  			0x35
#define ATA_CMD_WRITE_DMA_QUEUED_EXT		0x36
#define ATA_CMD_WRITE_FPDMA_QUEUED		0x61

#define ATA_CMD_READ_PIO			0x20
#define ATA_CMD_READ_PIO_MULTIPLE               0xC4
#define ATA_CMD_READ_PIO_EXT			0x24
#define ATA_CMD_READ_PIO_MULTIPLE_EXT 		0x29
#define ATA_CMD_WRITE_PIO			0x30
#define ATA_CMD_WRITE_PIO_MULTIPLE              0xC5
#define ATA_CMD_WRITE_PIO_EXT			0x34
#define ATA_CMD_WRITE_PIO_MULTIPLE_EXT		0x39
#define ATA_CMD_WRITE_PIO_MULTIPLE_FUA_EXT      0xCE

#ifndef ATA_CMD_SMART
#define ATA_CMD_SMART					0xB0
#endif

enum {
	SG_CDB2_TLEN_NODATA	= 0 << 0,
	SG_CDB2_TLEN_FEAT	= 1 << 0,
	SG_CDB2_TLEN_NSECT	= 2 << 0,

	SG_CDB2_TLEN_BYTES	= 0 << 2,
	SG_CDB2_TLEN_SECTORS	= 1 << 2,

	SG_CDB2_TDIR_TO_DEV	= 0 << 3,
	SG_CDB2_TDIR_FROM_DEV	= 1 << 3,

	SG_CDB2_CHECK_COND	= 1 << 5,
};

#define SG_READ			0
#define SG_WRITE		1
#define SG_PIO			0
#define SG_DMA			1

enum {
	/*
	 * These (redundantly) specify the category of the request
	 */
	TASKFILE_CMD_REQ_NODATA	= 0,	/* ide: IDE_DRIVE_TASK_NO_DATA */
	TASKFILE_CMD_REQ_IN	= 2,	/* ide: IDE_DRIVE_TASK_IN */
	TASKFILE_CMD_REQ_OUT	= 3,	/* ide: IDE_DRIVE_TASK_OUT */
	TASKFILE_CMD_REQ_RAW_OUT= 4,	/* ide: IDE_DRIVE_TASK_RAW_WRITE */
	/*
	 * These specify the method of transfer (pio, dma, multi, ..)
	 */
	TASKFILE_DPHASE_NONE	= 0,	/* ide: TASKFILE_IN */
	TASKFILE_DPHASE_PIO_IN	= 1,	/* ide: TASKFILE_IN */
	TASKFILE_DPHASE_PIO_OUT	= 4,	/* ide: TASKFILE_OUT */
};
#define SG_ATA_LBA48		1
#define SG_ATA_PROTO_NON_DATA	( 3 << 1)
#define SG_ATA_PROTO_PIO_IN	( 4 << 1)
#define SG_ATA_PROTO_PIO_OUT	( 5 << 1)
#define SG_ATA_PROTO_DMA	( 6 << 1)
#define SG_ATA_PROTO_UDMA_IN	(11 << 1)
#define SG_ATA_PROTO_UDMA_OUT	(12 << 1)

typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER{
	SCSI_PASS_THROUGH_DIRECT        sptd;
	unsigned long                   Filler;
	unsigned char                   Sense_Buffer[SENSE_INFO_BUFFER_SIZE];
}SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;

int mv_linux_proc_info(struct Scsi_Host *pSHost, char *pBuffer,
		       char **ppStart,  off_t offset, int length, int inout);
void IOHBARequestCallback(MV_PVOID This, PMV_Request pReq);
struct hba_extension;
int mv_register_chdev(struct hba_extension *hba);
void mv_unregister_chdev(struct hba_extension *hba);

#endif /* ifndef __MV_HBA_LINUX_INTERFACE__ */
