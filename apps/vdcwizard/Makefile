
TOP = ../..

include ${TOP}/make/config/prebase.mk

QT = 1

PROGRAM = vdcwizard

MOC_DIR = ./moc

LIBRARIES += $(QT_LIB) vdf common proj udunits2 $(NETCDF_LIBS) expat

ifeq ($(ARCH),Darwin)
MAKEFILE_LDFLAGS += -framework Cocoa -framework AppKit
MAKEFILE_LDFLAGS += -framework ApplicationServices -framework AGL
MAKEFILE_LDFLAGS += -F$(QTDIR)/lib -framework QtCore -framework QtGui
endif

MAKEFILE_INCLUDE_DIRS += $(addprefix $(QTDIR)/include/, Qt QtCore QtGui)


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
CmdLine \
ErrMsg

include ${TOP}/make/config/base.mk
