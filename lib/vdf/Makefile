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
ifeq ($(ARCH),WIN32)
MAKEFILE_LDFLAGS += /LIBPATH $(NETCDF_LIB_PATH)
else
MAKEFILE_LDFLAGS += -L$(NETCDF_LIB_PATH)
endif
endif

ifdef	EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

ifdef EXPAT_LIB_PATH
ifeq ($(ARCH),WIN32)
MAKEFILE_LDFLAGS += /LIBPATH $(EXPAT_LIB_PATH)
else
MAKEFILE_LDFLAGS += -L$(EXPAT_LIB_PATH)
endif
endif

#LIB_DEFS = vdf.def
  
ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"VDF_EXPORTS" 
MAKEFILE_CXXFLAGS += /D"DLL_NETCDF"
endif

LIBRARIES = common expat netcdf

ifeq ($(ARCH),WIN32)
LIBRARIES += mpr
endif


include ${TOP}/make/config/base.mk
