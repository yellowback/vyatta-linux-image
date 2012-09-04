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
#if !defined(COM_NVRAM_H)
#define COM_NVRAM_H
typedef union {
    struct {
        MV_U32 low;
        MV_U32 high;
    } parts;
    MV_U8       b[8];
    MV_U16      w[4];
    MV_U32      d[2];        
} SAS_ADDR, *PSAS_ADDR;

/* Generate  PHY tunning parameters */
typedef struct _PHY_TUNING {
	MV_U8	Trans_Emphasis_En:1;			/* 1 bit,  transmitter emphasis enable  */
	MV_U8	Trans_Emphasis_Amp:4;		    	/* 4 bits, transmitter emphasis amplitude */
	MV_U8	Reserved_2bit_1:3;				/* 3 bits, reserved space */
         MV_U8	Trans_Amp:5;					/* 5 bits, transmitter amplitude */
	MV_U8	Trans_Amp_Adjust:2;			/* 2 bits, transmitter amplitude adjust */
	MV_U8	Reserved_2bit_2:1;				/* 1 bit, 	reserved space */
	MV_U8	Reserved[2];						/* 2 bytes, reserved space */
} PHY_TUNING, *PPHY_TUNING;

typedef struct _FFE_CONTROL {
	MV_U8	FFE_Capacitor_Select:4;			/* 4 bits,  FFE Capacitor Select  (value range 0~F)  */
	MV_U8	FFE_Resistor_Select	:3;		    /* 3 bits,  FFE Resistor Select (value range 0~7) */
   	MV_U8	Reserved			:1;			/* 1 bit reserve*/
} FFE_CONTROL, *pFFE_CONTROL;

typedef struct _Mv_Phy_Status{
	MV_U32	phy_id;
	MV_U32	subtractive;
	MV_U8	sas_address[8];
	MV_U32	device_type;	//sata or sas
	MV_U32	link_rate;
}Mv_Phy_Status, *pMv_Phy_Status;


/* HBA_FLAG_XX */
#define HBA_FLAG_INT13_ENABLE				MV_BIT(0)	//int 13h enable/disable
#define HBA_FLAG_SILENT_MODE_ENABLE			MV_BIT(1)	//silent mode enable/disable
#define HBA_FLAG_ERROR_STOP					MV_BIT(2)	//if error then stop
#define HBA_FLAG_OPTIMIZE_CPU_EFFICIENCY	MV_BIT(3)	//if 1, enable interrupt coalescing, optimize CPU efficiency
#define HBA_FLAG_AUTO_REBUILD_ON            MV_BIT(4) // auto rebuild on/off                                                   //XIN
#define HBA_FLAG_SMART_ON					MV_BIT(6) // smart on/off
#define HBA_FLAG_DISABLE_MOD_CONSOLIDATE	MV_BIT(7)	//if 1, disable module consolidation
#define HBA_FLAG_ENABLE_BUZZER			MV_BIT(8)
#define HBA_FLAG_SERIAL_CONSOLE_ENABLE			MV_BIT(9) //Bios enable or disable console redirection feature

#define NVRAM_DATA_MAJOR_VERSION		0
#define NVRAM_DATA_MINOR_VERSION		1

/* for RAID_Feature */
#define RAID_FEATURE_DISABLE_RAID5			MV_BIT(0)
#define RAID_FEATURE_ENABLE_RAID			MV_BIT(1)

