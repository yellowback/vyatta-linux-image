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
#include "core_sas.h"
#include "core_internal.h"
#include "core_manager.h"
#include "core_util.h"

#include "core_error.h"

PMV_Request sas_make_read_capacity_req(pl_root *root, domain_device *device);
PMV_Request sas_make_start_stop_unit_req(pl_root *root,
	domain_device *device, MV_BOOLEAN start);
PMV_Request sas_make_test_unit_ready_req(pl_root *root, domain_device *device,
	MV_U8 control);
static void sas_init_req_callback(MV_PVOID root_p, PMV_Request req);
extern void sas_init_target_device(pl_root *root, domain_port *port,domain_device *dev,MV_U16 targetid, MV_U16 lun);
extern void get_new_and_update_base_id(pl_root *root,domain_base *base);

MV_BOOLEAN sas_init_state_machine(MV_PVOID dev)
{
	domain_device *device = (domain_device *)dev;
	domain_port *port = device->base.port;
	pl_root *root = device->base.root;
	PMV_Request req = NULL;
	domain_phy *phy;
	MV_U8 i;

	CORE_DPRINT(("dev %d state 0x%x.\n", device->base.id, device->state));

	switch (device->state) {
	case DEVICE_STATE_RESET_DONE:
		if (!IS_BEHIND_EXP(device)) {
			core_sleep_millisecond(root->core, 1);
			for (i = 0; i < root->phy_num; i++) {
				if (port->phy_map & MV_BIT(i)) {
					phy = &root->phy[i];
					WRITE_PORT_CONFIG_ADDR(root, phy,
						CONFIG_PHY_CONTROL);
					WRITE_PORT_CONFIG_DATA(root, phy, 0x00);
				}
			}
		}

		/* to do standard inquiry */
		req = sas_make_inquiry_req(root, device, MV_FALSE, 0,
			sas_init_req_callback);
		break;

	case DEVICE_STATE_INQUIRY_DONE:
		/* to do inquiry about vital product data and find WWN */
		req = sas_make_inquiry_req(root, device, MV_TRUE, 0x83,
			sas_init_req_callback);
		break;

	case DEVICE_STATE_INQUIRY_EVPD_DONE:
		if (core_check_duplicate_device(root, device) == MV_TRUE) {
			CORE_PRINT(("Duplicate SAS Address. Set down\n"));
			core_init_entry_done(root, device->base.port, NULL);
			pal_set_down_disk(root, device, MV_FALSE);
			return MV_TRUE;
		}
		device->state = DEVICE_STATE_CHECK_DUPLICATE_DONE;

	case DEVICE_STATE_CHECK_DUPLICATE_DONE:
		if (!IS_BEHIND_EXP(device)) {
			if (SAS_SLOW_SPINUP(device))
				core_sleep_millisecond(root->core, 500);
			else
				core_sleep_millisecond(root->core, 1);
			for (i = 0; i < root->phy_num; i++) {
				if (port->phy_map & MV_BIT(i)) {
					phy = &root->phy[i];
					WRITE_PORT_CONFIG_ADDR(root, phy,
						CONFIG_PHY_CONTROL);
					WRITE_PORT_CONFIG_DATA(root, phy, 0x04);
				}
			}
		}

		/* to do start unit */
		req = sas_make_start_stop_unit_req(root, device, MV_TRUE);
		break;

	case DEVICE_STATE_STARTSTOP_DONE:
		/* to do test unit ready */
		req = sas_make_test_unit_ready_req(root, device, 0);
		break;

	case DEVICE_STATE_TEST_UNIT_READY_DONE:
		/* to do read capacity */

		req = sas_make_read_capacity_req(root, device);
		break;

	case DEVICE_STATE_READ_CAPACITY_DONE:
                req = sas_make_marvell_specific_req(device,
                        CDB_CORE_ENABLE_READ_AHEAD,
                        sas_init_req_callback);
		break;

	case DEVICE_STATE_SET_READ_AHEAD_DONE:
		req = sas_make_marvell_specific_req(device,
			CDB_CORE_ENABLE_WRITE_CACHE,
			sas_init_req_callback);
		break;

	case DEVICE_STATE_SET_CACHE_ENABLE_DONE:
		req = sas_make_log_sense_req(device,
			0x00, sas_init_req_callback);
		break;
	case DEVICE_STATE_LOG_SENSE_DONE:
	case DEVICE_STATE_REPORT_LUN:
		if (device->base.LUN == 0) {
			req = sas_make_report_lun_req(root,device,
				sas_init_req_callback);
			break;
		}
		/* initialization procedure is done. */
		device->state = DEVICE_STATE_INIT_DONE;
	case DEVICE_STATE_INIT_DONE:
	default:
		core_init_entry_done(root, port, dev);
		return MV_TRUE;
	}

	if (req != NULL) {
		core_append_init_request(root, req);
		return MV_TRUE;
	} else {
		return MV_FALSE;
	}
}

