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
#include "core_type.h"

#include "core_device.h"
#include "core_internal.h"
#include "core_resource.h"
#include "core_manager.h"
#include "com_list.h"

#include "core_error.h"
#include "core_util.h"

domain_base *get_device_by_id(lib_device_mgr *lib_dev, MV_U16 id)
{
	if (id >= MAX_ID)
		return NULL;

	return lib_dev->device_map[id];
}

MV_U16 get_id_by_phyid(lib_device_mgr *lib_dev, MV_U16 phy_id)
{
	domain_base *base;
	MV_U16 i = 0xFFFF;

	for(i=0; i< MAX_ID; i++) {
		base = lib_dev->device_map[i];
		if (base == NULL) continue;
		if (base->port == NULL) continue;
		if (base->port->phy == NULL) continue;
		if (base->port->phy->id == phy_id){
			return i;
		}				
	}
	return 0xFFFF;
}

MV_U16 get_id_by_targetid_lun(MV_PVOID ext,  MV_U16 id, MV_U16 lun)
{
	core_extension *core = HBA_GetModuleExtension(ext, MODULE_CORE);
	lib_device_mgr *lib_dev = &core->lib_dev;
	MV_U16 i = id;
	if (id >= MAX_ID)
		return i;
	for(i=0; i<MAX_ID; i++) {
		domain_base *base;
		base = lib_dev->device_map[i];
		if (base ==NULL)
			continue;
		if ( (base->TargetID== id)&&(base->LUN == lun)){
			return i;
		}				
	}
	for(i=0; i<MAX_ID; i++) {
		domain_base *base;
		base = lib_dev->device_map[i];
		if (base ==NULL)
			return i;			
	}
	return 0xffff;
}

MV_U16 get_device_lun(lib_device_mgr *lib_dev, MV_U16 id)
{
	domain_base *base=NULL;
	base = lib_dev->device_map[id];
	if(base == NULL)
		return 0xffff;
	return base->LUN ;

}
MV_U16 get_device_targetid(lib_device_mgr *lib_dev, MV_U16 id)
{
	domain_base *base=NULL;
	base = lib_dev->device_map[id];
	if(base == NULL)
		return 0xFFFF;
	return base->TargetID;

}

domain_device *get_device_by_register_set(pl_root *root,
	MV_U8 register_set)
{
	lib_device_mgr *lib_dev = root->lib_dev;
	domain_device *device;
	domain_base *base;
	MV_U16 i;

	for (i = 0; i < MAX_ID; i++) {
		base = lib_dev->device_map[i];
		if ((base != NULL) && (base->type == BASE_TYPE_DOMAIN_DEVICE)) {
			device = (domain_device *)base;
			if (IS_STP_OR_SATA(device) &&
				(device->base.root == root) &&
				(device->register_set == register_set)) {
				return device;
			}
		}
	}

	return NULL;
}

MV_U16 get_available_dev_id_new(lib_device_mgr *lib_dev)
{
	MV_U16 i;
	for (i=PORT_NUMBER; i<MAX_ID; i++) {
		if (lib_dev->device_map[i] == NULL){
			return i;
			}
	}
	return MAX_ID;
}

MV_U16 get_available_dev_id(lib_device_mgr *lib_dev)
{
	int i;

	for (i=MAX_ID-1; i>=0; i--) {
		if (lib_dev->device_map[i] == NULL){
			return i;
		}
	}
	return MAX_ID;
}

MV_U16
get_avail_non_storage_dev_id(lib_device_mgr *lib_dev)
{
	MV_U16 i;

	for (i=MV_MAX_HD_DEVICE_ID; i<MAX_ID; i++) {
		if (lib_dev->device_map[i] == NULL)
			return i;
	}
	return MAX_ID;
}

MV_VOID release_available_dev_id(lib_device_mgr *lib_dev, MV_U16 id)
{
	MV_ASSERT(id < MAX_ID);
	lib_dev->device_map[id] = NULL;
}

MV_BOOLEAN change_device_map(lib_device_mgr *lib_dev, domain_base *base,
	MV_U16 old_id, MV_U16 new_id)
{
	MV_ASSERT(old_id < MAX_ID);
	MV_ASSERT(new_id < MAX_ID);
	MV_ASSERT(lib_dev->device_map[old_id] == base);

	if (lib_dev->device_map[new_id] != NULL)
		return MV_FALSE;

	lib_dev->device_map[old_id] = NULL;
	lib_dev->device_map[new_id] = base;
	return MV_TRUE;
}

