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
#ifndef __MV_COM_REQUEST_DETAIL_H__
#define __MV_COM_REQUEST_DETAIL_H__

#include "com_pd_struct.h"
#include "com_enc_struct.h"
#include "com_vd_struct.h"
#include "com_raid_struct.h"
#include "com_array_struct.h"
#include "com_adapter_struct.h"

#pragma pack(8)

typedef struct _HD_Info_Request
{
    RequestHeader header;
    HD_Info  hdInfo[1];	
} HD_Info_Request, *PHD_Info_Request;

typedef struct _HD_FreeSpaceInfo_Request
{
    RequestHeader header;
    HD_FreeSpaceInfo  hdFreeSpaceInfo[1];
} HD_FreeSpaceInfo_Request, *PHD_FreeSpaceInfo_Request;

typedef struct _HD_Config_Request
{
    RequestHeader header;
    HD_Config  hdConfig[1];
} HD_Config_Request, *PHD_Config_Request;

typedef struct _HD_SMART_Status_Request
{
    RequestHeader header;
    HD_SMART_Status  hdSmartStatus[1];
} HD_SMART_Status_Request, *PHD_SMART_Status_Request;

typedef struct _HD_Block_Info_Request
{
    RequestHeader header;
    HD_Block_Info  hdBlockInfo[1];
} HD_Block_Info_Request, *PHD_Block_Info_Request;

typedef struct _HD_RAID_Status_Request
{
    RequestHeader header;
    HD_RAID_Status  hdRaidStatus[1];
} HD_RAID_Status_Request, *PHD_RAID_Status_Request;

typedef struct _HD_BGA_Status_Request
{
    RequestHeader header;
    HD_BGA_Status  hdBgaStatus[1];
} HD_BGA_Status_Request, *PHD_BGA_Status_Request;

typedef struct _Block_Info_Request
{
    RequestHeader header;
    Block_Info  blockInfo[1];
} Block_Info_Request, *PBlock_Info_Request;

typedef struct _LD_Info_Request
{
    RequestHeader header;
    LD_Info  ldInfo[1];
} LD_Info_Request, *PLD_Info_Request;

typedef struct _LD_Status_Request
{
    RequestHeader header;
    LD_Status  ldStatus[1];
} LD_Status_Request, *PLD_Status_Request;

typedef struct _LD_Config_Request
{
    RequestHeader header;
    LD_Config  ldConfig[1];
} LD_Config_Request, *PLD_Config_Request;

typedef struct _DG_Info_Request
{
    RequestHeader header;
    DG_Info  dgInfo[1];
} DG_Info_Request, *PDG_Info_Request;

typedef struct _DG_Config_Request
{
    RequestHeader header;
    DG_Config  dgConfig[1];
} DG_Config_Request, *PDG_Config_Request;

typedef struct _RCT_Record_Request
{
    RequestHeader header;
    RCT_Record  rctRecord[1];
} RCT_Record_Request, *PRCT_Record_Request;

/* Port Multiplexier */
typedef struct _PM_Info_Request
{
    RequestHeader header;
    PM_Info  pmInfo[1];
} PM_Info_Request, *PPM_Info_Request;

/* Expander */ 
typedef struct _Exp_Info_Request
{
    RequestHeader header;
    Exp_Info  expInfo[1];
} Exp_Info_Request, *PExp_Info_Request;

typedef struct _Enclosure_Info_Request
{
    RequestHeader header;
    Enclosure_Info  encInfo[1];
} Enclosure_Info_Request, *PEnclosure_Info_Request;

typedef struct _EncElementType_Info_Request
{
    RequestHeader header;
    EncElementType_Info  encEleTypeInfo[1];
} EncElementType_Info_Request, *PEncElementType_Info_Request;

typedef struct _EncElement_Config_Request
{
    RequestHeader header;
    EncElement_Config  encEleConfig[1];
} EncElement_Config_Request, *PEncElement_Config_Request;

#pragma pack()

#endif
