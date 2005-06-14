TOP = ../

include $(TOP)/make/config/arch.mk

SUBDIRS = vdfcreate raw2vdf vdf2raw

ifeq ($(BUILD_GUI), 1)
SUBDIRS += vaporgui
endif

include ${TOP}/make/config/base.mk

