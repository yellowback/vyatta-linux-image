#
#
# Note: This Makefile is for 2.6 kernel only, at present.
#
# V0.0.0.6 Ver.Make

# default to build for the running kernel
ifeq ("x", "x$(KERNEL_SRC)")
  KERNEL_SRC=/lib/modules/$(shell uname -r)/build
  
  ifeq ("powerpc", "$(ARCH)")
	KERNEL_SRC=/root/katmai_29/lib/modules/2.6.29/build
  endif

  ifeq ("arm", "$(ARCH)")
	KERNEL_SRC=/root/armadaxp/linux-2.6.35.9
  endif
endif

# use KERNEL_SRC if not called by our driver disk maker
ifeq ("x", "x$(KERNEL_SOURCE_DIR)")
        KERNEL_SRC_DIR=$(KERNEL_SRC)
else
	KERNEL_SRC_DIR=$(KERNEL_SOURCE_DIR)
endif

ifeq ("x", "x$(KERNEL_BUILD_DIR)")
	KERNEL_BLD_DIR=$(KERNEL_SRC_DIR)
else
        KERNEL_BLD_DIR=$(KERNEL_BUILD_DIR)
endif

#ifneq ($(KERNELRELEASE),)
#	include $(SUBDIRS)/mv_conf.mk
#else
#	include mv_conf.mk
#endif
SUPPORT_VANIR=y

ifneq ("arm", "$(ARCH)")
CONFIG_64BIT=$(shell [ -f $(KERNEL_BLD_DIR)/.config ] && cat $(KERNEL_BLD_DIR)/.config | grep -m 1 CONFIG_64BIT | awk -F= '{print $$2}')
CONFIG_REGPARM=$(shell [ -f $(KERNEL_BLD_DIR)/.config ] && cat $(KERNEL_BLD_DIR)/.config | grep -m 1 CONFIG_REGPARM | awk -F= '{print $$2}')
CONFIG_SUSE_KERNEL=$(shell [ -f $(KERNEL_BLD_DIR)/.config ] && cat $(KERNEL_BLD_DIR)/.config | grep -m 1 CONFIG_SUSE_KERNEL | awk -F= '{print $$2}')
endif

ifeq ($(ARCH), )
ifeq ($(strip $(CONFIG_64BIT)),y)
	ARCH_TYPE=x86_64
else	
	ARCH_TYPE=i386
endif
else
	ARCH_TYPE=$(ARCH)
endif

ifeq ("powerpc", "$(ARCH)")
	MV_CC     =  $(CROSS_COMPILE)gcc
	MV_LD     =  $(CROSS_COMPILE)ld
else
ifeq ("arm", "$(ARCH)") 
	MV_CC	  =  $(CROSS_COMPILE)gcc
	MV_LD	  =  $(CROSS_COMPILE)ld
else
	MV_CC     =  $(CROSS_COMPILE)$(CC)
	MV_LD     =  $(CROSS_COMPILE)$(LD)
endif
endif

export LD_LIBRARY_PATH
core_com_objs := core/api/core_api.o  core/api/core_api_exp.o core/api/core_api_bbu.o core/api/core_api_flash.o core/api/core_api_led.o core/api/core_api_pd.o \
		core/core/core_alarm.o core/core/core_device.o	 \
	   core/core/core_error.o \
	   core/core/core_event.o core/core/core_large_req.o \
	   core/core/core_manager.o \
	   core/core/core_protocol.o \
	   core/core/core_resource.o \
	   core/core/core_util.o  \
	   core/pl/core_gpio.o \
	   core/pl/core_rmw_flash.o \
	   core/pm/core_pm.o \
	   core/sas_base/sas_error.o core/sas_base/sas_handler.o \
	   core/sas_base/sas_util.o core/sas_base/sas_init.o \
	   core/sas_smp/core_expander.o \
	   core/sata_base/core_sata.o core/sata_base/sata_error.o \
	   core/scsi/core_sat.o \
	   core/scsi/core_ses.o


