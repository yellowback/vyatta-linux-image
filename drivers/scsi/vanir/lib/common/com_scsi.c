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
#include "com_type.h"
#include "com_define.h"
#include "com_dbg.h"
#include "com_scsi.h"
#include "com_util.h"

/* NODRV device is used to send controller commands. 
 * Now we are using "Storage array controller device" as Microsoft recommended.
 * Peripheral Device Type: 03h Processor ( can be 3h or 0ch )
 * Peripheral Qualifier: 0h
 * Response Data Format: 2h ( must be 2 )
 * Version: 4h ( must be 4, 5 or 6 ) 
 * Only need support minimum 36 bytes inquiry data. 
 * Must return EVPD 0x0, 0x83, 0x80 */

	/* If VIRTUAL_DEVICE_TYPE==0x10, Device ID is SCSI\BridgeMARVELL_Virtual_Device__
	 * If VIRTUAL_DEVICE_TYPE==0x0C, Device ID is SCSI\ArrayMARVELL_Virtual_Device__
	 * If VIRTUAL_DEVICE_TYPE==0x03, Device ID is SCSI\ProcessorMARVELL_Virtual_Device__ */
	#define VIRTUAL_DEVICE_TYPE	0x0C
	/* Standard Inquiry Data for Virtual Device */
	MV_U8 BASEATTR MV_INQUIRY_VIRTUALD_DATA[] = {
					VIRTUAL_DEVICE_TYPE,0x00,0x04,0x02,0x20,0x00,0x00,0x00,//?Version should be 0x4 instead of 0x2.
					'M', 'A', 'R', 'V', 'E', 'L', 'L', ' ',
					'V', 'i', 'r', 't', 'u', 'a', 'l', ' ',
					'D', 'e', 'v', 'i', 'c', 'e', ' ', ' ',
					0x31,0x2E,0x30,0x30
					};

	/* EVPD Inquiry Page for Virtual Device */
	MV_U8 BASEATTR MV_INQUIRY_VPD_PAGE0_VIRTUALD_DATA[] = {
		VIRTUAL_DEVICE_TYPE, 0x00, 0x00, 0x03, 0x00, 0x80, 0x83};

	MV_U8 BASEATTR MV_INQUIRY_VPD_PAGE80_VIRTUALD_DATA[] = {
		VIRTUAL_DEVICE_TYPE, 0x80, 0x00, 0x08, 'V', ' ', 'D', 'e', 'v', 'i', 'c', 'e'};

	MV_U8 BASEATTR MV_INQUIRY_VPD_PAGE83_VIRTUALD_DATA[] = {
		VIRTUAL_DEVICE_TYPE, 0x83, 0x00, 0x14, 0x02, 0x01, 0x00, 0x10,
		'M',  'A',  'R',  'V',  'E',  'L',  'L',  ' ',	/* T10 Vendor Identification */
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01	/* Vendor Specific Identifier */
	};

MV_VOID MV_SetSenseData(
	IN PMV_Sense_Data pSense,
	IN MV_U8 SenseKey,
	IN MV_U8 AdditionalSenseCode,
	IN MV_U8 ASCQ
	)
{
	/* The caller should make sure it's a valid sense buffer. */
	MV_DASSERT( pSense!=NULL );

	if ( pSense!=NULL ) {
		MV_ZeroMemory(pSense, sizeof(MV_Sense_Data));

		pSense->Valid = 0;
		pSense->ErrorCode = MV_SCSI_RESPONSE_CODE;
		pSense->SenseKey = SenseKey;
		pSense->AdditionalSenseCode = AdditionalSenseCode;
		pSense->AdditionalSenseCodeQualifier = ASCQ;
		pSense->AdditionalSenseLength = sizeof(MV_Sense_Data) - 8;
	}
}