/* 
	HBA_Info_Page is saved in Flash/NVRAM, total 256 bytes.
	The data area is valid only Signature="MRVL".
	If any member fills with 0xFF, the member is invalid.
*/
typedef struct _HBA_Info_Page{
	// Dword 0
	MV_U8     	Signature[4];                 	/* 4 bytes, structure signature,should be "MRVL" at first initial */

	// Dword 1
	MV_U8     	MinorRev;                 		/* 2 bytes, NVRAM data structure version */
	MV_U8		MajorRev;
	MV_U16    	Next_Page;					  	/* 2 bytes, For future data structure expansion, 0 is for NULL pointer and current page is the last page. */

	// Dword 2
	MV_U8     	Major;                   		/* 1 byte,  BIOS major version */
	MV_U8     	Minor;                  		/* 1 byte,	BIOS minor version */
	MV_U8     	OEM_Num;                     	/* 1 byte,  OEM number */
	MV_U8     	Build_Num;                    	/* 1 byte,  Build number */

	// Dword 3
	MV_U8     	Page_Code;					  	/* 1 byte,  eg. 0 for the 1st page  */
	MV_U8     	Max_PHY_Num;				  	/* 1 byte,   maximum PHY number */
	MV_U8		RAID_Feature;					/* 1 byte,  RAID settings (see RAID_FEATURE_XX)
															bit 0 - disable RAID 5 (default 1 = disabled)
												*/
	MV_U8		Reserved2;

	// Dword 4
	MV_U32     	HBA_Flag;                     	/* 4 bytes, should be 0x0000,0000 at first initial
													HBA flag:  refers to HBA_FLAG_XX
												*/
	// Dword 5	                                              
	MV_U32     	Boot_Device;					/* 4 bytes, select boot device */
												/* for ata device, it is CRC of the serial number + model number. */
												/* for sas device, it is CRC of sas address */
												/* for VD, it is VD GUI */

	// Dword 6-8	
	MV_U8		DSpinUpGroup;				/* spin up group */
	MV_U8		DSpinUpTimer;				/* spin up timer */
	MV_U8		Delay_Time;					/* delay time, default value = 5 second */
       MV_U8           bbu_charge_threshold;
       MV_U8           bbu_temp_lowerbound;
       MV_U8           bbu_temp_upperbound;
       MV_U16          bbu_volt_lowerbound;
       MV_U16          bbu_volt_upperbound;
	MV_U8		Reserved3[2];				  	/* 4 bytes, reserved	*/

	// Dword 9-13
	MV_U8     	Serial_Num[20];				  	/* 20 bytes, controller serial number */

	// Dword 14-29
	SAS_ADDR	SAS_Address[8];               /* 64 bytes, SAS address for each port */

	// Dword 30-31
	FFE_CONTROL  FFE_Control[8];			/* 8 bytes for vanir 8 port PHY FFE seeting
										BIT 0~3 : FFE Capacitor select(value range 0~F)
										BIT 4~6 : FFE Resistor select(value range 0~7)
										BIT 7: reserve. */
	//Dword 32 -33
	MV_U8		Product_ID[8];			/* 8 bytes for vanir bios to differentiate VA6800m HBA and VA6400m HBA */

	//Dword 34 -38
	MV_U8		Reserved4[20];			/* 20 bytes, reserve space for future,initial as 0xFF */

 	//Dword 39 -43
	MV_U8		  model_number[20];		 /* 20 bytes, Florence model name */
 	
	// Dword 44-45
	MV_U8     	PHY_Rate[8];                  	/* 8 bytes,  0:  1.5G, 1: 3.0G, should be 0x01 at first initial */

	// Dword 46-53
	PHY_TUNING   PHY_Tuning[8];				/* 32 bytes, PHY tuning parameters for each PHY*/

	// Dword 54-62
	MV_U32     	Reserved5[7];                 	/* 9 dword, reserve space for future,initial as 0xFF */
	MV_U8  		BGA_Rate;
	MV_U8  		Sync_Rate;
	MV_U8 		Init_Rate;
	MV_U8  		Rebuild_Rate;
	MV_U8  		Migration_Rate;
	MV_U8  		Copyback_Rate;
	MV_U8  		MP_Rate;
	MV_U8     	Reserved7; 
	// Dword 63
	MV_U8     	Reserved6[3];                 	/* 3 bytes, reserve space for future,initial as 0xFF */
	MV_U8     	Check_Sum;                    	/* 1 byte,   checksum for this structure,Satisfy sum of every 8-bit value of this structure */
}HBA_Info_Page, *pHBA_Info_Page;			/* total 256 bytes */

#define FLASH_PARAM_SIZE 	(sizeof(HBA_Info_Page))
#define ODIN_FLASH_SIZE		0x40000  				//.256k bytes
#define PARAM_OFFSET		ODIN_FLASH_SIZE - 0x100 //.255k bytes

// PD infomation Page
#define FLASH_PD_INFO_PAGE_SIZE 	(sizeof(pd_info_page))
#define PAGE_INTERVAL_DISTANCE		0x100

#define PD_PAGE_PD_ID_INAVAILABLE 	0
#define PD_PAGE_PD_ID_AVAILABLE 	1

