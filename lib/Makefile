TOP = ../

include $(TOP)/make/config/arch.mk

ifeq ($(ARCH),WIN32) 
SUBDIRS = common vdf jpeg
else
SUBDIRS = common vdf idl jpeg
endif

include ${TOP}/make/config/base.mk

