TOP = ../../..

include ${TOP}/make/config/prebase.mk
QT = 1

COMPILE_ONLY = 1
PROGRAM = vaporgui

ifdef	VOLUMIZER_ROOT
MAKEFILE_CXXFLAGS += -I$(VOLUMIZER_ROOT)/include
endif

MAKEFILE_INCLUDE_DIRS := -I../guis -I../misc -I.. -I../../../lib/params -I../../../lib/render

ifdef   NETCDF_INC_PATH
MAKEFILE_CXXFLAGS += -I$(NETCDF_INC_PATH)
endif

ifdef   EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif
ifeq ($(BUILD_VOLUMIZER), 1)
MAKEFILE_CXXFLAGS += -DVOLUMIZER
endif

ifdef   TIFF_INC_PATH
MAKEFILE_INCLUDE_DIRS += -I$(TIFF_INC_PATH)
endif

ifdef   GTIFF_INC_PATH
MAKEFILE_INCLUDE_DIRS += -I$(GTIFF_INC_PATH)
endif

ifdef   PROJ_INC_PATH
MAKEFILE_INCLUDE_DIRS += -I$(PROJ_INC_PATH)
endif





ifeq ($(ARCH),WIN32) 
MAKEFILE_LDFLAGS += "/LIBPATH:$(VOLUMIZER_ROOT)/lib"
ifdef	QT_LIB_PATH
MAKEFILE_LDFLAGS += "/LIBPATH:$(QT_LIB_PATH)"
endif

else   
ifdef	QT_LIB_PATH
MAKEFILE_LDFLAGS += -L$(QT_LIB_PATH)
endif
endif

ifeq ($(ARCH),Darwin)
MAKEFILE_INCLUDE_DIRS += -I/usr/X11R6/include
MAKEFILE_LDFLAGS += -framework OpenGL  -L/usr/X11R6/lib
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

