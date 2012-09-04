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
#include "linux_main.h"
#include "hba_mod.h"
#include "linux_iface.h"
#include "hba_timer.h"
#include "hba_api.h"

/*
 *
 * Other exposed functions
 *
 */

MV_U32 MV_PCI_READ_DWORD(MV_PVOID This, MV_U8 reg)
{
   	MV_U32 v;
	PHBA_Extension p_hba = (PHBA_Extension)HBA_GetModuleExtension(This,MODULE_HBA);
	MV_ASSERT( p_hba != NULL );
	MV_PCI_READ_CONFIG_DWORD(p_hba, reg, &v);
	return v;
}

MV_VOID  MV_PCI_WRITE_DWORD(MV_PVOID This, MV_U32 val, MV_U8 reg)
{
	PHBA_Extension p_hba = (PHBA_Extension)HBA_GetModuleExtension(This,MODULE_HBA);
	MV_ASSERT( p_hba != NULL );
	MV_PCI_WRITE_CONFIG_DWORD(p_hba, reg, val);
	return;
}

/*
 * The extension is the calling module extension.
 *   It can be any module extension.
 */
void HBA_ModuleStarted(MV_PVOID extension)
{
	struct mv_mod_desc *mod_desc = __ext_to_gen(extension)->desc;
	struct hba_extension *hba;
	struct mv_mod_desc *desc;
	struct mv_adp_desc *hba_desc;
	MV_DPRINT(( "start HBA_ModuleStarted addr %p, id %d.\n",mod_desc, mod_desc->module_id));
	mod_desc->status = MV_MOD_STARTED;
	desc = mod_desc;
	while (desc->parent)
		desc = desc->parent;
	hba = (struct hba_extension *) desc->extension;
	hba_desc = hba->desc->hba_desc;
	if (__mv_is_mod_all_started(desc->hba_desc)) {
		MV_DPRINT(( "all modules have been started.\n"));
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
		atomic_set(&hba_desc->hba_sync, 0);
#else
		complete(&hba_desc->cmpl);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
		hba->State = DRIVER_STATUS_STARTED;
		hba->desc->ops->module_notification(hba,
						    EVENT_MODULE_ALL_STARTED,
						    NULL);
	} else {
		if (mod_desc->parent && mod_desc->parent->parent)
		{
			MV_DPRINT(("start module %d.....\n",mod_desc->parent->module_id));
			mod_desc->parent->ops->module_start(mod_desc->parent->extension);
		}
	}

}

void hba_map_sg_to_buffer(void *preq)
{
	struct scsi_cmnd *scmd =NULL;
	struct scatterlist *sg =NULL;
	PMV_Request        req =NULL;
	void * virt_address;
	unsigned long flags = 0;
	unsigned int i = 0;

	req  = (PMV_Request) preq;

	if (REQ_TYPE_OS != req->Req_Type)
		return;
	scmd = (struct scsi_cmnd *) req->Org_Req_Scmd;
	sg = (struct scatterlist *) mv_rq_bf(scmd);

	if (mv_use_sg(scmd)) {
		BUG_ON(!req->Data_Transfer_Length);
		req->Data_Buffer = hba_mem_alloc(req->Data_Transfer_Length, MV_TRUE);
		if (!req->Data_Buffer) {
			MV_DPRINT(("can not allocate memory %x.\n", req->Data_Transfer_Length));
		}
		BUG_ON(!req->Data_Buffer);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
		req->Data_Transfer_Length = 0;
		for(i = 0; i < mv_use_sg(scmd); i++, sg++) {
			local_irq_save(flags);
			virt_address = map_sg_page(sg);
			memcpy(req->Data_Buffer + req->Data_Transfer_Length ,\
				 virt_address + sg->offset, sg->length);
			kunmap_atomic(virt_address, KM_IRQ0);
			req->Data_Transfer_Length += sg->length;
			local_irq_restore(flags);
		}
#else
		memset(req->Data_Buffer, 0, req->Data_Transfer_Length);
		local_irq_save(flags);
		if (scmd->sc_data_direction == DMA_TO_DEVICE)
			sg_copy_to_buffer(scsi_sglist(scmd),  scsi_sg_count(scmd), req->Data_Buffer, req->Data_Transfer_Length);
		local_irq_restore(flags);

#endif
	} else {
	        req->Data_Buffer = mv_rq_bf(scmd);
	}
}

