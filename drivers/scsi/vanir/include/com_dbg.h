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
#if !defined(COMMON_DEBUG_H)
#define COMMON_DEBUG_H

/* 
 *	Marvell Debug Interface
 * 
 *	MACRO
 *		MV_DEBUG is defined in debug version not in release version.
 *	
 *	Debug funtions:
 *		MV_PRINT:	print string in release and debug build.
 *		MV_DPRINT:	print string in debug build.
 *		MV_TRACE:	print string including file name, line number in release and debug build.
 *		MV_DTRACE:	print string including file name, line number in debug build.
 *		MV_ASSERT:	assert in release and debug build.
 *		MV_DASSERT: assert in debug build.
  *		MV_WARNON: warn in debug build.
 */

#include "com_define.h"
/*
 *
 * Debug funtions
 *
 */
#define MV_PRINT(format, arg...) 	ossw_printk(format,##arg);
#define MV_ASSERT(_x_)  do{if (!(_x_)) MV_PRINT("ASSERT: function:%s, line:%d\n", __FUNCTION__, __LINE__);}while(0)
#   define MV_TRACE(_x_)                                        \
              do {                                              \
                 MV_PRINT("%s(%d) ", __FILE__, __LINE__);       \
                 MV_PRINT _x_;                                  \
           } while(0)

extern MV_U16 mv_debug_mode;
#define MV_DPRINT(_x_)                do {\
	if (mv_debug_mode & GENERAL_DEBUG_INFO) \
	MV_PRINT _x_;\
	} while (0)
#define MV_DASSERT(x)	        do {\
	if (mv_debug_mode & GENERAL_DEBUG_INFO) \
	MV_ASSERT(x);\
	} while (0)
#define MV_DTRACE	        MV_DTRACE

MV_BOOLEAN mvLogRegisterModule(MV_U8 moduleId, MV_U32 filterMask, char* name);
MV_BOOLEAN mvLogSetModuleFilter(MV_U8 moduleId, MV_U32 filterMask);
MV_U32 mvLogGetModuleFilter(MV_U8 moduleId);
void mvLogMsg(MV_U8 moduleId, MV_U32 type, char* format, ...);

#endif /* COMMON_DEBUG_H */

