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
#ifndef __CORE_DISCOVER_H
#define __CORE_DISCOVER_H

#include "mv_config.h"
#include "core_sata.h"

/* assume the maximum number of phys in an expander device is 128 */
#define MAXIMUM_EXPANDER_PHYS 		128
/* assume the maximum number of indexes per phy is 128 */
#define MAXIMUM_EXPANDER_INDEXES 		128

/* limit to 8 initiators for this example */
#define MAXIMUM_INITIATORS 			8

/* defines for address frame types */
#define OF_FRAME_TYPE_SHIFT                              0
#define ADDRESS_IDENTIFY_FRAME 		0x00
#define ADDRESS_OPEN_FRAME 			0x01


/* defines for SMP frame types */
#define SMP_REQUEST_FRAME 			0x40
#define SMP_RESPONSE_FRAME 			0x41


/* defines for SMP request functions */
#define REPORT_GENERAL 				0x00
#define REPORT_MANUFACTURER_INFORMATION 	0x01
#define READ_GPIO_REGISTER			0x02
#define REPORT_SELF_CONFIGURATION_STATUS	0x03
#define DISCOVER 				0x10
#define REPORT_PHY_ERROR_LOG 			0x11
#define REPORT_PHY_SATA 			0x12
#define REPORT_ROUTE_INFORMATION 		0x13
#define REPORT_PHY_EVENT_INFORMATION		0x14
#define REPORT_PHY_BROADCAST_COUNTS		0x15
#define DISCOVER_LIST				0x16
#define REPORT_EXPANDER_ROUTE_TABLE		0x17
#define CONFIGURE_GENERAL			0x80
#define ENABLE_DISABLE_ZONING			0x81
#define WRITE_GPIO_REGISTER 			0x82
#define ZONED_BROADCAST				0x85
#define CONFIGURE_ROUTE_INFORMATION 		0x90
#define PHY_CONTROL 				0x91
#define PHY_TEST 				0x92
#define CONFIGURE_PHY_EVENT_INFORMATION		0x93


enum sas_proto {
	SATA_PROTO    = 1,
	SAS_PROTO_SMP = 2,		/* protocol */
	SAS_PROTO_STP = 4,		/* protocol */
	SAS_PROTO_SSP = 8,		/* protocol */
	SAS_PROTO_ALL = 0xE,
};

/* defines for open responses, arbitrary values, not defined in the spec */
#define OPEN_ACCEPT 				0
#define OPEN_REJECT_BAD_DESTINATION 		1
#define OPEN_REJECT_RATE_NOT_SUPPORTED 	2
#define OPEN_REJECT_NO_DESTINATION 		3
#define OPEN_REJECT_PATHWAY_BLOCKED 		4
#define OPEN_REJECT_PROTOCOL_NOT_SUPPORTED 	5
#define OPEN_REJECT_RESERVE_ABANDON 		6
#define OPEN_REJECT_RESERVE_CONTINUE 		7
#define OPEN_REJECT_RESERVE_INITIALIZE 	8
#define OPEN_REJECT_RESERVE_STOP 		9
#define OPEN_REJECT_RETRY 			10
#define OPEN_REJECT_STP_RESOURCES_BUSY 	11
#define OPEN_REJECT_WRONG_DESTINATION 	12
#define OPEN_REJECT_WAITING_ON_BREAK 		13

/* definitions for discovery algorithm use */
enum
{
	SAS_SIMPLE_LEVEL_DESCENT = 0,
	SAS_UNIQUE_LEVEL_DESCENT
};
enum
{
	SAS_10_COMPATIBLE = 0,
	SAS_11_COMPATIBLE
};

/* definitions for SMP function results */
enum SMPFunctionResult
{
	SMP_REQUEST_ACCEPTED = 0x0,
	SMP_FUNCTION_ACCEPTED = 0x0,
	SMP_UNKNOWN_FUNCTION = 0x1,
	SMP_FUNCTION_FAILED = 0x2,
	SMP_INVALID_REQUEST_FRAME_LENGTH = 0x3,
        SMP_INVALID_EXPANDER_CHANGE_COUNT = 0x4,
        SMP_BUSY = 0x5,
        SMP_INCOMPLETE_DESCRIPTOR_LIST = 0x6,

	SMP_PHY_DOES_NOT_EXIST = 0x10,
	SMP_INDEX_DOES_NOT_EXIST = 0x11,
	SMP_PHY_DOES_NOT_SUPPORT_SATA = 0x12,
	SMP_UNKNOWN_PHY_OPERATION = 0x13,
	SMP_UNKNOWN_PHY_TEST_FUNCTION = 0x14,
	SMP_PHY_TEST_FUNCTION_IN_PROGRESS = 0x15,
	SMP_PHY_VACANT = 0x16,
	SMP_UNKNOWN_PHY_EVENT_SOURCE = 0x17,
        SMP_UNKNOWN_DESCRIPTOR_TYPE = 0x18,
        SMP_UNKNOW_PHY_FILTER = 0x19,
        SMP_AFFILIATION_VIOLATION = 0x1a,

	SMP_ZONE_VIOLATION = 0x20,
	SMP_NO_MANAGEMENT_ACCESS_RIGHTS = 0x21,
	SMP_UNKNOWN_ENABLE_DISABLE_ZONING_VALUE = 0x22,
        SMP_ZONE_LOCK_VIOLATION = 0x23,
        SMP_NOT_ACTIVATED = 0x24,
        SMP_ZONE_GROUP_OUT_OF_RANGE = 0x25,
        SMP_NO_PHYSICAL_PRESENCE = 0x26,
        SMP_SAVING_NOT_SUPPORTED = 0x27,
        SMP_SOURCE_ZONE_GROUP_DOES_NOT_EXIST = 0X28,
        SMP_DISABLED_PASSWORD_NOT_SUPPORTED = 0x29,
};


/* DEV_TYPE values */
enum DeviceType {
	NO_DEVICE   = 0,	  /* protocol */
	SAS_END_DEV = 1,	  /* protocol */
	EDGE_DEV    = 2,	  /* protocol */
	FANOUT_DEV  = 3,	  /* protocol */
	SAS_HA      = 4,
	SATA_DEV    = 5,
	SATA_PM     = 7,
	SATA_PM_PORT= 8,
};


/* RoutingAttribute */
enum RoutingAttribute
{
	DIRECT = 0,
	SUBTRACTIVE,
	TABLE,
	/*
		this attribute is a psuedo attribute, used to reflect the function
		result of SMP_PHY_VACANT in a fabricated discover response
	*/
	PHY_NOT_USED = 15
};

/* ConnectorType */
enum ConnectorType
{
	UNKNOWN_CONNECTOR = 0,
	SFF_8470_EXTERNAL_WIDE,
	SFF_8484_INTERNAL_WIDE = 0x10,
	SFF_8484_COMPACT_INTERNAL_WIDE,
	SFF_8482_BACKPLANE = 0x20,
	SATA_HOST_PLUG,
	SAS_DEVICE_PLUG,
	SATA_DEVICE_PLUG
};
/* RouteFlag */
enum DisableRouteEntry
{
	ENABLED = 0,
	DISABLED
};

/* Connect_Rate */
enum PhysicalLinkRate {
	PHY_LINKRATE_NONE = 0,
	PHY_LINKRATE_UNKNOWN = 0,
	PHY_DISABLED,
	PHY_RESET_PROBLEM,
	PHY_SPINUP_HOLD,
	PHY_PORT_SELECTOR,
	RESET_IN_PROGRESS,
	/*
	 * this is a psuedo link rate, used to reflect the function
	 * result of SMP_PHY_VACANT in a fabricated discover response
	*/
	PHY_DOES_NOT_EXIST,
	PHY_LINKRATE_1_5 = 0x08,
	PHY_LINKRATE_G1  = PHY_LINKRATE_1_5,
	PHY_LINKRATE_3   = 0x09,
	PHY_LINKRATE_G2  = PHY_LINKRATE_3,
	PHY_LINKRATE_6   = 0x0A,
};

/* PhyOperation */
enum PhyOperation
{
	PHY_NOP = 0,
	LINK_RESET,
	HARD_RESET,
	DISABLE,
	CLEAR_ERROR_LOG = 5,
	CLEAR_AFFILIATION,
	TRANSMIT_SATA_PORT_SELECTION_SIGNAL,
	CLEAR_STP_NEXUS_LOSS
};

/* EnableDisableZoning*/
enum EnableDisableZoning
{
	NO_CHANGE=0,
	ENABLE_ZONING,
	DISABLE_ZONING,
};

/* BroadcastType*/
enum BroadcastType
{
	BROADCAST_CHANGE = 0,
	BROADCAST_RESERVED_CHANGE0,
	BROADCAST_RESERVED_CHANGE1,
	BROADCAST_SES,
	BROADCAST_EXPANDER,
	BROADCAST_ASYNCHRONOUS_EVENT,
	BROADCAST_RESERVED_CHANGE3,
	BROADCAST_RESERVED_CHANGE4,
};

/* DescriptorType */
enum DescriptorType
{
	LONG_FORMAT_DESCRIPTOR=0,
	SHORT_FORMAT_DESCRIPTOR,
};

/* PortType */
enum PortType
{
	PORT_IS_TARGET=0,
	PORT_IS_INITIATOR
};


/*
 *   The structures assume a char bitfield is valid, this is compiler
 * dependent defines would be more portable, but less descriptive
 * the Identify frame is exchanged following OOB, for this
 * code it contains the identity information for the attached device
 * and the initiator application client.
 */
enum
{
	FRAME_PORT_TYPE_SMP         = (1 << 1),
	FRAME_PORT_TYPE_STP         = (1 << 2),
	FRAME_PORT_TYPE_SSP         = (1 << 3),
	FRAME_ATT_DEV_TYPE_SMP      = (1 << 1),
	FRAME_ATT_DEV_TYPE_STP      = (1 << 2),
	FRAME_ATT_DEV_TYPE_SSP      = (1 << 3),
	FRAME_ATT_DEV_SATA_DEV      = (1 << 0),
	FRAME_ATT_DEV_SATA_HOST     = (1 << 0),
	FRAME_ATT_DEV_SATA_SELECTOR = (1 << 7),
};