void hba_unmap_sg_to_buffer(void *preq)
{
	struct scsi_cmnd *scmd = NULL;
	struct scatterlist *sg = NULL;
	PMV_Request        req = NULL;
	unsigned long flags = 0;
	void * virt_address;
	unsigned long i = 0;

	req  = (PMV_Request) preq;

	if (REQ_TYPE_OS != req->Req_Type)
		return;

	scmd = (struct scsi_cmnd *) req->Org_Req_Scmd;
	sg   = (struct scatterlist *) mv_rq_bf(scmd);

	if (mv_use_sg(scmd)) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
		req->Data_Transfer_Length = 0;
		for(i = 0; i < mv_use_sg(scmd); i++, sg++) {
			local_irq_save(flags);
			virt_address = map_sg_page(sg);
			memcpy(virt_address + sg->offset,req->Data_Buffer
				+ req->Data_Transfer_Length, sg->length);
			kunmap_atomic(virt_address , KM_IRQ0);
			req->Data_Transfer_Length += sg->length;
			local_irq_restore(flags);
		}
#else

		local_irq_save(flags);
		if (scmd->sc_data_direction == DMA_FROM_DEVICE)
			sg_copy_from_buffer(scsi_sglist(scmd),  scsi_sg_count(scmd), req->Data_Buffer, req->Data_Transfer_Length);
		local_irq_restore(flags);
#endif
		hba_mem_free(req->Data_Buffer,req->Data_Transfer_Length, MV_TRUE);
		req->Data_Buffer = mv_rq_bf(scmd);
	}
}

MV_BOOLEAN hba_msi_enabled(void *ext)
{
	struct hba_extension *phba = (struct hba_extension *)HBA_GetModuleExtension(ext, MODULE_HBA);
	return (phba->msi_enabled);
}

MV_PVOID HBA_GetModuleExtension(MV_PVOID ext, MV_U32 mod_id)
{
	struct mv_mod_desc *mod_desc;
	struct mv_adp_desc *hba_desc ;

	if(ext == NULL)
		return	NULL;
	mod_desc=(struct mv_mod_desc *)__ext_to_gen(ext)->desc;
	BUG_ON(NULL == mod_desc);
	hba_desc = mod_desc->hba_desc;

	MV_ASSERT(mod_id<MAX_MODULE_NUMBER);
	if (hba_desc !=NULL ) {
		LIST_FOR_EACH_ENTRY(mod_desc, &hba_desc->online_module_list, mod_entry)
		{
			BUG_ON(NULL == mod_desc);
			if (mod_desc->status != MV_MOD_GONE) {
				if ((mod_desc->module_id == mod_id) && (mod_desc->extension)) {
					MV_DASSERT(mod_desc->extension);
					return mod_desc->extension;
				}
			}
		}
	}
	return	NULL;
}

MV_BOOLEAN __is_scsi_cmd_simulated(MV_U8 cmd_type)
{
	switch (cmd_type)
	{
	case SCSI_CMD_INQUIRY:
	case SCSI_CMD_READ_CAPACITY_10:
	case SCSI_CMD_READ_CAPACITY_16:
	case SCSI_CMD_SYNCHRONIZE_CACHE_10:
	case SCSI_CMD_TEST_UNIT_READY:
	case SCSI_CMD_REQUEST_SENSE:
	case SCSI_CMD_RESERVE_6:
	case SCSI_CMD_RELEASE_6:
	case SCSI_CMD_REPORT_LUN:
	case SCSI_CMD_MODE_SENSE_6:
	case SCSI_CMD_MODE_SENSE_10:
	case SCSI_CMD_MODE_SELECT_6:
	case SCSI_CMD_MODE_SELECT_10:
		return MV_TRUE;
	default:
		return MV_FALSE;
	}
}
MV_BOOLEAN __is_scsi_cmd_rcv_snd_diag(MV_U8 cmd_type)
{
	switch (cmd_type){
	case API_SCSI_CMD_RCV_DIAG_RSLT:
	case API_SCSI_CMD_SND_DIAG	:
		return  MV_TRUE;
	default:
		return MV_FALSE;
	}
}

