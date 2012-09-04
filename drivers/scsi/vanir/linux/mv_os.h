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
#ifndef __LINUX_OS_H__
#define __LINUX_OS_H__

#ifndef LINUX_VERSION_CODE
#   include <linux/version.h>
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)) && (!defined AUTOCONF_INCLUDED)
#   include <linux/config.h>
#endif

#include <linux/list.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/reboot.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/pci.h>
#include <linux/completion.h>
#include <linux/blkdev.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>

#include <linux/device.h>
#include <linux/nmi.h>

#include <linux/slab.h>
#include <linux/mempool.h>
#include <linux/ctype.h>
#include "linux/hdreg.h"

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/div64.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_tcq.h>
#include <scsi/scsi_transport.h>
#include <scsi/scsi_ioctl.h>
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,9)
#include <scsi/scsi_request.h>
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,19)
#include <linux/freezer.h>
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,6))
	#include <linux/moduleparam.h>
	#endif
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 7) */

/* OS specific flags */


#ifndef NULL
#   define NULL 0
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,11)
#define PCI_D0 0
#include <linux/suspend.h>
typedef u32 pm_message_t;

static inline int try_to_freeze(unsigned long refrigerator_flags)
{
        if (unlikely(current->flags & PF_FREEZE)) {
               refrigerator(refrigerator_flags);
                return 1;
        } else
             return 0;
}
#endif

#define MV_INLINE inline
#define CDB_INQUIRY_EVPD    1

/* If VER_BUILD ,the 4th bit is 0 */
#if (VER_BUILD < 1000)
#define NUM_TO_STRING(num1, num2, num3, num4) # num1"."# num2"."# num3".""0"# num4
#else
#define NUM_TO_STRING(num1, num2, num3, num4) # num1"."# num2"."# num3"."# num4
#endif
#define VER_VAR_TO_STRING(major, minor, oem, build) NUM_TO_STRING(major, \
								  minor, \
								  oem,   \
								  build)

#define mv_version_linux   VER_VAR_TO_STRING(VER_MAJOR, VER_MINOR,       \
					     VER_OEM, VER_BUILD) VER_TEST

#ifndef TRUE
#define TRUE 	1
#define FALSE	0
#endif

#ifdef CONFIG_64BIT
#   define __KCONF_64BIT__
#endif /* CONFIG_64BIT */

#if defined(__LITTLE_ENDIAN)
#   define __MV_LITTLE_ENDIAN__  1
#elif defined(__BIG_ENDIAN)
#   define __MV_BIG_ENDIAN__     1
#else
#   error "error in endianness"
#endif

#if defined(__LITTLE_ENDIAN_BITFIELD)
#   define __MV_LITTLE_ENDIAN_BITFIELD__   1
#elif defined(__BIG_ENDIAN_BITFIELD)
#   define __MV_BIG_ENDIAN_BITFIELD__      1
#else
#   error "error in endianness"
#endif


#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 25)
#define mv_use_sg(cmd)	cmd->use_sg
#define mv_rq_bf(cmd)	cmd->request_buffer
#define mv_rq_bf_l(cmd)	cmd->request_bufflen
#else
#define mv_use_sg(cmd)	scsi_sg_count(cmd)
#define mv_rq_bf(cmd)	scsi_sglist(cmd)
#define mv_rq_bf_l(cmd)	scsi_bufflen(cmd)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
#define map_sg_page(sg)		kmap_atomic(sg->page, KM_IRQ0)
#define map_sg_page_sec(sg)		kmap_atomic(sg->page, KM_IRQ1)
#else
#define map_sg_page(sg)		kmap_atomic(sg_page(sg), KM_IRQ0)
#define map_sg_page_sec(sg)		kmap_atomic(sg_page(sg), KM_IRQ1)
#endif


struct gen_module_desc {
/* Must the first */
	struct mv_mod_desc *desc;
};
#define __ext_to_gen(_ext)       ((struct gen_module_desc *) (_ext))


/* os timer function */
void ossw_init_timer(struct timer_list *timer);
u8 ossw_add_timer(struct timer_list *timer,
		    u32 msec,
		    void (*function)(unsigned long),
		    unsigned long data);
void ossw_del_timer(struct timer_list *timer);

int ossw_time_expired(unsigned long time_value);
unsigned long ossw_set_expired_time(u32 msec);

/* os spin lock function */
void  ossw_local_irq_save(unsigned long *flags);
void ossw_local_irq_restore(unsigned long *flags);
void  ossw_local_irq_disable(void);
void ossw_local_irq_enable(void);

/* expect pointers */
void ossw_init_spin_lock(void *ext);
void ossw_spin_lock(void *ext);
void ossw_spin_unlock(void *ext);
void ossw_spin_lock_irq(void *ext);
void ossw_spin_unlock_irq(void *ext);
void ossw_spin_lock_irq_save(void *ext, unsigned long *flags);
void ossw_spin_unlock_irq_restore(void *ext, unsigned long *flags);
void ossw_spin_lock_irq_save_spin_up(void *ext, unsigned long *flags);
void ossw_spin_unlock_irq_restore_spin_up(void *ext, unsigned long *flags);

