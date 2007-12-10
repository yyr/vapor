TOP = ../

include $(TOP)/make/config/prebase.mk

SUBDIRS = vdfcreate raw2vdf vdf2raw vaporversion ncdf2vdf wrf2vdf \
	wrfvdfcreate asciitf2vtf vdfls

ifeq ($(BUILD_AMR_SUPPORT), 1)
SUBDIRS += flashamr2vdf
endif

ifeq ($(BUILD_GUI), 1)
SUBDIRS += vaporgui
endif

include ${TOP}/make/config/base.mk