void HBARequestCallback(MV_PVOID This,PMV_Request pReq)
{
	struct hba_extension *phba = (struct hba_extension *)HBA_GetModuleExtension(This, MODULE_HBA);
	struct scsi_cmnd *scmd = (struct scsi_cmnd *)pReq->Org_Req_Scmd;

	mv_complete_request(phba, scmd, pReq);
	phba->Io_Count--;
	hba_req_cache_free(phba,pReq);
}

void HBA_GetControllerInfor(
	IN MV_PVOID extension,
	OUT PController_Infor pController
	)
{
	pController->Base_Address = __ext_to_gen(extension)->desc->hba_desc->Base_Address;
	pController->Vendor_Id = __ext_to_gen(extension)->desc->hba_desc->Vendor_Id;
	pController->Device_Id = __ext_to_gen(extension)->desc->hba_desc->Device_Id;
	pController->Revision_Id = __ext_to_gen(extension)->desc->hba_desc->Revision_Id;
	pController->Pci_Device = __ext_to_gen(extension)->desc->hba_desc->dev;
	pController->run_as_none_raid = __ext_to_gen(extension)->desc->hba_desc->RunAsNonRAID;
}

 void mv_hba_get_controller_pre(
	 IN MV_PVOID extension,
	 OUT PController_Infor pController
	 )
 {
	 struct mv_adp_desc *hba_desc=(struct mv_adp_desc *)extension;
	 pController->Base_Address = hba_desc->Base_Address;
	 pController->Vendor_Id = hba_desc->Vendor_Id;
	 pController->Device_Id = hba_desc->Device_Id;
	 pController->Revision_Id = hba_desc->Revision_Id;
 }

MV_U32 HBA_ModuleGetResourceQuota(enum Resource_Type type, MV_U16 maxIo)
{
	MV_U32 size = 0;

	if (type == RESOURCE_CACHED_MEMORY) {
		size = sizeof(HBA_Extension);

		size += 8;
		size = ROUNDING(size, 8);

		size +=  RAID_Feature_GetResourceQuota(maxIo);
		size = ROUNDING(size, 8);

		size += Timer_GetResourceQuota(maxIo);
		size = ROUNDING(size, 8);

		size += sizeof(Driver_Event_Entry) * MAX_EVENTS;
		size = ROUNDING(size, 8);
		MV_ASSERT(size == ROUNDING(size, 8));
	
		size += sizeof(struct mv_lu) * MV_MAX_ID;
		size = ROUNDING(size, 8);
		MV_ASSERT(size == ROUNDING(size, 8));
	}
	return size;
}

void core_disable_ints(void * ext);
void core_enable_ints(void * ext);
MV_BOOLEAN Core_InterruptCheckIRQ(MV_PVOID This);
void Core_InterruptHandleIRQ(MV_PVOID This);
static void run_tasklet(PHBA_Extension   phba)
{
	struct mv_mod_desc *core_desc;

	int retval = MV_FALSE;
		
	spin_lock_bh(&phba->desc->hba_desc->global_lock);
	core_desc=__get_lowest_module(phba->desc->hba_desc);
	       
	retval = Core_InterruptCheckIRQ(core_desc->extension);
       if(!retval)
       	goto out;
       
	Core_InterruptHandleIRQ(core_desc->extension);
		
out:
	spin_unlock_bh(&phba->desc->hba_desc->global_lock);
       core_enable_ints(core_desc->extension);
	
}

