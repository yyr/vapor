TOP = ../../..

include ${TOP}/make/config/prebase.mk
QT = 1

COMPILE_ONLY = 1
PROGRAM = vaporgui

ifdef	VOLUMIZER_ROOT
MAKEFILE_CXXFLAGS += -I$(VOLUMIZER_ROOT)/include
endif

MAKEFILE_INCLUDE_DIRS :=  -I../main -I../misc -I../../../lib/params -I../../../lib/render

ifeq ($(ARCH),WIN32) 
MAKEFILE_LDFLAGS += "/LIBPATH:$(VOLUMIZER_ROOT)/lib"
endif

ifeq ($(ARCH),Darwin)
MAKEFILE_INCLUDE_DIRS += -I/System/Library/Frameworks/OpenGL.framework/Headers -I/System/Library/Frameworks/AGL.framework/Headers
MAKEFILE_LDFLAGS += -framework CoreFoundation -framework CoreServices -framework Carbon -framework AppKit -framework ApplicationServices -framework OpenGL -framework AGL
MAKEFILE_CXXFLAGS += -Wno-unused-variable -Wno-non-virtual-dtor
endif   


VPATH = ../misc ../main ui

include guis.mk
include ui/ui.mk

HEADERS := 

FILES := $(GUI_FILES)

QT_HEADERS := $(QT_GUI_HEADERS) 

MOC_DIR = ../moc
UI_DIR = ./ui

  
include ${TOP}/make/config/base.mk


cat:
	echo "QTDIR=$(QTDIR)"