struct Identify
{
/* byte 0 */
#ifdef __MV_BIG_ENDIAN_BITFIELD__
	MV_U8 RestrictedByte0Bit7 : 1;
	MV_U8 DeviceType          : 3;
	MV_U8 AddressFrameType    : 4;
#else
	/* 	0x0: ADDRESS_IDENTIFY_FRAME */
	MV_U8 AddressFrameType    : 4;
	/*  001b: End device
	 *  010b: Edge expander device
	 *  011b: Fanout expander device
	 *  Others: Reserved.
	 */
	MV_U8 DeviceType          : 3;
	MV_U8 RestrictedByte0Bit7 : 1;
#endif /*  __MV_BIG_ENDIAN_BITFIELD__ */
/* byte 1 */
	MV_U8 RestrictedByte1;
/* byte 2 */
	MV_U8 InitiatorBits;
/* byte 3 */
	MV_U8 TargetBits;
/* byte 4-11 */
	MV_U8 DeviceName[8];
/*  byte 12-19  */
	MV_U64 SASAddress;
/* byte 20 */
	MV_U8 PhyIdentifier;
/* byte 21 */
#ifdef __MV_BIG_ENDIAN_BITFIELD__
	MV_U8 ReservedByte21Bit1_7 : 7;
	MV_U8 BreakReplyCapable    : 1;
#else
	MV_U8 BreakReplyCapable    : 1;
	MV_U8 ReservedByte21Bit1_7 : 7;
#endif /* __MV_BIG_ENDIAN_BITFIELD__ */
/* byte 22-27 */
	MV_U8 ReservedByte22_27[6];
/* byte 28-31 */
	MV_U32 CRC;
}; /* struct Identify */

/*
 * replaced by struct _OPEN_ADDRESS_FRAME
*/

/* request specific bytes for a general input function */
struct SMPRequestGeneralInput
{
	/* byte 4-7 */
	MV_U32 CRC;
};

/* request specific bytes for a phy input function */
struct SMPRequestPhyInput
{
	/* byte 4-7 */
	MV_U8 IgnoredByte4_7[4];
	/* byte 8 */
	MV_U8 ReservedByte8;
	/* byte 9 */
	MV_U8 PhyIdentifier;
	/* byte 10 */
	MV_U8 IgnoredByte10;
	/* byte 11 */
	MV_U8 ReservedByte11;
	/* byte 12-15 */
	MV_U32 CRC;
}; /* struct SMPRequestPhyInput */


/* request specific bytes for a self configuration expander function */
struct SMPRequestSelfConfigurationInput
{
	/* byte 4-6 */
	MV_U8 IgnoredByte4_6[3];
	/* byte 7 */
	MV_U8 StartSelfConfigurationStatusDescriptorIndex;
	/* byte 8-11 */
	MV_U32 CRC;
}; /* struct SMPRequestSelfConfigurationInput */

/* request specific bytes for a request route function */
struct SMPRequestRouteInformationInput
{
	/* byte 4-5 */
	MV_U8 IgnoredByte4_5[2];
	/* byte 6-7 */
	MV_U8 ExpanderRouteIndex[2];
	/* byte 8 */
	MV_U8 ReservedByte8;
	/* byte 9 */
	MV_U8 PhyIdentifier;
	/* byte 10 */
	MV_U8 IgnoredByte10;
	/* byte 11 */
	MV_U8 ReservedByte11;
	/* byte 12-15 */
	MV_U32 CRC;
}; /* struct SMPRequestRouteInformationInput */

/* request specific bytes for SMP DISCOVER LIST function */
struct SMPRequestDiscoverList
{
	/* byte 4-7 */
	MV_U8 ReservedByte4_7[4];
	/* byte 8 */
	MV_U8 StartingPhyIdentifier;
	/* byte 9 */
	MV_U8 MaximumNumberOfDescriptors;

	/* byte 10 */
	MV_U8 PhyFilter:4;
	MV_U8 IgnoredByte10Bit4_6:3;
	MV_U8 IgnoreZoneGroup:1;

	/* byte 11 */
	MV_U8 DescriptorType:4;
	MV_U8 IgnoredByte11Bit4_7:4;

	/* byte 12-15 */
	MV_U8 IgnoredByte12_15[4];

	/* byte 16-27 */
	MV_U8 VendorSpecific[12];

	/* byte 28-31 */
	MV_U32 CRC;
}; /* struct SMPRequestDiscoverList */

/* request specific bytes for SMP REPORT EXPANDER ROUTE TABLE function */
struct SMPRequestReportExpanderRouteTable
{
	/* byte 4-7 */
	MV_U8 ReservedByte4_7[4];
	/* byte 8-9 */
	MV_U8 MaximumNumberOfDescriptors[2];

	/* byte 10-15 */
	MV_U8 StartingRoutedSASAddressIndex[6];

	/* byte 16-18 */
	MV_U8 IgnoredByte16_18[3];

	/* byte 19 */
	MV_U8 StartingPhyIdentifier;

	/* byte 20-27 */
	MV_U8 IgnoredByte20_27[8];

	/* byte 28-31 */
	MV_U32 CRC;
}; /* struct SMPRequestReportExpanderRouteTable */

/* request specific bytes for SMP CONFIGUARE GENERAL function */
struct SMPRequestConfigureGeneral
{
	/* byte 4-5 */
	MV_U8 ExpectedExpanderChangeCount[2];
	/* byte 6-7 */
	MV_U8 IgnoredByte6_7[2];

	/* byte 8 */
	MV_U8 UpdateSTPBusInactivityTimeLimit:1;
	MV_U8 UpdateSTPMaxConnectTimeLimit:1;
	MV_U8 UpdateSTPSMPNexusLossTime:1;
	MV_U8 IgnoredByte8Bit3_7:5;

	/* byte 9 */
	MV_U8 IgnoredByte9;

	/* byte 10-11 */
	MV_U8 STPBusInactivityTimeLimit[2];
	/* byte 12-13 */
	MV_U8 STPMaxConnectTimeLimit[2];

	/* byte 14-15 */
	MV_U8 STPSMPNexusLossTime[2];

	/* byte 16-19 */
	MV_U32 CRC;
}; /* struct SMPRequestConfigureGeneral */

/* request specific bytes for ENABLE DISABLE ZONING function */
struct SMPRequestEnableDisableZoning
{
	/* byte 4-5 */
	MV_U8 ExpectedExpanderChangeCount[2];

	/* byte 6-7 */
	MV_U8 IgnoredByte4_5[2];

	/* byte 8 */
	MV_U8 EnableDisableZoning:1;
	MV_U8 ReservedByte8Bit1_7:7;

	/* byte 9-11 */
	MV_U8 IgnoredByte9_11;

	/* byte 12-15 */
	MV_U32 CRC;
}; /* struct SMPRequestEnableDisableZoning */

/* request specific bytes for SMP ZONED BROADCAST function */
struct SMPRequestZonedBroadcast
{
	/* byte 4-5 */
	MV_U8 RestrictByte4_5[2];
	/* byte 6 */
	MV_U8 BroadcastType:3;
	MV_U8 ReservedByte6Bit3_7:5;
	/* byte 7 */
	MV_U8 NumberOfBroadcastSourceZoneGroups;
	/* byte 8-135 */
	MV_U8 BroadcastSourceZoneGroup[0x80];
	/* value between 0 and 127 */
	/*
	 * byte 135 -n: PAD field
	 * Contains zero,one,two or three bytes set to 0x00 such that
	 * total length of the SMP request is a multiple of four
	*/
	MV_U8 Pad[3];
	/* byte n-4-n */
	MV_U32 CRC;
}; /* struct SMPRequestZonedBroadcast */

/* request specific bytes for SMP ConfigureRouteInformation function */
struct SMPRequestConfigureRouteInformation
{
	/* byte 4-5 */
	MV_U8 ExpectedExpanderChangeCount[2];
	/* byte 6-7 */
	MV_U8 ExpanderRouteIndex[2];
	/* byte 8 */
	MV_U8 ReservedByte8;
	/* byte 9 */
	MV_U8 PhyIdentifier;
	/* byte 10-11 */
	MV_U8 ReservedByte10_11[2];
	/* byte 12 */
	MV_U8 IgnoredByte12Bit0_6:7;
	/*
	* if a routing error is detected
	* then the route is disabled by
	* setting this bit
	*/
	MV_U8 DisableRouteEntry:1;
	/* byte 13-15 */
	MV_U8 IgnoredByte13_15[3];
	/* byte 16-23 */
	MV_U8 RoutedSASAddress[8]; 	/*
					 * identical to the AttachedSASAddress
					 * found through discovery
					 */
	/* byte 24-39 */
	MV_U8 ReservedByte24_39[16];

	/* byte 40-43 */
	MV_U32 CRC;
}; /* struct SMPRequestConfigureRouteInformation */

/* request specific bytes for SMP Phy Control function */
struct SMPRequestPhyControl
{
	/* byte 4-5 */
	MV_U8 ExpectedExpanderChangeCount[2];

	/* byte 6-8 */
	MV_U8 IgnoredByte6_8[3];

	/* byte 9 */
	MV_U8 PhyIdentifier;

	/* byte 10 */
	MV_U8 PhyOperation;

	/* byte 11 */
	MV_U8 UpdatePartialPathwayTimeoutValue:1;
	MV_U8 ReservedByte11Bit1_7:7;

	/* byte 12-31 */
	MV_U8 IgnoredByte12_31[20];

	/* byte 32 */
	MV_U8 IgnoredByte32Bit0_3:4;
	MV_U8 ProgrammedMinimumPhysicalLinkRate:4;

	/* byte 33 */
	MV_U8 IgnoredByte33Bit0_3:4;
	MV_U8 ProgrammedMaximumPhysicalLinkRate:4;

	/* byte 34-35 */
	MV_U8 IgnoredByte34_35[2];

	/* byte 36 */
	MV_U8 PartialPathwayTimeoutValue:4;
	MV_U8 ReservedByte36Bit4_7:4;

	/* byte 37-39 */
	MV_U8 ReservedByte37_39[3];

	/* byte 40-43 */
	MV_U32 CRC;
}; /* struct SMPRequestPhyControl */