MV_U16 add_device_map(lib_device_mgr *lib_dev, domain_base *base)
{
	MV_U16 id = get_available_dev_id(lib_dev);
	if (id >= MAX_ID) {
		CORE_DPRINT(("out of device ids.\n"));
		return MAX_ID;
	}
	lib_dev->device_map[id] = base;
	return id;
}

MV_VOID remove_device_map(lib_device_mgr *lib_dev, MV_U16 id)
{
	release_available_dev_id(lib_dev, id);
}

MV_VOID set_up_new_base(pl_root *root, domain_port *port,
	domain_base *base,
	command_handler *handler, enum base_type type, MV_U16 size)
{
	base->port = port;
	base->handler = handler;
	base->queue_depth = 1; /* will be update later */
	base->root = root;
	base->type = type;
	base->LUN = 0;
	base->TargetID = 0xFFFF;
	base->multi_lun = MV_FALSE;
	base->struct_size = size;
	base->outstanding_req = 0;
	base->parent = &port->base; /* by default set to port */
       base->blocked = MV_FALSE;
	base->cmd_issue_stopped = MV_FALSE;

	/* error handling */
	MV_LIST_HEAD_INIT(&base->err_ctx.sent_req_list);
	MV_DASSERT(base->err_ctx.eh_state == 0);
	base->err_ctx.eh_type = EH_TYPE_NONE;
	base->err_ctx.eh_state = EH_STATE_NONE;
	base->err_ctx.error_req = NULL;
	base->err_ctx.state = BASE_STATE_NONE;
	base->err_ctx.pm_eh_error_port = 0xff;
	base->err_ctx.pm_eh_active_port = 0xff;
	base->err_ctx.timeout_count = 0;
	base->err_ctx.timer_id = NO_CURRENT_TIMER;
	base->err_ctx.eh_timer = NO_CURRENT_TIMER;	
	base->err_ctx.error_count = 0;
}

MV_VOID set_up_new_device(pl_root *root, domain_port *port,
	domain_device *device,
	command_handler *handler)
{
	set_up_new_base(root, port, &device->base,
		handler,
		BASE_TYPE_DOMAIN_DEVICE,
		sizeof(domain_device));
	device->status = DEVICE_STATUS_FUNCTIONAL;
	device->register_set = NO_REGISTER_SET;
	device->curr_phy_map = 0;
}

MV_VOID set_up_new_pm(pl_root *root, domain_port *port,
	domain_pm *pm)
{
	set_up_new_base(root, port, &pm->base,
		(command_handler *)core_get_handler(root, HANDLER_PM),
                BASE_TYPE_DOMAIN_PM,
                sizeof(domain_pm));
	pm->status = PM_STATUS_EXISTING | PM_STATUS_FUNCTIONAL;
	pm->state = PM_STATE_RESET_DONE;
	pm->register_set = NO_REGISTER_SET;
	pm->sata_sig_timer = NO_CURRENT_TIMER;


	set_up_new_base(root, port, &port->base,
		(command_handler *)core_get_handler(root, HANDLER_SATA_PORT),
		BASE_TYPE_DOMAIN_PORT,
		sizeof(domain_port));
	port->pm = pm;
	port->sata_sig_timer = NO_CURRENT_TIMER;

}

MV_VOID set_up_new_expander(pl_root *root, domain_port *port,
	domain_expander *expander)
{
	set_up_new_base(root, port, &expander->base,
		(command_handler *)core_get_handler(root, HANDLER_SMP),
		BASE_TYPE_DOMAIN_EXPANDER,
		sizeof(domain_expander));

	expander->state = EXP_STATE_REPORT_GENERAL;
	expander->status = EXP_STATUS_EXISTING | EXP_STATUS_FUNCTIONAL;
	MV_LIST_HEAD_INIT(&expander->device_list);
	MV_LIST_HEAD_INIT(&expander->expander_list);
	expander->device_count = 0;
	expander->expander_count = 0;
       expander->has_been_setdown = MV_FALSE;
	expander->has_been_reset = MV_TRUE;
	expander->timer_tag = NO_CURRENT_TIMER;
	MV_ZeroMemory(expander->route_table, sizeof(MV_U16)*MAXIMUM_EXPANDER_PHYS);
}
MV_VOID set_up_new_enclosure(pl_root *root, domain_port *port,
	domain_enclosure *enclosure,
	command_handler *handler)
{
	set_up_new_base(root, port, &enclosure->base,
		handler,
		BASE_TYPE_DOMAIN_ENCLOSURE,
		sizeof(domain_enclosure));
	MV_LIST_HEAD_INIT(&enclosure->expander_list);
	enclosure->status = ENCLOSURE_STATUS_EXISTING | ENCLOSURE_STATUS_FUNCTIONAL;
	enclosure->enc_flag = ENC_FLAG_FIRST_INIT;
}
