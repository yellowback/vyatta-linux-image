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
#ifndef __CORE_SAT_H
#define __CORE_SAT_H

#include "mv_config.h"
MV_BOOLEAN sat_categorize_cdb(pl_root *root, MV_Request *req);
MV_U8 scsi_ata_translation(pl_root *root, MV_Request *req);
MV_U8 sat_release_org_req(pl_root *root, MV_Request *req);
MV_BOOLEAN ata_fill_taskfile(domain_device *device, MV_Request *req, 
	MV_U8 tag, ata_taskfile *taskfile);
MV_BOOLEAN atapi_fill_taskfile(domain_device *device, MV_Request *req, 
	ata_taskfile *taskfile);
MV_BOOLEAN passthru_fill_taskfile(MV_Request *req, ata_taskfile *taskfile);
MV_VOID scsi_to_sata_fis(MV_Request *req, MV_PVOID fis_pool, 
	ata_taskfile *taskfile, MV_U8 pm_port);
MV_VOID sat_make_sense(MV_PVOID root_p, MV_Request *req, MV_U8 status,
        MV_U8 error, MV_U64 *lba);

MV_Request *sat_get_org_req(MV_Request *req);
MV_Request *sat_clear_org_req(MV_Request *req);

/**************************************************************/
/* buffer is sizeof mdPage_t */
#define LOG_SENSE_WORK_BUFER_LEN 512

/**************************************************************/
/* Format Unit Scsi command translation 
 */
#define LBA_BLOCK_COUNT		256
#define STANDARD_INQUIRY_LENGTH                 0x60
#define ATA_RETURN_DESCRIPTOR_LENGTH            14
#define MV_MAX_INTL_BUFFER_SIZE                 768

enum _ATA_SEND_DIAGNOSTIC_STATE {
        SEND_DIAGNOSTIC_START = 0,
        SEND_DIAGNOSTIC_SMART,
        SEND_DIAGNOSTIC_VERIFY_0,
        SEND_DIAGNOSTIC_VERIFY_MID,
        SEND_DIAGNOSTIC_VERIFY_MAX,
        SEND_DIAGNOSTIC_FINISHED
};

enum _ATA_START_STOP_STATE {
        START_STOP_START = 0,
        START_STOP_ACTIVE,
        START_STOP_IDLE_IMMEDIATE_SYNC,
        START_STOP_IDLE_IMMEDIATE,
        START_STOP_STANDBY_IMMEDIATE_SYNC,
        START_STOP_STANDBY_IMMEDIATE,
        START_STOP_STANDBY_SYNC,
        START_STOP_STANDBY,
        START_STOP_EJECT,
        START_STOP_FINISHED
};

enum _ATA_REASSIGN_BLOCKS_STATE {
        REASSIGN_BLOCKS_ERROR = MV_BIT(31),
};

enum _ATA_FORMAT_UNIT_STATE {
        FORMAT_UNIT_STARTED = 1,
};

#endif /* __CORE_SAT_H */
