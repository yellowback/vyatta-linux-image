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
#ifndef __HBA_EXPOSE_H__
#define __HBA_EXPOSE_H__
#include "hba_header.h"

typedef struct _Assigned_Uncached_Memory
{
	MV_PVOID		Virtual_Address;
	MV_PHYSICAL_ADDR	Physical_Address;
	MV_U32			Byte_Size;
	MV_U32			Reserved0;
} Assigned_Uncached_Memory, *PAssigned_Uncached_Memory;

typedef struct _Controller_Infor
{
	MV_LPVOID *Base_Address;
	MV_PVOID Pci_Device;
	MV_U16 Vendor_Id;
	MV_U16 Device_Id;
	MV_U8 Revision_Id;
	MV_U8 run_as_none_raid;
	MV_U8 Reserved[2];
} Controller_Infor, *PController_Infor;

typedef struct _SCSI_PASS_THROUGH_DIRECT {
	unsigned short Length;
	unsigned char  ScsiStatus;
	unsigned char  PathId;
	unsigned char  TargetId;
	unsigned char  Lun;
	unsigned char  CdbLength;
	unsigned char  SenseInfoLength;
	unsigned char  DataIn;
	unsigned long  DataTransferLength;
	unsigned long  TimeOutValue;
	void __user    *DataBuffer;
	unsigned long  SenseInfoOffset;
	unsigned char  Cdb[16];
}SCSI_PASS_THROUGH_DIRECT, *PSCSI_PASS_THROUGH_DIRECT,SCSI_REQUEST_BLOCK, *PSCSI_REQUEST_BLOCK;

/*ATA Protocols*/
enum _ATA_PROTOCOL {
	HRST 			= 0x00,
	SRST  			= 0x01,
 	BUS_IDLE 		= 0x02,
	NON_DATA 		= 0x03,
	PIO_DATA_IN 	= 0x04,
	PIO_DATA_OUT 	= 0x05,     
	DMA			= 0x06, 
	DMA_QUEUED	= 0x07,
	DEVICE_DIAGNOSTIC	= 0x08,
	DEVICE_RESET		= 0x09,
	UDMA_DATA_IN		= 0x0A,
	UDMA_DATA_OUT	= 0x0B,
	FPDMA				= 0x0C,
	RTN_INFO			= 0x0F,
};

#include "com_event_struct.h"
#include "com_event_define.h"
#include "com_event_define_ext.h"
#define MSG_QUEUE_DEPTH	2048

typedef struct _Driver_Event_Entry
{
	List_Head Queue_Pointer;
	DriverEvent_V2 Event;
} Driver_Event_Entry, *PDriver_Event_Entry;

MV_VOID
HBA_GetNextModuleSendFunction(
	IN MV_PVOID self_extension,
	OUT MV_PVOID *next_extension,
	OUT MV_VOID (**next_function)(MV_PVOID , PMV_Request)
	);

MV_VOID
HBA_GetUpperModuleNotificationFunction(
	IN MV_PVOID self_extension,
	OUT MV_PVOID *upper_extension,
	OUT MV_VOID (**upper_notificaton_function)(MV_PVOID,
						   enum Module_Event,
						   struct mod_notif_param *));

void hba_notify_upper_md(
			IN MV_PVOID extension,
			  enum Module_Event notifyEvent,
			  MV_PVOID event_param);


#define HBA_SleepMicrosecond(_x, _y) ossw_udelay(_y)
#define HBA_GetTimeInSecond          ossw_get_time_in_sec
#define HBA_GetMillisecondInDay      ossw_get_msec_of_time

MV_BOOLEAN HBA_CheckIsFlorence(MV_PVOID ext);

/*read pci config space*/
MV_U32 MV_PCI_READ_DWORD(MV_PVOID This, MV_U8 reg);
MV_VOID  MV_PCI_WRITE_DWORD(MV_PVOID This, MV_U32 val, MV_U8 reg);
void HBA_ModuleStarted(MV_PVOID extension);


void HBA_GetControllerInfor(
	IN MV_PVOID extension,
	OUT PController_Infor pController
	);


/* map bus addr in sg entry into cpu addr (access via. Data_Buffer) */
void hba_map_sg_to_buffer(void *preq);
void hba_unmap_sg_to_buffer(void *preq);

