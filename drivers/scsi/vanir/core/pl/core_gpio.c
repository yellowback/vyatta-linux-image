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
#include "core_type.h"
#include "core_manager.h"
#include "core_gpio.h"
#include "com_error.h"
#include "core_util.h"

MV_U32 sgpio_read_pci_register(MV_PVOID core_p, MV_U8 reg_address) {
	MV_U32 reg=0;
	reg = MV_PCI_READ_DWORD( core_p, reg_address );
	return reg;
}

void sgpio_write_pci_register(MV_PVOID core_p, MV_U8 reg_address, MV_U32 reg_value) {
	MV_PCI_WRITE_DWORD( core_p, reg_value, reg_address );
	return;
}

void sgpio_sendsgpioframe(MV_PVOID this, MV_U32 value)
{
	core_extension * pCore=(core_extension *)this;
	MV_U32 sgpio_ctl_reg;
	MV_U32 sgpio_data_out_l;
	MV_U32 sgpio_data_out_h;

	sgpio_ctl_reg = sgpio_read_pci_register( pCore, SGPIO_Control );
	sgpio_data_out_l = sgpio_read_pci_register( pCore, SGPIO_Data_Out_L );
	sgpio_data_out_h = sgpio_read_pci_register( pCore, SGPIO_Data_Out_H );

	sgpio_write_pci_register( pCore, SGPIO_Data_Out_L, value);
	sgpio_write_pci_register( pCore, SGPIO_Data_Out_H, 0x0);
	sgpio_data_out_l = sgpio_read_pci_register( pCore, SGPIO_Data_Out_L );
	sgpio_data_out_h = sgpio_read_pci_register( pCore, SGPIO_Data_Out_H );
	
	sgpio_ctl_reg = 0xe0;
	sgpio_ctl_reg |= SGPIO_START_BIT;
	sgpio_write_pci_register( pCore, SGPIO_Control, sgpio_ctl_reg);
	sgpio_ctl_reg = sgpio_read_pci_register( pCore, SGPIO_Control );

	core_sleep_millisecond(pCore, 500);

	sgpio_ctl_reg &= ~SGPIO_START_BIT;
	sgpio_write_pci_register( pCore, SGPIO_Control, sgpio_ctl_reg);
	sgpio_ctl_reg = sgpio_read_pci_register( pCore, SGPIO_Control );
}

