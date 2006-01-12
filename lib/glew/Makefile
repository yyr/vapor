TOP = ../..

include ${TOP}/make/config/arch.mk

ifeq ($(ARCH),WIN32)
MAKEFILE_CFLAGS += /D"GLEW_BUILD" 
LIBRARIES += opengl32
else
MAKEFILE_CFLAGS += -DGLEW_BUILD
LIBRARIES += GL
endif

SHARED = 0
LIBRARY = GLEW
FILES = glew


include ${TOP}/make/config/base.mk

