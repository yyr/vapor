TOP = .

include $(TOP)/make/config/prebase.mk

SUBDIRS = lib apps doc scripts examples

ifeq ($(BUILD_TESTAPPS), 1)
SUBDIRS += test_apps
endif

include ${TOP}/make/config/base.mk

ifeq ($(ARCH), Darwin)
DEPLIBS = libqt-mt libexpat
else
DEPLIBS = libqt-mt.so libexpat.so
endif

install-dep: install
	@$(ECHO) "Removing $(DEPLIBS) from $(INSTALL_LIBDIR)"
	@for i in $(DEPLIBS); do $(RM) $(INSTALL_LIBDIR)/$$i.*; done
	@$(ECHO) "Installing $(DEPLIBS) to $(INSTALL_LIBDIR)"
	@. $(INSTALL_BINDIR)/vapor-setup.sh; for i in $(DEPLIBS); do $(TOP)/buildutils/mklinks.pl $(ARCH) $$i $(INSTALL_BINDIR)/vaporgui $(INSTALL_LIBDIR); done
	@sed -e s#ARCH#$(ARCH)# < vapor-install.csh.sed > $(INSTALL_PREFIX_DIR)/vapor-install.csh
	@chmod +x $(INSTALL_PREFIX_DIR)/vapor-install.csh

