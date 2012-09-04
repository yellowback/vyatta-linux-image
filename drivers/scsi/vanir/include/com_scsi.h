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
#ifndef __MV_COM_SCSI_H__
#define __MV_COM_SCSI_H__

/*
 * SCSI command
 */
#define SCSI_CMD_INQUIRY                        0x12
#define SCSI_CMD_START_STOP_UNIT                0x1B
#define SCSI_CMD_TEST_UNIT_READY                0x00
#define SCSI_CMD_RESERVE_6                      0x16
#define SCSI_CMD_RELEASE_6                      0x17

#define SCSI_CMD_READ_6                         0x08
#define SCSI_CMD_READ_10                        0x28
#define SCSI_CMD_READ_12                        0xA8
#define SCSI_CMD_READ_16                        0x88
#define SCSI_CMD_READ_LONG_10					0x3E

#define SCSI_CMD_WRITE_6                        0x0A
#define SCSI_CMD_WRITE_10                       0x2A
#define SCSI_CMD_WRITE_12                       0xAA
#define SCSI_CMD_WRITE_16                       0x8A
#define SCSI_CMD_WRITE_LONG_10					0x3F

#define SCSI_CMD_READ_CAPACITY_10               0x25
#define SCSI_CMD_READ_CAPACITY_16               0x9E    /* 9Eh/10h */
/* values for service action in */
#define SCSI_CMD_SAI_READ_CAPACITY_16  			0x10

#define SCSI_CMD_VERIFY_10                      0x2F
#define SCSI_CMD_VERIFY_12                      0xAF
#define SCSI_CMD_VERIFY_16                      0x8F

#define SCSI_CMD_REQUEST_SENSE                  0x03
#define SCSI_CMD_MODE_SENSE_6                   0x1A
#define SCSI_CMD_MODE_SENSE_10                  0x5A
#define SCSI_CMD_MODE_SELECT_6                  0x15
#define SCSI_CMD_MODE_SELECT_10                 0x55

#define SCSI_CMD_LOG_SELECT                     0x4C
#define SCSI_CMD_LOG_SENSE                      0x4D

#define SCSI_CMD_WRITE_VERIFY_10                0x2E
#define SCSI_CMD_WRITE_VERIFY_12                0xAE
#define SCSI_CMD_WRITE_VERIFY_16                0x8E
#define SCSI_CMD_SYNCHRONIZE_CACHE_10           0x35
#define SCSI_CMD_SYNCHRONIZE_CACHE_16			0x91

#define SCSI_CMD_WRITE_SAME_10                  0x41
#define SCSI_CMD_WRITE_SAME_16                  0x93

#define SCSI_CMD_XDWRITE_10                     0x50
#define SCSI_CMD_XPWRITE_10                     0x51
#define SCSI_CMD_XDREAD_10                      0x52
#define SCSI_CMD_XDWRITEREAD_10                 0x53

#define SCSI_CMD_FORMAT_UNIT                    0x04

#define SCSI_CMD_RCV_DIAG_RSLT                  0x1C
#define SCSI_CMD_SND_DIAG                       0x1D
#define SCSI_CMD_READ_DEFECT_DATA_10            0x37
#define SCSI_CMD_REASSIGN_BLOCKS                0x07
#define SCSI_CMD_ATA_PASSTHRU_12                0xA1
#define SCSI_CMD_ATA_PASSTHRU_16                0x85
#define SCSI_CMD_DEALLOCATED			0xEC
	#define ATA_CMD_PM_CHECK 		0xE5
#define IS_ATA_PASSTHRU_CMD(req,cmd)			\
		(((req->Cdb[0]==SCSI_CMD_ATA_PASSTHRU_16) &&	\
		(req->Cdb[14]==cmd))||		\
		((req->Cdb[0]==SCSI_CMD_ATA_PASSTHRU_12) &&	\
		(req->Cdb[9]==cmd)))


