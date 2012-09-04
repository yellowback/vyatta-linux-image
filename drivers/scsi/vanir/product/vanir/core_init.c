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
#include "mv_config.h"
#include "core_init.h"
#include "core_type.h"
#include "core_hal.h"
#include "core_util.h"
#include "core_manager.h"

#include "core_sas.h"
#include "core_sata.h"
#include "core_error.h"
#include "core_expander.h"
#include "core_console.h"

extern void core_handle_init_queue(core_extension *core, MV_BOOLEAN single);
extern void core_handle_waiting_queue(core_extension *core);

MV_VOID io_chip_init_registers(pl_root *root);

void update_phy_info(pl_root *root, domain_phy *phy);
void update_port_phy_map(pl_root *root, domain_phy *phy);

void set_phy_tuning(pl_root *root, domain_phy *phy, PHY_TUNING phy_tuning)
{
	core_extension *core = root->core;
	MV_U32 tmp, setting_0 = 0, setting_1 = 0;
	MV_U8 i;

	/* Remap information for B0 chip:
	*
	* R0Ch -> R118h[15:0] (Adapted DFE F3 - F5 coefficient)
	* R0Dh -> R118h[31:16] (Generation 1 Setting 0)
	* R0Eh -> R11Ch[15:0]  (Generation 1 Setting 1)
	* R0Fh -> R11Ch[31:16] (Generation 2 Setting 0)
	* R10h -> R120h[15:0]  (Generation 2 Setting 1)
	* R11h -> R120h[31:16] (Generation 3 Setting 0)
	* R12h -> R124h[15:0]  (Generation 3 Setting 1)
	* R13h -> R124h[31:16] (Generation 4 Setting 0 (Reserved))
	*/

	/* A0 has a different set of registers */
	if (core->revision_id == VANIR_A0_REV) return;

	for (i = 0; i < 3; i++) {
		/* loop 3 times, set Gen 1, Gen 2, Gen 3 */
		switch (i) {
		case 0:
			setting_0 = GENERATION_1_SETTING;
			setting_1 = GENERATION_1_2_SETTING;
			break;
		case 1:
			setting_0 = GENERATION_1_2_SETTING;
			setting_1 = GENERATION_2_3_SETTING;
			break;
		case 2:
			setting_0 = GENERATION_2_3_SETTING;
			setting_1 = GENERATION_3_4_SETTING;
			break;
		}

		/* Set:
		*
		*   Transmitter Emphasis Enable
		*   Transmitter Emphasis Amplitude
		*   Transmitter Amplitude
		*/
		WRITE_PORT_VSR_ADDR(root, phy, setting_0);
		tmp = READ_PORT_VSR_DATA(root, phy);
		tmp &= ~(0xFBE << 16);
		tmp |= (((phy_tuning.Trans_Emphasis_En << 11) |
		(phy_tuning.Trans_Emphasis_Amp << 7) |
		(phy_tuning.Trans_Amp << 1)) << 16);
		WRITE_PORT_VSR_DATA(root, phy, tmp);

		/* Set Transmitter Amplitude Adjust */
		WRITE_PORT_VSR_ADDR(root, phy, setting_1);
		tmp = READ_PORT_VSR_DATA(root, phy);
		tmp &= ~(0xC000);
		tmp |= (phy_tuning.Trans_Amp_Adjust << 14);
		WRITE_PORT_VSR_DATA(root, phy, tmp);
	}
}

MV_VOID set_phy_ffe_tuning(pl_root *root, domain_phy *phy, FFE_CONTROL ffe)
{
	core_extension *core = (core_extension *)root->core;
	MV_U32 tmp;

	if ((core->revision_id == VANIR_A0_REV) || 
		(core->revision_id == VANIR_B0_REV)) return;

	/* FFE Resistor and Capacitor */
	/* R10Ch DFE Resolution Control/Squelch and FFE Setting
	 *
	 * FFE_FORCE            [7]
	 * FFE_RES_SEL          [6:4]
	 * FFE_CAP_SEL          [3:0]
	 */ 
	WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_FFE_CONTROL);
	tmp = READ_PORT_VSR_DATA(root, phy);
	tmp &= ~0xFF;

	/* Read from HBA_Info_Page */
	tmp |= ((0x1 << 7) |
		(ffe.FFE_Resistor_Select << 4) |
		(ffe.FFE_Capacitor_Select << 0));

	WRITE_PORT_VSR_DATA(root, phy, tmp);

	/* R064h PHY Mode Register 1
	 *
	 * DFE_DIS		18
	 */
	WRITE_PORT_VSR_ADDR(root, phy, VSR_REF_CLOCK_CRTL);
	tmp = READ_PORT_VSR_DATA(root, phy);
	tmp &= ~0x40001;

	tmp |= (0 << 18);
	WRITE_PORT_VSR_DATA(root, phy, tmp);

	/* R110h DFE F0-F1 Coefficient Control/DFE Update Control
	 *
	 * DFE_UPDATE_EN        [11:6]
	 * DFE_FX_FORCE         [5:0]
	 */
	WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_DFE_UPDATE_CRTL);
	tmp = READ_PORT_VSR_DATA(root, phy);
	tmp &= ~0xFFF;

	tmp |= ((0x3F << 6) | (0x0 << 0));
	WRITE_PORT_VSR_DATA(root, phy, tmp);

	/* R1A0h Interface and Digital Reference Clock Control/Reserved_50h
	 *
	 * FFE_TRAIN_EN         3
	 */
	WRITE_PORT_VSR_ADDR(root, phy, VSR_REF_CLOCK_CRTL);
	tmp = READ_PORT_VSR_DATA(root, phy);
	tmp &= ~0x8;
	
	tmp |= (0 << 3);
	WRITE_PORT_VSR_DATA(root, phy, tmp);
}

