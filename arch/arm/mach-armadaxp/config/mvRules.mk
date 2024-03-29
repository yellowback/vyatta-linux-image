# This flags will be used only by the Marvell arch files compilation.

###################################################################################################
# General definitions
###################################################################################################
CPU_ARCH    = ARM
CHIP        = 88F78xx0
VENDOR      = Marvell
ifeq ($(CONFIG_CPU_BIG_ENDIAN),y)
ENDIAN      = BE
else
ENDIAN      = LE
endif

###################################################################################################
# directory structure
###################################################################################################
# Main directory structure
PLAT_PATH	  = ../plat-armada
PLAT_DRIVERS	  = $(PLAT_PATH)/mv_drivers_lsp
HAL_DIR           = $(PLAT_PATH)/mv_hal
COMMON_DIR        = $(PLAT_PATH)/common
OSSERV_DIR        = $(PLAT_PATH)/linux_oss
CONFIG_DIR        = config
HAL_IF		  = mv_hal_if

# HALs
HAL_ETHPHY_DIR    = $(HAL_DIR)/eth-phy
HAL_FLASH_DIR     = $(HAL_DIR)/flash
HAL_RTC_DIR       = $(HAL_DIR)/rtc/integ_rtc
HAL_EXT_RTC_DIR   = $(HAL_DIR)/rtc/ext_rtc
HAL_VOICEBAND     = $(HAL_DIR)/voiceband
HAL_SLIC_DIR      = $(HAL_VOICEBAND)/slic
HAL_DAA_DIR       = $(HAL_VOICEBAND)/daa
HAL_SATA_DIR      = $(HAL_DIR)/sata/CoreDriver/
HAL_QD_DIR        = $(HAL_DIR)/qd-dsdt
HAL_SFLASH_DIR    = $(HAL_DIR)/sflash
HAL_CNTMR_DIR     = $(HAL_DIR)/cntmr
HAL_DRAM_DIR      = $(HAL_DIR)/ddr2_3/
#HAL_DRAM_SPD_DIR  = $(HAL_DIR)/ddr2_3/spd
HAL_GPP_DIR       = $(HAL_DIR)/gpp
HAL_TWSI_DIR      = $(HAL_DIR)/twsi
HAL_TWSI_ARCH_DIR = $(SOC_TWSI_DIR)/Arch$(CPU_ARCH)
HAL_UART_DIR      = $(HAL_DIR)/uart

ifeq ($(CONFIG_MV_ETH_NETA),y)
HAL_ETH_DIR       = $(HAL_DIR)/neta 
HAL_ETH_GBE_DIR   = $(HAL_DIR)/neta/gbe
HAL_ETH_NFP_DIR   = $(HAL_DIR)/neta/nfp
HAL_ETH_PNC_DIR   = $(HAL_DIR)/neta/pnc
HAL_ETH_BM_DIR    = $(HAL_DIR)/neta/bm
HAL_ETH_PMT_DIR   = $(HAL_DIR)/neta/pmt
LSP_NETWORK_DIR   = $(PLAT_DRIVERS)/mv_neta
LSP_NET_DEV_DIR   = $(LSP_NETWORK_DIR)/net_dev
LSP_NFP_MGR_DIR   = $(LSP_NETWORK_DIR)/nfp_mgr
LSP_PNC_DIR       = $(LSP_NETWORK_DIR)/pnc
LSP_BM_DIR        = $(LSP_NETWORK_DIR)/bm
LSP_PMT_DIR       = $(LSP_NETWORK_DIR)/pmt
LSP_HWF_DIR       = $(LSP_NETWORK_DIR)/hwf
LSP_L2FW_DIR      = $(LSP_NETWORK_DIR)/l2fw
LSP_SWITCH_DIR    = $(PLAT_DRIVERS)/mv_switch
endif

ifeq ($(CONFIG_MV_ETH_LEGACY),y)
HAL_ETH_DIR       = $(HAL_DIR)/eth
HAL_ETH_GBE_DIR   = $(HAL_DIR)/eth/gbe
HAL_ETH_NFP_DIR	  = $(HAL_DIR)/eth/nfp
LSP_NETWORK_DIR   = $(PLAT_DRIVERS)/mv_network
LSP_NET_DEV_DIR   = $(LSP_NETWORK_DIR)/mv_etherent
LSP_NFP_MGR_DIR   = $(LSP_NETWORK_DIR)/nfp_mgr
endif

HAL_CPU_DIR       = $(HAL_DIR)/cpu
HAL_SDMMC_DIR	  = $(HAL_DIR)/sdmmc
ifeq ($(CONFIG_MV_INCLUDE_PEX),y)
HAL_PCI_DIR	  = $(HAL_DIR)/pci
HAL_PEX_DIR       = $(HAL_DIR)/pex
endif
ifeq ($(CONFIG_MV_INCLUDE_TDM),y)
HAL_TDM_DIR       = $(HAL_DIR)/voiceband/tdm
endif
ifeq ($(CONFIG_MV_INCLUDE_USB),y)
HAL_USB_DIR       = $(HAL_DIR)/usb
endif
ifeq ($(CONFIG_MV_INCLUDE_CESA),y)
HAL_CESA_DIR	  = $(HAL_DIR)/cesa
HAL_CESA_AES_DIR  = $(HAL_DIR)/cesa/AES
endif
ifeq ($(CONFIG_MV_INCLUDE_XOR),y)
HAL_XOR_DIR       = $(HAL_DIR)/xor
endif
ifeq ($(CONFIG_MV_INCLUDE_SPI),y)
HAL_SPI_DIR       = $(HAL_DIR)/spi
endif
ifeq ($(CONFIG_MV_INCLUDE_AUDIO),y)
HAL_AUDIO_DIR     = $(HAL_DIR)/audio
endif
ifeq ($(CONFIG_MV_INCLUDE_NFC),y)
HAL_NFC_DIR     = $(HAL_DIR)/nfc
endif

