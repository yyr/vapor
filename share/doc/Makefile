TOP = ../

include $(TOP)/make/config/prebase.mk

SUBDIRS = Doxygen help

DOCS = IDLExtRefManual.doc QuickstartGuide.doc ReferenceManual.doc \
	VDCIntro.doc WRFsupport.doc GeorgiaCaseStudy.doc \
	WRFGetStarted.doc

VERSION = $(shell cat $(TOP)/Version)
INSTALL_DOC_DIR = $(INSTALL_PREFIX_DIR)/share/doc/$(VERSION)


include ${TOP}/make/config/base.mk

install::
	@$(ECHO) "Installing documentation files $(DOCS) in $(INSTALL_DOC_DIR)"
	@if test ! -d $(INSTALL_DOC_DIR); then $(MKDIR) $(INSTALL_DOC_DIR); fi
	@for i in $(DOCS); do $(INSTALL_NONEXEC) $$i $(INSTALL_DOC_DIR); done
