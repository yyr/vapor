TOP = ../..

include ${TOP}/make/config/prebase.mk

PROGRAM = raw2vdf
FILES = raw2vdf

ifdef   NETCDF_INC_PATH
MAKEFILE_CXXFLAGS += -I$(NETCDF_INC_PATH)
endif

ifdef   EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif


#LIB_DEFS = vdf.def
  
LIBRARIES = vdf common

include ${TOP}/make/config/base.mk
