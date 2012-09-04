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
/*
[ ReceiveDiagnostic ]
SES_PG_CONFIGURATION
(ConfigurationPageHeader),
    ( EnclosureDescriptorHeader + EnclosureDescriptor + TypeDescriptorHeader )

SES_PG_DEVICE_ELEMENT_STATUS 
(DeviceElementStatusPage),
    ( DeviceElementStatusDescriptorEIP0 + ( SASPhyDescriptor... ) ) /
    ( DeviceElementStatusDescriptorEIP1 + ( SASPhyDescriptor... ) )

SES_PG_ELEMENT_DESCRIPTOR 
(ElementDescriptorPage),
    ElementDescriptor

SES_PG_ENCLOSURE_STATUS 
(EnclosureStatusPage),
    DeviceElementForEnclosureStatusPage / ArrayElementForEnclosureStatusPage




[ SendDiagnostic ]
SES_PG_ENCLOSURE_CONTROL 
(EnclosureControlPage),
    DeviceElementForEnclosureControlPage / ArrayElementForEnclosureControlPage

*/

/* SES2 Page Code */
#define SES_PG_SUPPORTED_DIAGNOSTICS      0x00
#define SES_PG_CONFIGURATION              0x01   
#define SES_PG_ENCLOSURE_STATUS           0x02
#define SES_PG_ENCLOSURE_CONTROL          0x02
#define SES_PG_HELP_TEXT                  0x03   
#define SES_PG_STRING_IN                  0x04   
#define SES_PG_THRESHOLD_IN               0x05   
#define SES_PG_ELEMENT_DESCRIPTOR         0x07
#define SES_PG_SHORT_ENCLOSURE_STATUS     0x08
#define SES_PG_ENCLOSURE_BUSY             0x09
#define SES_PG_DEVICE_ELEMENT_STATUS      0x0a
#define SES_PG_SUBENCLOSURE_HELP_TEXT     0x0b
#define SES_PG_SUBENCLOSURE_STRING_IN     0x0c
#define SES_PG_SUPPORTED_SES_DIAGNOSTICS  0x0d
#define SES_PG_VENDOR_SPECIFIC_SES_PAGE   0x10
#define SES_PG_VENDOR_PAGEA0              0xa0

/* Element Type */
#define SES_TYPE_UNSPECIFIED         0x00
#define SES_TYPE_DEVICE              0x01
#define SES_TYPE_POWER_SUPPLY        0x02
#define SES_TYPE_FAN                 0x03
#define SES_TYPE_TEMPERATURE_SENSOR  0x04
#define SES_TYPE_DOOR_LOCK           0x05
#define SES_TYPE_SPEAKER             0x06
#define SES_TYPE_ES_CONTROLLER       0x07
#define SES_TYPE_SCC_CONTROLLER      0x08
#define SES_TYPE_NONVOLATILE_CACHE   0x09
#define SES_TYPE_UPS                 0x0B
#define SES_TYPE_DISPLAY             0x0C
#define SES_TYPE_KEYPAD              0x0D
#define SES_TYPE_ENCLOSURE           0x0E
#define SES_TYPE_SCSI_TRANSCEIVER    0x0F
#define SES_TYPE_LANGUAGE            0x10
#define SES_TYPE_COMM_PORT           0x11
#define SES_TYPE_VOLTAGE_SENSOR      0x12
#define SES_TYPE_CURRENT_SENSOR      0x13
#define SES_TYPE_SCSI_TARGET_PORT    0x14
#define SES_TYPE_SCSI_INITIATOR_PORT 0x15
#define SES_TYPE_SIMPLE_SUBENCLOSURE 0x16
#define SES_TYPE_ARRAY_DEVICE        0x17
#define SES_TYPE_VENDOR_SPECIFIC     0x80


	typedef struct _receive_diagnostic
	{
		MV_U8 operation_code;
		MV_U8 pcv:1;
		MV_U8 reserved:7;
		MV_U8 page_code;
		MV_U8 allocation_length[2];
		MV_U8 control;
	}receive_diagnostic;

typedef struct _send_diagnostic
{
	MV_U8 operation_code;
	MV_U8 unit_off_l:1;
	MV_U8 dev_off_l:1;
	MV_U8 self_test:1;
	MV_U8 reserved:1;
	MV_U8 pf:1;
	MV_U8 self_test_code:3;
	MV_U8 param_length[2];
	MV_U8 control;
}send_diagnostic;


/* SES_PG_CONFIGURATION */
typedef struct _configuration_page_header
{
	MV_U8 page_code;
	MV_U8 subenclosure_count;
	MV_U8 page_length[2];
	MV_U8 generation_code[4];
} configuration_page_header;

typedef struct _enclosure_descriptor_header
{
	MV_U8 enclosure_service_processes_count:3;
	MV_U8 reserved1:1;
	MV_U8 relative_enclosure_service_process_id:3;
	MV_U8 reserved2:1;
	MV_U8 subenclosure_id;
	MV_U8 number_of_element_types_supported;
	MV_U8 enclosure_descriptor_length;
   
} enclosure_descriptor_header;

