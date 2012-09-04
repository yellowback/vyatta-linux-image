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
#include "core_header.h"
#include "core_internal.h"
#include "core_manager.h"
#include "core_util.h"
#include "core_alarm.h"

MV_VOID
core_alarm_init(MV_PVOID core_p)
{
	core_extension *core = (core_extension *) core_p;

	core->alarm.alarm_state = ALARM_STATE_OFF;
	core->alarm.reg_setting = 0;
	core->alarm.timer_id = 0;

	core_alarm_enable_register(core);
}

MV_VOID
core_alarm_timer_callback(MV_PVOID core_p, MV_PVOID tmp)
{
	core_extension *core = (core_extension *)core_p;
	MV_U8 time;

	core->alarm.timer_id = 0;

	switch (core->alarm.alarm_state) {
	case ALARM_STATE_1S_1S:
		/* 1 second on , 1 second off */
		if (core->alarm.reg_setting == ALARM_REG_ON) {
			time = 1;
			core->alarm.reg_setting = ALARM_REG_OFF;
			core_alarm_set_register(core, MV_FALSE);
		} else {
			time = 1;
			core->alarm.reg_setting = ALARM_REG_ON;
			core_alarm_set_register(core, MV_TRUE);
		}
		core->alarm.timer_id = core_add_timer(core, time,
			core_alarm_timer_callback, core, NULL);
		break;
	case ALARM_STATE_OFF:	
		core->alarm.reg_setting = ALARM_REG_OFF;
		core_alarm_set_register(core, MV_FALSE);
		break;
	case ALARM_STATE_MUTE:	
		core->alarm.reg_setting = ALARM_REG_OFF;
		core_alarm_set_register(core, MV_FALSE);
		break;
	default:
		MV_ASSERT(MV_FALSE);
		break;
	}
}

MV_VOID
core_alarm_change_state(MV_PVOID core_p, MV_U8 state)
{
	core_extension *core = (core_extension *)core_p;

	switch (state) {
	case ALARM_STATE_OFF:
		core->alarm.alarm_state = state;
		if (core->alarm.timer_id == 0) 
			MV_DASSERT(core->alarm.reg_setting == ALARM_REG_OFF);
		break;
	case ALARM_STATE_1S_1S:
		/* 1 second on , 1 second off */
		core->alarm.alarm_state = state;
		if (core->alarm.timer_id == 0) {
			/* lets start in on state first */
			core->alarm.timer_id = core_add_timer(core, 1, 
				core_alarm_timer_callback, core, NULL);
			core->alarm.reg_setting = ALARM_REG_ON;
			core_alarm_set_register(core, MV_TRUE);
		}
		break;
	case ALARM_STATE_MUTE:
		core->alarm.alarm_state = ALARM_STATE_MUTE;
		break;
	default:
		CORE_DPRINT(("Invalid alarm state: %d.\n", state));
		break;
	}
}

