TOP = ../

include $(TOP)/make/config/arch.mk


SUBDIRS = common vdf jpeg

ifeq ($(HAVE_IDL), 1)
SUBDIRS += idl 
endif

include ${TOP}/make/config/base.mk

