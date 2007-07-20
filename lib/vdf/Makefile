TOP = ../..

include ${TOP}/make/config/prebase.mk

SHARED = 0
LIBRARY = vdf
FILES = \
	BlkMemMgr DataMgr Metadata WaveletBlock3D \
	WaveletBlock3DBufReader WaveletBlock3DBufWriter \
	VDFIOBase WaveletBlock3DIO WaveletBlock3DReader \
	WaveletBlock3DRegionReader WaveletBlock3DRegionWriter \
	WaveletBlock3DWriter XmlNode ImpExp vdf ExpatParseMgr \
	LayeredIO AMRIO \
	AMRTree AMRTreeBranch AMRData MetadataSpherical

HEADER_FILES = \
	BlkMemMgr DataMgr Metadata WaveletBlock3D \
	WaveletBlock3DBufReader WaveletBlock3DBufWriter \
	VDFIOBase WaveletBlock3DIO WaveletBlock3DReader \
	WaveletBlock3DRegionReader WaveletBlock3DRegionWriter \
	WaveletBlock3DWriter XmlNode ImpExp ExpatParseMgr AMRIO \
	LayeredIO AMRTree AMRTreeBranch AMRData MetadataSpherical

ifdef	NETCDF_INC_PATH
MAKEFILE_CXXFLAGS += -I$(NETCDF_INC_PATH)
endif

ifdef NETCDF_LIB_PATH
ifeq ($(ARCH),WIN32)
MAKEFILE_LDFLAGS += /LIBPATH:$(NETCDF_LIB_PATH)
else
MAKEFILE_LDFLAGS += -L$(NETCDF_LIB_PATH)
endif
endif

ifdef	EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

ifdef EXPAT_LIB_PATH
ifeq ($(ARCH),WIN32)
MAKEFILE_LDFLAGS += /LIBPATH:$(EXPAT_LIB_PATH)
else
MAKEFILE_LDFLAGS += -L$(EXPAT_LIB_PATH)
endif
endif

#LIB_DEFS = vdf.def
  
ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"VDF_EXPORTS" 
MAKEFILE_CXXFLAGS += /D"DLL_NETCDF"
endif

ifeq ($(ARCH),WIN32)
LIBRARIES = common libexpat netcdf
else
LIBRARIES = common expat netcdf
endif


ifeq ($(ARCH),WIN32)
LIBRARIES += mpr
endif


include ${TOP}/make/config/base.mk
