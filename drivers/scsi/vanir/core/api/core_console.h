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
#if !defined(CORE_CONSOLE_H)
#define CORE_CONSOLE_H

#include "mv_config.h"
#include "core_header.h"

typedef MV_U8 (*core_management_command_handler)(core_extension *, PMV_Request);
#define HD_WRITECACHE_OFF		0
#define HD_WRITECACHE_ON		1
MV_U8 api_verify_command(MV_PVOID root_p, MV_PVOID dev_p,
	MV_Request *req);

MV_U8
core_pd_command(
	IN MV_PVOID root_p,
	IN PMV_Request pReq
	);

MV_U8
core_pass_thru_send_command(
	IN MV_PVOID *core_p,
	IN PMV_Request pReq
	);

MV_U8 core_ses_command(
	IN MV_PVOID core_p,
	IN PMV_Request req
	);

MV_BOOLEAN
core_flash_command(
	IN MV_PVOID core_p,
	IN PMV_Request p_req
	);

MV_U8 core_api_inquiry(MV_PVOID core_p, PMV_Request req);
MV_U8 core_api_read_capacity(MV_PVOID core_p, PMV_Request req);
MV_U8 core_api_read_capacity_16(MV_PVOID core_p, PMV_Request req);
MV_U8 core_api_report_lun(MV_PVOID core_p, PMV_Request req);
MV_U8 core_api_disk_io_control(MV_PVOID core_p, PMV_Request req);
MV_U8 core_api_alarm_command(MV_PVOID core_p, PMV_Request req);

MV_U8
mv_get_bbu_info(MV_PVOID core, PMV_Request pReq);
MV_U8
mv_set_bbu_threshold(MV_PVOID core, PMV_Request pReq);
MV_U8
mv_bbu_power_change(MV_PVOID core, PMV_Request pReq);

MV_U8 core_pd_request_get_hd_info(core_extension * core_p, PMV_Request req);
MV_U8 core_pd_request_get_enclosure_info(core_extension * core_p, PMV_Request req);
MV_U8 core_pd_request_get_expander_info(core_extension * core_p, PMV_Request req);
MV_BOOLEAN core_pd_request_get_hd_config(core_extension * core_p, PMV_Request req);
MV_U8 core_pd_request_set_hd_config(core_extension * core_p, PMV_Request req);
MV_U8 core_pd_request_get_hd_status(core_extension * core_p, PMV_Request req);
MV_U8 core_pd_request_get_hd_info_ext(core_extension * core_p, PMV_Request req);
#endif /* CORE_CONSOLE_H */