void HBA_ModuleInitialize(MV_PVOID ext,
				 MV_U32   size,
				 MV_U16   max_io)
{
	PHBA_Extension phba = (PHBA_Extension)ext;
	MV_PTR_INTEGER temp = (MV_PTR_INTEGER)phba + sizeof(HBA_Extension);
	MV_U32 i;
	MV_U32 sg_num;
	PDriver_Event_Entry pEvent = NULL;

	phba->State    = DRIVER_STATUS_IDLE;
	phba->Io_Count = 0;
	phba->Ioctl_Io_Count = 0;
	phba->Max_Io   = max_io;

	init_completion(&phba->desc->hba_desc->cmpl);
	init_completion(&phba->desc->hba_desc->ioctl_cmpl);

	if (max_io > 1)
		sg_num   = MAX_SG_ENTRY;
	else
		sg_num   = MAX_SG_ENTRY_REDUCED;

	sg_num *= 2;
	MV_DPRINT(("check struct _MV_Request  %x.\n",sizeof(struct _MV_Request)));

	phba->max_sg_count = sg_num;
	MV_DPRINT(("HBA allocate max sg count %d.\n",phba->max_sg_count));
	hba_req_cache_create(phba);
	if(!phba->mv_mempool){
		MV_PRINT("allcate cache failed\n");
		alloc_uncached_failed(HBA_GetModuleExtension( ext, MODULE_HBA ));
		return;
	}

	tasklet_init(&phba->desc->hba_desc->mv_tasklet,
			(void (*)(unsigned long))run_tasklet, (unsigned long)phba);
	spin_lock_init(&phba->desc->hba_desc->tasklet_count_lock);	

	MV_LIST_HEAD_INIT(&phba->Stored_Events);
	MV_LIST_HEAD_INIT(&phba->Free_Events);
	phba->Num_Stored_Events = 0;
	phba->SequenceNumber = 0;

	MV_ASSERT(sizeof(Driver_Event_Entry) ==
		  ROUNDING(sizeof(Driver_Event_Entry), 8));
	temp = ROUNDING(((MV_PTR_INTEGER) temp), 8);

	for (i = 0; i < MAX_EVENTS; i++) {
		pEvent = (PDriver_Event_Entry) temp;
		List_AddTail(&pEvent->Queue_Pointer, &phba->Free_Events);
		temp += sizeof(Driver_Event_Entry);
	}

	for (i = 0; i < MV_MAX_TARGET_NUMBER; i++) {
		phba->mv_unit[i].sdev = NULL;
		phba->mv_unit[i].lun = 0xFFFF;
		phba->mv_unit[i].id = i;
		phba->mv_unit[i].target_id = 0xFFFF;
	}
	
	phba->p_raid_feature = (MV_PVOID)temp;
	RAID_Feature_Initialize(phba->p_raid_feature , max_io);
	temp += RAID_Feature_GetResourceQuota(max_io);
	Timer_Initialize(phba, (MV_PU8)temp, max_io);

}


void HBA_ModuleShutdown(MV_PVOID extension)
{
	PHBA_Extension hba = (PHBA_Extension) extension;
	struct mv_adp_desc *hba_desc = hba->desc->hba_desc;

	if (DRIVER_STATUS_STARTED == hba->State) {
		scsi_remove_host(hba_desc->hba_host);
		hba_send_shutdown_req((MV_PVOID)hba);
		while(hba->Io_Count)
			HBA_SleepMillisecond(NULL,10);
		free_irq(hba_desc->dev->irq, hba);
		
		if (hba->msi_enabled)
			pci_disable_msi(hba->desc->hba_desc->dev);

		scsi_host_put(hba_desc->hba_host);
		hba_desc->hba_host = NULL;
	}
	
	tasklet_kill(&hba_desc->mv_tasklet);
	hba_req_cache_destroy(hba);
	Timer_Stop(hba);

	hba->State = DRIVER_STATUS_SHUTDOWN;
}

