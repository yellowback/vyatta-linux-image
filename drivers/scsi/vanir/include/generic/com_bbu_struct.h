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
#ifndef __MV_COM_BBU_STRUCT_H__
#define __MV_COM_BBU_STRUCT_H__

#include "com_define.h"

#pragma pack(8)

/* battery status */
#define BBU_STATUS_NOT_PRESENT          0
#define BBU_STATUS_PRESENT              MV_BIT(0)
#define BBU_STATUS_LOW_BATTERY          MV_BIT(1)
#define BBU_STATUS_CHARGING             MV_BIT(2)
#define BBU_STATUS_FULL_CHARGED         MV_BIT(3)
#define BBU_STATUS_DISCHARGE            MV_BIT(4)
#define BBU_STATUS_RELEARN              MV_BIT(5)
#define BBU_STATUS_OVER_TEMP_WARNING    MV_BIT(6)
#define BBU_STATUS_OVER_TEMP_ERROR      MV_BIT(7)
#define BBU_STATUS_UNDER_TEMP_WARNING   MV_BIT(8)
#define BBU_STATUS_UNDER_TEMP_ERROR     MV_BIT(9)
#define BBU_STATUS_OVER_VOLT_WARNING    MV_BIT(10)
#define BBU_STATUS_OVER_VOLT_ERROR      MV_BIT(11)
#define BBU_STATUS_UNDER_VOLT_WARNING   MV_BIT(12)
#define BBU_STATUS_UNDER_VOLT_ERROR     MV_BIT(13)
#define BBU_STATUS_GREATER_LOWERBOUND	MV_BIT(14)
#define BBU_STATUS_POWER_STOP_ALL	MV_BIT(15)

#define BBU_NOT_PRESENT         0
#define BBU_NORMAL                   1
#define BBU_ABNORMAL               2

#define BBU_SUPPORT_NONE                 0
#define BBU_SUPPORT_SENSOR_TEMPERATURE   MV_BIT(0)
#define BBU_SUPPORT_SENSOR_VOLTAGE       MV_BIT(1)
#define BBU_SUPPORT_SENSOR_ECAPACITY     MV_BIT(2)
#define BBU_SUPPORT_CHANGE_MAXEC         MV_BIT(3)
#define BBU_SUPPORT_RELEARN              MV_BIT(4)

#define BBU_SENSOR_NOTSUPPORT    0
#define BBU_SENSOR_TEMPERATURE   MV_BIT(0)
#define BBU_SENSOR_VOLTAGE       MV_BIT(1)
#define BBU_SENSOR_ECAPACITY     MV_BIT(2)
#define BBU_CHANGE_MAXEC         MV_BIT(3)
#define BBU_SUPPORT_RELEARN      MV_BIT(4)

typedef struct _BBU_Info
{
    //Total 64 Bytes
    MV_U32          status;
    MV_U32          prev_status;
    
    MV_U16          voltage; /* unit is 1 mV */
    MV_U16          supportMaxCapacity;
    MV_U16          supportMinCapacity;
    MV_U16          maxCapacity;
    
    MV_U16          curCapacity;
    MV_U16          time_to_empty;
    MV_U16          time_to_full;
    MV_U16          recharge_cycle;

    MV_U16          featureSupport;
    MV_I16          temperature; /*unit is 0.01 Celsius */
    MV_U16           volt_lowerbound;
    MV_U16           volt_upperbound;
    
    MV_U8           temp_lowerbound;
    MV_U8           temp_upperbound;
    MV_U8           percentage;			// current percentage of barrtery capacity.
    MV_U8           percent_to_charge;	// if percentage is lower than this number,it should begin to charge.
    MV_U8           flags;
    MV_U8           reserved1[3];
    
    MV_U8           reserved2[24];
} BBU_Info, *PBBU_Info;

#define BBU_ACT_RELEARN          0
#define BBU_ACT_FORCE_CHARGE     1
#define BBU_ACT_FORCE_DISCHARGE  2
#define BBU_ACT_STOP_ALL         3

#define BBU_THRESHOLD_TYPE_CAPACITY        0
#define BBU_THRESHOLD_TYPE_TEMPERATURE     1
#define BBU_THRESHOLD_TYPE_VOLTAGE         2

#pragma pack()

#endif
