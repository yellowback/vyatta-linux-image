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
#include "core_header.h"
#include "core_util.h"
#include "core_error.h"

/*
 [ ReceiveDiagnostic	]
 SES_PG_CONFIGURATION
 (ConfigurationPageHeader),
	( EnclosureDescriptorHeader	+ EnclosureDescriptor +	TypeDescriptorHeader )
 SES_PG_DEVICE_ELEMENT_STATUS
 (DeviceElementStatusPage),
	( DeviceElementStatusDescriptorProtocolEIP0	+ DeviceElementStatusDescriptorEIP0	+ (	SASPhyDescriptor...	) )	/
	( DeviceElementStatusDescriptorProtocolEIP1	+ DeviceElementStatusDescriptorEIP1	+ (	SASPhyDescriptor...	) )
 SES_PG_ELEMENT_DESCRIPTOR
 (ElementDescriptorPage),
	ElementDescriptor
 SES_PG_ENCLOSURE_STATUS
 (EnclosureStatusPage),
	DeviceElementForEnclosureStatusPage	/ ArrayElementForEnclosureStatusPage

 [ SendDiagnostic ]
 SES_PG_ENCLOSURE_CONTROL
 (EnclosureControlPage),
	DeviceElementForEnclosureControlPage / ArrayElementForEnclosureControlPage
*/

void core_expander_set_led(
	IN core_extension *core,
	IN domain_device *device,
	IN MV_U8 flag
	);

MV_Request *ses_make_receive_diagnostic_request(domain_enclosure *enc,MV_U8 page_code,
	MV_ReqCompletion completion);

#define	ses_make_check_support_page_request(a, callback) (ses_make_receive_diagnostic_request(a,SES_PG_SUPPORTED_DIAGNOSTICS,callback))
#define	ses_make_configuration_request(a,callback)		(ses_make_receive_diagnostic_request(a,SES_PG_CONFIGURATION,callback))
#define	ses_make_element_status_request(a,callback)		(ses_make_receive_diagnostic_request(a,SES_PG_DEVICE_ELEMENT_STATUS,callback))
#define	ses_make_element_descriptor_request(a,callback)	(ses_make_receive_diagnostic_request(a,SES_PG_ELEMENT_DESCRIPTOR,callback))
#define	ses_make_enclosure_status_request(a)		(ses_make_receive_diagnostic_request(a,SES_PG_ENCLOSURE_STATUS,ses_get_status_callback))

#define LIST_ALL_ASSOCIATED_EXPANDER_TO_ENCLOSURE(expander, enclosure) LIST_FOR_EACH_ENTRY_TYPE(expander,&enclosure->expander_list, domain_expander,enclosure_queue_pointer)
#define LIST_ALL_ASSOCIATED_DEVICE_TO_EXPANDER(device, expander) LIST_FOR_EACH_ENTRY_TYPE(device, &expander->device_list, domain_device,base.exp_queue_pointer)
void ses_parse_supported_page(domain_enclosure *enc,	PMV_Request	req)
{
	MV_U8 *response = (MV_PU8)core_map_data_buffer(req);

	enc->supported_page_count = response[3];
	if (enc->supported_page_count > 32)
		enc->supported_page_count = 32;
	MV_CopyMemory(enc->supported_page,&response[4],enc->supported_page_count);
	
	core_unmap_data_buffer(req);
}
MV_BOOLEAN ses_check_supported_page(domain_enclosure *enc, MV_U8 page_code)
{
	MV_U8 i;
	if(page_code == SES_PG_SUPPORTED_DIAGNOSTICS)
		return MV_TRUE;
	for(i = 0; i < enc->supported_page_count; i++){
		if(enc->supported_page[i] == page_code)
			return MV_TRUE;
	}
	return MV_FALSE;
}
void ses_assign_device_element_number(domain_enclosure *enc,
	sas_phy_descriptor *sas_phy_descriptor, MV_U8 element_index)
{
	pl_root *root = enc->base.root;
	domain_device *device = NULL;
	domain_expander *expander = NULL;
	MV_U8 i;
	MV_U64 val64;

	LIST_ALL_ASSOCIATED_EXPANDER_TO_ENCLOSURE(expander, enc){
		LIST_ALL_ASSOCIATED_DEVICE_TO_EXPANDER(device, expander){
			U64_ASSIGN(val64, MV_CPU_TO_BE64(device->sas_addr));
			if(MV_Equals((MV_U8 *)&val64, sas_phy_descriptor->sas_address, 8)){
				device->ses_element_index = element_index;
				device->enclosure = enc;
				break;
			}
		}
	}
}

