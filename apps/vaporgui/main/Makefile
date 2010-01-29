TOP = ../../..

include ${TOP}/make/config/prebase.mk
QT = 1

COMPILE_ONLY = 1
PROGRAM = vaporgui

MAKEFILE_INCLUDE_DIRS := -I../guis -I../misc -I.. -I../../../lib/params -I../../../lib/render


ifeq ($(ARCH),WIN32) 
MAKEFILE_LDFLAGS += "/LIBPATH:$(VOLUMIZER_ROOT)/lib"
endif

ifeq ($(ARCH),Darwin)
MAKEFILE_INCLUDE_DIRS += -I/System/Library/Frameworks/OpenGL.framework/Headers -I/System/Library/Frameworks/AGL.framework/Headers
MAKEFILE_LDFLAGS += -framework CoreFoundation -framework CoreServices -framework Carbon -framework AppKit -framework ApplicationServices -framework OpenGL -gramework AGL
MAKEFILE_CXXFLAGS += -Wno-unused-variable -Wno-non-virtual-dtor
endif

VPATH = ../misc ../guis 

MOC_DIR = ../moc
UI_DIR = ../guis/ui

include main.mk
include $(UI_DIR)/ui.mk

FILES := $(MAIN_FILES)

QT_HEADERS := $(QT_MAIN_HEADERS) 


  
include ${TOP}/make/config/base.mk


cat:
	echo "QTDIR=$(QTDIR)"

