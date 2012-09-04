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
#include "mv_os.h"
#include "hba_mod.h"

/* os timer function */
void ossw_init_timer(struct timer_list *timer)
{
	timer->function = NULL;
	init_timer(timer);
}

u8 ossw_add_timer(struct timer_list *timer,
		    u32 msec,
		    void (*function)(unsigned long),
		    unsigned long data)
{
	u64 jif;

	if(timer_pending(timer))
		del_timer(timer);

	timer->function = function;
	timer->data     = data;

	jif = (u64) (msec * HZ);
	do_div(jif, 1000);		   /* wait in unit of second */
	timer->expires = jiffies + 1 + jif;

	add_timer(timer);
	return	0;
}


void ossw_del_timer(struct timer_list *timer)
{
	if (timer->function)
		del_timer(timer);
	timer->function = NULL;
}

int ossw_time_expired(unsigned long time_value)
{
	return (time_before(time_value, jiffies));
}

unsigned long ossw_set_expired_time(u32 msec)
{
	return (jiffies + msecs_to_jiffies(msec));
}

/* os spin lock function */
void  ossw_local_irq_save(unsigned long *flags){ unsigned long save_flag;local_irq_save(save_flag); *flags = save_flag; }
void ossw_local_irq_restore(unsigned long *flags){unsigned long save_flag = *flags;local_irq_restore(save_flag);}
void  ossw_local_irq_disable(void){ local_irq_disable();}
void ossw_local_irq_enable(void){local_irq_enable();}
/* expect pointers */
void ossw_init_spin_lock(void *ext)
{
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->global_lock;
	spin_lock_init(plock);
}


void ossw_spin_lock(void *ext)
{
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->global_lock;
	spin_lock(plock);
}

void ossw_spin_unlock(void *ext)
{
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->global_lock;
	spin_unlock(plock);
}

void ossw_spin_lock_irq(void *ext)
{
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->global_lock;
	spin_lock_irq(plock);
}

void ossw_spin_unlock_irq(void *ext)
{
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->global_lock;
	spin_unlock_irq(plock);
}

void ossw_spin_lock_irq_save(void *ext, unsigned long *flags)
{
	unsigned long save_flag;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->global_lock;
	spin_lock_irqsave(plock, save_flag);
	*flags = save_flag;
}

void ossw_spin_unlock_irq_restore(void *ext, unsigned long *flags)
{
	unsigned long save_flag = *flags;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->global_lock;
	spin_unlock_irqrestore(plock, save_flag);
}

void ossw_spin_lock_irq_save_spin_up(void *ext, unsigned long *flags)
{
	unsigned long save_flag;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->device_spin_up;
	spin_lock_irqsave(plock, save_flag);
	*flags = save_flag;
}

void ossw_spin_unlock_irq_restore_spin_up(void *ext, unsigned long *flags)
{
	unsigned long save_flag = *flags;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	struct mv_adp_desc *hba_desc = mod_desc->hba_desc;
	spinlock_t *plock = &hba_desc->device_spin_up;
	spin_unlock_irqrestore(plock, save_flag);
}

/* os get time function */
u32 ossw_get_time_in_sec(void)
{
	struct timeval tv;

	do_gettimeofday(&tv);
	return (u32) tv.tv_sec;
}

u32 ossw_get_msec_of_time(void)
{
	struct timeval tv;

	do_gettimeofday(&tv);
	return (u32) tv.tv_usec*1000*1000;
}

/*kernel time.h:extern struct timezone sys_tz;*/
extern struct timezone sys_tz;
u32 ossw_get_local_time(void)
{
	static MV_U32 utc_time=0,local_time=0;
	utc_time=ossw_get_time_in_sec();
	local_time = (u32)(utc_time - (sys_tz.tz_minuteswest * 60));
	return local_time;
}

/* os bit endian function */
u16 ossw_cpu_to_le16(u16 x)  	{ return cpu_to_le16(x);}
u32 ossw_cpu_to_le32(u32 x)	{ return cpu_to_le32(x);}
u64 ossw_cpu_to_le64(u64 x)   	{ return cpu_to_le64(x);}
u16 ossw_cpu_to_be16(u16 x)      	{ return cpu_to_be16(x);}
u32 ossw_cpu_to_be32(u32 x)	{ return cpu_to_be32(x);}
u64 ossw_cpu_to_be64(u64 x)   	{ return cpu_to_be64(x);}

u16 ossw_le16_to_cpu(u16 x)      	{ return le16_to_cpu(x);}
u32 ossw_le32_to_cpu(u32 x)      	{ return le32_to_cpu(x);}
u64 ossw_le64_to_cpu(u64 x)      	{ return le64_to_cpu(x);}
u16 ossw_be16_to_cpu(u16 x)      	{ return be16_to_cpu(x);}
u32 ossw_be32_to_cpu(u32 x)      	{ return be32_to_cpu(x);}
u64 ossw_be64_to_cpu(u64 x)      	{ return be64_to_cpu(x);}