/* request specific bytes for SMP Phy Test function */
struct SMPRequestPhyTest
{
	/* byte 4-5 */
	MV_U8 ExpectedExpanderChangeCount[2];

	/* byte 6-8 */
	MV_U8 IgnoredByte6_8[3];

	/* byte 9 */
	MV_U8 PhyIdentifier;

	/* byte 10 */
	MV_U8 PhyTestFunction;

	/* byte 11 */
	MV_U8 PhyTestPattern;

	/* byte 12-14 */
	MV_U8 ReservedByte12_14[3];

	/* byte 15 */
	MV_U8 PhyTestPatternPhysicalLinkRate:4;
	MV_U8 ReservedByte15Bit4_7:4;

	/* byte 16-18 */
	MV_U8 ReservedByte16_18[3];

	/* byte 19 */
	MV_U8 PhyTestPatternDwordsControl;

	/* byte 20-27 */
	MV_U8 PhyTestPatternDwords[8];

	/* byte 28-39 */
	MV_U8 ReservedByte28_39[12];

	/* byte 40-43 */
	MV_U32 CRC;
}; /* struct SMPRequestPhyTest */

/* request specific bytes for SMP CONFIGUARE PHY EVENT INFORMATION function */
struct SMPRequestConfigurePhyEventInformation
{
	/* byte 4-5 */
	MV_U8 ExpectedExpanderChangeCount[2];

	/* byte 6 */
	MV_U8 ClearPeaks:1;
	MV_U8 ReservedByte6Bit1_7:6;

	/* byte 7-8 */
	MV_U8 ReservedByte7_8[2];

	/* byte 9 */
	MV_U8 PhyIdentifier;

	/* byte 10 */
	MV_U8 ReservedByte10;

	/* byte 11 */
	MV_U8 NumberOfPhyEventConfigurationDescriptors;

	/* byte 11-138 */
	MV_U8 PhyEventConfigurationDescriptors[0x80];
	MV_U32 CRC;
}; /* struct SMPRequestConfigurePhyEventInformation */


/*
 * request sent specific to SMP Read SGPIO
 * Corresponds to READ_GPIO_REGISTER define
 */
struct SMPRequestReadGPIORegister
{
	/* byte 4 */
	MV_U8 RegisterCount;
	/* byte 5-7 */
	MV_U8 ReservedByte[3];
	MV_U32 CRC;
};

/*
	request sent specific to SMP Write SGPIO
	Corresponds to WRITE_GPIO_REGISTER define
*/
struct SMPRequestWriteGPIORegister
{
	/* byte 4 */
	MV_U8 RegisterCount;

	/* byte 5-7 */
	MV_U8 ReservedByte[3];

	/* byte 8-15 */
	MV_U8 Data_Out_Low[8];

	/* byte 16-23 */
	MV_U8 Data_Out_High[8];

	MV_U32 CRC;
};

/*
	generic structure referencing an SMP Request, must be initialized
	before being used
*/
typedef struct _smp_request
{
	/* byte 0 */
	MV_U8 smp_frame_type; /* always SMP_REQUEST_FRAME = 0x40 */

	/* byte 1 */
	MV_U8 function;

	/* byte 2 */
	MV_U8 resp_len;		/* Allocated response length, set to zero 
				   for compatibility */
	/* byte 3 */
	MV_U8 req_len;		/* Request length, set to zero 
				   for compatibility */
	/* bytes 4-n */
	union
	{
	/* #define REPORT_GENERAL 			0x00 */
		struct SMPRequestGeneralInput 		ReportGeneral;

	/* #define REPORT_MANUFACTURER_INFORMATION 	0x01 */
		struct SMPRequestGeneralInput 		ReportManufacturerInformation;

	/* #define READ_GPIO_REGISTER			0x02 */
		struct SMPRequestReadGPIORegister		ReadGPIORegister;

	/* #define REPORT_SELF_CONFIGURATION_STATUS	0x03 */
		struct SMPRequestSelfConfigurationInput	ReportSelfConfigurationStatus;

	/* #define DISCOVER 				0x10 */
		struct SMPRequestPhyInput 		Discover;

	/* #define REPORT_PHY_ERROR_LOG 			0x11 */
		struct SMPRequestPhyInput 		ReportPhyErrorLog;

	/* #define REPORT_PHY_SATA 			0x12 */
		struct SMPRequestPhyInput 		ReportPhySATA;

	/* #define REPORT_ROUTE_INFORMATION 		0x13 */
		struct SMPRequestRouteInformationInput 	ReportRouteInformation;

	/* #define REPORT_PHY_EVENT_INFORMATION		0x14 */
		struct SMPRequestPhyInput			ReportPhyEventInformation;

	/* #define REPORT_PHY_BROADCAST_COUNTS		0x15 */
		struct SMPRequestPhyInput			ReportPhyBroadcaseCounts;

	/* #define DISCOVER_LIST				0x16 */
		struct SMPRequestDiscoverList		DiscoverList;

	/* #define REPORT_EXPANDER_ROUTE_TABLE		0x17 */
		struct SMPRequestReportExpanderRouteTable	ReportExpanderRouteTable;

	/* #define CONFIGURE_GENERAL			0x80 */
		struct SMPRequestConfigureGeneral		ConfigureGeneral;

	/* #define ENABLE_DISABLE_ZONING			0x81 */
		struct SMPRequestEnableDisableZoning	EnableDisableZoning;

	/* #define WRITE_GPIO_REGISTER			0x82 */
		struct SMPRequestWriteGPIORegister		WriteGPIORegister;

	/* #define ZONED_BROADCAST			0x85 */
		struct SMPRequestZonedBroadcast		ZonedBroadcast;

	/* #define CONFIGURE_ROUTE_INFORMATION 		0x90 */
		struct SMPRequestConfigureRouteInformation 	ConfigureRouteInformation;

	/* #define PHY_CONTROL 				0x91 */
		struct SMPRequestPhyControl 		PhyControl;

	/* #define PHY_TEST 				0x92 */
		struct SMPRequestPhyTest 			PhyTest;

	/* #define CONFIGURE_PHY_EVENT_INFORMATION	0x93 */
		struct SMPRequestConfigurePhyEventInformation	ConfigurePhyEventInformation;

	} request;
} smp_request; /* struct _smp_request */

typedef struct _smp_request mv_smp_command_table;

/*
request specific bytes for SMP Report General response, intended to be
referenced by SMPResponse
*/
struct SMPResponseReportGeneral
{
	/*
	byte 4-5: EXPANDER CHANGE COUNT
	Counts the number of Broadcast (Change)s originated by an expander device.
	Management device servers in expander devices shall support this field.
	Management device servers in other device types (e.g., end devices) shall
	set this field to 0000h. This field shall be set to at least 0001h at power
	on. If the expander device has originated Broadcast (Change) for any reason
	described in 7.11 since transmitting a REPORT GENERAL response, it shall increment
	this field at least once from the value in the previous REPORT GENERAL response.
	It shall not increment this field when forwarding a Broadcast (Change). This field
	shall wrap to at least 0001h after the maximum value (i.e., FFFFh) has been reached.
	*/
	MV_U8 ExpanderChangeCount[2];
	/*
	byte 6-7: EXPANDER ROUTE INDEXES
	Contains the maximum number of expander route indexes per phy for the expander
	device. Management device servers in externally configurable expander devices
	containing phy-based expander route tables shall support this field. Management
	device servers in other device types (e.g., end devices, externally configurable
	expander devices with expander-based expander route tables, and self-configuring
	expander devices) shall set the EXPANDER ROUTE INDEXES field to zero. Not all
	phys in an externally configurable expander device are required to support the
	maximum number indicated by this field.
	*/
	MV_U8 ExpanderRouteIndexes[2];

	/* byte 8: reserved */

	MV_U8 ReservedByte8;
	/*
	byte 9:NUMBER OF PHYS
	Contains the number of phys in the device, including any virtual phys and any
	vacant phys
	*/
	MV_U8 NumberOfPhys;


	/*
	byte 10-bit0:
	EXTERNALLY CONFIGURABLE ROUTE TABLE
	Set to one indicates that the management device server is in an externally
	configurable expander device that has a phy-based expander route table
	that is required to be configured with the SMP CONFIGURE ROUTE INFORMATION
	function. An EXTERNALLY CONFIGURABLE ROUTE TABLE bit set to zero indicates
	that the management device server is not in an externally configurable expander
	device (e.g., it is in an end device, in a self-configuring expander device,
	or in an expander device with no phys with table routing attributes).

	byte 10-bit1:
	CONFIGURING
	Set to one indicates that the management device server is in a self-configuring
	expander device, the self-configuring expander device¡¯s management application
	client is currently performing the discover process, and it has identified at
	least one change to its expander routing table. A CONFIGURING bit set to zero
	indicates that the management device server is not in a self-configuring expander
	device currently performing the discover process and changing its expander
	routing table. Changes in this bit from one to zero result in a Broadcast (Change)
	being originated. Management device servers in self-configuring expander
	devices shall support this bit. Management device servers in externally
	configurable expander devices and in other device types shall set the CONFIGURING
	bit to zero.

	byte 10-bit2:
	CONFIGURES OTHERS
	Set to one indicates that the expander device is a self-configuring expander
	device that performs the configuration subprocess. A CONFIGURES OTHERS bit set
	to zero indicates the expander device may or may not perform the configuration
	subprocess. Self-configuring expander devices compliant with this standard
	shall set the CONFIGURES OTHERS bit to one.If the CONFIGURES OTHERS bit is set
	to zero, the expander device may configure all externally configurable expander
	devices in the SAS domain.

	byte 10-bit7:
	TABLE TO TABLE SUPPORTED
	Set to one indicates the expander device is a self-configuring expander
	device that supports its table routing phys being attached to table routing
	phys in other expander devices. The TABLE TO TABLE SUPPORTED bit shall only
	be set to one if the EXTERNALLY CONFIGURABLE ROUTE TABLE bit is set to
	zero. A TABLE TO TABLE SUPPORTED bit set to zero indicates the expander device
	is not a self-configuring expander device that supports its table routing
	phys being attached to table routing phys in other expander devices.
	*/


