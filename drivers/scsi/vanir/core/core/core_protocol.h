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
#ifndef __CORE_PROTOCOL_H
#define __CORE_PROTOCOL_H

#include "mv_config.h"
#include "core_type.h"
#include "core_internal.h"

typedef struct _hw_buf_wrapper {
	struct _hw_buf_wrapper		*next;
	MV_PVOID					vir;	/* virtual memory address */
	MV_PHYSICAL_ADDR			phy;	/* physical memory address */
} hw_buf_wrapper;

typedef struct _saved_fis {
	MV_U32 dw1;
	MV_U32 dw2;
	MV_U32 dw3;
	MV_U32 dw4;
} saved_fis;

struct _pl_root {
	MV_LPVOID	mmio_base;
	MV_PVOID	core;
	lib_device_mgr	 *lib_dev;
	lib_resource_mgr *lib_rsrc;

	MV_U64      sata_reg_set;		   /* bit map. need to change if register set is more than 64 */
	MV_U32		phy_num;
	MV_U32		max_register_set;	   /* per chip core */
	MV_U32       running_num; /* how many requests are outstanding on the ASIC */

	MV_U32		slot_count_support;
	MV_U32		unassoc_fis_offset;
	MV_U32		capability;

	PMV_Request	*running_req;
	saved_fis	*saved_fis_area;

	domain_port	ports[MAX_PORT_PER_PL];
	domain_phy	phy[MAX_PHY_PER_PL];

	Tag_Stack slot_pool;

	MV_U16 delv_q_size;
	MV_U16 cmpl_q_size;
	MV_U16 last_delv_q;
	MV_U16 last_cmpl_q;

	/* variables to save the interrupt status */
	MV_U32		comm_irq;
	MV_U32          comm_irq_mask;

	/*keeps the base phy num of the root*/
	MV_U32		base_phy_num;

	/* command header list */
	MV_PVOID cmd_list;
	MV_PHYSICAL_ADDR cmd_list_dma;

	/* command table */
	hw_buf_wrapper *cmd_table_wrapper;

	/* received FIS */
	MV_PVOID rx_fis;
	MV_PHYSICAL_ADDR rx_fis_dma;

	/* delivery queue */
	MV_PU32 delv_q;
	MV_PHYSICAL_ADDR delv_q_dma;

	/* completion queue: cmpl_wp is the write pointer. cmpl_q is the real queue entry 0 */
	MV_PVOID cmpl_wp;
	MV_PHYSICAL_ADDR cmpl_wp_dma;
	MV_PU32  cmpl_q;
	MV_PHYSICAL_ADDR cmpl_q_dma;

	MV_U32 max_cmd_slot_width;
};

#define MAX_EVENTS 20
typedef struct _event_record{
	List_Head queue_pointer;
	struct _pl_root *root;
	MV_U8 phy_id;
	MV_U8 event_id;
	unsigned long handle_time;
} event_record;

enum _core_context_type {
	CORE_CONTEXT_TYPE_NONE = 0, /* no valid context */
	CORE_CONTEXT_TYPE_CDB =	1, /* context is the cdb and buffer */
	CORE_CONTEXT_TYPE_LARGE_REQUEST = 2, /* context for the large request support */
        CORE_CONTEXT_TYPE_SUB_REQUEST = 3,
        CORE_CONTEXT_TYPE_ORG_REQ = 4, /* Used in SAT and error handling */
	CORE_CONTEXT_TYPE_API = 5,
	CORE_CONTEXT_TYPE_RESET_SATA_PHY = 6,
	CORE_CONTEXT_TYPE_CLEAR_AFFILIATION = 7,
	CORE_CONTEXT_TYPE_MAX,
};

enum _core_req_type {
	CORE_REQ_TYPE_NONE = 0,
	CORE_REQ_TYPE_INIT = 1, /* request generated in init state machine */
	CORE_REQ_TYPE_ERROR_HANDLING = 2, /* request generated during error handling */
	CORE_REQ_TYPE_RETRY = 3, /* is error handling request too, retry org req */
};

