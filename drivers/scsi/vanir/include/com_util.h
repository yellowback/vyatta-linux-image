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
#ifndef __MV_COM_UTIL_H__
#define __MV_COM_UTIL_H__

#include "com_define.h"
#include "com_type.h"

#define MV_ZeroMemory(buf, len)           ossw_memset(buf, 0, len)
#define MV_FillMemory(buf, len, pattern)  ossw_memset(buf, pattern, len)
#define MV_CopyMemory(dest, source, len)  ossw_memcpy(dest, source, len)
#define MV_CompareMemory(buf0, buf1, len)  ossw_memcmp(buf0, buf1, len)

 /* ffz - find first zero in word. define in linux kernel bitops.h*/
#define ffc(v)  ossw_ffz(v) 
#define _rotr(v, i)		ossw_rotr32(v, i)
/*ffs - normally return from 1 to MSB, if not find set bit, return 0*/
#define fc(v, i) (ossw_ffs(~(_rotr(v, i))) + i)

static __inline int
ffc64(MV_U64 v)
{
    int i;

    if ((i = ffc(v.parts.low)) >= 0) {
        return i;
    }

    if ((i = ffc(v.parts.high)) >= 0) {
        return 32 + i;
    }

	return -1;
}

void MV_ZeroMvRequest(PMV_Request pReq);
void MV_CopySGTable(PMV_SG_Table pTargetSGTable, PMV_SG_Table pSourceSGTable);
/* offset and size are all byte count. */
void MV_CopyPartialSGTable(PMV_SG_Table pTargetSGTable, PMV_SG_Table pSourceSGTable, MV_U32 offset, MV_U32 size);

MV_BOOLEAN MV_Equals(MV_PU8 des, MV_PU8 src, MV_U32 len);

#define	U64_ASSIGN(x,y)				  	((x).value = (y))
#define	U64_ASSIGN_U64(x,y)			  	((x).value = (y).value)
#define	U64_COMP_U64(x,y)			  	((x) == (y).value)
#define U64_COMP_U64_VALUE(x,y)			((x).value == (y).value)
#define U32_ASSIGN_U64(v64, v32)		((v64).value = (v32))
#define	U64_SHIFT_LEFT(v64, v32)		((v64).value=(v64).value << (v32))
#define	U64_SHIFT_RIGHT(v64, v32)		((v64).value=(v64).value >> (v32))
#define U64_ZERO_VALUE(v64)				((v64).value = 0)

#define MV_SWAP_32(x)                             \
           (((MV_U32)((MV_U8)(x)))<<24 |          \
            ((MV_U32)((MV_U8)((x)>>8)))<<16 |     \
            ((MV_U32)((MV_U8)((x)>>16)))<<8 |     \
            ((MV_U32)((MV_U8)((x)>>24))) )
#define MV_SWAP_64(x)                             \
           (((_MV_U64) (MV_SWAP_32((x).parts.low))) << 32 | \
	    MV_SWAP_32((x).parts.high))
#define MV_SWAP_16(x)                             \
           (((MV_U16) ((MV_U8) (x))) << 8 |       \
	    (MV_U16) ((MV_U8) ((x) >> 8)))

#define MV_CPU_TO_LE16      ossw_cpu_to_le16
#define MV_CPU_TO_LE32      ossw_cpu_to_le32
#define MV_CPU_TO_LE64(x)   ossw_cpu_to_le64((x).value)
#define MV_CPU_TO_BE16      ossw_cpu_to_be16
#define MV_CPU_TO_BE32      ossw_cpu_to_be32
#define MV_CPU_TO_BE64(x)   ossw_cpu_to_be64((x).value)

#define MV_LE16_TO_CPU      ossw_le16_to_cpu
#define MV_LE32_TO_CPU      ossw_le32_to_cpu
#define MV_LE64_TO_CPU(x)   ossw_le64_to_cpu((x).value)
#define MV_BE16_TO_CPU      ossw_be16_to_cpu
#define MV_BE32_TO_CPU      ossw_be32_to_cpu
#define MV_BE64_TO_CPU(x)   ossw_be64_to_cpu((x).value)

/*
 * big endian bit-field structs that are larger than a single byte
 * need swapping
 */
#ifdef __MV_BIG_ENDIAN__
#define MV_CPU_TO_LE16_PTR(pu16)        \
   *((MV_PU16)(pu16)) = MV_CPU_TO_LE16(*(MV_PU16) (pu16))
#define MV_CPU_TO_LE32_PTR(pu32)        \
   *((MV_PU32)(pu32)) = MV_CPU_TO_LE32(*(MV_PU32) (pu32))

