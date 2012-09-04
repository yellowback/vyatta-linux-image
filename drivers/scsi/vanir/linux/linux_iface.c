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
#include <linux/hdreg.h>
#include "linux_main.h"
#include "linux_iface.h"
#include "hba_mod.h"
#include <scsi/scsi_eh.h>

#define SECTOR_SIZE 512
#define HBA_REQ_TIMER_IOCTL (15)

#define MV_DEVFS_NAME "mv"
#define IOCTL_BUF_LEN (1024*1024)

void ioctlcallback(MV_PVOID This, PMV_Request req)
{
	struct hba_extension *hba = (struct hba_extension *) This;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_set(&hba->desc->hba_desc->hba_ioctl_sync, 0);
#else
	complete(&hba->desc->hba_desc->ioctl_cmpl);
#endif

	hba_req_cache_free(hba,req);
}

int mv_linux_proc_info(struct Scsi_Host *pSHost, char *pBuffer,
		       char **ppStart,off_t offset, int length, int inout)
{
	int len = 0;
	int datalen = 0;
	if (!pSHost || !pBuffer)
		return (-ENOSYS);

	if (inout == 1) {
		return (-ENOSYS);
	}

	len = sprintf(pBuffer,"Marvell %s Driver , Version %s\n",
		      mv_product_name, mv_version_linux);

	datalen = len - offset;
	if (datalen < 0) {
		datalen = 0;
		*ppStart = pBuffer + len;
	} else {
		*ppStart = pBuffer + offset;
	}
	return datalen;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
typedef struct mutex  mutex_t;
#else
typedef struct semaphore  mutex_t;
#define mutex_init(lock)  sema_init(lock, 1)
#define mutex_lock(lock)  down(lock)
#define mutex_unlock(lock)  up(lock)
#endif

static mutex_t  ioctl_index_mutex;

void * kbuf_array[512] = {NULL,};
unsigned char mvcdb[512][16];
unsigned long kbuf_index[512/8] = {0,};

static inline int mv_is_api_cmd(int cmd)
{
	return (cmd >= API_BLOCK_IOCTL_DEFAULT_FUN) && \
		(cmd < API_BLOCK_IOCTL_DEFAULT_FUN + API_IOCTL_MAX );
}

/* ATA_16(0x85) SAT command and 0xec for Identify */
char ata_ident[] = {0x85, 0x08, 0x0e, 0x00, 0x00, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xec, 0x00};

/**
 *	id_to_string - Convert IDENTIFY DEVICE page into string
 *	@iden: IDENTIFY DEVICE results we will examine
 *	@s: string into which data is output
 *	@ofs: offset into identify device page
 *	@len: length of string to return. must be an even number.
 *
 *	The strings in the IDENTIFY DEVICE page are broken up into
 *	16-bit chunks.  Run through the string, and output each
 *	8-bit chunk linearly, regardless of platform.
 *
 */
static void id_to_string(const u16 *iden, unsigned char *s,
		   unsigned int ofs, unsigned int len)
{
	unsigned int c;
	while (len > 0) {
		c = iden[ofs] >> 8;
		*s = c;
		s++;

		c = iden[ofs] & 0xff;
		*s = c;
		s++;

		ofs++;
		len -= 2;
	}
}
static int io_get_identity(void *kbuf)
{
	char buf[40];
	u16 *dst = kbuf;
	unsigned int i,j;
	
	if (!dst)
		return -1;

#ifdef __MV_BIG_ENDIAN_BITFIELD__
        for (i = 0; i < 256; i++)
                dst[i] = MV_LE16_TO_CPU(dst[i]);
#endif

	/*identify data -> model number: word 27-46*/
	id_to_string(dst, buf, 27, 40);
	memcpy(&dst[27], buf, 40);

	/*identify data -> firmware revision: word 23-26*/
	id_to_string(dst, buf, 23, 8);
	memcpy(&dst[23], buf, 8);
	
	/*identify data -> serial number: word 10-19*/
	id_to_string(dst, buf, 10, 20);
	memcpy(&dst[10], buf, 20);

	return 0;
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 12)

/**
 * scsi_normalize_sense - normalize main elements from either fixed or
 *			descriptor sense data format into a common format.
 *
 * @sense_buffer:	byte array containing sense data returned by device
 * @sb_len:		number of valid bytes in sense_buffer
 * @sshdr:		pointer to instance of structure that common
 *			elements are written to.
 *
 * Notes:
 *	The "main elements" from sense data are: response_code, sense_key,
 *	asc, ascq and additional_length (only for descriptor format).
 *
 *	Typically this function can be called after a device has
 *	responded to a SCSI command with the CHECK_CONDITION status.
 *
 * Return value:
 *	1 if valid sense data information found, else 0;
 **/
int scsi_normalize_sense(const u8 *sense_buffer, int sb_len,
                         struct scsi_sense_hdr *sshdr)
{
	if (!sense_buffer || !sb_len)
		return 0;

	memset(sshdr, 0, sizeof(struct scsi_sense_hdr));

	sshdr->response_code = (sense_buffer[0] & 0x7f);

	if (!scsi_sense_valid(sshdr))
		return 0;

	if (sshdr->response_code >= 0x72) {
		/*
		 * descriptor format
		 */
		if (sb_len > 1)
			sshdr->sense_key = (sense_buffer[1] & 0xf);
		if (sb_len > 2)
			sshdr->asc = sense_buffer[2];
		if (sb_len > 3)
			sshdr->ascq = sense_buffer[3];
		if (sb_len > 7)
			sshdr->additional_length = sense_buffer[7];
	} else {
		/* 
		 * fixed format
		 */
		if (sb_len > 2)
			sshdr->sense_key = (sense_buffer[2] & 0xf);
		if (sb_len > 7) {
			sb_len = (sb_len < (sense_buffer[7] + 8)) ?
					 sb_len : (sense_buffer[7] + 8);
			if (sb_len > 12)
				sshdr->asc = sense_buffer[12];
			if (sb_len > 13)
				sshdr->ascq = sense_buffer[13];
		}
	}

	return 1;
}
#endif	
/**
 *	mv_ata_task_ioctl - Handler for HDIO_DRIVE_TASK ioctl
 *	@scsidev: Device to which we are issuing command
 *	@arg: User provided data for issuing command
 *
 *	LOCKING:
 *	Defined by the SCSI layer.  We don't really care.
 *
 *	RETURNS:
 *	Zero on success, negative errno on error.
 */
int mv_ata_task_ioctl(struct scsi_device *scsidev, void __user *arg)
{
	int rc = 0;
	u8 scsi_cmd[MAX_COMMAND_SIZE];
	u8 args[7];
	struct scsi_sense_hdr sshdr;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	struct scsi_request *sreq;
#endif
	if (arg == NULL)
		return -EINVAL;

	if (copy_from_user(args, arg, sizeof(args)))
		return -EFAULT;

	memset(scsi_cmd, 0, sizeof(scsi_cmd));
	scsi_cmd[0]  = ATA_16;
	scsi_cmd[1]  = (3 << 1); /* Non-data */
	scsi_cmd[4]  = args[1];
	scsi_cmd[6]  = args[2];
	scsi_cmd[8]  = args[3];
	scsi_cmd[10] = args[4];
	scsi_cmd[12] = args[5];
	scsi_cmd[14] = args[0];
	#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	sreq = scsi_allocate_request(scsidev, GFP_KERNEL);
	if (!sreq) {
		rc= -EINTR;
		scsi_release_request(sreq);
		return rc;
	}
	sreq->sr_data_direction = DMA_NONE;
	scsi_wait_req(sreq, scsi_cmd, NULL, 0, (10*HZ), 5);

	/* 
	 * If there was an error condition, pass the info back to the user. 
	 */
	rc = sreq->sr_result;
#elif LINUX_VERSION_CODE >=KERNEL_VERSION(2, 6, 29)
	if (scsi_execute_req(scsidev, scsi_cmd, DMA_NONE, NULL, 0, &sshdr,
				     (10*HZ), 5,0))
			rc = -EIO;


#else
	if (scsi_execute_req(scsidev, scsi_cmd, DMA_NONE, NULL, 0, &sshdr,
			     (10*HZ), 5))
		rc = -EIO;
#endif

	return rc;
}
/****************************************************************
*  Name:   mv_ial_ht_ata_cmd
*
*  Description:    handles mv_sata ata IOCTL special drive command (HDIO_DRIVE_CMD)
*
*  Parameters:     scsidev - Device to which we are issuing command
*                  arg     - User provided data for issuing command
*
*  Returns:        0 on success, otherwise of failure.
*
****************************************************************/
static int mv_ial_ht_ata_cmd(struct scsi_device *scsidev, void __user *arg)
{
	int rc = 0;
     	u8 scsi_cmd[MAX_COMMAND_SIZE];
      	u8 args[4] , *argbuf = NULL, *sensebuf = NULL;
      	int argsize = 0;
      	enum dma_data_direction data_dir;
      	int cmd_result;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	struct scsi_request *sreq;
#endif
      	if (arg == NULL)
          	return -EINVAL;
  
      	if (copy_from_user(args, arg, sizeof(args)))
          	return -EFAULT;
      	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	sensebuf = kmalloc(SCSI_SENSE_BUFFERSIZE, GFP_NOIO);
	if (sensebuf) {
		memset(sensebuf, 0, SCSI_SENSE_BUFFERSIZE);
	}
#else
	sensebuf = kzalloc(SCSI_SENSE_BUFFERSIZE, GFP_NOIO);
#endif 

      	if (!sensebuf)
          	return -ENOMEM;
  
      	memset(scsi_cmd, 0, sizeof(scsi_cmd));
      	if (args[3]) {
          	argsize = SECTOR_SIZE * args[3];
          	argbuf = kmalloc(argsize, GFP_KERNEL);
          	if (argbuf == NULL) {
              		rc = -ENOMEM;
              		goto error;
     	}
  
     	scsi_cmd[1]  = (4 << 1); /* PIO Data-in */
     	scsi_cmd[2]  = 0x0e;
      	data_dir = DMA_FROM_DEVICE;
      	} else {
       		scsi_cmd[1]  = (3 << 1); /* Non-data */
       		scsi_cmd[2]  = 0x20;
      		data_dir = DMA_NONE;
	}
  
	scsi_cmd[0] = ATA_16;
  
   	scsi_cmd[4] = args[2];
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
    	if (args[0] == WIN_SMART) {
#else
	if (args[0] == ATA_CMD_SMART) {
#endif	
        	scsi_cmd[6]  = args[3];
       		scsi_cmd[8]  = args[1];
     		scsi_cmd[10] = 0x4f;
    		scsi_cmd[12] = 0xc2;
      	} else {
          	scsi_cmd[6]  = args[1];
      	}
      	scsi_cmd[14] = args[0];
      	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	sreq = scsi_allocate_request(scsidev, GFP_KERNEL);
	if (!sreq) {
		rc= -EINTR;
		goto free_req;
	}
	sreq->sr_data_direction = data_dir;
	scsi_wait_req(sreq, scsi_cmd, argbuf, argsize, (10*HZ), 5);

	/* 
	 * If there was an error condition, pass the info back to the user. 
	 */
	cmd_result = sreq->sr_result;
	sensebuf = sreq->sr_sense_buffer;
   
#elif LINUX_VERSION_CODE >=KERNEL_VERSION(2, 6, 29)
      	cmd_result = scsi_execute(scsidev, scsi_cmd, data_dir, argbuf, argsize,
                                sensebuf, (10*HZ), 5, 0,0);
#else
      	cmd_result = scsi_execute(scsidev, scsi_cmd, data_dir, argbuf, argsize,
                                sensebuf, (10*HZ), 5, 0);
#endif

  
      	if (driver_byte(cmd_result) == DRIVER_SENSE) {/* sense data available */
         	u8 *desc = sensebuf + 8;
          	cmd_result &= ~(0xFF<<24);
          	if (cmd_result & SAM_STAT_CHECK_CONDITION) {
              	struct scsi_sense_hdr sshdr;
              	scsi_normalize_sense(sensebuf, SCSI_SENSE_BUFFERSIZE,
                                   &sshdr);
              	if (sshdr.sense_key==0 &&
                  	sshdr.asc==0 && sshdr.ascq==0)
                  	cmd_result &= ~SAM_STAT_CHECK_CONDITION;
          	}
  
          	/* Send userspace a few ATA registers (same as drivers/ide) */
          	if (sensebuf[0] == 0x72 &&     /* format is "descriptor" */
              		desc[0] == 0x09 ) {        /* code is "ATA Descriptor" */
              		args[0] = desc[13];    /* status */
              		args[1] = desc[3];     /* error */
              		args[2] = desc[5];     /* sector count (0:7) */
              		if (copy_to_user(arg, args, sizeof(args)))
                  		rc = -EFAULT;
          	}
      	}
  
      	if (cmd_result) {
          	rc = -EIO;
          	goto free_req;
      	}
  
      	if ((argbuf) && copy_to_user(arg + sizeof(args), argbuf, argsize))
          	rc = -EFAULT;
      	
free_req:
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	scsi_release_request(sreq);
#endif		
error:
      	if (sensebuf) kfree(sensebuf);
      	if (argbuf) kfree(argbuf);
      	return rc;
}

static int check_dma (__u8 ata_op)
{
	switch (ata_op) {
		case ATA_CMD_READ_DMA_EXT:
		case ATA_CMD_READ_FPDMA_QUEUED:
		case ATA_CMD_WRITE_DMA_EXT:
		case ATA_CMD_WRITE_FPDMA_QUEUED:
		case ATA_CMD_READ_DMA:
		case ATA_CMD_WRITE_DMA:
			return SG_DMA;
		default:
			return SG_PIO;
	}
}
unsigned char excute_taskfile(struct scsi_device *dev,ide_task_request_t *req_task,u8 
 rw,char *argbuf,unsigned int buff_size)
{
	int rc = 0;
     	u8 scsi_cmd[MAX_COMMAND_SIZE];
      	u8 *sensebuf = NULL;
      	int argsize=0;
	enum dma_data_direction data_dir;
      	int cmd_result;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	struct scsi_request *sreq;
#endif
      	argsize=buff_size;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	sensebuf = kmalloc(SCSI_SENSE_BUFFERSIZE, GFP_NOIO);
	if (sensebuf) {
		memset(sensebuf, 0, SCSI_SENSE_BUFFERSIZE);
	}
#else
	sensebuf = kzalloc(SCSI_SENSE_BUFFERSIZE, GFP_NOIO);
#endif 
      	if (!sensebuf)
          	return -ENOMEM;
  
      	memset(scsi_cmd, 0, sizeof(scsi_cmd));

  	data_dir = DMA_FROM_DEVICE;
	scsi_cmd[0] = ATA_16;
	scsi_cmd[13] = 0x40;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 24)
      	scsi_cmd[14] = ((task_struct_t *)(&req_task->io_ports))->command;
#else
	scsi_cmd[14] = ((char *)(&req_task->io_ports))[7];
#endif
	if(check_dma(scsi_cmd[14])){
		scsi_cmd[1] = argbuf ? SG_ATA_PROTO_DMA : SG_ATA_PROTO_NON_DATA;
	} else {
		scsi_cmd[1] = argbuf ? (rw ? SG_ATA_PROTO_PIO_OUT : SG_ATA_PROTO_PIO_IN) : SG_ATA_PROTO_NON_DATA;
	}
	scsi_cmd[ 2] = SG_CDB2_CHECK_COND;
	if (argbuf) {
		scsi_cmd[2] |= SG_CDB2_TLEN_NSECT | SG_CDB2_TLEN_SECTORS;
		scsi_cmd[2] |= rw ? SG_CDB2_TDIR_TO_DEV : SG_CDB2_TDIR_FROM_DEV;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	sreq = scsi_allocate_request(dev, GFP_KERNEL);
	if (!sreq) {
		rc= -EINTR;
		goto free_req;
	}
	sreq->sr_data_direction = data_dir;
	scsi_wait_req(sreq, scsi_cmd, argbuf, argsize, (10*HZ), 5);

	/* 
	 * If there was an error condition, pass the info back to the user. 
	 */
	cmd_result = sreq->sr_result;
	sensebuf = sreq->sr_sense_buffer;
   
#elif LINUX_VERSION_CODE >=KERNEL_VERSION(2, 6, 29)
      	cmd_result = scsi_execute(dev, scsi_cmd, data_dir, argbuf, argsize,
                                sensebuf, (10*HZ), 5, 0,0);
#else
      	cmd_result = scsi_execute(dev, scsi_cmd, data_dir, argbuf, argsize,
                                sensebuf, (10*HZ), 5, 0);
#endif
  
      	if (driver_byte(cmd_result) == DRIVER_SENSE) {/* sense data available */
         	u8 *desc = sensebuf + 8;
          	cmd_result &= ~(0xFF<<24);
  		
          	if (cmd_result & SAM_STAT_CHECK_CONDITION) {
              	struct scsi_sense_hdr sshdr;
              	scsi_normalize_sense(sensebuf, SCSI_SENSE_BUFFERSIZE,
                                   &sshdr);
              	if (sshdr.sense_key==0 &&
                  	sshdr.asc==0 && sshdr.ascq==0)
                  	cmd_result &= ~SAM_STAT_CHECK_CONDITION;
          	}
      	}
  
      	if (cmd_result) {
          	rc = EIO;
		MV_PRINT("EIO=%d\n",-EIO);
          	goto free_req;
      	}

free_req:
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
	scsi_release_request(sreq);
#endif		
      	if (sensebuf) kfree(sensebuf);
      	return rc;
}
u8 mv_do_taskfile_ioctl(struct scsi_device *dev,void __user *arg){
	ide_task_request_t *req_task=NULL;
	char __user *buf = (char __user *)arg;
	u8 *outbuf	= NULL;
	u8 *inbuf	= NULL;
	int err		= 0;
	int tasksize	= sizeof(ide_task_request_t);
	int taskin	= 0;
	int taskout	= 0;
	int rw = SG_READ;
	
	req_task = kzalloc(tasksize, GFP_KERNEL);
	if (req_task == NULL) return -ENOMEM;
	if (copy_from_user(req_task, buf, tasksize)) {
		kfree(req_task);
		return -EFAULT;
	}

	switch (req_task->req_cmd) {
		case TASKFILE_CMD_REQ_OUT:
		case TASKFILE_CMD_REQ_RAW_OUT:
			rw         = SG_WRITE;
			break;
		case TASKFILE_CMD_REQ_IN:
			break;
	}
	taskout = (int) req_task->out_size;
	taskin  = (int) req_task->in_size;


	if (taskout) {
		int outtotal = tasksize;
		outbuf = kzalloc(taskout, GFP_KERNEL);
		if (outbuf == NULL) {
			err = -ENOMEM;
			goto abort;
		}
		if (copy_from_user(outbuf, buf + outtotal, taskout)) {
			err = -EFAULT;
			goto abort;
		}
	}

	if (taskin) {
		int intotal = tasksize + taskout;
		inbuf = kzalloc(taskin, GFP_KERNEL);
		if (inbuf == NULL) {
			err = -ENOMEM;
			goto abort;
		}
		if (copy_from_user(inbuf, buf + intotal, taskin)) {
			err = -EFAULT;
			goto abort;
		}
	}
	
	switch(req_task->data_phase) {
		case TASKFILE_DPHASE_PIO_OUT:
			err = excute_taskfile(dev,req_task,rw,outbuf,taskout);
			break;
		default:
			err = -EFAULT;
			goto abort;
	}
	if (copy_to_user(buf, req_task, tasksize)) {
		err = -EFAULT;
		goto abort;
	}
	if (taskout) {
		int outtotal = tasksize;
		if (copy_to_user(buf + outtotal, outbuf, taskout)) {
			err = -EFAULT;
			goto abort;
		}
	}
	if (taskin) {
		int intotal = tasksize + taskout;
		if (copy_to_user(buf + intotal, inbuf, taskin)) {
			err = -EFAULT;
			goto abort;
		}
	}
abort:
	kfree(req_task);
	kfree(outbuf);
	kfree(inbuf);

	return err;
}

int mv_new_ioctl(struct scsi_device *dev, int cmd, void __user *arg)
{
	int error,writing = 0, length;
	int console,nr_hba;
	int nbit;
	static int mutex_flag = 0;
	void * kbuf = NULL;
	int val = 0;
	struct scsi_idlun idlun;
	struct request *rq;
	struct request_queue * q = dev->request_queue;
	PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER psptdwb = NULL;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,9)
	struct scsi_request *sreq;
#endif

	if(mutex_flag == 0){
		mutex_init(&ioctl_index_mutex);
		mutex_flag = 1;
	}
	switch (cmd){

	case HDIO_GET_IDENTITY:
		psptdwb = hba_mem_alloc(sizeof(*psptdwb),MV_FALSE);
		if (!psptdwb)
			return -ENOMEM;
		psptdwb->sptd.DataTransferLength = 512;
		psptdwb->sptd.DataBuffer = arg;
		psptdwb->sptd.CdbLength = 16;
		memcpy((void*)psptdwb->sptd.Cdb, ata_ident, 16);
		goto fetch_data;

	case HDIO_GET_32BIT:
		if (copy_to_user(arg, &val, 1))
			return -EFAULT;
		return 0;
	case HDIO_SET_32BIT:
		val = (unsigned long)arg;
		if (val != 0)
			return -EINVAL;
		return 0;
	case HDIO_DRIVE_CMD:
             	if (!capable(CAP_SYS_ADMIN) || !capable(CAP_SYS_RAWIO))
                 	return -EACCES;
 
             	return mv_ial_ht_ata_cmd(dev, arg);

	case HDIO_DRIVE_TASK:
		if (!capable(CAP_SYS_ADMIN) || !capable(CAP_SYS_RAWIO))
			return -EACCES;
		return mv_ata_task_ioctl(dev, arg);
	case HDIO_DRIVE_TASKFILE:
		return mv_do_taskfile_ioctl(dev,arg);

	default:
		break;
	}

	switch(cmd - API_BLOCK_IOCTL_DEFAULT_FUN){
	case API_IOCTL_GET_VIRTURL_ID:
		console = VIRTUAL_DEVICE_ID;
		if (copy_to_user(((PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER) arg)->sptd.DataBuffer,
			(void *)&console,sizeof(int)))
			return -EIO;
		return 0;
	case API_IOCTL_GET_HBA_COUNT:
		nr_hba = __mv_get_adapter_count();
		if (copy_to_user(((PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER) arg)->sptd.DataBuffer,
			(void *)&nr_hba,sizeof(unsigned int)))
			return -EIO;
		return 0;
	case API_IOCTL_LOOKUP_DEV:
		if(copy_from_user(&idlun, ((PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER) arg)->sptd.DataBuffer,
			sizeof(struct scsi_idlun)))
			return -EIO;
		if(dev->host->host_no != ((idlun.dev_id) >> 24))
			return EFAULT;
		return 0;
	case API_IOCTL_CHECK_VIRT_DEV:
		if( dev->id != VIRTUAL_DEVICE_ID)
			return -EFAULT;
		return 0;
	case API_IOCTL_DEFAULT_FUN:
		break;
	default:
		return -ENOTTY;
	}

	psptdwb = hba_mem_alloc(sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),MV_FALSE);
	if (!psptdwb)
		return -ENOMEM;
	error = copy_from_user(psptdwb, (void *)arg, sizeof(*psptdwb));
	if (error) {
		hba_mem_free(psptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),MV_FALSE);
		return -EIO;
	}