#define IS_VU_CMD(req,cmd)			\
		(((req->Cdb[0]== SCSI_CMD_ATA_PASSTHRU_16) &&	\
		(req->Cdb[14]== SCSI_CMD_MARVELL_VENDOR_UNIQUE)   &&   \
		(req->Cdb[4]== cmd))||		\
		((req->Cdb[0]== SCSI_CMD_ATA_PASSTHRU_12) &&	\
		(req->Cdb[9]== SCSI_CMD_MARVELL_VENDOR_UNIQUE) && \
		(req->Cdb[3]== cmd)))

/*	security commmand */
#define ATA_CMD_SEC_PASSWORD			0xF1
#define ATA_CMD_SEC_UNLOCK			0xF2
#define ATA_CMD_SEC_ERASE_PRE			0xF3
#define ATA_CMD_SEC_ERASE_UNIT			0xF4
#define ATA_CMD_SEC_FREEZE_LOCK			0xF5
#define ATA_CMD_SEC_DISABLE_PASSWORD		0xF6

#define SCSI_CMD_PERSISTENT_RESERVE_IN			0x5E
#define SCSI_CMD_PERSISTENT_RESERVE_OUT			0x5F

/* MMC */
#define SCSI_CMD_REPORT_LUN                     0xA0
#define SCSI_CMD_PREVENT_MEDIUM_REMOVAL         0x1E
#define SCSI_CMD_READ_SUB_CHANNEL               0x42
#define SCSI_CMD_READ_TOC                       0x43
#define SCSI_CMD_READ_DISC_STRUCTURE            0xAD
#define SCSI_CMD_READ_CD                        0xBE
#define SCSI_CMD_GET_EVENT_STATUS_NOTIFICATION  0x4A
#define SCSI_CMD_BLANK                          0xA1
#define SCSI_CMD_READ_DISC_INFO                 0x51

/* SCSI Read/Write Buffer Command */
#define SCSI_CMD_READ_BUFFER		0x3C
#define SCSI_CMD_WRITE_BUFFER		0x3B

#ifndef SMART_CMD
#define SMART_CMD                               0xb0
#endif /* SMART_CMD */

#define SCSI_IS_READ(cmd)                       \
           (((cmd) == SCSI_CMD_READ_6) ||       \
	    ((cmd) == SCSI_CMD_READ_10)  ||     \
            ((cmd) == SCSI_CMD_READ_12)  ||     \
	    ((cmd) == SCSI_CMD_READ_16))

#define SCSI_IS_WRITE(cmd)                      \
           (((cmd) == SCSI_CMD_WRITE_6)  ||     \
	    ((cmd) == SCSI_CMD_WRITE_10) ||     \
	    ((cmd) == SCSI_CMD_WRITE_12) ||     \
	    ((cmd) == SCSI_CMD_WRITE_16))

#define SCSI_IS_MODE_SENSE(cmd)                 \
           (((cmd) == SCSI_CMD_MODE_SENSE_6) || \
	    ((cmd) == SCSI_CMD_MODE_SENSE_10))

#define SCSI_IS_REQUEST_SENSE(cmd)              \
           (((cmd) == SCSI_CMD_REQUEST_SENSE))

#define SCSI_IS_VERIFY(cmd)                     \
           (((cmd) == SCSI_CMD_VERIFY_10) ||    \
	    ((cmd) == SCSI_CMD_VERIFY_16))

#define SCSI_IS_READ_WRITE_VERIFY(cmd)			\
		(SCSI_IS_READ(cmd) || SCSI_IS_WRITE(cmd) || SCSI_IS_VERIFY(cmd))
#define SMP_CDB_USE_ADDRESS               0x01

