TOP = ../..

include ${TOP}/make/config/prebase.mk

QT = 1

PROGRAM = asciitf2vtf
FILES = asciitf2vtf


#LIB_DEFS = vdf.def

LIBRARIES = params flow vdf pthread dl util m python2.6 common netcdf tiff geotiff proj expat 

ifeq ($(QT_FRAMEWORK),1)
MAKEFILE_LDFLAGS += -F$(QTDIR)/lib -framework QtGui
else
LIBRARIES += QtGui 
endif

ifneq ($(ARCH),Darwin)
LIBRARIES += $(GLU_LIB) $(OGL_LIB)
endif

include ${TOP}/make/config/base.mk

ifeq ($(ARCH),Darwin)
MAKEFILE_LDFLAGS += -framework OpenGL -framework CoreFoundation -framework CoreServices
endif
