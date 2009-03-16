TOP = ../..

include ${TOP}/make/config/prebase.mk

QT = 1
UI_DIR = ui
LIBRARY = params
FILES = \
	datastatus dvrparams flowparams glutil \
	histo mapperfunction params probeparams regionparams \
	tfinterpolator transferfunction viewpoint twoDparams \
	animationparams	viewpointparams ParamsBase \
	Colormap OpacityMap ParamNode ParamsIso ColorMapBase \
	MapperFunctionBase OpacityMapBase TransferFunctionLite

HEADER_FILES = ColorMapBase MapperFunctionBase OpacityMapBase \
	TransferFunctionLite tfinterpolator ParamsBase


ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"PARAMS_EXPORTS"
endif

ifdef	TIFF_INC_PATH
MAKEFILE_INCLUDE_DIRS += -I$(TIFF_INC_PATH)
endif

ifdef	GTIFF_INC_PATH
MAKEFILE_INCLUDE_DIRS += -I$(GTIFF_INC_PATH)
endif

ifdef	PROJ_INC_PATH
MAKEFILE_INCLUDE_DIRS += -I$(PROJ_INC_PATH)
endif

ifeq ($(ARCH),Darwin)
MAKEFILE_INCLUDE_DIRS += -I/usr/X11R6/include
MAKEFILE_CXXFLAGS += -Wno-unused-variable -Wno-non-virtual-dtor

LIBRARIES += qt-mt

ifdef   QT_LIB_PATH
MAKEFILE_LDFLAGS += -L$(QT_LIB_PATH)
endif
endif

include ${TOP}/make/config/base.mk

