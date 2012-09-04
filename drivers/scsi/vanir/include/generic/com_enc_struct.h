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
#ifndef __MV_COM_ENC_STRUCT_H__
#define __MV_COM_ENC_STRUCT_H__
#include "com_physical_link.h"

#define MAX_ENCLOSURE_API      32

#pragma pack(8)

// light types
#define SGPIO_LED_ACTIVITY                      0
#define SGPIO_LED_LOCATE                        1
#define SGPIO_LED_ERROR                         2
#define SGPIO_LED_REBUILD						3		/* for Supermicro backplanes */
#define SGPIO_LED_HOT_SPARE                     4
#define SGPIO_LED_REBUILD_MASK                  0x00000080
#define SGPIO_LED_ERROR_MASK                    0x00000040
#define SGPIO_LED_ALL                           0xff
// light status - the following values are set according to hardware
//       values, do not change unless necessary
#define SGPIO_LED_LOW                           0
#define SGPIO_LED_HIGH                          1
#define SGPIO_LED_BLINK_A                       2
#define SGPIO_LED_BLINK_INVRT_A                 3
#define SGPIO_LED_BLINK_SOF                     4
#define SGPIO_LED_BLINK_EOF                     5
#define SGPIO_LED_BLINK_B                       6
#define SGPIO_LED_BLINK_INVRT_B                 7
// pre-set flags for RAID module to use
#define LED_FLAG_REBUILD                      1
#define LED_FLAG_HOT_SPARE                    2
#define LED_FLAG_FAIL_DRIVE                   3
#define LED_FLAG_HOT_SPARE_OFF                0x12
#define LED_FLAG_OFF_ALL                      0xff
#define LED_FLAG_ACT					4

#define LED_FLAG_WORKING_CHECK                0x10
#define LED_FLAG_REBUILD_OFF                  0x11
#define LED_FLAG_FAIL_DRIVE_OFF               0x13


//add as enclosure

#define ENC_STATUS_NOT_EXIST        0
#define ENC_STATUS_OK               1
#define ENC_STATUS_UNSTABLE         2

typedef struct _Enclosure_Info
{
	Link_Entity     Link;
	MV_U8           AdapterID;
	MV_U8           Status;           // Refer to ENC_STATUS_XXX
	MV_U8           reserved1[2];
	MV_U8           LogicalID[8+1];
	MV_U8           VendorID[8+1];
	MV_U8           ProductID[16+1];
	MV_U8           RevisionLevel[4+1];
	MV_U8			reserved2[3];
	// If ExpanderCount > 0, the enclosure might have more than one parant, in this case
	// Link.Parent.DevType will be set to DEVICE_TYPE_NONE and all other Parent fields are meaningless.
	// To get all its parent info, use each of ExpanderIDs[] to find out its individual parent.
	MV_U8			ExpanderCount;
	MV_U16			ExpanderIDs[8];
	MV_U8           path;
	MV_U8           target;
	MV_U8           lun;
	MV_U8           slice;
	MV_U8           reserved3[20];
}Enclosure_Info, *PEnclosure_Info;

#define MAX_ENCELEMENT      512
#define ENC_COOLING_STOP 0

typedef struct _EncCooling_Element
{
	MV_U16     curSpeed;
	MV_U8      curSpeedcode;
	MV_U8      reserved[5];
} EncCooling_Element;

typedef struct _EncPower_Element
{
	MV_U8      DCOverVoltage:1;
	MV_U8      DCUnderVoltage:1;
	MV_U8      DCOverCurrent:1;
	MV_U8      hotSwap:1;
	MV_U8      tempWarn:1;
	MV_U8      reservedbit:3;
	MV_U8      reserved[7];
} EncPower_Element;

typedef struct _EncTemperatureSensor_Element
{
	MV_U8      temperature;
	MV_U8      OTFailure:1;
	MV_U8      OTWarning:1;
	MV_U8      UTFailure:1;
	MV_U8      UTWarning:1;
	MV_U8      reservedbit:4;
	MV_U8      reserved[6];
} EncTemperatureSensor_Element;

typedef struct _EncDoorLock_Element
{
	MV_BOOLEAN       unlocked;
	MV_U8            reserved[7];
} EncDoorLock_Element;

typedef struct _EncAudibleAlarm_Element
{
	MV_U8      mute:1;
	MV_U8      remind:1;
	MV_U8      reservedbit:6;
	MV_U8      reserved[7];
} EncAudibleAlarm_Element;

#define ENC_DISPLAY_NOCHANGE            0
#define ENC_DISPLAY_ENCCONTROL          1
#define ENC_DISPLAY_CLIENT              2

typedef struct _EncDisplay_Element
{
	MV_U8       character1;
	MV_U8       character2;
	MV_U8       mode;
	MV_U8       reserved[5];
} EncDisplay_Element;

typedef struct _EncVoltageSensor_Element
{
	MV_U16     voltage;
	MV_U8      warnOver:1;
	MV_U8      warnUnder:1;
	MV_U8      critOver:1;
	MV_U8      critUnder:1;
	MV_U8      reservedbit:4;
	MV_U8      reserved[5];
} EncVoltageSensor_Element;

