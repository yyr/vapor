TOP = ../..

include ${TOP}/make/config/arch.mk

SHARED = 0
QT = 1
PROGRAM	= vaporgui

INCLUDE_DIRS +=  -I"../../include/VAPoR/"   -I"./ui" -I$(QTDIR)/include -I$(EXPAT_INC_PATH)
LIBRARIES += common vdf

ifeq ($(ARCH),WIN32) 
CXXFLAGS += /DVOLUMIZER  
LDFLAGS += "$(VOLUMIZER_ROOT)/lib/vz.lib"
LDFLAGS += "$(QTDIR)/lib/qt-mt332.lib"
LDFLAGS += "$(QTDIR)/lib/qtmain.lib"
LDFLAGS += "opengl32.lib"
LDFLAGS += "glu32.lib"
else   
CXXFLAGS += -DVOLUMIZER
LDFLAGS += -lvz   
LDFLAGS += -L$(QTDIR)/lib 
LDFLAGS += -lGLU -lGL  
endif
ifeq ($(ARCH),Linux)
LDFLAGS += -lqt-mt  
endif
ifeq ($(ARCH),IRIX64)
LDFLAGS += -lqt
endif



FILES := \
	animationparams \
	coloradjustdialog colorpicker command contourparams \
	DVRBase DVRDebug DVRVolumizer dvrparams glbox glutil glwindow \
	histo isosurfaceparams main mainform minmaxcombo \
	opacadjustdialog panelcommand params regionparams \
	renderer session tabmanager tfeditor tfelocationtip \
	tfframe tfinterpolator trackball transferfunction \
	vcr viewpointparams vizactivatecommand vizmgrdialog viznameedit \
	vizselectcombo vizwin vizwinmgr volumizerrenderer 

QT_HEADERS := \
	coloradjustdialog colorpicker \
	glbox \
	mainform minmaxcombo \
	opacadjustdialog \
	renderer tabmanager \
	tfframe \
	vcr vizmgrdialog viznameedit \
	vizselectcombo vizwin vizwinmgr volumizerrenderer 


MOC_DIR = moc
UI_DIR = ui

UI_FILES := animationtab viztab regiontab contourplanetab \
	dvr isotab sessionparameters
  
include ${TOP}/make/config/base.mk
#LDFLAGS += $(QTLIB)

#testtarget:;echo $(FILES)
