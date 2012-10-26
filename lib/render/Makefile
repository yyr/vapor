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
	manip proberenderer\
	arrowrenderer \
	trackball flowrenderer \
	VolumeRenderer BBox ShaderProgram TextureBrick \
	Vect3d Matrix3d Stopwatch DVRTexture3d \
	DVRShader \
	isorenderer GLModelNode \
	DVRSpherical DVRRayCaster \
	ModelRenderer \
	ShaderMgr jfilewrite

QT_HEADERS = \
	renderer \
	spintimer
  

ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += -D"RENDER_EXPORTS"
endif

ifeq ($(ARCH),Darwin)
MAKEFILE_INCLUDE_DIRS += -I/System/Library/Frameworks/OpenGL.framework/Headers -I/System/Library/Frameworks/AGL.framework/Headers
MAKEFILE_CXXFLAGS += -Wno-unused-variable -Wno-non-virtual-dtor 
endif

MAKEFILE_INCLUDE_DIRS += -I../params 

include ${TOP}/make/config/base.mk
