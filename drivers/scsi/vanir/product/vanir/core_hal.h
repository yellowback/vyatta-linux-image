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
#ifndef __CORE_HAL_H
#define __CORE_HAL_H

#include "mv_config.h"
#include "core_type.h"
#include "core_discover.h"
#include "core_cpu.h"

#define MAX_NUMBER_IO_CHIP                      2 /* how many io chips per hardware */
#define MAX_REGISTER_SET_PER_IO_CHIP            64
#define MAX_PORT_PER_PL                         4
#define MAX_PHY_PER_PL                          4
#define CORE_MAX_REQUEST_NUMBER                 2000 /* per io chip */


typedef enum _VANIR_REVISION_ID {
	VANIR_A0_REV		= 0xA0,
	VANIR_B0_REV		= 0x01,
	VANIR_C0_REV		= 0x02,
	VANIR_C1_REV		= 0x03,
	VANIR_C2_REV		= 0xC2,
} VANIR_REVISION_ID;
/*
  ========================================================================
chip registers
  ========================================================================
*/
#define MV_IO_CHIP_REGISTER_BASE                                     0x20000
#define MV_IO_CHIP_REGISTER_RANGE                                    0x04000

enum vanir_common_regs {
	/* SATA/SAS port common registers */
	COMMON_PORT_IMPLEMENT        = 0x9C,  /* port implement register */
	COMMON_PORT_TYPE        = 0xa0,  /* port type register */
	COMMON_CONFIG           = 0x100, /* configuration register */
	COMMON_CONTROL          = 0x104, /* control register */
	COMMON_LST_ADDR         = 0x108, /* command list DMA addr */
	COMMON_LST_ADDR_HI      = 0x10c, /* command list DMA addr hi */
	COMMON_FIS_ADDR         = 0x110, /* FIS rx buf addr */
	COMMON_FIS_ADDR_HI      = 0x114, /* FIS rx buf addr hi */

	COMMON_SATA_REG_SET0    = 0x0118, /* SATA/STP Register Set 0 */
	COMMON_SATA_REG_SET1    = 0x011c, /* SATA/STP Register Set 1 */

	COMMON_DELV_Q_CONFIG    = 0x120, /* delivery queue configuration */
	COMMON_DELV_Q_ADDR      = 0x124, /* delivery queue base address */
	COMMON_DELV_Q_ADDR_HI   = 0x128, /* delivery queue base address hi */
	COMMON_DELV_Q_WR_PTR    = 0x12c, /* delivery queue write pointer */
	COMMON_DELV_Q_RD_PTR    = 0x130, /* delivery queue read pointer */
	COMMON_CMPL_Q_CONFIG    = 0x134, /* completion queue configuration */
	COMMON_CMPL_Q_ADDR      = 0x138, /* completion queue base address */
	COMMON_CMPL_Q_ADDR_HI   = 0x13c, /* completion queue base address hi */
	COMMON_CMPL_Q_WR_PTR    = 0x140, /* completion queue write pointer */
	COMMON_CMPL_Q_RD_PTR    = 0x144, /* completion queue read pointer */

	COMMON_COAL_CONFIG      = 0x148, /* interrupt coalescing config */
	COMMON_COAL_TIMEOUT     = 0x14c, /* interrupt coalescing time wait */
	COMMON_IRQ_STAT         = 0x150, /* interrupt status */
	COMMON_IRQ_MASK         = 0x154, /* interrupt enable/disable mask */

	COMMON_SRS_IRQ_STAT0    = 0x0158, /* SRS interrupt status 0 */
	COMMON_SRS_IRQ_MASK0    = 0x015c, /* SRS intr enable/disable mask 0 */
	COMMON_SRS_IRQ_STAT1    = 0x0160, /* SRS interrupt status 1*/
	COMMON_SRS_IRQ_MASK1    = 0x0164, /* SRS intr enable/disable mask 1*/

	COMMON_NON_SPEC_NCQ_ERR0= 0x0168, /* Non Specific NCQ Error 0 */
	COMMON_NON_SPEC_NCQ_ERR1= 0x016c, /* Non Specific NCQ Error 1 */

	COMMON_CMD_ADDR         = 0x0170, /* Command Address Port */
	COMMON_CMD_DATA         = 0x0174, /* Command Data Port */

	/* Port interrupt status/mask register set $i (0x180/0x184-0x1b8/0x1bc) */
	COMMON_PORT_IRQ_STAT0   = 0x0180,
	COMMON_PORT_IRQ_MASK0   = 0x0184,

	COMMON_PORT_ALL_IRQ_STAT   = 0x01c0, /* All Port interrupt status */
	COMMON_PORT_ALL_IRQ_MASK   = 0x01c4, /* All Port interrupt enable/disable mask */

	COMMON_STP_CLR_AFFILIATION_DIS0= 0x01c8, /* STP Clear Affiliation Disable 0 */
	COMMON_STP_CLR_AFFILIATION_DIS1= 0x01cc, /* STP Clear Affiliation Disable 1 */

	/* port serial control/status register set $i (0x1d0-0x1ec) */
	COMMON_PORT_PHY_CONTROL0  = 0x01d0,
	COMMON_PORT_ALL_PHY_CONTROL=0x01f0, /* All port serial status/control */

	/* port config address/data regsiter set $i (0x200/0x204 - 0x238/0x23c) */
	COMMON_PORT_CONFIG_ADDR0  = 0x0200,
	COMMON_PORT_CONFIG_DATA0  = 0x0204,
	COMMON_PORT_ALL_CONFIG_ADDR  = 0x0240, /* All Port config address */
	COMMON_PORT_ALL_CONFIG_DATA  = 0x0244, /* All Port config data */