	MV_U8 ConfigurableRouteTable:1;		/* BIT 0*/
	MV_U8 Configuring:1;					/* BIT 1*/
	MV_U8 ConfiguresOthers:1;			/* BIT 2*/
	MV_U8 ReservedByte10Bit3_6:4;		/* BIT 3-6*/
	MV_U8 TableToTableSupported:1;		/* BIT 7*/

	/* byte 11: reserved */
	MV_U8 ReservedByte11;

	/*
	byte 12-19:
	ENCLOSURE LOGICAL IDENTIFIER
	Identifies the enclosure, if any, in which the device is located, and is
	defined in SES-2. The ENCLOSURE LOGICAL IDENTIFIER field shall be set to
	the same value reported by the enclosure services process, if any, for
	the enclosure. An ENCLOSURE LOGICAL IDENTIFIER field set to zero indicates
	no enclosure information is available.
	*/
	MV_U8 EnclosureLogicalIdentifier[8];

	/* 	byte 20-29: reserved */
	MV_U8 ReservedByte20_29[10];

	/*
	byte 30-31:
	STP BUS INACTIVITY TIME LIMIT
	Contains the bus inactivity time limit for STP connections which is set by
	the CONFIGURE GENERAL function
	*/
	MV_U8 STPBusInactivityTimeLimit[2];

	/*
	byte 32-33:
	STP MAXIMUM CONNECT TIME LIMIT
	Contains the maximum connect time limit for STP connections
	which is set by the CONFIGURE GENERAL function.
	*/
	MV_U8 STPMaxConnectTimeLimit[2];

	/*
	byte 34-35:
	STP SMP I_T NEXUS LOSS TIME
	Contains the time that an STP target port and an SMP initiator port retry
	certain connection requests which is set by the CONFIGURE GENERAL function.
	*/
	MV_U8 STPSMPNexusLossTime[2];

	/*
	byte 36-bit0:
	ZONING ENABLED
	Set to one indicates that zoning is enabled in the expander device. A ZONING
	ENABLED bit set to zero indicates that zoning is disabled in the expander
	device. The ZONING ENABLED bit shall be set to zero if the ZONING SUPPORTED bit
	is set to zero.

	byte 36-bit1:
	ZONING SUPPORTED
	Set to one indicates that zoning is supported by the expander device(i.e., it
	is a zoning expander device). A ZONING SUPPORTED bit set to zero indicates that
	zoning is not supported by the expander device.

	byte 36-bit2:
	PHYSICAL PRESENCE ASSERTED
	Set to one indicates that the expander device is currently detecting
	physical presence. A PHYSICAL PRESENCE ASSERTED bit set to zero indicates that
	the expander device is not currently detecting physical presence. The PHYSICAL
	PRESENCE ASSERTED bit shall be set to zero if the PHYSICAL PRESENCE SUPPORTED
	bit is set to zero.

	byte 36-bit3:
	PHYSICAL PRESENCE SUPPORTED
	Set to one indicates that the expander device supports physical presense
	as a mechanism for allowing zoning to be enabled or disabled from phys in
	zone groups without access to zone group 2. A PHYSICAL PRESENCE SUPPORTED bit
	set to zero indicates that the expander device does not support physical
	presence as a mechanism for allowing zoning to be enabled or disabled.

	*/
	MV_U8 ZoningEnabled:1;				/* Bit 0*/
	MV_U8 ZoningSupported:1;			/* Bit 1*/
	MV_U8 PhysicalPresenceAsserted:1;	/* Bit 2*/
	MV_U8 PhysicalPresenceSupported:1;	/* Bit 3*/
	MV_U8 ReservedByte36Bit4_7:4;		/* Bit 4-7*/

	/* 	byte 37: reserved */
	MV_U8 ReservedByte37;

	/*
	byte 38-39:
	MAXIMUM NUMBER OF ROUTED SAS ADDRESSES
	Contains the number of routed SAS addresses in an expander-based expander route
	table. Management device servers in expander devices containing expander-based
	expander route tables shall support this field. Management device servers in
	other device types (e.g., end devices and expander devices with phy-based expander
	route tables) shall set this field to 0000h.
	*/
	MV_U8 MaxNumRoutedSASAddr[2];

	/* 	byte 40-43 CRC */
	MV_U32 CRC;
}; /* struct SMPResponseReportGeneral */


	/*
	request specific bytes for SMP Report Manufacturer Information response,
	intended to be referenced by SMPResponse
	*/
struct SMPResponseReportManufacturerInformation
{
	/* byte 4-5 */
	MV_U8 ExpanderChangeCount[2];

	/* byte 6-7 */
	MV_U8 ReservedByte6_7[2];

	/* byte 8	*/
	MV_U8 SAS11Format:1;
	MV_U8 ReservedByte8_Bit1_7:7;

	/* byte 9-11 */
	MV_U8 IgnoredByte9_11[3];

	/* byte 12-19 */
	MV_U8 VendorIdentification[8];

	/* byte 20-35 */
	MV_U8 ProductIdentification[16];

	/* byte 36-39 */
	MV_U8 ProductRevisionLevel[4];

	/* byte 40-47 */
	MV_U8 ComponentVendorIdentification[8];

	/* byte 48-49 */
	MV_U8 ComponentID[2];

	/* byte 50 */
	MV_U8 ComponentRevisionID;

	/* byte 51 */
	MV_U8 ReservedByte51;

	/* byte 52-59 */

	MV_U8 VendorSpecific[8];
	/* byte 60-63 */
	MV_U32 CRC;
}; /* struct SMPResponseReportManufacturerInformation */

/*
	request specific bytes for SMP Report Self-Configuration Status response,
	intended to be referenced by SMPResponse
*/
struct SMPResponseReportSelfConfigurationStatus
{
	/* byte 4-5 */
	MV_U8 ExpanderChangeCount[2];

	/* byte 6 */
	MV_U8 ReservedByte6;

	/* byte 7 */
	MV_U8 StartingSelfConfigurationStatusDescriptorIndex;

	/* byte 8-12	*/
	MV_U8 ReservedByte8_12[5];

	/* byte 13 */
	MV_U8 MaxSupportedSelfConfigurationStatusDescriptors;

	/* byte 14 */
	MV_U8 TotalNumberOfSelfConfigurationStatusDescriptors;

	/* byte 15 */
	MV_U8 NumberOfSelfConfigurationStatusDescriptors;

	/* byte 16-31 */
	MV_U8 SelfConfigurationStatusDescriptor[16];
/*
	byte SelfConfigurationStatusDescriptor_1[16];
		...
	byte SelfConfigurationStatusDescriptor_n[16];

*/
	MV_U32 CRC;
}; /* struct SMPResponseReportSelfConfigurationStatus */

/*
	request specific bytes for SMP Report Phy Error Log response,
	intended to be referenced by SMPResponse
*/
struct SMPResponseReportPhyErrorLog
{
	/* byte 4-5 */
	MV_U8 ExpanderChangeCount[2];

	/* byte 6-8 */
	MV_U8 ReservedByte6_8[3];

	/* byte 9 */
	MV_U8 PhyIdentifier;

	/* byte 10-11 */
	MV_U8 ReservedByte10_11[2];

	/* byte 12-15 */
	MV_U8 InvalidDwordCount[4];

	/* byte 16-19 */
	MV_U8 RunningDisparityErrorCount[4];

	/* byte 20-23 */
	MV_U8 LossOfDwordSynchronizationCount[4];

	/* byte 24-27 */
	MV_U8 PhyResetProblemCount[8];

	/* byte 28-31 */
	MV_U32 CRC;
}; /* struct SMPResponseReportPhyErrorLog */

/*
	Response specific bytes for SMP Discover Descriptor, intended to be
	referenced by SMPResponseDiscover, SMPResponseDiscoverList
*/
struct DiscoverLongFormatDescriptor
{
	/* byte 48-49: Reserved */
	MV_U8 ReservedByte48_49[2];

