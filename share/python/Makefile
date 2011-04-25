TOP = ../..

include $(TOP)/make/config/prebase.mk

PFILES = pythonSystemStartup.py WRF_TK.py WRF_PV.py WRF_DBZ.py WRF_SLP.py WRF_RH.py WRF_ETH.py


INSTALL_PYTHON_DIR = $(INSTALL_SHAREDIR)/python

include $(TOP)/make/config/base.mk

install:: 
	@$(ECHO) "Installing python files $(PFILES) in $(INSTALL_PYTHON_DIR)."
	@if test ! -d $(INSTALL_PYTHON_DIR); then $(MKDIR) $(INSTALL_PYTHON_DIR); fi
	@for i in $(PFILES); do $(INSTALL_NONEXEC) $$i $(INSTALL_PYTHON_DIR); done
