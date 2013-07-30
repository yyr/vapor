# Important Definitions:
# MOC_DIR      ok
# LIBRARIES    ?
# MAKEFILE_LDFLAGS ?
# HEADERS      ?
# QT_HEADERS   ok
# FILES                ok
# UI_DIR       ok
# UI_FILES     ok
# MAKEFILE_INCLUDE_DIRS (maybe)

#/Users/pearse/Documents/Vapor2.2/git/apps/vaporgui/Makefile has following vars:
# MOC_DIR - contains QT moc files
# UI_DIR - contains QT .ui files
#  SHARED
#  QT
#  SUBDIRS
#  PROGRAM
#  MAKEFILE_INCLUDE_DIRS
# LIBRARIES
# MAKEFILE_LDFLAGS
#  MAKEFILE_CXXFLAGS
# QT_HEADERS - points to a list "GUI_QT_HEADERS", contained in /guis/guis.mk
# FILES - points to a list "GUI_FILES", contained in /guis/guis.mk

#/Users/pearse/Documents/Vapor2.2/git/apps/vaporgui/Makefile does not have:
# UI_FILES - list of .ui files, defined in /Users/pearse/Documents/Vapor2.2/git/apps/vaporgui/guis/ui/ui.mk

#/Users/pearse/Documents/Vapor2.2/git/apps/vaporgui/guis/guis.mk contains:
# GUI_FILES
# GUI_QT_HEADERS

####################

TOP = ../..

include ${TOP}/make/config/prebase.mk

QT = 1

PROGRAM = fileconverterwizard

MOC_DIR = ./moc

LIBRARIES += $(QT_LIB) common 
LIBRARIES += vdf proj common udunits2 $(NETCDF_LIBS) expat
#QDialog QtGui QtCore
#add all library #includes to this definintion?

#MAKEFILE_LDFLAGS = -framework QDialog -framework QtCore -framework QtGui
MAKEFILE_LDFLAGS += -framework CoreFoundation -framework CoreServices
MAKEFILE_LDFLAGS += -framework Cocoa -framework AppKit -framework
MAKEFILE_LDFLAGS += ApplicationServices -framework OpenGL -framework AGL

#ifeq ($(QT_FRAMEWORK), 1)
MAKEFILE_LDFLAGS += -F$(QTDIR)/lib -framework QtCore -framework QtGui
MAKEFILE_LDFLAGS += -framework QtOpenGL
#endif


HEADERS = \
popdataadvanced \
populatedatapage \
createvdfcomment \
createvdfadvanced \
createvdfpage \
selectfilepage \
intropage \
fcwizard \
showtimeconflicts

QT_HEADERS = \
fcwizard \
intropage \
selectfilepage \
createvdfpage \
createvdfadvanced \
createvdfcomment\
populatedatapage \
popdataadvanced \
showtimeconflicts


FILES = \
fcwizard \
createvdfadvanced \
createvdfcomment \
createvdfpage \
intropage \
main \
popdataadvanced \
populatedatapage \
selectfilepage \
showtimeconflicts

UI_DIR = ./ui
UI_FILES = \
Page1 \
Page2 \
Page3 \
Page3adv \
Page3cmt \
Page4 \
Page4adv \
Page4conf 

include ${TOP}/make/config/base.mk