typedef struct _EncDevice_Element
{
	MV_U8      SlotAddress;
	MV_U8      Report:1;
	MV_U8      Ident:1;
	MV_U8      Rmv:1;
	MV_U8      ReadyToInst:1;
	MV_U8      EncBypassedB:1;
	MV_U8      EncBypassedA:1;
	MV_U8      DoNotRemove:1;
	MV_U8      AppClientBypassedA:1;

	MV_U8      DeviceBypassedB:1;
	MV_U8      DeviceBypassedA:1;
	MV_U8      ByPassedB:1;
	MV_U8      ByPassedA:1;
	MV_U8      DeviceOff:1;
	MV_U8      FaultReqstd:1;
	MV_U8      FaultSensed:1;
	MV_U8      AppClientBypassedB:1;
	MV_U8      reserved[5];
} EncDevice_Element;

typedef struct _EncArrayDevice_Element
	{
	MV_U8      RrAbort:1;	         // Rebuild or Remap Abort
	MV_U8      RebuildRemap:1;		// Rebuild or Remap
	MV_U8      InFailedArray:1;
	MV_U8      InCritArray:1;
	MV_U8      ConsChk:1;
	MV_U8      HotSpare:1;
	MV_U8      RsvdDevice:1;
	MV_U8      Ok:1;

	MV_U8      Report:1;
	MV_U8      Ident:1;
	MV_U8      Rmv:1;
	MV_U8      ReadyToInst:1;
	MV_U8      EncBypassedB:1;
	MV_U8      EncBypassedA:1;
	MV_U8      DoNotRemove:1;
	MV_U8      AppClientBypassedA:1;

	MV_U8      DeviceBypassedB:1;
	MV_U8      DeviceBypassedA:1;
	MV_U8      ByPassedB:1;
	MV_U8      ByPassedA:1;
	MV_U8      DeviceOff:1;
	MV_U8      FaultReqstd:1;
	MV_U8      FaultSensed:1;
	MV_U8      AppClientBypassedB:1;

	MV_U8      reserved[5];
} EncArrayDevice_Element;

#define ENC_ELEMENTTYPE_UNKNOWN             0
#define ENC_ELEMENTTYPE_DEVICE              1
#define ENC_ELEMENTTYPE_POWERSUPPLY         2
#define ENC_ELEMENTTYPE_COOLING             3
#define ENC_ELEMENTTYPE_TEMPERATURE         4
#define ENC_ELEMENTTYPE_DOORLOCK            5
#define ENC_ELEMENTTYPE_AUDIBLEALARM        6
#define ENC_ELEMENTTYPE_ENCSERVICE          7
#define ENC_ELEMENTTYPE_SCCCONTROLLER       8
#define ENC_ELEMENTTYPE_NONVOLATILECACHE    9
#define ENC_ELEMENTTYPE_INVALIDOPERATION    10
#define ENC_ELEMENTTYPE_UNINTERRPOWER       11
#define ENC_ELEMENTTYPE_DISPLAY             12
#define ENC_ELEMENTTYPE_KEYPAD              13
#define ENC_ELEMENTTYPE_ENC                 14
#define ENC_ELEMENTTYPE_SCSIPORT            15
#define ENC_ELEMENTTYPE_LANGUAGE            16
#define ENC_ELEMENTTYPE_COMMUNICATIONPORT   17
#define ENC_ELEMENTTYPE_VOLTAGESENSOR       18
#define ENC_ELEMENTTYPE_CURRENTSENSOR       19
#define ENC_ELEMENTTYPE_SCSITARGETPORT      20
#define ENC_ELEMENTTYPE_SCSIINITIATORPORT   21
#define ENC_ELEMENTTYPE_SIMPLESUBENC        22
#define ENC_ELEMENTTYPE_ARRAYDEVICE         23
#define ENC_ELEMENTTYPE_SASEXP              24
#define ENC_ELEMENTTYPE_SASCONNECTOR        25

#define MV_SUPPORT_STATUS_ELEMENT_TYPE(type)	((type==ENC_ELEMENTTYPE_COOLING)||	\
						(type==ENC_ELEMENTTYPE_DEVICE) || \
						(type==ENC_ELEMENTTYPE_ARRAYDEVICE) || \
						(type==ENC_ELEMENTTYPE_POWERSUPPLY) || \
						(type==ENC_ELEMENTTYPE_TEMPERATURE) || \
						(type==ENC_ELEMENTTYPE_DOORLOCK) || \
						(type==ENC_ELEMENTTYPE_AUDIBLEALARM) || \
						(type==ENC_ELEMENTTYPE_VOLTAGESENSOR) || \
						(type==ENC_ELEMENTTYPE_ARRAYDEVICE) || \
						(type==ENC_ELEMENTTYPE_DISPLAY))

