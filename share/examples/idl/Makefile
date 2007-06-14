TOP = ../..

include ${TOP}/make/config/prebase.mk

IDL_EXAMPLES = AddMagVDF.pro AddCurlVDF.pro PrintMetaVDF.pro \
	curl_findiff.pro deriv_findiff.pro pencil2vapor.pro \
	MakeLinCmb.pro ReadRegionVDF.pro ReadVDF.pro \
	WriteTimeVaryVDF.pro WriteVDF.pro expregion.pro impexp.pro \
	impregion.pro marschner_lobb.pro QuickStartEx1.pro myderiv.pro


VERSION = $(shell cat $(TOP)/Version)
INSTALL_IDL_EX_DIR = $(INSTALL_PREFIX_DIR)/share/examples/$(VERSION)/idl

include ${TOP}/make/config/base.mk

install:: 
	@$(ECHO) "Installing IDL example files $(IDL_EXAMPLES) in $(INSTALL_IDL_EX_DIR)."
	@if test ! -d $(INSTALL_IDL_EX_DIR); then $(MKDIR) $(INSTALL_IDL_EX_DIR); fi
	@for i in $(IDL_EXAMPLES); do $(INSTALL_NONEXEC) $$i $(INSTALL_IDL_EX_DIR); done
