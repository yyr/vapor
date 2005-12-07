TOP = ../../..

include ${TOP}/make/config/arch.mk
QT = 1
PROGRAM = vaporgui
COMPILE_ONLY = 1
ifdef	VOLUMIZER_ROOT
MAKEFILE_CXXFLAGS += -I$(VOLUMIZER_ROOT)/include
endif


ifdef   NETCDF_INC_PATH
MAKEFILE_CXXFLAGS += -I$(NETCDF_INC_PATH)
endif

ifdef   EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

MAKEFILE_INCLUDE_DIRS := -I../guis -I../render -I../main 


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

VPATH = ../guis ../render ../main

MOC_DIR = ../moc
UI_DIR = ../guis/ui
include params.mk
include ../guis/ui/ui.mk

HEADERS := 

FILES := $(PARAMS_FILES)

QT_HEADERS := $(QT_PARAMS_HEADERS) 

  
include ${TOP}/make/config/base.mk

#MAKEFILE_LDFLAGS += $(QTLIB)

#testtarget:;echo $(FILES)

cat:
	echo "QTDIR=$(QTDIR)"

