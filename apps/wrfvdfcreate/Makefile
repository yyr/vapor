TOP = ../..

include ${TOP}/make/config/prebase.mk

PROGRAM = wrfvdfcreate
FILES = wrfvdfcreate

#LIB_DEFS = vdf.def

ifdef PROJ_INC_PATH
MAKEFILE_INCLUDE_DIRS += -I$(PROJ_INC_PATH)
endif

ifdef PROJ_LIB_PATH
MAKEFILE_LDFLAGS += -L$(PROJ_LIB_PATH)
endif



LIBRARIES = vdf common netcdf proj $(EXPAT_LIB)

include ${TOP}/make/config/base.mk

