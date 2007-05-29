TOP = ../..

include ${TOP}/make/config/prebase.mk

PROGRAM = wrf2vdf
FILES = wrf2vdf

ifdef   NETCDF_INC_PATH
MAKEFILE_CXXFLAGS += -I$(NETCDF_INC_PATH)
endif

ifdef   EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

ifdef NETCDF_LIB_PATH
ifeq ($(ARCH),WIN32)
MAKEFILE_LDFLAGS += /LIBPATH:$(NETCDF_LIB_PATH)
else
MAKEFILE_LDFLAGS += -L$(NETCDF_LIB_PATH)
endif
endif

#LIB_DEFS = vdf.def
  
LIBRARIES = vdf common netcdf

include ${TOP}/make/config/base.mk