fetch_data:
	length = psptdwb->sptd.DataTransferLength;

	if (length){
		if(length > IOCTL_BUF_LEN || (kbuf = hba_mem_alloc(length,MV_TRUE)) == NULL ){
			hba_mem_free(psptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),MV_FALSE);
			return -ENOMEM;
		}
		if(copy_from_user(kbuf, psptdwb->sptd.DataBuffer, length)){
			hba_mem_free(psptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),MV_FALSE);
			hba_mem_free(kbuf,length,MV_TRUE);
			return -EIO;
		}

		if (SCSI_IS_WRITE(psptdwb->sptd.Cdb[0]))
			writing = 1;
	}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,9)
	rq = blk_get_request(q, writing ? WRITE : READ, GFP_KERNEL);
	if (!rq) {
		hba_mem_free(psptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),MV_FALSE);
		return -ENOMEM;
	}
#else
	sreq = scsi_allocate_request(dev, GFP_KERNEL);
	if (!sreq) {
		MV_PRINT("SCSI internal ioctl failed, no memory\n");
		return -ENOMEM;
	}
	rq = sreq->sr_request;
#endif

	rq->cmd_len = psptdwb->sptd.CdbLength;
	psptdwb->sptd.ScsiStatus = REQ_STATUS_PENDING;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
	rq->cmd_type = REQ_TYPE_BLOCK_PC;
	rq->tag = psptdwb->sptd.ScsiStatus;
