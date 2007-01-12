TOP = ../..

include ${TOP}/make/config/prebase.mk

PROGRAM = test_datamgr
FILES = test_datamgr


LDFLAGS += $(EXPATLIB)

LIBRARIES = common vdf netcdf_c++ netcdf

MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
MAKEFILE_CXXFLAGS += -I$(NETCDF_INC_PATH)
MAKEFILE_LDFLAGS += $(EXPATLIB) -L$(NETCDF_LIB_PATH) 

include ${TOP}/make/config/base.mk

