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
#include "core_manager.h"
#include "core_resource.h"
#include "core_error.h"
#include "core_util.h"
#include "core_sas.h"

/*
 * PAL Manager drive the event handling
 * 1. don't do event handling when this port has error requests in the queue
 *    so error handling state machine won't mess up with event handling
 */

static void pal_event_handling(domain_phy *phy, MV_U8 event_id);
static void pl_handle_hotplug_event(pl_root *root, domain_phy *phy);
static void pl_handle_unplug(pl_root *root, domain_phy *phy);
static void pl_handle_async_event(pl_root *root, domain_phy *phy);
void pl_handle_plugin(pl_root *root, domain_phy *phy);
void sas_handle_unplug(pl_root *root, domain_port *port, domain_phy *phy);
void sas_handle_plugin(pl_root *root, domain_port *port, domain_phy *phy);
void sata_handle_unplug(pl_root *root, domain_port *port, domain_phy *phy);
MV_VOID exp_handle_broadcast(pl_root *root, domain_phy *phy);

extern void update_phy_info(pl_root *root, domain_phy *phy);
extern void update_port_phy_map(pl_root *root, domain_phy *phy);
extern void sas_init_port(pl_root *root, domain_port *port);
extern void sata_init_port(pl_root *root, domain_port *port);
extern void free_port(pl_root *root, domain_port *port);
extern void exp_update_direct_attached_phys(domain_port *port);

MV_BOOLEAN pal_check_duplicate_event(pl_root *root, event_record *old_event, 
        MV_U8 phy_id, MV_U8 event_id)
{
        /* duplicate event on the same phy */
        if ((old_event->root == root) && (old_event->event_id == event_id) && 
                (old_event->phy_id == phy_id)) {
                return MV_TRUE;
        }

        /* duplicate broadcast for same wideport */
	if ((event_id == PL_EVENT_BROADCAST_CHANGE) &&
		(old_event->event_id == event_id) && 
        	(root->phy[old_event->phy_id].port) && 
		(old_event->root->phy[old_event->phy_id].port == root->phy[phy_id].port)) {
		return MV_TRUE;
        }

        /* duplicate phy change for same wideport */
        if ((event_id == PL_EVENT_PHY_CHANGE) && 
        	(old_event->event_id == event_id) && 
        	(root->phy[old_event->phy_id].port) && 
        	(old_event->root->phy[old_event->phy_id].port == root->phy[phy_id].port)) {
              return MV_TRUE;
        }

        return MV_FALSE;
}

void pal_notify_event(pl_root *root, MV_U8 phy_id, MV_U8 event_id)
{
	core_extension *core = (core_extension *)root->core;
	event_record *event, *tmp_event;

	if (core->state != CORE_STATE_STARTED)
		return;

	MV_ASSERT((event_id == PL_EVENT_PHY_CHANGE)
		|| (event_id == PL_EVENT_ASYNC_NOTIFY)
		|| (event_id == PL_EVENT_BROADCAST_CHANGE));

	/*
	 * If the incoming event already has a duplicate in the event queue,
	 * don't need to add it again.
	 */
	LIST_FOR_EACH_ENTRY_TYPE(tmp_event, &core->event_queue, event_record,
		queue_pointer) {
                if (pal_check_duplicate_event(root, tmp_event, phy_id, event_id)) {
                        CORE_EVENT_PRINT(("duplicate event 0x%x on root %p phy 0x%x.\n",\
				event_id, root, phy_id));
			return;
                }
	}

	/* save the event for late handling */
	event = get_event_record(root->lib_rsrc);
	if (event) {
		event->root = root;
		event->phy_id = phy_id;
		event->event_id = event_id;
		EVENT_SET_DELAY_TIME(event->handle_time, 500);
		List_AddTail(&event->queue_pointer, &core->event_queue);
	} else {
		CORE_EVENT_PRINT(("no resource to queue event 0x%x "
			"on root %p phy 0x%x.\n",
			event_id, root, phy_id));
	}
}
void core_clean_event_queue(core_extension *core)
{
	event_record *event = NULL;
	pl_root *root;
	
	while (!List_Empty(&core->event_queue)) {
		event = (event_record *)List_GetFirstEntry(
			&core->event_queue, event_record, queue_pointer);
		root = event->root;
		free_event_record(root->lib_rsrc, event);
	}
}
void core_handle_event_queue(core_extension *core)
{
	event_record *event = NULL;
	MV_BOOLEAN ret;
	pl_root *root;
	domain_phy *phy;
	event_record *old_event = NULL;
	domain_port *port;
	MV_U32 i;

	while (!List_Empty(&core->event_queue)) {
		event = (event_record *)List_GetFirstEntry(
			&core->event_queue, event_record, queue_pointer);
		if (event == old_event) {
			List_Add(&event->queue_pointer, &core->event_queue);
			return;
		}

		root = event->root;
		phy = &root->phy[event->phy_id];
		port = phy->port;


		for (i = 0; i < root->phy_num; i++) {
			port = &root->ports[i];
			/* if there is init request running,
			 * don't do the event handling
			 * it can be two cases,
			 * a. during bootup(init state machine), event comes.
			 * b. hotplug comes again before the former one completes.
			 * we won't do multiple event handling on the same port */
			if (port_has_init_req(root, port)) {
				if (old_event == NULL) old_event = event;
                                List_Add(&event->queue_pointer, &core->event_queue);
				return;
			}
		}
		
		if (event->event_id == PL_EVENT_PHY_CHANGE) {
			if (MV_FALSE == time_is_expired(event->handle_time)) {
				List_Add(&event->queue_pointer, &core->event_queue);
				return;
			}
		}

		pal_event_handling(phy, event->event_id);
		free_event_record(root->lib_rsrc, event);
	}
}

