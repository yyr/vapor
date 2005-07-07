TOP = ../..

include ${TOP}/make/config/arch.mk

SHARED = 0
QT = 1
PROGRAM	= vaporgui

ifdef	VOLUMIZER_ROOT
MAKEFILE_CXXFLAGS += -I$(VOLUMIZER_ROOT)/include
endif

ifdef	EXPAT_INC_PATH
MAKEFILE_CXXFLAGS +=  -I$(EXPAT_INC_PATH)
endif

LIBRARIES += common vdf vjpeg

ifeq ($(ARCH),WIN32) 
MAKEFILE_CXXFLAGS += /DVOLUMIZER  
MAKEFILE_LDFLAGS += "$(VOLUMIZER_ROOT)/lib/vz.lib"
MAKEFILE_LDFLAGS += "$(QTDIR)/lib/qt-mt334.lib"
MAKEFILE_LDFLAGS += "$(QTDIR)/lib/qtmain.lib"
MAKEFILE_LDFLAGS += "opengl32.lib"
MAKEFILE_LDFLAGS += "glu32.lib"
MAKEFILE_LDFLAGS += "mpr.lib"
else   
MAKEFILE_CXXFLAGS += -DVOLUMIZER
MAKEFILE_LDFLAGS += -lvz   
MAKEFILE_LDFLAGS += -L$(QTDIR)/lib 
MAKEFILE_LDFLAGS += -lqt-mt -lGL -lGLU
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
	viewpointparams viewpoint vizactivatecommand vizfeatureparams \
	vizselectcombo vizwin vizwinmgr volumizerrenderer 

QT_HEADERS := \
	coloradjustdialog colorpicker \
	glbox \
	loadtfdialog \
	mainform \
	opacadjustdialog \
	renderer savetfdialog sessionparams tabmanager \
	tfframe \
	vizselectcombo vizwin vizfeatureparams vizwinmgr volumizerrenderer 


MOC_DIR = moc
UI_DIR = ui

UI_FILES := animationtab viztab regiontab contourplanetab \
	dvr isotab sessionparameters vizfeatures
  
include ${TOP}/make/config/base.mk
#MAKEFILE_LDFLAGS += $(QTLIB)

#testtarget:;echo $(FILES)

cat:
	echo "QTDIR=$(QTDIR)"