#else
  #if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,9)
	rq->flags |= REQ_BLOCK_PC;
	rq->rq_status = psptdwb->sptd.ScsiStatus;
  #else
  	rq->tag = psptdwb->sptd.ScsiStatus;
  #endif
#endif
	rq->timeout = msecs_to_jiffies(psptdwb->sptd.TimeOutValue);
	if(!rq->timeout)
		rq->timeout = 60 * HZ;
	memcpy(rq->cmd, psptdwb->sptd.Cdb, psptdwb->sptd.CdbLength);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,17)
	if(length && blk_rq_map_kern(q, rq,kbuf, length, __GFP_WAIT)){
		error = -EIO;
		goto out;
	}
	rq->retries = 1;
#else
	rq->data = kbuf;
	rq->data_len = length;
#endif

	rq->sense = psptdwb->Sense_Buffer;
	rq->sense_len = psptdwb->sptd.SenseInfoLength;



	mutex_lock(&ioctl_index_mutex);
	nbit = find_first_zero_bit((unsigned long*)kbuf_index,512);
	if(nbit >= 512){
		mutex_unlock(&ioctl_index_mutex);
		error = -ENOSPC;
		goto out;
	}
	__set_bit(nbit, kbuf_index);
	mutex_unlock(&ioctl_index_mutex);

	rq->errors= nbit + 1;
	kbuf_array[nbit] = kbuf;
	memcpy((void*)mvcdb[nbit],rq->cmd,16);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,9)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14)
	error = blk_execute_rq(q, NULL, rq, 0);