	/* byte 50-51: Vendor specific */
	MV_U8 VendorSpecific[2];

/*
	byte 52-59: ATTACHED DEVICE NAME
	Contains the value of the the device name field received in the IDENTIFY
	address frame during the identification sequence. If the attached port is
	an expander port or a SAS port, the ATTACHED DEVICE NAME field contains the
	device name of the attached expander device or SAS device. If the attached port
	is a SATA device port, the ATTACHED DEVICE NAME field contains 00000000 00000000h.
	The ATTACHED DEVICE NAME field shall be updated:
		a) after the identification sequence completes, if a SAS phy or expander phy is attached; or
		b) after the COMSAS Detect Timeout timer expires, if a SATA phy is attached.
*/
	MV_U8 AttachedDeviceName[8];

/*
Zone group		Configurable 	Description
				zone permission
				table entries

0 				No 		Phys in zone group 0 only have access to
						other phys in zone group 1.

1 				No 		Phys in zone group 1 have access to other
						phys in all zone groups.

2 				Yes		Phys in zone group 2 have access to other
						phys based on the zone permission table.
						A management device server in a zoning
						expander device with zoning enabled only
						allows management application clients using
						phys in zone groups with access to zone
						group 2 to perform the following SMP functions:
							a) SMP zoning configuration functions (e.g., CONFIGURE ZONE
							PERMISSION); and
							b) SMP expander configuration functions (i.e., CONFIGURE GENERAL).
						A management device server in a zoning
						expander device with zoning enabled only
						allows management application clients to
						perform certain SMP phy-based control and
						configuration functions (i.e., PHY CONTROL,
						PHY TEST FUNCTION, and CONFIGURE PHY EVENT
						INFORMATION) if the zone group of the management
						application client¡¯s phy has access to
						zone group 2 or the zone group of the specified phy.

3 			Yes			Phys in zone group 3 have access to other phys based
						on the zone permission table.
						A management device server in a zoning expander device
						with zoning enabled only allows management application
						clients using a phy in a zone group with access to
						zone group 3 to perform certain SMP zoning-related
						functions (i.e., ZONED BROADCAST).

4 to 7 			Reserved

8 to 127 Yes 					Phys in zone groups 8 through 127 have access to other
								phys based on the zone permission table.



			Source zone group determination
Attribute of the expander phy that		Source zone group
received the OPEN address frame
ZONE ADDRESS	Routing	INSIDE ZPSDS
RESOLVED bit	method	bit
0 				Any		0 		Zone group of the receiving expander phy

0 				Any 	1			Source zone group specified by the SOURCE ZONE
								GROUP field in the received OPEN address frame.

1				Direct	0			Zone group of the receiving expander phy.

1 				Direct	1			Not applicable.


1 				Subtractive 0			Zone group of the receiving expander phy.

1				Subtractive 1			Not applicable.


1				Table	0			Zone group stored in the zoning expander route
								table for the source SAS address. If the source
								SAS address is not found in the zoning expander
								route table then the zone group of the receiving
								expander phy.
1				Table 	1	 		Not applicable.



			Destination zone group determination
Routing method of the			Destination zone group
destination expander phy
Direct 						Zone group of the destination expander phy.

Subtractive 					Zone group of the destination expander phy (i.e., the
						subtractive expander phy).

Table 						Zone group stored in the zoning expander route table for
						the destination SAS address.

At the completion of a link reset sequence, if a SATA device is attached to an expander phy, the
zoning expander device with zoning enabled shall set the INSIDE ZPSDS bit to zero for that expander phy.
At the completion of a link reset sequence, if a SATA device is not attached to an expander phy, the zoning
expander device with zoning enabled shall update the phy zone information fields based on:
a) the REQUESTED INSIDE ZPSDS bit and the INSIDE ZPSDS PERSISTENT bit in the zone phy information (i.e.,
the bits transmitted in the outgoing IDENTIFY address frame); and
b) the REQUESTED INSIDE ZPSDS bit and INSIDE ZPSDS PERSISTENT bit received in the incoming IDENTIFY
address frame.

		Phy zone information fields after by a link reset seqeunce
REQUESTED						INSIDE ZPSDS		Phy zone information field changes
INSIDE ZPSDS bit				PERSISTENT bit

Transmitted Received 			Transmitted Received
0 			0 or 1				0 or 1 		0 or 1 	The zoning expander device shall set the INSIDE
	 													ZPSDS bit to zero.

1 			0					0 or 1 		0 or 1	The zoning expander device shall set the INSIDE
														ZPSDS bit to zero.

1 			1 					0			0		If the SAS address received in the IDENTIFY
													address frame during the identification
													sequence is different from the SAS address
													prior to the completion of the link reset
1			1					0			1		sequence, then the zoning expander device
													shall set:
													a) the REQUESTED INSIDE ZPSDS bit to zero; and
1			1					1			0		b) the INSIDE ZPSDS bit to zero.
													If the SAS address received in the IDENTIFY
													address frame during the identification
													sequence is the same as the SAS address prior
													to the completion of the link reset sequence,
													then the zoning expander device shall set:
													a) the INSIDE ZPSDS bit to one;
													b) the ZONE GROUP field to one; and
													c) the ZONE ADDRESS RESOLVED bit to zero.

1			1					1			1		The zoning expander device shall set:
													a) the INSIDE ZPSDS bit to one;
													b) the ZONE GROUP field to one; and
													c) the ZONE ADDRESS RESOLVED bit to zero.


If the ZONE GROUP PERSISTENT bit is set to one, then a link reset sequence shall not cause the zone
group value of an expander phy to change unless the INSIDE ZPSDS bit changes from zero to one.
If the ZONE GROUP PERSISTENT bit is set to zero, then table 29 specifies events based on the initial
condition of an expander phy that shall cause a zoning expander device with zoning enabled to change the
ZONE GROUP field of the expander phy to its default value (e.g., zero).

	Events that cause the ZONE GROUP field to be set to its default value when the ZONE GROUP
	PERSISTENT bit set to zero
Initial condition 			Event after the initial condition is established
Completed link reset		A subsequent link reset sequence completes and:
sequence with a				a) the SAS address received in the IDENTIFY address frame during
SAS device attached			the identification sequence is different from the SAS address prior to the
					completion of the link reset sequence; or
					b) a SATA device is attached.

Completed link reset		Either:
sequence with a			a) A subsequent link reset sequence completes and:
SATA device			A) a hot-plug timeout occurred between the time of the initial
attached				condition and the time the link reset sequence completed;
				B) the zoning expander device has detected the possibility that a new SATA
				device has been inserted. The method of detection is outside the scope of
				this standard (e.g., an enclosure services process reports a change in the
				ELEMENT STATUS CODE field in the Device or Array Device element (see
				SES-2), or a change in the WORLD WIDE NAME field in the attached SATA
				device¡¯s IDENTIFY DEVICE or IDENTIFY PACKET DEVICE data (see
				ATA8-ACS)); or
				C) a SAS phy or expander phy is attached;
				or
				b) The expander phy is disabled with the SMP PHY CONTROL function
				DISABLE phy operation.

The BPP determines the source zone group(s) of the Broadcast as follows:
a) if the BPP receives a Broadcast Event Notify message from an expander phy (i.e., a zoning expander
phy received a BROADCAST), the Broadcast has a single source zone group set to the zone group of
that expander phy; and
b) if the BPP receives a message from the management device server indicating that it received an SMP
ZONED BROADCAST request from an SMP initiator port that has access to zone group 3, the
Broadcast has each of the source zone groups specified in the SMP ZONED BROADCAST request.
The BPP forwards the Broadcast to each expander port other than the one on which the Broadcast was
received (i.e., the expander port that received the BROADCAST or SMP ZONED BROADCAST request) if
any of the source zone groups have access to the zone group of the expander port.
To forward a Broadcast to an expander port:
a) if the expander port¡¯s INSIDE ZPSDS bit is set to one, the BPP shall request that the SMP initiator port
establish a connection on at least one phy in the expander port to the SMP target port of the attached
expander device and transmit an SMP ZONED BROADCAST request specifying the source zone
group(s); and
b) if the expander port¡¯s INSIDE ZPSDS bit is set to zero, the BPP shall send a Transmit Broadcast
message to at least one phy in the expander port, causing it to transmit a BROADCAST.


	byte 60-bit0: ZONING ENABLED
	Set to one indicates that zoning is enabled in the expander device.
	A ZONING ENABLED bit set to zero indicates that zoning is disabled
	in the expander device.

	byte 60-bit1: INSIDE ZPSDS
	Indicates if the phy is inside or on the boundary of a ZPSDS.
	0: The phy is attached to an end device, an expander device that does not support
	   zoning, or a zoning expander device with zoning disabled.
	1: The phy is attached a zoning expander device with zoning enabled and is thus
	   inside a ZPSDS.

	The bit is not directly changeable and only changes following a link reset sequence,
	based on the REQUESTED INSIDE ZPSDS bit.

	byte 60-bit2: ZONE GROUP PERSISTENT
	Specifies the method of determining the zone group value of the phy after a link
	reset sequence when the INSIDE ZPSDS is set to zero.

	byte 60-bit3: ZONE ADDRESS RESOLVED
	Specifies the method used to determine the source code group for a connection request
	received by a phy at the boundary of the ZPSDS.
	The bit may be set to one when:
	 a) the phy is contained within a zoning expander device; and
	 b) the INSIDE ZPSDS bit for the phy is set to zero.
	The bit may be set to zero when:
	 a) the phy is contained within a non-zoning expander device; or
	 b) the phy is contained within a zoning expander device and the INSIDE ZPSDS for the
	    phy is set to one.


	byte 60-bit4: REQUESTED INSIDE ZPSDS
	Used to establish the boundary of the ZPSDS and to determine the value of other zone
	phy information fields after a link reset sequence.

	byte 60-bit5: INSIDE ZPSDS PERSISTENT
	Indicates the method used to determine the value of the INSIDE ZPSDS bit after a
	link reset sequence.

	byte 60-bit6: REQUESTED INSIDE ZPSDS CHANGED BY EXPANDER
	Set to one indicates that the zoning expander device set the REQUESTED INSIDE
	ZPSDS bit to zero in the zone phy information at the completion of the last
	link reset sequence. A REQUESTED INSIDE ZPSDS CHANGED BY EXPANDER bit set to
	zero indicates that the zoning expander device did not set the REQUESTED INSIDE
	ZPSDS bit to zero in the zone phy information at the completion of the last link
	reset sequence.
	The zone manager may use the REQUESTED INSIDE ZPSDS CHANGED BY EXPANDER bit to
	determine why the REQUESTED INSIDE ZPSDS bit has changed in the DISCOVER
	response from the value to which it last set the bit.


	All phys in an expander port shall have the same zone phy information. The zone phy
	information fields should be no-volatile and shall be preserved while zone is
	disabled.
*/
	MV_U8 ZoningEnabled:1;			/* bit 0 */
	MV_U8 InsideZPSDS:1;				/* bit 1 */
	MV_U8 ZoneGroupPersistent:1;		/* bit 2 */
	MV_U8 ZoneAddressResolved:1;		/* bit 3 */
	MV_U8 RequestedInsideZPSDS:1;	/* bit 4 */
	MV_U8 InsideZPSDSPersistent:1;	/* bit 5 */
	MV_U8 RequestedInsideZPSDSChangedbyExpander:1;	/* bit 6 */
	MV_U8 ReservedByte60Bit7:1;

/*	byte 61-62: Reserved */
	MV_U8 ReservedByte61_62[2];

/*
	byte 63: ZONE GROUP
	Contains a value in the range of 0 to 127 that specifies the zone group to which the phy
	blongs. The zone group of the SMP initiator port and SMP target port in a zoning expander
	device shall be 1.

*/
	MV_U8 ZoneGroup;

/*
	byte 64:SELF-CONFIGURATION STATUS
	Indicates the status of a self-configuring expander device pertaining to
	the specified phy.

	Code 	Description
	00h 	Reserved
	01h 	Error not related to a specific layer
	02h 	The expander device currently has a connection or is currently attempting to establish a
			connection with the SMP target port with the indicated SAS address.
	03h 	Expander route table is full. The expander device was not able to add the indicated SAS
			address to the expander route table.
	04h		Expander device is out of resources (e.g., it discovered too many SAS addresses while
			performing the discover process through a subtractive port). This does not affect the expander
			route table.
	05h-1Fh Reserved for status not related to specific layers

	Status reported by the phy layer:
	20h 	Error reported by the phy layer
	21h 	All phys in the expander port containing the indicated phy lost dword synchronication
	22h-3Fh Reserved for status reported by the phy layer

*/
	MV_U8 SelfConfigurationStatus;

/*
	byte 65: SELF-CONFIGURATION LEVELS COMPLETED
	Indicates the number of levels of expander devices beyond the expander port
	containing the specified phy for which the self-configuring expander device¡¯s
	management application client has completed the discover process.

	Code 	Description
	00h		The management application client:
			a) has not begun the discover process through the expander port containing the specified
			phy; or
			b) has not completed the discover process through the expander port containing the
			specified phy.
	01h 	The management application client has completed discovery of the expander device
			attached to the expander port containing the specified phy (i.e., level 1).
	02h		The management application client has completed discovery of the expander devices
			attached to the expander device attached to the expander port containing the specified phy
			(i.e., level 2).
	... ...
	FFh 	The management application client has completed discovery of the expander devices
			attached at level 255.
*/
	MV_U8 SelfConfgurationLevelsCompleted;

/*	byte 66-67: Reserved */
	MV_U8 ReservedByte66_67[2];

/*
	byte 68-75: SELF-CONFIGURATION SAS ADDRESS
	Indicates the SAS address of the SMP target port to which the self-configuring
	expander device established a connection or attempted to establish a connection
	using the specified phy and resulted in the status indicated by the
	SELF-CONFIGURATION STATUS field.
*/
	MV_U8 SelfConfigurationSASAddress[8];
}; /* DiscoverLongFormatDescriptor */



