TOP = ../..

include ${TOP}/make/config/prebase.mk

LIBRARY = common
FILES = MyBase OptionParser EasyThreads CFuncs Base64 Version PVTime \
	GetAppPath Proj4API GeoUtil
HEADER_FILES = MyBase OptionParser EasyThreads CFuncs Base64 Version PVTime \
	GetAppPath Proj4API GeoUtil errorcodes common


ifneq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += -DBINDIR=\"$(BINDIR)\"
MAKEFILE_CXXFLAGS += -DDSO_DIR=\"$(DSO_DIR)\"
MAKEFILE_CXXFLAGS += -DABS_TOP=\"$(ABS_TOP)\"
MAKEFILE_CXXFLAGS += -DQTDIR=\"$(QTDIR)\"
endif

ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"COMMON_EXPORTS"
endif

ifneq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += -DENABLE_THREADS
endif

#LIB_DEFS = common.def

#
# Force version to be updated before other processing
#
all:: VERSION


include ${TOP}/make/config/base.mk


VERSION: $(INCDIR)/$(PROJECT)/Version.h

DATE = $(shell date)

$(INCDIR)/$(PROJECT)/Version.h:: Version.h.sed  $(TOP)/Version 
	$(ECHO) "Rebuilding $(INCDIR)/$(PROJECT)/Version.h"
	sed -e 's/MAJOR/$(VERSION_MAJOR)/' -e 's/MINOR/$(VERSION_MINOR)/' -e 's/MICRO/$(VERSION_MICRO)/' -e 's/VERSION_RC/\"$(VERSION_RC)\"/' -e 's/DATE/\"$(DATE)\"/' < Version.h.sed > $(INCDIR)/$(PROJECT)/Version.h
