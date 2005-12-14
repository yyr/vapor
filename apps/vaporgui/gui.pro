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
win32:INCLUDEPATH += "C:/Expat-1.95.8/Source/lib"
win32:QMAKE_CXXFLAGS_DEBUG += /EHsc
win32:QMAKE_CXXFLAGS_RELEASE += /EHsc
DEFINES += VOLUMIZER


INCLUDEPATH += . \
	../../include \
	../../../apps/vaporgui/params \
	../../../apps/vaporgui/guis \
	../../../apps/vaporgui/main \
	../../../apps/vaporgui/render

MOC_DIR = ../../../apps/vaporgui/moc

SOURCES +=\ 
	   params/animationcontroller.cpp \
	   params/animationparams.cpp \
	   guis/coloradjustdialog.cpp \
	   guis/colorpicker.cpp \
	   main/command.cpp \
	   params/contourparams.cpp \
	   render/DVRBase.cpp \
	   render/DVRDebug.cpp \
	   render/DVRVolumizer.cpp \
	   params/dvrparams.cpp \
	   guis/flowmapframe.cpp \
	   params/flowparams.cpp \
	   render/flowrenderer.cpp \
	   render/glbox.cpp \
	   render/glutil.cpp \
	   render/glwindow.cpp \
	   params/histo.cpp \
	   params/isosurfaceparams.cpp \
 	   guis/loadtfdialog.cpp \
	   main/main.cpp \
           main/mainform.cpp \
	   main/messagereporter.cpp \	
	   guis/opacadjustdialog.cpp \
	   main/panelcommand.cpp \
	   params/params.cpp \
	   params/regionparams.cpp \
	   render/renderer.cpp \
	   guis/savetfdialog.cpp \
	   main/session.cpp \
	   params/sessionparams.cpp \
	   params/sharedcontrollerthread.cpp \
           main/tabmanager.cpp \
	   params/tfeditor.cpp \
	   guis/tfelocationtip.cpp \
	   guis/tfframe.cpp \
	   params/tfinterpolator.cpp \
	   render/trackball.cpp \
	   params/transferfunction.cpp \
	   params/unsharedcontrollerthread.cpp \
	   params/viewpoint.cpp \
	   params/viewpointparams.cpp \
	   params/vizfeatureparams.cpp \
	   main/vizactivatecommand.cpp \
	   guis/vizselectcombo.cpp \
           render/vizwin.cpp \
           main/vizwinmgr.cpp \
	   render/volumizerrenderer.cpp 
	   
HEADERS += \
	   params/animationcontroller.h \
	   params/animationparams.h \
	   guis/coloradjustdialog.h \
	   guis/colorpicker.h \
	   main/command.h \
	   params/contourparams.h \
	   params/controllerthread.h \
	   guis/flowmapframe.h \
	   params/flowparams.h \
	   render/DVRBase.h \
	   render/DVRDebug.h \
	   render/DVRVolumizer.h \
	   params/dvrparams.h \
	   render/flowrenderer.h \
	   render/glbox.h \
	   render/glwindow.h \
	   render/glutil.h \
	   params/histo.h \
	   params/isosurfaceparams.h \
	   params/vizfeatureparams.h \
	   guis/loadtfdialog.h \
           main/mainform.h \
	   main/messagereporter.h \
	   guis/opacadjustdialog.h \
	   main/panelcommand.h \
	   params/params.h \
	   params/regionparams.h \
	   render/renderer.h \
	   guis/savetfdialog.h \
	   main/session.h \
	   params/sessionparams.h \
           main/tabmanager.h \
	   params/tfeditor.h \
           guis/tfelocationtip.h \
	   params/tfinterpolator.h \
	   guis/tfframe.h \
	   render/trackball.h \
	   params/transferfunction.h \
	   params/viewpointparams.h \
	   main/vizactivatecommand.h \
	   guis/vizselectcombo.h \
	   render/vizwin.h \
           main/vizwinmgr.h \
	   render/volumizerrenderer.h 

FORMS +=  ../../../guis/ui/animationtab.ui \
	 ../../../guis/ui/viztab.ui \
	 ../../../guis/ui/dvr.ui \
	 ../../../guis/ui/flowtab.ui \
         ../../../guis/ui/regiontab.ui \ 
	 ../../../guis/ui/contourplanetab.ui \
       	 ../../../guis/ui/isotab.ui \
	 ../../../guis/ui/sessionparameters.ui \ 
	 ../../../guis/ui/vizfeatures.ui