void ses_parse_element_status_request(domain_enclosure *enc,	PMV_Request	req)
{
	device_element_status_page *element_status_page;
	device_element_status_descriptor *element_status_descriptor;
	sas_phy_descriptor *phy_descriptor;
	MV_U16 page_length;
	MV_U8 i, element_count, descriptor_count;

	element_status_page = (device_element_status_page *)core_map_data_buffer(req);
	page_length = (element_status_page->page_length[0]<<8) + element_status_page->page_length[1];

	if ( (page_length+4)> (MV_U16)(req->Data_Transfer_Length))
	{
		CORE_DPRINT(("failed, 0x%x scratch memory is needed.\n", (page_length+4)));
                core_unmap_data_buffer(req);
		return;
	}
	element_status_descriptor = (device_element_status_descriptor *) ((MV_PTR_INTEGER)element_status_page +
		sizeof(device_element_status_page));

	element_count=0;
	while ((MV_PTR_INTEGER)element_status_descriptor < ((MV_PTR_INTEGER)element_status_page+ page_length + 4)) {
		if (!element_status_descriptor->eip0.invalid) {
                /* SAS protocol, Device/Array descriptors */
			if ((element_status_descriptor->eip0.protocol_id == 0x06) &&
				(element_status_descriptor->eip0.descriptor_type == 0)) {
				if (element_status_descriptor->eip0.b_eip) { /* EIP1 */
					phy_descriptor = (sas_phy_descriptor *)((MV_PTR_INTEGER)element_status_descriptor +
															sizeof(device_element_status_descriptor_eip1));
					descriptor_count = element_status_descriptor->eip1.phy_descriptor_count;
				}
                                else { /*	EIP0 */
					phy_descriptor = (sas_phy_descriptor *)((MV_PTR_INTEGER)element_status_descriptor +
															sizeof(device_element_status_descriptor_eip0));
					descriptor_count = element_status_descriptor->eip0.phy_descriptor_count;
				}

                                for (i = 0; i < descriptor_count; i++) {
					if (element_status_descriptor->eip0.b_eip)
						ses_assign_device_element_number(enc,phy_descriptor,element_status_descriptor->eip1.element_index);
					else
						ses_assign_device_element_number(enc,phy_descriptor,element_count);
				}
			}
		}
		element_count++;
		element_status_descriptor = (device_element_status_descriptor *) ((MV_PTR_INTEGER)element_status_descriptor +
				element_status_descriptor->eip0.device_element_status_descriptor_length + 2);
	}
        core_unmap_data_buffer(req);
}

void ses_assign_device_overall_element_number(domain_enclosure *enc,
	MV_U8 element_index,	MV_U8 type_overall_count,	MV_U8 element_type)
{
	pl_root *root =enc->base.root;
	domain_base *base= NULL;
	domain_device *device;
	MV_U16 i;

	for(i=0; i<MAX_ID; i++){
		base = get_device_by_id(root->lib_dev, i);
		if((base == NULL)||(base->type !=  BASE_TYPE_DOMAIN_DEVICE))
			continue;
		device = (domain_device *)base;
		if((device->ses_element_index == element_index)&&
				(device->enclosure == enc)){
				device->ses_overall_element_index = element_index + type_overall_count;
				device->ses_element_type = element_type;
				break;
		}
	}
}


