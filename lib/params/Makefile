TOP = ../..

include ${TOP}/make/config/prebase.mk

QT = 1
UI_DIR = ui
LIBRARY = params
FILES = \
	datastatus dvrparams flowparams glutil \
	histo mapperfunction params probeparams regionparams \
	tfinterpolator transferfunction viewpoint twoDparams \
	twoDimageparams twoDdataparams \
	animationparams	viewpointparams ParamsBase Box \
	arrowparams animate\
	Colormap OpacityMap ParamNode ParamsIso ColorMapBase \
	MapperFunctionBase OpacityMapBase TransferFunctionLite \
	GeoTile GeoTileEquirectangular GeoTileMercator \
	pythonpipeline ModelParams ModelScene Transform3d

HEADER_FILES = ColorMapBase MapperFunctionBase OpacityMapBase \
	TransferFunctionLite tfinterpolator ParamsBase ParamNode
	
ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"PARAMS_EXPORTS"
endif


#ifeq ($(ARCH),Darwin)
#MAKEFILE_INCLUDE_DIRS += -I/System/Library/Frameworks/OpenGL.framework/Headers -I/System/Library/Frameworks/AGL.framework/Headers
#MAKEFILE_CXXFLAGS += -Wno-unused-variable -Wno-non-virtual-dtor
#endif


include ${TOP}/make/config/base.mk

