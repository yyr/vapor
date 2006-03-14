TOP = ..

include ${TOP}/make/config/arch.mk


include ${TOP}/make/config/base.mk

all: $(BINDIR)/vapor-setup.sh $(BINDIR)/vapor-setup.csh

$(BINDIR)/vapor-setup.sh: vapor-setup.sh.m4 Makefile
	echo bin = $(BINDIR)
	m4 -DARCH=$(ARCH) -DBUILD_64_BIT=$(BUILD_64_BIT) -DINSTALL_PREFIX_DIR=$(INSTALL_PREFIX_DIR) -DEXPAT_LIB_PATH=$(EXPAT_LIB_PATH) -DNETCDF_LIB_PATH=$(NETCDF_LIB_PATH) < vapor-setup.sh.m4 > $@

install:: $(BINDIR)/vapor-setup.sh
	@$(INSTALL_EXEC) $(BINDIR)/vapor-setup.sh $(INSTALL_BINDIR)

$(BINDIR)/vapor-setup.csh: vapor-setup.csh.m4 Makefile
	echo bin = $(BINDIR)
	m4 -DARCH=$(ARCH) -DBUILD_64_BIT=$(BUILD_64_BIT) -DINSTALL_PREFIX_DIR=$(INSTALL_PREFIX_DIR) -DEXPAT_LIB_PATH=$(EXPAT_LIB_PATH) -DNETCDF_LIB_PATH=$(NETCDF_LIB_PATH) < vapor-setup.csh.m4 > $@

install:: $(BINDIR)/vapor-setup.csh
	@$(INSTALL_EXEC) $(BINDIR)/vapor-setup.csh $(INSTALL_BINDIR)

clean::
	@$(RM) $(BINDIR)/vapor-setup.sh

subdirs::

clobber::	clean
