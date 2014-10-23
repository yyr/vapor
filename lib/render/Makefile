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

ifdef FREETYPE2_INC_DIR
MAKEFILE_INCLUDE_DIRS += -I$(FREETYPE2_INC_DIR)
endif

ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += -D"RENDER_EXPORTS"
endif

ifeq ($(ASSIMP_2),1)
MAKEFILE_CXXFLAGS += -DASSIMP_2
endif

ifneq ($(QT_FRAMEWORK), 1)
MAKEFILE_INCLUDE_DIRS += -I$(addprefix $(QTDIR)/include/, QtOpenGL)
else
QT_INCLUDE_DIRS += $(addprefix $(QTDIR)/lib/, $(addsuffix .framework/Headers,QtOpenGL))
endif

MAKEFILE_INCLUDE_DIRS += -I../params 

include ${TOP}/make/config/base.mk