	/* port vendor specific address/data register set $i (0x250/0x254-0x268/0x26c) */
	COMMON_PORT_VSR_ADDR0      = 0x0250,
	COMMON_PORT_VSR_DATA0      = 0x0254,
	COMMON_PORT_ALL_VSR_ADDR   = 0x0290, /* All port Vendor Specific Register addr */
	COMMON_PORT_ALL_VSR_DATA   = 0x0294, /* All port Vendor Specific Register Data */
};

/* these registers are accessed through port vendor specific address/data registers */
enum sas_sata_phy_regs {
    GENERATION_1_SETTING        = 0x118,
    GENERATION_1_2_SETTING      = 0x11C,    
    GENERATION_2_3_SETTING      = 0x120,
    GENERATION_3_4_SETTING      = 0x124,    
};

enum vanir_common_regs_bits {
	/* COMMON_PORT_TYPE */
	PORT_AUTO_DET_EN        = (0xFFU << 8),
	PORT_SAS_SATA_MODE      = (0xFFU << 0),

	/* COMMON_CONFIG register bits */
	CONFIG_CMD_TBL_BE       = (1U << 0),
	CONFIG_OPEN_ADDR_BE     = (1U << 1),
	CONFIG_RSPNS_FRAME_BE   = (1U << 2),
	CONFIG_DATA_BE          = (1U << 3),
	CONFIG_SAS_SATA_RST     = (1U << 5),
	CONFIG_STP_STOP_ON_ERR	= (1U << 25),

	/* COMMON_PHY_CTRL register definition */
	PHY_PHY_DSBL            = (0xFU << 12),
	PHY_PWR_OFF             = (0xFU << 24),

	/* COMMON_CONTROL : port control/status bits (R104h) */
	CONTROL_EN_CMD_ISSUE        = (1U << 0),
	CONTROL_RESET_CMD_ISSUE     = (1U << 1),
	CONTROL_ERR_STOP_CMD_ISSUE  = (1U << 3),
	CONTROL_FIS_RCV_EN          = (1U << 4),
	CONTROL_CMD_CMPL_SELF_CLEAR = (1U << 5),
	CONTROL_EN_SATA_RETRY       = (1U << 6),
	CONTROL_RSPNS_RCV_EN        = (1U << 7),
	

	CONTROL_EN_PORT_XMT_START   = 12,

	/* COMMON_DELV_Q_CONFIG (R120h) bits */
	DELV_QUEUE_SIZE_MASK        = (0xFFFU << 0),
	DELV_QUEUE_ENABLE           = (1U << 16),

	/* COMMON_CMPL_Q_CONFIG (R134h) bits */
	CMPL_QUEUE_SIZE_MASK        = (0xFFFU << 0),
	CMPL_QUEUE_ENABLE           = (1U << 16),
	CMPL_QUEUE_DSBL_ATTN_POST	= (1U << 17),

	/* COMMON_COAL_CONFIG (R148h) bits */
	INT_COAL_COUNT_MASK      = (0x1FFU << 0),
	INT_COAL_ENABLE          = (1U << 16),

	/* COMMON_COAL_TIMEOUT (R14Ch) bits */
	COAL_TIMER_MASK          = (0xFFFFU << 0),
	COAL_TIMER_UNIT_1MS      = (1U << 16),   /* 6.67 ns if set to 0 */

	/* COMMON_IRQ_STAT/MASK (R150h) bits */
	INT_CMD_CMPL               = (1U << 0),
	INT_CMD_CMPL_MASK          = (1U << 0),
	INT_CMD_ISSUE_STOPPED      = (1U << 1),
	INT_SRS                    = (1U << 3),

	INT_PORT_MASK_OFFSET       = 8,
	INT_PORT_MASK              = (0xFF << INT_PORT_MASK_OFFSET),
	INT_PHY_MASK_OFFSET        = 4,
	INT_PHY_MASK               = (0x0F << INT_PHY_MASK_OFFSET),
	INT_PORT_STOP_MASK_OFFSET  = 16,
	INT_PORT_STOP_MASK         = (0xFF << INT_PORT_STOP_MASK_OFFSET),

	INT_NON_SPCFC_NCQ_ERR	   = (1U << 25),

	INT_MEM_PAR_ERR            = (1U << 26),
	INT_DMA_PEX_TO			= (1U << 27),
	INT_PRD_BC_ERR			= (1U << 28),

	/* COMMON_PORT_IRQ_STAT/MASK (R160h) bits */
	IRQ_PHY_RDY_CHNG_MASK         = (1U << 0),
	IRQ_HRD_RES_DONE_MASK         = (1U << 1),
	IRQ_PHY_ID_DONE_MASK          = (1U << 2),
	IRQ_PHY_ID_FAIL_MASK          = (1U << 3),
	IRQ_PHY_ID_TIMEOUT            = (1U << 4),
	IRQ_HARD_RESET_RCVD_MASK      = (1U << 5),
	IRQ_PORT_SEL_PRESENT_MASK     = (1U << 6),
	IRQ_COMWAKE_RCVD_MASK         = (1U << 7),
	IRQ_BRDCST_CHNG_RCVD_MASK     = (1U << 8),
	IRQ_UNKNOWN_TAG_ERR           = (1U << 9),
	IRQ_IU_TOO_SHRT_ERR           = (1U << 10),
	IRQ_IU_TOO_LNG_ERR            = (1U << 11),
	IRQ_PHY_RDY_CHNG_1_TO_0       = (1U << 12),
	IRQ_SIG_FIS_RCVD_MASK         = (1U << 16),
	IRQ_BIST_ACTVT_FIS_RCVD_MASK  = (1U << 17),
	IRQ_ASYNC_NTFCN_RCVD_MASK     = (1U << 18),
	IRQ_UNASSOC_FIS_RCVD_MASK     = (1U << 19),
	IRQ_STP_SATA_RX_ERR_MASK      = (1U << 20),
	IRQ_STP_SATA_TX_ERR_MASK      = (1U << 21),
	IRQ_STP_SATA_CRC_ERR_MASK     = (1U << 22),
	IRQ_STP_SATA_DCDR_ERR_MASK    = (1U << 23),
	IRQ_STP_SATA_PHY_DEC_ERR_MASK = (1U << 24),
	IRQ_STP_SATA_SYNC_ERR_MASK    = (1U << 25),

