TOP = ../..

include ${TOP}/make/config/prebase.mk

PROGRAM = vdfcreate
FILES = vdfcreate


ifeq ($(BUILD_GUI),1)
LIBRARIES = pthread dl util m vdf python2.6 common netcdf expat
else
LIBRARIES = vdf common netcdf expat
endif


include ${TOP}/make/config/base.mk

