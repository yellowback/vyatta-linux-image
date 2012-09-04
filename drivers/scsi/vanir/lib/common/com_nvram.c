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
#include "mv_include.h"
#include "core_header.h"
#include "core_spi.h"
#include "com_nvram.h"
#include "hba_inter.h"
#include "hba_exp.h"

#define SERIAL_NUM_SIG  "MRVL"

static void Itoa(MV_PU8 buffer, MV_U16 data)
{
	MV_U16 ndig;
	MV_U8 char_hex;
	MV_U16 tmp, tmp_hex;
	MV_U16 ttchar;

	tmp=data; ndig=1;

    while(tmp/16>0){
		ndig++;
		tmp/=16;
    }

    tmp=data;
    while(ndig){
    		ttchar = tmp%16;
    		switch(ttchar){
    			case 10:
    				buffer[ndig-1] = 'a';
    				break;
    			case 11:
    				buffer[ndig-1] = 'b';
    				break;
    			case 12:
    				buffer[ndig-1] =  'c';
    				break;
    			case 13:
    				buffer[ndig-1] = 'd';
    				break;
    			case 14:
    				buffer[ndig-1] = 'e';
    				break;
    			case 15:
    				buffer[ndig-1] = 'f';
    				break;
    			default:
    				buffer[ndig-1] = (MV_U8)ttchar + '0';
   		}
		//buffer[ndig-1] = char_hex;
        tmp/=16;
		ndig--;
    }
}