	/* common port serial control/status (R180h) bits */
	SCTRL_STP_LINK_LAYER_RESET        = (1 << 0),
	SCTRL_PHY_HARD_RESET_SEQ          = (1 << 1),
	SCTRL_PHY_BRDCST_CHNG_NOTIFY      = (1 << 2),
	SCTRL_SSP_LINK_LAYER_RESET        = (1 << 3),
	SCTRL_MIN_SPP_PHYS_LINK_RATE_MASK = (0xF << 8),
	SCTRL_MAX_SPP_PHYS_LINK_RATE_MASK = (0xF << 12),
	SCTRL_NEG_SPP_PHYS_LINK_RATE_MASK_OFFSET = 16,
	SCTRL_NEG_SPP_PHYS_LINK_RATE_MASK = (0xF << SCTRL_NEG_SPP_PHYS_LINK_RATE_MASK_OFFSET),
	SCTRL_PHY_READY_MASK              = (1 << 20),
};


/* COMMON_DELV_Q_RD_PTR bits */
typedef struct _REG_COMMON_DELV_Q_RD_PTR
{
#ifdef __MV_BIG_ENDIAN_BITFIELD__
	MV_U32   Reserved:20;
	MV_U32   DELV_QUEUE_RD_PTR:12;
#else
	MV_U32   DELV_QUEUE_RD_PTR:12;
	MV_U32   Reserved:20;
#endif /* __MV_BIG_ENDIAN_BITFIELD__ */
} REG_COMMON_DELV_Q_RD_PTR, *PREG_COMMON_DELV_Q_RD_PTR;


/* COMMON_CMPL_Q_WR_PTR bits */
typedef struct _REG_COMMON_CMPL_Q_WR_PTR
{
#ifdef __MV_BIG_ENDIAN_BITFIELD__
	MV_U32   Reserved:20;
	MV_U32   CMPLN_QUEUE_WRT_PTR:12;
#else
	MV_U32   CMPLN_QUEUE_WRT_PTR:12;
	MV_U32   Reserved:20;
#endif /* __MV_BIG_ENDIAN_BITFIELD__ */
} REG_COMMON_CMPL_Q_WR_PTR, *PREG_COMMON_CMPL_Q_WR_PTR;

/* sas/sata command port registers */
enum cmd_regs {
	/* cmd pl timer */
	CMD_PL_TIMER = 0x138,

	/* cmd port layer timer 1 */
	CMD_PORT_LAYER_TIMER1 = 0x1E0,

        /* link timer */
        CMD_LINK_TIMER = 0x1E4,

	/* cmd port active register $i ((0x300-0x3ff) */
	CMD_PORT_ACTIVE0  = 0x300,

	/* SATA register set $i (0x800-0x8ff) task file data register */
	CMD_SATA_TFDATA0  = 0x800,

	/* SATA register set $i (0xa00-0xaff) association reg */
	CMD_SATA_ASSOC0   = 0xa00,
};

/* sas/sata configuration port registers */
enum config_regs {
	CONFIG_SATA_CONTROL    = 0x18, /* port SATA control register */
	CONFIG_PHY_CONTROL     = 0x1c, /* port phy control register */

	CONFIG_SATA_SIG0       = 0x20, /* port SATA signature FIS(Byte 0-3) */
	CONFIG_SATA_SIG1       = 0x24, /* port SATA signature FIS(Byte 4-7) */
	CONFIG_SATA_SIG2       = 0x28, /* port SATA signature FIS(Byte 8-11) */
	CONFIG_SATA_SIG3       = 0x2c, /* port SATA signature FIS(Byte 12-15)*/
	CONFIG_R_ERR_COUNT     = 0x30, /* port R_ERR count register */
	CONFIG_CRC_ERR_COUNT   = 0x34, /* port CRC error count register */
	CONFIG_WIDE_PORT       = 0x38, /* port wide participating register */

	CONFIG_CRN_CNT_INFO0   = 0x80, /* port current connection info register 0*/
	CONFIG_CRN_CNT_INFO1   = 0x84, /* port current connection info register 1*/
	CONFIG_CRN_CNT_INFO2   = 0x88, /* port current connection info register 2*/
	CONFIG_ID_FRAME0       = 0x100, /* Port device ID frame register 0, DEV Info*/
	CONFIG_ID_FRAME1       = 0x104, /* Port device ID frame register 1*/
	CONFIG_ID_FRAME2       = 0x108, /* Port device ID frame register 2*/
	CONFIG_ID_FRAME3       = 0x10c, /* Port device ID frame register 3, SAS Address lo*/
	CONFIG_ID_FRAME4       = 0x110, /* Port device ID frame register 4, SAS Address hi*/
	CONFIG_ID_FRAME5       = 0x114, /* Port device ID frame register 5, Phy Id*/
	CONFIG_ID_FRAME6       = 0x118, /* Port device ID frame register 6*/
	CONFIG_ATT_ID_FRAME0   = 0x11c, /* attached device ID frame register 0*/
	CONFIG_ATT_ID_FRAME1   = 0x120, /* attached device ID frame register 1*/
	CONFIG_ATT_ID_FRAME2   = 0x124, /* attached device ID frame register 2*/
	CONFIG_ATT_ID_FRAME3   = 0x128, /* attached device ID frame register 3*/
	CONFIG_ATT_ID_FRAME4   = 0x12c, /* attached device ID frame register 4*/
	CONFIG_ATT_ID_FRAME5   = 0x130, /* attached device ID frame register 5*/
	CONFIG_ATT_ID_FRAME6   = 0x134, /* attached device ID frame register 6*/
};