/*
 * Response specific bytes for SMP Discover, intended to be referenced
 * by SMPResponse
 */
struct SMPResponseDiscover
{
/*
	byte 4-5:
	EXPANDER CHANGE COUNT
	Counts the number of Broadcast (Change)s originated by an expander device.
	Management device servers in expander devices shall support this field.
	Management device servers in other device types (e.g., end devices) shall
	set this field to 0000h. This field shall be set to at least 0001h at power
	on. If the expander device has originated Broadcast (Change) for any reason
	described in 7.11 since transmitting a REPORT GENERAL response, it shall increment
	this field at least once from the value in the previous REPORT GENERAL response.
	It shall not increment this field when forwarding a Broadcast (Change). This field
	shall wrap to at least 0001h after the maximum value (i.e., FFFFh) has been reached.
*/
	MV_U8 ExpanderChangeCount[2];

/*	byte 6-8: Reserved	*/
	MV_U8 ReservedByte6_8[3];

/*
	byte 9: PHY IDENTIFIER
	Indicates the phy for which physical configuration link information
	is being returned.
*/
	MV_U8 PhyIdentifier;

/*	byte 10-11: Reserved	*/
	MV_U8 ReservedByte10_11[2];

/*
	byte 12-bit4-6:ATTACHED DEVICE TYPE
	001b: No device
	010b: End device
	010b: Expander device
	011b: Expander device compliant with a previous version of this standard
	Others: Reserved
*/
	MV_U8 ReservedByte12Bit0_3:4;	/* Bit0-3 */
	MV_U8 AttachedDeviceType:3;		/* Bit4-6 */
	MV_U8 IgnoredByte12Bit7:1;		/* Bit7 */
/*
	byte 13-bit0-3:	NEGOTIATED PHYSICAL LINK RATE
	0x0: UNKNOW, Phy is enabled; unknown physical link rate.
	0x1: DISABLED, Phy is disabled.
	0x2: PHY_RESET_PROBLEM
	0x3: SPINUP_HOLD, detected a SATA device and entered the SATA spinup hold state.
	0x4: PORT_SELECTOR
	0x5: RESET_IN_PROGRESS
	0x8: G1, Phy is enabled; 1,5 Gbps physical link rate.
	0x9: G2, Phy is enabled; 3,0 Gbps physical link rate.

*/
	MV_U8 NegotiatedPhysicalLinkRate:4;
	MV_U8 ReservedByte13Bit4_7:4;

/*
	byte 14-bit0:ATTACHED SATA HOST
	Set to one indicates a SATA host port is attached. An ATTACHED SATA HOST bit set
	to zero indicates a SATA host port is not attached.

	byte 14-bit1:ATTACHED SMP INITIATOR
	Set to one specifies that an SMP initiator port is present.


	byte 14-bit2:ATTACHED STP INITIATOR
	Set to one specifies that an STP initiator port is present.


	byte 14-bit3:ATTACHED SSP INITIATOR
	Set to one specifies that an SSP initiator port is present.



*/
	MV_U8 InitiatorBits;
	MV_U8 TargetBits;
/*
	byte 16-23:SAS ADDRESS
	Contains the value of the SAS ADDRESS field transmitted in the IDENTIFY address
	frame during the identification sequence. If the phy is an expander phy, the
	SAS ADDRESS field contains the SAS address of the expander device. If the phy
	is a SAS phy, the SAS ADDRESS field contains the SAS address of the SAS port.
*/
	MV_U8 SASAddress[8];


/*
	byte 24-31:ATTACHED SAS ADDRESS
	Contains the value of the SAS ADDRESS field received in the IDENTIFY
	address frame during the identification sequence. If the attached port
	is an expander port, the ATTACHED SAS ADDRESS field contains the SAS address
	of the attached expander device. If the attached port is a SAS port, the
	ATTACHED SAS ADDRESS field contains SAS address of the attached SAS port.
	If the attached port is a SATA device port, the ATTACHED SAS ADDRESS field
	contains the SAS address of the STP/SATA bridge.
	The ATTACHED SAS ADDRESS field shall be updated:
		a) after the identification sequence completes, if a SAS phy or expander
	phy is attached; or
		b) after the COMSAS Detect Timeout timer expires (see 6.8.3.9), if a SATA
	phy is attached.
	An STP initiator port should not make a connection request to the attached SAS address until the ATTACHED
	DEVICE TYPE field is set to a value other than 000b.
*/
	MV_U8 AttachedSASAddress[8];

/*
	byte 32: ATTACHED PHY IDENTIFIER
	Contains a phy identifier for the attached phy:
	a) If the attached phy is a SAS phy or an expander phy, the ATTACHED PHY
	IDENTIFIER field contains the value of the PHY IDENTIFIER field received
	in the IDENTIFY address frame during the identification sequence:
		A) If the attached phy is a SAS phy, the ATTACHED PHY IDENTIFIER field
		contains the phy identifier of the attached SAS phy in the attached SAS device;
		B) If the attached phy is an expander phy, the ATTACHED PHY IDENTIFIER
		field contains the phy identifier of the attached expander phy in the attached
		expander device;
	b) If the attached phy is a SATA device phy, the ATTACHED PHY IDENTIFIER field
	contains 00h;
	c) If the attached phy is a SATA port selector phy and the expander device is
	able to determine the port of the SATA port selector to which it is attached,
	the ATTACHED PHY IDENTIFIER field contains 00h or 01h; and
	d) If the attached phy is a SATA port selector phy and the expander device is not
	able to determine the port of the SATA port selector to which it is attached,
	the ATTACHED PHY IDENTIFIER field contains 00h.

	The ATTACHED PHY IDENTIFIER field shall be updated:
		a) after the identification sequence completes, if a SAS phy or expander phy is attached; or
		b) after the COMSAS Detect Timeout timer expires, if a SATA phy is attached.
*/

	MV_U8 AttachedPhyIdentifier;


/*
	byte 32-bit0:ATTACHED BREAK_REPLY CAPABLE
	Set to one indicate that the phy is capable of responding to received BREAK primitive
	sequences with a BREAK_REPLY primitive sequence.

	byte 32-bit1: ATTACHED REQUESTED INSIDE ZPSDS
	Is used to establish the boundary of ZPSDS. The bit is transmitted in the IDENTIFY
	address frame to the attached phy and is used to determine the value of other zone
	phy information fields after a link reset sequence.

	byte 32-bit2: ATTACHED INSIDE ZPSDS PERSISTENT
	Indicate the method used to determine the value of the INSIZE ZPSDS bit after a link
	reset sequence. The bit is transmitted in the IDENTIFY address frame.

*/
#ifdef __MV_BIG_ENDIAN_BITFIELD__
	MV_U8 ReservedByte32Bit3_7          : 5;
	MV_U8 AttachedInsideZPSDSPersistent : 1;  /* Bit 2 */
	MV_U8 AttachedRequestedInsideZPSDS  : 1;  /* Bit 1 */
	MV_U8 AttachedBreakReplyCapable     : 1;  /* Bit 0 */
#else
	MV_U8 AttachedBreakReplyCapable     : 1;  /* Bit 0 */
	MV_U8 AttachedRequestedInsideZPSDS  : 1;  /* Bit 1 */
	MV_U8 AttachedInsideZPSDSPersistent : 1;  /* Bit 2 */
	MV_U8 ReservedByte32Bit3_7          : 5;
#endif /* __MV_BIG_ENDIAN_BITFIELD__ */

/*	byte 34-39: Reserved */
	MV_U8 ReservedByte34_39[6];

/*
	byte 40-bit0-3:HARDWARE MINIMUM PHYSICAL LINK RATE
	Indicates the minimum physical link rate supported by the phy.
	0x8: 1.5Gbps
	0x9: 3.0Gbps
	Others: Reserved

	byte 40-bit4-7:PROGRAMMED MINIMUM PHYSICAL LINK RATE
	Indicates the minimum physical link rate set by the PHY CONTROL function.
	The default value shall be the value of the HARDWARE MINIMUM PHYSICAL LINK RATE
	field.

	0x0: Not programmable
	0x8: 1.5Gbps
	0x9: 3.0Gbps
	Others: Reserved
*/

