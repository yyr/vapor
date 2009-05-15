TOP = ../..

include ${TOP}/make/config/prebase.mk


PROGRAM = tiff2geotiff
FILES = tiff2geotiff geotiff_proj4 getopt

ifdef   TIFF_INC_PATH
MAKEFILE_INCLUDE_DIRS += -I$(TIFF_INC_PATH)
endif

ifdef   GTIFF_INC_PATH
MAKEFILE_INCLUDE_DIRS += -I$(GTIFF_INC_PATH)
endif

ifdef   PROJ_INC_PATH
MAKEFILE_INCLUDE_DIRS += -I$(PROJ_INC_PATH)
endif

  
ifdef TIFF_LIB_PATH
MAKEFILE_LDFLAGS += -L$(TIFF_LIB_PATH)
endif

ifdef GTIFF_LIB_PATH
MAKEFILE_LDFLAGS += -L$(GTIFF_LIB_PATH)
endif

ifdef PROJ_LIB_PATH
MAKEFILE_LDFLAGS += -L$(PROJ_LIB_PATH)
endif

LIBRARIES = tiff geotiff proj

include ${TOP}/make/config/base.mk
