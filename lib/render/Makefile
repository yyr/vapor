TOP = ../..

include ${TOP}/make/config/prebase.mk

QT = 1
LIBRARY = render
MOC_DIR = moc
UI_DIR = ui

FILES = \
	DVRBase DVRDebug \
	glwindow renderer twoDrenderer \
	customcontext \
	manip proberenderer\
	trackball flowrenderer isorenderer\
	VolumeRenderer BBox ShaderProgram TextureBrick \
	Vect3d Matrix3d Stopwatch DVRTexture3d \
	DVRShader DVRLookup DVRVolumizer DVRSpherical DVRRayCaster \
	DVRRayCaster2Var

QT_HEADERS = \
	renderer 
  
ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += -D"RENDER_EXPORTS"
endif

ifeq ($(ARCH),Darwin)
MAKEFILE_INCLUDE_DIRS += -I/usr/X11R6/include
MAKEFILE_CXXFLAGS += -Wno-unused-variable -Wno-non-virtual-dtor 
endif

MAKEFILE_INCLUDE_DIRS += -I../params -I$(PROJ_INC_PATH)
MAKEFILE_LDFLAGS += -L$(PROJ_LIB_PATH)

include ${TOP}/make/config/base.mk
