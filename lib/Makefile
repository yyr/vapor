TOP = ../

include $(TOP)/make/config/arch.mk


SUBDIRS = common vdf jpeg flow params

ifeq ($(BUILD_GUI), 1)
SUBDIRS += glew 
endif

ifeq ($(BUILD_IDL_WRAPPERS), 1)
SUBDIRS += idl 
endif

include ${TOP}/make/config/base.mk

