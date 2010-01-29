TOP = ../../..

include ${TOP}/make/config/prebase.mk
QT = 1
PROGRAM = vaporgui
COMPILE_ONLY = 1

MAKEFILE_INCLUDE_DIRS += -I../guis -I../main -I../../../lib/params -I../../../lib/render 

MAKEFILE_CXXFLAGS += -DBINDIR=\"$(BINDIR)\"
MAKEFILE_CXXFLAGS += -DDSO_DIR=\"$(DSO_DIR)\"
MAKEFILE_CXXFLAGS += -DABS_TOP=\"$(ABS_TOP)\"
MAKEFILE_CXXFLAGS += -DQTDIR=\"$(QTDIR)\"


ifeq ($(ARCH),WIN32) 
MAKEFILE_LDFLAGS += "/LIBPATH:$(VOLUMIZER_ROOT)/lib"
endif

ifeq ($(ARCH),Darwin)
MAKEFILE_INCLUDE_DIRS += -I/System/Library/Frameworks/OpenGL.framework/Headers -I/System/Library/Frameworks/AGL.framework/Headers
MAKEFILE_LDFLAGS += -framework CoreFoundation -framework CoreServices -framework Carbon -framework AppKit -framework ApplicationServices -framework OpenGL -framework AGL
MAKEFILE_CXXFLAGS += -Wno-unused-variable -Wno-non-virtual-dtor
endif

VPATH = ../guis ../main

MOC_DIR = ../moc
UI_DIR = ../guis/ui
include misc.mk
include ../guis/ui/ui.mk

HEADERS := 

FILES := $(MISC_FILES)

QT_HEADERS := $(QT_MISC_HEADERS) 

  
include ${TOP}/make/config/base.mk


#testtarget:;echo $(FILES)

cat:
	echo "QTDIR=$(QTDIR)"

