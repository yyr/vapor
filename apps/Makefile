TOP = ../

include $(TOP)/make/config/prebase.mk

SUBDIRS = vdfcreate raw2vdf vdf2raw vaporversion ncdf2vdf momvdfcreate \
	mom2vdf vdfls  cart2wrfvdf \
	romsvdfcreate roms2vdf ncdfvdfcreate

ifeq ($(BUILD_AMR_SUPPORT), 1)
SUBDIRS += flashamr2vdf 
endif

ifeq ($(BUILD_GUI), 1)
SUBDIRS += vaporgui tiff2geotiff
endif

include ${TOP}/make/config/base.mk

