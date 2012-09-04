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
#ifndef __CORE_CPU_H
#define __CORE_CPU_H

#include "mv_config.h"

#define MV_PCI_BAR                              2
#define FLASH_BAR_NUMBER                        2
#define MV_PCI_BAR_IO                           2

enum cpu_regs {
	CPU_MAIN_INT_CAUSE_REG        = 0x10200,
	_CPU_MAIN_IRQ_MASK_REG        = 0x10204,
	_CPU_MAIN_FIQ_MASK_REG        = 0x10208,
	_CPU_ENPOINT_MASK_REG         = 0x1020C,
	CPU_MAIN_IRQ_MASK_REG         = _CPU_ENPOINT_MASK_REG,
};

enum cpu_regs_bits {
	/* CPU_MAIN_INT_CAUSE_REG (R10200H) bits */
	INT_MAP_PCIE_ERR            = (1U << 31),
	INT_MAP_I2O_ERR             = (1U << 30),
	INT_MAP_COM_ERR             = (1U << 29),
	INT_MAP_PCIE_LNKUP          = (1U << 28),
	INT_MAP_SGPIO               = (1U << 25),
	INT_MAP_TWSI                = (1U << 24),
	INT_MAP_PFLASH              = (1U << 23),
	INT_MAP_UART                = (1U << 22),
	INT_MAP_GPIO_TEST           = (1U << 21),
	INT_MAP_CPU_CRTL            = (1U << 20),
	INT_MAP_BRIDGE            = (INT_MAP_CPU_CRTL),
	INT_MAP_SASINTB             = (1U << 19),
	INT_MAP_SASINTA             = (1U << 18),
	INT_MAP_SAS                 = (INT_MAP_SASINTA | INT_MAP_SASINTB),
	INT_MAP_XOR_2_3             = (1U << 17),
	INT_MAP_XOR_0_1             = (1U << 16),
	INT_MAP_XOR                 = (INT_MAP_XOR_0_1 | INT_MAP_XOR_2_3),
	INT_MAP_DL_CPU2PCIE3        = (1U << 15),
	INT_MAP_DL_CPU2PCIE2        = (1U << 14),
	INT_MAP_DL_CPU2PCIE1        = (1U << 13),
	INT_MAP_DL_CPU2PCIE0        = (1U << 12),
	INT_MAP_DL_PCIE32CPU        = (1U << 11),
	INT_MAP_DL_PCIE22CPU        = (1U << 10),
	INT_MAP_DL_PCIE12CPU        = (1U << 9),
	INT_MAP_DL_PCIE02CPU        = (1U << 8),
	INT_MAP_COM3OUT             = (1U << 7),
	INT_MAP_COM2OUT             = (1U << 6),
	INT_MAP_COM1OUT             = (1U << 5),
	INT_MAP_COM0OUT             = (1U << 4),
	INT_MAP_COM3IN              = (1U << 3),
	INT_MAP_COM2IN              = (1U << 2),
	INT_MAP_COM1IN              = (1U << 1),
	INT_MAP_COM0IN              = (1U << 0),
	INT_MAP_COM3INT             = (INT_MAP_COM3IN | INT_MAP_COM_ERR),
	INT_MAP_COM2INT             = (INT_MAP_COM2IN | INT_MAP_COM_ERR),
	INT_MAP_COM1INT             = (INT_MAP_COM1IN | INT_MAP_COM_ERR),
	INT_MAP_COM0INT             = (INT_MAP_COM0IN | INT_MAP_COM_ERR),
	INT_MAP_COMINT              = (INT_MAP_COM0INT | INT_MAP_COM1INT
	                             | INT_MAP_COM2INT | INT_MAP_COM3INT),
	INT_MAP_MU                  = (INT_MAP_DL_PCIE02CPU | INT_MAP_COM0INT),
};

enum xbar_address_map {
	INTRFC_PCIEA            = 0x00,
	INTRFC_SRAM             = 0x01,
	INTRFC_DDR_CS0          = 0x02,
	INTRFC_PBSRAM           = 0x03,
	INTRFC_PCIE1            = 0x04,
	INTRFC_DDR_CS1          = 0x06,
	INTRFC_PCIE2            = 0x08,
	INTRFC_DDR_CS2          = 0x0A,
	INTRFC_PCIE3            = 0x0C,
	INTRFC_DDR_CS3          = 0x0E,
	INTRFC_CORE_DMA          = (INTRFC_DDR_CS3),
};

#define CS_INTRFC_CORE_DMA 0

#endif /* __CORE_CPU_H */