LSP_TRACE_DIR   = $(PLAT_DRIVERS)/mv_trace

# Environment components
AXP_FAM_DIR	= armada_xp_family
SOC_DEVICE_DIR	= $(AXP_FAM_DIR)/device
SOC_CPU_DIR	= $(AXP_FAM_DIR)/cpu
BOARD_ENV_DIR	= $(AXP_FAM_DIR)/boardEnv
SOC_ENV_DIR	= $(AXP_FAM_DIR)/ctrlEnv
SOC_SYS_DIR	= $(AXP_FAM_DIR)/ctrlEnv/sys
HAL_IF_DIR	= mv_hal_if

#####################################################################################################
# Include path
###################################################################################################

LSP_PATH_I      = $(srctree)/arch/arm/mach-armadaxp
PLAT_PATH_I	= $(srctree)/arch/arm/plat-armada

HAL_PATH        = -I$(PLAT_PATH_I)/$(HAL_DIR) -I$(PLAT_PATH_I)/$(HAL_SATA_DIR) -I$(PLAT_PATH_I)/$(HAL_ETH_DIR)
AXP_FAM_PATH	= -I$(LSP_PATH_I)/$(AXP_FAM_DIR)
QD_PATH         = -I$(PLAT_PATH_I)/$(HAL_QD_DIR)/Include  -I$(PLAT_PATH_I)/$(HAL_QD_DIR)/Include/h/msApi 	\
                  -I$(PLAT_PATH_I)/$(HAL_QD_DIR)/Include/h/driver -I$(PLAT_PATH_I)/$(HAL_QD_DIR)/Include/h/platform
                     
COMMON_PATH   	= -I$(PLAT_PATH_I)/$(COMMON_DIR) -I$(srctree)
 
OSSERV_PATH     = -I$(PLAT_PATH_I)/$(OSSERV_DIR)
LSP_PATH        = -I$(LSP_PATH_I)
CONFIG_PATH     = -I$(LSP_PATH_I)/$(CONFIG_DIR)
HAL_IF_PATH	= -I$(LSP_PATH_I)/$(HAL_IF)
DRIVERS_LSP_PATH = -I$(PLAT_PATH_I)/$(PLAT_DRIVERS) -I$(PLAT_PATH_I)/$(LSP_NETWORK_DIR) -I$(PLAT_PATH_I)/$(LSP_SWITCH_DIR) \
		 -I$(PLAT_PATH_I)/$(LSP_TRACE_DIR)

EXTRA_INCLUDE  	= $(OSSERV_PATH) $(COMMON_PATH) $(HAL_PATH)  $(AXP_FAM_PATH) \
                  $(LSP_PATH) $(CONFIG_PATH) $(DRIVERS_LSP_PATH) $(HAL_IF_PATH)

###################################################################################################
# defines
###################################################################################################
MV_DEFINE = -DMV_LINUX -DMV_CPU_$(ENDIAN) -DMV_$(CPU_ARCH) 


ifeq ($(CONFIG_MV_GATEWAY),y)
EXTRA_INCLUDE	+= $(QD_PATH)
EXTRA_CFLAGS    += -DLINUX  
endif

ifeq ($(CONFIG_MV_INCLUDE_SWITCH),y)
EXTRA_INCLUDE   += $(QD_PATH)
EXTRA_CFLAGS    += -DLINUX
endif

ifeq ($(CONFIG_MV_CESA_TEST),y)
EXTRA_CFLAGS 	+= -DCONFIG_MV_CESA_TEST
endif

ifeq ($(CONFIG_SATA_DEBUG_ON_ERROR),y)
EXTRA_CFLAGS    += -DMV_LOG_ERROR
endif

ifeq ($(CONFIG_SATA_FULL_DEBUG),y)
EXTRA_CFLAGS    += -DMV_LOG_DEBUG
endif

ifeq ($(CONFIG_MV_SATA_SUPPORT_ATAPI),y)
EXTRA_CFLAGS    += -DMV_SUPPORT_ATAPI
endif

ifeq ($(CONFIG_MV_SATA_ENABLE_1MB_IOS),y)
EXTRA_CFLAGS    += -DMV_SUPPORT_1MBYTE_IOS
endif

ifeq ($(CONFIG_PCIE_VIRTUAL_BRIDGE_SUPPORT),y)
EXTRA_CFLAGS    +=-DPCIE_VIRTUAL_BRIDGE_SUPPORT
endif

ifeq ($(CONFIG_MV_CESA_CHAIN_MODE_SUPPORT),y)
EXTRA_CFLAGS    += -DMV_CESA_CHAIN_MODE_SUPPORT
endif

EXTRA_CFLAGS 	+= $(EXTRA_INCLUDE) $(MV_DEFINE)

EXTRA_AFLAGS 	+= $(EXTRA_CFLAGS)
