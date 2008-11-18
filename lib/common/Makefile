TOP = ../..

include ${TOP}/make/config/prebase.mk

SHARED = 0
LIBRARY = common
FILES = MyBase OptionParser EasyThreads CFuncs Base64 Version PVTime
HEADER_FILES = MyBase OptionParser EasyThreads CFuncs Base64 Version PVTime

ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"COMMON_EXPORTS"
endif

#LIB_DEFS = common.def

#
# Force version to be updated before other processing
#
all:: VERSION


include ${TOP}/make/config/base.mk


VERSION: $(INCDIR)/$(PROJECT)/Version.h

$(INCDIR)/$(PROJECT)/Version.h:: Version.h.sed  $(TOP)/Version
	$(ECHO) "Rebuilding $(INCDIR)/$(PROJECT)/Version.h"
	sed -e 's/MAJOR/$(VERSION_MAJOR)/' -e 's/MINOR/$(VERSION_MINOR)/' -e 's/RELEASE/$(VERSION_RELEASE)/' < Version.h.sed > $(INCDIR)/$(PROJECT)/Version.h


