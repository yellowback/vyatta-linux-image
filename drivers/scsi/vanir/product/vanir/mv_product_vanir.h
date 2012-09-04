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
#ifndef __MV_PRODUCT_VANIR_H__
#define __MV_PRODUCT_VANIR_H__

#define TRASH_BUCKET_SIZE 0x400  // 1k

/* driver capabilities */
#define MAX_BASE_ADDRESS                6
#define MV_MAX_TRANSFER_SIZE            (4*1024*1024)
#define MAX_SG_ENTRY                    130
#define MAX_SG_ENTRY_REDUCED            16
#define MV_MAX_PHYSICAL_BREAK           (MAX_SG_ENTRY - 1)

#define MAX_REQUEST_NUMBER_PERFORMANCE	    4096
#define MAX_REQUEST_PER_LUN_PERFORMANCE		128
#define MV_MAX_TARGET_NUMBER            128
#define MAX_EXPANDER_SUPPORTED		      10

/* hardware capabilities */
#define MAX_PM_SUPPORTED                      8
#define MAX_BLOCK_PER_HD_SUPPORTED            8
#define MAX_DEVICE_SUPPORTED_WHQL             8
#define MAX_DEVICE_SUPPORTED_PERFORMANCE      MV_MAX_TARGET_NUMBER
#define MAX_DEVICE_SUPPORTED_RAID             64

#endif/*__MV_PRODUCT_VANIR_H__*/
