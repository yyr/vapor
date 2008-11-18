TOP = .

include $(TOP)/make/config/prebase.mk

SUBDIRS = lib apps doc scripts examples share

ifeq ($(BUILD_TESTAPPS), 1)
SUBDIRS += test_apps
endif

include ${TOP}/make/config/base.mk

install-dep:: install
	@sed -e s#ARCH#$(ARCH)# < vapor-install.csh.sed > $(INSTALL_PREFIX_DIR)/vapor-install.csh
	@chmod +x $(INSTALL_PREFIX_DIR)/vapor-install.csh

MAC_BUNDLE_DIR = /tmp/vapor-macbundle

macbundle: install-dep
macbundle: FRC
	@if test ! -d $(MAC_BUNDLE_DIR); then $(MKDIR) $(MAC_BUNDLE_DIR); fi
	$(TOP)/buildutils/macbundle.pl $(INSTALL_PREFIX_DIR) $(TOP)/MacBundle/VAPOR.app $(MAC_BUNDLE_DIR) $(VERSION) 
	if test ! -d $(MAC_BUNDLE_DIR)/Install_Resources; then $(MKDIR) $(MAC_BUNDLE_DIR)/Install_Resources; fi
	$(CP) $(TOP)/buildutils/postflight $(MAC_BUNDLE_DIR)/Install_Resources
	$(CP) $(TOP)/Images/splash.jpg $(MAC_BUNDLE_DIR)/Install_Resources

FRC:
