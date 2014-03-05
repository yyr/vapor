TOP = ../../..

include ${TOP}/make/config/prebase.mk
QT = 1
PROGRAM = vaporgui
COMPILE_ONLY = 1

MAKEFILE_INCLUDE_DIRS += -I../guis -I../main -I../../../lib/params -I../../../lib/render 
MAKEFILE_INCLUDE_DIRS += -I$(PYTHONDIR)/Include
MAKEFILE_INCLUDE_DIRS += -I$(PYTHONDIR)/
MAKEFILE_INCLUDE_DIRS += -I$(PYTHONDIR)/site-packages/numpy/core/include/numpy

MAKEFILE_CXXFLAGS += -DBINDIR=\"$(BINDIR)\"
MAKEFILE_CXXFLAGS += -DDSO_DIR=\"$(DSO_DIR)\"
MAKEFILE_CXXFLAGS += -DABS_TOP=\"$(ABS_TOP)\"
MAKEFILE_CXXFLAGS += -DQTDIR=\"$(QTDIR)\"


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

