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
#ifndef __HBA_TIMER_H__
#define __HBA_TIMER_H__

#include "com_tag.h"
#include "hba_exp.h"

enum _tag_hba_msg_state{
	MSG_QUEUE_IDLE=0,
	MSG_QUEUE_PROC,
	MSG_QUEUE_NO_START
};

struct mv_hba_msg {
	MV_PVOID data;
	MV_U32   msg;
	MV_U32   param;
	List_Head msg_list;
};

struct mv_hba_msg_queue {
	spinlock_t lock;
	List_Head free;
	List_Head tasks;
	struct mv_hba_msg msgs[MSG_QUEUE_DEPTH];
};

enum {
	HBA_TIMER_IDLE = 0,
	HBA_TIMER_RUNNING,
	HBA_TIMER_LEAVING,
};
void hba_house_keeper_init(void);
void hba_house_keeper_run(void);
void hba_house_keeper_exit(void);
void hba_msg_insert(void *data, unsigned int msg, unsigned int param);
void hba_init_timer(PMV_Request req);
void hba_remove_timer(PMV_Request req);
void hba_remove_timer_sync(PMV_Request req);
void hba_add_timer(PMV_Request req, int timeout,
		   MV_VOID (*function)(MV_PVOID data));
extern struct mv_lu *mv_get_device_by_target_lun(struct hba_extension *hba, MV_U16  target_id, MV_U16 lun);
extern struct mv_lu *mv_get_avaiable_device(struct hba_extension * hba, MV_U16 target_id, MV_U16 lun);
extern MV_U16 get_id_by_targetid_lun(MV_PVOID ext, MV_U16 id,MV_U16 lun);

#define TIMER_INTERVAL_OS		1000		/* millisecond */
#define TIMER_INTERVAL_LARGE_UNIT	500		/* millisecond */
#define TIMER_INTERVAL_SMALL_UNIT	100		/* millisecond */

#endif /* __HBA_TIMER_H__ */


