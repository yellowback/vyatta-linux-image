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
#ifndef __MV_COM_TYPE_H__
#define __MV_COM_TYPE_H__

#include "com_define.h"
#include "com_list.h"
#include "mv_config.h"

/*
 * Data Structure
 */
#define MAX_CDB_SIZE                            32

struct _MV_Request;
typedef struct _MV_Request MV_Request, *PMV_Request;

#define REQ_STATUS_SUCCESS                      0x0
#define REQ_STATUS_NOT_READY                    0x1
#define REQ_STATUS_MEDIA_ERROR                  0x2
#define REQ_STATUS_BUSY                         0x3
#define REQ_STATUS_INVALID_REQUEST              0x4
#define REQ_STATUS_INVALID_PARAMETER            0x5
#define REQ_STATUS_NO_DEVICE                    0x6
/* Sense data structure is the SCSI "Fixed format sense datat" format. */
#define REQ_STATUS_HAS_SENSE                    0x7
#define REQ_STATUS_ERROR                        0x8
#define REQ_STATUS_ERROR_WITH_SENSE             0x10
#define REQ_STATUS_TIMEOUT                      0x11
#define REQ_STATUS_DIF_GUARD_ERROR		0x12
#define REQ_STATUS_DIF_REF_TAG_ERROR		0x13
#define REQ_STATUS_DIF_APP_TAG_ERROR		0x14

/* Request initiator must set the status to REQ_STATUS_PENDING. */
#define REQ_STATUS_PENDING                      0x80
#define REQ_STATUS_TIME_OUT				0x83	


/*
 * Don't change the order here.
 * Module_StartAll will start from big id to small id.
 * Make sure module_set setting matches the Module_Id
 * MODULE_HBA must be the first one. Refer to Module_AssignModuleExtension.
 * And HBA_GetNextModuleSendFunction has an assumption that the next level
 * has larger ID.
 */
enum Module_Id
{
        MODULE_HBA = 0,
        MODULE_CORE,
        MAX_MODULE_NUMBER
};
#define MAX_POSSIBLE_MODULE_NUMBER              MAX_MODULE_NUMBER

#include "com_sgd.h"

typedef sgd_tbl_t MV_SG_Table, *PMV_SG_Table;
typedef sgd_t MV_SG_Entry, *PMV_SG_Entry;

/*
 * MV_Request is the general request type passed through different modules.
 * Must be 64 bit aligned.
 */

#define DEV_ID_TO_TARGET_ID(_dev_id)    ((MV_U8)((_dev_id) & 0x00FF))
#define DEV_ID_TO_LUN(_dev_id)                ((MV_U8) (((_dev_id) & 0xFF00) >> 8))
#define __MAKE_DEV_ID(_target_id, _lun)   (((MV_U16)(_target_id)) | (((MV_U16)(_lun)) << 8))

typedef void (*MV_ReqCompletion)(MV_PVOID,PMV_Request);

struct _MV_Request {
	List_Head pool_entry;
	List_Head Queue_Pointer;

	MV_PVOID cmd_table;
	MV_PVOID sg_table;
	MV_PHYSICAL_ADDR cmd_table_phy;
	MV_PHYSICAL_ADDR sg_table_phy;

	MV_PVOID Virtual_Buffer;
	MV_U16 Device_Id;
	MV_U16 Req_Flag;
	
	MV_U8 Scsi_Status;
	MV_U8 Tag;
	MV_U16 Req_Type;
	MV_PVOID Cmd_Initiator;           /* Which module(extension pointer)
					     				creates this request. */
	MV_U16 Reserved1;
	MV_U8 Sense_Info_Buffer_Length;
	MV_U8 NCQ_Tag;
	MV_U32 Data_Transfer_Length;
	
	MV_U8 lun[8];

	MV_U16		EEDPFlags;   
	MV_U16		Reserved2[3];
	
	MV_U8 Cdb[MAX_CDB_SIZE];
	MV_PVOID Data_Buffer;
	MV_PVOID Sense_Info_Buffer;

	MV_SG_Table SG_Table;

	MV_PVOID Org_Req;                /* The original request. */

	/* Each module should only use Context to store module information. */
	MV_PVOID Context[MAX_POSSIBLE_MODULE_NUMBER];

	MV_PVOID Scratch_Buffer;          /* pointer to the scratch buffer
										 that this request used */
	MV_PVOID pRaid_Request;

	MV_LBA LBA;
	MV_U32 Sector_Count;
	MV_U32 Cmd_Flag;

	MV_U32 Time_Out;                  /* how many seconds we should wait
					     before treating request as
					     timed-out */
	MV_U32 Splited_Count;

 	MV_PVOID Org_Req_Scmd;                /* The original scmd request from OS*/

	MV_ReqCompletion	Completion; /* call back function */
};

#define MV_REQUEST_SIZE                   sizeof(MV_Request)
/*
 * Request flag is the flag for the MV_Request data structure.
 */
#define REQ_FLAG_LBA_VALID                MV_BIT(0)
#define REQ_FLAG_CMD_FLAG_VALID           MV_BIT(1)
#define REQ_FLAG_RETRY                    MV_BIT(2)
#define REQ_FLAG_INTERNAL_SG              MV_BIT(3)
#define REQ_FLAG_FLUSH                    MV_BIT(6)
#define REQ_FLAG_CONSOLIDATE			  MV_BIT(8)
#define REQ_FLAG_NO_CONSOLIDATE           MV_BIT(9)
#define REQ_FLAG_EXTERNAL				  MV_BIT(10)
#define REQ_FLAG_BYPASS_HYBRID            MV_BIT(12)
#define REQ_FLAG_CONTINUE_ON_ERROR        MV_BIT(13)	/* Continue to handle the request even hit error. */
#define REQ_FLAG_NO_ERROR_RECORD          MV_BIT(14)	/* Needn't record error. */
#define REQ_FLAG_TRIM_CMD          MV_BIT(15)	/* Trim support by verify. */

