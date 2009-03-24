TOP = ../..

include ${TOP}/make/config/prebase.mk


PROGRAM = tiff2geotiff
FILES = tiff2geotiff getopt
  
ifdef TIFF_LIB_PATH
MAKEFILE_LDFLAGS += -L$(TIFF_LIB_PATH)
endif
ifdef GTIFF_LIB_PATH
MAKEFILE_LDFLAGS += -L$(GTIFF_LIB_PATH)
endif

LIBRARIES = tiff geotiff

include ${TOP}/make/config/base.mk
