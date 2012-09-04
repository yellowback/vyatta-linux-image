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
#include "mv_include.h"

#include "com_request_detail.h"
#include "hba_api.h"
#include "hba_exp.h"
#include "hba_inter.h"

#include "core_header.h"

#define SMART_TAG  'gSMt'

#define MvAllocatePool(Size)    \
    hba_mem_alloc(Size, MV_FALSE)
    
#define MvFreePool(Buffer, Size)    \
    hba_mem_free(Buffer, Size, MV_FALSE)
      
extern void Core_GetDeviceId(
	MV_PVOID pModule,
	MV_U16 begin_id,
	PDevice_Index pDeviceIndex);

#define SMART_STATUS_REQUEST_TIMER	100

void HBA_SmartThresholdStatusCallback(MV_PVOID pModule, PMV_Request pMvReq)
{
	MV_U8 i = 0, nextDevice = 0;
	get_device_id  func;
	Device_Index DeviceIndex ;
	PHD_SMART_Status_Request smart_buf;
	PRAID_Feature praid_feature = (PRAID_Feature)pModule;
	PHBA_Extension pHBA = (PHBA_Extension)praid_feature->pHBA;

	void HBA_CheckSmartStatus (MV_PVOID ModulePointer, MV_PVOID temp);

	nextDevice = (((MV_U16)pMvReq->Cdb[6]) << 8) | ((MV_U16)pMvReq->Cdb[5]);
	nextDevice++;
	{
		func =  Core_GetDeviceId;
	}
	func(pHBA,nextDevice,&DeviceIndex);
	while (!DeviceIndex.end) {
			pMvReq->Cdb[0] = APICDB0_PD;
			pMvReq->Cdb[1] = APICDB1_PD_GETSTATUS;
			smart_buf = (PHD_SMART_Status_Request)pMvReq->Data_Buffer;
			MV_ZeroMemory(&smart_buf->header, sizeof(RequestHeader));
			smart_buf->header.startingIndexOrId = DeviceIndex.device_id;
			smart_buf->header.requestType = REQUEST_BY_ID;
			smart_buf->header.numRequested = 1;
			MV_ZeroMemory(&smart_buf->hdSmartStatus, sizeof(HD_SMART_Status));
			pMvReq->Data_Buffer = ( MV_PVOID )smart_buf;
			pMvReq->Data_Transfer_Length = sizeof(HD_SMART_Status_Request);
			pMvReq->Cdb[4] = APICDB4_PD_SMART_RETURN_STATUS;
			pMvReq->Cdb[5] = (MV_U8)DeviceIndex.index;
			pMvReq->Cdb[6] = (MV_U8)((DeviceIndex.index & 0xFF00) >> 8);
			pMvReq->Device_Id = VIRTUAL_DEVICE_ID;

			pMvReq->Scsi_Status = REQ_STATUS_PENDING;
			pMvReq->Cmd_Flag = 0;
			pMvReq->Cmd_Initiator = praid_feature;

			praid_feature->pNextFunction( praid_feature->pNextExtension, pMvReq);
			return;

	}
	if(pMvReq->Data_Buffer != NULL){
		smart_buf = (PHD_SMART_Status_Request)pMvReq->Data_Buffer;
		MvFreePool(smart_buf, sizeof(HD_SMART_Status_Request));
		}
	List_Add(&pMvReq->Queue_Pointer, &praid_feature->Internal_Request);
	praid_feature->SMART_Status_Timer_Handle = Timer_AddRequest( pHBA, SMART_STATUS_REQUEST_TIMER, HBA_CheckSmartStatus , praid_feature, NULL);
}


