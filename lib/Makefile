TOP = ../

include $(TOP)/make/config/arch.mk


SUBDIRS = common vdf

ifeq ($(BUILD_GUI), 1)
SUBDIRS += jpeg flow params glew render
endif


ifeq ($(BUILD_IDL_WRAPPERS), 1)
SUBDIRS += idl 
endif

include ${TOP}/make/config/base.mk

