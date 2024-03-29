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
#include "com_define.h"
#include "com_dbg.h"
#include "com_scsi.h"
#include "com_util.h"
#include "com_u64.h"
#include "hba_exp.h"
#include "com_extern.h"
void MV_ZeroMvRequest(PMV_Request pReq)
{
	PMV_SG_Entry pSGEntry;
	MV_U16 maxEntryCount;
	MV_PVOID pSenseBuffer;
	List_Head list;

	MV_DASSERT(pReq);
	pSGEntry = pReq->SG_Table.Entry_Ptr;
	maxEntryCount = pReq->SG_Table.Max_Entry_Count;
	pSenseBuffer = pReq->Sense_Info_Buffer;
	list = pReq->pool_entry;

	MV_ZeroMemory(pReq, MV_REQUEST_SIZE);
 	pReq->pool_entry = list;
	pReq->SG_Table.Entry_Ptr = pSGEntry;
	pReq->SG_Table.Max_Entry_Count = maxEntryCount;
	pReq->Sense_Info_Buffer = pSenseBuffer;
}

void MV_CopySGTable(PMV_SG_Table pTargetSGTable, PMV_SG_Table pSourceSGTable)
{
	MV_ASSERT(pTargetSGTable->Max_Entry_Count >= pSourceSGTable->Valid_Entry_Count);
	pTargetSGTable->Valid_Entry_Count = pSourceSGTable->Valid_Entry_Count;
	pTargetSGTable->Occupy_Entry_Count = pSourceSGTable->Occupy_Entry_Count;
	pTargetSGTable->Flag = pSourceSGTable->Flag;
	pTargetSGTable->Byte_Count = pSourceSGTable->Byte_Count;
	MV_CopyMemory(pTargetSGTable->Entry_Ptr, pSourceSGTable->Entry_Ptr,
					sizeof(MV_SG_Entry)*pTargetSGTable->Valid_Entry_Count);
}

void MV_CopyPartialSGTable(
	OUT PMV_SG_Table pTargetSGTable,
	IN PMV_SG_Table pSourceSGTable,
	IN MV_U32 offset,
	IN MV_U32 size
	)
{
	MV_U16  count = pTargetSGTable->Max_Entry_Count;
	sgd_t *entry = pTargetSGTable->Entry_Ptr;

	MV_DASSERT( (count>0) && (entry!=NULL) );
	sgd_table_init( pTargetSGTable, count, entry);
	sgdt_append_reftbl(pTargetSGTable, pSourceSGTable, offset, size);
}

MV_BOOLEAN MV_Equals(
	IN MV_PU8		des,
	IN MV_PU8		src,
	IN MV_U32		len
)
{
	MV_U32 i;

	for (i=0; i<len; i++) {
		if (*des != *src)
			return MV_FALSE;
		des++;
		src++;
	}
	return MV_TRUE;
}
/*
 * SG Table operation
 */
void SGTable_Init(
	OUT PMV_SG_Table pSGTable,
	IN MV_U8 flag
	)
{
	pSGTable->Valid_Entry_Count = 0;
	pSGTable->Flag = flag;
	pSGTable->Byte_Count = 0;
	pSGTable->Occupy_Entry_Count = 0;
}

void sgt_init(
       IN MV_U16 max_io,
	OUT PMV_SG_Table pSGTable,
	IN MV_U8 flag
	)
{
	if (max_io == 1)
		pSGTable->Max_Entry_Count = MAX_SG_ENTRY_REDUCED;
	else
		pSGTable->Max_Entry_Count = MAX_SG_ENTRY;

	pSGTable->Valid_Entry_Count = 0;
	pSGTable->Flag = flag;
	pSGTable->Byte_Count = 0;
	pSGTable->Occupy_Entry_Count = 0;
}

MV_BOOLEAN SGTable_Available(
	IN PMV_SG_Table pSGTable
	)
{
	return (pSGTable->Valid_Entry_Count < pSGTable->Max_Entry_Count);
}