MV_BOOLEAN ses_find_repeat_enclosure(domain_enclosure *enc,
	 enclosure_descriptor *enclosure_descriptor)
{
	pl_root *root = enc->base.root;
	domain_base *base = NULL;
	domain_enclosure *temp_enc = NULL;
	MV_U16 i;

	for (i=0; i<MAX_ID; i++){
		base = get_device_by_id(root->lib_dev, i);
		if(base == NULL)
			continue;
		if(base->type == BASE_TYPE_DOMAIN_ENCLOSURE){
			MV_U64 enclosure_logical_id = *((MV_U64 *)(&enclosure_descriptor->enclosure_logical_id[0]));
			temp_enc = (domain_enclosure *)base;
			if((enc != temp_enc)&&
				(!MV_CompareMemory(temp_enc->enclosure_logical_id,
						enclosure_descriptor->enclosure_logical_id, 8))){
				if (enclosure_logical_id.value == 0)
					return MV_FALSE;
				
				CORE_DPRINT(("duplicate enc %016llX.\n",(*(MV_U64 *)temp_enc->enclosure_logical_id)));		
				if((enc->base.parent)&&
					(enc->base.parent->type == BASE_TYPE_DOMAIN_EXPANDER))
					((domain_expander *)enc->base.parent)->enclosure = temp_enc;
					List_AddTail(&((domain_expander *)enc->base.parent)->enclosure_queue_pointer, &temp_enc->expander_list);
				return MV_TRUE;
			}
		}
	}
	return MV_FALSE;
}

MV_BOOLEAN ses_parse_configuration_request(
	domain_enclosure *enc,
	PMV_Request	req
)
{
	configuration_page_header *config_page;
	enclosure_descriptor_header *enc_descriptor_head;
	enclosure_descriptor *enc_descriptor;
	type_descriptor_header *type_descriptor_head;
	MV_U16 page_length;
	MV_U8 i, j, element_type_count, element_count;

	config_page = (configuration_page_header *)core_map_data_buffer(req);

	page_length = (config_page->page_length[0]<<8) + config_page->page_length[1];

	if ((page_length + 4) > (MV_U16)(req->Data_Transfer_Length)) {
		CORE_PRINT(("MakeSesConfigurationRequest failed, 0x%x scratch memory is needed.\n", (page_length+4)));
                core_unmap_data_buffer(req);
		return MV_TRUE;
	}

	enc_descriptor_head = (enclosure_descriptor_header *) ((MV_PTR_INTEGER)	config_page	+
		sizeof(configuration_page_header));
	/* total enclosure number =	primary	+ SubEnclosure = 1+	SubEnclosureCount*/
	element_type_count=0;
	for(i=0;i<=config_page->subenclosure_count;i++){
		element_type_count+=enc_descriptor_head->number_of_element_types_supported;
		enc_descriptor = (enclosure_descriptor *) ((MV_PTR_INTEGER)	enc_descriptor_head +
									sizeof(enclosure_descriptor_header) );
		if (ses_find_repeat_enclosure(enc, enc_descriptor)){
                     core_unmap_data_buffer(req);
			return MV_FALSE;
		}
		MV_CopyMemory(enc->enclosure_logical_id,
			enc_descriptor->enclosure_logical_id,
			8);
		MV_CopyMemory(enc->vendor_id,
			enc_descriptor->enclosure_vendor_id,
			8);
		MV_CopyMemory(enc->product_id,
			enc_descriptor->product_id,
			16);
		MV_CopyMemory(enc->product_revision,
			enc_descriptor->product_revision_level,
			4);

		enc_descriptor_head=	(enclosure_descriptor_header *) ((MV_PTR_INTEGER)enc_descriptor +
			enc_descriptor_head->enclosure_descriptor_length);
	}

	type_descriptor_head = (type_descriptor_header *)((MV_PTR_INTEGER) enc_descriptor_head);
	element_count=0;
	for	(i = 0;	i <	element_type_count; i++){
		if( (type_descriptor_head->element_type==SES_TYPE_DEVICE) ||
			(type_descriptor_head->element_type==SES_TYPE_ARRAY_DEVICE) ){
			for (j=0; j<type_descriptor_head->number_of_possible_elements; j++)
				ses_assign_device_overall_element_number(enc,
					j,
					element_count,
					type_descriptor_head->element_type);
		}
		element_count += type_descriptor_head->number_of_possible_elements;
		type_descriptor_head = (type_descriptor_header *)((MV_PTR_INTEGER)type_descriptor_head +
			sizeof(type_descriptor_header));
	}

	core_unmap_data_buffer(req);
	return MV_TRUE;
}

