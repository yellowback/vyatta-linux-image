#
# Makefile for the Marvell Gigabit Ethernet driver
#

ifeq ($(CONFIG_ARCH_FEROCEON),y)
	include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

ifeq ($(CONFIG_ARCH_ARMADA370),y)
	include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

ifeq ($(CONFIG_ARCH_ARMADA_XP),y)
	include $(srctree)/arch/arm/mach-armadaxp/config/mvRules.mk
endif

ifeq ($(CONFIG_MV_ETH_NFP_LIB),y)
	obj-$(CONFIG_MV_ETHERNET) += mv_netdev.o mv_ethernet.o mv_eth_sysfs.o
	obj-$(CONFIG_MV_PON)      += mv_pon_sysfs.o
	obj-$(CONFIG_MV_ETH_SWITCH) +=  mv_eth_switch.o
	obj-$(CONFIG_MV_ETH_TOOL) += mv_eth_tool.o
	obj-y += ../nfplib.a
else
	obj-$(CONFIG_MV_ETHERNET) += mv_netdev.o mv_ethernet.o mv_eth_sysfs.o
	obj-$(CONFIG_MV_PON)      += mv_pon_sysfs.o
	obj-$(CONFIG_MV_ETH_SWITCH) +=  mv_eth_switch.o
	obj-$(CONFIG_MV_ETH_TOOL) += mv_eth_tool.o
endif