/* os map sg address function */
void *ossw_kmap(void  *sg)
{
#ifndef CONFIG_ARM
	struct scatterlist *ksg = (struct scatterlist *)sg;
	void *kvaddr = NULL;


#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
	kvaddr = page_address(ksg->page);
	if (!kvaddr)
#endif
		kvaddr = map_sg_page(ksg);
	kvaddr += ksg->offset;
	return kvaddr;
#else
	BUG_ON(1);
#endif	
}

void ossw_kunmap(void  *sg, void *mapped_addr)
{
#ifndef CONFIG_ARM
	struct scatterlist *ksg = (struct scatterlist *)sg;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
	void *kvaddr = NULL;
	kvaddr = page_address(ksg->page);
	if (!kvaddr)
#endif
	kunmap_atomic(mapped_addr - ksg->offset, KM_IRQ0);
#else
	BUG_ON(1);
#endif
}

void *ossw_kmap_sec(void  *sg)
{
#ifndef CONFIG_ARM
	struct scatterlist *ksg = (struct scatterlist *)sg;
	void *kvaddr = NULL;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
	kvaddr = page_address(ksg->page);
	if (!kvaddr)
#endif
		kvaddr = map_sg_page_sec(ksg);
	kvaddr += ksg->offset;
	return kvaddr;
#else
	BUG_ON(1);
#endif
}

void ossw_kunmap_sec(void  *sg, void *mapped_addr)
{
#ifndef CONFIG_ARM
	struct scatterlist *ksg = (struct scatterlist *)sg;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
	void *kvaddr = NULL;
	kvaddr = page_address(ksg->page);
	if (!kvaddr)
#endif
	kunmap_atomic(mapped_addr - ksg->offset, KM_IRQ1);
#else
	BUG_ON(1);
#endif
}

struct pci_pool *ossw_pci_pool_create( char *name, void *ext ,
	size_t size, size_t align, size_t alloc)
{

	struct pci_dev *dev;
	struct mv_mod_desc *mod_desc = __ext_to_gen(ext)->desc;
	MV_DASSERT(mod_desc);
	dev = mod_desc->hba_desc->dev;
	sprintf(name,"%s%d",name,mod_desc->hba_desc->id);
	return pci_pool_create(name, dev, size, align, alloc);
}

void ossw_pci_pool_destroy(struct pci_pool * pool)
{
	pci_pool_destroy(pool);
}

void *ossw_pci_pool_alloc(struct pci_pool *pool, u64 *dma_handle)
{
	return pci_pool_alloc(pool, GFP_ATOMIC, (dma_addr_t *)dma_handle);
}

void ossw_pci_pool_free(struct pci_pool *pool, void *vaddr, u64 addr)
{
	pci_pool_free(pool, vaddr, addr);
}

void * ossw_kmem_cache_create(const char *name, size_t size,
		size_t align, unsigned long flags)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,22))
	return (void *)kmem_cache_create(name, size, align, flags, NULL);
#else
	return (void *)kmem_cache_create(name, size, align, flags, NULL, NULL);
#endif
}

void * ossw_kmem_cache_alloc( void * cachep, u32 flags)
{
	kmem_cache_t  *cache = (kmem_cache_t *)cachep;
	return kmem_cache_alloc(cache, (gfp_t)flags);
}

void ossw_kmem_cache_free(void *cachep, void *objp)
{
	kmem_cache_t  *cache = (kmem_cache_t *)cachep;
	kmem_cache_free(cache, objp);
}

void ossw_kmem_cache_distroy(void *cachep)
{
	kmem_cache_t  *cache = (kmem_cache_t *)cachep;
	kmem_cache_destroy(cache);
}

unsigned long ossw_virt_to_phys(void *address)
{
 	return virt_to_phys(address);
}

void * ossw_phys_to_virt(unsigned long address)
{
	return phys_to_virt(address);
}

/* os u64 div function */
u64 ossw_u64_div(u64 n, u64 base)
{
	do_div(n, (unsigned int)base);
	return n;
}

u64 ossw_u64_mod(u64 n, u64 base)
{
	return do_div(n, (unsigned int)base);
}


/* bit operation */
u32 ossw_rotr32(u32 v, int count)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12))
	return ror32(v, count);
#else
	return (v << count) | (v >> (32 - count));
#endif
}

 /*ffs - normally return from 1 to MSB, if not find set bit, return 0*/
int ossw_ffz(unsigned long v)	{return (ffs(~v)-1);}
int ossw_ffs(unsigned long v)	{return (ffs(v)-1);}

void *ossw_memcpy(void *dest, const void *source, size_t len) { return memcpy(dest, source, len);}
void *ossw_memset(void *buf, int pattern, size_t len) {return memset(buf, pattern, len);}
int ossw_memcmp(const void *buf0, const void *buf1, size_t len) {return memcmp(buf0, buf1, len); }


