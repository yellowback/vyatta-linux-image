/*
 * include/asm-arm/arch-aurora/dove.h
 *
 * Generic definitions for Marvell Dove MV88F6781 SoC
 *
 * Author: Tzachi Perelstein <tzachi@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __ASM_ARCH_AURORA_H
#define __ASM_ARCH_AURORA_H

#include <mach/vmalloc.h>

/****************************************************************/
/******************* System Address Mapping *********************/
/****************************************************************/

/*
 * Armada-XP address maps.
 *
 * phys		virt		size
 * e0000000	@runtime	128M	PCIe-0 Memory space
 * e8000000	@runtime	128M	PCIe-1 Memory space
 * f0000000	fab00000	16M	SPI-CS0 (Flash)
 * f1000000	fbb00000	1M	Internal Registers
 * f1100000	fbc00000	1M	PCIe-0 I/O space
 * f1200000	fbd00000	1M	PCIe-1 I/O space
 * f1300000	fbe00000	1M	PCIe-2 I/O space
 * f1400000	fbf00000	1M	PCIe-3 I/O space
 * f1500000	fc000000	1M	PCIe-4 I/O space
 * f1600000	fc100000	1M	PCIe-5 I/O space
 * f1700000	fc200000	1M	PCIe-6 I/O space
 * f1800000	fc300000	1M	PCIe-7 I/O space
 * f1900000	fc400000	1M	PCIe-8 I/O space
 * f1a00000	fc500000	1M	PCIe-9 I/O space
 * f1b00000	fc600000	1M	DMA based UART
 * f4000000	fe700000	1M	Device-CS0
 * f2000000	fc700000	32M	Boot-Device CS (NOR Flash)
 * f4100000	fe800000	1M	Device-CS1 (NOR Flash)
 * f4200000	fe900000	1M	Device-CS2 (NOR Flash)
 * f4300000	fea00000	1M	Device-CS3 (NOR Flash)
 * f4400000	feb00000	1M	CESA SRAM (2 units)
 * f4500000	fec00000	1M	NETA-BM (PNC)
 * fff00000	fed00000	1M	BootROM
 * f4700000	fee00800	1M	PMU Scratch pad
 * f4800000	fef00000	1M	Legacy Nand Flash
 */

/*
 * SDRAM Address decoding
 * These values are dummy. Uboot configures these values.
 */
#define SDRAM_CS0_BASE  		0x00000000
#define SDRAM_CS0_SIZE  		_256M
#define SDRAM_CS1_BASE  		0x10000000
#define SDRAM_CS1_SIZE  		_256M
#define SDRAM_CS2_BASE  		0x20000000
#define SDRAM_CS2_SIZE  		_256M
#define SDRAM_CS3_BASE  		0x30000000
#define SDRAM_CS3_SIZE  		_256M

/*
 * PEX Address Decoding
 * Virtual address not specified - remapped @runtime
 */
#define PEX0_MEM_PHYS_BASE		0xE0000000
#define PEX0_MEM_SIZE			_32M
#define PEX1_MEM_PHYS_BASE		0xE2000000
#define PEX1_MEM_SIZE			_32M
#define PEX2_MEM_PHYS_BASE		0xE4000000
#define PEX2_MEM_SIZE			_32M
#define PEX3_MEM_PHYS_BASE		0xE6000000
#define PEX3_MEM_SIZE			_32M
#define PEX4_MEM_PHYS_BASE		0xE8000000
#define PEX4_MEM_SIZE			_32M
#define PEX5_MEM_PHYS_BASE		0x0	/*TBD*/
#define PEX5_MEM_SIZE			_32M
#define PEX6_MEM_PHYS_BASE		0xEA000000		
#define PEX6_MEM_SIZE			_32M
#define PEX7_MEM_PHYS_BASE		0x0	/*TBD*/
#define PEX7_MEM_SIZE			_32M
#define PEX8_MEM_PHYS_BASE		0xEC000000		
#define PEX8_MEM_SIZE			_32M
#define PEX9_MEM_PHYS_BASE		0xEE000000
#define PEX9_MEM_SIZE			_32M

#define SPI_CS0_PHYS_BASE		0xF0000000
#define SPI_CS0_VIRT_BASE		0xFAB00000
#define SPI_CS0_SIZE			_1M