void MV_InitializeTargetIDTable(
	IN PMV_Target_ID_Map pMapTable
	)
{
	MV_FillMemory((MV_PVOID)pMapTable, sizeof(MV_Target_ID_Map)*MV_MAX_TARGET_NUMBER, 0xFF);
}

MV_U16 MV_GetTargetID(IN PMV_Target_ID_Map	pMapTable,IN MV_U8 deviceType)
{
	MV_U16 i;
	for (i=PORT_NUMBER; i < MV_MAX_TARGET_NUMBER; i++) {

		if ( (pMapTable[i].Type==0xff) && (pMapTable[i].Device_Id==0xffff) ) {
			return i;
		}
	}
	return 0xffff;
}

MV_U16 MV_MapTargetID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	)
{
	MV_U16 i=0xffff;

	if(deviceType==TARGET_TYPE_FREE_PD){
		if (pMapTable[deviceId].Type==0xFF) {	/* not mapped yet */
			pMapTable[deviceId].Device_Id = deviceId;
			pMapTable[deviceId].Type = deviceType;			
			
		} else{	
			i=MV_GetTargetID(pMapTable,deviceType);

			pMapTable[i].Device_Id = deviceId;
			pMapTable[i].Type = deviceType;
			deviceId=i;
		}
		i=deviceId;
	}else if(deviceType==TARGET_TYPE_LD){
			for (i=PORT_NUMBER; i<MV_MAX_TARGET_NUMBER; i++) {
				if (pMapTable[i].Type==0xFF) {	/* not mapped yet */
					pMapTable[i].Device_Id = deviceId;
					pMapTable[i].Type = deviceType;
					break;
					}
			}
	}

	return i;
}

MV_U16 MV_MapToSpecificTargetID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				specificId,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	)
{
	/* first check if the device can be mapped to the specific ID */
	if (specificId < MV_MAX_TARGET_NUMBER) {
		if (pMapTable[specificId].Type==0xFF) {	/* not used yet */
			pMapTable[specificId].Device_Id = deviceId;
			pMapTable[specificId].Type = deviceType;
			return specificId;
		}
	}
	/* cannot mapped to the specific ID */
	/* just map the device to first available ID */
	return MV_MapTargetID(pMapTable, deviceId, deviceType);
}

MV_U16 MV_RemoveTargetID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	)
{
	MV_U16 i;
	for (i=0; i < MV_MAX_TARGET_NUMBER; i++) {
		if ( (pMapTable[i].Type==deviceType) && (pMapTable[i].Device_Id==deviceId) ) {
			pMapTable[i].Type = 0xFF;
			pMapTable[i].Device_Id = 0xFFFF;
			break;
		}
	}
	if (i == MV_MAX_TARGET_NUMBER)
		i = 0xFFFF;
	return i;
}

MV_U16 MV_GetMappedID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	)
{
	MV_U16 mappedID;

	for (mappedID=0; mappedID<MV_MAX_TARGET_NUMBER; mappedID++) {
		if ( (pMapTable[mappedID].Type==deviceType) && (pMapTable[mappedID].Device_Id==deviceId) ){			
			break;
			}
		
	}
	if (mappedID >= MV_MAX_TARGET_NUMBER)
		mappedID = 0xFFFF;
	else {
		MV_DASSERT(mappedID < MV_MAX_TARGET_NUMBER);
		if (mappedID == VIRTUAL_DEVICE_ID) {
			/* Device is on LUN 1 */
			mappedID |= 0x0100;
		}
	}
	return mappedID;
}

