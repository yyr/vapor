TOP = ../../..

include ${TOP}/make/config/prebase.mk



include ${TOP}/make/config/base.mk

ifeq ($(BUILD_DOXYGEN), 1)

all:: Doxyfile DoxyfilePython

Doxyfile: FRC
	@$(ECHO) Building documentation
	@sed -e "s#TOP_OF_TREE#$(TOP)#" -e s#DOCDIR#$(DOCDIR)/library# $@.in > $@ 
	@@doxygen $@

DoxyfilePython: FRC
	@$(ECHO) Building documentation
	@sed -e "s#TOP_OF_TREE#$(TOP)#" -e s#DOCDIR#$(DOCDIR)/pydoc# $@.in > $@ 
	@@doxygen $@

FRC:


INSTALL_DOC_DIR = $(INSTALL_SHAREDIR)/doc

install:: all
	@$(ECHO) "Installing Doxygen documentation in $(INSTALL_DOC_DIR)"
	@if test ! -d $(INSTALL_DOC_DIR); then $(MKDIR) $(INSTALL_DOC_DIR); fi
	cd $(DOCDIR); tar cf - library | (cd $(INSTALL_DOC_DIR); tar xf -)
	cd $(DOCDIR); tar cf - pydoc | (cd $(INSTALL_DOC_DIR); tar xf -)

endif

clobber::
	@$(ECHO) Removing doxygen output
	@-rm -fr $(DOCDIR)/*