typedef struct _EncElementType_Info
{
	MV_U16                          enclosureID;
	MV_U16                          magicID;
	MV_U8                           type;
	MV_U8		                    status;
	MV_U8	                        elementID;
	MV_U8                           reserved;
	union {
		EncDevice_Element               device;
		EncArrayDevice_Element          arrayDevice;
		EncCooling_Element              fan;
		EncPower_Element                power;
		EncTemperatureSensor_Element    temperatureSensor;
		EncDoorLock_Element             doorLock;
		EncAudibleAlarm_Element         alarm;
		EncDisplay_Element              display;
		EncVoltageSensor_Element        voltageSensor;
		MV_U8                           unCode[8];
	};
} EncElementType_Info, *PEncElementType_Info;

#define MV_SUPPORT_CONTROL_ELEMENT_TYPE(type)	((type==ENC_ELEMENTTYPE_COOLING)||	\
									(type==ENC_ELEMENTTYPE_ARRAYDEVICE) || \
									(type==ENC_ELEMENTTYPE_DEVICE))

typedef struct _EncCooling_Control
{
	MV_U8   speedcode;
	MV_U8   reserved[7];
}EncCooling_Control,*PEncCooling_Control;

typedef struct _EncDevice_Control
{
	MV_U8   LedOn;
	MV_U8   reserved[7];
}EncDevice_Control,*PEncDevice_Control;

typedef struct _EncArrayDevice_Control
{
	MV_U8   LedOn;
	MV_U8   reserved[7];
}EncArrayDevice_Control,*PEncArrayDevice_Control;

typedef struct _EncElementType_Control
{
	MV_U16                          enclosureID;
	MV_U8                           type;
	MV_U16                          magicID;
	MV_U8                           control;
	MV_U8	                        elementID;
	MV_U8                           reserved;
	union {
		EncCooling_Control              fan;
		EncDevice_Control               device;
		EncArrayDevice_Control          arrayDevice;
		MV_U8                           unCode[8];
	};
} EncElementType_Control, *PEncElementType_Control;

typedef struct _DevElementStatusDescriptorEIP0
{
	MV_U8 config;
	MV_U8 length;
	MV_U8 infomantion[2];
} DevElementStatusDescriptorEIP0, *PDevElementStatusDescriptorEIP0;

typedef struct _DevElementStatusDescriptorEIP1
{
	MV_U8 config;
	MV_U8 length;
	MV_U8 reserved;
	MV_U8 elementIndex;
	MV_U8 infomantion[4];
} DevElementStatusDescriptorEIP1, *PDevElementStatusDescriptorEIP1;

typedef union
{
	DevElementStatusDescriptorEIP0 EIP0;
	DevElementStatusDescriptorEIP1 EIP1;
}DevElementStatusDescriptor, *PDevElementStatusDescriptor;

typedef struct _DevElementStatusPage
{
	MV_U8                      PageCode;
	MV_U8                      Reserved;
	MV_U16                     PageLength;
	MV_U8                      GenerationCode[4];
	DevElementStatusDescriptor DeviceElementStatusDescriptor;
	MV_U8                      Data[2032];
} DevElementStatusPage, *PDevElementStatusPage;

typedef struct _SasPhyDescriptor
{
	MV_U8 Reserved1:4;
	MV_U8 DeviceType:3;
	MV_U8 Reserved2:1;
	MV_U8 Reserved3;
	MV_U8 Reserved4:1;
	MV_U8 SMPInitiatorPort:1;
	MV_U8 STPInitiatorPort:1;
	MV_U8 SSPInitiatorPort:1;
	MV_U8 Reserved5:4;
	MV_U8 Reserved6:1;
	MV_U8 SMPTargetPort:1;
	MV_U8 STPTargetPort:1;
	MV_U8 SSPTargetPort:1;
	MV_U8 Reserved7:4;
	MV_U8 AttachedSASAddress[8];
	MV_U8 SASAddress[8];
	MV_U8 PhyIdentifier;
	MV_U8 Reserved8[7];
} SasPhyDescriptor, *PSasPhyDescriptor;

#define HIGH_CRITICAL_THRESHOLD MV_BIT(0)
#define HIGH_WARNING_THRESHOLD MV_BIT(1)
#define LOW_CRITICAL_THRESHOLD MV_BIT(2)
#define LOW_WARNING_THRESHOLD   MV_BIT(3)

#define MV_ELEMENTTHRESHOLD_TYPE(type)	((type==ENC_ELEMENTTYPE_TEMPERATURE)||	\
						(type==ENC_ELEMENTTYPE_VOLTAGESENSOR))

typedef struct _EncElement_Config
{
	MV_U16    enclosureID;
	MV_U16    magicID;
	MV_U8     type;
	MV_U8     status;
	MV_U8	  elementID;
	MV_U8     highCriticalThreshold;
	MV_U8     highWarningThreshold;
	MV_U8     lowCriticalThreshold;
	MV_U8     lowWarningThreshold;
	MV_U8	  reserved[5];
} EncElement_Config, *PEncElement_Config;

#pragma pack()

#endif