void MV_DecodeReadWriteCDB(
	IN MV_PU8 Cdb,
	OUT MV_LBA *pLBA,
	OUT MV_U32 *pSectorCount)
{
	MV_LBA tmpLBA;
	MV_U32 tmpSectorCount;

	if ((!SCSI_IS_READ(Cdb[0])) &&
	    (!SCSI_IS_WRITE(Cdb[0])) &&
	    (!SCSI_IS_VERIFY(Cdb[0])))
		return;

	/* This is READ/WRITE command */
	switch (Cdb[0]) {
	case SCSI_CMD_READ_6:
	case SCSI_CMD_WRITE_6:
		tmpLBA.value = (MV_U32)((((MV_U32)(Cdb[1] & 0x1F))<<16) |
					((MV_U32)Cdb[2]<<8) |
					((MV_U32)Cdb[3]));
		tmpSectorCount = (MV_U32)Cdb[4];
		break;
	case SCSI_CMD_READ_10:
	case SCSI_CMD_WRITE_10:
	case SCSI_CMD_VERIFY_10:
		tmpLBA.value = (MV_U32)(((MV_U32)Cdb[2]<<24) |
					((MV_U32)Cdb[3]<<16) |
					((MV_U32)Cdb[4]<<8) |
					((MV_U32)Cdb[5]));
		tmpSectorCount = ((MV_U32)Cdb[7]<<8) | (MV_U32)Cdb[8];
		break;
	case SCSI_CMD_READ_12:
	case SCSI_CMD_WRITE_12:
		tmpLBA.value = (MV_U32)(((MV_U32)Cdb[2]<<24) |
					((MV_U32)Cdb[3]<<16) |
					((MV_U32)Cdb[4]<<8) |
					((MV_U32)Cdb[5]));
		tmpSectorCount = (MV_U32)(((MV_U32)Cdb[6]<<24) |
					  ((MV_U32)Cdb[7]<<16) |
					  ((MV_U32)Cdb[8]<<8) |
					  ((MV_U32)Cdb[9]));
		break;
	case SCSI_CMD_READ_16:
	case SCSI_CMD_WRITE_16:
	case SCSI_CMD_VERIFY_16:
		tmpLBA.parts.high = (MV_U32)(((MV_U32)Cdb[2]<<24) |
				       ((MV_U32)Cdb[3]<<16) |
				       ((MV_U32)Cdb[4]<<8) |
				       ((MV_U32)Cdb[5]));
		tmpLBA.parts.low = (MV_U32)(((MV_U32)Cdb[6]<<24) |
				      ((MV_U32)Cdb[7]<<16) |
				      ((MV_U32)Cdb[8]<<8) |
				      ((MV_U32)Cdb[9]));

		tmpSectorCount = (MV_U32)(((MV_U32)Cdb[10]<<24) |
					  ((MV_U32)Cdb[11]<<16) |
					  ((MV_U32)Cdb[12]<<8) |
					  ((MV_U32)Cdb[13]));
		break;
	default:
		MV_DPRINT(("Unsupported READ/WRITE command [%x]\n", Cdb[0]));
		U64_SET_VALUE(tmpLBA, 0);
		tmpSectorCount = 0;
	}
	*pLBA = tmpLBA;
	*pSectorCount = tmpSectorCount;
}