void HBA_CheckSmartStatus (MV_PVOID pModule, MV_PVOID temp)
{
	MV_U16 id = 0;
	get_device_id  func;
	Device_Index DeviceIndex ;
	PHD_SMART_Status_Request smart_buf;
	PRAID_Feature praid_feature = (PRAID_Feature)pModule;
	PHBA_Extension pHBA = (PHBA_Extension)praid_feature->pHBA;

	{
		func =  Core_GetDeviceId;
	}
	func(pHBA,id,&DeviceIndex);

	while (!DeviceIndex.end) {
		PMV_Request pMvReq = List_GetFirstEntry((&praid_feature->Internal_Request), MV_Request, Queue_Pointer);
		if (pMvReq) {
			pMvReq->Completion = HBA_SmartThresholdStatusCallback;
			pMvReq->Cdb[0] = APICDB0_PD;
			pMvReq->Cdb[1] = APICDB1_PD_GETSTATUS;
			smart_buf = (PHD_SMART_Status_Request)MvAllocatePool(sizeof(HD_SMART_Status_Request));
			if (smart_buf == NULL) {
				MV_DPRINT(("Fail to get memory for HD_SMART_Status_Request!\n"));
        			return;
   			 }
		
			MV_ZeroMemory(&smart_buf->header, sizeof(RequestHeader));
			smart_buf->header.startingIndexOrId = DeviceIndex.device_id;
			smart_buf->header.requestType = REQUEST_BY_ID;
			smart_buf->header.numRequested = 1;
			MV_ZeroMemory(&smart_buf->hdSmartStatus, sizeof(HD_SMART_Status));
			pMvReq->Data_Buffer = ( MV_PVOID )smart_buf;
			pMvReq->Data_Transfer_Length = sizeof(HD_SMART_Status_Request);
			pMvReq->Cdb[4] = APICDB4_PD_SMART_RETURN_STATUS;
			pMvReq->Cdb[5] = (MV_U8)DeviceIndex.index;
			pMvReq->Cdb[6] = (MV_U8)((DeviceIndex.index & 0xFF00) >> 8);
			pMvReq->Device_Id = VIRTUAL_DEVICE_ID;

			pMvReq->Scsi_Status = REQ_STATUS_PENDING;
			pMvReq->Cmd_Flag = 0;
			pMvReq->Cmd_Initiator = praid_feature;

			praid_feature->pNextFunction( praid_feature->pNextExtension, pMvReq);
			return;
		}
	}
	praid_feature->SMART_Status_Timer_Handle = Timer_AddRequest( pHBA, SMART_STATUS_REQUEST_TIMER, HBA_CheckSmartStatus , praid_feature, NULL);
}


void HBA_SmartOnCallback(MV_PVOID pModule, PMV_Request pMvReq)
{
	MV_U8 i = 0, nextDevice = 0;
	get_device_id  func;
	Device_Index DeviceIndex ;
	PRAID_Feature praid_feature = (PRAID_Feature)pModule;
	PHBA_Extension pHBA = (PHBA_Extension)praid_feature->pHBA;

	nextDevice = (((MV_U16)pMvReq->Cdb[6]) << 8) | ((MV_U16)pMvReq->Cdb[5]);
	nextDevice++;
	func =  Core_GetDeviceId;
	func(pHBA,nextDevice,&DeviceIndex );
	while(!DeviceIndex.end){
		pMvReq->Cdb[0] = APICDB0_PD;
		pMvReq->Cdb[1] = APICDB1_PD_SETSETTING;
		pMvReq->Cdb[2] = (MV_U8)DeviceIndex.device_id;
		pMvReq->Cdb[3] = (MV_U8)((DeviceIndex.device_id & 0xFF00) >> 8);
		pMvReq->Cdb[4] = APICDB4_PD_SET_SMART_ON;
		pMvReq->Cdb[5] = (MV_U8)DeviceIndex.index;
		pMvReq->Cdb[6] = (MV_U8)((DeviceIndex.index & 0xFF00) >> 8);
		pMvReq->Device_Id = VIRTUAL_DEVICE_ID;

		pMvReq->Scsi_Status = REQ_STATUS_PENDING;
		pMvReq->Cmd_Flag = 0;
		pMvReq->Cmd_Initiator = praid_feature;

		praid_feature->pNextFunction( praid_feature->pNextExtension, pMvReq);
		return;

	}
	 List_Add(&pMvReq->Queue_Pointer, &praid_feature->Internal_Request);
}

