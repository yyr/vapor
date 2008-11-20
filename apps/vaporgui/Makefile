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

LIBRARIES = vjpeg flow params render vdf common netcdf $(EXPAT_LIB)
LIBRARIES += $(EXPAT_LIB) $(QT_LIB) GLEW $(OGL_LIB) $(GLU_LIB)

ifeq ($(BUILD_VOLUMIZER), 1)
LIBRARIES += vz
endif

ifeq ($(ARCH),WIN32) 

ifeq ($(BUILD_VOLUMIZER), 1)
MAKEFILE_LDFLAGS += "/LIBPATH:$(VOLUMIZER_ROOT)/lib"
endif

ifdef QT_LIB_PATH
MAKEFILE_LDFLAGS += "/LIBPATH:$(QT_LIB_PATH)"
endif

else   

ifdef	QT_LIB_PATH
MAKEFILE_LDFLAGS += -L$(QT_LIB_PATH)
endif

endif


ifeq ($(ARCH),Darwin)
MAKEFILE_INCLUDE_DIRS += -I/usr/X11R6/include
MAKEFILE_LDFLAGS += -framework OpenGL  -L/usr/X11R6/lib -framework CoreFoundation -framework CoreServices
MAKEFILE_CXXFLAGS += -Wno-unused-variable -Wno-non-virtual-dtor
endif   



VPATH = guis misc main 

QT_HEADERS := $(MISC_QT_HEADERS) $(MAIN_QT_HEADERS) $(GUI_QT_HEADERS)
FILES := $(MISC_FILES) $(GUI_FILES) $(MAIN_FILES) 

include ${TOP}/make/config/base.mk


cat:
	echo "QTDIR=$(QTDIR)"

