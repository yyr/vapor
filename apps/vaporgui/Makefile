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

MAKEFILE_INCLUDE_DIRS += -Iguis -Imain -Imisc -I../../lib/params -I../../lib/render


LIBRARIES = dl util m render params flow jpeg vdf common udunits2 
LIBRARIES += $(NETCDF_LIBS) expat $(QT_LIB) 
ifneq ($(QT_FRAMEWORK), 1)
LIBRARIES += QtOpenGL 
endif
LIBRARIES += GLEW tiff geotiff proj python$(PYTHONVERSION)

ifeq ($(BUILD_MODELS),1)
LIBRARIES += assimp
endif

ifneq ($(ARCH),Darwin)
LIBRARIES += $(GLU_LIB) $(OGL_LIB)
MAKEFILE_LDFLAGS += -Xlinker -export-dynamic
endif


ifeq ($(ARCH),Darwin)

MAKEFILE_LDFLAGS += -framework CoreFoundation -framework CoreServices 
MAKEFILE_LDFLAGS += -framework Cocoa -framework AppKit -framework 
MAKEFILE_LDFLAGS += ApplicationServices -framework OpenGL -framework AGL
MAKEFILE_LDFLAGS += -u _PyMac_Error 

ifeq ($(QT_FRAMEWORK), 1)
MAKEFILE_LDFLAGS += -F$(QTDIR)/lib -framework QtCore -framework QtGui 
MAKEFILE_LDFLAGS += -framework QtOpenGL -framework QtNetwork
endif

MAKEFILE_CXXFLAGS += -Wno-unused-variable -Wno-non-virtual-dtor

endif   



VPATH = guis misc main 

QT_HEADERS := $(MISC_QT_HEADERS) $(MAIN_QT_HEADERS) $(GUI_QT_HEADERS)
FILES := $(MISC_FILES) $(GUI_FILES) $(MAIN_FILES) 

include ${TOP}/make/config/base.mk