void hba_handle_set_smart(MV_PVOID pModule, PMV_Request pReq)
{
	get_device_id  func;
	Device_Index DeviceIndex;
	MV_U16 id = 0;
	PAdapter_Config pAdapterConfig = NULL;
	PMV_Request pTmpMvReq = NULL;
	PRAID_Feature praid_feature = (PRAID_Feature)pModule;
	PHBA_Extension pHBA = (PHBA_Extension)praid_feature->pHBA;

	pAdapterConfig = (PAdapter_Config)pReq->Data_Buffer;
	func =  Core_GetDeviceId;
	func(pHBA,id,&DeviceIndex);

	if (pAdapterConfig->PollSMARTStatus) {
		if (praid_feature->SMART_Status_Timer_Handle != NO_CURRENT_TIMER) {
				Timer_CancelRequest( pHBA, praid_feature->SMART_Status_Timer_Handle );
		}
		praid_feature->SMART_Status_Timer_Handle = NO_CURRENT_TIMER;
		while(!DeviceIndex.end) {
			pTmpMvReq = List_GetFirstEntry((&praid_feature->Internal_Request), MV_Request, Queue_Pointer);
			if (pTmpMvReq) {
				pTmpMvReq->Completion = HBA_SmartOnCallback;
				pTmpMvReq->Cdb[0] = APICDB0_PD;
				pTmpMvReq->Cdb[1] = APICDB1_PD_SETSETTING;
				pTmpMvReq->Cdb[2] = (MV_U8)DeviceIndex.device_id;
				pTmpMvReq->Cdb[3] = (MV_U8)((DeviceIndex.device_id & 0xFF00) >> 8);
				pTmpMvReq->Cdb[4] = APICDB4_PD_SET_SMART_ON;
				pTmpMvReq->Cdb[5] = (MV_U8)(DeviceIndex.index);
				pTmpMvReq->Cdb[6] = (MV_U8)((DeviceIndex.index & 0xFF00) >> 8);
				pTmpMvReq->Device_Id = VIRTUAL_DEVICE_ID;

				pTmpMvReq->Scsi_Status = REQ_STATUS_PENDING;
				pTmpMvReq->Cmd_Flag = 0;
				pTmpMvReq->Cmd_Initiator = praid_feature;
				praid_feature->pNextFunction( praid_feature->pNextExtension, pTmpMvReq);
				break;
			}
		}
		praid_feature->SMART_Status_Timer_Handle = Timer_AddRequest(pHBA, SMART_STATUS_REQUEST_TIMER,HBA_CheckSmartStatus, praid_feature, NULL);
	} else {
			if (praid_feature->SMART_Status_Timer_Handle != NO_CURRENT_TIMER)
				Timer_CancelRequest( pHBA,praid_feature->SMART_Status_Timer_Handle );
			praid_feature->SMART_Status_Timer_Handle = NO_CURRENT_TIMER;
		}
}