	MV_U8 HardwareMinimumPhysicalLinkRate:4;
	MV_U8 ProgrammedMinimumPhysicalLinkRate:4;

/*
	byte 41-bit0-3:HARDWARE MAXIMUM PHYSICAL LINK RATE
	Indicates the maximum physical link rate supported by the phy.
	0x8: 1.5Gbps
	0x9: 3.0Gbps
	Others: Reserved

	byte 41-bit4-7:PROGRAMMED MAXIMUM PHYSICAL LINK RATE
	Indicates the maximum physical link rate set by the PHY CONTROL function.
	The default value shall be the value of the HARDWARE MINIMUM PHYSICAL LINK RATE
	field.

	0x0: Not programmable
	0x8: 1.5Gbps
	0x9: 3.0Gbps
	Others: Reserved
*/
	MV_U8 HardwareMaximumPhysicalLinkRate:4;
	MV_U8 ProgrammedMaximumPhysicalLinkRate:4;

/*
	byte 42: PHY CHANGE COUNT
	Counts the number of Broadcast(Change)s originated by an expander phy.
	Expander devices shall support this field. Other device types shall not support
	this field. This field shall be set to zero at power on. The expander device shall
	increment this field at least once when it originates a Broadcast (Change) for
	any reason from the specified expander phy and shall not increment this field when
	forwarding a Broadcast (Change).
	After incrementing the PHY CHANGE COUNT field, the expander device is not
	required to increment the PHY CHANGE COUNT field again unless a DISCOVER response
	is transmitted. The PHY CHANGE COUNT field shall wrap to zero after the maximum
	value (i.e., FFh) has been reached.
	Application clients that use the PHY CHANGE COUNT field should read it often
	enough to ensure that it does not increment a multiple of 256 times between
	reading the field.
*/
	MV_U8 PhyChangeCount;


/*
	byte 43-bit0-3:PARTIAL PATHWAY TIMEOUT VALUE
	Indicates the partial pathway timeout value in microseconds set by the PHY
	CONTROL function. The recommended default value for PARTIAL PATHWAY TIMEOUT
	VALUE is 7 ¦Ìs.

	byte 43-bit7: VIRTUAL PHY
	Set to one indicates the phy is part of an internal port and the attached device
	is contained within the expander device. A VIRTUAL PHY bit set to zero indicates
	the phy is a physical phy and the attached device is not contained within the
	expander device.
*/
	MV_U8 PartialPathwayTimeoutValue:4;
	MV_U8 IgnoredByte36Bit4_6:3;
	MV_U8 VirtualPhy:1;

/*
	byte 44-bit0-3: ROUTING ATTRIBUTE
	Indicates the routing attribute supported by the phy.

	Code 	Name 			Description
	0h 		Direct 			Direct routing method for attached end devices.
							Attached expander devices are not supported on this phy.

	1h 		Subtractive		a) subtractive routing method for attached expander.
							b) direct routing method for attached end devices.

	2h 		Table			a) table routing method for attached expander devices;
							b) direct routing method for attached end devices.

	The ROUTING ATTRIBUTE field shall not change based on the attached device type.
*/

	MV_U8 RoutingAttribute:4;
	MV_U8 ReservedByte44Bit4_7:4;

/*
	byte 45-bit0-6:CONNECTOR TYPE
	Indicates the type of connector used to access the phy, as reported by the
	enclosure services process for the enclosure. A CONNECTOR TYPE field set to 00h
	indicates no connector information is available and that the CONNECTOR ELEMENT
	INDEX field and the CONNECTOR PHYSICAL LINK fields are invalid and shall be
	ignored.
*/
	MV_U8 ConnectorType:7;
	MV_U8 ReservedByte45Bit7:1;

/*
	byte 46:CONNECTOR ELEMENT INDEX
	Indicates the element index of the SAS Connector element representing the
	connector used to access the phy, as reported by the enclosure services
	process for the enclosure.
*/
	MV_U8 ConnectorElementIndex;

/*
	byte 47: CONNECTOR PHYSICAL LINK
	Indicates the physical link in the connector used to access the phy, as
	reported by the enclosure services process for the enclosure.
*/
	MV_U8 ConnectorPhysicalLink;

	struct DiscoverLongFormatDescriptor	LongFormatDescriptor;


/*
	byte 76-77: CRC
*/
	MV_U32 CRC;

}; /* struct SMPResponseDiscover */

/*
	request specific bytes for SMP Report PHY SATA response,
	intended to be referenced by SMPResponse
*/
struct SMPResponseReportPhySATA
{
	/* byte 4-5 */
	MV_U8 ExpanderChangeCount[2];

	/* byte 6-8 */
	MV_U8 ReservedByte6_8[3];

	/* byte 9 */
	MV_U8 PhyIdentifier;

	/* byte 10 */
	MV_U8 ReservedByte10;

	/* byte 11 */
	MV_U8 AffiliationValid:1;
	MV_U8 AffiliationSupported:1;
	MV_U8 STPNexusLossOccurred:1;
	MV_U8 ReservedByte11Bit3_7:5;

	/* byte 12-15 */
	MV_U8 ReservedByte10_15[4];

	/* byte 16-23 */
	MV_U8 STPSASAddress[8];

	/* 24-43 */
	sata_fis_reg_d2h fis;

	/* byte 44-47 */
	MV_U8 ReservedByte44_47[4];

	/* byte 48-55 */
	MV_U8 AffiliatedSTPInitiatorSASAddress[8];

	/* byte 56-63 */
	MV_U8 STPNexusLossSASAddress[8];

	/* byte 64-67 */
	MV_U32 CRC;
}; /* struct SMPResponseReportPhySATA */


/*
	response specific bytes for SMP Report Route Information, intended to be
	referenced by SMPResponse
*/
struct SMPResponseReportRouteInformation
{
	/* byte 4-5 */
	MV_U8 ExpanderChangeCount[2];
	/* byte 6-7 */
	MV_U8 ExpanderRouteIndex[2];
	/* byte 8 */
	MV_U8 ReservedByte8;
	/* byte 9 */
	MV_U8 PhyIdentifier;
	/* byte 10-11 */
	MV_U8 ReservedByte10_11[2];
	/* byte 12 */
	MV_U8 IgnoredByte12Bit0_6:7;
	MV_U8 ExpanderRouteEntryDisabled:1;
	/* byte 13-15 */
	MV_U8 IgnoredByte13_15[3];
	/* byte 16-23 */
	MV_U8 RoutedSASAddress[8];
	/* byte 24-39 */
	MV_U8 ReservedByte24_39[16];
	/* byte 40-43 */
	MV_U32 CRC;
}; /* struct SMPResponseReportRouteInformation */

/*
	response specific bytes for SMP Report Phy Event Information, intended to be
	referenced by SMPResponse
*/
struct SMPResponseReportPhyEventInformation
{
	/* byte 4-5 */
	MV_U8 ExpanderChangeCount[2];

	/* byte 6-8 */
	MV_U8 ReservedByte6_8[3];
	/* byte 9 */
	MV_U8 PhyIdentifier;
	/* byte 10-14 */
	MV_U8 ReservedByte10_14[5];

	/* byte 15 */
	MV_U8 NumberOfPhyEventDescriptors;

	/* byte 16-27 */
	MV_U8 PhyEventDescriptor[12];
/*
	byte PhyEventDescriptor[12];
		...
	byte PhyEventDescriptor[12];

*/
	MV_U32 CRC;
}; /* struct SMPResponseReportPhyEventInformation */


/*
	response specific bytes for SMP Report Phy Broadcast Counts, intended to be
	referenced by SMPResponse
*/
struct SMPResponseReportBroadcastCounts
{
	/* byte 4-5 */
	MV_U8 ExpanderChangeCount[2];

	/* byte 6-8 */
	MV_U8 ReservedByte6_8[3];
	/* byte 9 */
	MV_U8 PhyIdentifier;
	/* byte 10-11 */
	MV_U8 ReservedByte10_11[2];

	/* byte 12 */
	MV_U8 ChangeCountValid:1;
	MV_U8 ReservedChange0CountValid:1;
	MV_U8 ReservedChange1CountValid:1;
	MV_U8 SESCountValid:1;
	MV_U8 ExpanderCountValid:1;
	MV_U8 AsysnchronousEventCountValid:1;
	MV_U8 Reserved3CountValid:1;
	MV_U8 Reserved4CountValid:1;

	/* byte 13-15 */
	MV_U8 ReservedByte13_15[3];

	/* byte 16 */
	MV_U8 BroadcastChangeCount;

	/* byte 17 */
	MV_U8 BroadcastReserved0ChangeCount;

	/* byte 18 */
	MV_U8 BroadcastReserved1ChangeCount;

	/* byte 19 */
	MV_U8 BroadcastSESCount;

	/* byte 20 */
	MV_U8 BroadcastExpanderCount;

	/* byte 21 */
	MV_U8 BroadcastAsynchronousEventCount;

	/* byte 22 */
	MV_U8 BroadcastReserved3ChangeCount;

	/* byte 23 */
	MV_U8 BroadcastReserved4ChangeCount;

	MV_U32 CRC;
}; /* struct SMPResponseReportBroadcastCounts */


/*
	response specific bytes for SMP Discover List Descriptor, intended to be
	referenced by SMPResponseDiscoverList
*/
struct DiscoverShortFormatDescriptor
{
	/* byte 48 */
	MV_U8 PhyIdentifier;

	/* byte 49 */
	MV_U8 FunctionResult;

	/* byte 50 */
	MV_U8 ReservedByte50Bit0_3:4;
	MV_U8 AttachedDeviceType:3;
	MV_U8 ReservedByte50Bit7:1;

	/* byte 51 */
	MV_U8 NegotiatedPhysicalLinkRate:4;
	MV_U8 ReservedByte51Bit4_7:4;

	/* byte 52 */
	MV_U8 AttachedSataHost:1;
	MV_U8 AttachedSMPInitiator:1;
	MV_U8 AttachedSTPInitiator:1;
	MV_U8 AttachedSSPInitiator:1;
	MV_U8 ReservedByte52Bit4_7:4;

	/* byte 53 */
	MV_U8 AttachedSataDevice:1;
	MV_U8 AttachedSMPTarget:1;
	MV_U8 AttachedSTPTarget:1;
	MV_U8 AttachedSSPTarget:1;
	MV_U8 ReservedByte53Bit4_6:3;
	MV_U8 AttachedSataPortSelector:1;

	/* byte 54 */
	MV_U8 RoutingAttribute:4;
	MV_U8 ReservedByte54Bit4_6:3;
	MV_U8 VirtualPhy:1;

	/* byte 55 */
	MV_U8 ReservedByte55;

	/* byte 56 */
	MV_U8 ZoneGroup;

