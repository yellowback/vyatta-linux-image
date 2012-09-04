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
#include "com_u64.h"

MV_U64 U64_ADD_U32(MV_U64 v64, MV_U32 v32)
{
#ifdef _64_BIT_COMPILER
	v64.value += v32;
#else
	v64.parts.low += v32;
	v64.parts.high = 0;
#endif
	return v64;
}

MV_U64 U64_SUBTRACT_U32(MV_U64 v64, MV_U32 v32)
{
#ifdef _64_BIT_COMPILER
	v64.value -= v32;
#else
	v64.parts.low -= v32;
	v64.parts.high = 0;
#endif
	return v64;
}

MV_U64 U64_MULTIPLY_U32(MV_U64 v64, MV_U32 v32)
{
#ifdef _64_BIT_COMPILER
	v64.value *= v32;
#else
	v64.parts.low *= v32;
	v64.parts.high = 0;
#endif
	return v64;
}

MV_U32 U64_MOD_U32(MV_U64 v64, MV_U32 v32)
{
	if(v32)
		return ossw_u64_mod(v64.value, v32);
	else {
		MV_DPRINT(("Warning: divisor is zero in %s.\n", __FUNCTION__));
		return	0;	
	}
}

MV_U64 U64_DIVIDE_U32(MV_U64 v64, MV_U32 v32)
{
	if(v32)
		v64.value = ossw_u64_div(v64.value, v32);
	else {
		MV_DPRINT(("Warning: divisor is zero in %s.\n", __FUNCTION__));
		v64.value = 0;
	}

	return v64;
}

MV_I32 U64_COMPARE_U32(MV_U64 v64, MV_U32 v32)
{
	if (v64.parts.high > 0)
		return 1;
	if (v64.parts.low > v32)
		return 1;
#ifdef _64_BIT_COMPILER
	else if (v64.value == v32)
#else
	else if (v64.parts.low == v32)
#endif
		return 0;
	else
		return -1;
}

MV_U64 U64_ADD_U64(MV_U64 v1, MV_U64 v2)
{
#ifdef _64_BIT_COMPILER
	v1.value += v2.value;
#else
	v1.parts.low += v2.parts.low;
	v1.parts.high = 0;
#endif
	return v1;
}

MV_U64 U64_SUBTRACT_U64(MV_U64 v1, MV_U64 v2)
{
#ifdef _64_BIT_COMPILER
	v1.value -= v2.value;
#else
	v1.parts.low -= v2.parts.low;
	v1.parts.high = 0;
#endif
	return v1;
}

MV_U32 U64_DIVIDE_U64(MV_U64 v1, MV_U64 v2)
{
	MV_U32 ret = 0;
	while (v1.value > v2.value) {
		v1.value -= v2.value;
		ret++;
	}
	return ret;
}

MV_I32 U64_COMPARE_U64(MV_U64 v1, MV_U64 v2)
{
#ifdef _64_BIT_COMPILER
	if (v1.value > v2.value)
		return 1;
	else if (v1.value == v2.value)
		return 0;
	else
		return -1;
#else
	if (v1.value > v2.value)
		return 1;
	else if (v1.value == v2.value)
		return 0;
	else
		return -1;

#endif

}

