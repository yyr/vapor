TOP = ..

include ${TOP}/make/config/prebase.mk


include ${TOP}/make/config/base.mk

all:: $(BINDIR)/vapor-setup.sh $(BINDIR)/vapor-setup.csh vdfbkup.pl getWMSImage.sh

$(BINDIR)/vapor-setup.sh: FRC
	sed -e s#ARCH#$(ARCH)# \
		-e s#BUILD_64_BIT#$(BUILD_64_BIT)# \
		-e s#INSTALL_PREFIX_DIR#$(INSTALL_PREFIX_DIR)# \
		-e s#EXPAT_LIB_PATH#$(EXPAT_LIB_PATH)# \
		-e s#NETCDF_LIB_PATH#$(NETCDF_LIB_PATH)# \
		-e s#QT_LIB_PATH#$(QT_LIB_PATH)# \
		-e s#BUILD_IDL_WRAPPERS#$(BUILD_IDL_WRAPPERS)# \
		-e s#INSTALL_SHAREDIR#$(INSTALL_SHAREDIR)# \
		< vapor-setup.sh.sed > $@

vdfbkup.pl: FRC

install:: $(BINDIR)/vapor-setup.sh vdfbkup.pl getWMSImage.sh
	@$(INSTALL_EXEC) $(BINDIR)/vapor-setup.sh $(INSTALL_BINDIR)
	@$(INSTALL_EXEC) vdfbkup.pl $(INSTALL_BINDIR)
	@$(INSTALL_EXEC) getWMSImage.sh $(INSTALL_BINDIR)

$(BINDIR)/vapor-setup.csh: FRC
	sed -e s#ARCH#$(ARCH)# \
		-e s#BUILD_64_BIT#$(BUILD_64_BIT)# \
		-e s#INSTALL_PREFIX_DIR#$(INSTALL_PREFIX_DIR)# \
		-e s#EXPAT_LIB_PATH#$(EXPAT_LIB_PATH)# \
		-e s#NETCDF_LIB_PATH#$(NETCDF_LIB_PATH)# \
		-e s#QT_LIB_PATH#$(QT_LIB_PATH)# \
		-e s#BUILD_IDL_WRAPPERS#$(BUILD_IDL_WRAPPERS)# \
		-e s#INSTALL_SHAREDIR#$(INSTALL_SHAREDIR)# \
		< vapor-setup.csh.sed > $@

FRC:

install:: $(BINDIR)/vapor-setup.csh
	@$(INSTALL_EXEC) $(BINDIR)/vapor-setup.csh $(INSTALL_BINDIR)

clean::
	@$(RM) $(BINDIR)/vapor-setup.sh $(BINDIR)/vapor-setup.csh 

subdirs::

clobber::	clean
