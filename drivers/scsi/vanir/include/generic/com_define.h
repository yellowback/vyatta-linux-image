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
#ifndef COM_DEFINE_H
#define COM_DEFINE_H

 #include "mv_os.h"

/*
 *  This file defines Marvell OS independent primary data type for all OS.
 *
 *  We have macros to differentiate different CPU and OS.
 *
 *  CPU definitions:
 *  _CPU_X86_16B  
 *  Specify 16bit x86 platform, this is used for BIOS and DOS utility.
 *  _CPU_X86_32B
 *  Specify 32bit x86 platform, this is used for most OS drivers.
 *  _CPU_IA_64B
 *  Specify 64bit IA64 platform, this is used for IA64 OS drivers.
 *  _CPU_AMD_64B
 *  Specify 64bit AMD64 platform, this is used for AMD64 OS drivers.
 *
 *  OS definitions:
 *  _OS_WINDOWS
 *  _OS_LINUX
 *  _OS_FREEBSD
 *  _OS_BIOS
 *  __QNXNTO__
 */

#if !defined(IN)
#   define IN
#endif

#if !defined(OUT)
#   define OUT
#endif

#   define    BUFFER_PACKED               __attribute__((packed))

#define MV_BIT(x)                         (1L << (x))

#if !defined(NULL)
#   define NULL 0
#endif  /* NULL */

#define MV_TRUE                           1
#define MV_FALSE                          0
#define MV_SUCCESS						  1
#define MV_FAIL							  0

typedef unsigned char  MV_BOOLEAN, *MV_PBOOLEAN;
typedef unsigned char  MV_U8, *MV_PU8;
typedef signed char  MV_I8, *MV_PI8;

typedef unsigned short  MV_U16, *MV_PU16;
typedef signed short  MV_I16, *MV_PI16;

typedef void    MV_VOID, *MV_PVOID;
typedef void            *MV_LPVOID;

#   define BASEATTR 

typedef unsigned int MV_U32, *MV_PU32;
typedef   signed int MV_I32, *MV_PI32;
typedef unsigned long MV_ULONG, *MV_PULONG;
typedef   signed long MV_ILONG, *MV_PILONG;
typedef unsigned long long _MV_U64;
typedef   signed long long _MV_I64;

#   if defined(__KCONF_64BIT__)
#      define _SUPPORT_64_BIT
#   else
#      ifdef _SUPPORT_64_BIT
#         error Error 64_BIT CPU Macro
#      endif
#   endif /* defined(__KCONF_64BIT__) */

/*
 * Primary Data Type
 */
typedef union {
        struct {
#   if defined (__MV_LITTLE_ENDIAN__)
            MV_U32 low;
            MV_U32 high;
#   elif defined (__MV_BIG_ENDIAN__)
            MV_U32 high;
            MV_U32 low;
#   else
#           error "undefined endianness"
#   endif /* __MV_LITTLE_ENDIAN__ */
        } parts;
        _MV_U64 value;
    } MV_U64, *PMV_U64, *MV_PU64;

typedef _MV_U64 BUS_ADDRESS;

/* PTR_INTEGER is necessary to convert between pointer and integer. */
#if defined(_SUPPORT_64_BIT)
   typedef _MV_U64 MV_PTR_INTEGER;
#else
   typedef MV_U32 MV_PTR_INTEGER;
#endif /* defined(_SUPPORT_64_BIT) */

#ifdef _SUPPORT_64_BIT
#define _64_BIT_COMPILER     1
#endif

/* LBA is the logical block access */
typedef MV_U64 MV_LBA;

#if defined(_CPU_16B)
    typedef MV_U32 MV_PHYSICAL_ADDR;
#else
    typedef MV_U64 MV_PHYSICAL_ADDR;
#endif /* defined(_CPU_16B) */

typedef MV_I32 MV_FILE_HANDLE;


#include "com_product.h"

#define hba_local_irq_disable() ossw_local_irq_disable()
#define hba_local_irq_enable() ossw_local_irq_enable()
#define hba_local_irq_save(flag) ossw_local_irq_save(&flag)
#define hba_local_irq_restore(flag) ossw_local_irq_restore(&flag)

/* expect pointers */
#define OSSW_INIT_SPIN_LOCK(ext) ossw_init_spin_lock(ext)
#define OSSW_SPIN_LOCK(ext)            ossw_spin_lock(ext)
#define OSSW_SPIN_UNLOCK(ext)          ossw_spin_unlock(ext)
#define OSSW_SPIN_LOCK_IRQ(ext)            ossw_spin_lock_irq(ext)
#define OSSW_SPIN_UNLOCK_IRQ(ext)          ossw_spin_unlock_irq(ext)
#define OSSW_SPIN_LOCK_IRQSAVE(ext, flag)          ossw_spin_lock_irq_save(ext, &flag)
#define OSSW_SPIN_UNLOCK_IRQRESTORE(ext, flag)           ossw_spin_unlock_irq_restore(ext, &flag)
#define OSSW_SPIN_LOCK_IRQSAVE_SPIN_UP(ext, flag)          ossw_spin_lock_irq_save_spin_up(ext, &flag)
#define OSSW_SPIN_UNLOCK_IRQRESTORE_SPIN_UP(ext, flag)           ossw_spin_unlock_irq_restore_spin_up(ext, &flag)

/* Delayed Execution Services */
#define OSSW_INIT_TIMER(ptimer) ossw_init_timer(ptimer)

#if !defined( likely )
#define likely(x)		(x)
#define unlikely(x)		(x)
#endif

#define GENERAL_DEBUG_INFO	MV_BIT(0)	/*For MV_DPRINT*/
#define CORE_DEBUG_INFO		MV_BIT(1)	/*Core debug info: CORE_PRINT, CORE_EH_PRINT*/
#define RAID_DEBUG_INFO		MV_BIT(2)	/*Raid debug info*/
#define CACHE_DEBUG_INFO	MV_BIT(3)	/*Cache debug info*/
#define CORE_FULL_DEBUG_INFO	(CORE_DEBUG_INFO | \
							GENERAL_DEBUG_INFO) /*CORE_DPRINT, CORE_EH_PRINT, CORE_EVENT_PRINT*/
#define RAID_FULL_DEBUG_INFO	(RAID_DEBUG_INFO | \
							GENERAL_DEBUG_INFO) 
#define CACHE_FULL_DEBUG_INFO	(CACHE_DEBUG_INFO | \
							GENERAL_DEBUG_INFO) 

#define sg_map(x)	hba_map_sg_to_buffer(x)
#define sg_unmap(x)	hba_unmap_sg_to_buffer(x)

#define time_is_expired(x) 	ossw_time_expired(x)
#define EVENT_SET_DELAY_TIME(x, y) ((x) = ossw_set_expired_time(y))

#define msi_enabled(x)	hba_msi_enabled(x)

#endif /* COM_DEFINE_H */