MV_U8 find_ascii_number(MV_U8 *string, MV_U16 length)
{
	MV_U8 sum=0xff;
	MV_U16 i;

	for(i=0;i<length;i++)
	{
		if((*(string+i)<='9')&&(*(string+i)>='0'))
		{
			if(sum==0xff) sum=0;
			sum=sum*10 + *(string+i) - '0';
		}
		else if(sum!=0xff)
		{
			break;
		}
	}
	return sum;
}

void ses_assign_element_slot_number(
	domain_enclosure *enc,
	MV_U8 overall_element_index,
	MV_U8 slot_number
)
{
	pl_root *root = enc->base.root;
	domain_base *base= NULL;
	domain_device *device;
	MV_U16 i;

	for(i=0; i<MAX_ID; i++){
		base = get_device_by_id(root->lib_dev, i);
		if((base == NULL)||(base->type !=  BASE_TYPE_DOMAIN_DEVICE))
			continue;
		device = (domain_device *)base;
		if((device->enclosure == enc)&&
				(device->ses_overall_element_index == overall_element_index)){
				device->ses_slot_number = slot_number;
				break;
		}
	}

}

void ses_parse_element_descriptor_request(
	domain_enclosure *enc,
	PMV_Request	req
)
{
	element_descriptor_page *ele_descriptor_page;
	element_descriptor *ele_descriptor;
	MV_U16 page_length, desc_page_length;
	MV_U8 element_count, slot_number;

	ele_descriptor_page = (element_descriptor_page *)core_map_data_buffer(req);
	page_length = (ele_descriptor_page->page_length[0]<<8) + ele_descriptor_page->page_length[1];

	if ( (page_length+4)> (MV_U16)(req->Data_Transfer_Length)){
		CORE_DPRINT(("failed, 0x%x scratch memory is needed.\n", (page_length+4)));
                core_unmap_data_buffer(req);
		return;
	}
	ele_descriptor = (element_descriptor *) ((MV_PTR_INTEGER)ele_descriptor_page +
		sizeof(element_descriptor_page));

	element_count=0;
	while((MV_PTR_INTEGER)ele_descriptor<	((MV_PTR_INTEGER)ele_descriptor_page+ page_length + 4))
	{
		desc_page_length=	(ele_descriptor->descriptor_length[0]<<8) + ele_descriptor->descriptor_length[1];
		slot_number = find_ascii_number((MV_U8 *)((MV_PTR_INTEGER)ele_descriptor + 4),desc_page_length);
		ses_assign_element_slot_number(enc, element_count,slot_number);
		element_count++;
		ele_descriptor = (element_descriptor *) ((MV_PTR_INTEGER)ele_descriptor +
			desc_page_length + 4);
	}

	core_unmap_data_buffer(req);
}