MV_BOOLEAN add_event(IN MV_PVOID extension,
			    IN MV_U32 eventID,
			    IN MV_U16 deviceID,
			    IN MV_U8 severityLevel,
			    IN MV_U8 param_cnt,
			    IN MV_PU32 params,
			    IN MV_U8 SenseLength,
			    IN MV_PU8 psense,
			    IN MV_U16 trans_bit)
{
	struct hba_extension * hba = (struct hba_extension *) extension;
	PDriver_Event_Entry pEvent;
	static MV_U32 local_time=0;

	if (param_cnt > MAX_EVENT_PARAMS)
		return MV_FALSE;

	local_time = ossw_get_local_time();

	if (List_Empty(&hba->Free_Events)) {
		/* No free entry, we need to reuse the oldest entry from
		 * Stored_Events.
		 */
		MV_ASSERT(!List_Empty(&hba->Stored_Events));
		MV_ASSERT(hba->Num_Stored_Events == MAX_EVENTS);
		pEvent = List_GetFirstEntry((&hba->Stored_Events), Driver_Event_Entry, Queue_Pointer);
	} else {
		pEvent = List_GetFirstEntry((&hba->Free_Events), Driver_Event_Entry, Queue_Pointer);
		hba->Num_Stored_Events++;
		MV_ASSERT(hba->Num_Stored_Events <= MAX_EVENTS);
	}


	pEvent->Event.Event.AdapterID = hba->desc->hba_desc->id;
	pEvent->Event.Event.EventID = eventID;
	pEvent->Event.Event.SequenceNo = hba->SequenceNumber++;
	pEvent->Event.Event.Severity = severityLevel;
	pEvent->Event.Event.DeviceID = deviceID;
	pEvent->Event.Event.TimeStamp = local_time;

	if (param_cnt > 0 && params != NULL)
		MV_CopyMemory( (MV_PVOID)pEvent->Event.Event.Params, (MV_PVOID)params, param_cnt * 4 );
	if(SenseLength>0&&psense!=NULL)
	{
		if(SenseLength>MAX_EVENT_SENSE_DATA_COUNT)
			SenseLength=MAX_EVENT_SENSE_DATA_COUNT;
		pEvent->Event.SenseDataLength=SenseLength;
		MV_CopyMemory((MV_PVOID)pEvent->Event.SenseData, (MV_PVOID)psense, SenseLength);
	}
	else
		pEvent->Event.SenseDataLength=0;

	List_AddTail(&pEvent->Queue_Pointer, &hba->Stored_Events);
	return MV_TRUE;
}

void get_event(MV_PVOID This, PMV_Request pReq)
{
	struct hba_extension * hba = (struct hba_extension *) This;
	PEventRequest_V2 pEventReq = (PEventRequest_V2)pReq->Data_Buffer;
	PDriver_Event_Entry pfirst_event;
	MV_U8 count = 0;

	pEventReq->Count = 0;

	if ( hba->Num_Stored_Events > 0 )
	{
		MV_DASSERT( !List_Empty(&hba->Stored_Events) );
		while (!List_Empty(&hba->Stored_Events) &&
		       (count < MAX_EVENTS_RETURNED)) {
			pfirst_event = List_GetFirstEntry((&hba->Stored_Events), Driver_Event_Entry, Queue_Pointer);
			MV_CopyMemory(&pEventReq->Events[count],
				      &pfirst_event->Event,
				      sizeof(DriverEvent_V2));
			hba->Num_Stored_Events--;
			List_AddTail(&pfirst_event->Queue_Pointer,
				      &hba->Free_Events );
			count++;
		}
		pEventReq->Count = count;
	}

	pReq->Scsi_Status = REQ_STATUS_SUCCESS;
	return;
}

