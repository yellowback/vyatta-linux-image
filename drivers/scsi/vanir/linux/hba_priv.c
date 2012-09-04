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
#include "hba_header.h"
#include "hba_api.h"
#include "hba_mod.h"
#include "com_struct.h"
#include "com_nvram.h"

 void Core_Flash_BIOS_Version(MV_PVOID extension,PMV_Request req);

/* helper functions related to HBA_ModuleSendRequest */
void mvGetAdapterInfo( MV_PVOID This, PMV_Request pReq )
{
	PHBA_Extension pHBA = (PHBA_Extension)This;
	MV_PVOID pCore = HBA_GetModuleExtension(This,MODULE_CORE);
	MV_U8 i;
	HBA_Info_Page HBA_Info_Param;
	PAdapter_Info pAdInfo;

	pAdInfo = (PAdapter_Info)pReq->Data_Buffer;
	MV_ZeroMemory(pAdInfo, sizeof(Adapter_Info));
	pAdInfo->DriverVersion.VerMajor = VER_MAJOR;
	pAdInfo->DriverVersion.VerMinor = VER_MINOR;
	pAdInfo->DriverVersion.VerOEM = VER_OEM;
	pAdInfo->DriverVersion.VerBuild = VER_BUILD;

	pAdInfo->SystemIOBusNumber = pHBA->Adapter_Bus_Number;
	pAdInfo->SlotNumber = pHBA->Adapter_Device_Number;
	pAdInfo->VenID = pHBA->Vendor_Id;
	pAdInfo->DevID = pHBA->Device_Id;
	pAdInfo->SubDevID = pHBA->Sub_System_Id;
	pAdInfo->SubVenID = pHBA->Sub_Vendor_Id;
	pAdInfo->RevisionID = pHBA->Revision_Id;
	pAdInfo->AdvancedFeatures|=ADV_FEATURE_EVENT_WITH_SENSE_CODE;
	pAdInfo->AdvancedFeatures|=ADV_FEATURE_BIOS_OPTION_SUPPORT;
	pAdInfo->AdvancedFeatures|=ADV_FEATURE_NO_MUTIL_VD_PER_PD;
	pAdInfo->MaxBufferSize = 1;
	if ( pHBA->Device_Id == DEVICE_ID_THORLITE_2S1P ||
		pHBA->Device_Id == DEVICE_ID_THORLITE_2S1P_WITH_FLASH ) {
		pAdInfo->PortCount = 3;
		pAdInfo->PortSupportType = HD_TYPE_SATA | HD_TYPE_PATA;
	} else if (pHBA->Device_Id == DEVICE_ID_THORLITE_0S1P) {
		pAdInfo->PortCount = 1;
		pAdInfo->PortSupportType = HD_TYPE_SATA | HD_TYPE_PATA;
	} else if ((pHBA->Device_Id == DEVICE_ID_6440) ||
			   (pHBA->Device_Id == DEVICE_ID_6445) ||
			   (pHBA->Device_Id == DEVICE_ID_6340)||
			   (pHBA->Device_Id == DEVICE_ID_9440) ||
			   (pHBA->Device_Id == DEVICE_ID_9445)) {
		pAdInfo->PortCount = 4;
		pAdInfo->PortSupportType = HD_TYPE_SATA | HD_TYPE_SAS;
	} else if ((pHBA->Device_Id == DEVICE_ID_6485) ||
			(pHBA->Device_Id == DEVICE_ID_6480 )||
                           (pHBA->Device_Id == DEVICE_ID_9480) ||
                           (pHBA->Device_Id == DEVICE_ID_9485) ||
                           (pHBA->Device_Id == DEVICE_ID_948F)) {
		pAdInfo->PortCount = 8;
		pAdInfo->PortSupportType = HD_TYPE_SATA | HD_TYPE_SAS;
	} else if (pHBA->Device_Id == DEVICE_ID_6320) {
		pAdInfo->PortCount = 2;
		pAdInfo->PortSupportType = HD_TYPE_SATA | HD_TYPE_SAS;
	} else {
		pAdInfo->PortCount = 5;
		pAdInfo->PortSupportType = HD_TYPE_SATA | HD_TYPE_SAS;
	}

	if(mv_nvram_init_param(pHBA, &HBA_Info_Param)) {
		for(i=0;i<=19;i++)
			pAdInfo->SerialNo[i] = HBA_Info_Param.Serial_Num[i];
		for(i=0;i<=19;i++)
			pAdInfo->ModelNumber[i] = HBA_Info_Param.model_number[i];
	}
	if(MAX_DEVICE_SUPPORTED_PERFORMANCE <= 0xFF)
		pAdInfo->MaxHD = (MV_U8)MAX_DEVICE_SUPPORTED_PERFORMANCE;
	else
		pAdInfo->MaxHD_Ext = MAX_DEVICE_SUPPORTED_PERFORMANCE;
	pAdInfo->MaxExpander = MAX_EXPANDER_SUPPORTED;
	pAdInfo->MaxPM = MAX_PM_SUPPORTED;
	
	pAdInfo->MaxSpeed = pHBA->pcie_max_lnk_spd;
	pAdInfo->MaxLinkWidth = pHBA->pcie_max_bus_wdth;
	pAdInfo->CurrentSpeed = pHBA->pcie_neg_lnk_spd;
	pAdInfo->CurrentLinkWidth = pHBA->pcie_neg_bus_wdth;
	
	pAdInfo->SystemIOBusNumber = pHBA->desc->hba_desc->dev->bus->number;
	pAdInfo->SlotNumber  = PCI_SLOT(pHBA->desc->hba_desc->dev->devfn);
	pAdInfo->InterruptLevel = pHBA->desc->hba_desc->dev->irq;
	pAdInfo->InterruptVector = pHBA->desc->hba_desc->dev->irq;

	Core_Flash_BIOS_Version(pCore,pReq);
	
	pReq->Scsi_Status = REQ_STATUS_SUCCESS;
}