typedef struct _enclosure_descriptor
{
	MV_U8 enclosure_logical_id[8];
	MV_U8 enclosure_vendor_id[8];
	MV_U8 product_id[16];
	MV_U8 product_revision_level[4];
	MV_U8 vendor_specific; 
} enclosure_descriptor;

typedef struct _type_descriptor_header
{
	MV_U8 element_type;
	MV_U8 number_of_possible_elements;
	MV_U8 subenclosure_id;
	MV_U8 type_descriptor_text_length;
} type_descriptor_header;


/* SES_PG_DEVICE_ELEMENT_STATUS */
typedef struct _device_element_status_page
{
	MV_U8 page_code;
	MV_U8 reserved;
	MV_U8 page_length[2];   
	MV_U8 generation_code[4];
} device_element_status_page;

typedef struct _device_element_status_descriptor_eip0
{
	MV_U8 protocol_id:4;
	MV_U8 b_eip:1;
	MV_U8 reserved1:2;
	MV_U8 invalid:1;
	MV_U8 device_element_status_descriptor_length;
	MV_U8 phy_descriptor_count;
	MV_U8 not_all_phys:1;
	MV_U8 reserved2:5;
	MV_U8 descriptor_type:2;
} device_element_status_descriptor_eip0;

typedef struct _device_element_status_descriptor_eip1
{
	MV_U8 protocol_id:4;
	MV_U8 b_eip:1;
	MV_U8 reserved1:2;
	MV_U8 invalid:1;
	MV_U8 device_element_status_descriptor_length;
	MV_U8 reserved2;
	MV_U8 element_index;
	MV_U8 phy_descriptor_count;
	MV_U8 not_all_phys:1;
	MV_U8 reserved3:5;
	MV_U8 descriptor_type:2;
	MV_U8 reserved4;
	MV_U8 bay_number;
} device_element_status_descriptor_eip1;

typedef union 
{
	device_element_status_descriptor_eip0	eip0;
	device_element_status_descriptor_eip1	eip1;	
}device_element_status_descriptor;

typedef struct _sas_phy_descriptor
{
	MV_U8 reserved1:4;
	MV_U8 device_type:3;
	MV_U8 reserved2:1;
	MV_U8 reserved3;
	MV_U8 reserved4:1;
	MV_U8 smp_initiator_port:1;
	MV_U8 stp_initiator_port:1;
	MV_U8 ssp_initiator_port:1;
	MV_U8 reserved5:4;
	MV_U8 reserved6:1;
	MV_U8 smp_target_port:1;
	MV_U8 stp_target_port:1;
	MV_U8 ssp_target_port:1;
	MV_U8 reserved7:4;
	MV_U8 attached_sas_address[8];
	MV_U8 sas_address[8];
	MV_U8 phy_identifier;
	MV_U8 reserved8[7];
} sas_phy_descriptor;


/* SES_PG_ELEMENT_DESCRIPTOR */
typedef struct _element_descriptor_page
{
	MV_U8 page_code;
	MV_U8 reserved1;
	MV_U8 page_length[2];  
	MV_U8 generation_code[4];
} element_descriptor_page;

typedef struct _element_descriptor
{
	MV_U8 reserved1;
	MV_U8 reserved2;
	MV_U8 descriptor_length[2];   
	MV_U8 descriptor[1];
} element_descriptor;


/* SES_PG_ENCLOSURE_STATUS */
typedef struct _overall_enclosure_status
{
	MV_U8 unrecoverable_condition:1;
	MV_U8 critical_condition:1;
	MV_U8 noncritical_condition:1;
	MV_U8 information_condition:1;
	MV_U8 invalid_operation_requested:1;
	MV_U8 reserved1:3;
} overall_enclosure_status;

typedef struct _enclosure_status_page
{
	MV_U8 page_code;
	overall_enclosure_status status;
	MV_U8 page_length[2];   
	MV_U8 generation_code[4];
} enclosure_status_page;

typedef struct _device_element_for_enclosure_status_page
{
	MV_U8 element_status_code:4;
	MV_U8 swap:1;
	MV_U8 reserved1:1;
	MV_U8 prdfail:1;
	MV_U8 reserved2:1;

	MV_U8 slot_address;

	MV_U8 report:1;
	MV_U8 ident:1;
	MV_U8 rmv:1;
	MV_U8 ready_to_insert:1;
	MV_U8 enclosure_bypassed_b:1;
	MV_U8 enclosure_bypassed_a:1;
	MV_U8 do_not_remove:1;
	MV_U8 app_client_bypassed_a:1;

	MV_U8 device_bypassed_b:1;
	MV_U8 device_bypassed_a:1;
	MV_U8 bypassed_b:1;
	MV_U8 bypassed_a:1;
	MV_U8 device_off:1;
	MV_U8 fault_reqstd:1;
	MV_U8 fault_sensed:1;
	MV_U8 app_client_bypassed_b:1;
}device_element_for_enclosure_status_page;

