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
ifeq ($(BUILD_VOLUMIZER), 1)
MAKEFILE_CXXFLAGS += -DVOLUMIZER
endif


MAKEFILE_INCLUDE_DIRS := -I../guis -I../main -I../../../lib/params -I../../../lib/render 


ifeq ($(ARCH),WIN32) 
MAKEFILE_LDFLAGS += "/LIBPATH:$(VOLUMIZER_ROOT)/lib"
MAKEFILE_LDFLAGS += "/LIBPATH:$(QTDIR)/lib"

else   
MAKEFILE_LDFLAGS += -L$(QTDIR)/lib 
endif

ifeq ($(ARCH),Darwin)
MAKEFILE_INCLUDE_DIRS += -I/usr/X11R6/include
MAKEFILE_LDFLAGS += -framework OpenGL  -L/usr/X11R6/lib
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

#MAKEFILE_LDFLAGS += $(QTLIB)

#testtarget:;echo $(FILES)

cat:
	echo "QTDIR=$(QTDIR)"

