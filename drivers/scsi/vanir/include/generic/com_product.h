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
#ifndef COM_PRODUCT_H
#define COM_PRODUCT_H
/* Product device id */
#define VENDOR_ID					        0x11AB
#define VENDOR_ID_EXT                       0x1B4B
#define DEVICE_ID_ODIN                      0x6400
#define DEVICE_ID_THORLITE_2S1P				0x6121
#define DEVICE_ID_THORLITE_0S1P				0x6101
#define DEVICE_ID_THORLITE_1S1P				0x6111
#define DEVICE_ID_THOR_4S1P				    0x6141
#define DEVICE_ID_THOR_4S1P_NEW				0x6145
/* Revision ID starts from B1 */
#define DEVICE_ID_THORLITE_2S1P_WITH_FLASH              0x6122

 /* Odin lite version */
#define DEVICE_ID_6320					0x6320
#define DEVICE_ID_6340					0x6340 

/* mule-board */
#define DEVICE_ID_6440					0x6440

/* Non-RAID Odin */
#define DEVICE_ID_6445					0x6445

/* mule-board */
#define DEVICE_ID_6480					0x6480
#define DEVICE_ID_6485					0x6485


#define DEVICE_ID_8180					0x8180
#define DEVICE_ID_8440					0x8440
#define DEVICE_ID_8480					0x8480
#define DEVICE_ID_8110					0x8110
#define DEVICE_ID_8120					0x8120

/* Magni B0 */
#define DEVICE_ID_9123					0x9123
#define DEVICE_ID_9122					0x9122
#define DEVICE_ID_914D					0x914D

/* Magni lite */
#define DEVICE_ID_9182					0x9182
#define DEVICE_ID_9172					0x9172
#define DEVICE_ID_9181					0x9181
#define DEVICE_ID_9171					0x9171
#define DEVICE_ID_9192					0x9192
#define DEVICE_ID_91A1					0x91A1

/* Magni B1 */
#define DEVICE_ID_9023					0x9023
#define DEVICE_ID_90A3					0x90A3
#define DEVICE_ID_9110					0x9110
#define DEVICE_ID_9190					0x9190
#define DEVICE_ID_9111					0x9111
#define DEVICE_ID_9191					0x9191
#define DEVICE_ID_9120					0x9120
#define DEVICE_ID_91A0					0x91A0
#define DEVICE_ID_9122					0x9122
#define DEVICE_ID_91A2					0x91A2
#define DEVICE_ID_9128					0x9128
#define DEVICE_ID_91A8					0x91A8
#define DEVICE_ID_9028					0x9028
#define DEVICE_ID_90A8					0x90A8
#define DEVICE_ID_91A3					0x91A3
#define DEVICE_ID_9130					0x9130
#define DEVICE_ID_91B0					0x91B0

/* Magni-plus*/
#define DEVICE_ID_9200					0x9200
/* Vanir */
#define DEVICE_ID_9480					0x9480
#define DEVICE_ID_9485					0x9485

/* Vanir Lite */
#define DEVICE_ID_9440					0x9440
#define DEVICE_ID_9445					0x9445

/* Frey B1*/
#define DEVICE_ID_9580                              0x9580

/* Frey Plus */
#define DEVICE_ID_9588                              0x9588

#define IS_VANIR_CORE(core)  ((core->device_id == DEVICE_ID_9580) || (core->device_id == DEVICE_ID_9588))

#define IS_VANIR_944X(core)  ((core->device_id == DEVICE_ID_9440)||(core->device_id == DEVICE_ID_9445))
#define IS_VANIR_948X(core)  ((core->device_id == DEVICE_ID_9480)||(core->device_id == DEVICE_ID_9485))

#define IS_VANIR_94X0(core)  ((core->device_id == DEVICE_ID_9480)||(core->device_id == DEVICE_ID_9440))
#define IS_VANIR_94X5(core)  ((core->device_id == DEVICE_ID_9485)||(core->device_id == DEVICE_ID_9445))

#define IS_VANIR(core)  ( IS_VANIR_944X(core)|| IS_VANIR_948X(core) || IS_VANIR_CORE(core))


/* Florence */
#define DEVICE_ID_948F					0x948F
        
#define DEVICE_ID_UNKNOWN				0xFFFF

/*for magni*/
#define MAGNI_VENDORID                "Marvell"
#define MAGNI_PRODUCTID               "9123-1b4b"
#define MAGNI_PRODUCTID_II		"91xx Config"
#endif
