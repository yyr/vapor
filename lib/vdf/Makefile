TOP = ../..

include ${TOP}/make/config/arch.mk

SHARED = 0
LIBRARY = vdf
FILES = \
	BlkMemMgr DataMgr Lifting1D Metadata WaveletBlock3D \
	WaveletBlock3DBufReader WaveletBlock3DBufWriter \
	WaveletBlock3DIO WaveletBlock3DReader WaveletBlock3DRegionReader \
	WaveletBlock3DWriter XmlNode ImpExp vdf

HEADER_FILES = \
	BlkMemMgr DataMgr Lifting1D Metadata WaveletBlock3D \
	WaveletBlock3DBufReader WaveletBlock3DBufWriter \
	WaveletBlock3DIO WaveletBlock3DReader WaveletBlock3DRegionReader \
	WaveletBlock3DWriter XmlNode ImpExp

#LIB_DEFS = vdf.def
  
ifeq ($(ARCH),WIN32)
CXXFLAGS += /D"VDF_EXPORTS" 
LDFLAGS += ../../targets/win32/bin/common.lib 
LDFLAGS += "mpr.lib"
endif

LDFLAGS += $(EXPATLIB)


ifdef	EXPAT_INC_PATH
CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

include ${TOP}/make/config/base.mk