enum config_regs_bits {
	/* CONFIG_DEV_INFO/CONFIG_ATT_DEV_INFO (0x00) bits */
	PORT_DEV_TYPE_MASK     = (0x7U << 0),
	PORT_DEV_INIT_MASK     = (0x7U << 9),
	PORT_DEV_TRGT_MASK     = (0x7U << 17),

	PORT_DEV_SMP_INIT      = (1U << 9),
	PORT_DEV_STP_INIT      = (1U << 10),
	PORT_DEV_SSP_INIT      = (1U << 11),
	PORT_DEV_SMP_TRGT      = (1U << 17),
	PORT_DEV_STP_TRGT      = (1U << 18),
	PORT_DEV_SSP_TRGT      = (1U << 19),

	PORT_PHY_ID_MASK       = (0xFFU << 24),

	/* CONFIG_PHY_STATUS (0x1c) bits */
	PORT_PHY_OOB_DTCTD	   = (1U << 0),
	PORT_PHY_DW_SYNC	   = (1U << 1),
	PORT_PHY_RDY		   = (1U << 2),
	PORT_PHY_NGTD_SPEED	   = (1U << 4),
	PORT_PHY_IMP_CAL_DONE  = (1U << 5),
	PORT_PHY_KVCO		   = (0x7U << 6),
	PORT_PHY_PLL_LOCK	   = (1U << 9),
	PORT_PHY_PLL_CAL_DONE  = (1U << 10),
	PORT_PHY_RXIMP		   = (0xFU << 11),
	PORT_PHY_TXIMP		   = (0xFU << 15),

	/* CONFIG_WIDE_PORT (0x38) bits */
	WIDE_PORT_PHY_MASK     = (0xF << 0), /* phy map in a wide port */
};

/* Power Mode PM_CNTRL/PMODE */
#define PMODE_PARTIAL      0x10
#define PMODE_SLUMBER      0x01

/* CONFIG_R_ERR_COUNT/CONFIG_CRC_ERR_COUNT bits */
#define ERR_COUNT_MASK      0x0ffff

/* sas/sata vendor specific port registers */
enum vsr_regs {
	VSR_IRQ_STATUS      = 0x00,
	VSR_IRQ_MASK        = 0x04,
	VSR_PHY_CONFIG      = 0x08,
	VSR_PHY_STATUS      = 0x0c,

	VSR_PHY_MODE_REG_1	= 0x064,
	VSR_PHY_FFE_CONTROL	= 0x10C,
	VSR_PHY_DFE_UPDATE_CRTL	= 0x110,
	VSR_REF_CLOCK_CRTL	= 0x1A0,
};

enum vsr_reg_bits {
	/* VSR_IRQ_STATUS bits */
	VSR_IRQ_PHY_TIMEOUT     = (1U << 10),

	/* VSR_PHY_STATUS bits */
	VSR_PHY_STATUS_MASK     = 0x3f0000,
	VSR_PHY_STATUS_IDLE     = 0x00,
	VSR_PHY_STATUS_SAS_RDY  = 0x10,
	VSR_PHY_STATUS_HR_RDY   = 0x1d,
};

enum mv_pci_regs {
	MV_PCI_REG_CMD      = 0x04,
	MV_PCI_REG_DEV_CTRL = 0x78,
	MV_PCI_REG_MSI_CTRL = 0x50,

	MV_PCI_REG_ADDRESS_BASE = 0x8000,

	CORE_NVSRAM_MAPPING_BASE = 0x0,
	MV_PCI_REG_WIN0_CTRL = (MV_PCI_REG_ADDRESS_BASE + 0x420),
	MV_PCI_REG_WIN0_BASE = (MV_PCI_REG_ADDRESS_BASE + 0x424),
        MV_PCI_REG_WIN0_REMAP = (MV_PCI_REG_ADDRESS_BASE + 0x428),

	MV_PCI_REG_INT_CAUSE = (MV_PCI_REG_ADDRESS_BASE + 0x600),
	MV_PCI_REG_INT_ENABLE = (MV_PCI_REG_ADDRESS_BASE + 0x604),
};

enum mv_pci_regs_bits {
	/* MV_PCI_REG_MSI_CTRL bits */
	MV_PCI_MSI_EN       = (1 << 16),

	/* MV_PCI_REG_DEV_CTRL bits */
	MV_PCI_IO_EN        = (1 << 0),
	MV_PCI_MEM_EN       = (1 << 1),
	MV_PCI_BM_EN        = (1 << 2),    /* enable bus master */
	MV_PCI_INT_DIS      = (1 << 10),   /* disable INTx for MSI_EN */
	MV_PCI_DEV_EN       = MV_PCI_IO_EN | MV_PCI_MEM_EN | MV_PCI_BM_EN,
	MV_PCI_RD_REQ_SIZE  = 0x2000,
	MV_PCI_RD_REQ_MASK  = 0x00007000,
};

enum mv_sgpio_regs{
	SGPIO_REG_BASE    = 0xc200, /* SGPIO register start */
};

enum mv_i2c_regs{
	I2C_SOFTWARE_CONTROL_A = 0xc51c,
	I2C_HARDWARE_CONTROL_A = 0xc520,
	I2C_STATUS_DATA_A      = 0xc524,

	I2C_SOFTWARE_CONTROL_B = 0xc61c,
	I2C_HARDWARE_CONTROL_B = 0xc620,
	I2C_STATUS_DATA_B      = 0xc624,

	I2C_SOFTWARE_CONTROL_C = 0xc71c,
	I2C_HARDWARE_CONTROL_C = 0xc720,
	I2C_STATUS_DATA_C      = 0xc724,

};