void mvGetAdapterConfig( MV_PVOID This, PMV_Request pReq)
{
	HBA_Info_Page		HBA_Info_Param;
	PRAID_Feature praid_feature = (PRAID_Feature)This;
	PHBA_Extension pHBA = (PHBA_Extension)praid_feature->pHBA;
	PAdapter_Config	pAdapterConfig = (PAdapter_Config)pReq->Data_Buffer;
	PAdapter_Config_V2	pAdapterConfigV2 = NULL;

	if ( pReq->Data_Transfer_Length>=sizeof(Adapter_Config_V2) )
		pAdapterConfigV2 = (PAdapter_Config_V2)pAdapterConfig;

	if ( mv_nvram_init_param(pHBA, &HBA_Info_Param) ) {
		pAdapterConfigV2->InterruptCoalescing =
				(HBA_Info_Param.HBA_Flag&HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY) ? MV_TRUE: MV_FALSE;
		pAdapterConfigV2->PollSMARTStatus=
				(HBA_Info_Param.HBA_Flag&HBA_FLAG_SMART_ON) ? MV_TRUE: MV_FALSE;
		pAdapterConfigV2->AlarmOn = 
			(HBA_Info_Param.HBA_Flag & HBA_FLAG_ENABLE_BUZZER) ?
				MV_TRUE : MV_FALSE;
	} else {
		pAdapterConfigV2->InterruptCoalescing = MV_FALSE;
		if (praid_feature->SMART_Status_Timer_Handle != NO_CURRENT_TIMER)
			pAdapterConfigV2->PollSMARTStatus = MV_TRUE;
		else
			pAdapterConfigV2->PollSMARTStatus = MV_FALSE;
	}
	if (pHBA->RunAsNonRAID)
		pReq->Scsi_Status = REQ_STATUS_SUCCESS;
	else
		praid_feature->pNextFunction( praid_feature->pNextExtension, pReq );
}

void mvSetAdapterConfig( MV_PVOID This, PMV_Request pReq)
{
	HBA_Info_Page		HBA_Info_Param;
	PRAID_Feature praid_feature = (PRAID_Feature)This;
	PHBA_Extension pHBA = (PHBA_Extension)praid_feature->pHBA;
	PAdapter_Config	pAdapterConfig = (PAdapter_Config)pReq->Data_Buffer;
	PAdapter_Config_V2	pAdapterConfigV2 = NULL;
	MV_BOOLEAN need_update = MV_FALSE;

	if (pReq->Data_Transfer_Length>=sizeof(Adapter_Config_V2))
		pAdapterConfigV2 = (PAdapter_Config_V2)pAdapterConfig;

	if (mv_nvram_init_param(pHBA, &HBA_Info_Param)) {
		if ( pAdapterConfigV2->InterruptCoalescing ) {
			if ((HBA_Info_Param.HBA_Flag & HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY) == 0) {
					HBA_Info_Param.HBA_Flag |= HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
					need_update = MV_TRUE;
				}
			} else {
				if (HBA_Info_Param.HBA_Flag & HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY) {
					HBA_Info_Param.HBA_Flag &= ~HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
					need_update = MV_TRUE;
				}
			}
		if ( pAdapterConfigV2->PollSMARTStatus) {
				if ((HBA_Info_Param.HBA_Flag & HBA_FLAG_SMART_ON) == 0) {
					struct mod_notif_param param ={NULL,0,0,EVT_ID_SMART_FROM_OFF_TO_ON,0,SEVERITY_INFO,0,0,NULL,0};
					HBA_Info_Param.HBA_Flag |= HBA_FLAG_SMART_ON;
					HBA_ModuleNotification(pHBA,EVENT_LOG_GENERATED,&param);
					need_update = MV_TRUE;
				}
			} else {
				if (HBA_Info_Param.HBA_Flag & HBA_FLAG_SMART_ON) {
					struct mod_notif_param param ={NULL,0,0,EVT_ID_SMART_FROM_ON_TO_OFF,0,SEVERITY_INFO,0,0,NULL,0};
					HBA_Info_Param.HBA_Flag &= ~HBA_FLAG_SMART_ON;
					HBA_ModuleNotification(pHBA, EVENT_LOG_GENERATED,&param);
					need_update = MV_TRUE;
				}
			}
		if (pAdapterConfigV2->AlarmOn == MV_TRUE) {
			if ((HBA_Info_Param.HBA_Flag & HBA_FLAG_ENABLE_BUZZER) == 0) {
				struct mod_notif_param param ={NULL,0,0,EVT_ID_ALARM_TURN_ON,0,SEVERITY_INFO,0,0,NULL,0};
				HBA_Info_Param.HBA_Flag |= HBA_FLAG_ENABLE_BUZZER;
				HBA_ModuleNotification(pHBA,EVENT_LOG_GENERATED,&param);
				need_update = MV_TRUE;
			}
		} else {
			if (HBA_Info_Param.HBA_Flag & HBA_FLAG_ENABLE_BUZZER) {
				struct mod_notif_param param ={NULL,0,0,EVT_ID_ALARM_TURN_OFF,0,SEVERITY_INFO,0,0,NULL,0};
				HBA_Info_Param.HBA_Flag &= ~HBA_FLAG_ENABLE_BUZZER;
				HBA_ModuleNotification(pHBA, EVENT_LOG_GENERATED,&param);
				need_update = MV_TRUE;
			}
		}
		if (pAdapterConfigV2->SerialNo[0] != 0) {
			MV_U32 i;
			for(i=0; i<=20; i++)
				HBA_Info_Param.Serial_Num[i] = pAdapterConfigV2->SerialNo[i];
			need_update = MV_TRUE;
		}

		if (pAdapterConfigV2->ModelNumber[0] != 0) {
			MV_U32 i;
			for(i=0;i<=20;i++)
				HBA_Info_Param.model_number[i] = pAdapterConfigV2->ModelNumber[i];
			need_update = MV_TRUE;
		}

		if (need_update)
			mvuiHBA_modify_param(pHBA,&HBA_Info_Param);
	}

	hba_handle_set_smart(praid_feature,pReq);
	if(pHBA->RunAsNonRAID)
		pReq->Scsi_Status = REQ_STATUS_SUCCESS;
	else
		praid_feature->pNextFunction( praid_feature->pNextExtension, pReq );

}


