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
MAKEFILE_CXXFLAGS += -Iguis -Iparams -Irender -Imain

LIBRARIES += common vdf vjpeg flow

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

VPATH = guis params render main


FILES := \
	params/animationparams params/animationcontroller \
	params/contourparams  params/dvrparams params/flowparams \
	params/histo \
	params/isosurfaceparams params/params params/regionparams \
	params/sessionparams  params/sharedcontrollerthread\
	params/tfeditor  params/tfinterpolator \
	params/transferfunction params/unsharedcontrollerthread \
	params/viewpointparams params/viewpoint params/vizfeatureparams \
	guis/coloradjustdialog guis/colorpicker guis/loadtfdialog \
	guis/opacadjustdialog guis/savetfdialog \
	guis/tfelocationtip guis/tfframe guis/vizselectcombo \
	main/command main/main main/mainform main/messagereporter \
	main/panelcommand main/session main/tabmanager \
	main/vizactivatecommand main/vizwinmgr \
	render/DVRBase render/DVRDebug render/DVRVolumizer \
	render/glbox render/glutil render/glwindow render/renderer \
	render/trackball render/vizwin  render/volumizerrenderer \
	render/flowrenderer

QT_HEADERS := \
	guis/coloradjustdialog guis/colorpicker \
	guis/loadtfdialog guis/opacadjustdialog \
	guis/savetfdialog guis/tfframe guis/vizselectcombo \
	render/glbox render/renderer render/flowrenderer \
	render/vizwin render/volumizerrenderer \
	main/mainform main/tabmanager main/vizwinmgr \
	params/sessionparams params/vizfeatureparams 


MOC_DIR = moc
UI_DIR = guis/ui

UI_FILES := animationtab flowtab viztab regiontab contourplanetab \
	dvr isotab sessionparameters vizfeatures
  
include ${TOP}/make/config/base.mk
#MAKEFILE_LDFLAGS += $(QTLIB)

#testtarget:;echo $(FILES)

cat:
	echo "QTDIR=$(QTDIR)"