void HBA_ModuleNotification(MV_PVOID This, enum Module_Event event, struct mod_notif_param *event_param)
{
	struct hba_extension * hba = (struct hba_extension *)HBA_GetModuleExtension(This, MODULE_HBA);

	switch (event) {
	case EVENT_LOG_GENERATED:
		add_event(hba, event_param->event_id,
			  event_param->dev_id, event_param->severity_lvl,
			  event_param->param_count, (MV_PU32) event_param->p_param,
			  event_param->sense_length, (MV_PU8)event_param->p_sense,
			  event_param->tran_hex_bit);
		break;
	case EVENT_DEVICE_ARRIVAL:
	case EVENT_DEVICE_REMOVAL:
		if (hba->first_scan==0) {
			hba_msg_insert(hba,
				       event,
				       (event_param == NULL)?0:(event_param->lo|((event_param->hi<<16)&0xffff0000)) );
		}
		break;

	default:
		break;
	}
}


int HBA_GetResource(void *extension,
		    enum Resource_Type type,
		    MV_U32  size,
		    Assigned_Uncached_Memory *dma_res)
{
	struct mv_mod_res *mod_res;
	struct mv_mod_desc *mod_desc = __ext_to_gen(extension)->desc;
	if(!mod_desc){
		MV_DPRINT(("mod decript is destried.\n"));
	}
	mod_res = hba_mem_alloc(sizeof(struct mv_mod_res),MV_FALSE);
	if (NULL == mod_res) {
		MV_PRINT("unable to allocate memory for module %d resource management.\n", mod_desc->module_id);
		return -1;
	}

	memset(mod_res, 0, sizeof(struct mv_mod_res));
	mod_res->size = size;
	mod_res->type = type;
	switch (type) {
	case RESOURCE_UNCACHED_MEMORY :
		if (__alloc_consistent_mem(mod_res, mod_desc->hba_desc->dev)) {
			MV_PRINT("unable to allocate 0x%x uncached mem.\n", size);
			hba_mem_free(mod_res,sizeof(struct mv_mod_res),MV_FALSE);
			return -1;
		}
		List_Add(&mod_res->res_entry, &mod_desc->res_list);
		memset(mod_res->virt_addr, 0, size);
		dma_res->Virtual_Address  = mod_res->virt_addr;
		dma_res->Physical_Address = mod_res->bus_addr;
		dma_res->Byte_Size        = size;
		break;
	case RESOURCE_CACHED_MEMORY :
	default:
		hba_mem_free(mod_res,sizeof(struct mv_mod_res),MV_FALSE);
		MV_PRINT("unknown resource type %d.\n", type);
		return -1;
	}
	return 0;
}

int hba_get_uncache_resource(void *extension,
		    MV_U32  size,
		    Assigned_Uncached_Memory *dma_res)
{
		return HBA_GetResource(extension, RESOURCE_UNCACHED_MEMORY, size,dma_res);
}


void alloc_uncached_failed(void *extension)
{	
	struct mv_mod_desc *mod_desc = __ext_to_gen(extension)->desc;
	mod_desc->hba_desc->alloc_uncahemem_failed = MV_TRUE;
}

void * os_malloc_mem(void *extension, MV_U32 size, MV_U8 mem_type, MV_U16 alignment, MV_PHYSICAL_ADDR *phy)
{
	Assigned_Uncached_Memory dma_res;
	if(HBA_GetResource(extension, mem_type, size, &dma_res))
		return NULL;
	phy->value = dma_res.Physical_Address.value;
	return dma_res.Virtual_Address;
}

MV_VOID
HBA_GetNextModuleSendFunction(
	IN MV_PVOID self_extension,
	OUT MV_PVOID *next_extension,
	OUT MV_VOID (**next_function)(MV_PVOID , PMV_Request)
	)

 {
	   *(next_extension) = __ext_to_gen(self_extension)->desc->child->extension;
	   *(next_function) =  __ext_to_gen(self_extension)->desc->child->ops->module_sendrequest;
 }

