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
#ifndef __CORE_PM_H
#define __CORE_PM_H

#include "mv_config.h"
#include "core_type.h"

MV_BOOLEAN pm_state_machine(MV_PVOID dev);
MV_U8 pm_verify_command(MV_PVOID root_p, MV_PVOID dev,
       MV_Request *req);
MV_VOID pm_prepare_command(MV_PVOID root_p, MV_PVOID dev,
	MV_PVOID cmd_header, MV_PVOID cmd_table, 
	MV_Request *req);
MV_VOID pm_send_command(MV_PVOID root_p, MV_PVOID dev, 
	MV_Request *req);
MV_VOID pm_process_command(MV_PVOID root_p, MV_PVOID dev,
	MV_PVOID cmpl_q_p, MV_PVOID cmd_table, 
	MV_Request *req);

/* PM registers definitions */
#define PM_OP_READ					0
#define PM_OP_WRITE					1

#define PM_GSCR_ID					0
#define PM_GSCR_REVISION			1
#define PM_GSCR_INFO				2
#define PM_GSCR_ERROR				32
#define PM_GSCR_ERROR_ENABLE		33
#define PM_GSCR_FEATURES			64
#define PM_GSCR_FEATURES_ENABLE		96

#define PM_PSCR_SSTATUS				0
#define PM_PSCR_SERROR				1
#define PM_PSCR_SCONTROL			2
#define PM_PSCR_SACTIVE				3

#define PM_SERROR_EXCHANGED			MV_BIT(26)
#define PM_SERROR_COMM_WAKE			MV_BIT(18)
#define PM_SERROR_PHYRDY_CHANGE		MV_BIT(16)

#endif /* __CORE_PM_H */