#ifdef CONFIG_MACH_ARMADA_XP_FPGA
 #define INTER_REGS_PHYS_BASE		0xF1000000
 /* Make sure that no other machines are compiled in */
 #if defined (CONFIG_MACH_ARMADA_XP_DB) || defined (CONFIG_MACH_ARMADA_XP_RDSRV)
 #error	"Conflicting Board Configuration!!"
 #endif
#else
 #define INTER_REGS_PHYS_BASE		0xD0000000
#endif
#define INTER_REGS_BASE			0xFBB00000

#define PEX0_IO_PHYS_BASE		0xF1100000
#define PEX0_IO_VIRT_BASE		0xFBC00000
#define PEX0_IO_SIZE			_1M
#define PEX1_IO_PHYS_BASE		0xF1200000
#define PEX1_IO_VIRT_BASE		0xFBD00000
#define PEX1_IO_SIZE			_1M
#define PEX2_IO_PHYS_BASE		0xF1300000
#define PEX2_IO_VIRT_BASE		0xFBE00000
#define PEX2_IO_SIZE			_1M
#define PEX3_IO_PHYS_BASE		0xF1400000
#define PEX3_IO_VIRT_BASE		0xFBF00000
#define PEX3_IO_SIZE			_1M
#define PEX4_IO_PHYS_BASE		0xF1500000
#define PEX4_IO_VIRT_BASE		0xFC000000
#define PEX4_IO_SIZE			_1M
#define PEX5_IO_PHYS_BASE		0xF1600000
#define PEX5_IO_VIRT_BASE		0xFC100000
#define PEX5_IO_SIZE			_1M
#define PEX6_IO_PHYS_BASE		0xF1700000
#define PEX6_IO_VIRT_BASE		0xFC200000
#define PEX6_IO_SIZE			_1M
#define PEX7_IO_PHYS_BASE		0xF1800000
#define PEX7_IO_VIRT_BASE		0xFC300000
#define PEX7_IO_SIZE			_1M
#define PEX8_IO_PHYS_BASE		0xF1900000
#define PEX8_IO_VIRT_BASE		0xFC400000
#define PEX8_IO_SIZE			_1M
#define PEX9_IO_PHYS_BASE		0xF1A00000
#define PEX9_IO_VIRT_BASE		0xFC500000
#define PEX9_IO_SIZE			_1M

#define UART_REGS_BASE			0xF1B00000
#define UART_VIRT_BASE			0xFC600000
#define UART_SIZE			_1M

#define DEVICE_BOOTCS_PHYS_BASE		0xE8000000
#define DEVICE_BOOTCS_VIRT_BASE		0xFC700000
#define DEVICE_BOOTCS_SIZE		_128M
#define DEVICE_CS0_PHYS_BASE		0xF4000000
#define DEVICE_CS0_VIRT_BASE		0xFE700000
#define DEVICE_CS0_SIZE			_1M
#define DEVICE_CS1_PHYS_BASE		0xF4100000
#define DEVICE_CS1_VIRT_BASE		0xFE800000
#define DEVICE_CS1_SIZE			_1M
#define DEVICE_CS2_PHYS_BASE		0xF4200000
#define DEVICE_CS2_VIRT_BASE		0xFE900000
#define DEVICE_CS2_SIZE			_1M
#define DEVICE_CS3_PHYS_BASE		0xF4300000
#define DEVICE_CS3_VIRT_BASE		0xFEA00000
#define DEVICE_CS3_SIZE			_1M

#define CRYPT_ENG_PHYS_BASE(chan)	((chan == 0) ? 0xC8010000 : 0xF4480000)
#define CRYPT_ENG_VIRT_BASE(chan)	((chan == 0) ? 0xFEB00000 : 0xFEB10000)
#define CRYPT_ENG_SIZE			_64K


