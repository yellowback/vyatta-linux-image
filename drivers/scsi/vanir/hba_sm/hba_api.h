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
#ifndef _HBA_API_H
#define _HBA_API_H
typedef struct _RAID_Feature
{
	List_Head Internal_Request;
	MV_PVOID pHBA;
	MV_PVOID pUpperExtension;
	MV_PVOID pNextExtension;
	MV_VOID (*pNextFunction) (MV_PVOID, PMV_Request);
	MV_U16 SMART_Status_Timer_Handle;
	MV_U8 reserved[2];

} RAID_Feature, *PRAID_Feature;

MV_U32 RAID_Feature_GetResourceQuota(MV_U16 maxIo);
void RAID_Feature_Initialize(MV_PVOID This, MV_U16 maxIo);
void RAID_Feature_SetSendFunction( MV_PVOID This,
	MV_PVOID current_ext,
	MV_PVOID next_ext,
	MV_VOID (*next_function) (MV_PVOID, PMV_Request));
void mvSetAdapterConfig( MV_PVOID This, PMV_Request pReq);
void mvGetAdapterConfig( MV_PVOID This, PMV_Request pReq);
#endif