MV_U32 RAID_Feature_GetResourceQuota( MV_U16 maxIo)
{
	 MV_U32 size = 0;
	if (maxIo==1)
		size += sizeof(MV_SG_Entry) * MAX_SG_ENTRY_REDUCED * maxIo;
	else
		size += sizeof(MV_SG_Entry) * MAX_SG_ENTRY * maxIo;
	size += maxIo * sizeof(MV_Request);
	return size;
}
void RAID_Feature_Initialize(MV_PVOID This,MV_U16 maxIo)
{
	PRAID_Feature praid_feature = (PRAID_Feature)This;
	PMV_Request pReq;
	MV_U8 sgEntryCount;
	MV_U16 i;
	MV_PTR_INTEGER temp, tmpSG;

	temp = (MV_PTR_INTEGER) This;
	if (maxIo==1)
		sgEntryCount = MAX_SG_ENTRY_REDUCED;
	else
		sgEntryCount = MAX_SG_ENTRY;
	MV_LIST_HEAD_INIT(&praid_feature->Internal_Request);
	tmpSG = temp;
	temp = tmpSG + sizeof(MV_SG_Entry) * sgEntryCount * maxIo;
	for ( i=0; i<maxIo; i++ ) {
		pReq = (PMV_Request)temp;
		MV_ZeroMemory(pReq, sizeof(MV_Request));
		pReq->SG_Table.Entry_Ptr = (PMV_SG_Entry)tmpSG;
		pReq->SG_Table.Max_Entry_Count = sgEntryCount;
		List_AddTail(&pReq->Queue_Pointer, &praid_feature->Internal_Request);
		temp += sizeof(MV_Request);
		tmpSG += sizeof(MV_SG_Entry) * sgEntryCount;
	}
	return;
}

void RAID_Feature_SetSendFunction( MV_PVOID This,
    MV_PVOID current_ext,
    MV_PVOID next_ext,
    MV_VOID (*next_function) (MV_PVOID, PMV_Request))
{
	PRAID_Feature praid_feature = (PRAID_Feature)This;
	praid_feature->pUpperExtension = current_ext;
	praid_feature->pNextExtension = next_ext;
	praid_feature->pNextFunction = next_function;
	praid_feature->pHBA = current_ext;
	praid_feature->SMART_Status_Timer_Handle = NO_CURRENT_TIMER;
	return;
}