#else
	error = blk_execute_rq(q, NULL, rq);
#endif

#else
	sreq->sr_data_direction = writing ? DMA_TO_DEVICE : DMA_FROM_DEVICE;
	scsi_wait_req(sreq, rq->cmd, kbuf, length,  rq->timeout, 1);
	memcpy(rq->sense,sreq->sr_sense_buffer,rq->sense_len);
	if(sreq->sr_result)
		error = -EIO;
	else
		error = sreq->sr_result;
#endif/*#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,9)*/

	mutex_lock(&ioctl_index_mutex);
	__clear_bit(nbit,kbuf_index);
	mutex_unlock(&ioctl_index_mutex);
	kbuf_array[nbit] = NULL;
	memset((void*)mvcdb[nbit],0x00,16);

	if (length && (cmd == HDIO_GET_IDENTITY))
		io_get_identity(kbuf);
	
	if (error) {
		if ( rq->sense_len && rq->sense && ((MV_PU8)rq->sense)[0]) {
			MV_DPRINT(("%s has sense,rq->sense[0]=0x%x\n", __func__,((MV_PU8)rq->sense)[0]));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19) || LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,9)
			if (rq->tag == REQ_STATUS_ERROR_WITH_SENSE)
				error = rq->tag;
#else
			if(rq->rq_status == REQ_STATUS_ERROR_WITH_SENSE)
				error = rq->rq_status;
#endif
		}
	} else if (length && copy_to_user(psptdwb->sptd.DataBuffer,kbuf,length)) {
                        error = -EIO;
                        goto out;
	}
	if (mv_is_api_cmd(cmd) && copy_to_user((void*)arg,psptdwb,
		sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER))){
		error = -EIO;
		goto out;
	}

out:
	hba_mem_free(psptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),MV_FALSE);
	hba_mem_free(kbuf, psptdwb->sptd.DataTransferLength,MV_TRUE);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,9)
	blk_put_request(rq);
#else
	scsi_release_request(sreq);
#endif
	return error;
}

