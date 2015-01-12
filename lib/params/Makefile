TOP = ../..

include ${TOP}/make/config/prebase.mk

LIBRARY = params
FILES = \
	datastatus glutil \
	params regionparams arrowparams mousemodeparams \
	renderparams viewpoint  vizwinparams instanceparams \
	animationparams	viewpointparams ParamsBase Box \
	OpacityMap ColorMap OpacityMapBase ColorMapBase ParamNode command \
	mapperfunction MapperFunctionBase transferfunction histo \
	tfinterpolator
	  
	
HEADER_FILES = ParamsBase ParamNode MapperFunctionBase ColorMapBase \
	OpacityMapBase tfinterpolator
	
ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"PARAMS_EXPORTS"
endif


#ifeq ($(ARCH),Darwin)
#MAKEFILE_INCLUDE_DIRS += -I/System/Library/Frameworks/OpenGL.framework/Headers -I/System/Library/Frameworks/AGL.framework/Headers
#MAKEFILE_CXXFLAGS += -Wno-unused-variable -Wno-non-virtual-dtor
#endif


include ${TOP}/make/config/base.mk

