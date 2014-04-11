TOP = ../..

include ${TOP}/make/config/prebase.mk

LIBRARY = params
FILES = \
	datastatus glutil \
	params regionparams arrowparams mousemodeparams \
	viewpoint  \
	animationparams	viewpointparams ParamsBase Box \
	ParamNode command
	
HEADER_FILES = ParamsBase ParamNode
	
ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"PARAMS_EXPORTS"
endif


#ifeq ($(ARCH),Darwin)
#MAKEFILE_INCLUDE_DIRS += -I/System/Library/Frameworks/OpenGL.framework/Headers -I/System/Library/Frameworks/AGL.framework/Headers
#MAKEFILE_CXXFLAGS += -Wno-unused-variable -Wno-non-virtual-dtor
#endif


include ${TOP}/make/config/base.mk

