TOP=../..

include ${TOP}/make/config/prebase.mk

PROGRAM = test_glflow
FILES = test_glflow glflow

ifeq ($(ARCH),Darwin)
#MAKEFILE_INCLUDE_DIRS += -I/System/Library/Frameworks/GLUT.framework/Versions/A/Headers
endif


ifeq ($(ARCH),Darwin)
#MAKEFILE_LDFLAGS += -framework OpenGL -framework GLUT
MAKEFILE_LDFLAGS += -framework OpenGL -framework glfw3 #maybe?
else
#LIBRARIES += $(GLU_LIB) $(OGL_LIB) glut common
LIBRARIES += GL GLU glfw3 X11 Xxf86vm Xrandr pthread Xi common
endif


include ${TOP}/make/config/base.mk

