/*
 * arch/arm/mach/irq.c
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <asm/mach/arch.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <plat/msi.h>
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/mvSemaphore.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "gpp/mvGpp.h"
#include "gpp/mvGppRegs.h"
#include "mvOs.h"
#include "include/mach/smp.h"

unsigned int  irq_int_type[NR_IRQS];
static DEFINE_SPINLOCK(irq_controller_lock);

int max_per_cpu_irq = 28; // not enabled, default.
static int __init per_cpu_irq_setup(char *__unused)
{
	max_per_cpu_irq=7;
	return 1;
}

__setup("per_cpu_irq_enable", per_cpu_irq_setup);

static void axp_unmask_fabric_interrupt(int cpu)
{
	u32 val;
	val = MV_REG_READ(CPU_CF_LOCAL_MASK_REG(cpu));
	val |=  (1 << cpu);
	MV_REG_WRITE(CPU_CF_LOCAL_MASK_REG(cpu), val);

#ifdef CONFIG_SMP
	if (cpu > 0) { /*enabled for both cpu */
		val = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_MP));
		/* FIXME: assuming all 4 cpus */
		val |= 0xf;
		MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_MP), val);
	}
#endif
}

static void axp_mask_fabric_interrupt(int cpu)
{
	u32 val;
	val = MV_REG_READ(CPU_CF_LOCAL_MASK_REG(cpu));
	val &=  ~(1 << cpu);
	MV_REG_WRITE(CPU_CF_LOCAL_MASK_REG(cpu), val);

#ifdef CONFIG_SMP
	if (cpu > 0) { /*disabled for both cpu */
		val = MV_REG_READ(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_MP));
		/* FIXME: assuming all 4 cpus */
		val &= ~0xf;
		MV_REG_WRITE(CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_MP), val);
	}
#endif	
}

#ifdef CONFIG_ARMADAXP_USE_IRQ_INDIRECT_MODE
void axp_irq_mask(struct irq_data *d)
{	
	u32 irq=d->irq;
	if (irq < 8) // per CPU; treat giga as shared interrupt
		MV_REG_WRITE(CPU_INT_SET_MASK_LOCAL_REG, irq);
	else
		MV_REG_WRITE(CPU_INT_CLEAR_ENABLE_REG, irq);
}

void axp_irq_unmask(struct irq_data *d)
{	
	if (d->irq < 8) // per CPU
		MV_REG_WRITE(CPU_INT_CLEAR_MASK_LOCAL_REG, d->irq);
	else
		MV_REG_WRITE(CPU_INT_SET_ENABLE_REG, d->irq);
}

void axp_irq_disable(struct irq_data *d)
{	
	u32 irq=d->irq;
	MV_REG_WRITE(CPU_INT_CLEAR_ENABLE_REG, irq);
}

void axp_irq_enable(struct irq_data *d)
{	
	u32 irq=d->irq;
	MV_REG_WRITE(CPU_INT_SET_ENABLE_REG, irq);
}

#ifdef CONFIG_SMP
int axp_set_affinity(struct irq_data *d, const struct cpumask *mask_val,bool force)
{
	MV_U32 addr, temp;
	u32 irq=d->irq;
	addr = (CPU_INT_SOURCE_CONTROL_REG(irq));

	spin_lock(&irq_controller_lock);
	cpumask_copy(d->affinity, mask_val);
	d->node = cpumask_first(mask_val);
	temp = MV_REG_READ(addr);
	temp &= ~0xf;
	temp |= *cpus_addr(*mask_val);
	MV_REG_WRITE(addr, temp);
	spin_unlock(&irq_controller_lock);

	return 0;
}
#endif
#else /* CONFIG_ARMADAXP_USE_IRQ_INDIRECT_MODE */