#define SCSI_CMD_MARVELL_SPECIFIC               0xE1
#   define CDB_CORE_MODULE                      0x1
#      define CDB_CORE_SOFT_RESET_1				0x1
#      define CDB_CORE_SOFT_RESET_0				0x2
#      define CDB_CORE_IDENTIFY                 0x3
#      define CDB_CORE_SET_UDMA_MODE            0x4
#      define CDB_CORE_SET_PIO_MODE             0x5
#      define CDB_CORE_ENABLE_WRITE_CACHE       0x6
#      define CDB_CORE_DISABLE_WRITE_CACHE      0x7
#      define CDB_CORE_ENABLE_SMART             0x8
#      define CDB_CORE_DISABLE_SMART            0x9
#      define CDB_CORE_SMART_RETURN_STATUS      0xA
#      define CDB_CORE_SHUTDOWN                 0xB
#      define CDB_CORE_ENABLE_READ_AHEAD        0xC
#      define CDB_CORE_DISABLE_READ_AHEAD       0xD
#      define CDB_CORE_READ_LOG_EXT             0xE
#      define CDB_CORE_TASK_MGMT                0xF
#      define CDB_CORE_SMP                      0x10
#      define CDB_CORE_PM_READ_REG				0x11
#      define CDB_CORE_PM_WRITE_REG				0x12
#	 define CDB_CORE_RESET_DEVICE				0x13
#	 define CDB_CORE_RESET_PORT				0x14
#      define CDB_CORE_OS_SMART_CMD				0x15

#      define   CDB_CORE_ATA_IDENTIFY_ATA                            0x16
#      define   CDB_CORE_ATA_IDENTIFY_ATAPI                         0x17
#      define   CDB_CORE_ATA_SMART_READ_VALUES                 0x18
#      define   CDB_CORE_ATA_SMART_READ_THRESHOLDS         0x19
#      define   CDB_CORE_ATA_SMART_READ_LOG_SECTOR         0x1A
#      define   CDB_CORE_ATA_SMART_WRITE_LOG_SECTOR       0x1B
#      define   CDB_CORE_ATA_SMART_AUTO_OFFLINE                0x1C
#      define   CDB_CORE_ATA_SMART_AUTOSAVE                       0x1D
#      define   CDB_CORE_ATA_SMART_IMMEDIATE_OFFLINE      0x1E

#      define  CDB_CORE_ATA_SLEEP                                 0x20
#      define  CDB_CORE_ATA_IDLE                                   0x21
#      define  CDB_CORE_ATA_STANDBY                            0x22
#      define  CDB_CORE_ATA_IDLE_IMMEDIATE               0x23
#      define  CDB_CORE_ATA_STANDBY_IMMEDIATE        0x24
#      define  CDB_CORE_ATA_CHECK_POWER_MODE         0x25

#      define CDB_CORE_SMP_VIRTUAL_DISCOVER          0x30
#      define CDB_CORE_SMP_VIRTUAL_CONFIG_ROUTE      0x31
#	define CDB_CORE_STP_VIRTUAL_PHY_RESET			0x32
#	define CDB_CORE_STP_VIRTUAL_REPORT_SATA_PHY		0x33
#	define CDB_CORE_SSP_VIRTUAL_PHY_RESET			0x34
#	define CDB_CORE_SSP_VIRTUAL_REPORT_SATA_PHY		0x35
#	define CDB_CORE_SMP_VIRTUAL_RESET_SATA_PHY		0x36
#	define CDB_CORE_SMP_VIRTUAL_CLEAR_AFFILIATION_ALL	0x37

#	define CDB_CORE_SET_FEATURE_SPINUP	0x40

#define SCSI_CMD_MARVELL_VENDOR_UNIQUE         	0xFF
#   define MARVELL_VU_CMD_CMD_PACKET_PHASE	0xFC
#   define MARVELL_VU_CMD_DATA_TX_PHASE		0xFD
#   define MARVELL_VU_CMD_ASYNC_NOTIFY          0xFE


