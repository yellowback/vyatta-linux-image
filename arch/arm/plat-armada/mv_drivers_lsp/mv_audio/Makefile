#
# Makefile for the Marvell Audio ALSA Device Driver
#
ifeq ($(CONFIG_ARCH_FEROCEON),y)
	include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

ifeq ($(CONFIG_ARCH_ARMADA_XP),y)
	include $(srctree)/arch/arm/mach-armadaxp/config/mvRules.mk
endif

obj-$(CONFIG_SND_MRVL_AUDIO) += snd-mv88fx.o
#snd-mv88fx-objs		     += mv_snd.o ../../../../../sound/arm/devdma.o
snd-mv88fx-objs		     += mv88fx-pcm.o  cs42l51-hal.o mv88fx-hal.o #../../../../../sound/arm/devdma.o

