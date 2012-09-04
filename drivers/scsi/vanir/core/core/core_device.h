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
#ifndef _CORE_DEVICE_H
#define _CORE_DEVICE_H

#include "mv_config.h"
#include "core_type.h"
#include "core_internal.h"

struct _lib_device_mgr {
	domain_base      *device_map[MAX_ID];
	MV_U16 target_id_map[MV_MAX_TARGET_NUMBER];
};

domain_base *get_device_by_id(lib_device_mgr *lib_dev, MV_U16 id);
MV_U16 get_id_by_phyid(lib_device_mgr *lib_dev, MV_U16 phy_id);
MV_U16 get_id_by_targetid_lun(MV_PVOID ext, MV_U16 id,MV_U16 lun);
MV_U16 get_device_lun(lib_device_mgr *lib_dev, MV_U16 id);
MV_U16 get_device_targetid(lib_device_mgr *lib_dev, MV_U16 id);
domain_device *get_device_by_register_set(pl_root *root,
	MV_U8 register_set);
MV_U16 get_available_dev_id_new(lib_device_mgr *lib_dev);
MV_U16 get_available_dev_id(lib_device_mgr *lib_dev);
MV_U16 get_avail_non_storage_dev_id(lib_device_mgr *lib_dev);
MV_VOID release_available_dev_id(lib_device_mgr *lib_dev, MV_U16 id);

MV_U16 add_device_map(lib_device_mgr *lib_dev, domain_base *base);
MV_VOID remove_device_map(lib_device_mgr *lib_dev, MV_U16 id);
MV_BOOLEAN change_device_map(lib_device_mgr *lib_dev, domain_base *base,
	MV_U16 old_id, MV_U16 new_id);

MV_VOID set_up_new_base(pl_root *root, domain_port *port,
	domain_base *base,
	command_handler *handler, enum base_type type, MV_U16 size);
MV_VOID set_up_new_device(pl_root *root, domain_port *port,
	domain_device *device, command_handler *handler);
MV_VOID set_up_new_pm(pl_root *root, domain_port *port,
	domain_pm *pm);
MV_VOID set_up_new_expander(pl_root *root, domain_port *port,
	domain_expander *expander);
MV_VOID set_up_new_enclosure(pl_root *root, domain_port *port,
	domain_enclosure *enclosure,
	command_handler *handler);
#endif /* _CORE_DEVICE_H */