vanir_objs_$(SUPPORT_VANIR)   := product/vanir/core_spi.o   \
			       product/vanir/core_hal.o \
			       product/vanir/core_init.o	

odin_objs_$(SUPPORT_ODIN)   := 	product/odin/core_spi.o	\
					   product/odin/core_hal.o \
					   product/odin/core_init.o	
					   
thor_objs_$(SUPPORT_THOR)   := 	product/thor/core_spi.o	\
					   product/thor/core_hal.o \
					   product/thor/core_init.o	

CORE_OBJS       := $(core_com_objs) $(odin_objs_y) $(thor_objs_y) $(vanir_objs_y)


COMM_OBJS       := lib/common/com_util.o lib/common/com_u64.o      \
		   lib/common/com_scsi.o lib/common/com_tag.o      \
		   lib/common/com_sgd.o  lib/common/com_nvram.o    \
		   linux/hba_priv.o	hba_sm/hba_api.o

OSDEP_OBJS      := linux/linux_main.o    linux/hba_exp.o          \
		   linux/hba_mod.o       linux/hba_timer.o        \
		   linux/mv_os.o   linux/linux_iface.o  	

# for partial source tree building
include_dir := include  include/generic  include/icommon

header_dirs     :=   $(include_dir)

header_file_$(SUPPORT_VANIR) :=mv_config_vanir.h
header_file_$(SUPPORT_ODIN) :=mv_config_odin.h
header_file_$(SUPPORT_THOR) :=mv_config_thor.h

ifeq ($(RAID_MODULE), y)
LIBMV_OBJS	:=  $(COMM_OBJS) $(RAID_OBJS) $(CORE_OBJS) 
else
LIBMV_OBJS	:=  $(COMM_OBJS) $(CORE_OBJS) 
endif

HBA_OBJS        := $(OSDEP_OBJS) $(LIBMV_OBJS) 

ifneq ("arm", "$(ARCH)")
INCLUDE_DIR     = -I$(KERNEL_BLD_DIR)/include                          \
		  -I$(KERNEL_BLD_DIR)/include/scsi                     \
		  -I$(KERNEL_BLD_DIR)/drivers/scsi                     \
	 	  -I$(KERNEL_SRC_DIR)/include                          \
	 	  -I$(KERNEL_SRC_DIR)/include/scsi                     \
	 	  -I$(KERNEL_SRC_DIR)/drivers/scsi
endif


ifneq ($(KERNELRELEASE),)

ifeq ($(SUPPORT_VANIR),y)
obj-$(CONFIG_SCSI_MV_VANIR) += mv94xx.o
#obj-m         :=  mv94xx.o
mv94xx-objs   :=   $(HBA_OBJS) 
endif

ifeq ($(SUPPORT_ODIN),y)
obj-m         :=  mv64xx.o
mv64xx-objs   :=   $(HBA_OBJS) 
endif

ifeq ($(SUPPORT_THOR),y)
obj-m         :=  mv61xx.o
mv61xx-objs   :=   $(HBA_OBJS) 
endif

clean-files   +=   Modules.symvers
clean-files   +=   Module.symvers modules.order 
EXTRA_CFLAGS  :=   -I$(src)/include   -I$(src)/core  \
		   -I$(src)/include   -I$(src)/core/api  \
		   -I$(src)/include   -I$(src)/core/core  \
		   -I$(src)/include   -I$(src)/core/pl  \
		   -I$(src)/include   -I$(src)/core/pm  \
		   -I$(src)/include   -I$(src)/core/sas_base  \
		   -I$(src)/include   -I$(src)/core/sas_smp  \
		   -I$(src)/include   -I$(src)/core/sata_base  \
		   -I$(src)/include   -I$(src)/core/scsi  \
		   -I$(src)/raid       -I$(src)/.               \
		   -I$(src)/linux  -I$(src)/hba_sm -I$(src)/math            \
                   -I$(src)/cache      -I$(src)/include/generic \
		   -I$(src)/include/icommon