MV_BOOLEAN mv_nvram_moddesc_init_param( MV_PVOID This, pHBA_Info_Page pHBA_Info_Param)
{
	MV_U32 	param_flash_addr=PARAM_OFFSET,i = 0;
       core_extension	*core;
	AdapterInfo	AI;
	MV_BOOLEAN is_pre_init = MV_TRUE;
	MV_U32 time;
	MV_U16 temp;
	MV_U8 buffer[10];
	struct mod_notif_param param_bad = {NULL, 0, 0, EVT_ID_FLASH_FLASH_ERR, 0,  SEVERITY_WARNING, 0,0,NULL,0 };
	struct mod_notif_param param = {NULL, 0, 0, EVT_ID_FLASH_ERASE_ERR, 0,  SEVERITY_WARNING, 0,0,NULL,0};
	Controller_Infor Controller;
	PHBA_Extension phba_ext=NULL;

	if (!This)
		return MV_FALSE;

	if(!is_pre_init){
		phba_ext = (PHBA_Extension)HBA_GetModuleExtension(This, MODULE_HBA);
	}

       if (is_pre_init){
		 mv_hba_get_controller_pre(This, &Controller);
	} else {
		HBA_GetControllerInfor(This, &Controller);
	}

	AI.bar[5] = Controller.Base_Address[5];
	AI.bar[2] = Controller.Base_Address[2];

	if (-1 == OdinSPI_Init(&AI)) {
		MV_PRINT("Init flash rom failed.\n");
		return MV_FALSE;
	}

    	/* step 1 read param from flash offset = 0x3FFF00 */
	OdinSPI_ReadBuf( &AI, param_flash_addr, (MV_PU8)pHBA_Info_Param, FLASH_PARAM_SIZE);

	/* step 2 check the signature first */
	if(pHBA_Info_Param->Signature[0] == 'M'&& \
	    pHBA_Info_Param->Signature[1] == 'R'&& \
	    pHBA_Info_Param->Signature[2] == 'V'&& \
	    pHBA_Info_Param->Signature[3] == 'L' && \
	    (!mvVerifyChecksum((MV_PU8)pHBA_Info_Param,FLASH_PARAM_SIZE)))
	{
		if(pHBA_Info_Param->HBA_Flag == 0xFFFFFFFFL)
		{
			pHBA_Info_Param->HBA_Flag = 0;
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_INT13_ENABLE;
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_SILENT_MODE_ENABLE;
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_AUTO_REBUILD_ON; 
			pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SMART_ON;
		}
		
		if(pHBA_Info_Param->BGA_Rate == 0xff)
			pHBA_Info_Param->BGA_Rate = 0x7D;
		if(pHBA_Info_Param->Sync_Rate == 0xff)
			pHBA_Info_Param->Sync_Rate = 0x7D;
		if(pHBA_Info_Param->Init_Rate == 0xff)
			pHBA_Info_Param->Init_Rate = 0x7D;
		if(pHBA_Info_Param->Rebuild_Rate == 0xff)
			pHBA_Info_Param->Rebuild_Rate = 0x7D;

		for(i=0;i<8;i++)
		{
			if(pHBA_Info_Param->PHY_Rate[i]>0x2)
				pHBA_Info_Param->PHY_Rate[i] = 0x2; /*6Gbps*/
		}
	}
	else
	{
		MV_FillMemory((MV_PVOID)pHBA_Info_Param, FLASH_PARAM_SIZE, 0xFF);
		pHBA_Info_Param->Signature[0] = 'M';	
		pHBA_Info_Param->Signature[1] = 'R';
	   	pHBA_Info_Param->Signature[2] = 'V';
	  	pHBA_Info_Param->Signature[3] = 'L';

		// Set BIOS Version
		pHBA_Info_Param->Minor = NVRAM_DATA_MAJOR_VERSION;
		pHBA_Info_Param->Major = NVRAM_DATA_MINOR_VERSION;

		// Set SAS address
		for(i=0;i<MAX_NUMBER_IO_CHIP*MAX_PORT_PER_PL;i++) {
			pHBA_Info_Param->SAS_Address[i].b[0]=  0x50;
			pHBA_Info_Param->SAS_Address[i].b[1]=  0x05;
			pHBA_Info_Param->SAS_Address[i].b[2]=  0x04;
			pHBA_Info_Param->SAS_Address[i].b[3]=  0x30;
			pHBA_Info_Param->SAS_Address[i].b[4]=  0x11;
			pHBA_Info_Param->SAS_Address[i].b[5]=  0xab;
			pHBA_Info_Param->SAS_Address[i].b[6]=  (MV_U8)(i/4);
			pHBA_Info_Param->SAS_Address[i].b[7]=  0x00; 
		}
		
		/* init phy link rate */
		for(i=0;i<8;i++)
		{
			pHBA_Info_Param->PHY_Rate[i] = 0x2; /*Default is 6Gbps*/
		}

		MV_PRINT("pHBA_Info_Param->HBA_Flag = 0x%x \n",pHBA_Info_Param->HBA_Flag);

		pHBA_Info_Param->BGA_Rate = 0x7D;
		pHBA_Info_Param->Rebuild_Rate = 0x7D;
		pHBA_Info_Param->Init_Rate = 0x7D;
		pHBA_Info_Param->Sync_Rate = 0x7D;
		
		/* init setting flags */
		pHBA_Info_Param->HBA_Flag = 0;
		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_INT13_ENABLE;
		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_SILENT_MODE_ENABLE;
		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_AUTO_REBUILD_ON; 
		pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SMART_ON;

		MV_DPRINT(("Add serial number.\n"));
		MV_CopyMemory((MV_PU8)(pHBA_Info_Param->Serial_Num), SERIAL_NUM_SIG , 4);
		temp = (MV_U16)(Controller.Device_Id);
		MV_DPRINT(("%x.\n",Controller.Device_Id));
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[4])),buffer,4);
		time = HBA_GetMillisecondInDay();
		temp = (MV_U16)time;
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[8])),buffer, 4);
		temp=(MV_U16)(time>>16);
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[12])),buffer, 4);
		time = HBA_GetTimeInSecond();
		temp = (MV_U16)time;
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[16])),buffer, 4);

		/* set next page of HBA info Page*/
		pHBA_Info_Param->Next_Page = (MV_U16)(PAGE_INTERVAL_DISTANCE+FLASH_PD_INFO_PAGE_SIZE);

		/* write to flash and save it now */
		if(OdinSPI_SectErase( &AI, param_flash_addr) != -1) {
			MV_DPRINT(("mv_nvram_init_param: FLASH ERASE SUCCESS!\n"));
		}
		else {
			MV_DPRINT(("mv_nvram_init_param: FLASH ERASE FAILED!\n"));
			if(phba_ext) {
			phba_ext->FlashErase = 1; 
			if(phba_ext->State == DRIVER_STATUS_STARTED)
				HBA_ModuleNotification(phba_ext, EVENT_LOG_GENERATED,&param);
			}
		}

		pHBA_Info_Param->Check_Sum = 0;
		pHBA_Info_Param->Check_Sum=mvCalculateChecksum((MV_PU8)pHBA_Info_Param,sizeof(HBA_Info_Page))
			;
		/* init the parameter in ram */
		if(OdinSPI_WriteBuf( &AI, param_flash_addr, (MV_PU8)pHBA_Info_Param, FLASH_PARAM_SIZE) != -1) {
			MV_DPRINT(("mv_nvram_init_param: FLASH RECOVER SUCCESS!\n"));
		}
		else {
			MV_DPRINT(("mv_nvram_init_param: FLASH RECOVER FAILED!\n"));
			if(phba_ext) {
			phba_ext->FlashBad = 1; 
			if(phba_ext->State == DRIVER_STATUS_STARTED)
				HBA_ModuleNotification(phba_ext, EVENT_LOG_GENERATED,&param_bad);
			}
		}
	}

	return MV_TRUE;
}