/* ANSI SCSI-3 Log Pages retrieved by LOG SENSE. */
#define SUPPORTED_LPAGES                            0x00
#define BUFFER_OVERRUN_LPAGE                        0x01
#define WRITE_ERROR_COUNTER_LPAGE                   0x02
#define READ_ERROR_COUNTER_LPAGE                    0x03
#define READ_REVERSE_ERROR_COUNTER_LPAGE            0x04
#define VERIFY_ERROR_COUNTER_LPAGE                  0x05
#define NON_MEDIUM_ERROR_LPAGE                      0x06
#define LAST_N_ERROR_LPAGE                          0x07
#define FORMAT_STATUS_LPAGE                         0x08
#define TEMPERATURE_LPAGE                           0x0d
#define STARTSTOP_CYCLE_COUNTER_LPAGE               0x0e
#define APPLICATION_CLIENT_LPAGE                    0x0f
#define SELFTEST_RESULTS_LPAGE                      0x10
#define BACKGROUND_RESULTS_LPAGE                    0x15   /* SBC-3 */
#define IE_LPAGE                                    0x2f

/* Seagate vendor specific log pages. */
#define SEAGATE_CACHE_LPAGE                     0x37
#define SEAGATE_FACTORY_LPAGE                 0x3e

#define SCSI_IS_INTERNAL(cmd)        ((cmd) == SCSI_CMD_MARVELL_SPECIFIC)

/*
 * SCSI status
 */
#define SCSI_STATUS_GOOD                        0x00
#define SCSI_STATUS_CHECK_CONDITION             0x02
#define SCSI_STATUS_CONDITION_MET               0x04
#define SCSI_STATUS_BUSY                        0x08
#define SCSI_STATUS_INTERMEDIATE                0x10
#define SCSI_STATUS_INTERMEDIATE_MET            0x14
#define SCSI_STATUS_RESERVATION_CONFLICT        0x18
#define SCSI_STATUS_FULL                        0x28
#define SCSI_STATUS_ACA_ACTIVE                  0x30
#define SCSI_STATUS_ABORTED                     0x40

/*
 * SCSI sense key
 */
#define SCSI_SK_NO_SENSE                        0x00
#define SCSI_SK_RECOVERED_ERROR                 0x01
#define SCSI_SK_NOT_READY                       0x02
#define SCSI_SK_MEDIUM_ERROR                    0x03
#define SCSI_SK_HARDWARE_ERROR                  0x04
#define SCSI_SK_ILLEGAL_REQUEST                 0x05
#define SCSI_SK_UNIT_ATTENTION                  0x06
#define SCSI_SK_DATA_PROTECT                    0x07 
#define SCSI_SK_BLANK_CHECK                     0x08
#define SCSI_SK_VENDOR_SPECIFIC                 0x09
#define SCSI_SK_COPY_ABORTED                    0x0A
#define SCSI_SK_ABORTED_COMMAND                 0x0B
#define SCSI_SK_VOLUME_OVERFLOW                 0x0D
#define SCSI_SK_MISCOMPARE                      0x0E
/*
 * SCSI additional sense code
 */
#define SCSI_ASC_NO_ASC                         0x00
#define SCSI_ASC_LUN_NOT_READY                  0x04
#define SCSI_ASC_LOGICAL_UNIT_NOT_RESP_TO_SEL	0x05
#define SCSI_ASC_ECC_ERROR                      0x10
#define SCSI_ASC_UNRECOVERED_READ_ERROR         0x11
#define SCSI_ASC_ID_ADDR_MARK_NOT_FOUND         0x12
#define SCSI_ASC_RECORD_NOT_FOUND               0x14
#define SCSI_ASC_PARAMETER_LIST_LENGTH_ERROR	0x1A
#define SCSI_ASC_INVALID_OPCODE                 0x20
#define SCSI_ASC_LBA_OUT_OF_RANGE               0x21
#define SCSI_ASC_INVALID_FEILD_IN_CDB           0x24
#define SCSI_ASC_LOGICAL_UNIT_NOT_SUPPORTED     0x25
#define SCSI_ASC_INVALID_FIELD_IN_PARAMETER     0x26
#define SCSI_ASC_WRITE_PROTECTED                0x27
#define SCSI_ASC_MEDIA_CHANGED                  0x28

