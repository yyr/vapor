TOP = ../..

include ${TOP}/make/config/prebase.mk

ifeq ($(ARCH),WIN32)
MAKEFILE_CFLAGS += /D"GLEW_BUILD" 
LIBRARY = glew32
else
MAKEFILE_CFLAGS += -DGLEW_BUILD
LIBRARY = GLEW
endif

ifeq ($(ARCH),Darwin)
MAKEFILE_CFLAGS += -Wno-deprecated-declarations
endif

FILES = glew


include ${TOP}/make/config/base.mk

