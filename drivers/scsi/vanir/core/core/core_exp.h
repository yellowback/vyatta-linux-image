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
#ifndef __CORE_EXP_H
#define __CORE_EXP_H

#include "mv_config.h"
#include "core_type.h"

MV_U32 Core_ModuleGetResourceQuota(enum Resource_Type type, MV_U16 maxIo);
void Core_ModuleInitialize(MV_PVOID This, MV_U32 extensionSize, MV_U16 maxIo);
void Core_ModuleStart(MV_PVOID This);
MV_VOID Core_ModuleShutdown(MV_PVOID This);
void Core_ModuleNotification(MV_PVOID, enum Module_Event, struct mod_notif_param*);
void Core_ModuleSendRequest(MV_PVOID This, PMV_Request pReq);
void Core_ModuleMonitor(MV_PVOID This);
void Core_ModuleReset(MV_PVOID This);
MV_BOOLEAN Core_InterruptCheckIRQ(MV_PVOID This);
void Core_InterruptHandleIRQ(MV_PVOID This);
MV_BOOLEAN Core_InterruptServiceRoutine(MV_PVOID This);
MV_VOID core_push_queues(MV_PVOID core_p);
MV_VOID core_complete_requests(MV_PVOID core_p);
MV_U16 core_set_device_id(MV_U32 pad_test);
#define core_start_cmpl_notify(core)  	HBA_ModuleStarted(core)

#endif /* __CORE_EXP_H */