	/* byte 57 */
	MV_U8 ReservedByte57Bit0:1;
	MV_U8 InsideZPSDS:1;
	MV_U8 ZoneGroupPersistent:1;
	MV_U8 ZoneAddressResolved:1;
	MV_U8 RequestedInsideZPSDS:1;
	MV_U8 InsideZPSDSPersistent:1;
	MV_U8 ReservedByte57Bit6_7:2;

	/* byte 58 */
	MV_U8 AttachedPhyIdentifier;

	/* byte 59 */
	MV_U8 PhyChangeCount;

	/* byte 60-68 */
	MV_U8 AttachedSASAddress[8];

	/* byte 69-72 */
	MV_U8 ReservedByte68_72[4];


}; /* DiscoverShortFormatDescriptor */

union DiscoverListDescriptor
{
	struct DiscoverShortFormatDescriptor	ShortFormatDescriptor;
	struct DiscoverLongFormatDescriptor		LongFormatDescriptor;

};

/*
	response specific bytes for SMP Discover List, intended to be
	referenced by SMPResponse
*/
struct SMPResponseDiscoverList
{
	/* byte 4-5 */
	MV_U8 ExpanderChangeCount[2];

	/* byte 6-7 */
	MV_U8 ReservedByte6_8[2];
	/* byte 8 */
	MV_U8 StartingPhyIdentifier;

	/* byte 9 */
	MV_U8 NumberOfDescriptors;
	/* byte 10 */
	MV_U8 PhyFilter:4;
	MV_U8 ReservedByte10Bit4_7:4;
	/* byte 11 */
	MV_U8 DescriptorType:4;
	MV_U8 ReservedByte11Bit4_7:4;
	/* byte 12 */
	MV_U8 DescriptorLength;

	/* byte 13-15 */
	MV_U8 ReservedByte13_15[3];

	/* byte 16 */
	MV_U8 ConfigurableRouteTable:1;
	MV_U8 Congiguring:1;
	MV_U8 ReservedByte16Bit2_5:4;
	MV_U8 ZoningEnabled:1;
	MV_U8 ZoningSupported:1;

	/* byte 17-31 */
	MV_U8 ReservedByte17_31[15];

	/* byte 32-47 */
	MV_U8 VendorSpecific[16];

	/* byte 48 - n */
	union DiscoverListDescriptor 	FirstDescriptor;

/*
	DiscoverListDescriptor 	SecondDescriptor;
		...
	DiscoverListDescriptor 	LastDescriptor;

*/
	MV_U32 CRC;
}; /* struct SMPResponseDiscoverList */


/*
	response specific bytes for Expander Route Table Descriptor, intended to be
	referenced by SMPResponseReportExpanderTable
*/
struct ExpanderRouteTableDescriptor
{
	/* byte 32-37 */
	MV_U8 PhyBitMap[6];

	/* byte 38 */
	MV_U8 ReservedByte38Bit0_6:7;
	MV_U8 ZoneGroupValid:1;

	/* byte 39 */
	MV_U8 ZoneGroup;

	/* byte 40-48 */
	MV_U8 RoutedSASAddress[8];

};/* ExpanderRouteTableDescriptor */

/*
	response specific bytes for SMP Report Expander Route Table, intended to be
	referenced by SMPResponse
*/
struct SMPResponseReportExpanderTable
{
	/* byte 4-5 */
	MV_U8 ExpanderChangeCount[2];

	/* byte 6-7 */
	MV_U8 ExpanderRouteTableChangeCount[2];
	/* byte 8 */
	MV_U8 ReservedByte8Bit0:1;
	MV_U8 Configuring:1;
	MV_U8 ReservedByte8Bit2_7:6;


	/* byte 9 */
	MV_U8 ReservedByte9;

	/* byte 10-11 */
	MV_U8 NumberOfDescriptors[2];

	/* byte 12-13 */
	MV_U8 FirstRoutedSASAddressIndex[2];

	/* byte 14-15 */
	MV_U8 LastRoutedSASAddressIndex[2];

	/* byte 13-15 */
	MV_U8 ReservedByte13_15[3];

	/* byte 16-18 */
	MV_U8 ReservedByte16_18[3];

	/* byte 19 */
	MV_U8 StartingPhyIdentifier;

	/* byte 20-31 */
	MV_U8 ReservedByte20_31[12];

	/* byte 32 - n */
	struct ExpanderRouteTableDescriptor 	FirstRouteTableDescriptor;

/*
	ExpanderRouteTableDescriptor 	SecondRouteTableDescriptor;
		...
	ExpanderRouteTableDescriptor 	LastRouteTableDescriptor;

*/
	MV_U32 CRC;
}; /* struct SMPResponseReportExpanderTable */

struct SMPResponseConfigureFunction
{
	/* byte 4-7 */
	MV_U32 CRC;
};

/*
	response specific to SMP Read SGPIO
	Corresponds to READ_GPIO_REGISTER define
*/
struct SMPResponseReadGPIORegister
{
	/* byte 4-11 */
	MV_U8 Data_In_Low[8];

	/* byte 12-19 */
	MV_U8 Data_In_High[8];

	MV_U32 CRC;
};

/*
	response specific to SMP Write SGPIO
	Corresponds to WRITE_GPIO_REGISTER define
*/
struct SMPResponseWriteGPIORegister
{
	MV_U32 CRC;
};

/*
	generic structure referencing an SMP Response, must be initialized
	before being used
*/
typedef struct _smp_response
{
	/* byte 0 */
	MV_U8 smp_frame_type; /* always 41h for SMP responses */
	/* byte 1 */
	MV_U8 function;
	/* byte 2 */
	MV_U8 function_result;
	/* byte 3 */
	MV_U8 reservedbyte3;
	/* bytes 4-n */
	union
	{
/* #define REPORT_GENERAL 						0x00 */
		struct SMPResponseReportGeneral ReportGeneral;

/* #define REPORT_MANUFACTURER_INFORMATION 		0x01 */
		struct SMPResponseReportManufacturerInformation ReportManufacturerInformation;

/* #define READ_GPIO_REGISTER					0x02 */
		struct SMPResponseReadGPIORegister					ReadGPIORegister;

/* #define REPORT_SELF_CONFIGURATION_STATUS	0x03 */
		struct SMPResponseReportSelfConfigurationStatus	ReportSelfConfigurationStatus;

/* #define DISCOVER 							0x10 */
		struct SMPResponseDiscover 						Discover;

/* #define REPORT_PHY_ERROR_LOG 				0x11 */
		struct SMPResponseReportPhyErrorLog 			ReportPhyErrorLog;

/* #define REPORT_PHY_SATA 						0x12 */
		struct SMPResponseReportPhySATA 				ReportPhySATA;

/* #define REPORT_ROUTE_INFORMATION 			0x13 */
		struct SMPResponseReportRouteInformation 		ReportRouteInformation;

/* #define REPORT_PHY_EVENT_INFORMATION			0x14 */
		struct SMPResponseReportPhyEventInformation		ReportPhyEventInformation;

/* #define REPORT_PHY_BROADCAST_COUNTS			0x15 */
		struct SMPResponseReportBroadcastCounts			ReportBroadcastCounts;

/* #define DISCOVER_LIST						0x16 */
		struct SMPResponseDiscoverList					DiscoverList;

/* #define REPORT_EXPANDER_ROUTE_TABLE			0x17 */
		struct SMPResponseReportExpanderTable			ReportExpanderTable;

/* #define CONFIGURE_GENERAL					0x80 */
		struct SMPResponseConfigureFunction				ConfigureGeneral;

/* #define ENABLE_DISABLE_ZONING				0x81 */
		struct SMPResponseConfigureFunction				EnableDisableZoning;

/* #define WRITE_GPIO_REGISTER					0x82 */
		struct SMPResponseWriteGPIORegister			WriteGPIORegister;

/* #define ZONED_BROADCAST						0x85 */
		struct SMPResponseConfigureFunction				ZonedBroadcast;

/* #define CONFIGURE_ROUTE_INFORMATION 			0x90 */
		struct SMPResponseConfigureFunction				ConfigureRouteInformation;

/* #define PHY_CONTROL 							0x91 */
		struct SMPResponseConfigureFunction 			PhyControl;

/* #define PHY_TEST 							0x92 */
		struct SMPResponseConfigureFunction 			PhyTest;

/* #define CONFIGURE_PHY_EVENT_INFORMATION		0x93 */
		struct SMPResponseConfigureFunction 			ConfigurePhyEventInformation;

	} response;
} smp_response;

/*
	this structure is how this simulation obtains its knowledge about the
	initiator port that is doing the discover, it is not defined as part of
	the standard...
*/

struct ApplicationClientKnowledge
{
	MV_U64 SASAddress;
	MV_U8 NumberOfPhys;
	MV_U8 InitiatorBits;
	MV_U8 TargetBits;
};
	/*
	the RouteTableEntry structure is used to contain the internal copy of
	the expander route table
	*/
struct RouteTableEntry
{
	MV_U8 ExpanderRouteEntryDisabled;
	MV_U64 RoutedSASAddress;
};

	/*
	the TopologyTable structure is the summary of the information gathered
	during the discover process, the table presented here is not concerned
 	about memory resources consumed, production code would be more concerned
 	about specifying necessary elements explicitly.
	*/
struct TopologyTable
{
	/*
	 pointer to a simple list of expanders in topology
	a walk through this link will encounter all expanders in
	discover order.
	*/
	struct TopologyTable *Next;

	/*
		simple reference to this device, primarily to keep identification of
		this structure simple, otherwise, the only place the address is
		located is within the Phy element.
	*/
	MV_U64 SASAddress;

	/* information from REPORT_GENERAL	*/
	struct SMPResponseReportGeneral Device;
	/* information from DISCOVER */
	struct SMPResponseDiscover Phy[MAXIMUM_EXPANDER_PHYS];
	/* list of route indexes for each phy */
	MV_U16 RouteIndex[MAXIMUM_EXPANDER_PHYS];
	/* internal copy of the route table for the expander */
	struct RouteTableEntry	RouteTable[MAXIMUM_EXPANDER_PHYS][MAXIMUM_EXPANDER_INDEXES];
};

#endif /* __CORE_DISCOVER_H */
