TOP = ../..

include ${TOP}/make/config/prebase.mk

PROGRAM = asciitf2vtf
FILES = asciitf2vtf


#LIB_DEFS = vdf.def

LIBRARIES = params flow vdf common netcdf tiff geotiff proj $(EXPAT_LIB) $(QT_LIB) 

ifneq ($(ARCH),Darwin)
LIBRARIES += $(GLU_LIB) $(OGL_LIB)
endif

ifdef   QT_LIB_PATH
MAKEFILE_LDFLAGS += -L$(QT_LIB_PATH)
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


include ${TOP}/make/config/base.mk

ifeq ($(ARCH),Darwin)
MAKEFILE_LDFLAGS += -framework OpenGL -framework CoreFoundation -framework CoreServices
endif
