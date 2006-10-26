TOP = ../..

include ${TOP}/make/config/arch.mk

SHARED = 0
QT = 1
UI_DIR = ui
LIBRARY = params
FILES = \
	datastatus dvrparams flowparams glutil \
	histo mapperfunction params probeparams regionparams \
	tfinterpolator transferfunction viewpoint \
	animationparams	viewpointparams \
	Colormap OpacityMap

ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"PARAMS_EXPORTS"
endif

ifeq ($(ARCH),Darwin)
MAKEFILE_INCLUDE_DIRS += -I/usr/X11R6/include
MAKEFILE_LDFLAGS += -framework OpenGL -L/usr/X11R6/lib
MAKEFILE_CXXFLAGS += -Wno-unused-variable -Wno-non-virtual-dtor
endif


ifdef   EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH) -I$(NETCDF_INC_PATH)
endif

LIBRARIES = common vdf flow

ifeq ($(ARCH),WIN32)
LIBRARIES += qt-mt334 
LIBRARIES += glu32
LIBRARIES += opengl32
MAKEFILE_LDFLAGS += "/LIBPATH:$(QTDIR)\lib"
else

LIBRARIES += qt-mt
LIBRARIES += GL
LIBRARIES += GLU
MAKEFILE_LDFLAGS += -L$(QTDIR)/lib
endif

include ${TOP}/make/config/base.mk