/* Element Status */
enum
{
	SES_STATUS_UNSUPPORTED = 0x00,
	SES_STATUS_OK,
	SES_STATUS_CRITICAL,
	SES_STATUS_NONCRITICAL,
	SES_STATUS_UNRECOVERABLE,
	SES_STATUS_NOTINSTALLED,
	SES_STATUS_UNKNOWN,
	SES_STATUS_NOTAVAILABLE,
	SES_STATUS_RESERVED
}; 


typedef struct _array_element_for_enclosure_status_page
{
	MV_U8 status_code:4;
	MV_U8 swap:1;
	MV_U8 reserved1:1;
	MV_U8 predicted_failure:1;
	MV_U8 reserved2:1;

	MV_U8 rebuild_remap_abort:1;
	MV_U8 rebuild_remap:1;
	MV_U8 in_failed_array:1;
	MV_U8 in_critical_array:1;
	MV_U8 consistency_check:1;
	MV_U8 hot_spare:1;
	MV_U8 reserved_device:1;
	MV_U8 ok:1;

	MV_U8 report:1;
	MV_U8 identify:1;
	MV_U8 rmv:1;
	MV_U8 ready_to_insert:1;
	MV_U8 enclosure_bypassed_b:1;
	MV_U8 enclosure_bypassed_a:1;
	MV_U8 do_not_remove:1;
	MV_U8 app_bypassed_a:1;

	MV_U8 device_bypassed_b:1;
	MV_U8 device_bypassed_a:1;
	MV_U8 bypassed_b:1;
	MV_U8 bypassed_a:1;
	MV_U8 device_off:1;
	MV_U8 fault_requested:1;
	MV_U8 fault_sensed:1;
	MV_U8 app_bypassed_b:1;
} array_element_for_enclosure_status_page;

/* SES_PG_ENCLOSURE_CONTROL */
typedef struct _enclosure_control_page
{
	MV_U8 page_code;
	MV_U8 unrecoverable_condition:1;
	MV_U8 critical_condition:1;
	MV_U8 noncritical_condition:1;
	MV_U8 information_condition:1;
	MV_U8 reserved1:4;
	MV_U8 page_length[2];   
	MV_U8 generation_code[4];
} enclosure_control_page;

typedef struct _device_element_for_enclosure_control_page
{
	MV_U8 reserved1:4;
	MV_U8 reset_swap:1;
	MV_U8 disable:1;
	MV_U8 predicted_failure:1;
	MV_U8 select:1;

	MV_U8 reserved2;

	MV_U8 reserved3:1;
	MV_U8 request_identify:1;
	MV_U8 request_remove:1;
	MV_U8 request_insert:1;
	MV_U8 reserved4:2;
	MV_U8 do_not_remove:1;
	MV_U8 active:1;

	MV_U8 reserved5:2;
	MV_U8 enable_bypass_b:1;
	MV_U8 enable_bypass_a:1;
	MV_U8 device_off:1;
	MV_U8 request_fault:1;
	MV_U8 reserved6:2;
} device_element_for_enclosure_control_page;

typedef struct _array_element_for_enclosure_control_page
{
	MV_U8 reserved1:4;
	MV_U8 reset_swap:1;
	MV_U8 pdisable:1;
	MV_U8 predicted_failure:1;
	MV_U8 select:1;

	MV_U8 request_rebuild_remap_abort:1;
	MV_U8 request_rebuild_remap:1;
	MV_U8 request_in_failed_array:1;
	MV_U8 request_in_critical_array:1;
	MV_U8 request_consistency_check:1;
	MV_U8 request_hotspare:1;
	MV_U8 request_reserved_device:1;
	MV_U8 request_ok:1;

	MV_U8 reserved2:1;
	MV_U8 request_identify:1;
	MV_U8 request_remove:1;
	MV_U8 request_insert:1;
	MV_U8 reserved3:2;
	MV_U8 do_not_remove:1;
	MV_U8 active:1;

	MV_U8 reserved4:2;
	MV_U8 enable_bypass_b:1;
	MV_U8 enable_bypass_a:1;
	MV_U8 device_off:1;
	MV_U8 request_fault:1;
	MV_U8 reserved5:2;
} array_element_for_enclosure_control_page;

typedef union 
{
	device_element_for_enclosure_status_page status;
	device_element_for_enclosure_control_page control;	
}device_enclosure_element;

typedef union 
{
	array_element_for_enclosure_status_page status;
	array_element_for_enclosure_control_page control;	
}array_enclosure_element;

typedef union 
{
	device_enclosure_element device_type;
	array_enclosure_element array_type;	
}ses_enclosure_element;

