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
#ifndef __MV_COM_TAG_H__
#define __MV_COM_TAG_H__

#include "com_define.h"

typedef struct _Tag_Stack Tag_Stack, *PTag_Stack;

#define FILO_TAG 0x00
#define FIFO_TAG 0x01

/* if TagStackType!=FIFO_TAG, use FILO, */
/* if TagStackType==FIFO_TAG, use FIFO, PtrOut is the next tag to get */
/*  and Top is the number of available tags in the stack */
/* when use FIFO, get tag from PtrOut and free tag to (PtrOut+Top)%Size */
struct _Tag_Stack
{
	MV_PU16  Stack;
	MV_U16   Size;
	MV_U16   Top;
	MV_U16   PtrOut;
	MV_U8    TagStackType;
	MV_U8    Reserved[1];	
};

MV_VOID Tag_Init(PTag_Stack pTagStack, MV_U16 size);
MV_VOID Tag_Init_FIFO( PTag_Stack pTagStack, MV_U16 size );
MV_U16 Tag_GetOne(PTag_Stack pTagStack) ;
MV_VOID Tag_ReleaseOne(PTag_Stack pTagStack, MV_U16 tag);
MV_BOOLEAN Tag_IsEmpty(PTag_Stack pTagStack);

#endif /*  __MV_COM_TAG_H__ */
