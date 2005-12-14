TOP = ../..

include ${TOP}/make/config/arch.mk
include params/params.mk
include guis/guis.mk
include render/render.mk
include main/main.mk
MOC_DIR = moc
UI_DIR = guis/ui
include $(UI_DIR)/ui.mk

SHARED = 0
QT = 1

SUBDIRS = guis params render main

PROGRAM	= vaporgui
export PROGRAM

ifdef	VOLUMIZER_ROOT
MAKEFILE_CXXFLAGS += -I$(VOLUMIZER_ROOT)/include
endif

MAKEFILE_INCLUDE_DIRS := -Irender -Iguis -Imain -Iparams

ifdef   NETCDF_INC_PATH
MAKEFILE_CXXFLAGS += -I$(NETCDF_INC_PATH)
endif

ifdef   EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

LIBRARIES = vdf common vjpeg flow vz 

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
LIBRARIES += mpr
LIBRARIES += qt-mt334
LIBRARIES += qtmain
LIBRARIES += opengl32
LIBRARIES += glu32

else   
LIBRARIES += qt-mt 
LIBRARIES += GL
LIBRARIES += GLU
MAKEFILE_CXXFLAGS += -DVOLUMIZER
MAKEFILE_LDFLAGS += -L$(QTDIR)/lib 
endif

VPATH = guis params render main 

QT_HEADERS := $(PARAMS_QT_HEADERS) $(MAIN_QT_HEADERS) $(RENDER_QT_HEADERS) $(GUI_QT_HEADERS)
FILES := $(PARAMS_FILES) $(GUI_FILES) $(MAIN_FILES) $(RENDER_FILES)

include ${TOP}/make/config/base.mk
#MAKEFILE_LDFLAGS += $(QTLIB)

#testtarget:;echo $(FILES)

cat:
	echo "QTDIR=$(QTDIR)"

