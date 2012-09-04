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
#include "core_manager.h"
#include "core_type.h"
#include "core_internal.h"
#include "com_struct.h"
#include "com_api.h"
#include "com_error.h"
#include "core_api.h"
#include "core_util.h"
#include "com_flash.h"
#include "hba_inter.h"
#include "core_spi.h"

MV_U8
core_flash_bin(
	IN MV_PVOID core_p,
	IN PMV_Request req)
{
	core_extension  *core = (core_extension *)core_p;
	HBA_Extension   *hba = (PHBA_Extension)HBA_GetModuleExtension(core_p, MODULE_HBA);
	AdapterInfo		adapter_info;
	MV_U32               size = 0,length;
	MV_U32               buf_start_addr = 0,flash_start_addr = 0;
	MV_BOOLEAN       need_erase;
	MV_U8			flash_op,checksum;
	MV_U16			i,erase_sect_num;
	MV_U8			buff[16];
	PFlash_DriveData    data;

        data =(PFlash_DriveData)core_map_data_buffer(req);
	flash_op = 0;
	need_erase = MV_FALSE;
	length = 0;

	if (data == NULL ||
		req->Data_Transfer_Length < sizeof(Flash_DriveData)) {

		core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	switch (req->Cdb[2]) {
	case FLASH_UPLOAD:
		if (data->PageNumber == 0 && data->Data !=NULL)
			need_erase = MV_TRUE;
		else if(data->PageNumber>0)
			need_erase = MV_FALSE;
		flash_op = 1;
		size = data->Size;
		data->Size = 0;
		CORE_DPRINT(("Upload!\n"));
		break;
	case FLASH_DOWNLOAD:
		flash_op = 2;
		CORE_DPRINT(("Download!\n"));
		break;
	default:
		req->Scsi_Status = REQ_STATUS_INVALID_PARAMETER;
		CORE_DPRINT(("Unknown action!\n"));
	}

	adapter_info = *(AdapterInfo *)core->lib_flash.adapter_info;

	switch (req->Cdb[3]) {
	case  FLASH_TYPE_BIOS:
		buf_start_addr = 0x00000000;
		erase_sect_num = (MV_U16)(MAX_BIOS_SIZE /adapter_info.FlashSectSize);
		CORE_DPRINT(("BIOS Type!\n"));
		break;
	case  FLASH_TYPE_CONFIG:
		buf_start_addr = 0x30000;
		erase_sect_num = 1;
		CORE_DPRINT(("CONFIG Type!\n"));
		break;
	default:
		req->Scsi_Status=REQ_STATUS_INVALID_PARAMETER;
		CORE_DPRINT(("Unknown Type!\n"));
		core_unmap_data_buffer(req);
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	if (req->Cdb[3] == FLASH_TYPE_CONFIG) {
		if(need_erase) {
			flash_start_addr = buf_start_addr;
			if(OdinSPI_SectErase( &adapter_info, flash_start_addr) != -1){
				CORE_DPRINT(("Core_Flash_Bin: FLASH ERASE SUCCESS!Erase Sector\n"));
			} else {
				CORE_DPRINT(("Core_Flash_Bin: FLASH ERASE FAILED!Erase Sector\n"));
				req->Scsi_Status = REQ_STATUS_ERROR;
				return MV_FALSE;
			}
		}

	if (flash_op==1){
		checksum = mvCalculateChecksum(data->Data, size);
		flash_start_addr = buf_start_addr + data->PageNumber * DRIVER_LENGTH;

		if (OdinSPI_WriteBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data, size) == -1) {
			CORE_DPRINT(("Core_Flash_Bin: Write FLASH FAILED!PageNumber is %d\n",data->PageNumber));
			req->Scsi_Status=REQ_STATUS_ERROR;
			return MV_FALSE;
		} else {
			CORE_DPRINT(("Core_Flash_Bin: Write FLASH SUCCESS!PageNumber is %d,address is %x\n",\
				data->PageNumber,flash_start_addr));
		}

		MV_ZeroMemory(data->Data,size);
		if (OdinSPI_ReadBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data, size) == -1) {
			CORE_DPRINT(("Core_Flash_Bin: Read FLASH FAILED!Address is %x\n",flash_start_addr));
			req->Scsi_Status = REQ_STATUS_ERROR;
			return MV_FALSE;
		} else {
			CORE_DPRINT(("Core_Flash_Bin: Read FLASH SUCCESS!Address is %x\n",flash_start_addr));
		}

		/* check the sum and return the writen size */
		if (checksum == mvCalculateChecksum(data->Data, size)){
			data->Size = (MV_U16)size;
			req->Scsi_Status = REQ_STATUS_SUCCESS;
			CORE_DPRINT(("Core_Flash_Bin: CheckSum is OK!\n"));
		}
	}

	if (flash_op == 2){
		length = 64*1024;
		flash_start_addr = 0;

		/* calculate the page size */
		if((MV_U32)((data->PageNumber + 1) * DRIVER_LENGTH)< length)
		{
			/* read the whole page */
			size = DRIVER_LENGTH;
			data->Size=0;
			flash_start_addr = buf_start_addr + data->PageNumber * DRIVER_LENGTH;
			if (OdinSPI_ReadBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data, size) == -1) {
				CORE_DPRINT(("Core_Flash_Bin: Read FLASH FAILED!Address is %x\n",flash_start_addr));
				req->Scsi_Status=REQ_STATUS_ERROR;
				return MV_FALSE;
			}
			data->Size=(MV_U16)size;
			data->PageNumber++;
		} else {
			size=(MV_U16)(length - data->PageNumber * DRIVER_LENGTH);
			if(size != 0)
			{
				data->Size = 0;
				flash_start_addr = buf_start_addr + data->PageNumber * DRIVER_LENGTH;
				if (OdinSPI_ReadBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data, size) == -1) {
					CORE_DPRINT(("Core_Flash_Bin: Read FLASH FAILED!Address is %x\n",flash_start_addr));
					req->Scsi_Status = REQ_STATUS_ERROR;
					return MV_FALSE;
				}
			}
			data->Size = (MV_U16)size;
			data->isLastPage = MV_TRUE;
		}
	}
    }else if(req->Cdb[3] == FLASH_TYPE_BIOS){
	/* erase flash */
	if (need_erase) {
		for (i = 0; i < erase_sect_num; i++) {
			flash_start_addr = buf_start_addr + i * adapter_info.FlashSectSize;
			if (OdinSPI_SectErase(&adapter_info, flash_start_addr) != -1) {
				CORE_DPRINT(("FLASH ERASE SUCCESS!Erase Sector %d\n", i));
			} else {
				CORE_DPRINT(("FLASH ERASE FAILED!Erase Sector %d\n", i));
				req->Scsi_Status = REQ_STATUS_ERROR;
                                core_unmap_data_buffer(req);
				return MV_QUEUE_COMMAND_RESULT_FINISHED;
			}
		}
	}

	/* write flash */
	if (flash_op == 1) {
		/*get CheckSum */
		checksum  = mvCalculateChecksum(data->Data, size);
		flash_start_addr = buf_start_addr + data->PageNumber * DRIVER_LENGTH;
		if (OdinSPI_WriteBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data, size) == -1) {
			CORE_DPRINT(("Write FLASH FAILED!PageNumber is %d\n",data->PageNumber));
			req->Scsi_Status = REQ_STATUS_ERROR;
                        core_unmap_data_buffer(req);
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		} else {
			CORE_DPRINT(("Write FLASH SUCCESS!PageNumber is %d,address is %x\n",\
                                data->PageNumber, flash_start_addr));
		}

		MV_ZeroMemory(data->Data, size);
		if (OdinSPI_ReadBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data, size) == -1) {
			CORE_DPRINT(("Read FLASH FAILED!Address is %x\n", flash_start_addr));
			req->Scsi_Status = REQ_STATUS_ERROR;
                        core_unmap_data_buffer(req);
			return MV_QUEUE_COMMAND_RESULT_FINISHED;
		} else {
			CORE_DPRINT(("Read FLASH SUCCESS!Address is %x\n", flash_start_addr));
		}

		/* check the sum and return the writen size */
		if (checksum != mvCalculateChecksum(data->Data, size)){
			CORE_DPRINT(("The Checksum is wrong! \n"));
		} else {
			data->Size = (MV_U16)size;
			CORE_DPRINT(("CheckSum is OK!\n"));
		}
	}

		if (flash_op == 2) {
			/* calculate the bios size */
			flash_start_addr = 0;
			if (OdinSPI_ReadBuf(&adapter_info, flash_start_addr, buff, 16) == -1) {
				CORE_DPRINT(("Read FLASH FAILED!Address is %x\n", flash_start_addr));
				req->Scsi_Status = REQ_STATUS_ERROR;
				core_unmap_data_buffer(req);
				return MV_QUEUE_COMMAND_RESULT_FINISHED;
			}

			length=buff[2];

			flash_start_addr = 0x140;
			if (OdinSPI_ReadBuf(&adapter_info, flash_start_addr, buff, 16) == -1) {
				CORE_DPRINT(("Read FLASH FAILED!Address is %x\n",flash_start_addr));
				req->Scsi_Status = REQ_STATUS_ERROR;
				core_unmap_data_buffer(req);
				return MV_QUEUE_COMMAND_RESULT_FINISHED;
			}

			length += buff[0];
			length += buff[1];
			length *= 512;

			flash_start_addr = 0;
			buf_start_addr = 0;

			/* calculate the page size */
			if ((MV_U32)((data->PageNumber + 1) * DRIVER_LENGTH) <= length) {
				/* read the whole page. */
				size = DRIVER_LENGTH;
				data->Size = 0;
				flash_start_addr = buf_start_addr + data->PageNumber * DRIVER_LENGTH;
				if (OdinSPI_ReadBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data, size) == -1) {
					CORE_DPRINT(("Read FLASH FAILED!Address is %x\n",flash_start_addr));
					req->Scsi_Status = REQ_STATUS_ERROR;
					core_unmap_data_buffer(req);
					return MV_QUEUE_COMMAND_RESULT_FINISHED;
				}
				data->Size=(MV_U16)size;
				data->PageNumber++;
			} else { 
				/* read the last page */
				size = (MV_U16)(length-data->PageNumber * DRIVER_LENGTH);
				data->Size = 0;
				flash_start_addr = buf_start_addr+data->PageNumber*DRIVER_LENGTH;
				if (OdinSPI_ReadBuf(&adapter_info, flash_start_addr, (MV_U8*)data->Data, size) == -1) {
					CORE_DPRINT(("Read FLASH FAILED!Address is %x\n",flash_start_addr));
					req->Scsi_Status = REQ_STATUS_ERROR;
					core_unmap_data_buffer(req);
					return MV_QUEUE_COMMAND_RESULT_FINISHED;
				}
				data->Size = (MV_U16)size;
				data->isLastPage = MV_TRUE;
			}
		}
	}

	req->Scsi_Status = REQ_STATUS_SUCCESS;
	core_unmap_data_buffer(req);
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_U8
core_flash_rwtest(
	IN MV_PVOID core_p,
	IN PMV_Request req)
{
	core_extension  *core = (core_extension *)core_p;
	HBA_Extension   *hba = (PHBA_Extension)HBA_GetModuleExtension(core_p, MODULE_HBA);
	AdapterInfo		adapter_info;
	MV_U32               bytes = 0,offset = 0;
	MV_BOOLEAN		need_erase;
	MV_U8			flash_op,checksum;
	PDBG_Flash    data;

	data = (PDBG_Flash)core_map_data_buffer(req);
	need_erase = MV_FALSE;
	flash_op = 0;

	if (data == NULL ||
		req->Data_Transfer_Length < sizeof(DBG_Flash)) {

		core_unmap_data_buffer(req);
		req->Scsi_Status = REQ_STATUS_INVALID_REQUEST;
		return MV_QUEUE_COMMAND_RESULT_FINISHED;
	}

	switch (req->Cdb[2]) {	
		case FLASH_BYTE_WRITE:
				flash_op = 1;
				bytes = data->NumBytes;
				offset = data->OffSet;
				if (!(offset % FLASH_SECTOR_SIZE)){
					need_erase = MV_TRUE;
				}
				CORE_DPRINT(("Write Flash Test!\n"));
				break;
		case FLASH_BYTE_READ:
				flash_op = 2;
				bytes = data->NumBytes;
				CORE_DPRINT(("Read Flash Test!\n"));
				break;
		default:
			req->Scsi_Status = REQ_STATUS_INVALID_PARAMETER;
			CORE_DPRINT(("Unknown action!\n"));
	}
	
	adapter_info = *(AdapterInfo *)core->lib_flash.adapter_info;

	if(need_erase) {
		if(OdinSPI_SectErase( &adapter_info,  (offset/FLASH_SECTOR_SIZE)*FLASH_SECTOR_SIZE) != -1){
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: FLASH ERASE SUCCESS!Erase Sector %d\n", offset/FLASH_SECTOR_SIZE));
		} else {
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: FLASH ERASE FAILED!Erase Sector %d\n", offset/FLASH_SECTOR_SIZE));
			req->Scsi_Status = REQ_STATUS_ERROR;
			return MV_FALSE;
		}
	}
	
	/* write flash */
	if (flash_op==1) {
		checksum = mvCalculateChecksum(data->Data, bytes);
		
		if (OdinSPI_WriteBuf(&adapter_info, offset, (MV_U8*)data->Data, bytes) != -1) {
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: Write FLASH SUCCESS!\n"));
		} else {			
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: Write FLASH FAILED!\n"));
			req->Scsi_Status = REQ_STATUS_ERROR;
			core_unmap_data_buffer(req);
			return MV_FALSE;
		}
		
		bytes = data->NumBytes;
		MV_ZeroMemory(data->Data, data->NumBytes);
		if (OdinSPI_ReadBuf(&adapter_info, data->OffSet, (MV_U8*)data->Data, bytes) != -1) {
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: Read FLASH SUCCESS!Address is %x\n", data->OffSet));
		} else {
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: Read FLASH FAILED!Address is %x\n", data->OffSet));
			req->Scsi_Status = REQ_STATUS_ERROR;
			core_unmap_data_buffer(req);
			return MV_FALSE;
		}
		
		if (checksum == mvCalculateChecksum(data->Data, data->NumBytes)) {
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: CheckSum is OK!\n"));
		} else	
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: CheckSum Failed!\n"));
	}

	if (flash_op==2) {
		if(OdinSPI_ReadBuf(&adapter_info, data->OffSet, (MV_U8*)data->Data, bytes) != -1) {
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: Read FLASH SUCCESS!Address is %x\n", data->OffSet));
		} else {
			CORE_DPRINT(("Core_Flash_ReadWrite_Test: Read FLASH FAILED!Address is %x\n", data->OffSet));
			req->Scsi_Status = REQ_STATUS_ERROR;
			core_unmap_data_buffer(req);
			return MV_FALSE;
		}
	}
	
	req->Scsi_Status = REQ_STATUS_SUCCESS;
	core_unmap_data_buffer(req);
	return MV_QUEUE_COMMAND_RESULT_FINISHED;
}

MV_U8
core_flash_command(
	IN MV_PVOID core_p,
	IN PMV_Request p_req
	)
{
	MV_U8 status = MV_QUEUE_COMMAND_RESULT_FINISHED;

	switch (p_req->Cdb[1]) {
		case APICDB1_FLASH_BIN:
			status = core_flash_bin(core_p, p_req);
			break;
		case APICDB1_TEST_FLASH:
			status = core_flash_rwtest(core_p, p_req);
			break;
		default:
			p_req->Scsi_Status = REQ_STATUS_INVALID_PARAMETER;
			break;
	}
	return status;
}