void sgpio_initialize( MV_PVOID this ) {
	core_extension * core=(core_extension *)this;
	pl_root *root=NULL;
	domain_port *port;
	domain_device * device;
	MV_U32 sgpio_init_reg=0;
	MV_LPVOID mmio = core->mmio_base;
	MV_U32 tmp, reg_config0, reg_config1, dword_offset0, dword_offset1;
	MV_U32 count,sgpio;
	MV_U8 i, device_cnt0, device_cnt1, j;

	if ((core->device_id == DEVICE_ID_6440) || (core->device_id == DEVICE_ID_6445)
                        ||(core->device_id == 0x6320) ||(core->device_id == DEVICE_ID_6340))
	{
		sgpio_init_reg = sgpio_read_pci_register( core, SGPIO_Init );
		CORE_PRINT(("*  SGPIO_InitReg before initialization = 0x%x \n", sgpio_init_reg ));
		sgpio_init_reg &= ~SGPIO_Mode;
		sgpio_write_pci_register(core, SGPIO_Init, sgpio_init_reg );
		core_sleep_microsecond(core, 1);
		sgpio_init_reg |= SGPIO_Mode;

		/*
		*The following lines setup the desired clock frequency in SGPIO_Control register
		* before clk is started by SGPIO_Init.  Allowing clk to switch frequency during run-time
		* could result in writing on the wrong edge of the clk.
		*/
		sgpio_write_pci_register( core, SGPIO_Init, sgpio_init_reg );
		sgpio_init_reg = sgpio_read_pci_register( core, SGPIO_Init );
		CORE_PRINT(("*  SGPIO_InitReg after initialization = 0x%x \n", sgpio_init_reg ));
	}	else if ((core->device_id == DEVICE_ID_6480)  || (core->device_id == DEVICE_ID_6485) 
		||(core->device_id ==0x8180))	{
		/*####### disable first ######*/
		/* sgpio 0 registers */
		mv_sgpio_read_register(mmio, SGPIO_REG_ADDR(0,SGPIO_REG_CONFIG0), reg_config0);
		reg_config0 &=~(SGPIO_EN|BLINK_GEN_EN_B|BLINK_GEN_EN_A|AUTO_BIT_LEN);
		mv_sgpio_write_register(mmio, SGPIO_REG_ADDR(0,SGPIO_REG_CONFIG0), reg_config0);

		/* sgpio 1 registers */
		mv_sgpio_read_register(mmio, SGPIO_REG_ADDR(1,SGPIO_REG_CONFIG0), reg_config1);
		reg_config1 &=~(SGPIO_EN|BLINK_GEN_EN_B|BLINK_GEN_EN_A|AUTO_BIT_LEN);
		mv_sgpio_write_register(mmio, SGPIO_REG_ADDR(1,SGPIO_REG_CONFIG0), reg_config1);

		/*####### enable SGPIO mode in PCI config register ######*/
		sgpio_write_pci_register(core, 0x44, 0x80);

		/*###### setting Drive Source ######*/
		device_cnt0=0;
		dword_offset0=0;
		device_cnt1=0;
		dword_offset1=0;

		root = &core->roots[core->chip_info->start_host];
		for(j=0;j<core->chip_info->n_phy;j++){
			port = &root->ports[j];
			if(port->device_count==0)
			continue;
			LIST_FOR_EACH_ENTRY_TYPE(device, &port->device_list, domain_device, base.queue_pointer){
				if(device==NULL)
				break;
				if(device->base.parent->type == BASE_TYPE_DOMAIN_PORT){
				device->connection |= DC_SGPIO;
				if (j < 4){
					/* sgpio 0 */
					mv_sgpio_read_register(mmio, 
						SGPIO_REG_ADDR(0,SGPIO_REG_DRV_SRC_BASE + device_cnt0 - dword_offset0),
						tmp);
					tmp &=~(0xff<<(dword_offset0*8));

					mv_sgpio_write_register(mmio,
						SGPIO_REG_ADDR(0,SGPIO_REG_DRV_SRC_BASE + device_cnt0 - dword_offset0),
						tmp+(DRV_SRC_PHY(0,j)<<(dword_offset0*8)));

					device->sgpio_drive_number = j;
					device_cnt0++;
					dword_offset0++;
					dword_offset0%=4;
				}else{
					/* sgpio 1 */
					mv_sgpio_read_register(mmio, 
						SGPIO_REG_ADDR(1,SGPIO_REG_DRV_SRC_BASE + device_cnt1 - dword_offset1),
						tmp);
					tmp &=~(0xff<<(dword_offset1*8));

					mv_sgpio_write_register(mmio,
						SGPIO_REG_ADDR(1,SGPIO_REG_DRV_SRC_BASE + device_cnt1 - dword_offset1),
						tmp+(DRV_SRC_PHY(0,j)<<(dword_offset1*8)));

					device->sgpio_drive_number = j;
					device_cnt1++;
					dword_offset1++;
					dword_offset1%=4;
				}	

				}
			}
		}
		
		/*##### setting control mode #####*/
		/* sgpio 0 */
		mv_sgpio_write_register(mmio,
			SGPIO_REG_ADDR(0,SGPIO_REG_CONTROL),
			((SDOUT_MD_AUTO<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE));

		/* sgpio 1 */
		mv_sgpio_write_register(mmio,
			SGPIO_REG_ADDR(1,SGPIO_REG_CONTROL),
			((SDOUT_MD_AUTO<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE));
		/*##### setting blink speeds #####*/
		/* sgpio 0 */
		mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(0, SGPIO_REG_CONFIG1),
			tmp);
		tmp &= ~(BLINK_LOW_TM_A | BLINK_HI_TM_A);
		tmp |= 0x11;
		mv_sgpio_write_register(mmio,
			SGPIO_REG_ADDR(0, SGPIO_REG_CONFIG1),
			tmp);

		/* sgpio 1 */
		mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(1, SGPIO_REG_CONFIG1),
			tmp);
		tmp &= ~(BLINK_LOW_TM_A | BLINK_HI_TM_A);
		tmp |= 0x11;
		mv_sgpio_write_register(mmio,
			SGPIO_REG_ADDR(1, SGPIO_REG_CONFIG1),
			tmp);

		/*##### enable int #####*/

		/* sgpio 0 */
		mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(0,SGPIO_REG_INT_ENABLE),
		(MANUAL_MD_REP_DONE|SDIN_DONE));

		/* sgpio 1 */
		mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(1,SGPIO_REG_INT_ENABLE),
		(MANUAL_MD_REP_DONE|SDIN_DONE));

		/*##### re-enable SGPIO #####*/

		/* sgpio 0 */
		reg_config0 |=(SGPIO_EN|BLINK_GEN_EN_B|BLINK_GEN_EN_A);
		/* SGPIO stream needs minimum 12 bits*/
		if (device_cnt0 < 4)
			device_cnt0 = 4;
		reg_config0 += (device_cnt0*3)<<AUTO_BIT_LEN_OFFSET;

		mv_sgpio_write_register(mmio,
			SGPIO_REG_ADDR(0,SGPIO_REG_CONFIG0),
			reg_config0);

		/* sgpio 1 */
		reg_config1 |=(SGPIO_EN|BLINK_GEN_EN_B|BLINK_GEN_EN_A);
		/* SGPIO stream needs minimum 12 bits*/
		if (device_cnt1 < 4)
			device_cnt1 = 4;
		reg_config1 += (device_cnt1*3)<<AUTO_BIT_LEN_OFFSET;

		mv_sgpio_write_register(mmio,
			SGPIO_REG_ADDR(1,SGPIO_REG_CONFIG0),
			reg_config1);

		CORE_PRINT(("*  SGPIO_InitReg after initialization \n" ));

		for(sgpio=0;sgpio<2;sgpio++){
			count=0;
			for(i=0;i<100;i++) {
				mv_sgpio_read_register(mmio,
				SGPIO_REG_ADDR(sgpio,SGPIO_REG_INT_CAUSE),
				tmp);

				if(tmp&SDIN_DONE){
					CORE_PRINT(("SGPIO%d count:%d, interrupt:%d\n",sgpio,count,tmp));
					if (count==0) {
						mv_sgpio_read_register(mmio,
						SGPIO_REG_ADDR(sgpio,SGPIO_REG_INT_CAUSE),
						tmp);
						mv_sgpio_write_register(mmio,
						SGPIO_REG_ADDR(sgpio,SGPIO_REG_INT_CAUSE),
						tmp);
						mv_sgpio_write_register(mmio,
						SGPIO_REG_ADDR(sgpio,SGPIO_REG_CONTROL),
						((SDOUT_MD_AUTO<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE));
					} else {
						sgpio_isr(this,sgpio);
						break;
					}
					count++;
				} else
					core_sleep_millisecond(root->core, 10);
					if (i == 10) {
						CORE_PRINT(("no back plan connected\n"));
					break;
				}
			}
		}
	}

	if (IS_VANIR(core) || core->device_id == DEVICE_ID_948F) {
		/* sgpio 0 registers */
		mv_sgpio_read_register(mmio, SGPIO_REG_ADDR(0,SGPIO_REG_CONFIG0), reg_config0);
		reg_config0 &=~(SGPIO_EN|BLINK_GEN_EN_B|BLINK_GEN_EN_A|AUTO_BIT_LEN);
		mv_sgpio_write_register(mmio, SGPIO_REG_ADDR(0,SGPIO_REG_CONFIG0), reg_config0);

		/* sgpio 1 registers */
		mv_sgpio_read_register(mmio, SGPIO_REG_ADDR(1,SGPIO_REG_CONFIG0), reg_config1);
		reg_config1 &=~(SGPIO_EN|BLINK_GEN_EN_B|BLINK_GEN_EN_A|AUTO_BIT_LEN);
		mv_sgpio_write_register(mmio, SGPIO_REG_ADDR(1,SGPIO_REG_CONFIG0), reg_config1);

		/*####### enable SGPIO mode in PCI config register ######*/
		tmp = MV_REG_READ_DWORD(mmio, 0x10104) & ~0x00000300L;
		tmp |= 0x00000100L;
		MV_REG_WRITE_DWORD(mmio, 0x10104, tmp);
		/*###### setting Drive Source ######*/
		device_cnt0=0;
		dword_offset0=0;
		device_cnt1=0;
		dword_offset1=0;

		for (i = core->chip_info->start_host;i < (core->chip_info->start_host + core->chip_info->n_host); i++) {
			root = &core->roots[i];
			for(j = 0;j < MAX_PORT_PER_PL; j++) {
				port = &root->ports[j];
				if (port->device_count==0)
					continue;
				LIST_FOR_EACH_ENTRY_TYPE(device, &port->device_list, domain_device, base.queue_pointer) {
					if (device==NULL)
						break;
					if (device->base.parent->type == BASE_TYPE_DOMAIN_PORT) {
						device->connection |= DC_SGPIO;
						mv_sgpio_read_register(mmio,
						        SGPIO_REG_ADDR(i,SGPIO_REG_DRV_SRC_BASE + device_cnt0 - dword_offset0),
						        tmp);
						tmp &=~(0xff<<(dword_offset0*8));
						mv_sgpio_write_register(mmio,
							SGPIO_REG_ADDR(i,SGPIO_REG_DRV_SRC_BASE + device_cnt0 - dword_offset0),
						tmp+(DRV_SRC_PHY(0,i)<<(dword_offset0*8)));
						device->sgpio_drive_number = j;
						device_cnt0++;
						dword_offset0++;
						dword_offset0 %= 4;

					}
				}
			}
		}

	/*##### setting control mode #####*/
	/* sgpio 0 */
	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(0,SGPIO_REG_CONTROL),
		((SDOUT_MD_AUTO<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE));

	/* sgpio 1 */
	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(1,SGPIO_REG_CONTROL),
		((SDOUT_MD_AUTO<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE));
	/*##### setting blink speeds #####*/
	/* sgpio 0 */
	mv_sgpio_read_register(mmio,
		SGPIO_REG_ADDR(0, SGPIO_REG_CONFIG1),
		tmp);
	tmp &= ~(BLINK_LOW_TM_A | BLINK_HI_TM_A);
	tmp |= 0x11;
	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(0, SGPIO_REG_CONFIG1),
		tmp);

	/* sgpio 1 */
	mv_sgpio_read_register(mmio,
		SGPIO_REG_ADDR(1, SGPIO_REG_CONFIG1),
		tmp);
	tmp &= ~(BLINK_LOW_TM_A | BLINK_HI_TM_A);
	tmp |= 0x11;
	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(1, SGPIO_REG_CONFIG1),
		tmp);

	/*##### enable int #####*/

	/* sgpio 0 */
	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(0,SGPIO_REG_INT_ENABLE),
		(MANUAL_MD_REP_DONE|SDIN_DONE));

	/* sgpio 1 */
	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(1,SGPIO_REG_INT_ENABLE),
		(MANUAL_MD_REP_DONE|SDIN_DONE));

	/*##### re-enable SGPIO #####*/

	/* sgpio 0 */
	reg_config0 |=(SGPIO_EN|BLINK_GEN_EN_B|BLINK_GEN_EN_A);
	/* SGPIO stream needs minimum 12 bits*/
	if (device_cnt0 < 4)
		device_cnt0 = 4;
	reg_config0 += (device_cnt0*3)<<AUTO_BIT_LEN_OFFSET;

	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(0,SGPIO_REG_CONFIG0),
		reg_config0);

	/* sgpio 1 */
	reg_config1 |=(SGPIO_EN|BLINK_GEN_EN_B|BLINK_GEN_EN_A);
	/* SGPIO stream needs minimum 12 bits*/
	if (device_cnt1 < 4)
		device_cnt1 = 4;
	reg_config1 += (device_cnt1*3)<<AUTO_BIT_LEN_OFFSET;

	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(1,SGPIO_REG_CONFIG0),
		reg_config1);

	CORE_PRINT(("*  SGPIO_InitReg after initialization \n" ));

	for (sgpio=0;sgpio<2;sgpio++) {
		count=0;
		for (i=0;i<100;i++) {
			mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(sgpio,SGPIO_REG_INT_CAUSE),
			tmp);

			if (tmp & SDIN_DONE) {
				CORE_PRINT(("SGPIO%d count:%d, interrupt:%d\n",sgpio,count,tmp));
				if (count == 0) {
					mv_sgpio_read_register(mmio,
					SGPIO_REG_ADDR(sgpio,SGPIO_REG_INT_CAUSE),
					tmp);
					mv_sgpio_write_register(mmio,
					SGPIO_REG_ADDR(sgpio,SGPIO_REG_INT_CAUSE),
					tmp);
					mv_sgpio_write_register(mmio,
					SGPIO_REG_ADDR(sgpio,SGPIO_REG_CONTROL),
					((SDOUT_MD_AUTO<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE));
				} else {
					sgpio_isr(this,sgpio);
					break;
				}
					count++;
			} else
				core_sleep_millisecond(root->core, 10);
				if(i==10){
					CORE_PRINT(("no back plan connected\n"));
					break;
				}
			}
		}
        }
}

