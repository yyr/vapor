TOP = ../../..

include ${TOP}/make/config/arch.mk
QT = 1

COMPILE_ONLY = 1
PROGRAM = vaporgui

ifdef	VOLUMIZER_ROOT
MAKEFILE_CXXFLAGS += -I$(VOLUMIZER_ROOT)/include
endif

MAKEFILE_INCLUDE_DIRS := -I../guis -I../render -I../misc -I.. -I../../../lib/params

ifdef   NETCDF_INC_PATH
MAKEFILE_CXXFLAGS += -I$(NETCDF_INC_PATH)
endif

ifdef   EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif
ifeq ($(BUILD_VOLUMIZER), 1)
MAKEFILE_CXXFLAGS += -DVOLUMIZER
endif




ifeq ($(ARCH),WIN32) 
MAKEFILE_LDFLAGS += "/LIBPATH:$(VOLUMIZER_ROOT)/lib"
MAKEFILE_LDFLAGS += "/LIBPATH:$(QTDIR)/lib"

else   
MAKEFILE_LDFLAGS += -L$(QTDIR)/lib 
endif

VPATH = ../misc ../guis ../render 

MOC_DIR = ../moc
UI_DIR = ../guis/ui

include main.mk
include $(UI_DIR)/ui.mk

FILES := $(MAIN_FILES)

QT_HEADERS := $(QT_MAIN_HEADERS) 


  
include ${TOP}/make/config/base.mk


cat:
	echo "QTDIR=$(QTDIR)"