#define IS_PD_PAGE_PD_ID_AVAILABLE(flag) ((flag)==PD_PAGE_PD_ID_AVAILABLE)

#define HBA_INFO_PAGE_CODE 0x00
#define PD_INFO_PAGE_CODE 0x01

#define PD_PAGE_PD_STATE_OFFLINE 0
#define PD_PAGE_PD_STATE_ONLINE 1

#define MAX_NUM_PD_PAGE	1

#define PAGE_BUFFER_SIZE  (FLASH_PD_INFO_PAGE_SIZE+PAGE_INTERVAL_DISTANCE+FLASH_PARAM_SIZE)// total 0x1240 bytes(128 PDs); 0x2240 bytes(256 PDs)

/* This number may not equal to MAX_DEVICE_SUPPORTED_PERFORMANCE */
#define MAX_PD_IN_PD_PAGE_FLASH	128 // currently every PD page support 128 PD entry
#define MAX_PD_IN_TOTAL_PD_PAGE		(MAX_PD_IN_PD_PAGE_FLASH * MAX_NUM_PD_PAGE)
/* 
	PD_Info_Page is saved in Flash/NVRAM, currently total size is 0x1040 bytes.
	The data area is valid only Signature="MRVL".
	If any member fills with 0xFF, the member is invalid.
*/

typedef struct _Device_Index {
	MV_U16	index;
	MV_U16	device_id;
	MV_BOOLEAN end;
}Device_Index,*PDevice_Index;

typedef void(*get_device_id) (MV_PVOID, MV_U16,PDevice_Index);

typedef struct _pd_status_info {
	MV_U16	pd_id;
	MV_U8	status;
	MV_U8	reserved;
}pd_status_info,*p_pd_status_info;

struct _pd_info {
	// Dword 0-1
	MV_U64	pd_guid;

	// Dword 2
	MV_U16	pd_id;
	MV_U16	pd_scsi_id;

	// Dword 3
	MV_U8	status;
	MV_U8	type;
	MV_U8	cache_policy;
	MV_U8	reserved1;
	
	// Dword 4-7
	MV_U8	reserved2[16];
};

typedef struct _page_header{
	// Dword 0
	MV_U8	signature[4];		/* 4 bytes, structure signature,should be "MRVL" at first initial */
	// Dword 1
	MV_U8	page_code;		/* 1 byte, page code , here should be 0x01  for PD info Page */
	MV_U8	check_sum;		/* 1 byte,   checksum for this structure,Satisfy sum of every 8-bit value of this structure */
	MV_U16	data_length; 		/* 2 byte, data length, indicates the size of invalid data besides page header */
	// Dword 2
	MV_U16	next_page;		/* 2 byte, next page  */
	MV_U8	reserved[2];		/* 2 byte,   checksum for this structure,Satisfy sum of every 8-bit value of this structure */
}page_header, *p_page_header;

struct _pd_info_page{

	page_header header;

	MV_U8	reserved2[52];		/* 52 byte,   reserve space for future,initial as 0xFF */

	struct _pd_info pd_info_data[MAX_PD_IN_PD_PAGE_FLASH]; /* MAX_PD_IN_PD_PAGE_FLASH*32 bytes, could save 128 PD data in flash */
};

typedef struct _pd_info pd_info, *p_pd_info;
typedef struct _pd_info_page pd_info_page, *p_pd_info_page;
 
extern MV_BOOLEAN mv_page_signature_check( MV_PU8 Signature );

MV_BOOLEAN mv_nvram_init_param( MV_PVOID  This, pHBA_Info_Page pHBA_Info_Param );
MV_BOOLEAN mv_nvram_moddesc_init_param( MV_PVOID This, pHBA_Info_Page pHBA_Info_Param);

/* Caution: Calling this function, please do Read-Modify-Write. 
 * Please call to get the original data first, then modify corresponding field,
 * Then you can call this function. */
extern MV_BOOLEAN mvuiHBA_modify_param( MV_PVOID This, pHBA_Info_Page pHBA_Info_Param);
extern MV_U8	mvCalculateChecksum(MV_PU8	Address, MV_U32 Size);
extern MV_U8	mvVerifyChecksum(MV_PU8	Address, MV_U32 Size);
#endif		/* COM_NVRAM_H */
