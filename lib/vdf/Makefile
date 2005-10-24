TOP = ../..

include ${TOP}/make/config/arch.mk

SHARED = 0
LIBRARY = vdf
FILES = \
	BlkMemMgr DataMgr Lifting1D Metadata WaveletBlock3D \
	WaveletBlock3DBufReader WaveletBlock3DBufWriter \
	VDFIOBase WaveletBlock3DIO WaveletBlock3DReader \
	WaveletBlock3DRegionReader \
	WaveletBlock3DWriter XmlNode ImpExp vdf ExpatParseMgr 

HEADER_FILES = \
	BlkMemMgr DataMgr Lifting1D Metadata WaveletBlock3D \
	WaveletBlock3DBufReader WaveletBlock3DBufWriter \
	VDFIOBase WaveletBlock3DIO WaveletBlock3DReader \
	WaveletBlock3DRegionReader \
	WaveletBlock3DWriter XmlNode ImpExp ExpatParseMgr 




ifeq ($(BUILD_AMR_SUPPORT), 1)

FILES +=     AMRTree AMRTreeBranch AMRData AMRDataIO
HEADER_FILES +=     AMRTree AMRTreeBranch AMRData AMRDataIO
endif

ifdef	NETCDF_INC_PATH
MAKEFILE_CXXFLAGS += -I$(NETCDF_INC_PATH)
endif

ifdef NETCDF_LIB_PATH
MAKEFILE_LDFLAGS += -L$(NETCDF_LIB_PATH) -lnetcdf_c++ -lnetcdf 
endif


#LIB_DEFS = vdf.def
  
ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"VDF_EXPORTS" 
MAKEFILE_LDFLAGS += ../../targets/win32/bin/common.lib 
MAKEFILE_LDFLAGS += "mpr.lib"
endif

MAKEFILE_LDFLAGS += $(EXPATLIB)


ifdef	EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

include ${TOP}/make/config/base.mk