void ses_internal_req_callback(MV_PVOID root_p, PMV_Request req)
{
	pl_root *root = (pl_root *)root_p;
	domain_base *base = NULL;
	domain_enclosure *enc = NULL;

	base = get_device_by_id(root->lib_dev, req->Device_Id);
	if(base == NULL)
		return;
	MV_ASSERT(base != NULL);
	enc = (domain_enclosure *)base;

	if (req->Scsi_Status != REQ_STATUS_SUCCESS) {
		core_handle_init_error(root, base, req);
		return;
	}
	if(req->Cdb[0]==SCSI_CMD_RCV_DIAG_RSLT) {
		switch(req->Cdb[2]){
		case SES_PG_SUPPORTED_DIAGNOSTICS:
			ses_parse_supported_page(enc, req);
			enc->state = ENCLOSURE_CHECK_SUPPORT_PAGE_DONE;
			break;
		case SES_PG_DEVICE_ELEMENT_STATUS:
			ses_parse_element_status_request(enc,req);
			enc->state = ENCLOSURE_GET_DEVICE_ELEMENT_DONE;
			break;
		case SES_PG_CONFIGURATION:
			if (ses_parse_configuration_request(enc, req))
				enc->state = ENCLOSURE_GET_CONFIGUATION_DONE;
			else{
				core_init_entry_done(root, base->port, NULL);
				pal_set_down_enclosure(root, enc);
				return;
			}
			break;
		case SES_PG_ELEMENT_DESCRIPTOR:
			ses_parse_element_descriptor_request(enc, req);
			enc->state = ENCLOSURE_GET_ELEMENT_DISCRIPTER_DONE;
			break;
		}
		core_queue_init_entry(root, base, MV_FALSE);
	}

}


MV_BOOLEAN ses_state_machine(MV_PVOID enc_p)
{
	domain_enclosure *enc = (domain_enclosure *)enc_p;
	pl_root *root = enc->base.root;
	domain_port *port = enc->base.port;
	PMV_Request req = NULL;

	CORE_DPRINT(("enclosure state: 0x%x\n",enc->state));
	switch(enc->state){
	case ENCLOSURE_INQUIRY_DONE:
		req = ses_make_check_support_page_request(enc, ses_internal_req_callback);
		break;
	case ENCLOSURE_CHECK_SUPPORT_PAGE_DONE:
		if(ses_check_supported_page(enc, SES_PG_DEVICE_ELEMENT_STATUS) ) {
			req = ses_make_element_status_request(enc,ses_internal_req_callback);
			break;
		} else
			enc->state = ENCLOSURE_GET_DEVICE_ELEMENT_DONE;
	case ENCLOSURE_GET_DEVICE_ELEMENT_DONE:
		if(ses_check_supported_page(enc, SES_PG_CONFIGURATION) ) {
			req = ses_make_configuration_request(enc,ses_internal_req_callback);
			break;
		} else
			enc->state = ENCLOSURE_GET_CONFIGUATION_DONE;
	case ENCLOSURE_GET_CONFIGUATION_DONE:
		if(ses_check_supported_page(enc, SES_PG_ELEMENT_DESCRIPTOR) ) {
			req = ses_make_element_descriptor_request(enc,ses_internal_req_callback);
			break;
		} else
			enc->state = ENCLOSURE_GET_ELEMENT_DISCRIPTER_DONE;
	case ENCLOSURE_GET_ELEMENT_DISCRIPTER_DONE:
		enc->state = ENCLOSURE_INIT_DONE;
	case ENCLOSURE_INIT_DONE:
	default:
		core_init_entry_done(root, port, &enc->base);
		return MV_TRUE;
	}
	if (req != NULL) {
		core_append_init_request(root, req);
		return MV_TRUE;
	} else {
		return MV_FALSE;
	}
}

MV_Request *ses_write_control_diagnostic_command(domain_enclosure *enc, MV_Request *org_req);
void ses_get_status_callback(MV_PVOID root_p, PMV_Request req)
{
	pl_root *root = (pl_root *)root_p;
	enclosure_status_page *enc_status_page;
	MV_U16 page_length;
	core_context *ctx = req->Context[MODULE_CORE];
	domain_enclosure *enc = (domain_enclosure *)get_device_by_id(
		root->lib_dev, req->Device_Id);
	MV_Request *new_req = NULL;

	if(req->Scsi_Status != REQ_STATUS_SUCCESS)
		return;

	enc_status_page = (enclosure_status_page *)core_map_data_buffer(req);
	page_length = (enc_status_page->page_length[0]<<8) + enc_status_page->page_length[1];

	if ( (page_length+4)> (MV_U16)(req->Data_Transfer_Length)) {
		CORE_PRINT(("failed, 0x%x scratch memory is needed.\n", (page_length+4)));
                core_unmap_data_buffer(req);
		return;
	}
	new_req = ses_write_control_diagnostic_command(enc, req);
	if(new_req == NULL){
		CORE_DPRINT(("no resource for internal request\n"));
                core_unmap_data_buffer(req);
		return;
	}

	core_unmap_data_buffer(req);
	core_append_request(root,new_req);
}

