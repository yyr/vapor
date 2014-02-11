TOP = ../

include $(TOP)/make/config/prebase.mk


SUBDIRS = common vdf

ifeq ($(BUILD_GUI), 1)
SUBDIRS += params render
endif


ifeq ($(BUILD_IDL_WRAPPERS), 1)
SUBDIRS += idl 
endif

include ${TOP}/make/config/base.mk

