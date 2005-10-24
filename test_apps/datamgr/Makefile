TOP = ../..

include ${TOP}/make/config/arch.mk

PROGRAM = test_datamgr
FILES = test_datamgr


LDFLAGS += $(EXPATLIB)

LIBRARIES = common vdf

MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
MAKEFILE_CXXFLAGS += -I$(NETCDF_INC_PATH)
MAKEFILE_LDFLAGS += $(EXPATLIB) -L$(NETCDF_LIB_PATH) -lnetcdf -lnetcdf_c++

include ${TOP}/make/config/base.mk

