TOP = ../

include $(TOP)/make/config/prebase.mk

SUBDIRS += idl
SUBDIRS += quickstart
SUBDIRS += WRFCaseStudy

include ${TOP}/make/config/base.mk

install::
	@$(ECHO) "Installing default preferences file in $(INSTALL_PREFIX_DIR)."
	@if test ! -d $(INSTALL_PREFIX_DIR); then $(MKDIR) $(INSTALL_PREFIX_DIR); fi
	$(INSTALL_NONEXEC) vapor_prefs $(INSTALL_SHAREDIR)/.vapor_prefs