void axp_irq_mask(struct irq_data *d)
{	
	u32 irq = d->irq;
	MV_U32 addr, temp, gpio_indx;
	if ((irq >= IRQ_AURORA_GPIO_START) && (irq < IRQ_AURORA_MSI_START)) {
		/* GPIO Interrupts */
		/* calculate index in main interrupt */
		gpio_indx = IRQ_AURORA_GPIO_0_7 + ((irq - IRQ_AURORA_GPIO_START) >> 3);
		/* add 1 because there is a gap between IRQ_AURORA_GPIO_24_31
		   and IRQ_AURORA_GPIO_32_39 */
		if (gpio_indx > IRQ_AURORA_GPIO_24_31)
			gpio_indx++;
		addr = (CPU_INT_SOURCE_CONTROL_REG(gpio_indx));
	} else if (irq >= IRQ_AURORA_MSI_START) {
		/* Per CPU MSI Interrupts */
		if ((irq - IRQ_AURORA_MSI_START) < NR_PRIVATE_MSI_GROUP)
			addr = CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_IN_DRBL_LOW);
		else
			addr = CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_IN_DRBL_HIGH);
	} else 
		addr = CPU_INT_SOURCE_CONTROL_REG(irq);

#ifdef CONFIG_MV_AMP_ENABLE
	mvSemaLock(MV_SEMA_IRQ);
#else
	spin_lock(&irq_controller_lock);
#endif
	temp = MV_REG_READ(addr);

	if ((irq >= IRQ_AURORA_GPIO_START) && (irq < IRQ_AURORA_MSI_START)) {
		MV_U32 bitmask = 1 << (irq & (32-1));
		MV_U32 reg = (irq - IRQ_AURORA_GPIO_START) >> 5;
		MV_REG_BIT_RESET(GPP_INT_LVL_REG(reg), bitmask);
	}

	if (irq <= max_per_cpu_irq) // per CPU
		temp &= ~(1 << hard_smp_processor_id());
	/* for GPIO IRQs , don't disable INTS , they will be disabled in the units mask */
	else if (irq < IRQ_MAIN_INTS_NUM)
		temp &= ~0xf;
	
	MV_REG_WRITE(addr, temp);
#ifdef CONFIG_MV_AMP_ENABLE
	mvSemaUnlock(MV_SEMA_IRQ);
#else
	spin_unlock(&irq_controller_lock);
#endif
}

void axp_irq_unmask(struct irq_data *d)
{	
	u32 irq=d->irq;
	MV_U32 addr, temp, gpio_indx;
	unsigned int map = 0x1;
	if ((irq >= IRQ_AURORA_GPIO_START) && (irq < IRQ_AURORA_MSI_START)) {
		/* GPIO Interrupts */
		/* calculate index in main interrupt */
		gpio_indx = IRQ_AURORA_GPIO_0_7 + ((irq - IRQ_AURORA_GPIO_START) >> 3);
		/* add 1 because there is a gap between IRQ_AURORA_GPIO_24_31
		   and IRQ_AURORA_GPIO_32_39 */
		if (gpio_indx > IRQ_AURORA_GPIO_24_31)
			gpio_indx++;
		addr = (CPU_INT_SOURCE_CONTROL_REG(gpio_indx));
	} else if (irq >= IRQ_AURORA_MSI_START) {
		/* Per CPU MSI Interrupts */
		if ((irq - IRQ_AURORA_MSI_START) < NR_PRIVATE_MSI_GROUP)
			addr = CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_IN_DRBL_LOW);
		else
			addr = CPU_INT_SOURCE_CONTROL_REG(IRQ_AURORA_IN_DRBL_HIGH);
	} else 
		addr = CPU_INT_SOURCE_CONTROL_REG(irq);

#ifdef CONFIG_MV_AMP_ENABLE
	mvSemaLock(MV_SEMA_IRQ);
#else
	spin_lock(&irq_controller_lock);
#endif
	temp = MV_REG_READ(addr);

	if (irq >= IRQ_AURORA_GPIO_START) {
		MV_U32 bitmask = 1 << (irq & (32-1));
		MV_U32 reg = (irq - IRQ_AURORA_GPIO_START) >> 5;
		MV_REG_BIT_SET(GPP_INT_LVL_REG(reg), bitmask);
	}
#ifdef CONFIG_SMP
	else{
		map = get_hw_cpu_mask(*cpus_addr(*(d->affinity)));
	}
#endif
	//temp &= ~0xf;
	temp |= map;
	temp |= (0x1 << 28); /* Set IntEn for this source */
	MV_REG_WRITE(addr, temp);
