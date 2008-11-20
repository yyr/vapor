TOP = ../..


include ${TOP}/make/config/prebase.mk

ifeq ($(ARCH),WIN32)
LIBRARY = libidl_vdfmodule
else
LIBRARY = idl_vdfmodule
endif

FILES = \
	IDLMetadata IDLMetadataAuto IDLCommon IDLWbIo IDLVaporImport IDLLifting1D

  
ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"IDL_EXPORTS" 
MAKEFILE_LDFLAGS += /DEF:"../../make/win32/idl/idl.def"

ifdef	IDL_INC_PATH
MAKEFILE_LDFLAGS += $(IDL_INC_PATH)/../../bin/bin.x86/idl32.lib 
endif
endif

ifeq ($(ARCH),Darwin)
MAKEFILE_CXXFLAGS += -Wno-missing-braces
endif

ifeq ($(ARCH),Linux)
MAKEFILE_CXXFLAGS += -Wno-missing-braces
endif


ifdef IDL_INC_PATH
MAKEFILE_CXXFLAGS += -I$(IDL_INC_PATH)
endif


ifeq ($(ARCH), Darwin)
DLLSUFFIX = .so
endif

include ${TOP}/make/config/base.mk


IDLMetadata.cpp: $(DSO_DIR)/libidl_vdfmodule.dlm

IDLMetadataAuto.cpp: IDLMetadataAuto.cpp.template mkmeta.pl
	./mkmeta.pl < $@.template > $@

$(DSO_DIR)/libidl_vdfmodule.dlm: libidl_vdfmodule.dlm.template mkmeta.pl
	./mkmeta.pl < libidl_vdfmodule.dlm.template > $@


install:: $(DSO_DIR)/libidl_vdfmodule.dlm
	@$(INSTALL_NONEXEC) $(DSO_DIR)/libidl_vdfmodule.dlm $(INSTALL_LIBDIR)
