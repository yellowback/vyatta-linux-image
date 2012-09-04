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
#ifndef _CORE_ALARM_H
#define _CORE_ALARM_H

typedef enum _alarm_state {
	ALARM_STATE_OFF = 0,
	ALARM_STATE_1S_1S,	/* 1s on, 1s off */
	ALARM_STATE_MUTE,
	ALARM_STATE_MAX,
} alarm_state;

enum {
	ALARM_REG_OFF = 0,
	ALARM_REG_ON,
};

typedef struct _lib_alarm {
	MV_U8 alarm_state;
	MV_U8 reg_setting;
	MV_U16 timer_id;
} lib_alarm;

MV_VOID core_alarm_init(MV_PVOID core_p);
MV_VOID core_alarm_change_state(MV_PVOID core_p, MV_U8 state);

#endif /* _CORE_ALARM_H */