void MV_CodeReadWriteCDB(
	OUT MV_PU8	Cdb,
	IN MV_LBA	lba,
	IN MV_U32	sector,
	IN MV_U8	operationCode	/* The CDB[0] */
	)
{
	MV_DASSERT(
		SCSI_IS_READ(operationCode)
		|| SCSI_IS_WRITE(operationCode)
		|| SCSI_IS_VERIFY(operationCode)
		);

	MV_ZeroMemory(Cdb, MAX_CDB_SIZE);
	Cdb[0] = operationCode;

	/* This is READ/WRITE command */
	switch ( Cdb[0]) {
	case SCSI_CMD_READ_6:
	case SCSI_CMD_WRITE_6:
		Cdb[1] = (MV_U8)(lba.value>>16);
		Cdb[2] = (MV_U8)(lba.value>>8);
		Cdb[3] = (MV_U8)lba.value;
		Cdb[4] = (MV_U8)sector;
		break;
	case SCSI_CMD_READ_10:
	case SCSI_CMD_WRITE_10:
	case SCSI_CMD_VERIFY_10:
		Cdb[2] = (MV_U8)(lba.value>>24);
		Cdb[3] = (MV_U8)(lba.value>>16);
		Cdb[4] = (MV_U8)(lba.value>>8);
		Cdb[5] = (MV_U8)lba.value;
		Cdb[7] = (MV_U8)(sector>>8);
		Cdb[8] = (MV_U8)sector;
		break;
	case SCSI_CMD_READ_12:
	case SCSI_CMD_WRITE_12:
		Cdb[2] = (MV_U8)(lba.value>>24);
		Cdb[3] = (MV_U8)(lba.value>>16);
		Cdb[4] = (MV_U8)(lba.value>>8);
		Cdb[5] = (MV_U8)lba.value;
		Cdb[6] = (MV_U8)(sector>>24);
		Cdb[7] = (MV_U8)(sector>>16);
		Cdb[8] = (MV_U8)(sector>>8);
		Cdb[9] = (MV_U8)sector;
		break;
	case SCSI_CMD_READ_16:
	case SCSI_CMD_WRITE_16:
	case SCSI_CMD_VERIFY_16:
		Cdb[2] = (MV_U8)(lba.parts.high>>24);
		Cdb[3] = (MV_U8)(lba.parts.high>>16);
		Cdb[4] = (MV_U8)(lba.parts.high>>8);
		Cdb[5] = (MV_U8)lba.parts.high;
		Cdb[6] = (MV_U8)(lba.parts.low>>24);
		Cdb[7] = (MV_U8)(lba.parts.low>>16);
		Cdb[8] = (MV_U8)(lba.parts.low>>8);
		Cdb[9] = (MV_U8)lba.parts.low;
		Cdb[10] = (MV_U8)(sector>>24);
		Cdb[11] = (MV_U8)(sector>>16);
		Cdb[12] = (MV_U8)(sector>>8);
		Cdb[13] = (MV_U8)sector;
		break;
	default:
		MV_DPRINT(("Unsupported READ/WRITE command [%x]\n", Cdb[0]));
		MV_ASSERT(0);
	}
}

void MV_DumpRequest(PMV_Request pReq, MV_BOOLEAN detail)
{
	MV_DPRINT(("Device %d MV_Request: Cdb[%02x,%02x,%02x,%02x, %02x,%02x,%02x,%02x, %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x].\n",
		pReq->Device_Id,
		pReq->Cdb[0],
		pReq->Cdb[1],
		pReq->Cdb[2],
		pReq->Cdb[3],
		pReq->Cdb[4],
		pReq->Cdb[5],
		pReq->Cdb[6],
		pReq->Cdb[7],
		pReq->Cdb[8],
		pReq->Cdb[9],
		pReq->Cdb[10],
		pReq->Cdb[11],
		pReq->Cdb[12],
		pReq->Cdb[13],
		pReq->Cdb[14],
		pReq->Cdb[15]
		));

	if (detail) {
		MV_DPRINT(("Req Flag=0x%x\n", pReq->Cmd_Flag));
		MV_DPRINT(("Scsi_Status=0x%x\n", pReq->Scsi_Status));
		MV_DPRINT(("Tag=0x%x\n", pReq->Tag));
		MV_DPRINT(("Data_Transfer_Length=0x%x\n", pReq->Data_Transfer_Length));
		MV_DPRINT(("Sense_Info_Buffer_Length=%d\n", pReq->Sense_Info_Buffer_Length));
		MV_DPRINT(("Org_Req : %p\n", pReq->Org_Req));
	}
}

void MV_DumpSGTable(PMV_SG_Table pSGTable)
{
	sgdt_dump(pSGTable, " ");
}

