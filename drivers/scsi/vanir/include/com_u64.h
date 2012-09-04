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
#ifndef _U64_H__
#define _U64_H__

#define U64_ASSIGN_SAFE(v1, v2)	\
do{	int i; \
	for (i=0; i<sizeof((v1)); i++)	\
		((char*)&v1)[i] = ((char*)(v2))[i];	\
}while(0)

MV_U64 U64_ADD_U32(MV_U64 v64, MV_U32 v32);
MV_U64 U64_SUBTRACT_U32(MV_U64 v64, MV_U32 v32);
MV_U64 U64_MULTIPLY_U32(MV_U64 v64, MV_U32 v32);
MV_U64 U64_DIVIDE_U32(MV_U64 v64, MV_U32 v32);
MV_I32 U64_COMPARE_U32(MV_U64 v64, MV_U32 v32);
MV_U32 U64_MOD_U32(MV_U64 v64, MV_U32 v32);

MV_U64 U64_ADD_U64(MV_U64 v1, MV_U64 v2);
MV_U64 U64_SUBTRACT_U64(MV_U64 v1, MV_U64 v2);
MV_U32 U64_DIVIDE_U64(MV_U64 v1, MV_U64 v2);
MV_I32 U64_COMPARE_U64(MV_U64 v1, MV_U64 v2);
#define U64_SET_VALUE(v64, v32)	do { v64.value = v32; } while(0)
#define U64_SET_MAX_VALUE(v64)	do { v64.parts.low = v64.parts.high = 0xFFFFFFFFL; } while(0);

#endif