enum _core_req_flag {
	CORE_REQ_FLAG_NEED_D2H_FIS	= (1 << 0),
};

/* eh req includes retried req */
#define CORE_IS_EH_RETRY_REQ(ctx) \
	((ctx)->req_type==CORE_REQ_TYPE_RETRY)
#define CORE_IS_EH_REQ(ctx) \
	(((ctx)->req_type==CORE_REQ_TYPE_ERROR_HANDLING) \
		|| ((ctx)->req_type==CORE_REQ_TYPE_RETRY))
#define CORE_IS_INIT_REQ(ctx) \
	((ctx)->req_type==CORE_REQ_TYPE_INIT)

enum _error_info {
	EH_INFO_CMD_ISS_STPD = (1U << 0),
	EH_INFO_NEED_RETRY = (1U << 1),
	EH_INFO_STP_WD_TO_RETRY = (1U << 2),
	EH_INFO_SSP_WD_TO_RETRY = (1U << 3),
};

#define EH_INFO_WD_TO_RETRY (EH_INFO_STP_WD_TO_RETRY | EH_INFO_SSP_WD_TO_RETRY)

/*
 * core context is used for one time request handling.
 * when calling Completion routine, the core_context will be released.
 * even you want to retry the request, the core cotext will be a new one.
 * so dont try to save information like retry_count in core context
 */
typedef struct _core_context {
	struct _core_context *next;

	/*
	 * following is the request context
	 */
	MV_U16 slot; /* which slot it is used */
	MV_U8 type; /* for the following union data structure CORE_CONTEXT_TYPE_XXX */
	MV_U8 req_type; /* core internal request type CORE_REQ_TYPE_XXX */
	MV_U32 req_state;

	MV_U8  ncq_tag;
	MV_U8  req_flag;	/* core internal request flag CORE_REQ_FLAG_XXX */
	MV_U8  reserved[2];

	MV_PVOID buf_wrapper; /* wrapper for the scratch buffer */
	MV_PVOID sg_wrapper; /* wrapper for the hw sg buffer */

	/*
	 *
	 */
	MV_PVOID handler;

	/*
	 * for error handling
	 */
	MV_U32 error_info;

        MV_PVOID received_fis;
        union {
                struct {
                        MV_Request *org_req;
                        MV_U32 other;
                } org;
                struct {
                        MV_U8 sub_req_cmplt;
                        MV_U8 sub_req_count;
                } large_req;
                struct {
                        MV_PVOID org_buff_ptr;
                } sub_req;
                /* for smp virtual reqs */
                struct {
                        MV_U8 current_phy_id;
                        MV_U8 req_remaining;
                } smp_discover;
		struct {
                        MV_U8 curr_dev_count;
                        MV_U8 total_dev_count;
                        MV_U8 req_remaining;
		} smp_reset_sata_phy;
		struct {
			MV_U8 state;
			MV_U8 curr_dev_count;
                        MV_U8 total_dev_count;
			MV_U8 req_remaining;
			MV_U8 need_wait;
		} smp_clear_aff;
                struct {
                        MV_U8 phy_count;
                        MV_U8 address_count;
                        MV_U8 current_phy;
                        MV_U8 current_addr;
                        MV_U8 req_remaining;
                        MV_PVOID org_exp;
                } smp_config_route;
                /*for API expander request*/
                struct{
                        MV_U16 start;
                        MV_U16 end;
                        MV_U32 remaining;
                        MV_PVOID pointer;
                }api_req;
                struct{
                        MV_U16 phy_index;
                        MV_PVOID buffer;
                }api_smp;
		struct{
			MV_U8 affiliation_valid;
		}smp_report_phy;
        } u;
} core_context;

