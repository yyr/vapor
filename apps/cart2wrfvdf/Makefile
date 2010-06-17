TOP = ../..

include ${TOP}/make/config/prebase.mk

PROGRAM = cart2wrfvdf
FILES = cart2wrfvdf


#LIB_DEFS = vdf.def
ifeq ($(BUILD_GUI),1)
LIBRARIES = pthread dl util m vdf python2.6 common netcdf expat
else
LIBRARIES = vdf common netcdf expat
endif

include ${TOP}/make/config/base.mk