const char* MV_DumpSenseKey(MV_U8 sense)
{
	switch ( sense )
	{
		case SCSI_SK_NO_SENSE:
			return "SCSI_SK_NO_SENSE";
		case SCSI_SK_RECOVERED_ERROR:
			return "SCSI_SK_RECOVERED_ERROR";
		case SCSI_SK_NOT_READY:
			return "SCSI_SK_NOT_READY";
		case SCSI_SK_MEDIUM_ERROR:
			return "SCSI_SK_MEDIUM_ERROR";
		case SCSI_SK_HARDWARE_ERROR:
			return "SCSI_SK_HARDWARE_ERROR";
		case SCSI_SK_ILLEGAL_REQUEST:
			return "SCSI_SK_ILLEGAL_REQUEST";
		case SCSI_SK_UNIT_ATTENTION:
			return "SCSI_SK_UNIT_ATTENTION";
		case SCSI_SK_DATA_PROTECT:
			return "SCSI_SK_DATA_PROTECT";
		case SCSI_SK_BLANK_CHECK:
			return "SCSI_SK_BLANK_CHECK";
		case SCSI_SK_VENDOR_SPECIFIC:
			return "SCSI_SK_VENDOR_SPECIFIC";
		case SCSI_SK_COPY_ABORTED:
			return "SCSI_SK_COPY_ABORTED";
		case SCSI_SK_ABORTED_COMMAND:
			return "SCSI_SK_ABORTED_COMMAND";
		case SCSI_SK_VOLUME_OVERFLOW:
			return "SCSI_SK_VOLUME_OVERFLOW";
		case SCSI_SK_MISCOMPARE:
			return "SCSI_SK_MISCOMPARE";
		default:
			MV_DPRINT(("Unknown sense key 0x%x.\n", sense));
			return "Unknown sense key";
	}
}

static MV_U32  BASEATTR crc_tab[] = {
        0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
        0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
        0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
        0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
        0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
        0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
        0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
        0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
        0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
        0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
        0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
        0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
        0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
        0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
        0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
        0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
        0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
        0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
        0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
        0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
        0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
        0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
        0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
        0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
        0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
        0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
        0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
        0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
        0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
        0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
        0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
        0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
        0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
        0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
        0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
        0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
        0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
        0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
        0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
        0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
        0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
        0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
        0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
        0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
        0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
        0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
        0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
        0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
        0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
        0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
        0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
        0x2d02ef8dL
};

/* Calculate CRC and generate PD_Reference number */
MV_U32 MV_CRC(
	IN	MV_PU8		pData,
	IN	MV_U16		len
)
{
	MV_U16 i;
	MV_U32 crc = MV_MAX_U32;

	for (i = 0;  i < len;  i ++) {
		crc = crc_tab[(crc ^ pData[i]) & 0xff] ^ (crc >> 8);
	}
	return MV_CPU_TO_BE32(crc);
}

MV_U32 MV_CRC_EXT(
	IN	MV_U32		crc,
	IN	MV_PU8		pData,
	IN	MV_U32		len
)
{
	MV_U16 i;

	for (i = 0;  i < len;  i ++) {
		crc = crc_tab[(crc ^ pData[i]) & 0xff] ^ (crc >> 8);
	}
	return crc;
}

MV_VOID init_target_id_map(MV_U16 *map_table, MV_U32 size)
{
	MV_FillMemory(map_table, size, 0xFFFF);
}

MV_U16 get_available_target_id(MV_U16 *map_table, MV_U16 id)
{
	MV_U16 i;

	for (i=0; i<id; i++) {
		if (map_table[i] == 0xffff)
			return i;
	}
	return id;

}
MV_U16 add_target_map(MV_U16 *map_table, MV_U16 device_id,MV_U16 max_id)
{
	MV_U16 target_id;

	target_id=device_id;
	if(target_id >= max_id)
		return 0xFFFF;
	map_table[target_id] = device_id;

	return target_id;
}

MV_U16 remove_target_map(MV_U16 *map_table, MV_U16 target_id, MV_U16 max_id)
{
	if((target_id != 0xFFFF) && (target_id >= max_id))
		return target_id;
	MV_FillMemory(&map_table[target_id], sizeof(MV_U16), 0xFFFF);
	return target_id;

}