enum mv_spi_regs{
	ODIN_SPI_CTRL_REG = 0xc800,
	ODIN_SPI_ADDR_REG = 0xc804,
	ODIN_SPI_WR_DATA_REG	= 0xc808,
	ODIN_SPI_RD_DATA_REG = 0xc80c,
};
enum mv_spi_reg_bit{
	SPI_CTRL_READ = MV_BIT( 2 ),
	SPI_CTRL_AddrValid = MV_BIT( 1 ),
	SPI_CTRL_SpiStart = MV_BIT( 0 ),
};

/* for buzzer */
enum test_pin_regs {
	TEST_PIN_OUTPUT_VALUE = 0x10068,
	TEST_PIN_OUTPUT_ENABLE = 0x1006C,
	TEST_PIN_BUZZER		= MV_BIT(2),
};


/*
  ========================================================================
	Software data structures/macros
  ========================================================================
*/

#define MAX_SSP_RESP_SENSE_SIZE      sizeof(MV_Sense_Data)
#define MAX_SMP_RESP_SIZE			 1016

/*
 * Hardware related format. Never change their size. Must follow hardware
 * specification.
 */
enum _mv_command_header_bit_ops {
	CH_BIST=  (1UL << 4),
	CH_ATAPI = (1UL << 5),
	CH_FPDMA = (1UL << 6),
	CH_RESET = (1UL << 7),
	CH_PI_PRESENT = (1UL <<8),
	CH_SSP_TP_RETRY = (1UL << 9),
	CH_SSP_VERIFY_DATA_LEN = ( 1UL <<10),
	CH_SSP_FIRST_BURST = (1UL << 11),
	CH_SSP_PASS_THRU = (1UL << 12),

	CH_SSP_FRAME_TYPE_SHIFT = 13,
	CH_PRD_TABLE_LEN_SHIFT = 16,

	CH_PM_PORT_MASK = 0xf,

	CH_FRAME_LEN_MASK = 0xff,
	CH_LEAVE_AFFILIATION_OPEN_SHIFT = 9,
	CH_MAX_SIMULTANEOUS_CONNECTIONS_SHIFT = 12,
	CH_MAX_RSP_FRMAE_LEN_MASK = 0x1ff,

	XBAR_CT_CS_SHIFT = 0,
	XBAR_OAF_CS_SHIFT = 4,
	XBAR_SB_CS_SHIFT = 0,
	XBAR_PRD_CS_SHIFT = 4,

	PI_BLK_INDX_FLDS_PRNST = (1UL << 13),
	PI_KEY_TAG_FLDS_PRNST = (1UL << 14),
	PI_T10_FLDS_PRNST = (1UL << 15),
};
/* for command list */
typedef struct _mv_command_header
{
	MV_U32   ctrl_nprd;
	MV_U16 frame_len;
	MV_U16 max_rsp_frame_len;

	MV_U16 tag;
	MV_U16 target_tag;
/* DWORD 3 */
	MV_U32   data_xfer_len;         /* in bytes */
/* DWORD 4-5*/
	_MV_U64  table_addr;
/* DWORD 6-7*/
	_MV_U64  open_addr_frame_addr;
/* DWORD 8-9*/
	_MV_U64  status_buff_addr;
	_MV_U64  prd_table_addr;
	
	MV_U16	 interface_select;
	MV_U16	 pir_fmt;
	MV_U32	 reserved[3];
} mv_command_header;

/* SSP_SSPFrameType */
#define FRAME_TYPE_COMMAND         0x00
#define FRAME_TYPE_TASK            0x01
#define FRAME_TYPE_XFER_RDY         0x04 /* Target mode */
#define FRAME_TYPE_RESPONSE         0x05 /* Target mode */
#define FRAME_TYPE_RD_DATA         0x06 /* Target mode */
#define FRAME_TYPE_RD_DATA_RESPONSE   0x07 /* Target mode, read data after response frame */
/* Tag */
#define SSP_I_HEADR_TAG_MASK      0xfe00   /* lower 9 bits from HW command slot */
#define SSP_T_HEADR_TAG_MASK      0xffff
#define STP_HEADR_TAG_MASK         0x001f   /* NCQ uses lower 5 bits  */
/* TargetTag */
#define SSP_T_HEADR_TGTTAG_MASK      0xfe00   /* lower 9 bits from HW command slot */

/* for command table */
/* SSP frame header */
typedef struct _ssp_frame_header
{
	MV_U8   frame_type;
	MV_U8   hashed_dest_sas_addr[3];
	MV_U8   reserved1;
	MV_U8   hashed_src_sas_addr[3];
	MV_U8   reserved2[2];
	MV_U8 _ssp_c_t_r;
	MV_U8 _ssp_n_f;
	MV_U8   reserved5[4];
	MV_U16  tag;         /* command tag */
	MV_U16  target_tag;      /* Target Port Transfer Tag, for target to tag multiple XFER_RDY */
	MV_U32  data_offset;
}ssp_frame_header;

/* SSP Command UI */
typedef struct _ssp_command_iu
{
	MV_U8   lun[8];
	MV_U8   reserved1;
	MV_U8 _c_iu_ta_fb;
	MV_U8 reserved3;
	MV_U8 _c_iu_e_cdb_len;
	MV_U8   cdb[16];
}ssp_command_iu;

/* SSP TASK UI */
typedef struct _ssp_task_iu
{
	MV_U8   lun[8];
	MV_U8   reserved1[2];
	MV_U8   task_function;
	MV_U8   reserved2;
	MV_U16  tag;
	MV_U8   reserved3[14];
}ssp_task_iu;

/* SSP XFER_RDY UI */
typedef struct _ssp_xferrdy_iu
{
	MV_U32   data_offset;
	MV_U32   data_len;
	MV_U8    reserved3[4];
}ssp_xferrdy_iu;

