TOP = ../..

include ${TOP}/make/config/prebase.mk

ifeq ($(ARCH),WIN32)
MAKEFILE_CFLAGS += /D"GLEW_BUILD" 
LIBRARIES += opengl32
LIBRARY = glew32
else
MAKEFILE_CFLAGS += -DGLEW_BUILD
LIBRARIES += GL
LIBRARY = GLEW
endif

ifeq ($(ARCH),Darwin)
MAKEFILE_LDFLAGS += -framework OpenGL  -L/usr/X11R6/lib
MAKEFILE_CFLAGS += -Wno-deprecated-declarations
endif

SHARED = 0
FILES = glew


include ${TOP}/make/config/base.mk

