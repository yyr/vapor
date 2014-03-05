TOP = ../..

include ${TOP}/make/config/prebase.mk

LIBRARY = render

FILES = \
	visualizer renderer \
	arrowrenderer \
	ControlExecutive \
	jfilewrite


ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += -D"RENDER_EXPORTS"
endif

ifeq ($(ARCH),Darwin)
MAKEFILE_CXXFLAGS += -Wno-unused-variable -Wno-non-virtual-dtor 
endif

MAKEFILE_INCLUDE_DIRS += -I../params 

include ${TOP}/make/config/base.mk
