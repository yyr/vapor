TOP = ../..

include ${TOP}/make/config/arch.mk

EX_FILES = PrintMetaVDF.pro ReadRegionVDF.pro ReadVDF.pro \
	WriteTimeVaryVDF.pro WriteVDF.pro expregion.pro impexp.pro \
	impregion.pro marschner_lobb.pro QuickStartEx1.pro myderiv.pro

include ${TOP}/make/config/base.mk

INSTALL_IDL_EX_DIR = $(INSTALL_PREFIX_DIR)/examples/idl


install:: 
	@$(ECHO) "Installing IDL example files $(EX_FILES) in $(INSTALL_IDL_EX_DIR)."
	@if test ! -d $(INSTALL_IDL_EX_DIR); then $(MKDIR) $(INSTALL_IDL_EX_DIR); fi
	@for i in $(EX_FILES); do $(INSTALL_NONEXEC) $$i $(INSTALL_IDL_EX_DIR); done

