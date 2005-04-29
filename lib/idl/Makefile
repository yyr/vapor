TOP = ../..

include ${TOP}/make/config/arch.mk

SHARED = 0
LIBRARY = idl_vdfmodule
FILES = \
	IDLMetadata IDLMetadataAuto IDLCommon IDLWbIo IDLVaporImport

  
ifeq ($(ARCH),WIN32)
LIBRARY = libidl_vdfmodule
CXXFLAGS += /D"IDL_EXPORTS" 
LDFLAGS += /DEF:"../../make/win32/idl/idl.def"
LDFLAGS += ../../targets/win32/bin/common.lib 
LDFLAGS += ../../targets/win32/bin/vdf.lib
LDFLAGS += $(EXPATLIB)

ifdef	IDL_INC_PATH
LDFLAGS += $(IDL_INC_PATH)/../../bin/bin.x86/idl32.lib 
endif

else

LIBRARY = idl_vdfmodule
LDFLAGS += $(EXPATLIB) -lvdf -lcommon
endif

ifdef IDL_INC_PATH
CXXFLAGS += -I$(IDL_INC_PATH)
endif

ifdef	EXPAT_INC_PATH
CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

include ${TOP}/make/config/base.mk

IDLMetadata.cpp: libidl_vdfmodule.dlm

IDLMetadataAuto.cpp: IDLMetadataAuto.cpp.template mkmeta.pl
	./mkmeta.pl < $@.template > $@

libidl_vdfmodule.dlm: libidl_vdfmodule.dlm.template mkmeta.pl
	./mkmeta.pl < $@.template > $@


install:: libidl_vdfmodule.dlm
	@$(INSTALL_NONEXEC) libidl_vdfmodule.dlm $(INSTALL_LIBDIR)
