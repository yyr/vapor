TOP = ../../..

include ${TOP}/make/config/prebase.mk
QT = 1

COMPILE_ONLY = 1
PROGRAM = vaporgui

ifeq ($(ASSIMP_2),1)
MAKEFILE_CXXFLAGS += -DASSIMP_2
endif

MAKEFILE_INCLUDE_DIRS +=  -I../main -I../misc -I../../../lib/params -I../../../lib/render

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

