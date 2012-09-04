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
#ifndef __CORE_ERROR_H
#define __CORE_ERROR_H

#include "mv_config.h"
#include "core_type.h"
#include "core_internal.h"
#include "core_protocol.h"

/* eh_state */
enum _eh_type
{
	EH_TYPE_NONE = 0, /* no error */
	EH_TYPE_MEDIA_ERROR = 1, /* return is finished with error status */
	EH_TYPE_TIMEOUT = 2, /* timeout */
};

#define EH_STATE_NONE 0

enum _sas_eh_timeout_state
{
	SAS_EH_TIMEOUT_STATE_NONE = EH_STATE_NONE,
	SAS_EH_TIMEOUT_STATE_ABORT_REQUEST = 1,
	SAS_EH_TIMEOUT_STATE_LU_RESET = 2,
	SAS_EH_TIMEOUT_STATE_DEVICE_RESET = 3,
	SAS_EH_TIMEOUT_STATE_DEVICE_RESET_WAIT =4,
	SAS_EH_TIMEOUT_STATE_PORT_RESET = 5,
	SAS_EH_TIMEOUT_STATE_SET_DOWN = 6,
};

enum _sas_eh_media_state
{
	SAS_EH_MEDIA_STATE_NONE = EH_STATE_NONE,
	SAS_EH_MEDIA_STATE_RETRY = 1,
	SAS_EH_MEDIA_STATE_HARD_RESET_WAIT = 2,
	SAS_EH_MEDIA_STATE_AFTER_RESET = 3,
};

enum _sata_eh_timeout_state
{
	SATA_EH_TIMEOUT_STATE_NONE = EH_STATE_NONE,
	SATA_EH_TIMEOUT_STATE_SOFT_RESET_1,
	SATA_EH_TIMEOUT_STATE_SOFT_RESET_WAIT,
	SATA_EH_TIMEOUT_STATE_SOFT_RESET_0,
	SATA_EH_TIMEOUT_STATE_HARD_RESET_WAIT,
	SATA_EH_TIMEOUT_STATE_HARD_RESET,
	SATA_EH_TIMEOUT_STATE_PORT_RESET,
	SATA_EH_TIMEOUT_STATE_DEV_HARD_RESET,
	SATA_EH_TIMEOUT_STATE_DEV_HARD_RESET_DONE,
	SATA_EH_TIMEOUT_STATE_PM_RESET_WAIT,
	SATA_EH_TIMEOUT_STATE_PM_SRST_1,
	SATA_EH_TIMEOUT_STATE_PM_SRST_0,
	SATA_EH_TIMEOUT_STATE_PM_SRST_WAIT,
	SATA_EH_TIMEOUT_STATE_PM_REINIT,
	SATA_EH_TIMEOUT_STATE_SET_DOWN,
	SATA_EH_TIMEOUT_STATE_RETRY_ORG_REQ,
};

enum _sata_eh_media_state
{
	SATA_EH_MEDIA_STATE_NONE = EH_STATE_NONE,
	SATA_EH_MEDIA_STATE_READ_LOG_EXTENT = 1,
	SATA_EH_MEDIA_STATE_RETRY = 2,
	SATA_EH_MEDIA_STATE_SOFT_RESET_1 = 3,
	SATA_EH_MEDIA_STATE_SOFT_RESET_0 = 4,
	SATA_EH_MEDIA_STATE_HARD_RESET_WAIT = 5,
	SATA_EH_MEDIA_STATE_AFTER_RESET = 6,

};

enum _pm_eh_state
{
	PM_EH_STATE_NONE = EH_STATE_NONE,
	PM_EH_HARD_RESET_DONE		= 1,
	PM_EH_QRIGA_WORKAROUND		= 2,
	PM_EH_ENABLE_FEATURES		= 3,
	PM_EH_DISABLE_ASYNOTIFY		= 4,
	PM_EH_CLEAR_ERROR_INFO		= 5,
	PM_EH_SPIN_UP_ALL_PORTS		= 6,
	PM_EH_ENABLE_PM_PORT_1		= 7,
	PM_EH_SPIN_UP_DONE			= 8,
	PM_EH_ENABLE_PM_PORT_0		= 9,
	PM_EH_SPIN_UP_WAIT			= 10,
	PM_EH_CHECK_PHY_RDY			= 11,
	PM_EH_CHECK_PHY_RDY_DONE	= 12,
	PM_EH_CLEAR_X_BIT			= 13,
	PM_EH_WAIT_FOR_READY		= 14,
	PM_EH_ISSUE_SOFT_RESET_1	= 15,
	PM_EH_ISSUE_SOFT_RESET_0	= 16,
	PM_EH_WAIT_SIG				= 17,
	PM_EH_SIG_DONE				= 18,
	PM_EH_WAIT_AFTER_RESET		= 19,
	PM_EH_ENABLE_ASYNOTIFY		= 20,
	PM_EH_ENABLE_ASYNOTIFY_WAIT = 21,
	PM_EH_DONE					= 22,
};

