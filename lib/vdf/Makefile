TOP = ../..

include ${TOP}/make/config/prebase.mk

SHARED = 0
LIBRARY = vdf
FILES = \
	BlkMemMgr DataMgr Metadata WaveletBlock1D WaveletBlock2D WaveletBlock3D \
	WaveletBlock3DBufReader WaveletBlock3DBufWriter \
	VDFIOBase WaveletBlockIOBase WaveletBlock3DIO WaveletBlock3DReader \
	WaveletBlock3DRegionReader WaveletBlock3DRegionWriter \
	WaveletBlock3DWriter XmlNode ImpExp vdf ExpatParseMgr \
	LayeredIO AMRIO \
	AMRTree AMRTreeBranch AMRData MetadataSpherical WRF \
	WaveletBlock2DIO WaveletBlock2DRegionReader WaveletBlock2DRegionWriter \
	Transpose

HEADER_FILES = \
	BlkMemMgr DataMgr Metadata WaveletBlock1D WaveletBlock2D WaveletBlock3D \
	WaveletBlock3DBufReader WaveletBlock3DBufWriter \
	VDFIOBase WaveletBlockIOBase WaveletBlock3DIO WaveletBlock3DReader \
	WaveletBlock3DRegionReader WaveletBlock3DRegionWriter \
	WaveletBlock3DWriter XmlNode ImpExp ExpatParseMgr AMRIO \
	LayeredIO AMRTree AMRTreeBranch AMRData MetadataSpherical WRF \
	WaveletBlock2DIO WaveletBlock2DRegionReader WaveletBlock2DRegionWriter 


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
