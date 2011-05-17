TOP = ../..

include $(TOP)/make/config/prebase.mk

TFS = farge_turb1.vtf grayscale.vtf reversedOpaque.vtf reversed.vtf default.vtf defaultOpaque.vtf \
precip_11lev.vtf precip4_11lev.vtf precip2_15lev.vtf precip4_diff_19lev.vtf precip2_17lev.vtf  precip_diff_12lev.vtf precip3_16lev.vtf precip_diff_1lev.vtf \
BkBlAqGrYeOrReViWh.vtf BlAqGrYeOrReVi.vtf BlAqGrYeOrRe.vtf BlGrYeOrReVi.vtf BlRe.vtf testcmap.vtf wh-bl-gr-ye-re.vtf



INSTALL_PALETTE_DIR = $(INSTALL_SHAREDIR)/palettes

include $(TOP)/make/config/base.mk

install:: 
	@$(ECHO) "Installing color palette files $(TFS) in $(INSTALL_PALETTE_DIR)."
	@if test ! -d $(INSTALL_PALETTE_DIR); then $(MKDIR) $(INSTALL_PALETTE_DIR); fi
	@for i in $(TFS); do $(INSTALL_NONEXEC) $$i $(INSTALL_PALETTE_DIR); done
