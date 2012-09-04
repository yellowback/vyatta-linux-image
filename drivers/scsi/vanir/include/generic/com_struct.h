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
#ifndef __MV_COM_STRUCT_H__
#define __MV_COM_STRUCT_H__

#include "com_define.h"

#define GET_ALL                                 0xFF
#define ID_UNKNOWN                              0x7F

#define INVALID_ID                              0xFF
#define UNDEFINED_ID                            0xFFFF


#define MAX_NUM_ADAPTERS                        4

#include "com_adapter_struct.h"

#include "com_pd_struct.h"
#include "com_enc_struct.h"

#include "com_dbg_struct.h"
#include "com_bbu_struct.h"

#include "com_vd_struct.h"
#include "com_array_struct.h"

#include "com_passthrough_struct.h"
#include "com_flash_struct.h"

#include "com_request_header.h"
#include "com_request_detail.h"

#endif /*  __MV_COM_STRUCT_H__ */