void sas_inquiry_callback(domain_device *dev, PMV_Request req);
void sas_report_luns_callback(domain_device *dev, PMV_Request req);
void sas_read_capacity_callback(domain_device *dev, PMV_Request req);
void sas_start_stop_unit_callback(domain_device *dev, PMV_Request req);
void sas_test_unit_ready_callback(domain_device *dev, MV_Request *req);
void sas_log_sense_callback(domain_device *dev, PMV_Request req);
MV_VOID sas_enable_read_ahead_callback(MV_Request *req, domain_device *dev);
MV_VOID sas_enable_write_cache_callback(MV_Request *req, domain_device *dev);

static void sas_init_req_callback(MV_PVOID root_p, PMV_Request req)
{
	pl_root *root = (pl_root *)root_p;
	domain_device *dev;

	dev = (domain_device *)get_device_by_id(root->lib_dev, req->Device_Id);
	MV_ASSERT(dev != NULL);

	if (req->Cdb[0] == SCSI_CMD_MARVELL_SPECIFIC && 
		req->Cdb[1] == CDB_CORE_MODULE) {
		switch (req->Cdb[2]) {
		case CDB_CORE_ENABLE_READ_AHEAD:
			sas_enable_read_ahead_callback(req, dev);
			break;
		case CDB_CORE_ENABLE_WRITE_CACHE:
			sas_enable_write_cache_callback(req, dev);
			break;
		default:
			MV_ASSERT(MV_FALSE);
			break;
		}
	
	} else if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		core_handle_init_error(root, &dev->base, req);
		return;
	}

	switch (req->Cdb[0]) {
	case SCSI_CMD_INQUIRY:
		sas_inquiry_callback(dev, req);
		break;

	case SCSI_CMD_READ_CAPACITY_10:
	case SCSI_CMD_READ_CAPACITY_16:
		sas_read_capacity_callback(dev, req);
		break;
	case SCSI_CMD_START_STOP_UNIT:
		sas_start_stop_unit_callback(dev, req);
		break;
	case SCSI_CMD_TEST_UNIT_READY:
		sas_test_unit_ready_callback(dev, req);
		break;
	case SCSI_CMD_LOG_SENSE:
		sas_log_sense_callback(dev, req);
		break;
	case SCSI_CMD_MARVELL_SPECIFIC:
		break;
	case SCSI_CMD_REPORT_LUN:
		sas_report_luns_callback(dev, req);
		break;
	default:
		MV_ASSERT(MV_FALSE);
		break;
	}
	if(dev->dev_type != DT_ENCLOSURE
		&& dev->dev_type != DT_SES_DEVICE)
	core_queue_init_entry(root, &dev->base, MV_FALSE);
}


