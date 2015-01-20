TOP = ../..

include ${TOP}/make/config/prebase.mk

LIBRARY = render

FILES = \
	visualizer renderer trackball\
	arrowrenderer manip \
	ControlExecutive \
	jfilewrite

ifdef FREETYPE2_INC_DIR
MAKEFILE_INCLUDE_DIRS += -I$(FREETYPE2_INC_DIR)
endif
ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += -D"RENDER_EXPORTS"
endif

ifeq ($(ARCH),Darwin)
MAKEFILE_CXXFLAGS += -Wno-unused-variable -Wno-non-virtual-dtor 
endif

MAKEFILE_INCLUDE_DIRS += -I../params 

include ${TOP}/make/config/base.mk
