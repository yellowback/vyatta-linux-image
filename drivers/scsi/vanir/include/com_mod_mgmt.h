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
#ifndef __MV_MODULE_MGMT__
#define __MV_MODULE_MGMT__

#include "com_define.h"
#include "com_type.h"
#include "com_util.h"

enum {
	/* module status (module_descriptor) */
	MV_MOD_VOID   = 0,
	MV_MOD_UNINIT,
	MV_MOD_REGISTERED,  /* module ops pointer registered */
	MV_MOD_INITED,      /* resource assigned */ 
	MV_MOD_FUNCTIONAL,
	MV_MOD_STARTED,
	MV_MOD_DEINIT,      /* extension released, be gone soon */
	MV_MOD_GONE,
};



struct mv_mod_desc;


struct mv_mod_res {
	List_Head      res_entry;
	MV_PHYSICAL_ADDR       bus_addr;
	MV_PVOID               virt_addr;
	
	MV_U32                 size;
	
	MV_U16                 type;          /* enum Resource_Type */
	MV_U16                 align;
};

typedef struct _Module_Interface
{
	MV_U8      module_id;
	MV_U32     (*get_res_desc)(enum Resource_Type type, MV_U16 maxIo);
	MV_VOID    (*module_initialize)(MV_PVOID extension,
					MV_U32   size,
					MV_U16   max_io);
	MV_VOID    (*module_start)(MV_PVOID extension);
	MV_VOID    (*module_stop)(MV_PVOID extension);
	MV_VOID    (*module_notification)(MV_PVOID extension, 
					  enum Module_Event event, 
					  struct mod_notif_param *param);
	MV_VOID    (*module_sendrequest)(MV_PVOID extension, 
					 PMV_Request pReq);
	MV_VOID    (*module_reset)(MV_PVOID extension);
	MV_VOID    (*module_monitor)(MV_PVOID extension);
	MV_BOOLEAN (*module_service_isr)(MV_PVOID extension);
} Module_Interface, *PModule_Interface;

#define mv_module_ops _Module_Interface

#define mv_set_mod_ops(_ops, _id, _get_res_desc, _init, _start,            \
		       _stop, _send, _reset, _mon, _send_eh, _isr, _xor)   \
           {                                                               \
		   _ops->id                      = id;                     \
		   _ops->get_res_desc            = _get_res_desc;          \
		   _ops->module_initialize       = _init;                  \
		   _ops->module_start            = _start;                 \
		   _ops->module_stop             = _stop;                  \
		   _ops->module_sendrequest      = _send;                  \
		   _ops->module_reset            = _reset;                 \
		   _ops->module_monitor          = _mon;                   \
		   _ops->module_send_eh_request  = _send_eh;               \
		   _ops->module_service_isr      = _isr;                   \
		   _ops->module_send_xor_request = _xor;                   \
	   }


/* module descriptor */
struct mv_mod_desc {
	List_Head          mod_entry;      /* kept in a list */
	
	struct mv_mod_desc         *parent;
	struct mv_mod_desc         *child;

	MV_U32                     extension_size;
	MV_U8                      status;
	MV_U8                      ref_count;
	MV_U8                      module_id;
	MV_U8                      res_entry;

	MV_PVOID                   extension;      /* module extention */
	struct mv_module_ops       *ops;           /* interface operations */

	struct mv_adp_desc         *hba_desc;
	List_Head           res_list;
};

#endif /* __MV_MODULE_MGMT__ */
