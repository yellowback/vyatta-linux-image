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
#ifndef __MV_COM_FLASH_STRUCT_H__
#define __MV_COM_FLASH_STRUCT_H__

#define 	 FLASH_DOWNLOAD                 0xf0
#define	 FLASH_UPLOAD                       0xf

//for read/write flash test command
#define	 FLASH_BYTE_WRITE			0
#define 	 FLASH_BYTE_READ			1

#define    FLASH_TYPE_CONFIG              0
#define    FLASH_TYPE_BIN                    1
#define    FLASH_TYPE_BIOS                  2
#define    FLASH_TYPE_FIRMWARE         3
#define    FLASH_TYPE_AUTOLOAD         4	


#define 	FLASH_ERASE_PAGE                      0x1  //Erase bios or PD page or hba info page
#define	FLASH_PD_PAGE					1	// Erase PD page in flash memory but not in-uses PD id 
#define	FLASH_PD_PAGE_FORCE			254	// Force to erase the whole PD page even PD id is in-use. Used by manufacturing only!

#endif
