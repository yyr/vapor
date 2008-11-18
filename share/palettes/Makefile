TOP = ../..

include ${TOP}/make/config/prebase.mk

TFS = farge_turb1.vtf grayscale.vtf reversedOpaque.vtf reversed.vtf


INSTALL_PALETTE_DIR = $(INSTALL_PREFIX_DIR)/share/palettes

include ${TOP}/make/config/base.mk

install:: 
	@$(ECHO) "Installing color palette files $(TFS) in $(INSTALL_PALETTE_DIR)."
	@if test ! -d $(INSTALL_PALETTE_DIR); then $(MKDIR) $(INSTALL_PALETTE_DIR); fi
	@for i in $(TFS); do $(INSTALL_NONEXEC) $$i $(INSTALL_PALETTE_DIR); done