#define SCSI_ASC_CMD_SEQUENCE_ERROR             0x2C
#define SCSI_ASC_SAVING_PARAMETERS_NOT_SUPPORT  0x39
#define SCSI_ASC_MEDIUM_NOT_PRESENT             0x3A
#define SCSI_ASC_LOGICAL_UNIT_FAILURE           0x3E
#define SCSI_ASC_INTERNAL_TARGET_FAILURE        0x44
#define SCSI_ASC_SCSI_PARITY_ERROR              0x47
#define SCSI_ASC_MISCOMPARE_DURING_VERIFY 			 0x1D
#define SCSI_ASC_MEDIUM_FORMAT_CORRUPTED 	     0x31
#define SCSI_ASC_MEDIA_LOAD_EJECT_FAILURE       0x53
#define SCSI_ASC_SYSTEM_RESOURCE_FAILURE        0x55
#define SCSI_ASC_OPERATOR_MEDIUM_REMOVAL_REQUEST 0x5A
#define SCSI_ASC_FAILURE_PREDICTION_THRESHOLD_EXCEEDED	0x5D
#define SCSI_ASC_CONFIGURATION_FAILURE          0x67

#define SCSI_ADSENSE_NO_SENSE  0x98 
#define SCSI_ADSENSE_INVALID_CDB 0x99

#define  SCSI_CMD_READ_DEFECT_DATA_10        0x37
/*
 * SCSI additional sense code qualifier
 */
#define SCSI_ASCQ_NO_ASCQ                       0x00
#define SCSI_ASCQ_FORMAT_FAILED				0x01
#define SCSI_ASCQ_INSUFFICIENT_RESERVATION_RESOURCES 0x02
#define SCSI_ASCQ_INTERVENTION_REQUIRED         0x03
#define SCSI_ASCQ_FORMAT_IN_PROGRESS            0x04
#define SCSI_ASCQ_MAINTENANCE_IN_PROGRESS       0x80
#define SCSI_ASCQ_HIF_GENERAL_HD_FAILURE		0x10
#define SCSI_ASCQ_ATA_PASSTHRU_INFO             0x1D
#define SCSI_ASCQ_INSUFFICIENT_RESERVATION_RESOURCES 0x02

/* SCSI command CDB helper functions. */
#define SCSI_CDB10_GET_LBA(cdb)                  \
           ((MV_U32) (((MV_U32) cdb[2] << 24) |  \
		      ((MV_U32) cdb[3] << 16) |  \
		      ((MV_U32) cdb[4] << 8)  |  \
		      (MV_U32) cdb[5]))

#define SCSI_CDB10_SET_LBA(cdb, lba)             \
           {                                     \
              cdb[2] = (MV_U8)(lba >> 24);       \
              cdb[3] = (MV_U8)(lba >> 16);       \
              cdb[4] = (MV_U8)(lba >> 8);        \
              cdb[5] = (MV_U8)lba;               \
           }

#define SCSI_CDB16_SET_LBA(cdb, lba)             \
           {                                     \
              cdb[2] = (MV_U8)(lba >> 56);       \
              cdb[3] = (MV_U8)(lba >> 48);       \
              cdb[4] = (MV_U8)(lba >> 40);       \
              cdb[5] = (MV_U8)(lba >> 32);       \
			  cdb[6] = (MV_U8)(lba >> 24);       \
			  cdb[7] = (MV_U8)(lba >> 16);       \
			  cdb[8] = (MV_U8)(lba >> 8);        \
			  cdb[9] = (MV_U8)lba;               \
           }

#define SCSI_CDB10_GET_SECTOR(cdb)    ((cdb[7] << 8) | cdb[8])

