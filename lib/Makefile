TOP = ../

include $(TOP)/make/config/arch.mk

ifeq ($(ARCH),WIN32) 
SUBDIRS = common vdf 
else
SUBDIRS = common vdf idl
endif

include ${TOP}/make/config/base.mk