/* SSP RESPONSE UI */
typedef struct _ssp_response_iu
{
	MV_U8   reserved1[10];
	MV_U8  data_pres;
	MV_U8   status;
	MV_U32  reserved3;
	MV_U32  sense_data_len;
	MV_U32  resp_data_len;
	MV_U8   data[MAX_SSP_RESP_SENSE_SIZE];
}ssp_response_iu;

/* DataPres */
#define NO_SENSE_RESPONSE   0x0
#define RESPONSE_ONLY      0x1
#define SENSE_ONLY         0x2
#define RESERVED         0x3

/* SSP Protection Information Record */
typedef struct _protect_info_record
{
#ifdef __MV_BIG_ENDIAN_BITFIELD__
	/*DWORD 0*/
	MV_U32 USR_DT_SZ:12;
	MV_U32 reserved2:6;
	MV_U32 SKIP_EN:1;
	MV_U32 reserved1:5;
	MV_U32 PRD_DATA_INCL_T10:1;
	MV_U32 INCR_LBAT:1;
	MV_U32 INCR_LBRT:1;
	MV_U32 CHK_DSBL_MD:1;
	MV_U32 T10_CHK_EN:1;
	MV_U32 T10_RPLC_EN:1;
	MV_U32 T10_RMV_EN:1;
	MV_U32 T10_INSRT_EN:1;
	/*DWORD 1,2*/
	MV_U32 LBRT_CHK_VAL;      /* Logical Block Reference Tag */
	MV_U32 LBRT_GEN_VAL;      /* Logical Block Reference Tag Gen Value*/
	/*DWORD 3*/
	MV_U32 LBAT_CHK_MASK:16;	/* Logical Block Application Tag Check Mask*/
	MV_U32 LBAT_CHK_VAL:16;      /* Logical Block Application Tag Check Value*/
	/*DWORD 4*/	
	MV_U32 T10_RPLC_MSK:8;	/*T10 Replace Mask*/
	MV_U32 T10_CHK_MSK:8;	/*T10 Check Mask*/
	MV_U32 LBAT_GEN_VAL:16;      /* Logical Block Application Tag Gen Value*/
	/*DWORD 5,6*/	
	MV_U32 reserved0[2]; /*DW5 and DW6 not required when SKIP_EN is reset.*/
#else
	/*DWORD 0*/
	MV_U32 T10_INSRT_EN:1;
	MV_U32 T10_RMV_EN:1;
	MV_U32 T10_RPLC_EN:1;
	MV_U32 T10_CHK_EN:1;
	MV_U32 CHK_DSBL_MD:1;
	MV_U32 INCR_LBRT:1;
	MV_U32 INCR_LBAT:1;
	MV_U32 PRD_DATA_INCL_T10:1;
	MV_U32 reserved1:5;
	MV_U32 SKIP_EN:1;
	MV_U32 reserved2:6;
	MV_U32 USR_DT_SZ:12;
	/*DWORD 1,2*/
	MV_U32 LBRT_CHK_VAL;      /* Logical Block Reference Tag */
	MV_U32 LBRT_GEN_VAL;      /* Logical Block Reference Tag Gen Value*/
	/*DWORD 3*/
	MV_U32 LBAT_CHK_VAL:16;      /* Logical Block Application Tag Check Value*/
	MV_U32 LBAT_CHK_MASK:16;	/* Logical Block Application Tag Check Mask*/
	/*DWORD 4*/
	MV_U32 LBAT_GEN_VAL:16;      /* Logical Block Application Tag Gen Value*/
	MV_U32 T10_CHK_MSK:8;	/*T10 Check Mask*/
	MV_U32 T10_RPLC_MSK:8;	/*T10 Replace Mask*/
	/*DWORD 5,6*/
	MV_U32 reserved0[2]; /*DW5 and DW6 not required when SKIP_EN is reset.*/
#endif /* __MV_BIG_ENDIAN_BITFIELD__ */
}protect_info_record;


/* SSP Command Table */
typedef struct _mv_ssp_command_table
{
	ssp_frame_header frame_header;
	union
	{
		struct {
			ssp_command_iu command_iu;
			protect_info_record pir;
		} command;
		ssp_task_iu task;
		ssp_xferrdy_iu xfer_rdy;
		ssp_response_iu response;
	} data;
} mv_ssp_command_table;

/* SATA STP Command Table */
typedef struct _mv_sata_stp_command_table
{
	MV_U8   fis[64];                        /* Command FIS */
	MV_U8   atapi_cdb[32];                     /* ATAPI CDB */
} mv_sata_stp_command_table;

#define OF_MODE_SHIFT 7
#define OF_MODE_TARGET 0x0
#define OF_MODE_INITIATOR 0x1
#define OF_PROT_TYPE_SHIFT 4
/* Open Address Frame */
typedef struct _open_addr_frame
{
	MV_U8   frame_control; /* frame, protocol, initiator etc. */
	MV_U8   connection_rate;   /* connection rate, feature etc. */
	MV_U8   connect_tag[2];
	MV_U8   dest_sas_addr[8];
/* HW will generate Byte 12 after... */
	MV_U8   src_sas_addr[8];
	MV_U8   src_zone_grp;
	MV_U8   blocked_count;
	MV_U8   awt[2];
	MV_U8   cmp_features2[4];
	MV_U32  first_burst_size;      /* for hardware use*/
}open_addr_frame;

/* Protocol */
#define PROTOCOL_SMP      0x0
#define PROTOCOL_SSP      0x1
#define PROTOCOL_STP      0x2

enum _mv_error_record_info_type_ {
	BFFR_PERR      = (1UL << 0),
	WD_TMR_TO_ERR  = (1UL << 1),
	CREDIT_TO_ERR  = (1UL << 2),
	WRONG_DEST_ERR = (1UL << 3),

