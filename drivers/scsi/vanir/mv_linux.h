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
#ifndef __MV_LINUX_H__
#define __MV_LINUX_H__

#define CACHE_NAME_LEN 			32

/*Max ID report to OS: HD+Enclosure+PM+virtual device*/
#define MV_MAX_ID                                      \
        (MV_MAX_TARGET_NUMBER + MAX_EXPANDER_SUPPORTED + \
         MAX_PM_SUPPORTED + 1)/*reserved 1 ID for virtual device ID*/			

#endif /* __MV_LINUX_H__ */