MV_BOOLEAN hba_msi_enabled(void *ext);

static inline MV_BOOLEAN
HBA_ModuleGetPhysicalAddress(MV_PVOID Module,
			     MV_PVOID Virtual,
			     MV_PVOID TranslationContext,
			     MV_PU64 PhysicalAddress,
			     MV_PU32 Length)
{
	panic("not supposed to be called.\n");
	return MV_FALSE;
};

int HBA_GetResource(void *extension,
		    enum Resource_Type type,
		    MV_U32  size,
		    Assigned_Uncached_Memory *dma_res);

int hba_get_uncache_resource(void *extension,
		    MV_U32  size,
		    Assigned_Uncached_Memory *dma_res);

void alloc_uncached_failed(void *extension);

MV_PVOID HBA_GetModuleExtension(MV_PVOID ext, MV_U32 mod_id);

MV_PVOID sgd_kmap(sgd_t  *sg);
MV_VOID sgd_kunmap(sgd_t  *sg,MV_PVOID mapped_addr);

MV_BOOLEAN __is_scsi_cmd_simulated(MV_U8 cmd_type);
MV_BOOLEAN __is_scsi_cmd_rcv_snd_diag(MV_U8 cmd_type);

void HBARequestCallback(MV_PVOID This,PMV_Request pReq);
void HBA_SleepMillisecond(MV_PVOID ext, MV_U32 msec);
void HBA_ModuleInitialize(MV_PVOID ext,
				 MV_U32   size,
				 MV_U16   max_io);
void HBA_ModuleShutdown(MV_PVOID extension);
void HBA_ModuleNotification(MV_PVOID This,
			     enum Module_Event event,
			     struct mod_notif_param *event_param);

MV_U32 HBA_ModuleGetResourceQuota(enum Resource_Type type, MV_U16 maxIo);
void HBA_ModuleStart(MV_PVOID extension);
void HBA_ModuleSendRequest(MV_PVOID this, PMV_Request req);


MV_U16 Timer_AddRequest(
	IN MV_PVOID extension,
	IN MV_U32 time_unit,
	IN MV_VOID (*routine) (MV_PVOID, MV_PVOID),
	IN MV_PVOID context1,
	IN MV_PVOID context2
	);

MV_U16 Timer_AddSmallRequest(
	IN MV_PVOID extension,
	IN MV_U32 time_unit,
	IN MV_VOID (*routine) (MV_PVOID, MV_PVOID),
	IN MV_PVOID context1,
	IN MV_PVOID context2
	);

void Timer_CancelRequest(
	IN MV_PVOID extension,
	IN MV_U16 request_index
	);

void Timer_CheckRequest(
	IN MV_PVOID extension
	);

MV_U32 Timer_GetResourceQuota(MV_U16 maxIo);
void Timer_Initialize(
	IN  MV_PVOID This,
	IN MV_PU8 pool,
	IN MV_U16 max_io
	);
void Timer_Stop(MV_PVOID This);

void * os_malloc_mem(void *extension, MV_U32 size, MV_U8 mem_type, MV_U16 alignment, MV_PHYSICAL_ADDR *phy);
MV_VOID core_push_queues(MV_PVOID core_p);
#define NO_CURRENT_TIMER		0xffff

void mv_hba_get_controller_pre(
	IN MV_PVOID extension,
	OUT PController_Infor pController
	);

MV_BOOLEAN add_event(IN MV_PVOID extension,
			    IN MV_U32 eventID,
			    IN MV_U16 deviceID,
			    IN MV_U8 severityLevel,
			    IN MV_U8 param_cnt,
			    IN MV_PU32 params,
			    IN MV_U8 SenseLength,
			    IN MV_PU8 psense,
			    IN MV_U16 trans_bit);

void get_event(MV_PVOID This, PMV_Request pReq);
MV_PVOID hba_mem_alloc(MV_U32 size,MV_BOOLEAN sg_use);
MV_VOID hba_mem_free(MV_PVOID mem_pool, MV_U32 size,MV_BOOLEAN sg_use);
void mvs_hexdump(u32 size, u8 *data, u32 baseaddr, const char *prefix);

MV_U32 hba_parse_ata_protocol(struct scsi_cmnd *scmd);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
extern unsigned int mv_prot_mask;
#endif

#endif