PMV_Request sas_make_read_capacity_req(pl_root *root, domain_device *device)
{
	PMV_Request req;
	MV_U32 buf_size;

	if (device->capability & DEVICE_CAPABILITY_PROTECTION_INFORMATION_SUPPORTED)
		buf_size = 0x20;
	else
		buf_size = 8;

	/* allocate resource */
	req = get_intl_req_resource(root, buf_size);
	if (req == NULL) return NULL;

	/* Prepare read capacity */
	if (device->capability & DEVICE_CAPABILITY_PROTECTION_INFORMATION_SUPPORTED) {
		req->Cdb[0] = SCSI_CMD_READ_CAPACITY_16;
		req->Cdb[1] = 0x10;
		req->Cdb[13] = 0x20;
	} else {
		req->Cdb[0] = SCSI_CMD_READ_CAPACITY_10;
	}

	req->Device_Id = device->base.id;

	req->Cmd_Flag = CMD_FLAG_DATA_IN;
	req->Completion = sas_init_req_callback;
	return req;
}

PMV_Request sas_make_start_stop_unit_req(pl_root *root,
	domain_device *device, MV_BOOLEAN start)
{
	PMV_Request req;
	MV_U32 buf_size = 0;

	/* allocate resource */
	req = get_intl_req_resource(root, 0);
	if (req == NULL) return NULL;

	/* Prepare start/stop unit command */
	req->Cdb[0] = SCSI_CMD_START_STOP_UNIT;
	req->Cdb[1] = (MV_TRUE==start)? 0: 1;	/* immediate */
	req->Cdb[4] = (MV_TRUE==start)? 1: 0;

	req->Device_Id = device->base.id;

	req->Cmd_Flag = 0;
	req->Completion = sas_init_req_callback;
	return req;
}

PMV_Request sas_make_test_unit_ready_req(pl_root *root, domain_device *device,
	MV_U8 control)
{
	PMV_Request req;

	req = get_intl_req_resource(root, 0);
	if (req == NULL) return NULL;

	/* Prepare start/stop unit command */
	req->Cdb[0] = SCSI_CMD_TEST_UNIT_READY;
	req->Cdb[5] = control;

	req->Device_Id = device->base.id;
	req->Completion = sas_init_req_callback;
	return req;
}

/*
 * state machine callback functions
 */
void parse_evpd_inquiry(domain_device *dev, MV_PU8 buf)
{
	MV_PU8 desc;
	MV_U32 vpd_len;
	MV_U8 fujitsu;

	/* it's the "device identification" page */
	if (buf[1] != 0x83)
		CORE_DPRINT(("device identification error %x.\n",buf[1]));
	MV_DASSERT(buf[1] == 0x83);

	vpd_len = ((buf[2] << 8) | buf[3]) + 4;
	desc = buf + 4;
	fujitsu = IS_FUJITSU(dev);

	/* See SPC 7.6.3 for Device Identification VPD page */
	while (desc < buf + vpd_len) {
		MV_U8 proto = desc[0] >> 4;
		MV_U8 code_set = desc[0] & 0xf;
		MV_U8 piv = desc[1] & 0x80;
		MV_U8 assoc = (desc[1] & 0x30) >> 4;
		MV_U8 ident_type = desc[1] & 0xf;
		MV_U8 len = desc[3];

		/* NAA identification type */
		if (!fujitsu) {
			if (piv && ident_type == 0x03 && assoc == 0x02 \
				&& code_set == 0x01 && len == 0x08 && proto == 0x06) {
				dev->WWN.value = MV_BE64_TO_CPU(*(MV_U64 *)&desc[4]);
				return;
			}
            		/* Association is zero for SAT device (SAT spec 2.0) */
			if (piv == 0 && ident_type == 0x03 && (assoc == 0x02 || assoc == 0x00)  \
				&& code_set == 0x01 && len == 0x08 && proto == 0) {
				dev->WWN.value = MV_BE64_TO_CPU(*(MV_U64 *)&desc[4]);
				return;
			}
		} else {
			if (ident_type == 0x03 && assoc == 0x0 && code_set == 0x01
				&& len == 0x08) {
				dev->WWN.value = MV_BE64_TO_CPU(*(MV_U64 *)&desc[4]);
				return;
			}
		}
		desc += len + 4;
	}

	CORE_PRINT(("Didn't find the WWN for device %d.\n", dev->base.id));
}

