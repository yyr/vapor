TOP = ../..

include ${TOP}/make/config/arch.mk

SHARED = 0
LIBRARY = idl_vdfmodule
FILES = \
	IDLMetadata IDLMetadataAuto IDLCommon IDLWbIo IDLVaporImport IDLLifting1D

  
ifeq ($(ARCH),WIN32)
LIBRARY = libidl_vdfmodule
MAKEFILE_CXXFLAGS += /D"IDL_EXPORTS" 
MAKEFILE_LDFLAGS += /DEF:"../../make/win32/idl/idl.def"
MAKEFILE_LDFLAGS += ../../targets/win32/bin/common.lib 
MAKEFILE_LDFLAGS += ../../targets/win32/bin/vdf.lib
MAKEFILE_LDFLAGS += $(EXPATLIB)

ifdef	IDL_INC_PATH
MAKEFILE_LDFLAGS += $(IDL_INC_PATH)/../../bin/bin.x86/idl32.lib 
endif

else

LIBRARY = idl_vdfmodule
MAKEFILE_LDFLAGS += $(EXPATLIB) -lvdf -lcommon
endif

ifdef IDL_INC_PATH
MAKEFILE_CXXFLAGS += -I$(IDL_INC_PATH)
endif

ifdef	EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

include ${TOP}/make/config/base.mk

IDLMetadata.cpp: $(DSO_DIR)/libidl_vdfmodule.dlm

IDLMetadataAuto.cpp: IDLMetadataAuto.cpp.template mkmeta.pl
	./mkmeta.pl < $@.template > $@

$(DSO_DIR)/libidl_vdfmodule.dlm: libidl_vdfmodule.dlm.template mkmeta.pl
	./mkmeta.pl < libidl_vdfmodule.dlm.template > $@


install:: $(DSO_DIR)/libidl_vdfmodule.dlm
	@$(INSTALL_NONEXEC) $(DSO_DIR)/libidl_vdfmodule.dlm $(INSTALL_LIBDIR)