/*
 * Request Type is the type of MV_Request.
 */
enum {
	/* use a value other than 0, and now they're bit-mapped */
	REQ_TYPE_OS       = 0x01,
	REQ_TYPE_RAID     = 0x02,
	REQ_TYPE_CACHE    = 0x04,
	REQ_TYPE_INTERNAL = 0x08,
	REQ_TYPE_SUBLD    = 0x10,
	REQ_TYPE_SUBBGA   = 0x20,
	REQ_TYPE_MP       = 0x40,
	REQ_TYPE_DS		  = 0x80,
	REQ_TYPE_CORE     = 0x100,
};

#define CMD_FLAG_NON_DATA                 MV_BIT(0)  /* 1-non data;
							0-data command */
#define CMD_FLAG_DMA                      MV_BIT(1)  /* 1-DMA */
#define CMD_FLAG_PIO			  MV_BIT(2)  /* 1-PIO */
#define CMD_FLAG_DATA_IN                  MV_BIT(3)  /* 1-host read data */
#define CMD_FLAG_DATA_OUT                 MV_BIT(4)	 /* 1-host write data */

#define CMD_FLAG_SMART                    MV_BIT(5)  /* 1-SMART command; 0-non SMART command*/
#define CMD_FLAG_SMART_ATA_12       MV_BIT(6)  /* SMART ATA_12  */
#define CMD_FLAG_SMART_ATA_16       MV_BIT(7)  /* SMART ATA_16; */
#define CMD_FLAG_CACHE_OS_DATABUF	 MV_BIT(9)


#define CMD_FLAG_CACHE_OS_DATABUF	  MV_BIT(9)
/*
 * The last 16 bit only can be set by the target. Only core driver knows
 * the device characteristic.
 */
#define CMD_FLAG_NCQ                      MV_BIT(16)
#define CMD_FLAG_TCQ                      MV_BIT(17)
#define CMD_FLAG_48BIT                    MV_BIT(18)
#define CMD_FLAG_PACKET                   MV_BIT(19)  /* ATAPI packet cmd */

#define CMD_FLAG_SCSI_PASS_THRU           MV_BIT(20)
#define CMD_FLAG_ATA_PASS_THRU            MV_BIT(21)

#define CMD_FLAG_SOFT_RESET               MV_BIT(22)

typedef struct _MV_Target_ID_Map
{
	MV_U16   Device_Id;
	MV_U8    Type;                    /* 0:LD, 1:Free Disk */
	MV_U8    Reserved;
} MV_Target_ID_Map, *PMV_Target_ID_Map;

/* Resource type */
enum Resource_Type
{
	RESOURCE_CACHED_MEMORY = 0,
	RESOURCE_UNCACHED_MEMORY
};

/* Module event type */
enum Module_Event
{
	EVENT_MODULE_ALL_STARTED = 0,
	EVENT_DEVICE_ARRIVAL,
	EVENT_DEVICE_REMOVAL,
	EVENT_LOG_GENERATED,
};

/* Error_Handling_State */
enum EH_State
{
	EH_NONE = 0,
	EH_ABORT_REQUEST,
	EH_LU_RESET,
	EH_DEVICE_RESET,
	EH_PORT_RESET,
	EH_CHIP_RESET,
	EH_SET_DISK_DOWN
};

typedef enum
{
	EH_REQ_NOP = 0,
	EH_REQ_ABORT_REQUEST,
	EH_REQ_HANDLE_TIMEOUT,
	EH_REQ_RESET_BUS,
	EH_REQ_RESET_CHANNEL,
	EH_REQ_RESET_DEVICE,
	EH_REQ_RESET_ADAPTER
}eh_req_type_t;

struct mod_notif_param {
        MV_PVOID  p_param;
        MV_U16    hi;
        MV_U16    lo;

        /* for event processing */
        MV_U32    event_id;
        MV_U16    dev_id;
        MV_U8     severity_lvl;
        MV_U8     param_count;
	MV_U8	sense_length;
	MV_PVOID p_sense;
	MV_U16	tran_hex_bit;
};

/*
 * Exposed Functions
 */

/*
 *
 * Miscellaneous Definitions
 *
 */
/* Rounding */

/* Packed */

#define MV_MAX(x,y)        (((x) > (y)) ? (x) : (y))
#define MV_MIN(x,y)        (((x) < (y)) ? (x) : (y))
#define MV_MAX_U64(x, y)   ((((x).value) > ((y).value)) ? (x) : (y))
#define MV_MIN_U64(x, y)   ((((x).value) < ((y).value)) ? (x) : (y))

#define MV_MAX_U8          0xFF
#define MV_MAX_U16         0xFFFF
#define MV_MAX_U32         0xFFFFFFFFL

#define ROUNDING_MASK(x, mask)  (((x)+(mask))&~(mask))
#define ROUNDING(value, align)  ROUNDING_MASK(value,   \
						 (typeof(value)) (align-1))
#define OFFSET_OF(type, member) offsetof(type, member)
#define SIZE_OF_POINTER (sizeof(void*))

#endif /* __MV_COM_TYPE_H__ */