void parse_standard_inquiry(domain_device *dev, MV_PU8 buf)
{
	/* parse standard inquiry data to find out the device type */
	pl_root *root = dev->base.root;
        domain_port *port = dev->base.port;
	domain_enclosure *enc;
	if (buf[5]&0x01) {
		/* A PROTECT bit set to one indicates that
		 * the logical unit supports protection information. */
		dev->capability |= DEVICE_CAPABILITY_PROTECTION_INFORMATION_SUPPORTED;
	}
	if((buf[6]&0x40)||((buf[0]&0x1f)==0x0d))	{
		if((buf[0]&0x1f)==0x0d){
			dev->dev_type = DT_ENCLOSURE;
		}else
			dev->dev_type = DT_SES_DEVICE;

		enc = get_enclosure_obj(root, root->lib_rsrc);
		if (enc) {
		        if(dev->base.parent&&dev->base.parent->port){
			        set_up_new_enclosure(root, port, enc,
				        (command_handler *)
				        core_get_handler(root, HANDLER_ENC));
			        enc->base.parent = dev->base.parent;
			        enc->state = ENCLOSURE_INQUIRY_DONE;
			        enc->negotiated_link_rate = dev->negotiated_link_rate;
			        enc->setting = dev->setting;
			        enc->parent_phy_id = dev->parent_phy_id;
			        MV_CopyMemory(&enc->sas_addr, &dev->sas_addr, 8);
			        if(IS_BEHIND_EXP(dev)){
				        List_AddTail(&((domain_expander *)dev->base.parent)->enclosure_queue_pointer, &enc->expander_list);
				        ((domain_expander *)dev->base.parent)->enclosure = enc;
			        }
		        }
		        MV_CopyMemory(enc->vendor_id, &buf[8], 8);
		        MV_CopyMemory(enc->product_revision, &buf[32],4);
		        MV_CopyMemory(enc->product_id, &buf[16], 16);
		}
                
		/*set down disk*/
              /* in callback routine, should call core_init_entry_done to clean up for the HD
                 * set down disk will minus init_count only if it still in the init queue 
                 * and should set down disk after enclosure get related information */
		core_init_entry_done(root, port, NULL);
		pal_set_down_disk(root, dev, MV_FALSE);
		get_new_and_update_base_id(root,&enc->base);


                if (!enc) {
        	        CORE_DPRINT(("ran out of enclosure. abort initialization\n"));
	        	return;
                } else {            
		        core_queue_init_entry(root, &enc->base, MV_TRUE);
                }
	}else if((buf[0]&0x1f)==0x01){
		dev->dev_type = DT_SEQ_ACCESS;
		dev->base.queue_depth = 1;
	} else{
		dev->dev_type=buf[0] & 0x1F;
		}
	/*
	 * Standard inquiry:
	 * Byte 8-15 T10 Vendor Identification
	 * Byte 16-31 Product Identification
	 * Byte 32-35 Revision Level
	 * Byte 36-55 Vendor Specific. FIJISHU use it as the serial number.
	 * Need double check other vendors.
	 */

	MV_CopyMemory(dev->model_number, &buf[8], 24);
	MV_CopyMemory(dev->firmware_revision, &buf[32], 4);
	MV_CopyMemory(dev->serial_number, &buf[36], 20);
}

