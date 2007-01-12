TOP = ../..

include ${TOP}/make/config/prebase.mk

PROGRAM = vdfcreate
FILES = vdfcreate


ifdef   NETCDF_INC_PATH
MAKEFILE_CXXFLAGS += -I$(NETCDF_INC_PATH)
endif

ifdef   EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

LIBRARIES = vdf common

include ${TOP}/make/config/base.mk