void sgpio_smpreq_callback(core_extension * core, PMV_Request req)
{
	MV_U8 port_id;
	smp_response *smp_resp;
	domain_port * port;
	struct _domain_sgpio * sgpio_result = &core->lib_gpio.sgpio_result;
	smp_resp= (smp_response *)req->Scratch_Buffer;

	switch (smp_resp->function) {
	case READ_GPIO_REGISTER:
		CORE_PRINT(("Read GPIO Register Response: \n"));
		CORE_PRINT(("Data In High 0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		smp_resp->response.ReadGPIORegister.Data_In_High[0],
		smp_resp->response.ReadGPIORegister.Data_In_High[1],
		smp_resp->response.ReadGPIORegister.Data_In_High[2],
		smp_resp->response.ReadGPIORegister.Data_In_High[3],
		smp_resp->response.ReadGPIORegister.Data_In_High[4],
		smp_resp->response.ReadGPIORegister.Data_In_High[5],
		smp_resp->response.ReadGPIORegister.Data_In_High[6],
		smp_resp->response.ReadGPIORegister.Data_In_High[7]));
		CORE_PRINT(("Data In Low 0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		smp_resp->response.ReadGPIORegister.Data_In_Low[0],
		smp_resp->response.ReadGPIORegister.Data_In_Low[1],
		smp_resp->response.ReadGPIORegister.Data_In_Low[2],
		smp_resp->response.ReadGPIORegister.Data_In_Low[3],
		smp_resp->response.ReadGPIORegister.Data_In_Low[4],
		smp_resp->response.ReadGPIORegister.Data_In_Low[5],
		smp_resp->response.ReadGPIORegister.Data_In_Low[6],
		smp_resp->response.ReadGPIORegister.Data_In_Low[7]));
		break;
	case WRITE_GPIO_REGISTER:
		CORE_PRINT(("Receive Write GPIO Register Response: \n"));
		break;
	}
	MV_CopyMemory(sgpio_result, req->Scratch_Buffer, sizeof(req->Scratch_Buffer) );
}

