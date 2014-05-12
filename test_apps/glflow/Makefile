TOP=../..

include ${TOP}/make/config/prebase.mk

PROGRAM = test_glflow
FILES = test_glflow glflow

ifeq ($(ARCH),Darwin)
MAKEFILE_INCLUDE_DIRS += -I/usr/local/include
endif


ifeq ($(ARCH),Darwin)
MAKEFILE_LDFLAGS += -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo -L/opt/local/lib
LIBRARIES += glfw3 common
else
LIBRARIES += GL GLU glfw3 X11 Xxf86vm Xrandr pthread Xi common
endif


include ${TOP}/make/config/base.mk