	CNCTN_RT_NT_SPRTD_ERR = (1UL << 4),
	PRTCL_NOT_SPRTD_ERR   = (1UL << 5),

	BAD_DEST_ERR   = (1UL << 6),
	BRK_RCVD_ERR   = (1UL << 7),
	STP_RSRCS_BSY_ERR = (1UL << 8),

	NO_DEST_ERR = (1UL << 9),
	PTH_BLKD_ERR = (1UL << 10),

	OPEN_TMOUT_ERR = (1UL << 11),
	CNCTN_CLSD_ERR = (1UL << 12),
	ACK_NAK_TO     = (1UL << 13),
	NAK_ERR        = (1UL << 14),
	INTRLCK_ERR    = (1UL << 15),
	DATA_OVR_UNDR_FLW_ERR = (1UL << 16),
	Reserved1      = (1UL << 17),

	UNEXP_XFER_RDY_ERR = (1UL << 18),
	XFR_RDY_OFFST_ERR  = (1UL << 19),
	RD_DATA_OFFST_ERR  = (1UL << 20),

	TX_STOPPED_EARLY   = (1UL << 22),

	R_ERR     = (1UL << 23),
	TFILE_ERR = (1UL << 24),
	SYNC_ERR  = (1UL << 25),

	DMAT_RCVD = (1UL << 26),
	UNKNWN_FIS_ERR = (1UL << 27),
	RTRY_LMT_ERR  = (1UL << 28),
	RESP_BFFR_OFLW = (1UL << 29),

	PI_ERR  = (1UL << 30),
	CMD_ISS_STPD = (1UL << 31),

	USR_BLK_NM_MASK = 0xfff,

	/* Protection information */
	REF_CHK_ERR = (1UL << 12),
	APP_CHK_ERR = (1UL << 13),
	GRD_CHK_ERR =  (1UL << 14),

	SLOT_BSY_ERR = (1UL << 31),
};
/* Error Information Record */
typedef struct _err_info_record
{
	MV_U32 err_info_field_1;
	MV_U32 err_info_field_2;
}err_info_record;


/* Status Buffer */
typedef struct _status_buffer
{
	err_info_record err_info;
	union
	{
		struct _ssp_response_iu ssp_resp;
		MV_U8   smp_resp[MAX_SMP_RESP_SIZE];
	} data;
}status_buffer;

#define MAX_RESPONSE_FRAME_LENGTH \
	(MV_MAX(sizeof(struct _ssp_response_iu), MAX_SMP_RESP_SIZE))



/* Command Table */
typedef struct _mv_command_table
{
	open_addr_frame   open_address_frame;
	status_buffer	  status_buff;

	union
	{
		mv_ssp_command_table ssp_cmd_table;
		mv_smp_command_table smp_cmd_table;
		mv_sata_stp_command_table stp_cmd_table;
	} table;
} mv_command_table;

enum sas_rx_tx_ring_bits {
/* RX (completion) ring bits */
	RXQ_RSPNS_GOOD		= (1U << 23),	/* Response good */
	RXQ_SLOT_RST_CMPLT	= (1U << 21),	/* Slot reset complete */
	RXQ_CMD_RCVD		= (1U << 20),	/* target cmd received */
	RXQ_ATTN		= (1U << 19),	/* attention */
	RXQ_RSPNS_XFRD		= (1U << 18),	/* response frame xfer'd */
	RXQ_ERR_RCRD_XFRD	= (1U << 17),	/* err info rec xfer'd */
	RXQ_CMD_CMPLT		= (1U << 16),	/* cmd complete */
	RXQ_SLOT_MASK		= 0xfff,	/* slot number */

/* TX (delivery) ring bits */
	TXQ_MODE_I              = (1UL << 28),
	TXQ_CMD_SSP             = (1UL << 29),
	TXQ_CMD_SMP             = (2UL << 29),
	TXQ_CMD_STP             = (3UL << 29),

	TXQ_PHY_SHIFT  = 12,
	TXQ_REGSET_SHIFT = 20,
	TXQ_PRIORITY_SHIFT = 27,
};

#define CMD_SSP         0x01
#define CMD_SMP         0x02
#define CMD_STP         0x03
#define CMD_SSP_TGT      0x04
#define CMD_SLOT_RESET   0x07

enum prd_slt_chain_bit{
	PRD_CHAIN_BIT = (1UL<<24),
	PRD_IF_SELECT_BIT = (0xFUL<<28),
	
	PRD_IF_SELECT_SHIFT = 28,
};

/*prd table for vanir 3 DW*/
typedef struct _prd_t
{
	MV_U32	baseAddr_low;
	MV_U32  baseAddr_high;
	/*DW3: IF select, chain and size*/
	MV_U32 size;

} prd_t;

typedef struct _prd_context
{
	prd_t * prd;
	MV_U16 avail;
	MV_U16 reserved;
}prd_context;
/*
  ========================================================================
	Accessor macros and functions
  ========================================================================
*/
#define READ_PORT_IRQ_STAT(root, phy) \
   (MV_REG_READ_DWORD(root->mmio_base, COMMON_PORT_IRQ_STAT0 + (phy->asic_id * 8)))

#define WRITE_PORT_IRQ_STAT(root, phy, tmp) \
   (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_IRQ_STAT0 + (phy->asic_id * 8), tmp))

#define READ_PORT_IRQ_MASK(root, phy) \
   (MV_REG_READ_DWORD(root->mmio_base, COMMON_PORT_IRQ_MASK0 + (phy->asic_id * 8)))

#define WRITE_PORT_IRQ_MASK(root, phy, tmp) \
   (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_IRQ_MASK0 + (phy->asic_id * 8), tmp))

