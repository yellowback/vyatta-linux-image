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
#ifndef __CORE_EXPANDER_H
#define __CORE_EXPANDER_H

#include "mv_config.h"

#define SMP_VIRTUAL_REQ_BUFFER_SIZE			sizeof(smp_virtual_config_route_buffer)

typedef enum _CLEAR_AFF_STATUS {
	CLEAR_AFF_STATUS_NO_CLEAR = 0,
	CLEAR_AFF_STATUS_IS_SATA,
	CLEAR_AFF_STATUS_NEED_CLEAR,
	CLEAR_AFF_STATUS_NEED_RESET,
	CLEAR_AFF_STATUS_MAX,
} CLEAR_AFF_STATUS;

typedef struct _smp_virtual_config_route_buffer
{
	MV_U64	sas_addr[MAXIMUM_EXPANDER_PHYS];
	MV_U8	phy_id[MAX_WIDEPORT_PHYS];
} smp_virtual_config_route_buffer;

enum _CORE_CONTEXT_CLEAR_AFF_STATE {
	CLEAR_AFF_STATE_REPORT_PHY_SATA = 0,
	CLEAR_AFF_STATE_CLEAR_AFF,
	CLEAR_AFF_STATE_MAX,
};

MV_BOOLEAN exp_state_machine(MV_PVOID expander);
MV_Request *smp_make_phy_control_req(domain_device *device,
	MV_U8 operation, MV_PVOID callback);

#endif /* __CORE_EXPANDER_H */
