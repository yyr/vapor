TOP = ../..

include ${TOP}/make/config/arch.mk

SHARED = 0
QT = 1
PROGRAM	= vaporgui

INCLUDE_DIRS +=  -I"../../include/VAPoR/"   -I"./ui" -I"$(QTDIR)/include"
INCLUDE_DIRS += -I$(VOLUMIZER_ROOT)/include

ifdef	EXPAT_INC_PATH
INCLUDE_DIRS +=  -I$(EXPAT_INC_PATH)
endif

LIBRARIES += common vdf vjpeg

ifeq ($(ARCH),WIN32) 
CXXFLAGS += /DVOLUMIZER  
LDFLAGS += "$(VOLUMIZER_ROOT)/lib/vz.lib"
LDFLAGS += "$(QTDIR)/lib/qt-mt334.lib"
LDFLAGS += "$(QTDIR)/lib/qtmain.lib"
LDFLAGS += "opengl32.lib"
LDFLAGS += "glu32.lib"
LDFLAGS += "mpr.lib"
else   
CXXFLAGS += -DVOLUMIZER
LDFLAGS += -lvz   
LDFLAGS += -L$(QTDIR)/lib 
LDFLAGS += -lqt-mt -lGL -lGLU
endif



FILES := \
	animationparams animationcontroller \
	coloradjustdialog colorpicker command contourparams \
	DVRBase DVRDebug DVRVolumizer dvrparams glbox glutil glwindow \
	histo isosurfaceparams loadtfdialog main mainform \
	messagereporter opacadjustdialog panelcommand params regionparams \
	renderer savetfdialog session sessionparams sharedcontrollerthread\
	tabmanager tfeditor tfelocationtip \
	tfframe tfinterpolator trackball transferfunction \
	unsharedcontrollerthread \
	viewpointparams viewpoint vizactivatecommand \
	vizselectcombo vizwin vizwinmgr volumizerrenderer 

QT_HEADERS := \
	coloradjustdialog colorpicker \
	glbox \
	loadtfdialog \
	mainform \
	opacadjustdialog \
	renderer savetfdialog sessionparams tabmanager \
	tfframe \
	vizselectcombo vizwin vizwinmgr volumizerrenderer 


MOC_DIR = moc
UI_DIR = ui

UI_FILES := animationtab viztab regiontab contourplanetab \
	dvr isotab sessionparameters
  
include ${TOP}/make/config/base.mk
#LDFLAGS += $(QTLIB)

#testtarget:;echo $(FILES)

cat:
	echo "QTDIR=$(QTDIR)"
