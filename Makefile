TOP = .

include $(TOP)/make/config/prebase.mk

SUBDIRS = lib apps doc scripts examples

ifeq ($(BUILD_TESTAPPS), 1)
SUBDIRS += test_apps
endif

include ${TOP}/make/config/base.mk

all:: Version

Version: $(BINDIR)/vaporversion
	@LD_LIBRARY_PATH=$(DSO_DIR); export LD_LIBRARY_PATH; $(BINDIR)/vaporversion > Version

$(BINDIR)/vaporversion:

install-dep:: install
	@sed -e s#ARCH#$(ARCH)# < vapor-install.csh.sed > $(INSTALL_PREFIX_DIR)/vapor-install.csh
	@chmod +x $(INSTALL_PREFIX_DIR)/vapor-install.csh