MV_BOOLEAN mv_nvram_init_param( MV_PVOID _phba_ext, pHBA_Info_Page pHBA_Info_Param)
{
	MV_U32 	param_flash_addr=PARAM_OFFSET,i = 0;
	AdapterInfo	AI;
	PHBA_Extension phba_ext = ( PHBA_Extension )_phba_ext;
	MV_U32 time;
	MV_U16 temp;
	MV_U8 buffer[10];
	struct mod_notif_param param_bad = {NULL, 0, 0, EVT_ID_FLASH_FLASH_ERR, 0,  SEVERITY_WARNING, 0,0,NULL,0 };
	struct mod_notif_param param = {NULL, 0, 0, EVT_ID_FLASH_ERASE_ERR, 0,  SEVERITY_WARNING, 0,0,NULL,0};
	Controller_Infor Controller;

	if ( !phba_ext   )
		return MV_FALSE;

	HBA_GetControllerInfor( phba_ext, &Controller);

	AI.bar[5] = Controller.Base_Address[5];
	AI.bar[2] = Controller.Base_Address[2];

	if (-1 == OdinSPI_Init(&AI)) {
		MV_PRINT("Init flash rom failed.\n");
		return MV_FALSE;
	}

    	/* step 1 read param from flash offset = 0x3FFF00 */
	OdinSPI_ReadBuf( &AI, param_flash_addr, (MV_PU8)pHBA_Info_Param, FLASH_PARAM_SIZE);

	/* step 2 check the signature first */
	if(pHBA_Info_Param->Signature[0] == 'M'&& \
	    pHBA_Info_Param->Signature[1] == 'R'&& \
	    pHBA_Info_Param->Signature[2] == 'V'&& \
	    pHBA_Info_Param->Signature[3] == 'L' && \
	    (!mvVerifyChecksum((MV_PU8)pHBA_Info_Param,FLASH_PARAM_SIZE)))
	{
		if(pHBA_Info_Param->HBA_Flag == 0xFFFFFFFFL)
		{
			pHBA_Info_Param->HBA_Flag = 0;
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_INT13_ENABLE;
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_SILENT_MODE_ENABLE;
			pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_AUTO_REBUILD_ON; 
			pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SMART_ON;
			pHBA_Info_Param->HBA_Flag |= HBA_FLAG_ENABLE_BUZZER;
		}
		
		if(pHBA_Info_Param->BGA_Rate == 0xff)
			pHBA_Info_Param->BGA_Rate = 0x7D;
		if(pHBA_Info_Param->Sync_Rate == 0xff)
			pHBA_Info_Param->Sync_Rate = 0x7D;
		if(pHBA_Info_Param->Init_Rate == 0xff)
			pHBA_Info_Param->Init_Rate = 0x7D;
		if(pHBA_Info_Param->Rebuild_Rate == 0xff)
			pHBA_Info_Param->Rebuild_Rate = 0x7D;
		for(i=0;i<8;i++)
		{
			if(pHBA_Info_Param->PHY_Rate[i] >= 0x2)
				pHBA_Info_Param->PHY_Rate[i] = 0x2; /*6Gbps*/
		}
	}
	else
	{
		MV_FillMemory((MV_PVOID)pHBA_Info_Param, FLASH_PARAM_SIZE, 0xFF);
		pHBA_Info_Param->Signature[0] = 'M';	
		pHBA_Info_Param->Signature[1] = 'R';
	   	pHBA_Info_Param->Signature[2] = 'V';
	  	pHBA_Info_Param->Signature[3] = 'L';

		// Set BIOS Version
		pHBA_Info_Param->Minor = NVRAM_DATA_MAJOR_VERSION;
		pHBA_Info_Param->Major = NVRAM_DATA_MINOR_VERSION;

		// Set SAS address
		for(i=0;i<MAX_NUMBER_IO_CHIP*MAX_PORT_PER_PL;i++) {
			pHBA_Info_Param->SAS_Address[i].b[0]=  0x50;
			pHBA_Info_Param->SAS_Address[i].b[1]=  0x05;
			pHBA_Info_Param->SAS_Address[i].b[2]=  0x04;
			pHBA_Info_Param->SAS_Address[i].b[3]=  0x30;
			pHBA_Info_Param->SAS_Address[i].b[4]=  0x11;
			pHBA_Info_Param->SAS_Address[i].b[5]=  0xab;
			pHBA_Info_Param->SAS_Address[i].b[6]=  (MV_U8)(i/4);
			pHBA_Info_Param->SAS_Address[i].b[7]=  0x00; 
		}
		
		/* init phy link rate */
		for(i=0;i<8;i++)
		{
			pHBA_Info_Param->PHY_Rate[i] = 0x2; /*Default is 6Gbps*/
		}

		MV_PRINT("pHBA_Info_Param->HBA_Flag = 0x%x \n",pHBA_Info_Param->HBA_Flag);

		/* init bga rate */
		pHBA_Info_Param->BGA_Rate = 0x7D;

		pHBA_Info_Param->Rebuild_Rate = 0x7D;
		pHBA_Info_Param->Init_Rate = 0x7D;
		pHBA_Info_Param->Sync_Rate = 0x7D;
		
		/* init setting flags */
		pHBA_Info_Param->HBA_Flag = 0;
		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_INT13_ENABLE;
		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_SILENT_MODE_ENABLE;
		pHBA_Info_Param->HBA_Flag &= ~HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY;
		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_AUTO_REBUILD_ON; 
		pHBA_Info_Param->HBA_Flag &= ~ HBA_FLAG_SMART_ON;
		pHBA_Info_Param->HBA_Flag |= HBA_FLAG_ENABLE_BUZZER;

		MV_DPRINT(("Add serial number.\n"));
		MV_CopyMemory((MV_PU8)(pHBA_Info_Param->Serial_Num), SERIAL_NUM_SIG , 4);

		temp = (MV_U16)(Controller.Device_Id);
		MV_DPRINT(("%x.\n",Controller.Device_Id));

		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[4])),buffer,4);
		time = HBA_GetMillisecondInDay();
		temp = (MV_U16)time;
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[8])),buffer, 4);
		temp=(MV_U16)(time>>16);
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[12])),buffer, 4);
		time = HBA_GetTimeInSecond();
		temp = (MV_U16)time;
		Itoa(buffer,temp);
		MV_CopyMemory((MV_PU8)(&(pHBA_Info_Param->Serial_Num[16])),buffer, 4);

		/* set next page of HBA info Page*/
		pHBA_Info_Param->Next_Page = (MV_U16)(PAGE_INTERVAL_DISTANCE+FLASH_PD_INFO_PAGE_SIZE);

		/* write to flash and save it now */
		if(OdinSPI_SectErase( &AI, param_flash_addr) != -1) {
			MV_DPRINT(("mv_nvram_init_param: FLASH ERASE SUCCESS!\n"));
		}
		else {
			MV_DPRINT(("mv_nvram_init_param: FLASH ERASE FAILED!\n"));
			if(phba_ext) {
			phba_ext->FlashErase = 1; 
			if(phba_ext->State == DRIVER_STATUS_STARTED)
				HBA_ModuleNotification(phba_ext, EVENT_LOG_GENERATED,&param);
			}
		}

		pHBA_Info_Param->Check_Sum = 0;
		pHBA_Info_Param->Check_Sum=mvCalculateChecksum((MV_PU8)pHBA_Info_Param,sizeof(HBA_Info_Page));
		
		/* init the parameter in ram */
		if(OdinSPI_WriteBuf( &AI, param_flash_addr, (MV_PU8)pHBA_Info_Param, FLASH_PARAM_SIZE) != -1) {
			MV_DPRINT(("mv_nvram_init_param: FLASH RECOVER SUCCESS!\n"));
		}
		else {
			MV_DPRINT(("mv_nvram_init_param: FLASH RECOVER FAILED!\n"));
			if(phba_ext) {
			phba_ext->FlashBad = 1; 
			if(phba_ext->State == DRIVER_STATUS_STARTED)
				HBA_ModuleNotification(phba_ext, EVENT_LOG_GENERATED,&param_bad);
			}
		}
	}

	return MV_TRUE;
}
/* Caution: Calling this function, please do Read-Modify-Write. 
 * Please call to get the original data first, then modify corresponding field,
 * Then you can call this function. */
