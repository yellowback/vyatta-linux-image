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
#if !defined( _SPI_HAL_H_ )
#define _SPI_HAL_H_

#include "core_header.h"

#define FMemR8( addr, off  )         MV_REG_READ_BYTE( addr, off )
#define FMemR16( addr, off )         MV_REG_READ_WORD( addr, off )
#define FMemR32( addr, off )         MV_REG_READ_DWORD( addr, off )
#define FMemW8( addr, off, var )     MV_REG_WRITE_BYTE( addr, off, var )
#define FMemW16( addr, off, var )    MV_REG_WRITE_WORD( addr, off, var )
#define FMemW32( addr, off, var )    MV_REG_WRITE_DWORD( addr, off, var )
#define MV_IOR8( addr, off )         MV_IO_READ_BYTE( addr, off )
#define MV_IOR16( addr, off )        MV_IO_READ_WORD( addr, off )
#define MV_IOR32( addr, off )        MV_IO_READ_DWORD( addr, off )
#define MV_IOW8( addr, off, data )   MV_IO_WRITE_BYTE( addr, off, data )
#define MV_IOW16( addr, off, data )  MV_IO_WRITE_WORD( addr, off, data )
#define MV_IOW32( addr, off, data )  MV_IO_WRITE_DWORD( addr, off, data )

#   define DelayMSec(_X_)        HBA_SleepMillisecond(NULL, _X_)
#   define DelayUSec(_X_)        HBA_SleepMicrosecond(NULL, _X_)

#endif