#ifdef CONFIG_ARMADA_XP_REV_Z1
#define XOR0_PHYS_BASE                 (INTER_REGS_PHYS_BASE | 0x60800)
#define XOR1_PHYS_BASE                 (INTER_REGS_PHYS_BASE | 0x60900)
#else
#define XOR0_PHYS_BASE			(INTER_REGS_PHYS_BASE | 0x60900)
#define XOR1_PHYS_BASE			(INTER_REGS_PHYS_BASE | 0xF0900)
#endif
#define XOR0_HIGH_PHYS_BASE		(INTER_REGS_PHYS_BASE | 0x60B00)
#define XOR1_HIGH_PHYS_BASE		(INTER_REGS_PHYS_BASE | 0xF0B00)

#define PNC_BM_PHYS_BASE		0xF4500000
#define PNC_BM_VIRT_BASE		0xFEC00000
#define PNC_BM_SIZE			_1M

#define BOOTROM_PHYS_BASE		0xFFF00000
#define BOOTROM_VIRT_BASE		0xFED00000
#define BOOTROM_SIZE			_1M


#define PMU_SCRATCH_PHYS_BASE		0xF4700000
#define PMU_SCRATCH_VIRT_BASE		0xFEE00000
#define PMU_SCRATCH_SIZE		_1M

#define LEGACY_NAND_PHYS_BASE		0xF4800000
#define LEGACY_NAND_VIRT_BASE		0xFEF00000
#define LEGACY_NAND_SIZE		_1M

#define	LCD_PHYS_BASE			(INTER_REGS_PHYS_BASE | 0xE0000)

#define AXP_NFC_PHYS_BASE	(INTER_REGS_PHYS_BASE | 0xD0000)

/*
 * Linux native definitiotns
 */
#define SDRAM_OPERATION_REG		(INTER_REGS_BASE | 0x1418)
#define AXP_UART_PHYS_BASE(port)	(INTER_REGS_PHYS_BASE | 0x12000 + (port * 0x100))
#define DDR_VIRT_BASE			(INTER_REGS_BASE | 0x00000)
#define AXP_BRIDGE_VIRT_BASE		(INTER_REGS_BASE | 0x20000)
#define AXP_BRIDGE_PHYS_BASE		(INTER_REGS_PHYS_BASE | 0x20000)
#define DDR_WINDOW_CPU_BASE		(DDR_VIRT_BASE | 0x1500)
#define AXP_SW_TRIG_IRQ			(AXP_BRIDGE_VIRT_BASE | 0x0A04)
#define AXP_SW_TRIG_IRQ_PHYS		(AXP_BRIDGE_PHYS_BASE | 0x0A04)
#define AXP_SW_TRIG_IRQ_CPU_TARGET_OFFS	8
#define AXP_SW_TRIG_IRQ_INITID_MASK	0x1F
#define AXP_PER_CPU_BASE		(AXP_BRIDGE_VIRT_BASE | 0x1000)
#define AXP_IRQ_VIRT_BASE		(AXP_PER_CPU_BASE)
#define AXP_CPU_INTACK			0xB4
#define AXP_IRQ_SEL_CAUSE_OFF		0xA0
#define AXP_IN_DOORBELL_CAUSE		0x78
#define AXP_IN_DRBEL_CAUSE			(AXP_PER_CPU_BASE | 0x78)
#define AXP_IN_DRBEL_MSK			(AXP_PER_CPU_BASE | 0x7c)

#ifdef CONFIG_MACH_ARMADA_XP_FPGA
#define AXP_CPU_RESUME_ADDR_REG(cpu)	(AXP_BRIDGE_VIRT_BASE | 0x984)
#else
#define AXP_CPU_RESUME_ADDR_REG(cpu)	(AXP_BRIDGE_VIRT_BASE | (0x2124+(cpu)*0x100))
#endif
#define AXP_CPU_RESUME_CTRL_REG		(AXP_BRIDGE_VIRT_BASE | 0x988)
#define AXP_CPU_RESET_REG(cpu)		(AXP_BRIDGE_VIRT_BASE | (0x800+(cpu)*8))
#define AXP_CPU_RESET_OFFS		0

#define AXP_L2_CLEAN_WAY_REG		(INTER_REGS_BASE | 0x87BC) 
#define AXP_L2_MNTNC_STAT_REG		(INTER_REGS_BASE | 0x8704)
#define AXP_SNOOP_FILTER_PHYS_REG		(INTER_REGS_PHYS_BASE | 0x21020)

#endif
