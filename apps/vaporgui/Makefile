TOP = ../..

include ${TOP}/make/config/prebase.mk
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
ifdef QT_LIB_PATH
MAKEFILE_LDFLAGS += "/LIBPATH:$(QT_LIB_PATH)"
endif
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
ifdef	QT_LIB_PATH
MAKEFILE_LDFLAGS += -L$(QT_LIB_PATH)
endif
endif


ifeq ($(ARCH),Darwin)
MAKEFILE_INCLUDE_DIRS += -I/usr/X11R6/include
MAKEFILE_LDFLAGS += -framework OpenGL  -L/usr/X11R6/lib
MAKEFILE_CXXFLAGS += -Wno-unused-variable -Wno-non-virtual-dtor
endif   



VPATH = guis misc main 

QT_HEADERS := $(MISC_QT_HEADERS) $(MAIN_QT_HEADERS) $(GUI_QT_HEADERS)
FILES := $(MISC_FILES) $(GUI_FILES) $(MAIN_FILES) 

include ${TOP}/make/config/base.mk


cat:
	echo "QTDIR=$(QTDIR)"

