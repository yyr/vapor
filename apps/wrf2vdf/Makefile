TOP = ../..

include ${TOP}/make/config/prebase.mk

PROGRAM = wrf2vdf
FILES = wrf2vdf


#LIB_DEFS = vdf.def

ifeq ($(BUILD_GUI),1)
LIBRARIES = pthread dl util m vdf python2.6 common netcdf proj expat
else
LIBRARIES = vdf common netcdf proj expat
endif

include ${TOP}/make/config/base.mk