extern MV_VOID pm_hot_plug(domain_pm *pm);
static void pal_event_handling(domain_phy *phy, MV_U8 event_id)
{
        pl_root *root = phy->root;
        MV_U32 reg = 0;
	MV_U16 dev_id = 0xFFFF;
	domain_device *dev  = NULL;

        CORE_EVENT_PRINT(("handle event: phy id %d, event id %d.\n", phy->id, event_id));
	switch (event_id) {
	case PL_EVENT_PHY_CHANGE:
		CORE_EVENT_PRINT(("phy change event: phy %d, irq_status is 0x%x\n", \
			phy->id, phy->irq_status));

		pl_handle_hotplug_event(root, phy);
		break;

	case PL_EVENT_ASYNC_NOTIFY:
		CORE_EVENT_PRINT(("asynchronous notification event: " \
			"root %p phy id %d phy type =0x%x \n", root, phy->id,phy->type));
			
		if ((phy->port)&&(phy->port->type == PORT_TYPE_SATA) && (phy->port->pm)) {
			pm_hot_plug(phy->port->pm);
		} else if ((phy->port) && (phy->type == PORT_TYPE_SATA)) { 
			reg = MV_REG_READ_DWORD(root->rx_fis,SATA_UNASSOC_D2H_FIS(phy) + 0x0);
			if (((reg & 0xFF) == SATA_FIS_TYPE_SET_DEVICE_BITS) && (reg & MV_BIT(15))) {
				dev_id = get_id_by_phyid(root->lib_dev,phy->id);
				dev = (domain_device *)get_device_by_id(root->lib_dev, dev_id);
	  			mv_cancel_timer(root->core,&dev->base);
				pal_abort_running_req_for_async_sdb_fis(root, dev);

				CORE_EVENT_PRINT(("Notify device %d async event(0x%x) to API.\n",dev_id,PL_EVENT_SDB_ASYNC_NOTIFY));
				core_generate_event(root->core, PL_EVENT_SDB_ASYNC_NOTIFY, dev_id,SEVERITY_OTHER, 0, NULL,0);

				CORE_EVENT_PRINT(("Sending Ack of SDB FIS to FW.\n"));
				pl_handle_async_event(root,phy);
			}
		}
		break;

	case PL_EVENT_BROADCAST_CHANGE:
		CORE_EVENT_PRINT(("broadcast event: root %p phy %d.\n", \
			root, phy->id));
		exp_handle_broadcast(root, phy);
		break;

	default:
		MV_ASSERT(MV_FALSE);
	}
}

