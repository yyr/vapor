
TOP = ../..

include ${TOP}/make/config/prebase.mk

QT = 1

PROGRAM = vdcwizard

MOC_DIR = ./moc

LIBRARIES += $(QT_LIB) vdf common proj udunits2 $(NETCDF_LIBS) expat
#common
#QDialog QtGui QtCore
#add all library #includes to this definintion?

ifeq ($(ARCH),Darwin)
#MAKEFILE_LDFLAGS = -framework QDialog -framework QtCore -framework QtGui
#MAKEFILE_LDFLAGS += -framework CoreFoundation -framework CoreServices
MAKEFILE_LDFLAGS += -framework Cocoa -framework AppKit
MAKEFILE_LDFLAGS += -framework ApplicationServices -framework OpenGL -framework AGL

#ifeq ($(QT_FRAMEWORK), 1)
MAKEFILE_LDFLAGS += -F$(QTDIR)/lib -framework QtCore -framework QtGui
MAKEFILE_LDFLAGS += -framework QtOpenGL
#endif
endif


HEADERS = \
popdataadvanced \
populatedatapage \
createvdfcomment \
createvdfaddnewvar \
createvdfadvanced \
createvdfpage \
selectfilepage \
vdfbadfile \
errormessage \
intropage \
vdcwizard \
dataholder

QT_HEADERS = \
vdcwizard \
intropage \
selectfilepage \
vdfbadfile \
errormessage \
createvdfpage \
createvdfaddnewvar \
createvdfadvanced \
createvdfcomment\
populatedatapage \
popdataadvanced 


FILES = \
vdcwizard \
createvdfadvanced \
createvdfaddnewvar \
createvdfcomment \
createvdfpage \
intropage \
main \
popdataadvanced \
populatedatapage \
selectfilepage \
vdfbadfile \
errormessage \
dataholder

UI_DIR = ./ui
UI_FILES = \
Page1 \
Page2 \
Page2badfile \
Page3 \
Page3addnewvar \
Page3adv \
Page3cmt \
Page4 \
Page4adv \
ErrMsg

include ${TOP}/make/config/base.mk