void sas_inquiry_callback(domain_device *dev, PMV_Request req)
{
	MV_U8 *buf = (MV_U8 *)core_map_data_buffer(req);

	if (req->Cdb[1] & MV_BIT(0)) {
 		/* pass EVPD inquiry */
		parse_evpd_inquiry(dev, buf);

		dev->state = DEVICE_STATE_INQUIRY_EVPD_DONE;
	} else {
		/* pass standard inquiry */
		parse_standard_inquiry(dev, buf);
		if(dev->dev_type != DT_DIRECT_ACCESS_BLOCK){
			dev->state = DEVICE_STATE_INIT_DONE;
			if (dev->base.LUN == 0) {
				dev->state=DEVICE_STATE_REPORT_LUN;
			}
		} else
			dev->state = DEVICE_STATE_INQUIRY_DONE;
		
	}
	if((dev->dev_type == DT_ENCLOSURE)||
		(dev->dev_type == DT_SES_DEVICE)){
		dev->state = DEVICE_STATE_INIT_DONE;
		dev->status &= ~DEVICE_STATUS_FUNCTIONAL;
	}

        core_unmap_data_buffer(req);
}

MV_U16 sas_get_exist_target_lun( pl_root *root, MV_U16 target, MV_U16 *lun_map)
{
	core_extension *core = root->core;
	lib_device_mgr *lib_dev = &core->lib_dev;
	MV_U16 i=0;
	MV_U16 org_lun_cnt =0;
	for(i = 0; i <MV_MAX_LUN_NUMBER;i++){
		lun_map[i] = 0xFFFF;
	}

	for(i=0; i<MAX_ID; i++) {
		domain_base *base;
		base = lib_dev->device_map[i];
		if (base ==NULL)
			continue;
		 if ( base->TargetID == target){
			lun_map[org_lun_cnt] = base->LUN;
			org_lun_cnt++;
		}				
	}
	return org_lun_cnt;
}

MV_U16 	sas_check_target_lun( pl_root *root, MV_U16 target, MV_U16 lun)
{
	core_extension *core = root->core;
	lib_device_mgr *lib_dev = &core->lib_dev;
	MV_U16 i=0;
	
	for(i=0; i<MAX_ID; i++) {
		domain_base *base;
		base = lib_dev->device_map[i];
		if (base && ( base->TargetID== target) && ( base->LUN== lun)){
			return MV_TRUE;
		}				
	}
	return MV_FALSE;
}

struct mv_scsi_lun {
	MV_U8 scsi_lun[8];
};

int mv_scsilun_to_int(struct mv_scsi_lun *scsilun)
{
	int i;
	unsigned int lun;

	lun = 0;
	for (i = 0; i < sizeof(lun); i += 2)
		lun = lun | (((scsilun->scsi_lun[i] << 8) |
			      scsilun->scsi_lun[i + 1]) << (i * 8));
	return lun;
}