MV_VOID
HBA_GetUpperModuleNotificationFunction(
	IN MV_PVOID self_extension,
	OUT MV_PVOID *upper_extension,
	OUT MV_VOID (**upper_notificaton_function)(MV_PVOID,
						   enum Module_Event,
						   struct mod_notif_param *))
{
	*(upper_extension) = __ext_to_gen(self_extension)->desc->parent->extension;
	*(upper_notificaton_function) = __ext_to_gen(self_extension)->desc->parent->ops->module_notification;
}


void hba_notify_upper_md(
			IN MV_PVOID extension,
			  enum Module_Event notifyEvent,
			  MV_PVOID event_param)
{
	__ext_to_gen(extension)->desc->parent->ops->module_notification(
                                            __ext_to_gen(extension)->desc->parent->extension,
					    					notifyEvent,
					    					event_param);
}

#ifndef CONFIG_X86_64
#define ossw_mdelay(x) ((__builtin_constant_p(x) && (x)<5)? ossw_udelay((x)*1000):\
	({unsigned long __ms=(x); while (__ms--) ossw_udelay(1000);}))
#else
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,13))
	#ifndef CONFIG_ARM
#define ossw_mdelay(x) ((__builtin_constant_p(x) && (x)<5)? ossw_udelay((x)*1000):\
	({unsigned long __ms=(x); while (__ms--){ if(__ms%2000) {touch_nmi_watchdog();touch_softlockup_watchdog();} ossw_udelay(1000);}}))
	#else
#define ossw_mdelay(x) ((__builtin_constant_p(x) && (x)<5)? ossw_udelay((x)*1000):\
	({unsigned long __ms=(x); while (__ms--){ if(__ms%2000) {touch_nmi_watchdog();} ossw_udelay(1000);}}))
	#endif

#else
#define ossw_mdelay(x) ((__builtin_constant_p(x) && (x)<5)? ossw_udelay((x)*1000):\
	({unsigned long __ms=(x); while (__ms--){ if(__ms%2000) touch_nmi_watchdog(); ossw_udelay(1000);}}))
#endif
#endif

static void mv_touch_nmi_watchdog(void)
{
#ifdef CONFIG_X86_64
	touch_nmi_watchdog();
#endif /* CONFIG_X86_64*/

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 13)
#ifndef CONFIG_ARM
	touch_softlockup_watchdog();
#endif
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 13) */
}

void HBA_SleepMillisecond(MV_PVOID ext, MV_U32 msec)
{
	MV_U32 	tmp=0;
	MV_U32	mod_msec=2000;
	if (in_interrupt() || irqs_disabled()){
		mv_touch_nmi_watchdog();
		if (msec<=mod_msec)
			ossw_mdelay(msec);
		else {
			for (tmp=0;tmp<msec/mod_msec;tmp++) {
				ossw_mdelay(mod_msec);
				mv_touch_nmi_watchdog();
			}
			if (msec%mod_msec)
				ossw_mdelay(msec%mod_msec);
		}
		mv_touch_nmi_watchdog();
	} else {
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout( msec );
	}
}

MV_PVOID hba_mem_alloc(MV_U32 size,MV_BOOLEAN sg_use)
{
	MV_PVOID mem_pool;

	if(sg_use){
		if(size > (128*1024 -16))
			WARN_ON(1);
	}
	if( (size <= 4*PAGE_SIZE)||(sg_use))
		mem_pool = kmalloc(size,GFP_ATOMIC);
	else if ((size > 4*PAGE_SIZE)&&(!in_interrupt())&&(!irqs_disabled()))
		mem_pool = vmalloc(size);
	else
		mem_pool = kmalloc(size,GFP_ATOMIC);

	return mem_pool;
}

