TOP = ../..

include ${TOP}/make/config/prebase.mk

PROGRAM = flashamr2vdf
FILES = flashamr2vdf flashhdf5


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


LIBRARIES = common vdf hdf5 netcdf $(EXPAT_LIB)


include ${TOP}/make/config/base.mk
