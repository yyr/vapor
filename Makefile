TOP = .

include $(TOP)/make/config/arch.mk

SUBDIRS = lib apps test_apps doc

include ${TOP}/make/config/base.mk