void sas_report_luns_callback(domain_device *dev, PMV_Request req)
{
	MV_U8 *buf = (MV_U8 *)core_map_data_buffer(req);
	struct mv_scsi_lun *lun_data;
	MV_U8 tmp;
	unsigned int length,lun_cnt;
	int i,k;
	MV_U16 lunp,id;
	MV_U16  lun,target;
	MV_U16 lun_map[MV_MAX_LUN_NUMBER], org_lun_map[MV_MAX_LUN_NUMBER];
	MV_BOOLEAN  lun_tag = MV_FALSE;
	pl_root *root = (pl_root *)dev->base.root;
	core_extension *core =  (core_extension *)root->core;
	domain_port *port = (domain_port *)dev->base.port;
	domain_device *dev_down;

	length = ((buf[0] << 24) |(buf[1] << 16)|(buf[2] << 8)|(buf[3] << 0));	
	lun_cnt = length/8;
	
	if(lun_cnt == 0)
		goto finished;
	else if(lun_cnt == 1)
		dev->base.multi_lun = MV_FALSE;
	else if(lun_cnt >1)
		dev->base.multi_lun = MV_TRUE;
	
	target = dev->base.TargetID;
	lun_data = (struct mv_scsi_lun *)buf;
	if(dev->state == DEVICE_STATE_LOG_SENSE_DONE) {  
		for(i = 1; i < (lun_cnt+1); i++)  {
			lun = (MV_U32)((buf[i*8]<<8) | (buf[i*8+1]<<0)|(buf[i*8+2]<<24)|(buf[i*8+3]<<16))&0xffff;
			if(dev->base.LUN ==0 && lun!=0){
				sas_init_target_device(root, port,dev,target, lun);
			}
		}
	} else if(dev->state == DEVICE_STATE_REPORT_LUN){
		MV_U16 org_lun_cnt = sas_get_exist_target_lun(root, target, org_lun_map);
		for(i = 0; i <MV_MAX_LUN_NUMBER;i++){
			lun_map[i] = 0xFFFF;
		}
		for (i = 1; i < (lun_cnt+1); i++) {
			lun = (MV_U32)((buf[i*8]<<8)|(buf[i*8+1]<<0)|(buf[i*8+2]<<24)|(buf[i*8+3]<<16))&0xffff;
			lun_map[i-1]=lun;
			if(lun > MV_MAX_LUN_NUMBER)
				continue;
			if(sas_check_target_lun(root, target, lun) == MV_FALSE) {
				sas_init_target_device(root, port,dev,target,lun);
			}
		}
		for(k = 0; k < org_lun_cnt ; k++){
			lun_tag = MV_FALSE;
			for(i = 0; i < lun_cnt; i++){
				if(org_lun_map[k] == lun_map[i]){
					lun_tag = MV_TRUE;
					break;
				}					
			}
			if(lun_tag == MV_FALSE){
				id = get_id_by_targetid_lun(core, dev->base.TargetID, org_lun_map[k]);
				dev_down = (domain_device *)get_device_by_id(root->lib_dev, id);
				pal_set_down_disk(root, dev_down, MV_TRUE);
			}					
		}	
	}
finished:
		dev->state = DEVICE_STATE_INIT_DONE;

		core_unmap_data_buffer(req);
}

void sas_read_capacity_callback(domain_device *dev, PMV_Request req)
{
	pl_root *root = dev->base.root;
	core_extension *core = (core_extension *)root->core;
	MV_U8 *buf = (MV_U8 *)core_map_data_buffer(req);
	core_context *ctx = req->Context[MODULE_CORE];
	struct _error_context *err_ctx = &dev->base.err_ctx;

	/* read capacity 10 */
	if (req->Cdb[0] == SCSI_CMD_READ_CAPACITY_10) {
		dev->sector_size = buf[7]+(buf[6]<<8)+(buf[5]<<16)+(buf[4]<<24);

		U32_ASSIGN_U64(dev->max_lba, (MV_U32)(buf[3]+(buf[2]<<8)+(buf[1]<<16)+(buf[0]<<24)));

		if ((dev->sector_size == 0) ||
			(dev->sector_size > dev->max_lba.value))
		{
			MV_DASSERT(MV_FALSE);
			err_ctx->retry_count++;
                     core_unmap_data_buffer(req);
			core_sleep_millisecond(root->core, 10);
			return;
		}
	} else {
		/* read capacity 16 */
		dev->sector_size = buf[11]+(buf[10]<<8)+(buf[9]<<16)+(buf[8]<<24);
		U32_ASSIGN_U64(dev->max_lba, buf[7]+(buf[6]<<8)+(buf[5]<<16)+(buf[4]<<24)+
				(((_MV_U64) buf[3])<<32) +
 				(((_MV_U64) buf[2])<<40) +
				(((_MV_U64) buf[1])<<48) +
				(((_MV_U64) buf[0])<<56));
		if(buf[12]&0x01) {
			dev->setting |= DEVICE_SETTING_PI_ENABLED;
			dev->sector_size +=8;
		}
	}

       dev->max_transfer_size = CORE_MAX_TRANSFER_SIZE;

	CORE_PRINT(("Device[0x%x] sector size 0x%x max LBA 0x%08x%08x\n",
		req->Device_Id, dev->sector_size,
		dev->max_lba.parts.high, dev->max_lba.parts.low));

	if (core->is_dump == MV_TRUE)
		dev->state = DEVICE_STATE_INIT_DONE;
	else
		dev->state = DEVICE_STATE_READ_CAPACITY_DONE;

        core_unmap_data_buffer(req);
}