#define MV_LE16_TO_CPU_PTR(pu16)        \
   *((MV_PU16)(pu16)) = MV_LE16_TO_CPU(*(MV_PU16) (pu16))
#define MV_LE32_TO_CPU_PTR(pu32)        \
   *((MV_PU32)(pu32)) = MV_LE32_TO_CPU(*(MV_PU32) (pu32))
# else  /* __MV_BIG_ENDIAN__ */
#define MV_CPU_TO_LE16_PTR(pu16)        /* Nothing */
#define MV_CPU_TO_LE32_PTR(pu32)        /* Nothing */
#define MV_LE16_TO_CPU_PTR(pu32)
#define MV_LE32_TO_CPU_PTR(pu32)
#endif /* __MV_BIG_ENDIAN__ */

/* definitions - following macro names are used by RAID module
   must keep consistent */
#define CPU_TO_BIG_ENDIAN_16(x)        MV_CPU_TO_BE16(x)
#define CPU_TO_BIG_ENDIAN_32(x)        MV_CPU_TO_BE32(x)
#define CPU_TO_BIG_ENDIAN_64(x)        MV_CPU_TO_BE64(x)

void SGTable_Init(
    OUT PMV_SG_Table pSGTable,
    IN MV_U8 flag
    );

void sgt_init(
    IN MV_U16 max_io,
    OUT PMV_SG_Table pSGTable,
    IN MV_U8 flag
    );

#define SGTable_Append sgdt_append

MV_BOOLEAN SGTable_Available(
    IN PMV_SG_Table pSGTable
    );

void MV_InitializeTargetIDTable(
    IN PMV_Target_ID_Map pMapTable
    );

MV_U16 MV_GetTargetID(
	IN PMV_Target_ID_Map	
	pMapTable,IN MV_U8 deviceType
	);

MV_U16 MV_MapTargetID(
    IN PMV_Target_ID_Map    pMapTable,
    IN MV_U16                deviceId,
    IN MV_U8                deviceType
    );

MV_U16 MV_MapToSpecificTargetID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				specificId,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	);

MV_U16 MV_RemoveTargetID(
    IN PMV_Target_ID_Map    pMapTable,
    IN MV_U16                deviceId,
    IN MV_U8                deviceType
    );

MV_U16 MV_GetMappedID(
	IN PMV_Target_ID_Map	pMapTable,
	IN MV_U16				deviceId,
	IN MV_U8				deviceType
	);

void MV_DecodeReadWriteCDB(
	IN MV_PU8 Cdb,
	OUT MV_LBA *pLBA,
	OUT MV_U32 *pSectorCount);

void MV_CodeReadWriteCDB(
	OUT MV_PU8	Cdb,
	IN MV_LBA	lba,
	IN MV_U32	sector,
	IN MV_U8	operationCode	/* The CDB[0] */
	);

#define MV_SetLBAandSectorCount(pReq) do {								\
	MV_DecodeReadWriteCDB(pReq->Cdb, &pReq->LBA, &pReq->Sector_Count);	\
	pReq->Req_Flag |= REQ_FLAG_LBA_VALID;								\
} while (0)

void MV_DumpRequest(PMV_Request pReq, MV_BOOLEAN detail);
void MV_DumpSGTable(PMV_SG_Table pSGTable);
const char* MV_DumpSenseKey(MV_U8 sense);

MV_U32 MV_CRC(
	IN MV_PU8  pData,
	IN MV_U16  len
);
MV_U32 MV_CRC_EXT(
	IN	MV_U32		crc,
	IN	MV_PU8		pData,
	IN	MV_U32		len
);

#define MV_MOD_ADD(value, mod)                    \
           do {                                   \
              (value)++;                          \
              if ((value) >= (mod))               \
                 (value) = 0;                     \
           } while (0);

void MV_CHECK_OS_SG_TABLE(
    IN PMV_SG_Table pSGTable
    );

/* used for endian-ness conversion */
static inline MV_VOID mv_swap_bytes(MV_PVOID buf, MV_U32 len)
{
	MV_U32 i;
	MV_U8  tmp, *p;

	/* we expect len to be in multiples of 2 */
	if (len & 0x1)
		return;

	p = (MV_U8 *) buf;
	for (i = 0; i < len / 2; i++)
	{
		tmp = p[i];
		p[i] = p[len - i - 1];
		p[len - i - 1] = tmp;
	}
}

MV_VOID init_target_id_map(MV_U16 *map_table, MV_U32 size);
MV_U16 add_target_map(MV_U16 *map_table, MV_U16 device_id,MV_U16 max_id);
MV_U16 remove_target_map(MV_U16 *map_table, MV_U16 target_id, MV_U16 max_id);

#endif /*  __MV_COM_UTIL_H__ */
