TOP = ..

include ${TOP}/make/config/prebase.mk


include ${TOP}/make/config/base.mk

all: $(BINDIR)/vapor-setup.sh $(BINDIR)/vapor-setup.csh

$(BINDIR)/vapor-setup.sh: vapor-setup.sh.sed Makefile
	sed -e s#ARCH#$(ARCH)# \
		-e s#BUILD_64_BIT#$(BUILD_64_BIT)# \
		-e s#INSTALL_PREFIX_DIR#$(INSTALL_PREFIX_DIR)# \
		-e s#EXPAT_LIB_PATH#$(EXPAT_LIB_PATH)# \
		-e s#NETCDF_LIB_PATH#$(NETCDF_LIB_PATH)# \
		-e s#QT_LIB_PATH#$(QT_LIB_PATH)# \
		-e s#BUILD_IDL_WRAPPERS#$(BUILD_IDL_WRAPPERS)# \
		< vapor-setup.sh.sed > $@

install:: $(BINDIR)/vapor-setup.sh
	@$(INSTALL_EXEC) $(BINDIR)/vapor-setup.sh $(INSTALL_BINDIR)

$(BINDIR)/vapor-setup.csh: vapor-setup.csh.sed Makefile
	sed -e s#ARCH#$(ARCH)# \
		-e s#BUILD_64_BIT#$(BUILD_64_BIT)# \
		-e s#INSTALL_PREFIX_DIR#$(INSTALL_PREFIX_DIR)# \
		-e s#EXPAT_LIB_PATH#$(EXPAT_LIB_PATH)# \
		-e s#NETCDF_LIB_PATH#$(NETCDF_LIB_PATH)# \
		-e s#QT_LIB_PATH#$(QT_LIB_PATH)# \
		-e s#BUILD_IDL_WRAPPERS#$(BUILD_IDL_WRAPPERS)# \
		< vapor-setup.csh.sed > $@

install:: $(BINDIR)/vapor-setup.csh
	@$(INSTALL_EXEC) $(BINDIR)/vapor-setup.csh $(INSTALL_BINDIR)

clean::
	@$(RM) $(BINDIR)/vapor-setup.sh $(BINDIR)/vapor-setup.csh

subdirs::

clobber::	clean