ifeq ($(SUPPORT_VANIR),y)
EXTRA_CFLAGS += -I$(src)/product/vanir
endif

ifeq ($(SUPPORT_ODIN),y)
EXTRA_CFLAGS += -I$(src)/product/odin
endif

ifeq ($(SUPPORT_THOR),y)
EXTRA_CFLAGS += -I$(src)/product/thor
endif

EXTRA_CFLAGS  +=  -D__MV_LINUX__ $(INCLUDE_DIR) -std=gnu99 #-D__MV_DEBUG__

ifeq ($(CONFIG_64BIT), y)
EXTRA_CFLAGS  +=   -D_64_SYS_
else
EXTRA_CFLAGS  +=   -D_32_LEGACY_
endif

EXTRA_CFLAGS  +=   -D_64_BIT_COMPILER

EXTRA_CFLAGS  +=   -include $(src)/mv_config.h

ifeq ($(RAID_MODULE), y)
EXTRA_CFLAGS  +=   -DRAID_DRIVER=1
endif

ifeq ($(RAID6_MODULE), y)
EXTRA_CFLAGS  +=   -DSUPPORT_RAID6=1
endif

ifeq ($(CACHE_MODULE), y)
EXTRA_CFLAGS  +=   -DCACHE_MODULE_SUPPORT=1
endif

ifeq ($(SUPPORT_VANIR), y)
EXTRA_CFLAGS  +=   -DPRODUCTNAME_VANIR
endif

ifeq ($(SUPPORT_ODIN), y)
EXTRA_CFLAGS  +=   -DPRODUCTNAME_ODIN
endif

ifeq ($(SUPPORT_THOR), y)
EXTRA_CFLAGS  +=   -DPRODUCTNAME_THOR
endif

ifeq ($(CONFIG_SUSE_KERNEL), y)
EXTRA_CFLAGS  += -DIS_OPENSUSE_SLED_SLES=1
else
EXTRA_CFLAGS	+= -DIS_OPENSUSE_SLED_SLES=0
endif

EXTRA_CFLAGS  +=  -Wno-unused-variable 
EXTRA_CFLAGS  += -D__LEGACY_OSSW__=1 -D_OS_LINUX	  

else

# Why use SUBDIRS? for backward compatibility
all:
	$(MAKE) ARCH=$(ARCH_TYPE) CC=$(MV_CC) LD=$(MV_LD) CROSS_COMPILE=$(CROSS_COMPILE) V=$(V) -C $(KERNEL_BLD_DIR) SUBDIRS=`pwd` modules

kbuild:
	-@cat patch.kbuild | patch -p1 -N -d $(KERNEL_SRC_DIR)/drivers/scsi
	@sh patch.sh $(KERNEL_SRC_DIR)/drivers/scsi a
	@mkdir -p $(KERNEL_SRC_DIR)/drivers/scsi/mv
	@cp -r * $(KERNEL_SRC_DIR)/drivers/scsi/mv
	@cp -f Makefile.kbuild $(KERNEL_SRC_DIR)/drivers/scsi/mv/Makefile
	@echo done.

ukbuild:
	-@cat patch.kbuild | patch -p1 -N -R -d $(KERNEL_SRC_DIR)/drivers/scsi
	@sh patch.sh $(KERNEL_SRC_DIR)/drivers/scsi
	@rm -rf $(KERNEL_SRC_DIR)/drivers/scsi/mv
	@echo Patch has been successfully rewinded.

clean:  
	$(MAKE) ARCH=$(ARCH_TYPE) CC=$(MV_CC) LD=$(MV_LD) CROSS_COMPILE=$(CROSS_COMPILE) V=$(V) -C $(KERNEL_BLD_DIR) SUBDIRS=`pwd` clean
endif