#define DC_ATA                                      MV_BIT(0)
#define DC_SCSI                                     MV_BIT(1)
#define DC_SERIAL                                   MV_BIT(2)
#define DC_PARALLEL                                 MV_BIT(3)
#define DC_ATAPI                                    MV_BIT(4)
#define DC_SGPIO                                    MV_BIT(5)
#define DC_I2C                                      MV_BIT(6)

/* PD's Device type defined in SCSI-III specification */
#define DT_DIRECT_ACCESS_BLOCK                      0x00
#define DT_SEQ_ACCESS                               0x01
#define DT_PRINTER                                  0x02
#define DT_PROCESSOR                                0x03
#define DT_WRITE_ONCE                               0x04
#define DT_CD_DVD                                   0x05
#define DT_OPTICAL_MEMORY                           0x07
#define DT_MEDIA_CHANGER                            0x08
#define DT_STORAGE_ARRAY_CTRL                       0x0C
/* an actual enclosure */
#define DT_ENCLOSURE                                0x0D

/* The following are defined by Marvell */
#define DT_EXPANDER                                 0x20
#define DT_PM                                       0x21
#define DT_SES_DEVICE                               0x22

#define IS_SSP(dev) ((dev->connection & DC_SCSI) && \
                     (dev->connection & DC_SERIAL) && \
                     !(dev->connection & DC_ATA))

#define IS_STP(dev) ((dev->connection & DC_SCSI) && \
                     (dev->connection & DC_SERIAL) && \
                     (dev->connection & DC_ATA))

#define IS_SATA(dev) (!(dev->connection & DC_SCSI) && \
                      (dev->connection & DC_SERIAL) && \
                      (dev->connection & DC_ATA))

#define IS_STP_OR_SATA(dev) ((dev->connection & DC_ATA) && \
                             (dev->connection & DC_SERIAL))

#define IS_ATAPI(dev) ((dev->connection & DC_ATAPI) && \
                       (dev->connection & DC_ATA))

#define IS_TAPE(dev)           (dev->dev_type == DT_SEQ_ACCESS)
#define IS_ENCLOSURE(dev)      (dev->dev_type == DT_ENCLOSURE)
#define IS_HDD(dev)            (dev->dev_type == DT_DIRECT_ACCESS_BLOCK)
#define IS_OPTICAL(dev)        ((dev->dev_type == DT_WRITE_ONCE) || \
                                (dev->dev_type == DT_CD_DVD) || \
                                (dev->dev_type == DT_OPTICAL_MEMORY))

#define IS_SGPIO(dev)          (dev->connection & DC_SGPIO)
#define IS_I2C(dev)            (dev->connection & DC_I2C)

#define IS_BEHIND_PM(dev)      (dev->pm != NULL)

MV_BOOLEAN prot_init_pl(pl_root *root, MV_U16 max_io,
	MV_PVOID core, MV_LPVOID mmio,
	lib_device_mgr *lib_dev, lib_resource_mgr *lib_rsrc);

MV_QUEUE_COMMAND_RESULT prot_send_request(pl_root *root,
	struct _domain_base *base, MV_Request *req);

MV_U32 prot_get_delv_q_entry(pl_root *root);
MV_VOID prot_write_delv_q_entry(pl_root *root, MV_U32 entry);

PMV_Request get_intl_req_resource(pl_root *root, MV_U32 buf_size);
MV_VOID intl_req_release_resource(lib_resource_mgr *rsrc, PMV_Request req);

MV_VOID io_chip_clear_int(pl_root *root);
MV_VOID io_chip_handle_int(pl_root *root);

void prot_fill_sense_data(MV_Request *req, MV_U8 sense_key,
	MV_U8 ad_sense_code);

MV_VOID prot_clean_slot(pl_root *root, domain_base *base, MV_U16 slot,
	MV_Request *req);

err_info_record * prot_get_command_error_info(mv_command_table *cmd_table,
	 MV_PU32 cmpl_q);
#endif /* __CORE_PROTOCOL_H */
