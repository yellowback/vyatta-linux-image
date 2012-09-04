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
#ifndef __MV_COM_FLASH_H__
#define __MV_COM_FLASH_H__

#include "com_define.h"
#include "com_event_struct.h"

#define DRIVER_LENGTH                      1024*16

#pragma pack(8)

typedef struct _Flash_DriverData
{
	MV_U16            Size;
	MV_U8             PageNumber;
	MV_BOOLEAN        isLastPage;
	MV_U16            Reserved[2];
	MV_U8             Data[DRIVER_LENGTH];
}
Flash_DriveData, *PFlash_DriveData;

#pragma pack()

#endif /* __MV_COM_FLASH_H__ */