void sgpio_smprequest_read(pl_root * root, PMV_Request req)
{
	smp_request*smp_req;
	PMV_SG_Table sg_table;

	if( req == NULL ){
		CORE_DPRINT(("ERROR: No more free internal requests. Request aborted.\n"));
		return;
	}

	sg_table = &req->SG_Table;
	/* Prepare identify ATA task */
	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_SMP;
	req->Device_Id = 0xFFFF;
	req->Data_Transfer_Length = sizeof(smp_request);
	smp_req = (smp_request *)core_map_data_buffer(req);
	smp_req->function=READ_GPIO_REGISTER;
	smp_req->smp_frame_type=SMP_REQUEST_FRAME;
	core_unmap_data_buffer(req);
	req->Completion = (void(*)(MV_PVOID,PMV_Request))sgpio_smpreq_callback;

	/* Make SG table */
	SGTable_Init(sg_table, 0);

	/* Send this internal request */
	Core_ModuleSendRequest(root->core, req);
}

void sgpio_smprequest_write(core_extension * core)
{
	PMV_Request req = get_intl_req_resource(&core->roots[core->chip_info->start_host],sizeof(smp_request));
	smp_request*smp_req;
	PMV_SG_Table sg_table;

	if(req == NULL ){
		CORE_DPRINT(("ERROR: No more free internal requests. Request aborted.\n"));
		return;
	}
	sg_table = &req->SG_Table;

	/* Prepare identify ATA task */
	req->Cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	req->Cdb[1] = CDB_CORE_MODULE;
	req->Cdb[2] = CDB_CORE_SMP;
	req->Device_Id = 0xFFFF;
	req->Cmd_Initiator = core;
	req->Data_Transfer_Length = sizeof(smp_request);
	smp_req=(smp_request *)core_map_data_buffer(req);
	smp_req->function=WRITE_GPIO_REGISTER;
	smp_req->smp_frame_type=SMP_REQUEST_FRAME;
	core_unmap_data_buffer(req);
	req->Completion = (void(*)(MV_PVOID,PMV_Request))sgpio_smpreq_callback;

	/* Make SG table */
	SGTable_Init(sg_table, 0);

	/* Send this internal request */
	Core_ModuleSendRequest(core, req);
}

