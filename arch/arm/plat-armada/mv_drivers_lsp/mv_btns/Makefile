#
# Makefile for the Marvell btns Driver
#
ifeq ($(CONFIG_ARCH_FEROCEON),y)
	include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

ifeq ($(CONFIG_ARCH_ARMADA_XP),y)
	include $(srctree)/arch/arm/mach-armadaxp/config/mvRules.mk
endif

obj-y += btns_driver.o
