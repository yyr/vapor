TOP = ../..

include ${TOP}/make/config/arch.mk
include misc/misc.mk
include guis/guis.mk
include main/main.mk

MOC_DIR = moc
UI_DIR = guis/ui
include $(UI_DIR)/ui.mk

SHARED = 0
QT = 1

SUBDIRS = guis misc main

PROGRAM	= vaporgui
export PROGRAM

ifdef	VOLUMIZER_ROOT
MAKEFILE_CXXFLAGS += -I$(VOLUMIZER_ROOT)/include
endif

MAKEFILE_INCLUDE_DIRS := -Iguis -Imain -Imisc -I../../lib/params -I../../lib/render

ifdef   NETCDF_INC_PATH
MAKEFILE_CXXFLAGS += -I$(NETCDF_INC_PATH)
endif

ifdef   EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

LIBRARIES = vdf common vjpeg flow params render

ifeq ($(BUILD_VOLUMIZER), 1)
LIBRARIES += vz 
endif

ifeq ($(ARCH),WIN32) 
MAKEFILE_LDFLAGS += "/LIBPATH:$(VOLUMIZER_ROOT)/lib"
MAKEFILE_LDFLAGS += "/LIBPATH:$(QTDIR)/lib"
LIBRARIES += mpr
LIBRARIES += qt-mt334
LIBRARIES += qtmain
LIBRARIES += opengl32
LIBRARIES += glu32
LIBRARIES += glew32

else   
LIBRARIES += qt-mt 
LIBRARIES += GLEW
LIBRARIES += GL
LIBRARIES += GLU
MAKEFILE_LDFLAGS += -L$(QTDIR)/lib 
endif

VPATH = guis misc main 

QT_HEADERS := $(MISC_QT_HEADERS) $(MAIN_QT_HEADERS) $(GUI_QT_HEADERS)
FILES := $(MISC_FILES) $(GUI_FILES) $(MAIN_FILES) 

include ${TOP}/make/config/base.mk


cat:
	echo "QTDIR=$(QTDIR)"

