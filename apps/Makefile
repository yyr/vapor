TOP = ../

include $(TOP)/make/config/arch.mk

SUBDIRS = vdfcreate raw2vdf vdf2raw 

ifeq ($(BUILD_AMR_SUPPORT), 1)
SUBDIRS += flashamr2vdf
endif

ifeq ($(BUILD_GUI), 1)
SUBDIRS += vaporgui
endif

include ${TOP}/make/config/base.mk

