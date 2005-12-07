TOP = ../../..

include ${TOP}/make/config/arch.mk
QT = 1
PROGRAM = vaporgui

COMPILE_ONLY = 1
ifdef	VOLUMIZER_ROOT
MAKEFILE_CXXFLAGS += -I$(VOLUMIZER_ROOT)/include
endif

MAKEFILE_INCLUDE_DIRS += -I../guis -I../params -I../main

ifdef   NETCDF_INC_PATH
MAKEFILE_CXXFLAGS += -I$(NETCDF_INC_PATH)
endif

ifdef   EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif



ifeq ($(ARCH),WIN32) 
MAKEFILE_CXXFLAGS += /DVOLUMIZER  
#MAKEFILE_LDFLAGS += "$(VOLUMIZER_ROOT)/lib/vz.lib"
#MAKEFILE_LDFLAGS += "$(QTDIR)/lib/qt-mt334.lib"
#MAKEFILE_LDFLAGS += "$(QTDIR)/lib/qtmain.lib"
#MAKEFILE_LDFLAGS += "opengl32.lib"
#MAKEFILE_LDFLAGS += "glu32.lib"
#MAKEFILE_LDFLAGS += "mpr.lib"
MAKEFILE_LDFLAGS += "/LIBPATH:$(VOLUMIZER_ROOT)/lib"
MAKEFILE_LDFLAGS += "/LIBPATH:$(QTDIR)/lib"

else   
MAKEFILE_CXXFLAGS += -DVOLUMIZER
MAKEFILE_LDFLAGS += -L$(QTDIR)/lib 
endif

VPATH = ../guis ../params ../main ../guis/ui
MOC_DIR = ../moc
UI_DIR = ../guis/ui

include render.mk
include $(UI_DIR)/ui.mk
HEADERS := 

FILES := $(RENDER_FILES)

QT_HEADERS := $(QT_RENDER_HEADERS) 


  
include ${TOP}/make/config/base.mk


cat:
	echo "QTDIR=$(QTDIR)"

