# ------------------------------------------- 
# Subdir relative project main directory: ./src/apps/gui
# Target is an application:  ../../platform/bin/vaporgui
# This is not used for building Makefiles, only used for
# building vcproj.  All relative paths for linking files are
# relative to the location of the vcproj file, which is 
# located at ../../make/win32/vcproj/

TEMPLATE = app

win32:TARGET = ../../../targets/win32/bin/vaporgui

win32:OBJECTS_DIR = ../../../targets/win32/built/vaporgui/


win32:LIBS += ../../../targets/win32/bin/vdf.lib
win32:LIBS += ../../../targets/win32/bin/common.lib

CONFIG += opengl 
CONFIG += debug
	
CONFIG -= dlopen_opengl
REQUIRES = opengl

win32:CONFIG += thread

win32:LIBS += $(VOLUMIZER_ROOT)/lib/vz.lib
win32:INCLUDEPATH += $(VOLUMIZER_ROOT)/include
win32:INCLUDEPATH += "B:/Expat-1.95.8/Source/lib"
win32:QMAKE_CXXFLAGS_DEBUG += /EHsc
win32:QMAKE_CXXFLAGS_RELEASE += /EHsc
DEFINES += VOLUMIZER


INCLUDEPATH += ./ui \
	../../include

MOC_DIR = ../../../apps/vaporgui/moc

SOURCES +=\ 
	   coloradjustdialog.cpp \
	   colorpicker.cpp \
	   command.cpp \
	   contourparams.cpp \
	   DVRBase.cpp \
	   DVRDebug.cpp \
	   DVRVolumizer.cpp \
	   dvrparams.cpp \
	   glbox.cpp \
	   glutil.cpp \
	   glwindow.cpp \
	   histo.cpp \
	   isosurfaceparams.cpp \
	   main.cpp \
           mainform.cpp \
           minmaxcombo.cpp \
	   opacadjustdialog.cpp \
	   panelcommand.cpp \
	   params.cpp \
	   regionparams.cpp \
	   renderer.cpp \
	   session.cpp \
           tabmanager.cpp \
	   tfeditor.cpp \
	   tfelocationtip.cpp \
	   tfframe.cpp \
	   tfinterpolator.cpp \
	   trackball.cpp \
	   transferfunction.cpp \
	   vcr.cpp \
	   viewpointparams.cpp \
	   vizactivatecommand.cpp \
           vizmgrdialog.cpp \
           viznameedit.cpp \
	   vizselectcombo.cpp \
           vizwin.cpp \
           vizwinmgr.cpp \
	   volumizerrenderer.cpp 
	   
HEADERS += \
	   coloradjustdialog.h \
	   colorpicker.h \
	   command.h \
	   contourparams.h \
	   DVRBase.h \
	   DVRDebug.h \
	   DVRVolumizer.h \
	   dvrparams.h \
	   glbox.h \
	   glwindow.h \
	   glutil.h \
	   histo.h \
	   isosurfaceparams.h \
           mainform.h \
           minmaxcombo.h \
	   opacadjustdialog.h \
	   panelcommand.h \
	   params.h \
	   regionparams.h \
	   renderer.h \
	   session.h \
           tabmanager.h \
	   tfeditor.h \
           tfelocationtip.h \
	   tfinterpolator.h \
	   tfframe.h \
	   trackball.h \
	   transferfunction.h \
	   vcr.h \
	   viewpointparams.h \
	   vizactivatecommand.h \
           vizmgrdialog.h \
           viznameedit.h \
	   vizselectcombo.h \
	   vizwin.h \
           vizwinmgr.h \
	   volumizerrenderer.h 
FORMS += ./ui/viztab.ui \
	 ./ui/dvr.ui \
         ./ui/regiontab.ui \ 
	 ./ui/contourplanetab.ui \
       	 ./ui/isotab.ui \
	 ./ui/sessionparameters.ui