#define SCSI_CDB10_SET_SECTOR(cdb, sector)      \
           {                                    \
              cdb[7] = (MV_U8)(sector >> 8);    \
              cdb[8] = (MV_U8)sector;           \
           }

#define SCSI_CDB16_SET_SECTOR(cdb, sector)      \
           {                                    \
              cdb[10] = (MV_U8)(sector >> 24);  \
			  cdb[11] = (MV_U8)(sector >> 16);  \
			  cdb[12] = (MV_U8)(sector >> 8);   \
			  cdb[13] = (MV_U8)sector;          \
           }

#define MV_SCSI_RESPONSE_CODE                   0x70
#define MV_SCSI_DIRECT_ACCESS_DEVICE            0x00

typedef struct _MV_Sense_Data
{
	MV_U8 ErrorCode:7;
	MV_U8 Valid:1;
	MV_U8 SegmentNumber;
	MV_U8 SenseKey:4;
	MV_U8 Reserved:1;
	MV_U8 IncorrectLength:1;
	MV_U8 EndOfMedia:1;
	MV_U8 FileMark:1;
	MV_U8 Information[4];
	MV_U8 AdditionalSenseLength;
	MV_U8 CommandSpecificInformation[4];
	MV_U8 AdditionalSenseCode;
	MV_U8 AdditionalSenseCodeQualifier;
	MV_U8 FieldReplaceableUnitCode;
	MV_U8 SenseKeySpecific[3];
}MV_Sense_Data, *PMV_Sense_Data;

MV_VOID MV_SetSenseData(
	IN PMV_Sense_Data pSense,
	IN MV_U8 SenseKey,
	IN MV_U8 AdditionalSenseCode,
	IN MV_U8 ASCQ
	);

/* Virtual Device Inquiry Related */
#define VIRTUALD_INQUIRY_DATA_SIZE		36
#define VPD_PAGE0_VIRTUALD_SIZE			7
#define VPD_PAGE80_VIRTUALD_SIZE		12
#define VPD_PAGE83_VIRTUALD_SIZE		24

extern MV_U8 BASEATTR MV_INQUIRY_VIRTUALD_DATA[];
extern MV_U8 BASEATTR MV_INQUIRY_VPD_PAGE0_VIRTUALD_DATA[];
extern MV_U8 BASEATTR MV_INQUIRY_VPD_PAGE80_VIRTUALD_DATA[];
extern MV_U8 BASEATTR MV_INQUIRY_VPD_PAGE83_VIRTUALD_DATA[];

enum log_sense_page_code {
        SUPPORTED_LOG_PAGES_LOG_PAGE            = 0x00,
        WRITE_ERROR_COUNTER_LOG_PAGE            = 0x02,
        READ_ERROR_COUNTER_LOG_PAGE             = 0x03,
        READ_REVERSE_ERROR_COUNTER_LOG_PAGE     = 0x04,
        VERIFY_ERROR_COUNTER_LOG_PAGE           = 0x05,
        TEMPERATURE_LOG_PAGE                    = 0x0D,
        SELF_TEST_RESULTS_LOG_PAGE              = 0x10, 
        INFORMATIONAL_EXCEPTIONS_LOG_PAGE       = 0x2F,
};

enum mode_sense_page_code {
        DIRECT_ACCESS_BLOCK_DEVICE_MODE_PAGE    = 0x00,
        RW_ERROR_RECOVERY_MODE_PAGE             = 0x01,
        CACHE_MODE_PAGE                         = 0x08,
        CONTROL_MODE_PAGE                       = 0x0A,
        PORT_MODE_PAGE                          = 0x19,
        INFORMATIONAL_EXCEPTIONS_CONTROL_MODE_PAGE = 0x1C,
        ALL_MODE_PAGE                           = 0x3F,
};

#endif /*  __MV_COM_SCSI_H__ */