void sgpio_process_sdin(MV_PVOID this, MV_U32 sgpio)
{
	core_extension * core=(core_extension *)this;
	MV_LPVOID mmio = core->mmio_base;
	MV_U32 tmp;

	CORE_DPRINT(("sgpio: sdin done :"));
	{
		mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(sgpio,SGPIO_REG_RAW_DIN0 ),
			tmp);
		CORE_DPRINT((" 0x%08x",tmp));
		
		if(((tmp&SDIN_BACK_PLAN_PRESENCE_PATTERN)==SDIN_BACK_PLAN_PRESENCE_PATTERN)
		&&((tmp&SDIN_DATA_MASK)!=SDIN_DATA_MASK)){
			CORE_DPRINT(("back plan present\n"));
			if((tmp&SDIN_DEVICE0_PRESENCE_PATTERN)==SDIN_DEVICE0_PRESENCE_PATTERN){
				CORE_DPRINT(("device 0 presence\n"));
				
			}
			if((tmp&SDIN_DEVICE1_PRESENCE_PATTERN)==SDIN_DEVICE1_PRESENCE_PATTERN)
			CORE_DPRINT(("device 1 presence\n"));
			if((tmp&SDIN_DEVICE2_PRESENCE_PATTERN)==SDIN_DEVICE2_PRESENCE_PATTERN)
			CORE_DPRINT(("device 2 presence\n"));
			if((tmp&SDIN_DEVICE3_PRESENCE_PATTERN)==SDIN_DEVICE3_PRESENCE_PATTERN)
			CORE_DPRINT(("device 3 presence\n"));
		} else
			CORE_PRINT(("no back plan connected\n"));
	}
	CORE_DPRINT(("\n"));
	/* sgpio 0 */
	mv_sgpio_write_register(mmio,
	SGPIO_REG_ADDR(sgpio,SGPIO_REG_CONTROL),
	((SDOUT_MD_AUTO<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE));

}

