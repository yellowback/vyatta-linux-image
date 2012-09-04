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
#ifndef __CORE_MANAGER_H
#define __CORE_MANAGER_H

#include "mv_config.h"
#include "core_type.h"
#include "core_internal.h"
#include "core_resource.h"
#include "core_device.h"
#include "core_gpio.h"
#include "core_rmw_flash.h"
#include "core_alarm.h"

struct device_spin_up{
	List_Head	list;
	pl_root		*roots;	
	struct _domain_base *base;
};

enum CORE_STATE {
	CORE_STATE_IDLE = 0,
	CORE_STATE_STARTING,
	CORE_STATE_STARTED,
};

typedef struct _core_extension {
	MV_PVOID			  desc;
	MV_U16                          vendor_id;
	MV_U16                          device_id;

	const struct mvs_chip_info *chip_info;

	MV_U8                           revision_id;
	MV_U8                           is_dump;
	MV_U8                           state;
	union {
		MV_U8                   bh_enabled;
		MV_U8                   reserved0;
	};
	MV_U8				s3_state;
	MV_PVOID                        sg_pool;
	MV_PVOID                        ct_pool;
	char				sg_name[CACHE_NAME_LEN];
	char				ct_name[CACHE_NAME_LEN];

	MV_LPVOID                       mmio_base;
	MV_LPVOID                       io_base;
	MV_LPVOID                       nvsram_base;
	command_handler                 handlers[MAX_NUMBER_HANDLER];

	MV_U16                          max_io;
	MV_U16                          hw_sg_entry_count; /* It's hardware sg entry count per buffer */
	MV_U16                          init_queue_count; /* count the number devices need initialization */

	MV_U32                          irq_mask; /* mask for main irq */
	MV_U32                          main_irq; /* save interrupt status */

	lib_resource_mgr                lib_rsrc;
	lib_device_mgr                  lib_dev;

	Counted_List_Head               init_queue;

	List_Head                       event_queue;

	Counted_List_Head               error_queue;

	Counted_List_Head               waiting_queue;
	Counted_List_Head               high_priority_queue;

	List_Head                       complete_queue;

	pl_root                         roots[MAX_NUMBER_IO_CHIP];

	lib_gpio			lib_gpio;
       lib_rmw_flash                   lib_flash;
	lib_alarm			alarm;
	MV_PHYSICAL_ADDR                trash_bucket_dma;
	MV_U8	run_as_none_raid;

	MV_U8 		spin_up_group;
	MV_U8		spin_up_time;		
	List_Head	device_spin_up_list;
	MV_U16	 	device_spin_up_timer;
	MV_U8		reserved[2];
} core_extension;
#define IS_CORE_STARTED(core)	(((core_extension *)core)->state & CORE_STATE_STARTED)

MV_U32 Core_ModuleGetResourceQuota(enum Resource_Type type, MV_U16 maxIo);
MV_VOID Core_ModuleInitialize(MV_PVOID ModulePointer, MV_U32 extensionSize, MV_U16 maxIo);
MV_VOID Core_ModuleStart(MV_PVOID This);
MV_VOID Core_ModuleSendRequest(MV_PVOID This, PMV_Request pReq);
MV_VOID Core_ModuleMonitor(MV_PVOID This);
MV_VOID Core_ModuleNotification(MV_PVOID This, enum Module_Event event, struct mod_notif_param *param);
MV_VOID Core_ModuleReset(MV_PVOID This);
MV_VOID Core_ModuleShutdown(MV_PVOID This);

MV_VOID core_set_chip_options(MV_PVOID ext);
MV_VOID core_queue_completed_req(MV_PVOID ext, MV_Request *req);
MV_VOID core_complete_requests(MV_PVOID core_p);
MV_VOID core_abort_all_running_requests(MV_PVOID core_p);

void core_disable_ints(void *ext);
void core_enable_ints(void *ext);
MV_BOOLEAN core_reset_controller(core_extension * core);
MV_BOOLEAN core_clear_int(core_extension *core);
MV_VOID core_handle_int(core_extension *core);
void update_port_phy_map(pl_root *root, domain_phy *phy);
MV_VOID controller_init(core_extension *core);
MV_VOID io_chip_init_registers(pl_root *root);
void update_phy_info(pl_root *root, domain_phy *phy);
void core_set_cmd_header_selector(mv_command_header *cmd_header);
void set_phy_tuning(pl_root *root, domain_phy *phy, PHY_TUNING phy_tuning);
MV_VOID set_phy_ffe_tuning(pl_root *root, domain_phy *phy, FFE_CONTROL ffe);
MV_VOID set_phy_rate(pl_root *root, domain_phy *phy, MV_U8 rate);
MV_U8 get_min_negotiated_link_rate(domain_port *port);

struct mvs_chip_info {
	MV_U16		chip_id;
	MV_U8 		n_host;
	MV_U8		start_host;
	MV_U32 		n_phy;
	MV_U32 		fis_offs;
	MV_U32 		fis_count;
	MV_U32 		srs_sz;
	MV_U32 		slot_width;
};

#endif /* __CORE_MANAGER_H */
