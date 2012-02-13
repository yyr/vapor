TOP = ../..

include $(TOP)/make/config/prebase.mk

PFILES = $(wildcard *.tif)
PFILES += $(wildcard *.tiff)


INSTALL_IMAGES_DIR = $(INSTALL_SHAREDIR)/images

include $(TOP)/make/config/base.mk

install:: 
	@$(ECHO) "Installing image files $(PFILES) in $(INSTALL_IMAGES_DIR)."
	@if test ! -d $(INSTALL_IMAGES_DIR); then $(MKDIR) $(INSTALL_IMAGES_DIR); fi
	@for i in $(PFILES); do $(INSTALL_NONEXEC) $$i $(INSTALL_IMAGES_DIR); done
