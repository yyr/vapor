TOP = ../..

include ${TOP}/make/config/arch.mk

SHARED = 0
LIBRARY = idl_vdfmodule
FILES = \
	IDLMetadata IDLMetadataAuto IDLCommon IDLWbIo IDLVaporImport

  
ifeq ($(ARCH),WIN32)
CXXFLAGS += /D"IDL_EXPORTS" 
LDFLAGS += ../../targets/win32/bin/common.lib 
LDFLAGS += ../../targets/win32/bin/vdf.lib
LDFLAGS += $(IDL_INC_PATH)/../../bin/bin.x86/idl32.lib 
LDFLAGS += $(EXPATLIB)
else

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