/* os read pci config function */
int MV_PCI_READ_CONFIG_DWORD(void * ext, u32 offset, u32 *ptr)	{return pci_read_config_dword(__ext_to_gen(ext)->desc->hba_desc->dev, offset, ptr);}
int MV_PCI_READ_CONFIG_WORD(void * ext, u32 offset, u16 *ptr)	{return pci_read_config_word(__ext_to_gen(ext)->desc->hba_desc->dev, offset, ptr);}
int MV_PCI_READ_CONFIG_BYTE(void * ext, u32 offset, u8 *ptr) 		{return  pci_read_config_byte(__ext_to_gen(ext)->desc->hba_desc->dev, offset, ptr);}
int MV_PCI_WRITE_CONFIG_DWORD(void *ext, u32 offset, u32 val)	{return  pci_write_config_dword(__ext_to_gen(ext)->desc->hba_desc->dev, offset, val);}
int MV_PCI_WRITE_CONFIG_WORD(void *ext, u32 offset, u16 val) 	{return pci_write_config_word(__ext_to_gen(ext)->desc->hba_desc->dev, offset, val);}
int MV_PCI_WRITE_CONFIG_BYTE(void *ext, u32 offset, u8 val)		{return pci_write_config_byte(__ext_to_gen(ext)->desc->hba_desc->dev, offset, val);}

/* System dependent macro for flushing CPU write cache */
void MV_CPU_WRITE_BUFFER_FLUSH(void) 	{smp_wmb();}
void MV_CPU_READ_BUFFER_FLUSH(void)  	{smp_rmb();}
void MV_CPU_BUFFER_FLUSH(void)       			{smp_mb();}

/* register read write: memory io */
void MV_REG_WRITE_BYTE(void *base, u32 offset, u8 val)		{writeb(val, base + offset);}
void MV_REG_WRITE_WORD(void *base, u32 offset, u16 val)	{writew(val, base + offset);}
void MV_REG_WRITE_DWORD(void *base, u32 offset, u32 val)   {writel(val, base + offset);}

u8 		MV_REG_READ_BYTE(void *base, u32 offset)			{return readb(base + offset);}
u16 		MV_REG_READ_WORD(void *base, u32 offset)			{return readw(base + offset);}
u32 		MV_REG_READ_DWORD(void *base, u32 offset)		{return readl(base + offset);}

/* register read write: port io */
void	MV_IO_WRITE_BYTE(void *base, u32 offset, u8 val)	{outb(val, (unsigned)(MV_PTR_INTEGER)(base + offset));}
void MV_IO_WRITE_WORD(void *base, u32 offset, u16 val)    {outw(val, (unsigned)(MV_PTR_INTEGER)(base + offset));}
void MV_IO_WRITE_DWORD(void *base, u32 offset, u32 val)    {outl(val, (unsigned)(MV_PTR_INTEGER)(base + offset));}

u8	MV_IO_READ_BYTE(void *base, u32 offset)	{return inb((unsigned)(MV_PTR_INTEGER)(base + offset));}
u16	MV_IO_READ_WORD(void *base, u32 offset)	{return inw((unsigned)(MV_PTR_INTEGER)(base + offset));}
u32 	MV_IO_READ_DWORD(void *base, u32 offset)	{return inl((unsigned)(MV_PTR_INTEGER)(base + offset));}


/* os print function */
int  ossw_printk(char *fmt, ...)
{
	va_list args;
	static char buf[1024];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	return printk("%s",  buf);
}

#ifdef _SUPPORT_64_BIT
void MV_DUMP_SP(void)
{
	printk("THREAD_SIZE= %d,PID =%d.\n", (unsigned int)THREAD_SIZE,(unsigned int)current->tgid);
	dump_stack();
}

#else
void MV_DUMP_SP(void )
{
	unsigned long sp;
#ifdef CONFIG_X86
	__asm__ __volatile__("andl %%esp,%0" :"=r" (sp) : "0" (THREAD_SIZE - 1));
	printk("SP = %ld ,THREAD_SIZE= %d,PID =%d.\n",sp, (unsigned int)THREAD_SIZE,(unsigned int)current->tgid);
#elif defined(CONFIG_PPC)
	__asm__ __volatile__("mr %0, 1":"=r"(sp));
	printk("SP = %ld ,THREAD_SIZE= %d,PID =%d.\n",sp, (unsigned int)THREAD_SIZE,(unsigned int)current->tgid);
#elif defined(CONFIG_ARM)

#else
#error "Please add the corresponding stack retrieval info."
#endif
	dump_stack();
}
#endif


/* Sleeping is disallowed if any of these macroes evalute as true*/
void MV_DUMP_CTX(void)
{
	if( in_irq())
		printk("Present process is in hard IRQ context.\n");
	if(in_softirq())
		printk("Present process is in soft IRQ(BH) context.\n");
	if( in_interrupt())
		 printk("Present process is in hard/soft IRQ context.\n");
	if(in_atomic())
		 printk("Present process is  in preemption-disabled context .\n");
}

