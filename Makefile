TOP = .

include $(TOP)/make/config/arch.mk

SUBDIRS = lib apps doc scripts

ifeq ($(BUILD_TESTAPPS), 1)
SUBDIRS += test_apps
endif

include ${TOP}/make/config/base.mk


install-dep: install
	. $(INSTALL_BINDIR)/vapor-setup.sh; for i in libqt-mt.so libexpat.so; do $(TOP)/buildutils/mklinks.pl $$i $(INSTALL_BINDIR)/vaporgui $(INSTALL_LIBDIR); done
	sed -e s#ARCH#$(ARCH)# < vapor-install.csh.sed > $(INSTALL_PREFIX_DIR)/vapor-install.csh
	chmod +x $(INSTALL_PREFIX_DIR)/vapor-install.csh

