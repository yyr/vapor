TOP = ../..

include ${TOP}/make/config/prebase.mk



include ${TOP}/make/config/base.mk

doc: Doxyfile

Doxyfile: FRC
	@$(ECHO) Building documentation
	@sed -e "s#TOP_OF_TREE#$(TOP)#" -e s#DOCDIR#$(DOCDIR)/library# $@.in > $@ 
	@@doxygen $@

FRC:

clobber::
	@$(ECHO) Removing doxygen output
	@-rm -fr $(DOCDIR)/*