enum _atapi_eh_media_state
{
	ATAPI_EH_MEDIA_STATE_NONE = SATA_EH_MEDIA_STATE_NONE,
	ATAPI_EH_MEDIA_STATE_REQUEST_SENSE = 1,
};

MV_VOID mv_add_timer(MV_PVOID core_p, MV_Request *req);
MV_VOID mv_cancel_timer(MV_PVOID core_p, MV_PVOID base_p);
MV_VOID mv_renew_timer(MV_PVOID core_p, MV_Request *req);
MV_VOID core_req_timeout(MV_PVOID dev_p, MV_PVOID req_p);
MV_BOOLEAN sata_error_handler(MV_PVOID dev_p, MV_Request *req);
MV_BOOLEAN ssp_error_handler(MV_PVOID dev_p, MV_Request *req);
void sata_handle_taskfile_error(pl_root *root, MV_Request *req);
MV_U32 pal_abort_device_running_req(pl_root *root, domain_base *base);
MV_U32 pal_abort_port_running_req(pl_root *root, domain_port *port);
void pal_clean_expander_outstanding_req(pl_root *root, domain_expander *exp);
MV_U32 core_complete_device_waiting_queue(pl_root *root, domain_base *base,
	MV_U8 scsi_status);
MV_Request *core_eh_retry_org_req(pl_root *root, MV_Request *req,
	MV_ReqCompletion func);
MV_VOID prot_reset_slot(pl_root *root, domain_base *base, MV_U16 slot,
	MV_Request *req);

void pal_set_down_expander(pl_root *root, domain_expander *exp);
void pal_set_down_multi_lun_disk(pl_root *root, domain_device *dev, MV_BOOLEAN notify_os);
void pal_set_down_disk(pl_root *root, domain_device *dev, MV_BOOLEAN notify_os);
void pal_set_down_error_disk(pl_root *root, domain_device *dev, MV_BOOLEAN notify_os);
void pal_set_down_port(pl_root *root, domain_port *port);
void pal_set_down_enclosure(pl_root *root, domain_enclosure *enc);
void pal_abort_running_req_for_async_sdb_fis(pl_root *root, domain_device *dev);

MV_BOOLEAN port_is_running_error_handling(pl_root *root, domain_port *port);
MV_BOOLEAN port_has_error_req(pl_root *root, domain_port *port);
MV_BOOLEAN port_has_init_req(pl_root *root, domain_port *port);
MV_BOOLEAN device_has_error_req(pl_root *root, domain_base *base);
void core_handle_init_error(pl_root *root, domain_base *base, MV_Request *req);
MV_Request *core_make_port_reset_req(pl_root *root, domain_port *port,
	MV_PVOID base, MV_ReqCompletion func);
MV_Request *core_make_device_reset_req(pl_root *root, domain_port *port,
	domain_device *dev, MV_ReqCompletion func);

MV_BOOLEAN core_simulate_error(pl_root *root, domain_base *base,
	MV_PU32 cmpl_q, mv_command_table *cmd_table,
	MV_Request *req);

extern MV_U16 mv_debug_mode;
#define CORE_EH_PRINT(_x_)        do {if (CORE_DEBUG_INFO & mv_debug_mode)\
		{if (GENERAL_DEBUG_INFO & mv_debug_mode)\
	  	  {MV_DPRINT(("CORE EH: %s ", __FUNCTION__));\
	  	  MV_DPRINT (_x_);}\
	  	 else \
	  	  {MV_PRINT("CORE EH:  ");\
	  	  MV_PRINT _x_;} }\
	} while (0)

#define CORE_EVENT_PRINT(_x_) do {if (CORE_DEBUG_INFO & mv_debug_mode)\
	{MV_DPRINT(("CORE EVENT: %s ", __FUNCTION__));\
	MV_DPRINT (_x_);}\
	} while (0)

#endif /* __CORE_ERROR_H */