MV_VOID set_phy_rate(pl_root *root, domain_phy *phy, MV_U8 rate)
{
	MV_U32 tmp;

	WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_CONFIG);
	tmp = READ_PORT_VSR_DATA(root, phy);
	tmp &= 0x80000F; 
	/*bit 18:4 for phy rate config. 		(1.5Gbps, 3Gbps, 6Gbps).*/
	/*Bit18:15:	Tx Requested Logical Link Rate(Multiplexing)(0000,	1000b, 1001b)
	* Bit14:9:	Tx Supported Physical Link Rates	(110000b, 111100b, 111111b)
	* Bit8: 		Parity bit to ensure bit14:8 is odd (1, 0, 1)
	* Bit7: 		SNW3 supported				(0, 0, 1)
	* Bit6:4		Support speed List				(001b, 011b, 111b)
	*/

	switch(rate) {
	case 0x0:
		tmp |= 0x0000611eL; /* support 1.5Gbps */
		break;
	case 0x1:
		tmp |= 0x0004783eL; /* support 1.5,3.0Gbps */
		break;
	case 0x2:
	default:
		tmp |= 0x0004fffeL; /* support 1.5,3.0Gbps,6.0Gbps */
		break;
	}
	WRITE_PORT_VSR_DATA(root, phy, tmp);
}