void sas_start_stop_unit_callback(domain_device *dev, PMV_Request req)
{
	dev->state = DEVICE_STATE_STARTSTOP_DONE;
}

void sas_test_unit_ready_callback(domain_device *dev, MV_Request *req)
{
	struct _error_context *err_ctx = &dev->base.err_ctx;
	MV_U8 sense_key;

	if (req->Scsi_Status == REQ_STATUS_HAS_SENSE) {
		sense_key = ((MV_U8 *)req->Sense_Info_Buffer)[2] & 0x0f;
		if ((sense_key == SCSI_SK_UNIT_ATTENTION) ||
			(sense_key == SCSI_SK_NOT_READY)) {
			err_ctx->retry_count++;
			core_sleep_millisecond(dev->base.root->core, 10);
			return;
		}
	}

	dev->state = DEVICE_STATE_TEST_UNIT_READY_DONE;
}

void sas_log_sense_callback(domain_device *dev, PMV_Request req)
{
	MV_U16 length;
	MV_U16 i;
	MV_PU8 data_buf;

	dev->capability &= ~DEVICE_CAPABILITY_SMART_SUPPORTED;
	dev->setting &= ~DEVICE_SETTING_SMART_ENABLED;
	data_buf = (MV_PU8)core_map_data_buffer(req);
	length = (data_buf[2] << 8) + data_buf[3];

	/* Check whether it supports 0x2F log page which we will use
	        to check smart status. */
	for (i = 0; i < length; i++) {
		if (data_buf[4 + i] ==
			INFORMATIONAL_EXCEPTIONS_LOG_PAGE) {
			dev->capability |=
				DEVICE_CAPABILITY_SMART_SUPPORTED;
			dev->setting |=
				DEVICE_SETTING_SMART_ENABLED;
			break;
		}
	}

	dev->state = DEVICE_STATE_LOG_SENSE_DONE;
	core_unmap_data_buffer(req);
}

MV_VOID
sas_enable_read_ahead_callback(MV_Request *req, domain_device *dev)
{
	dev->state = DEVICE_STATE_SET_READ_AHEAD_DONE;
	req->Scsi_Status = REQ_STATUS_SUCCESS;
}

MV_VOID
sas_enable_write_cache_callback(MV_Request *req, domain_device *dev)
{
	if (req->Scsi_Status == REQ_STATUS_SUCCESS) {
		dev->capability |= DEVICE_CAPABILITY_WRITECACHE_SUPPORTED;
	} else {
		dev->capability &= ~DEVICE_CAPABILITY_WRITECACHE_SUPPORTED;
		req->Scsi_Status = REQ_STATUS_SUCCESS;

	}
	dev->state = DEVICE_STATE_SET_CACHE_ENABLE_DONE;
}

void sas_check_wide_port_device(MV_PVOID port_p)
{
	domain_port *port = (domain_port *)port_p;
	pl_root *root = port->base.root;
	domain_device *dev = NULL, *matched_dev = NULL;
	MV_U8 phy_id = 0, i, count = 0;

	/* find the device that was attached to the port */
	LIST_FOR_EACH_ENTRY_TYPE(dev, &port->device_list, domain_device,
		base.queue_pointer) {
		if (dev->base.parent == &port->base)
			matched_dev = dev;
		count++;
	}
	MV_ASSERT(matched_dev != NULL);
}