MV_BOOLEAN mvuiHBA_modify_param( MV_PVOID This, pHBA_Info_Page pHBA_Info_Param)
{
       core_extension *p_core_ext;
	MV_U32	param_flash_addr = PARAM_OFFSET;
	AdapterInfo				AI;
	PHBA_Extension	pHBA = NULL;

	if (!This)
		return MV_FALSE;

	pHBA = (PHBA_Extension)HBA_GetModuleExtension(This,MODULE_HBA);

	AI.bar[5] = pHBA->Base_Address[5];
	AI.bar[2] = pHBA->Base_Address[2];
	if (-1 == OdinSPI_Init(&AI))
		return MV_FALSE;

	if ( pHBA_Info_Param->Signature[0]!= 'M'
		|| pHBA_Info_Param->Signature[1]!= 'R'
		|| pHBA_Info_Param->Signature[2]!= 'V'
		|| pHBA_Info_Param->Signature[3]!= 'L' ) {
		MV_DASSERT( MV_FALSE );
		return MV_FALSE;
	}

	p_core_ext = HBA_GetModuleExtension(This, MODULE_CORE);
	if( p_core_ext == NULL )
		return MV_FALSE;

	pHBA_Info_Param->Check_Sum = 0;
	pHBA_Info_Param->Check_Sum=mvCalculateChecksum((MV_PU8)pHBA_Info_Param,sizeof(HBA_Info_Page));

        if (core_rmw_write_flash(p_core_ext, PARAM_OFFSET,
                (MV_PU8)pHBA_Info_Param, sizeof(HBA_Info_Page)) == MV_FALSE) {
	        if(OdinSPI_SectErase( &AI, param_flash_addr) != -1) {
		        MV_DPRINT(("mvuiHBA_modify_param: FLASH ERASE SUCCESS!\n"));
	        } else {
		        MV_DPRINT(("mvuiHBA_modify_param: FLASH ERASE FAILED!\n"));
                        MV_DASSERT(MV_FALSE);
	        }
                OdinSPI_WriteBuf(&AI, param_flash_addr, (MV_PU8)pHBA_Info_Param,
                        sizeof(HBA_Info_Page));
        }

	return MV_TRUE;
}


MV_U8	mvCalculateChecksum(MV_PU8	Address, MV_U32 Size)
{
	MV_U8 checkSum;
	MV_U32 temp=0;
       checkSum = 0;
       for (temp = 0 ; temp < Size ; temp ++)
       {
                checkSum += Address[temp];
       }
        
       checkSum = (~checkSum) + 1;

	return	checkSum;
}

MV_U8	mvVerifyChecksum(MV_PU8	Address, MV_U32 Size)
{
	MV_U8	checkSum=0;
	MV_U32 	temp=0;
       for (temp = 0 ; temp < Size ; temp ++)
       {
            checkSum += Address[temp];
       }
        
	return	checkSum;
}

MV_BOOLEAN mv_page_signature_check( MV_PU8 Signature )
{
	if(	Signature[0] == 'M'&& \
		Signature[1] == 'R'&& \
		Signature[2] == 'V'&& \
		Signature[3] == 'L')
		return MV_TRUE;
	else
		return MV_FALSE;
	
}
