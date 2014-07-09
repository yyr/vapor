TOP = ../..

include ${TOP}/make/config/prebase.mk

QT = 1
LIBRARY = render
MOC_DIR = moc
UI_DIR = ui

FILES = \
	DVRBase DVRDebug \
	glwindow renderer twoDrenderer \
	twoDimagerenderer twoDdatarenderer \
	manip proberenderer isolinerenderer \
	arrowrenderer \
	trackball flowrenderer \
	VolumeRenderer BBox ShaderProgram TextureBrick \
	Vect3d Matrix3d Stopwatch DVRTexture3d \
	DVRShader \
	isorenderer GLModelNode \
	DVRSpherical DVRRayCaster \
	ModelRenderer \
	ShaderMgr jfilewrite \
	textRenderer

QT_HEADERS = \
	renderer \
	spintimer
  

LIBRARIES += ftgl freetype  
#MAKEFILE_INCLUDE_DIRS += -I/opt/local/include/freetype2
MAKEFILE_INCLUDE_DIRS += -I/glade/p/DASG/VAPOR/third-party/apps-2.3.0/Darwin_x86_64/include/freetype2

ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += -D"RENDER_EXPORTS"
endif

MAKEFILE_INCLUDE_DIRS += -I$(addprefix $(QTDIR)/include/, QtOpenGL)

ifeq ($(ARCH),Darwin)
MAKEFILE_CXXFLAGS += -Wno-unused-variable -Wno-non-virtual-dtor 
MAKEFILE_INCLUDE_DIRS += -I/glade/p/DASG/VAPOR/third-party/apps-2.3.0/Darwin_x86_64/include/freetype2
endif

MAKEFILE_INCLUDE_DIRS += -I../params 

include ${TOP}/make/config/base.mk