MV_Request *ses_make_receive_diagnostic_request(domain_enclosure *enc,MV_U8 page_code,
	MV_ReqCompletion completion)
{
	pl_root *root = enc->base.root;
	MV_Request	*req = get_intl_req_resource(root,SES_SCRATCH_BUFFER_SIZE);
	core_context *ctx = NULL;

	if( req == NULL ){
		CORE_DPRINT(("ERROR: No more free internal requests. Request aborted.\n"));
		return NULL;
	}

	req->Tag =	0xac;
	ctx = req->Context[MODULE_CORE];
	ctx->handler = enc->base.handler;

	req->Device_Id	= enc->base.id;

	req->Cmd_Flag = CMD_FLAG_DATA_IN;
	req->Completion = completion;
	MV_DASSERT(req->Data_Buffer != NULL);
	/* Prepare read	receive	diagnostic result */
	req->Cdb[0] = SCSI_CMD_RCV_DIAG_RSLT;
	req->Cdb[1] = 0x01; /* PCV */
	req->Cdb[2] = page_code;
	req->Cdb[3] = (MV_U8)((req->Data_Transfer_Length&0xff00)>>8);
	req->Cdb[4] = (MV_U8)(req->Data_Transfer_Length&0xff);

	return req;
}

void ses_fill_enclosure_element_status(domain_enclosure *enc,
	MV_U8 overall_element_index, ses_enclosure_element *element_status)
{
	pl_root *root = enc->base.root;
	domain_device *device = NULL;
	domain_base *base = NULL;
	MV_U16 i;

	for(i=0; i<MAX_ID; i++){
		base = get_device_by_id(root->lib_dev, i);
		if((base == NULL)||(base->type !=  BASE_TYPE_DOMAIN_DEVICE))
			continue;
		device = (domain_device *)base;
		if((device->enclosure == enc)&&
			(device->ses_overall_element_index == overall_element_index)){
			switch(device->ses_request_flag){
			case LED_FLAG_REBUILD:
				if (device->ses_element_type == SES_TYPE_DEVICE){
					element_status->device_type.control.select = 1;
					element_status->device_type.control.request_identify = 1;
				}else if (device->ses_element_type == SES_TYPE_ARRAY_DEVICE){
					element_status->array_type.control.select = 1;
					element_status->array_type.control.request_rebuild_remap = 1;
				}
				break;

			case LED_FLAG_HOT_SPARE:
				if (device->ses_element_type == SES_TYPE_DEVICE){
					element_status->device_type.control.select = 1;
					element_status->device_type.control.request_identify = 1;
				}else if (device->ses_element_type == SES_TYPE_ARRAY_DEVICE){
					element_status->array_type.control.select = 1;
					element_status->array_type.control.request_hotspare = 1;
				}
				break;

			case LED_FLAG_FAIL_DRIVE:
				if (device->ses_element_type == SES_TYPE_DEVICE){
					element_status->device_type.control.select = 1;
					element_status->device_type.control.request_fault = 1;
				}else if (device->ses_element_type == SES_TYPE_ARRAY_DEVICE){
					element_status->array_type.control.select = 1;
					element_status->array_type.control.request_fault = 1;
				}
				break;
			case LED_FLAG_HOT_SPARE_OFF:
				if(device->ses_element_type == SES_TYPE_DEVICE){
					element_status->device_type.control.select = 1;
					element_status->device_type.control.request_identify = 0;
				}else if(device->ses_element_type == SES_TYPE_ARRAY_DEVICE){
					element_status->array_type.control.select = 1;
					element_status->array_type.control.request_hotspare = 0;
				}
				break;
			case LED_FLAG_OFF_ALL:
				/* turn off all the lights that we currently control - if added more lights
				 * in other sections, make sure to add them here for turning off */
				if (device->ses_element_type == SES_TYPE_DEVICE){
					element_status->device_type.control.select = 1;
					element_status->device_type.control.request_identify = 0;
					element_status->device_type.control.request_fault = 0;
				}else if (device->ses_element_type == SES_TYPE_ARRAY_DEVICE){
					element_status->array_type.control.select = 1;
					element_status->array_type.control.request_identify = 0;
					element_status->array_type.control.request_rebuild_remap = 0;
					element_status->array_type.control.request_hotspare = 0;
					element_status->array_type.control.request_fault = 0;
				}
				break;
			}
			break;
		}
	}
}