void sgpio_isr(MV_PVOID this, MV_U32 sgpio)
{
	core_extension * core=(core_extension *)this;
	MV_LPVOID mmio = core->mmio_base;
	MV_U32 tmp;
	void (*callback)(MV_PVOID PortExtension, MV_PVOID context) = core->lib_gpio.sgpio_callback;

	mv_sgpio_read_register(mmio,
		SGPIO_REG_ADDR(sgpio,SGPIO_REG_INT_CAUSE),
		tmp);
	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(sgpio,SGPIO_REG_INT_CAUSE),
		tmp);

	if(tmp&SDIN_DONE)
		sgpio_process_sdin(this,sgpio);
	else if(tmp&MANUAL_MD_REP_DONE)
		callback(this, core->lib_gpio.sgpio_cb_context);
}

MV_U8 sgpio_send_sdout(MV_PVOID This, MV_U8 byte_len, MV_PVOID pbuffer,
	void (*call_back)(MV_PVOID PortExtension, MV_PVOID context),	MV_PVOID cb_context)
{
	core_extension * pCore=(core_extension *)This;
	MV_LPVOID mmio = pCore->mmio_base;
	MV_U32 tmp,cntrl_tmp;
	MV_U8 i,j;

	if(pCore->lib_gpio.sgpio_sdout_inuse)
		return(MV_FALSE);

	mv_sgpio_read_register(mmio,
	SGPIO_REG_ADDR(0,SGPIO_REG_CONTROL),
	cntrl_tmp);
	cntrl_tmp &=~(SDIN_MD_MASK|SDOUT_MD_MASK|MANUAL_MD_REP_CNT_MASK);
	mv_sgpio_write_register(mmio,
	SGPIO_REG_ADDR(0,SGPIO_REG_CONTROL),
	cntrl_tmp+(SCLK_HALT<<SDOUT_MD_OFFSET)+SDIN_MD_ONCE);

	pCore->lib_gpio.sgpio_sdout_inuse=1;
	pCore->lib_gpio.sgpio_callback=call_back;
	pCore->lib_gpio.sgpio_cb_context=cb_context;
	for (i = 0; (i < (byte_len/4)) && (i < MAX_SDOUT_DWORD); i++) {
		tmp=*((MV_U32 *)pbuffer + i*4);
		mv_sgpio_write_register(mmio,
		        SGPIO_REG_ADDR(0,SGPIO_REG_RAW_DOUT0 + i*4),
		        tmp);
	}
	if ((i<MAX_SDOUT_DWORD) && (byte_len%4)) {
		mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(0,SGPIO_REG_RAW_DOUT0 + i*4),
			tmp);
	for(j=0;j<(byte_len%4);j++)
		tmp |= (*((MV_U8 *)pbuffer + i*4 + j)) << (j*8);
		mv_sgpio_write_register(mmio,
			SGPIO_REG_ADDR(0,SGPIO_REG_RAW_DOUT0 + i*4),
			tmp);
	}

	mv_sgpio_read_register(mmio,
		SGPIO_REG_ADDR(0,SGPIO_REG_CONFIG0),
		tmp);
	tmp &= ~MANUAL_BIT_LEN;
	tmp |= (byte_len*8)<<MANUAL_BIT_LEN_OFFSET;
	mv_sgpio_write_register(mmio,
		SGPIO_REG_ADDR(0,SGPIO_REG_CONFIG0),
		tmp);
	cntrl_tmp +=
	(SDOUT_MD_MAUTO<<SDOUT_MD_OFFSET)+
	SDIN_MD_ONCE+
	(1<<MAUNAL_MD_REP_CNT_OFFSET);
	mv_sgpio_write_register(mmio,
	SGPIO_REG_ADDR(0,SGPIO_REG_CONTROL),
	cntrl_tmp);

	return MV_TRUE;
}