/* os get time function */
u32 ossw_get_time_in_sec(void);
u32 ossw_get_msec_of_time(void);
u32 ossw_get_local_time(void);

/* os kmem_cache */
#define MV_ATOMIC GFP_ATOMIC

struct pci_pool *ossw_pci_pool_create(char *name, void *ext,
	size_t size, size_t align, size_t alloc);
void ossw_pci_pool_destroy(struct pci_pool * pool);
void *ossw_pci_pool_alloc(struct pci_pool *pool, u64 *dma_handle);
void ossw_pci_pool_free(struct pci_pool *pool, void *vaddr, u64 addr);


void * ossw_kmem_cache_create(const char *, size_t, size_t, unsigned long);
void * ossw_kmem_cache_alloc( void *, unsigned);
void ossw_kmem_cache_free(void *, void *);
void ossw_kmem_cache_distroy(void *);

unsigned long ossw_virt_to_phys(void *);
void * ossw_phys_to_virt(unsigned long address);


/* os bit endian function */
u16 ossw_cpu_to_le16(u16 x);
u32 ossw_cpu_to_le32(u32 x);
u64 ossw_cpu_to_le64(u64 x);
u16 ossw_cpu_to_be16(u16 x);
u32 ossw_cpu_to_be32(u32 x);
u64 ossw_cpu_to_be64(u64 x);

u16 ossw_le16_to_cpu(u16 x);
u32 ossw_le32_to_cpu(u32 x);
u64 ossw_le64_to_cpu(u64 x) ;
u16 ossw_be16_to_cpu(u16 x);
u32 ossw_be32_to_cpu(u32 x);
u64 ossw_be64_to_cpu(u64 x) ;

/* os map sg address function */
void *ossw_kmap(void  *sg);
void ossw_kunmap(void  *sg, void *mapped_addr);
void *ossw_kmap_sec(void  *sg);
void ossw_kunmap_sec(void  *sg, void *mapped_addr);

/* MISC Services */
#define ossw_udelay(x) udelay(x)

/* os u64 div function */
u64 ossw_u64_div(u64 n, u64 base);
u64 ossw_u64_mod(u64 n, u64 base);


/* bit operation */
u32 ossw_rotr32(u32 v, int count);

int ossw_ffz(unsigned long v);
int ossw_ffs(unsigned long v);

void *ossw_memcpy(void *dest, const void *source, size_t len) ;
void *ossw_memset(void *buf, int patten, size_t len) ;
int ossw_memcmp(const void *buf0, const void *buf1, size_t len) ;

/* os read pci config function */
int MV_PCI_READ_CONFIG_DWORD(void * ext, u32 offset, u32 *ptr);
int MV_PCI_READ_CONFIG_WORD(void * ext, u32 offset, u16 *ptr);
int MV_PCI_READ_CONFIG_BYTE(void * ext, u32 offset, u8 *ptr);
int MV_PCI_WRITE_CONFIG_DWORD(void *ext, u32 offset, u32 val);
int MV_PCI_WRITE_CONFIG_WORD(void *ext, u32 offset, u16 val);
int MV_PCI_WRITE_CONFIG_BYTE(void *ext, u32 offset, u8 val);

/* System dependent macro for flushing CPU write cache */
void MV_CPU_WRITE_BUFFER_FLUSH(void);
void MV_CPU_READ_BUFFER_FLUSH(void);
void MV_CPU_BUFFER_FLUSH(void);

/* register read write: memory io */
void MV_REG_WRITE_BYTE(void *base, u32 offset, u8 val);
void MV_REG_WRITE_WORD(void *base, u32 offset, u16 val);
void MV_REG_WRITE_DWORD(void *base, u32 offset, u32 val);

u8 		MV_REG_READ_BYTE(void *base, u32 offset);
u16 		MV_REG_READ_WORD(void *base, u32 offset)	;
u32 		MV_REG_READ_DWORD(void *base, u32 offset);

/* register read write: port io */
void	MV_IO_WRITE_BYTE(void *base, u32 offset, u8 val);
void MV_IO_WRITE_WORD(void *base, u32 offset, u16 val) ;
void MV_IO_WRITE_DWORD(void *base, u32 offset, u32 val) ;

u8	MV_IO_READ_BYTE(void *base, u32 offset);
u16	MV_IO_READ_WORD(void *base, u32 offset);
u32 	MV_IO_READ_DWORD(void *base, u32 offset);

int  ossw_printk(char *fmt, ...);

void MV_DUMP_SP(void);
void MV_DUMP_CTX(void);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
#ifdef _SUPPORT_64_BIT
	typedef u64 resource_size_t;
 #else
	typedef u32 resource_size_t;
#endif
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 23)
typedef struct kmem_cache kmem_cache_t;
#endif

#endif /* LINUX_OS_H */


