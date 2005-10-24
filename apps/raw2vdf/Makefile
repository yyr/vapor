TOP = ../..

include ${TOP}/make/config/arch.mk

PROGRAM = raw2vdf
FILES = raw2vdf


MAKEFILE_LDFLAGS += $(EXPATLIB)

ifdef	EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

ifdef   NETCDF_INC_PATH
MAKEFILE_CXXFLAGS += -I$(NETCDF_INC_PATH)
endif


LIBRARIES = common vdf 


include ${TOP}/make/config/base.mk

