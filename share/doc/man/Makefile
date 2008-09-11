TOP = ../..

include ${TOP}/make/config/prebase.mk



include ${TOP}/make/config/base.mk

DOCS = asciitf2vtf vdfbkup vdfcreate vdfedit vdfls raw2vdf vdf2raw \
	wrfvdfcreate wrf2vdf ncdf2vdf


doc: 
	@if test ! -d $(DOCDIR)/commands/html; then $(MKDIR) $(DOCDIR)/commands/html; fi
	@for i in $(DOCS); do echo "Processing $$i.pod"; pod2html --infile $$i.pod --outfile $(DOCDIR)/commands/html/$$i.html; done
	@if test ! -d  pod2htmd.tmp; then $(RM) pod2htmd.tmp; fi
	@if test ! -d  pod2htmi.tmp; then $(RM) pod2htmi.tmp; fi

clobber::