MV_U8 sgpio_read_register(MV_PVOID this, MV_U8  reg_type, MV_U8 reg_indx,
		MV_U8    reg_cnt, MV_PVOID pbuffer)
{
	core_extension * pCore=(core_extension *)this;
	MV_LPVOID mmio = pCore->mmio_base;
	MV_U32 tmp;
	MV_U8 i;

	switch(reg_type){
		case REG_TYPE_CONFIG:
			MV_CopyMemory(pbuffer, (MV_PVOID)&pCore->lib_gpio.sgpio_config, sizeof(struct _sgpio_config_reg));
			break;
		case REG_TYPE_RX:
			for (i = 0; (i < reg_cnt) && (i < MAX_SDIN_DWORD); i++) {
				mv_sgpio_read_register(mmio,
				SGPIO_REG_ADDR(0,SGPIO_REG_RAW_DIN0 + (reg_indx + i)*4),
				tmp);
				*((MV_U32 *)pbuffer + i*4)=tmp;
			}
			break;
		case REG_TYPE_RX_GP:
			for (i = 0; (i < reg_cnt) && ((reg_indx + i) < (MAX_SDIN_DWORD+1)); i++) {
				if((reg_indx+i)==0)
					*(MV_U32 *)(pbuffer)=0;
				else {
					mv_sgpio_read_register(mmio,
						SGPIO_REG_ADDR(0,
						SGPIO_REG_RAW_DIN0 + (reg_indx + i -1)*4),
						tmp);
					*((MV_U32 *)pbuffer + i*4)=tmp;
				}
			}
			break;
		case REG_TYPE_TC:
			for(i = 0; (i < reg_cnt) && (i < MAX_AUTO_CTRL_DWORD); i++) {
			mv_sgpio_read_register(mmio,
			SGPIO_REG_ADDR(0,SGPIO_REG_DRV_CTRL_BASE + (reg_indx + i)*4),
			tmp);
			*((MV_U32 *)pbuffer + i*4)=tmp;
			}
			break;
		case REG_TYPE_TC_GP:
			for (i = 0; (i < reg_cnt) && (i < MAX_SDOUT_DWORD); i++) {
				if((reg_indx+i)==0){
					mv_sgpio_read_register(mmio,
						SGPIO_REG_ADDR(0,SGPIO_REG_CONTROL),
						tmp);
					tmp &=~(MANUAL_MD_SLOAD_PTRN_MASK|AUTO_MD_SLOAD_PTRN_MASK);
					tmp|=((*((MV_U8 *)pbuffer+3))<<16 )+
					((*((MV_U8 *)pbuffer+3))<<20 );
					mv_sgpio_write_register(mmio,
						SGPIO_REG_ADDR(0,SGPIO_REG_CONTROL),tmp);
				} else {
					mv_sgpio_read_register(mmio,
						SGPIO_REG_ADDR(0,SGPIO_REG_RAW_DOUT0 + (reg_indx + i -1)*4),
						tmp);
					*((MV_U32 *)pbuffer + i*4) = tmp;
				}
			}
			break;
		default:
			return(MV_FALSE);
	}
	return(MV_TRUE);
}

