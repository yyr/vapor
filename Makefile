TOP = .

include $(TOP)/make/config/arch.mk

SUBDIRS = lib apps doc

ifeq ($(BUILD_TESTAPPS), 1)
SUBDIRS += test_apps
endif

include ${TOP}/make/config/base.mk

