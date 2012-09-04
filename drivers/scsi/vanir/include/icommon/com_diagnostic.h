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
#ifndef __MV_COM_DIAGNOSTIC_H__
#define __MV_COM_DIAGNOSTIC_H__
#ifndef COM_DEFINE_H //for magni not use include com_define to define MV_U32
#include "com_define.h"
#endif

typedef struct _SendDiaCDB
{
	MV_U8 OperationCode;
	MV_U8 UnitOffL:1;
	MV_U8 DevOffL:1;
	MV_U8 SelfTest:1;
	MV_U8 Reserved:1;
	MV_U8 PF:1;
	MV_U8 SelfTestCode:3;
	MV_U8 Reserved2;
	MV_U8 ParamLength[2];
	MV_U8 Control;
}SendDiaCDB;

typedef struct _ReceiveDiaCDB
{
	MV_U8 OperationCode;
	MV_U8 PCV:1;
	MV_U8 Reserved:7;
	MV_U8 PageCode;
	MV_U8 AllocationLength[2];
	MV_U8 Control;
}ReceiveDiaCDB;

typedef struct _DianosticPage
{
	MV_U8    PageCode;
	MV_U8    PageCodeSpec;
	MV_U8    PageLength[2];
	MV_U8    Parameters[1];
}DianosticPage, *PDianosticPage;

#define MV_DIAGNOSTIC_CDB_LENGTH   12
#define MV_DIANOSTICPAGE_NONE_DATA  0

#define SCSI_CMD_RECEIVE_DIAGNOSTIC_RESULTS   0x1c  //spc3r23 p97
#define SCSI_CMD_SEND_DIAGNOSTIC              0x1d  //spc3r23 p97

typedef struct _MVATAPIDiagnosticParameters
{
	MV_U8    cdbOffset;
	MV_U8    senseOffset;
	MV_U8    dataOffset;
	MV_U8	 reserved;
	MV_U8    cdb[16];
	MV_U8    senseBuffer[32];
	MV_U32   dataLength;
	MV_U8    Data[1];
}MVATAPIDiagParas, *PMVATAPIDiagParas;

#define MV_API_SEND_PAGE                      0xe0
#define MV_API_RECEV_PAGE                     0xe1

typedef MVATAPIDiagParas send_diagnostic_page;
typedef DianosticPage diagnostic_page;
typedef MVATAPIDiagParas recv_diagnostic_page;

#endif