MV_U8 sgpio_write_register( MV_PVOID this,MV_U8 reg_type,MV_U8 reg_indx,
	MV_U8 reg_cnt, MV_PVOID pbuffer)
{
	core_extension * pCore=(core_extension *)this;
	MV_LPVOID mmio = pCore->mmio_base;
	MV_U32 tmp;
	MV_U8 i;

	switch (reg_type) {
		case REG_TYPE_CONFIG:
			MV_CopyMemory((MV_PVOID)&pCore->lib_gpio.sgpio_config, pbuffer, sizeof(struct _sgpio_config_reg));
			break;
		case REG_TYPE_TC:
			for (i = 0; (i < reg_cnt) && (i < MAX_AUTO_CTRL_DWORD); i++) {
				tmp = *((MV_U32 *)pbuffer + i*4);
				mv_sgpio_write_register(mmio,
					SGPIO_REG_ADDR(0,SGPIO_REG_DRV_CTRL_BASE + (reg_indx + i)*4), tmp);
			}
			break;
		case REG_TYPE_TC_GP:
			for(i=0;(i<reg_cnt)&&(i<MAX_SDOUT_DWORD);i++){
				mv_sgpio_read_register(mmio,
					SGPIO_REG_ADDR(0,SGPIO_REG_RAW_DOUT0 + (reg_indx + i)*4),
					tmp);
				*((MV_U32 *)pbuffer + i*4) = tmp;
			}
			break;
		case REG_TYPE_RX:
			default:
		return(MV_FALSE);
	}
	return(MV_TRUE);
}

