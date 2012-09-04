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
#include "core_manager.h"
#include "core_type.h"
#include "core_internal.h"
#include "com_struct.h"
#include "com_api.h"
#include "com_error.h"
#include "core_api.h"
#include "core_util.h"
#include "hba_inter.h"
MV_U8 core_get_battery_status(MV_PVOID p_core_ext)
{
    PHBA_Extension hba_ptr = (PHBA_Extension)HBA_GetModuleExtension((core_extension *)p_core_ext, MODULE_HBA);
    MV_U8 bbu_status = BBU_NORMAL;

    bbu_status = BBU_NOT_PRESENT;

    return bbu_status;
}

MV_U8
mv_get_bbu_info(MV_PVOID core, PMV_Request pReq)
{
    PHBA_Extension hba_ptr = (PHBA_Extension) HBA_GetModuleExtension(core, MODULE_HBA);

    if (pReq->Sense_Info_Buffer != NULL)
        ((MV_PU8)pReq->Sense_Info_Buffer)[0] = ERR_NOT_SUPPORTED;
    pReq->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;

    return MV_QUEUE_COMMAND_RESULT_FINISHED;
}


MV_U8
mv_set_bbu_threshold(MV_PVOID core, PMV_Request pReq)
{
    PHBA_Extension hba_ptr = (PHBA_Extension) HBA_GetModuleExtension(core, MODULE_HBA);
    HBA_Info_Page		hba_info_param;
    MV_U8 type = pReq->Cdb[2];
    MV_U16 lowerbound = (pReq->Cdb[3] << 8) | (pReq->Cdb[4]);
    MV_U16 upperbound = (pReq->Cdb[5] << 8) | (pReq->Cdb[6]);

    if (pReq->Sense_Info_Buffer != NULL)
        ((MV_PU8)pReq->Sense_Info_Buffer)[0] = ERR_NOT_SUPPORTED;
    pReq->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;

    return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_U8
mv_bbu_power_change(MV_PVOID core, PMV_Request pReq)
{
    PHBA_Extension hba_ptr = (PHBA_Extension) HBA_GetModuleExtension(core, MODULE_HBA);
    MV_U8 type = pReq->Cdb[2];

    if (pReq->Sense_Info_Buffer != NULL)
        ((MV_PU8)pReq->Sense_Info_Buffer)[0] = ERR_NOT_SUPPORTED;
    pReq->Scsi_Status = REQ_STATUS_ERROR_WITH_SENSE;

    return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

