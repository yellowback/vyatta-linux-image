/*
 * arch/arm/plat-armada/armadaxp_suspend.S
 *
 * CPU idle low level implementation for Marvell ARMADA-XP SoCs
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 */
#include <linux/linkage.h>
#include <asm/assembler.h>
#include <mach/armadaxp.h>

/*
* armadaxp_cpu_suspend: enter cpu deepIdle state
* input:
*/
ENTRY(armadaxp_cpu_suspend)
	/* Save ARM registers */
	stmfd	sp!, {r4-r12, lr}		@ save registers on stack
        /*
         * Save the CP15 context
         */
        mrc     p15, 0, r2, c1, c0, 0           @ save CP15 - CR
        mrc     p15, 0, r3, c3, c0, 0           @ save CP15 - DACR
        mrc     p15, 0, r4, c13, c0, 0          @ save CP15 - FCSE
        mrc     p15, 0, r5, c2, c0, 0           @ save CP15 - TTBR
        mrc     p15, 0, r6, c13, c0, 1          @ save CP15 - context ID
        mrc     p15, 1, r7, c15, c1, 0          @ save CP15 - extra features
        mrc     p15, 0, r8, c1, c0, 1           @ save CP15 - Aux CR
        mov     r9, r13                         @ save resume SP
        stmfd   sp!, {r2-r9}
	mrc	p15, 1, r3, c15, c2, 0		@ save CP15 - Aux Func Modes Ctrl 0
	mrc	p15, 1, r4, c15, c1, 2		@ save CP15 - Aux Debug Modes Ctrl 2
        mrc     p15, 1, r5, c15, c1, 1          @ save CP15 - Aux Debug Modes Ctrl 1
        mrc     p15, 0, r6, c9, c14, 0          @ save CP15 - PMC
        mrc     p15, 0, r7, c10, c2, 0          @ save CP15 - PRRR
        mrc     p15, 0, r8, c10, c2, 1          @ save CP15 - NMRR
	
        stmfd   sp!, {r3-r8}

	/*
	* TODO: Save Debug Registers
	*/

        /*
	* Save the physical address of the resume SP
	*/
        mov     r0, sp
        bl      suspend_phys_addr
        ldr     r1, =suspend_saved_sp