void HBA_ModuleSendRequest(MV_PVOID this, PMV_Request req)
{
	PHBA_Extension phba = (PHBA_Extension) this;

	if (phba->RunAsNonRAID) {
		switch (req->Cdb[0]) {
		case APICDB0_ADAPTER:
			if (req->Cdb[1] == APICDB1_ADAPTER_GETINFO) {
				mvGetAdapterInfo(phba, req);
				req->Completion(req->Cmd_Initiator, req);
			} else if (req->Cdb[1] == APICDB1_ADAPTER_GETCONFIG) {
				mvGetAdapterConfig(phba->p_raid_feature,req);
				req->Completion(req->Cmd_Initiator, req);
			} else if (req->Cdb[1] == APICDB1_ADAPTER_SETCONFIG) {
				mvSetAdapterConfig(phba->p_raid_feature,req);
				req->Completion(req->Cmd_Initiator, req);
			} else {
				req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
				req->Completion(req->Cmd_Initiator, req);
			}
			break;
		case APICDB0_EVENT:
			if (req->Cdb[1] == APICDB1_EVENT_GETEVENT)
				get_event(phba, req);
			else
				req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;

			req->Completion(req->Cmd_Initiator, req);
			break;
		case APICDB0_LD:
		case APICDB0_BLOCK:
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			req->Completion(req->Cmd_Initiator, req);
			break;
		case APICDB0_PD:
		case APICDB0_FLASH:
		case APICDB0_PASS_THRU_CMD_SCSI:
		case APICDB0_PASS_THRU_CMD_ATA:
		case API_SCSI_CMD_RCV_DIAG_RSLT:
		case API_SCSI_CMD_SND_DIAG:
			phba->desc->child->ops->module_sendrequest(
				phba->desc->child->extension,req);
			break;
		case APICDB0_IOCONTROL:
			if (req->Cdb[1] == APICDB1_GET_OS_DISK_INFO) {
				phba->desc->child->ops->module_sendrequest(
					phba->desc->child->extension,req);
			}
			break;
		case SCSI_CMD_REPORT_LUN:
			if (req->Device_Id == VIRTUAL_DEVICE_ID) {
				req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
				req->Completion(req->Cmd_Initiator, req);
			} else {
				phba->desc->child->ops->module_sendrequest(
				phba->desc->child->extension,req);
			}

			break;
		case SCSI_CMD_REQUEST_SENSE:
			if (req->Device_Id == VIRTUAL_DEVICE_ID) {
				sg_map(req);
				if (req->Sense_Info_Buffer!= NULL) {
					((MV_PU8)req->Sense_Info_Buffer)[0] = 0x70;		/* Current */
					((MV_PU8)req->Sense_Info_Buffer)[2] = 0x00;		/* Sense Key*/
					((MV_PU8)req->Sense_Info_Buffer)[7] = 0x00;		/* additional sense length */
					((MV_PU8)req->Sense_Info_Buffer)[12] = 0x00;	/* additional sense code */
				}
				sg_unmap(req);
				req->Scsi_Status = REQ_STATUS_SUCCESS;
				req->Completion(req->Cmd_Initiator, req);
			} else {
				phba->desc->child->ops->module_sendrequest(
				phba->desc->child->extension,req);
			}
			break;
		default:
			phba->desc->child->ops->module_sendrequest(
				phba->desc->child->extension,req);

			break;
		}
	} else {
		switch (req->Cdb[0]) {
		case APICDB0_ADAPTER:
			if (req->Cdb[1] == APICDB1_ADAPTER_GETINFO) {
				mvGetAdapterInfo(phba, req);
				req->Completion(req->Cmd_Initiator, req);
			} else if (req->Cdb[1] == APICDB1_ADAPTER_GETCONFIG) {
				mvGetAdapterConfig(phba->p_raid_feature,req);
				req->Completion(req->Cmd_Initiator, req);
			} else if (req->Cdb[1] == APICDB1_ADAPTER_SETCONFIG) {
				mvSetAdapterConfig(phba->p_raid_feature,req);
				req->Completion(req->Cmd_Initiator, req);
			} else if (req->Cdb[1] == APICDB1_ADAPTER_POWER_STATE_CHANGE){
				phba->desc->child->ops->module_sendrequest(
				phba->desc->child->extension,req);
			} else if (req->Cdb[1] == APICDB1_ADAPTER_MUTE) {
				phba->desc->child->ops->module_sendrequest(
				phba->desc->child->extension,req);
			} else {
				req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
				req->Completion(req->Cmd_Initiator, req);
			}
			break;
		case APICDB0_EVENT:
			if (req->Cdb[1] == APICDB1_EVENT_GETEVENT)
				get_event(phba, req);
			else
				req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;

			req->Completion(req->Cmd_Initiator, req);
			break;
		default:
			phba->desc->child->ops->module_sendrequest(
				phba->desc->child->extension,req);
			break;
		}
	}
}

 MV_BOOLEAN mv_nvram_init_param( MV_PVOID This, pHBA_Info_Page pHBA_Info_Param);

 MV_U32 Core_ModuleGetResourceQuota(enum Resource_Type type, MV_U16 max_io);
 MV_VOID Core_ModuleInitialize(MV_PVOID module, MV_U32 size, MV_U16 max_io);
 MV_VOID Core_ModuleStart(MV_PVOID This);
 MV_VOID Core_ModuleShutdown(MV_PVOID core_p);
 MV_VOID Core_ModuleNotification(MV_PVOID core_p, enum Module_Event event,
	 struct mod_notif_param *param);
 MV_VOID Core_ModuleSendRequest(MV_PVOID core_p, PMV_Request req);
 MV_VOID Core_ModuleReset(MV_PVOID core_p);
 MV_VOID Core_ModuleMonitor(MV_PVOID core_p);
 MV_BOOLEAN Core_InterruptCheckIRQ(MV_PVOID This);

struct mv_module_ops *mv_core_register_module(void)
{
	static struct mv_module_ops __core_mod_ops = {
		.module_id              = MODULE_CORE,
		.get_res_desc           = Core_ModuleGetResourceQuota,
		.module_initialize      = Core_ModuleInitialize,
		.module_start           = Core_ModuleStart,
		.module_stop            = Core_ModuleShutdown,
		.module_notification    = Core_ModuleNotification,
		.module_sendrequest     = Core_ModuleSendRequest,
		.module_reset           = Core_ModuleReset,
		.module_monitor         = Core_ModuleMonitor,
		.module_service_isr 	= Core_InterruptCheckIRQ,
	};

	return &__core_mod_ops;
}