MV_VOID core_dump_common_reg(void *root_p)
{
	pl_root *root = (pl_root *)root_p;
	MV_LPVOID mmio = root->mmio_base;
	MV_U8 i;
	MV_DPRINT(("COMMON_PORT_IMPLEMENT(0x9C)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_PORT_IMPLEMENT)));
	MV_DPRINT(("COMMON_PORT_TYPE(0xA0)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_PORT_TYPE)));
	MV_DPRINT(("COMMON_CONFIG(0x100)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_CONFIG)));
	MV_DPRINT(("COMMON_CONTROL(0x104)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_CONTROL)));
	MV_DPRINT(("COMMON_LST_ADDR(0x108)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_LST_ADDR)));
	MV_DPRINT(("COMMON_LST_ADDR_HI(0x10C)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_LST_ADDR_HI)));
	MV_DPRINT(("COMMON_FIS_ADDR(0x110)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_FIS_ADDR)));
	MV_DPRINT(("COMMON_FIS_ADDR_HI(0x114)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_FIS_ADDR_HI)));
	MV_DPRINT(("COMMON_SATA_REG_SET0(0x118)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_SATA_REG_SET0)));
	MV_DPRINT(("COMMON_SATA_REG_SET1(0x11C)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_SATA_REG_SET1)));
	MV_DPRINT(("COMMON_DELV_Q_CONFIG(0x120)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_DELV_Q_CONFIG)));
	MV_DPRINT(("COMMON_DELV_Q_ADDR(0x124)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_DELV_Q_ADDR)));
	MV_DPRINT(("COMMON_DELV_Q_ADDR_HI(0x128)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_DELV_Q_ADDR_HI)));
	MV_DPRINT(("COMMON_DELV_Q_WR_PTR(0x12C)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_DELV_Q_WR_PTR)));
	MV_DPRINT(("COMMON_DELV_Q_RD_PTR(0x130)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_DELV_Q_RD_PTR)));
	MV_DPRINT(("COMMON_CMPL_Q_CONFIG(0x134)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_CMPL_Q_CONFIG)));
	MV_DPRINT(("COMMON_CMPL_Q_ADDR(0x138)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_CMPL_Q_ADDR)));
	MV_DPRINT(("COMMON_CMPL_Q_ADDR_HI(0x13C)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_CMPL_Q_ADDR_HI)));
	MV_DPRINT(("COMMON_CMPL_Q_WR_PTR(0x140)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_CMPL_Q_WR_PTR)));
	MV_DPRINT(("COMMON_CMPL_Q_RD_PTR(0x144)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_CMPL_Q_RD_PTR)));

	MV_DPRINT(("COMMON_IRQ_STAT(0x150)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_IRQ_STAT)));
	MV_DPRINT(("COMMON_IRQ_MASK(0x154)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_IRQ_MASK)));
	MV_DPRINT(("COMMON_SRS_IRQ_STAT0(0x158)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_SRS_IRQ_STAT0)));
	MV_DPRINT(("COMMON_SRS_IRQ_MASK0(0x15C)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_SRS_IRQ_MASK0)));
	MV_DPRINT(("COMMON_SRS_IRQ_STAT1(0x160)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_SRS_IRQ_STAT1)));
	MV_DPRINT(("COMMON_SRS_IRQ_MASK1(0x164)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_SRS_IRQ_MASK1)));
	MV_DPRINT(("COMMON_NON_SPEC_NCQ_ERR0(0x168)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_NON_SPEC_NCQ_ERR0)));
	MV_DPRINT(("COMMON_NON_SPEC_NCQ_ERR1(0x16C)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_NON_SPEC_NCQ_ERR1)));
	MV_DPRINT(("COMMON_PORT_ALL_IRQ_STAT(0x1C0)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_PORT_ALL_IRQ_STAT)));
	MV_DPRINT(("COMMON_PORT_ALL_PHY_CONTROL(0x1F0)=%08X.\n", MV_REG_READ_DWORD(mmio, COMMON_PORT_ALL_PHY_CONTROL)));
	
	for (i = 0; i < 4; i++) {
		MV_DPRINT(("phy %d READ_PORT_IRQ_STAT(0x%03X): 0x%08x\n", i, COMMON_PORT_IRQ_STAT0 + (i * 8), MV_REG_READ_DWORD(root->mmio_base, COMMON_PORT_IRQ_STAT0 + (i * 8))));	
		MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_VSR_ADDR0 + (i * 8), VSR_IRQ_STATUS);
		MV_DPRINT(("phy %d VSR_IRQ_STATUS: 0x%08x\n", i, MV_REG_READ_DWORD(root->mmio_base, COMMON_PORT_VSR_DATA0 + (i * 8))));	
		MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_VSR_ADDR0 + (i * 8), VSR_PHY_CONFIG);
		MV_DPRINT(("phy %d VSR_PHY_CONFIG: 0x%08x\n", i, MV_REG_READ_DWORD(root->mmio_base, COMMON_PORT_VSR_DATA0 + (i * 8))));	
		MV_REG_WRITE_DWORD(root->mmio_base, COMMON_PORT_VSR_ADDR0 + (i * 8), VSR_PHY_STATUS);
		MV_DPRINT(("phy %d VSR_PHY_STATUS: 0x%08x\n", i, MV_REG_READ_DWORD(root->mmio_base, COMMON_PORT_VSR_DATA0 + (i * 8))));	

	}
	for (i = 0; i < 4; i++) {
		MV_DPRINT(("phy %d COMMON_PORT_IRQ_MASK0(0x%03X): 0x%08x\n", i, COMMON_PORT_IRQ_MASK0 + (i * 8), MV_REG_READ_DWORD(root->mmio_base, COMMON_PORT_IRQ_MASK0 + (i * 8))));	
	}

	for (i = 0; i < 4; i++) {
		MV_DPRINT(("phy %d COMMON_PORT_PHY_CONTROL0(0x%03X): 0x%08x\n", i, COMMON_PORT_PHY_CONTROL0 + (i * 4), MV_REG_READ_DWORD(root->mmio_base, COMMON_PORT_PHY_CONTROL0 + (i * 4)))); 
	}
}


MV_VOID io_chip_init_registers(pl_root *root)
{
	MV_LPVOID mmio;
	MV_U8 i,j;
	MV_U32 tmp;
	MV_U32 loop =0;
	HBA_Info_Page hba_info_param;
	MV_BOOLEAN hba_info_valid = MV_FALSE;
	MV_U32 temp;
	domain_phy *phy;
	_MV_U64 u64_sas_addr;
	MV_U8 def_sas_addr[8] = {0x50,0x05,0x04,0x30,0x11,0xab,0x00,0x00};
	MV_U8 *sas_addr = (void*)&u64_sas_addr;
	core_extension *core = (core_extension *)root->core;

	def_sas_addr[7] = (MV_U8)root->base_phy_num;

	MV_CopyMemory(sas_addr, def_sas_addr, sizeof(u64_sas_addr) );
	mmio = root->mmio_base;
	core_set_chip_options(root);

	tmp = MV_REG_READ_DWORD(mmio, COMMON_CONFIG);
	tmp |= CONFIG_SAS_SATA_RST;
	MV_REG_WRITE_DWORD(mmio, COMMON_CONFIG, tmp);
	core_sleep_millisecond(core, 250);

	/* fix 1.5G by pll */
	if (core->revision_id == VANIR_A0_REV) {
		MV_REG_WRITE_DWORD(mmio, COMMON_PORT_ALL_VSR_ADDR, 0x00000104);
		MV_REG_WRITE_DWORD(mmio, COMMON_PORT_ALL_VSR_DATA, 0x00018080);
	}
	/* disable phy until sas address is set*/
	MV_REG_WRITE_DWORD(mmio, COMMON_PORT_ALL_VSR_ADDR, VSR_PHY_CONFIG);
	if (core->revision_id == VANIR_A0_REV || core->revision_id == VANIR_B0_REV)
		/* set 6G/3G/1.5G, multiplexing, without SSC */
		MV_REG_WRITE_DWORD(mmio, COMMON_PORT_ALL_VSR_DATA, 0x0084d4fe);
	else
		/* set 6G/3G/1.5G, multiplexing, with and without SSC */
		MV_REG_WRITE_DWORD(mmio, COMMON_PORT_ALL_VSR_DATA, 0x0084fffe);

	if (core->revision_id == VANIR_B0_REV) {
		MV_REG_WRITE_DWORD(mmio, COMMON_PORT_ALL_VSR_ADDR,0x00000144);
		MV_REG_WRITE_DWORD(mmio, COMMON_PORT_ALL_VSR_DATA,0x08001006);
		MV_REG_WRITE_DWORD(mmio, COMMON_PORT_ALL_VSR_ADDR,0x000001b4);
		MV_REG_WRITE_DWORD(mmio, COMMON_PORT_ALL_VSR_DATA,0x0000705f);
	}

	/* reset control */
	MV_REG_WRITE_DWORD(mmio, COMMON_CONTROL, 0);

	/* enable retry 127 times */
        MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, 0x128);
        tmp = MV_REG_READ_DWORD(mmio, COMMON_CMD_DATA);
        tmp &=~0xffff;
        if ((core->revision_id == VANIR_A0_REV)
                || (core->revision_id == VANIR_B0_REV)
                || (core->revision_id == VANIR_C0_REV)) {
                tmp |= 0x007f;
        } else {
                tmp |= 0x7f7f;
        }
        MV_REG_WRITE_DWORD(mmio, COMMON_CMD_DATA, tmp);

	/* extend open frame timeout to max */
	MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, 0x124);
	tmp = MV_REG_READ_DWORD(mmio, COMMON_CMD_DATA);
	tmp &=~0xffff;
	tmp |=0x3fff;
	MV_REG_WRITE_DWORD(mmio, COMMON_CMD_DATA, tmp);

	/* multiplexing mark  b16 src_zone_grp, b8-14 open retry timer, b15 open retry enable*/
	MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, 0x134);
	tmp = MV_REG_READ_DWORD(mmio, COMMON_CMD_DATA);
	tmp &=0xFFFF00FF;
	tmp |=0x00028200;
	MV_REG_WRITE_DWORD(mmio, COMMON_CMD_DATA, tmp);

	/* set max connection time to 64 us*/
	MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, 0x138);
	MV_REG_WRITE_DWORD(mmio, COMMON_CMD_DATA, 0x003f003f);
 
	/* set WDTIMEOUT to 550 ms */
	MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, 0x13c);
	MV_REG_WRITE_DWORD(mmio, COMMON_CMD_DATA, 0x7a0000); /* up with PLL */

	/* not to halt for different port op during wideport link change */
	MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, 0x1a4);
	MV_REG_WRITE_DWORD(mmio, COMMON_CMD_DATA, 0xffefbf7d);

	if(mv_nvram_init_param(
		HBA_GetModuleExtension((root->core), MODULE_HBA),
		&hba_info_param)) {
		hba_info_valid = MV_TRUE;
	}

	if (hba_info_valid) {
		for (i = 0; i < root->phy_num; i++) {
			phy = &root->phy[i];
			temp = (MV_U32)(*(MV_PU32)&
				hba_info_param.PHY_Tuning[root->base_phy_num+i]);
			if (temp == 0xFFFFFFFFL){
                                switch(core->revision_id) {
				case VANIR_A0_REV:
				case VANIR_B0_REV:
				case VANIR_C0_REV:
				case VANIR_C1_REV:
				case VANIR_C2_REV:
				default:
				        hba_info_param.PHY_Tuning[root->base_phy_num+i].Trans_Emphasis_Amp = 0x6; 
				        hba_info_param.PHY_Tuning[root->base_phy_num+i].Trans_Amp = 0x1A;
				        hba_info_param.PHY_Tuning[root->base_phy_num+i].Trans_Amp_Adjust = 0x3;
				        break;
				}
			}

			temp = (MV_U8)(*(MV_PU8)&
				hba_info_param.FFE_Control[root->base_phy_num+i]);
			if (temp == 0xFFL) {
				switch(core->revision_id) {
				case VANIR_C0_REV:
				case VANIR_C1_REV:
				case VANIR_C2_REV:
					hba_info_param.FFE_Control[root->base_phy_num+i].FFE_Resistor_Select = 0x7;
					hba_info_param.FFE_Control[root->base_phy_num+i].FFE_Capacitor_Select = 0xC;
					break;
				case VANIR_A0_REV:
				case VANIR_B0_REV:
				default:
					hba_info_param.FFE_Control[root->base_phy_num+i].FFE_Resistor_Select = 0x7;
					hba_info_param.FFE_Control[root->base_phy_num+i].FFE_Capacitor_Select = 0x7;
					break;
				}
			}

			temp = (MV_U8)(*(MV_PU8)&
				hba_info_param.PHY_Rate[root->base_phy_num+i]);
			if (temp == 0xFFL)
				hba_info_param.PHY_Rate[root->base_phy_num+i] = 0x2; /*set default phy_rate = 6Gbps*/

			set_phy_tuning( root, phy, 
				hba_info_param.PHY_Tuning[root->base_phy_num+i]);
			set_phy_ffe_tuning(root, phy,
				hba_info_param.FFE_Control[root->base_phy_num+i]);
			set_phy_rate(root, phy,
				hba_info_param.PHY_Rate[root->base_phy_num+i]);
		}
	}

	for (i=0; i<root->phy_num; i++) {
		phy = &root->phy[i];

		/* Set Phy Id */
		mv_set_dev_info(root, phy);

		/* Set SAS Addr */
		if (hba_info_valid) {
			for (j=0; j<8; j++) {
				sas_addr[j] = hba_info_param.SAS_Address[root->base_phy_num+i].b[j];
			}
		}
		if ((U64_COMPARE_U32((*(MV_U64 *)sas_addr), 0) == 0) ||
			(U64_COMP_U64(0xffffffffffffffffULL, (*(MV_U64 *)sas_addr)))) {
			U64_ASSIGN((*(MV_U64 *)sas_addr), 0x0000ab1130040550ULL);
		}
		mv_set_sas_addr(root, phy, sas_addr);

		if (core->revision_id == VANIR_A0_REV) {
			WRITE_PORT_VSR_ADDR(root, phy, 0x000001b4);
			WRITE_PORT_VSR_DATA(root, phy, 0x8300ffc1);
		}

		WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_CONFIG);
		tmp = READ_PORT_VSR_DATA(root, phy);
		tmp |= MV_BIT(0);
		WRITE_PORT_VSR_DATA(root, phy, tmp & 0xfd7fffff); /* enable phy */

		if (core->revision_id == VANIR_B0_REV) {
			WRITE_PORT_VSR_ADDR(root, phy,0x00000144);
			WRITE_PORT_VSR_DATA(root, phy,0x08001006);
			WRITE_PORT_VSR_ADDR(root, phy,0x000001b4);
			WRITE_PORT_VSR_DATA(root, phy,0x0000705f);
		}
	}
	
       if (HBA_CheckIsFlorence(root->core))
                core_sleep_millisecond(root->core, 400);
       else
                core_sleep_millisecond(root->core, 100);

	for (i = 0; i < root->phy_num; i++) {
		phy = &root->phy[i];
		
		/* reset irq */
		tmp = READ_PORT_IRQ_STAT(root, phy);
		WRITE_PORT_IRQ_STAT(root, phy, tmp);

		/* enable phy change interrupt and broadcast change */
		tmp = IRQ_PHY_RDY_CHNG_MASK | IRQ_BRDCST_CHNG_RCVD_MASK |
			IRQ_UNASSOC_FIS_RCVD_MASK | IRQ_SIG_FIS_RCVD_MASK |
			IRQ_ASYNC_NTFCN_RCVD_MASK | IRQ_PHY_RDY_CHNG_1_TO_0;
                phy->phy_irq_mask = tmp;
		WRITE_PORT_IRQ_MASK(root, phy, tmp);
	
	}

	/* reset CMD queue */
	tmp = MV_REG_READ_DWORD(mmio, COMMON_CONTROL);
	tmp |= CONTROL_RESET_CMD_ISSUE;
	MV_REG_WRITE_DWORD(mmio, COMMON_CONTROL, tmp);

	tmp = MV_REG_READ_DWORD(mmio, COMMON_CONFIG);
	tmp |= CONFIG_CMD_TBL_BE | CONFIG_DATA_BE;
	tmp &= ~CONFIG_OPEN_ADDR_BE;
	tmp |= CONFIG_RSPNS_FRAME_BE;
	tmp |= CONFIG_STP_STOP_ON_ERR;
	MV_REG_WRITE_DWORD(mmio, COMMON_CONFIG, tmp);
	CORE_DPRINT(("COMMON_CONFIG (default) = 0x%x\n", tmp));

	/* assign command list address */
	MV_REG_WRITE_DWORD(mmio, COMMON_LST_ADDR, root->cmd_list_dma.parts.low);
	MV_REG_WRITE_DWORD(mmio, COMMON_LST_ADDR_HI,
		root->cmd_list_dma.parts.high);

	/* assign FIS address */
	MV_REG_WRITE_DWORD(mmio, COMMON_FIS_ADDR, root->rx_fis_dma.parts.low);
	MV_REG_WRITE_DWORD(mmio, COMMON_FIS_ADDR_HI,
		root->rx_fis_dma.parts.high);

	/* assign delivery queue address */
	tmp = 0;
	MV_REG_WRITE_DWORD(mmio, COMMON_DELV_Q_CONFIG, tmp);
	tmp = DELV_QUEUE_SIZE_MASK & root->delv_q_size;
	tmp |= DELV_QUEUE_ENABLE;
	MV_REG_WRITE_DWORD(mmio, COMMON_DELV_Q_CONFIG, tmp);
	MV_REG_WRITE_DWORD(mmio, COMMON_DELV_Q_ADDR, root->delv_q_dma.parts.low);
	MV_REG_WRITE_DWORD(mmio, COMMON_DELV_Q_ADDR_HI,
		root->delv_q_dma.parts.high);

	/* write in a default value for completion queue pointer in memory */
	tmp = 0xFFF;
	MV_CopyMemory( root->cmpl_wp, &tmp, 4 );

	/* assign completion queue address */
	tmp = 0;
	MV_REG_WRITE_DWORD(mmio, COMMON_CMPL_Q_CONFIG, tmp);
	tmp =  CMPL_QUEUE_SIZE_MASK & root->cmpl_q_size;
	tmp |= CMPL_QUEUE_ENABLE;
	MV_REG_WRITE_DWORD(mmio, COMMON_CMPL_Q_CONFIG, tmp);
	MV_REG_WRITE_DWORD(mmio, COMMON_CMPL_Q_ADDR, root->cmpl_wp_dma.parts.low);
	MV_REG_WRITE_DWORD(mmio, COMMON_CMPL_Q_ADDR_HI,
		root->cmpl_wp_dma.parts.high);

	if (hba_info_valid) {
		if (hba_info_param.HBA_Flag & HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY) {
			tmp = 0;
			/* ((PREG_COMMON_COAL_CONFIG)&tmp)->INT_COAL_COUNT=1; */
			if ( root->slot_count_support > 0x1ff )
				tmp =  INT_COAL_COUNT_MASK & 0x1ff;
			else
				tmp =  INT_COAL_COUNT_MASK & root->slot_count_support;
			tmp |= INT_COAL_ENABLE;

			MV_REG_WRITE_DWORD(mmio, COMMON_COAL_CONFIG, tmp);
			tmp = 0x10400;
			MV_REG_WRITE_DWORD(mmio, COMMON_COAL_TIMEOUT, tmp);
		} else {
			tmp = 0;
			if ( root->slot_count_support>0x1ff )
				tmp =  INT_COAL_COUNT_MASK & 0x1ff;
			else
				tmp =  INT_COAL_COUNT_MASK & root->slot_count_support;
			tmp |= INT_COAL_ENABLE;

			MV_REG_WRITE_DWORD(mmio, COMMON_COAL_CONFIG, tmp);
			tmp = 0x1000;
			MV_REG_WRITE_DWORD(mmio, COMMON_COAL_TIMEOUT, tmp);
		}
	}

	/* enable CMD/CMPL_Q/RESP mode */
	tmp = MV_REG_READ_DWORD(mmio, COMMON_CONTROL);
	tmp |= CONTROL_EN_CMD_ISSUE|CONTROL_EN_SATA_RETRY;

	tmp |= CONTROL_FIS_RCV_EN;
	MV_REG_WRITE_DWORD(mmio, COMMON_CONTROL, tmp);

	/* enable completion queue interrupt */
	tmp = (INT_PORT_MASK | INT_CMD_CMPL );
	tmp |= INT_NON_SPCFC_NCQ_ERR;
	tmp |= INT_PHY_MASK;
	root->comm_irq_mask = tmp;
	MV_REG_WRITE_DWORD(mmio, COMMON_IRQ_MASK, tmp);

	MV_REG_WRITE_DWORD(root->mmio_base,
		COMMON_CMD_ADDR, CMD_PL_TIMER);
        tmp = 0x003F003F;
	MV_REG_WRITE_DWORD(root->mmio_base, COMMON_CMD_DATA, tmp);

        /* extent SMP link timeout value to max to fix smp request waterdog timeout */
       	MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, CMD_LINK_TIMER);
	tmp = MV_REG_READ_DWORD(mmio, COMMON_CMD_DATA);
	tmp |= 0xFFFF0000;
	MV_REG_WRITE_DWORD(mmio, COMMON_CMD_DATA, tmp);
	MV_REG_WRITE_DWORD(mmio, COMMON_CMD_ADDR, 0x1BC);
	tmp = MV_REG_READ_DWORD(mmio, COMMON_CMD_DATA);
	tmp |= 0x00000200;
	MV_REG_WRITE_DWORD(mmio, COMMON_CMD_DATA, tmp);
}

void update_phy_info(pl_root *root, domain_phy *phy)
{
	MV_LPVOID mmio = root->mmio_base;
	MV_U32 reg;
	
	WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_STATUS);
	reg = READ_PORT_VSR_DATA(root, phy);
	reg = (MV_U8)((reg & VSR_PHY_STATUS_MASK) >> 16) & 0xff;
	switch(reg)
	{
	case VSR_PHY_STATUS_SAS_RDY:
		phy->type = PORT_TYPE_SAS;
		break;
	case VSR_PHY_STATUS_HR_RDY:
	default:
		phy->type = PORT_TYPE_SATA;
		break;
	}

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME5);
	reg = READ_PORT_CONFIG_DATA(root, phy);
	phy->dev_info = (reg & 0xff) << 24; /* Phy ID */
 	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME0);
	reg = READ_PORT_CONFIG_DATA(root, phy);
	/* 10000008 type(28..30), Target(16..19), Init(8..11) */
	phy->dev_info |= ((reg & 0x70) >> 4) +
		((reg & 0x0f000000) >> 8) +
		((reg & 0x0f0000) >> 8);

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME4);
	phy->dev_sas_addr.parts.low =
		CPU_TO_BIG_ENDIAN_32(READ_PORT_CONFIG_DATA(root, phy));

	WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ID_FRAME3);
	phy->dev_sas_addr.parts.high =
		CPU_TO_BIG_ENDIAN_32(READ_PORT_CONFIG_DATA(root, phy));

	phy->phy_status =
		READ_PORT_PHY_CONTROL(root, phy);

	if (phy->phy_status & SCTRL_PHY_READY_MASK)
	{
		if (phy->type & PORT_TYPE_SAS)
		{
			WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME5);
			reg = READ_PORT_CONFIG_DATA(root, phy);
			phy->att_dev_info = (reg & 0xff) << 24; /* phy id */

 			WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME0);
			reg = READ_PORT_CONFIG_DATA(root, phy);
			/* 10000008 type(28..30), Target(16..19), Init(8..11) */
			phy->att_dev_info |= ((reg & 0x70) >> 4) +
				((reg & 0x0f000000) >> 8) +
				((reg & 0x0f0000) >> 8);

			WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME4);
			phy->att_dev_sas_addr.parts.low =
				MV_BE32_TO_CPU(READ_PORT_CONFIG_DATA(root, phy));

			WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_ATT_ID_FRAME3);
			phy->att_dev_sas_addr.parts.high =
				MV_BE32_TO_CPU(READ_PORT_CONFIG_DATA(root, phy));
			/* SAS address is stored as BE, so ... */
			phy->att_dev_sas_addr.parts.low = MV_CPU_TO_LE32(phy->att_dev_sas_addr.parts.low);
			phy->att_dev_sas_addr.parts.high = MV_CPU_TO_LE32(phy->att_dev_sas_addr.parts.high);
		} else {
			phy->att_dev_info = 0;
			phy->att_dev_sas_addr.parts.low = 0;
			phy->att_dev_sas_addr.parts.high = 0;
		}

		WRITE_PORT_CONFIG_ADDR(root, phy, CONFIG_PHY_CONTROL);
		WRITE_PORT_CONFIG_DATA(root, phy, 0x04);
	}

	phy->irq_status |= READ_PORT_IRQ_STAT(root, phy);

	WRITE_PORT_VSR_ADDR(root, phy, VSR_IRQ_STATUS);
	reg = READ_PORT_VSR_DATA(root, phy);
	if (reg & VSR_IRQ_PHY_TIMEOUT) {
		WRITE_PORT_VSR_DATA(root, phy, VSR_IRQ_PHY_TIMEOUT);
		CORE_DPRINT(("Has VSR_IRQ_PHY TIMEOUT: %08X.\n",reg));
		WRITE_PORT_VSR_ADDR(root, phy, VSR_PHY_STATUS);
		reg = READ_PORT_VSR_DATA(root, phy);
		reg = (MV_U8)((reg >> 16) & 0x3f);
		CORE_EVENT_PRINT(("PHY State Machine Timeout. "\
				  "PHY Status 0x%x.\n", reg));
	}

	CORE_EVENT_PRINT(("phy %d irq_status = 0x%x.\n", \
		phy->id, phy->irq_status));
}


#define mv_enable_msi(_core)                                               \
{                                                               \
             MV_U32 temp;                                                  \
             temp = MV_PCI_READ_DWORD(_core, MV_PCI_REG_CMD);        \
             temp |= MV_PCI_INT_DIS;                                       \
             MV_PCI_WRITE_DWORD(_core, temp, MV_PCI_REG_CMD);        \
             temp = MV_PCI_READ_DWORD(_core, MV_PCI_REG_MSI_CTRL);   \
             temp |= MV_PCI_MSI_EN;                                        \
             MV_PCI_WRITE_DWORD(_core, temp, MV_PCI_REG_MSI_CTRL);  \
}




MV_VOID controller_init(core_extension *core)
{
	MV_LPVOID mmio = core->mmio_base;
	MV_U32 tmp;

	tmp = MV_PCI_READ_DWORD(core, MV_PCI_REG_CMD);
	tmp |= MV_PCI_DEV_EN;
	MV_PCI_WRITE_DWORD(core, tmp, MV_PCI_REG_CMD);

	core->irq_mask = INT_MAP_SAS;
       core->irq_mask |= INT_MAP_XOR;

	if (msi_enabled(core))
		mv_enable_msi(core);
	
	MV_REG_WRITE_DWORD(mmio, CPU_MAIN_IRQ_MASK_REG, core->irq_mask);

        /* clear PCIe error registers to 0 */
        MV_REG_WRITE_DWORD(core->mmio_base, 0x14000, 0x2013);
        MV_REG_WRITE_DWORD(core->mmio_base, 0x14004, 0xFFFF);
        MV_REG_WRITE_DWORD(core->mmio_base, 0x14004, 0x0000);

	if (IS_VANIR(core) || core->device_id == 0x948F) {
		tmp = MV_PCI_READ_DWORD(core, MV_PCI_REG_DEV_CTRL);
		if ((tmp & MV_PCI_RD_REQ_MASK) > MV_PCI_RD_REQ_SIZE) {
			tmp &= ~MV_PCI_RD_REQ_MASK;
			tmp |= MV_PCI_RD_REQ_SIZE;
			MV_PCI_WRITE_DWORD(core, tmp, MV_PCI_REG_DEV_CTRL);
		}
	}
}

void core_set_cmd_header_selector(mv_command_header *cmd_header)
{
	cmd_header->interface_select = CS_INTRFC_CORE_DMA;
}

void map_phy_id(pl_root *root) {}

MV_BOOLEAN core_reset_controller(core_extension *core){return MV_TRUE;}

extern MV_BOOLEAN ses_state_machine( MV_PVOID enc_p );
MV_VOID core_init_handlers(core_extension *core)
{

	core->handlers[HANDLER_SATA].init_handler = sata_device_state_machine;
	core->handlers[HANDLER_SATA].verify_command = sata_verify_command;
	core->handlers[HANDLER_SATA].prepare_command = sata_prepare_command;
	core->handlers[HANDLER_SATA].send_command = sata_send_command;
	core->handlers[HANDLER_SATA].process_command = sata_process_command;
	core->handlers[HANDLER_SATA].error_handler = sata_error_handler;

	core->handlers[HANDLER_SATA_PORT].init_handler = sata_port_state_machine;

	core->handlers[HANDLER_PM].init_handler = pm_state_machine;
	core->handlers[HANDLER_PM].verify_command = pm_verify_command;
	core->handlers[HANDLER_PM].prepare_command = pm_prepare_command;
	core->handlers[HANDLER_PM].send_command = pm_send_command;
	core->handlers[HANDLER_PM].process_command = pm_process_command;
	core->handlers[HANDLER_PM].error_handler = NULL;

	core->handlers[HANDLER_SSP].init_handler = sas_init_state_machine;
	core->handlers[HANDLER_SSP].verify_command = ssp_verify_command;
	core->handlers[HANDLER_SSP].prepare_command = ssp_prepare_command;
	core->handlers[HANDLER_SSP].send_command = ssp_send_command;
	core->handlers[HANDLER_SSP].process_command = ssp_process_command;
	core->handlers[HANDLER_SSP].error_handler = ssp_error_handler;

	core->handlers[HANDLER_SMP].init_handler = exp_state_machine;
	core->handlers[HANDLER_SMP].verify_command = smp_verify_command;
	core->handlers[HANDLER_SMP].prepare_command = smp_prepare_command;
	core->handlers[HANDLER_SMP].send_command = smp_send_command;
	core->handlers[HANDLER_SMP].process_command = smp_process_command;
	core->handlers[HANDLER_SMP].error_handler = NULL;

	core->handlers[HANDLER_STP].init_handler = sata_device_state_machine;
	core->handlers[HANDLER_STP].verify_command = sata_verify_command;
	core->handlers[HANDLER_STP].prepare_command = stp_prepare_command;
	core->handlers[HANDLER_STP].send_command = sata_send_command;
	core->handlers[HANDLER_STP].process_command = stp_process_command;
	core->handlers[HANDLER_STP].error_handler = sata_error_handler;

	core->handlers[HANDLER_ENC].init_handler = ses_state_machine;
	core->handlers[HANDLER_ENC].verify_command = ssp_verify_command;
	core->handlers[HANDLER_ENC].prepare_command = ssp_prepare_command;
	core->handlers[HANDLER_ENC].send_command = ssp_send_command;
	core->handlers[HANDLER_ENC].process_command = ssp_process_command;
	core->handlers[HANDLER_ENC].error_handler = NULL;
	
	core->handlers[HANDLER_API].verify_command = api_verify_command;

}