#ifdef CONFIG_MV_AMP_ENABLE
	mvSemaUnlock(MV_SEMA_IRQ);
#else
	spin_unlock(&irq_controller_lock);
#endif
}


#ifdef CONFIG_SMP
int axp_set_affinity(struct irq_data *d, const struct cpumask *mask_val,bool force)
{
	cpumask_copy((*d).affinity, mask_val);
	spin_lock(&irq_controller_lock);
	(*d).node = cpumask_first(mask_val);
	spin_unlock(&irq_controller_lock);
	axp_irq_unmask(d);
	return 0;
}
#endif
#endif /* CONFIG_ARMADAXP_USE_IRQ_INDIRECT_MODE */

#ifdef CONFIG_SMP
void second_cpu_init(void)
{

	struct irq_data *d = irq_get_irq_data(IRQ_AURORA_IN_DRBL_LOW);
	unsigned long temp;
 	/* open IPI mask */
	temp = MV_REG_READ(AXP_IN_DRBEL_MSK) | 0xff;
	MV_REG_WRITE(AXP_IN_DRBEL_MSK, temp);

	axp_irq_unmask(d);
}
#endif

static struct irq_chip axp_irq_chip = {
	.name		= "axp_irq",
	.irq_mask		= axp_irq_mask,
	.irq_mask_ack	= axp_irq_mask,
	.irq_unmask		= axp_irq_unmask,
#ifdef CONFIG_ARMADAXP_USE_IRQ_INDIRECT_MODE
	.irq_disable	= axp_irq_disable,
	.irq_enable		= axp_irq_enable,
#else
	.irq_disable	= axp_irq_mask,
	.irq_enable		= axp_irq_unmask,
#endif
#ifdef CONFIG_SMP
	.irq_set_affinity   = axp_set_affinity,
#endif
};


void __init axp_init_irq(void)
{
	u32 irq;	

	/* MASK all interrupts */
	/* Enable IRQ in control register */
	for (irq = 0; irq < IRQ_MAIN_INTS_NUM; irq++) {
		axp_irq_mask(irq_get_irq_data(irq));
#ifndef CONFIG_SMP
#ifdef CONFIG_ARMADAXP_USE_IRQ_INDIRECT_MODE
		MV_REG_WRITE(CPU_INT_CLEAR_MASK_LOCAL_REG, irq);
#endif
#endif

	}
	/*
	 * Register IRQ sources
	 */
	for (irq = 0; irq < IRQ_AURORA_MSI_START ; irq++) {
		irq_set_chip(irq, &axp_irq_chip);
		irq_set_chip_data(irq, 0); 
		irq_set_handler(irq, handle_level_irq);
		//irq_desc[irq].status |= IRQ_LEVEL;
		irq_set_status_flags(irq,IRQ_LEVEL);
		set_irq_flags(irq, IRQF_VALID);
	}

#ifdef CONFIG_SMP
	{
		u32/*void __iomem **/addr;
        	/* Set the default affinity to the boot cpu. */
        	cpumask_clear(irq_default_affinity);
        	cpumask_set_cpu(smp_processor_id(), irq_default_affinity);
		/* open IPI mask */
		/* this  register write does the job of axp_irq_unmask(IRQ_AURORA_IN_DRBL_LOW)
		   i.e. enable / unmask the DRBL_LOW interrupt.
		*/
	        MV_REG_WRITE(CPU_INT_CLEAR_MASK_LOCAL_REG, 0);
		addr = /*(void __iomem *)*/(AXP_IN_DRBEL_MSK);
		MV_REG_WRITE(addr, 0xf0ff); // only IPI 0
	}
#endif
 
	armada_msi_init();

}

int pmu_request_irq(int irq, irq_handler_t handler)
{
	int i;
	int ret = request_irq(irq, handler, IRQF_DISABLED | IRQF_NOBALANCING, "armpmu", NULL);
	if (!ret) {
		for_each_online_cpu(i) {
			axp_unmask_fabric_interrupt(i);
		}
	}
	return ret;
}

void pmu_free_irq(int irq)
{
	int i;
	for_each_online_cpu(i) {
		axp_mask_fabric_interrupt(i);
	}
	free_irq(irq, NULL);
}