static void pl_handle_hotplug_event(pl_root *root, domain_phy *phy)
{
	MV_U32 irq = phy->irq_status;
	MV_U16 j = 0;
	domain_port *port = phy->port;

	CORE_EVENT_PRINT(("phy %d irq_status = 0x%x.\n", phy->id, irq));
	if (irq & IRQ_PHY_RDY_CHNG_1_TO_0) {
		if (!mv_is_phy_ready(root, phy)) {
			pl_handle_unplug(root, phy);
		} else {
			CORE_EVENT_PRINT(("unplug interrupt but "\
				"phy is still connected.\n"));
			pl_handle_unplug(root, phy);
			pl_handle_plugin(root, phy);
		}
	} else if (irq & IRQ_PHY_RDY_CHNG_MASK) {
		/* wait for PHY to be ready */
		while (j < 500) {
			core_sleep_millisecond(root->core, 10);
			if (mv_is_phy_ready(root, phy))
				break;
			j++;
		}

		if (mv_is_phy_ready(root, phy)) {
			if (port &&
				((port->device_count!=0) || (port->pm!=NULL) || (port->expander_count!=0))
				) {
				pl_handle_unplug(root, phy);
			}
			pl_handle_plugin(root, phy);
		} else {
			CORE_EVENT_PRINT(("plugin interrupt but phy is gone.\n"));
		}
	} else {
		CORE_EVENT_PRINT(("nothing to do.\n"));
	}

	phy->irq_status &= ~(IRQ_PHY_RDY_CHNG_MASK|IRQ_PHY_RDY_CHNG_1_TO_0);
}

static void pl_handle_unplug(pl_root *root, domain_phy *phy)
{
	domain_port *port = phy->port;

	if (NULL == port) {
		CORE_EVENT_PRINT(("unplug phy %d which not exist.\n",
			phy->id));
		return;
	}
	if (port->type & PORT_TYPE_SATA) {
		sata_handle_unplug(root, port, phy);
	} else {
		sas_handle_unplug(root, port, phy);
	}
	CORE_DPRINT(("phy %d is now gone\n", phy->id));
}

void pl_handle_plugin(pl_root *root, domain_phy *phy)
{
	domain_port *port;
	MV_U32 reg = 0;

	update_phy_info(root, phy);
	update_port_phy_map(root, phy);
	port = phy->port;

	if (port == NULL) {
		return;
	}

	if (port->type & PORT_TYPE_SATA) {
	
		reg = READ_PORT_IRQ_STAT(root, phy);
		if (reg & IRQ_UNASSOC_FIS_RCVD_MASK) {
			WRITE_PORT_IRQ_STAT(root, phy, IRQ_UNASSOC_FIS_RCVD_MASK);
		} 
		mv_reset_phy(root, port->phy_map, MV_TRUE);

		core_sleep_millisecond(root->core, 100);
		sata_init_port(root, port);
	} else {
		sas_handle_plugin(root, port, phy);
	}
}

void sas_handle_plugin(pl_root *root, domain_port *port, domain_phy *phy)
{
	MV_U8 i;
	MV_U16 j = 0;
	MV_U8 phyrdy_check_mask = 0;
	core_extension *core = root->core;
	
	if(((core->revision_id != VANIR_C1_REV) && (core->revision_id != VANIR_C2_REV))
           && (port->phy_num == 1))
	{
		WRITE_PORT_PHY_CONTROL(root, phy, 0x1);
		
		/* Check Other phys that need to handle plugin event.*/
		for (i = 0; i < root->phy_num; i++) {
			if (root->phy[i].port == NULL)
				phyrdy_check_mask |= MV_BIT(i);
		}

		while ((j < 10) && phyrdy_check_mask) {
			for (i = 0; i < root->phy_num; i++) {
				if ((phyrdy_check_mask & MV_BIT(i))	&&
					(mv_is_phy_ready(root, (&root->phy[i])))) {
						MV_DPRINT(("CORE: STP Hotplug WorkAround Phy%d STP Link Rst\n", i));
						WRITE_PORT_PHY_CONTROL(root, (&root->phy[i]), 0x1);
						phyrdy_check_mask &= ~MV_BIT(i);
				}
			}
			core_sleep_millisecond(root->core, 10);
			j++;
		}
	}

	if (!List_Empty(&port->expander_list)) {
		/* part of a wide port. update phy info */
		MV_ASSERT(port->att_dev_info & (PORT_DEV_SMP_TRGT | PORT_DEV_STP_TRGT));
		exp_update_direct_attached_phys(port);
	} else {
		sas_init_port(root, port);
	}
}

