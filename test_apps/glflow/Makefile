TOP=../..

include ${TOP}/make/config/prebase.mk

PROGRAM = test_glflow
FILES = test_glflow glflow

ifeq ($(ARCH),Darwin)
MAKEFILE_INCLUDE_DIRS += -I/opt/local/include
endif


ifeq ($(ARCH),Darwin)
MAKEFILE_LDFLAGS += -framework OpenGL -L/opt/local/lib
LIBRARIES += glfw common
else
LIBRARIES += GL GLU glfw3 X11 Xxf86vm Xrandr pthread Xi common
endif


include ${TOP}/make/config/base.mk