#define READ_PORT_PHY_CONTROL(root, phy) \
   (MV_REG_READ_DWORD(root->mmio_base, COMMON_PORT_PHY_CONTROL0 + (phy->asic_id * 4)))

#define WRITE_PORT_PHY_CONTROL(root, phy, tmp) \
   (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_PHY_CONTROL0 + (phy->asic_id * 4), tmp))

#define READ_PORT_VSR_DATA(root, phy) \
   (MV_REG_READ_DWORD(root->mmio_base, COMMON_PORT_VSR_DATA0 + (phy->asic_id * 8)))

#define WRITE_PORT_VSR_DATA(root, phy, tmp) \
   (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_VSR_DATA0 + (phy->asic_id * 8), tmp))

#define WRITE_PORT_VSR_ADDR(root, phy, tmp) \
   (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_VSR_ADDR0 + (phy->asic_id * 8), tmp))

#define READ_PORT_CONFIG_DATA(root, phy) \
   (MV_REG_READ_DWORD(root->mmio_base, COMMON_PORT_CONFIG_DATA0 + (phy->asic_id * 8)))

#define WRITE_PORT_CONFIG_DATA(root, phy, tmp) \
   (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_CONFIG_DATA0 + (phy->asic_id * 8), tmp))

#define WRITE_PORT_CONFIG_ADDR(root, phy, tmp) \
   (MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_CONFIG_ADDR0 + (phy->asic_id * 8), tmp))

#define READ_REGISTER_SET_ENABLE(root, i) \
	MV_REG_READ_DWORD(root->mmio_base,(((i) > 31) ? COMMON_SATA_REG_SET1 : COMMON_SATA_REG_SET0))

#define WRITE_REGISTER_SET_ENABLE(root, i, tmp) \
	MV_REG_WRITE_DWORD(root->mmio_base, (((i) > 31) ? COMMON_SATA_REG_SET1 : COMMON_SATA_REG_SET0), tmp)

#define READ_SRS_IRQ_STAT(root, i) \
	MV_REG_READ_DWORD(root->mmio_base, (((i)>31) ? COMMON_SRS_IRQ_STAT1 : COMMON_SRS_IRQ_STAT0))

#define WRITE_SRS_IRQ_STAT(root, i, tmp) \
	MV_REG_WRITE_DWORD(root->mmio_base, (((i)>31) ? COMMON_SRS_IRQ_STAT1 : COMMON_SRS_IRQ_STAT0), tmp)

MV_VOID mv_set_dev_info(MV_PVOID root_p, MV_PVOID phy_p);
MV_VOID mv_set_sas_addr(MV_PVOID root_p, MV_PVOID phy_p, MV_PU8 sas_addr);
MV_VOID mv_reset_phy(MV_PVOID root_p, MV_U8 logic_phy_map, MV_BOOLEAN hard_reset);

MV_VOID hal_clear_srs_irq(MV_PVOID root_p, MV_U32 set, MV_BOOLEAN clear_all);
MV_VOID hal_enable_register_set(MV_PVOID root_p, MV_PVOID base_p);
MV_BOOLEAN sata_is_register_set_stopped(MV_PVOID root_p, MV_U8 set);

MV_VOID hal_disable_io_chip(MV_PVOID root_p);

#define hal_has_phy_int(common_irq, phy)	\
	(common_irq & \
		(MV_BIT(phy->asic_id + INT_PORT_MASK_OFFSET) \
		| MV_BIT(phy->asic_id + INT_PHY_MASK_OFFSET)))


#define hal_remove_phy_int(common_irq, phy)	\
	(common_irq & \
		~(MV_BIT(phy->asic_id + INT_PORT_MASK_OFFSET) \
			| MV_BIT(phy->asic_id + INT_PHY_MASK_OFFSET)))

#define mv_is_phy_ready(root, phy) \
               (READ_PORT_PHY_CONTROL(root, phy) & SCTRL_PHY_READY_MASK)

#define get_phy_link_rate(phy_status)	(MV_U8)\
	(((phy_status&SCTRL_NEG_SPP_PHYS_LINK_RATE_MASK) >> \
	SCTRL_NEG_SPP_PHYS_LINK_RATE_MASK_OFFSET) + SAS_LINK_RATE_1_5_GBPS)

#define mv_sgpio_write_register(mmio, reg, value)		\
	 MV_REG_WRITE_DWORD(mmio, SGPIO_REG_BASE+reg, value)

#define mv_sgpio_read_register(mmio, reg, value)		\
	 value = MV_REG_READ_DWORD(mmio, SGPIO_REG_BASE+reg)

enum command_handler_defs	
{
	HANDLER_SSP = 0,
	HANDLER_SATA,
	HANDLER_SATA_PORT,
	HANDLER_STP,
	HANDLER_PM,
	HANDLER_SMP,
	HANDLER_ENC,
	HANDLER_API,
	HANDLER_I2C,
	MAX_NUMBER_HANDLER
};

int core_prepare_hwprd(MV_PVOID core, sgd_tbl_t * source, MV_PVOID prd);

MV_U8 sata_get_register_set(MV_PVOID root_p);
MV_VOID sata_free_register_set(MV_PVOID root_p, MV_U8 set);
MV_VOID io_chip_handle_cmpl_queue_int(MV_PVOID root_p);
MV_U16 prot_set_up_sg_table(MV_PVOID root_p, MV_Request *req, MV_PVOID sg_wrapper_p);
MV_VOID core_fill_prd(MV_PVOID prd_ctx, MV_U64 bass_addr, MV_U32 size);
#define mv_reset_stp(x, y)

MV_VOID core_alarm_enable_register(MV_PVOID core_p);
MV_VOID core_alarm_set_register(MV_PVOID core_p, MV_U8 value);
MV_VOID core_dump_common_reg(void *root_p);

#endif /* __CORE_HAL_H */