void sas_handle_unplug(pl_root *root, domain_port *port, domain_phy *phy)
{
	 domain_expander *exp = NULL;
	 MV_U8 i, phy_id = 0;
        MV_U8 phy_map_left;

        domain_phy *tmp_phy;
        phy_map_left = port->phy_map;
        core_sleep_millisecond(root->core, 100);
        for (i = 0; i < root->phy_num; i++) {
                tmp_phy = &root->phy[i];
                if (phy_map_left & MV_BIT(i)) {
  	                update_phy_info(root, tmp_phy);
	                update_port_phy_map(root, tmp_phy);
                }
        }
        phy_map_left = port->phy_map;
        CORE_EVENT_PRINT(("handle unplug event port %p phy id %d, phy_map 0x%x.\n", \
                port, phy->id, phy_map_left));

	if (phy_map_left) {
		if (!List_Empty(&port->expander_list)) {
			pal_abort_port_running_req(root, port);
			exp_update_direct_attached_phys(port);
			LIST_FOR_EACH_ENTRY_TYPE(exp, &port->expander_list,
				domain_expander, base.queue_pointer) {
				pal_clean_expander_outstanding_req(root, exp);
			}
		} else 
			pal_set_down_port(root, port);
	} else {
		pal_set_down_port(root, port);
	}
}

void sata_handle_unplug(pl_root *root, domain_port *port, domain_phy *phy)
{
	domain_device *device;
	MV_U32 reg;

	/* it means directly attached disk is unplugged
	 * or pm is unplugged. */
	pal_set_down_port(root, port);
	mv_reset_stp(root, port->phy_map);
	update_phy_info(root, phy);
}

MV_VOID exp_handle_broadcast(pl_root *root, domain_phy *phy)
{
	domain_port *port = phy->port;
	domain_expander *exp;
	core_extension *core = (core_extension *)root->core;
       MV_U8 phy_map_left;
       MV_U8 i;
       domain_phy *tmp_phy;
       
	if (!port)
		return;

	CORE_DPRINT(("double checking phy status before broadcast\n"));

       phy_map_left = port->phy_map;
       core_sleep_millisecond(root->core, 100);
       core_sleep_millisecond(root->core, 100);
       for (i = 0; i < root->phy_num; i++) {
           tmp_phy = &root->phy[i];
           if (phy_map_left & MV_BIT(i)) {
	      	    update_phy_info(root, tmp_phy);
	    	    update_port_phy_map(root, tmp_phy);
           }
       }
       phy_map_left = port->phy_map;

	if (phy_map_left == 0) {
	    /* remove devices attached. complete all requests */
	    pal_set_down_port(root, port);

	    /* port variables are cleared already in update_port_phy_map */
           return;
       }

       exp_update_direct_attached_phys(port);

	/* clear old routing tables & find root expander on this port */
	LIST_FOR_EACH_ENTRY_TYPE(exp, &port->expander_list, domain_expander,
		base.queue_pointer) {
		MV_ZeroMemory(exp->route_table, sizeof(MV_U16)*MAXIMUM_EXPANDER_PHYS);
		if (exp->base.parent == &port->base) {
			exp->state = EXP_STATE_DISCOVER;
			core_queue_init_entry(root, &exp->base, MV_TRUE);			
		}
	}
}

static void handle_async_event_callback(MV_PVOID root_p, PMV_Request req)
{	
	domain_device *device  = NULL;
	pl_root *root = (pl_root *)root_p;
	
	device = (domain_device*)get_device_by_id(root->lib_dev,req->Device_Id ); 
	if(device)
		CORE_EVENT_PRINT(("Sending ACK of SDB FIS finished,device->status = 0x%x\n",device->status));	
}

static void pl_handle_async_event(pl_root *root, domain_phy *phy)
{
	MV_Request *req;
	domain_device *device  = NULL;

	req = get_intl_req_resource(root, 0);
	if (req == NULL)
		return;
	
	req->Cdb[0] = SCSI_CMD_ATA_PASSTHRU_12;
	req->Cdb[1] = ATA_PROTOCOL_NON_DATA << 1;
	req->Cdb[2] = 0;
	req->Cdb[3] = MARVELL_VU_CMD_ASYNC_NOTIFY; 
	req->Cdb[4] = 0;
	req->Cdb[5] = 0;
	req->Cdb[6] = 0;
	req->Cdb[7] = 0;
	req->Cdb[8] = 0;
	req->Cdb[9] = SCSI_CMD_MARVELL_VENDOR_UNIQUE;
	req->Cdb[10] = 0;
	req->Cdb[11] = 0;
	
	req->Time_Out = CORE_REQUEST_TIME_OUT_SECONDS;
	req->Cmd_Flag = CMD_FLAG_NON_DATA;
	
	req->Device_Id = get_id_by_phyid(root->lib_dev,phy->id);
	device = (domain_device*)get_device_by_id(root->lib_dev,req->Device_Id ); 
	device->status |= DEVICE_STATUS_WAIT_ASYNC;
	
	req->Completion = handle_async_event_callback;	
	core_append_request(root, req);

}

