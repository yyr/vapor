TOP = ../..

include ${TOP}/make/config/prebase.mk

SHARED = 0
QT = 1
LIBRARY = render
MOC_DIR = moc
UI_DIR = ui

FILES = \
	DVRBase DVRDebug \
	glwindow renderer \
	manip proberenderer\
	trackball flowrenderer isorenderer\
	VolumeRenderer BBox ShaderProgram TextureBrick \
	Vect3d Matrix3d Stopwatch DVRTexture3d \
	DVRShader DVRLookup DVRVolumizer DVRSpherical

QT_HEADERS = \
	renderer 
  
ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += -D"RENDER_EXPORTS"
endif

ifeq ($(ARCH),Darwin)
MAKEFILE_INCLUDE_DIRS += -I/usr/X11R6/include
MAKEFILE_LDFLAGS += -framework OpenGL  -L/usr/X11R6/lib
MAKEFILE_CXXFLAGS += -Wno-unused-variable -Wno-non-virtual-dtor 
endif

MAKEFILE_CXXFLAGS += -I../params

ifdef   EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

ifdef   NETCDF_INC_PATH
MAKEFILE_CXXFLAGS += -I$(NETCDF_INC_PATH)
endif

LIBRARIES = common vdf flow params vjpeg

ifeq ($(ARCH),WIN32)
LIBRARIES += qt-mt334 
LIBRARIES += glu32
LIBRARIES += opengl32
LIBRARIES += glew32
MAKEFILE_LDFLAGS += "/LIBPATH:$(VOLUMIZER_ROOT)/lib"
MAKEFILE_LDFLAGS += "/LIBPATH:$(QT_LIB_PATH)"
else

LIBRARIES += qt-mt
LIBRARIES += GL
LIBRARIES += GLU
LIBRARIES += GLEW
ifdef	QT_LIB_PATH
MAKEFILE_LDFLAGS += -L$(QT_LIB_PATH)
endif
MAKEFILE_CXXFLAGS += -I$(VOLUMIZER_ROOT)/include
endif

ifeq ($(BUILD_VOLUMIZER), 1)
LIBRARIES += vz 
endif

include ${TOP}/make/config/base.mk
