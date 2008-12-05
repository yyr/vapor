TOP = ../..

include ${TOP}/make/config/prebase.mk

PROGRAM = asciitf2vtf
FILES = asciitf2vtf


#LIB_DEFS = vdf.def

LIBRARIES = params flow vdf common netcdf $(EXPAT_LIB) $(QT_LIB) 

ifneq ($(ARCH),Darwin)
LIBRARIES += $(GLU_LIB) $(OGL_LIB)
endif

include ${TOP}/make/config/base.mk

ifeq ($(ARCH),Darwin)
MAKEFILE_LDFLAGS += -framework OpenGL -framework CoreFoundation -framework CoreServices
endif