MV_VOID hba_mem_free(MV_PVOID mem_pool, MV_U32 size,MV_BOOLEAN sg_use)
{
	if( (size <= 4*PAGE_SIZE)||(sg_use))
		kfree(mem_pool);
	else if( (size > 4*PAGE_SIZE)&&(!in_interrupt())&&(!irqs_disabled()))
		vfree(mem_pool);
	else
		kfree(mem_pool);
}

void mvs_hexdump(u32 size, u8 *data, u32 baseaddr, const char *prefix)
{
	u32 i;
	u32 run;
	u32 offset;

	offset = 0;
	printk("%s : \n", prefix);
	while (size) {
		printk("%08X : ", baseaddr + offset);
		if (size >= 16)
			run = 16;
		else
			run = size;
		size -= run;
		for (i = 0; i < 16; i++) {
			if (i < run)
				printk("%02X ", (u32)data[i]);
			else
				printk("   ");
		}
		printk(": ");
		for (i = 0; i < run; i++)
			printk("%c",
				isalnum(data[i]) ? data[i] : '.');
		printk("\n");
		data = &data[16];
		offset += run;
	}
	printk("\n");
}

MV_BOOLEAN HBA_CheckIsFlorence(MV_PVOID ext)
{
	struct hba_extension *phba = (struct hba_extension *)HBA_GetModuleExtension(ext, MODULE_HBA);
	MV_U16 device_id;
	device_id = phba->Device_Id;
	return (device_id == DEVICE_ID_948F ? MV_TRUE : MV_FALSE);
}

MV_U32 hba_parse_ata_protocol(struct scsi_cmnd *scmd)
{
	MV_U8 protocol = 0, t_length = 0, t_dir = 0;
	MV_U32 cmd_flag =0;

	protocol = (scmd->cmnd[1]>> 1) & 0x0F;
	if(protocol== HRST || protocol==SRST){
		return cmd_flag;
	}
	
	t_length = scmd->cmnd[2] & 0x03;
	t_dir = (scmd->cmnd[2] >> 3) & 0x01;
	
	if (t_length == 0){
		cmd_flag = CMD_FLAG_NON_DATA;
	}else {
		if (t_dir == 0)
			cmd_flag = CMD_FLAG_DATA_OUT;
		else
			cmd_flag = CMD_FLAG_DATA_IN;
	}
	switch (protocol) {
	case NON_DATA:
		cmd_flag |= CMD_FLAG_NON_DATA;
		break;
	case PIO_DATA_IN:
		cmd_flag |= CMD_FLAG_PIO;
		if (!(cmd_flag & CMD_FLAG_DATA_IN))
			cmd_flag |= CMD_FLAG_DATA_IN;
		break;
	case PIO_DATA_OUT:
		cmd_flag |= CMD_FLAG_PIO;
		if (!(cmd_flag & CMD_FLAG_DATA_OUT))
			cmd_flag |= CMD_FLAG_DATA_OUT;
		break;
	case DMA:
		cmd_flag |= CMD_FLAG_DMA;
		break;
	case DMA_QUEUED:
		cmd_flag |= (CMD_FLAG_DMA | CMD_FLAG_TCQ);
		break;
	case DEVICE_DIAGNOSTIC:
	case DEVICE_RESET:
		break;
	case UDMA_DATA_IN:
		cmd_flag |= CMD_FLAG_DMA;
		if (!(cmd_flag & CMD_FLAG_DATA_IN))
			cmd_flag |= CMD_FLAG_DATA_IN;
		break;
	case UDMA_DATA_OUT:
		cmd_flag |= CMD_FLAG_DMA;
		if (!(cmd_flag & CMD_FLAG_DATA_OUT))
			cmd_flag |= CMD_FLAG_DATA_OUT;
		break;
	case FPDMA:
		cmd_flag |= (CMD_FLAG_DMA | CMD_FLAG_NCQ);
		break;
	case RTN_INFO:
		break;
	default:
		MV_PRINT("Unsupported ATA Protocol = 0x%x\n", protocol);
		break;
	}
	return cmd_flag;
}