#ifdef CONFIG_SMP
        mrc     p15, 0, r2, c0, c0, 5
        and     r2, r2, #15
        str     r0, [r1, r2, lsl #2]
#else
        str     r0, [r1]
#endif
	/*
	* Flush L1 DCache
	*/
#ifdef CONFIG_CPU_V6
	bl v6_flush_kern_cache_all
#elif CONFIG_CPU_V7
	bl v7_flush_kern_cache_all
#else
#error "CPU Arch version not defined!\n"
#endif
	bl armadaxp_fabric_prepare_deepIdle

	/*
	* Issue a Data Synchronization Barrier instruction to ensure that all
	* state saving has been	completed.
	*/
#ifdef CONFIG_CPU_V6
	mcr     p15, 0, r0, c7, c10, 4	@ Data Synchronization Barrier
#elif defined (CONFIG_CPU_V7)
	dsb				@ Data Synchronization Barrier
#endif
	ldr	r2, =(SDRAM_OPERATION_REG - INTER_REGS_BASE)
	ldr	r0, =INTER_REGS_BASE
	orr	r2, r2, r0
	ldr	r0, [r2]
#ifdef CONFIG_CPU_BIG_ENDIAN
	ldr	r3, =0x07000000
#else
	ldr	r3, =0x00000007
#endif
	orr	r0, r0, r3
	str	r0, [r2]
#ifdef CONFIG_CPU_V6
	mcr     p15, 0, r1, c7, c0, 4	@ wait for interrupt
#elif defined (CONFIG_CPU_V7)
	wfi				@ wait for interrupt
#endif
	/* if we reach this point then deepIdle returned from regret mode and cpu
	* state retained
	*/

	mov	r0, #1
	ldmfd   sp!, {r3-r8}
	ldmfd   sp!, {r2-r9}
	
	ldmfd   sp!, {r4-r12, pc}
ENDPROC(armadaxp_cpu_suspend)


ENTRY(armadaxp_cpu_resume)

#ifdef CONFIG_CPU_ENDIAN_BE32
        /* convert CPU to big endian */
        .word 0x100f11ee /* mrc p15, 0, r0, c1, c0 */
        .word 0x800080e3 /* orr r0, r0, #0x80 */
        .word 0x100f01ee /* mcr p15, 0, r0, c1, c0 */
#endif
#ifdef CONFIG_CPU_ENDIAN_BE8
                setend  be
#endif
//	mcr	p15, 0, r0, c8, c7, 0   @ flush I,D TLBs
//	mcr     p15, 0, r0, c7, c6, 0   @ invalidate D cache
	
//wait:	
//	ldr	r0, =0xD0018100
//	ldr	r1, =0x8000000
//	str	r1, [r0]                        @ set 7seg
//	ldr	r1, [r0]
//	b 	wait

#ifdef CONFIG_SMP
        adr     r0, suspend_saved_sp
        mrc     p15, 0, r1, c0, c0, 5
        and     r1, r1, #15
        ldr     r0, [r0, r1, lsl #2]    @ stack phys addr
#else
        ldr     r0, suspend_saved_sp            @ stack phys addr
#endif

	ldmfd   r0!, {r3-r8}

	mcr     p15, 1, r3, c15, c2, 0          @ restore CP15 - Aux Func Modes Ctrl 0
	mcr     p15, 1, r4, c15, c1, 2          @ restore CP15 - Aux Debug Modes Ctrl 2
	mcr     p15, 1, r5, c15, c1, 1          @ restore CP15 - Aux Debug Modes Ctrl 1
	mcr     p15, 0, r6, c9, c14, 0          @ restore CP15 - PMC
	mcr     p15, 0, r7, c10, c2, 0          @ restore CP15 - PRRR
	mcr     p15, 0, r8, c10, c2, 1          @ restore CP15 - NMRR
	ldmfd   r0!, {r2-r9}
	mcr	p15, 0, r8, c1, c0, 1		@ restore CP15 - Aux CR
	mcr	p15, 1, r7, c15, c1, 0		@ restore CP15 - extra features
	mcr	p15, 0, r4, c13, c0, 0		@ restore CP15 - FCSE
	mcr	p15, 0, r3, c3, c0, 0		@ restore CP15 - DACR

	/* load identity page table */
	ldr	r3, identity_page_table_phys
	mcr	p15, 0, r3, c2, c0, 0		@ set CP15 - TTBR
	mov	r3, #0
	mcr	p15, 0, r3, c13, c0, 1          @ set 0 in CP15 - context ID
	mcr	p15, 0, r2, c1, c0, 0		@ restore CP15 - CR  @enable mmu
	mrc     p15, 0, r3, c0, c0, 0           @ read id reg
	ldr	r3, resume2
	mov	pc, r3
ENDPROC(armadaxp_cpu_resume)

	/* stage 2 of the resume function that runs from PAGE_OFFSET virtual space */
ENTRY(armadaxp_cpu_resume2)	
	/* restore original page table*/
	mcr	p15, 0, r5, c2, c0, 0		@ restore CP15 - TTBR
	mcr	p15, 0, r6, c13, c0, 1          @ restore CP15 - context ID
	mcr     p15, 0, r0, c8, c7, 0           @ TLB invalidate
	mov	sp, r9				@ restore virtual sp
	mov	r0, #0
	ldmfd   sp!, {r4-r12, pc}               @ restore SVC registers

ENDPROC(armadaxp_cpu_resume2)
	
resume2:
	.long	armadaxp_cpu_resume2

suspend_saved_sp:
#ifdef CONFIG_SMP
	.rept	CONFIG_NR_CPUS
#endif
	.long	0	@ physical SP saved here
#ifdef CONFIG_SMP
	.endr
#endif
	.global identity_page_table_phys
identity_page_table_phys:
	.long	0
