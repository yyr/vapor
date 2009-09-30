TOP = ../

include $(TOP)/make/config/prebase.mk

SUBDIRS = Doxygen help man

DOCS = IDLExtRefManual.doc QuickstartGuide.doc ReferenceManual.doc \
	VDCIntro.doc WRFsupport.doc GeorgiaCaseStudy.doc \
	VAPOR-WRF-NCL.doc WRFGetStarted.doc

INSTALL_DOC_DIR = $(INSTALL_SHAREDIR)/doc


include ${TOP}/make/config/base.mk

install::
	@$(ECHO) "Installing documentation files $(DOCS) in $(INSTALL_DOC_DIR)"
	@if test ! -d $(INSTALL_DOC_DIR); then $(MKDIR) $(INSTALL_DOC_DIR); fi
	@for i in $(DOCS); do $(INSTALL_NONEXEC) $$i $(INSTALL_DOC_DIR); done