void ses_update_control_buffer(enclosure_control_page *control_page,
	enclosure_status_page *status_page, MV_U16 page_length,
	domain_enclosure *enc)
{
	MV_U8 element_count;
	ses_enclosure_element *element_status;



	element_status = (ses_enclosure_element *) ((MV_PTR_INTEGER)status_page +
		sizeof(enclosure_status_page)+ sizeof(ses_enclosure_element));
	element_count=0;
	while((MV_PTR_INTEGER)element_status<	((MV_PTR_INTEGER)status_page+ page_length))
	{
		ses_fill_enclosure_element_status(enc, element_count, element_status);

		element_count++;
		element_status = (ses_enclosure_element *) ((MV_PTR_INTEGER)element_status +
			sizeof(ses_enclosure_element));
	}
	MV_CopyMemory(control_page, status_page, page_length);
}

void ses_send_control_callback(MV_PVOID root_p, PMV_Request req)
{
	pl_root *root = (pl_root *)root_p;
	domain_base *base = NULL;

	base = get_device_by_id(root->lib_dev, req->Device_Id);
	if(base == NULL){
		CORE_PRINT(("enclosure has been removed\n"));
		return;
	}
	if(req->Scsi_Status == REQ_STATUS_SUCCESS)
		return;
}
MV_Request *ses_write_control_diagnostic_command(domain_enclosure *enc, MV_Request *org_req)
{
	pl_root *root = enc->base.root;
	enclosure_status_page *enc_status_page;
	enclosure_control_page *enc_control_page;
	MV_U16 page_length;
	PMV_Request	req=NULL;
	core_context *ctx;

	enc_status_page = (enclosure_status_page *)core_map_data_buffer(org_req);
	page_length = (enc_status_page->page_length[0]<<8) + enc_status_page->page_length[1];

	req = get_intl_req_resource(root, page_length+4);
	if (req == NULL) {
		CORE_DPRINT(("ERROR: No more free internal requests. Request aborted.\n"));
                core_unmap_data_buffer(org_req);
		return NULL;
	}

	enc_control_page = (enclosure_control_page *)core_map_data_buffer(req);
	ses_update_control_buffer(enc_control_page, enc_status_page, page_length + 4, enc);

	req->Tag = 0xac;

	req->Device_Id	= enc->base.id;
	ctx = req->Context[MODULE_CORE];
	ctx->handler = enc->base.handler;

	req->Cmd_Flag &= ~CMD_FLAG_DATA_IN;

	req->Completion = (void(*)(MV_PVOID,PMV_Request))ses_send_control_callback;

	/* Prepare send	diagnostic */
	req->Cdb[0] = SCSI_CMD_SND_DIAG;
	req->Cdb[1] = 0x10;  /* set PF bit to 1 */
	req->Cdb[3] = (MV_U8)((req->Data_Transfer_Length&0xff00)>>8);
	req->Cdb[4] = (MV_U8)(req->Data_Transfer_Length&0xff);

	core_unmap_data_buffer(req);
	core_unmap_data_buffer(org_req);

	return req;

}



