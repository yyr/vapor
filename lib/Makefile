TOP = ../

include $(TOP)/make/config/arch.mk


SUBDIRS = common vdf jpeg flow

ifeq ($(BUILD_IDL_WRAPPERS), 1)
SUBDIRS += idl 
endif

include ${TOP}/make/config/base.mk

