TOP = ../../..

include ${TOP}/make/config/prebase.mk
QT = 1

COMPILE_ONLY = 1
PROGRAM = vaporgui

MAKEFILE_INCLUDE_DIRS := -I../guis -I../misc -I.. -I../../../lib/params -I../../../lib/render

ifneq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += -DPYTHONVERSION=\"$(PYTHONVERSION)\"
MAKEFILE_CXXFLAGS += -DPYTHONDIR=\"$(PYTHONDIR)\"
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

