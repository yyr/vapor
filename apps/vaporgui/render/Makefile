TOP = ../../..

include ${TOP}/make/config/arch.mk
QT = 1
PROGRAM = vaporgui

COMPILE_ONLY = 1
ifdef	VOLUMIZER_ROOT
MAKEFILE_CXXFLAGS += -I$(VOLUMIZER_ROOT)/include
endif

MAKEFILE_INCLUDE_DIRS += -I../guis -I../misc -I../main -I../../../lib/params

ifdef   NETCDF_INC_PATH
MAKEFILE_CXXFLAGS += -I$(NETCDF_INC_PATH)
endif

ifdef   EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif
ifeq ($(BUILD_VOLUMIZER), 1)
MAKEFILE_CXXFLAGS += -DVOLUMIZER
endif


VPATH = ../guis ../misc ../main ../guis/ui
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

