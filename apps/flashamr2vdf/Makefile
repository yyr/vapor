TOP = ../..

include ${TOP}/make/config/arch.mk

PROGRAM = flashamr2vdf
FILES = flashamr2vdf flashhdf5


ifdef   NETCDF_INC_PATH
MAKEFILE_CXXFLAGS += -I$(NETCDF_INC_PATH)
endif

ifdef   EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

ifdef   HDF5_INC_PATH
MAKEFILE_CXXFLAGS += -I$(HDF5_INC_PATH)
endif

ifdef HDF5_LIB_PATH
ifeq ($(ARCH),WIN32)
MAKEFILE_LDFLAGS += /LIBPATH:$(HDF5_LIB_PATH)
else
MAKEFILE_LDFLAGS += -L$(HDF5_LIB_PATH)
endif
endif


LIBRARIES = common vdf hdf5


include ${TOP}/make/config/base.mk
