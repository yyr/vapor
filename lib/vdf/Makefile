TOP = ../..

include ${TOP}/make/config/prebase.mk

LIBRARY = vdf
FILES = \
	BlkMemMgr XmlNode ExpatParseMgr ImpExp Metadata MetadataVDC \
	MetadataSpherical \
	WaveletBlock1D  WaveletBlock2D WaveletBlock3D \
	AMRTreeBranch AMRTree AMRData \
	VDFIOBase WaveletBlockIOBase WaveletBlock3DReader \
	WaveletBlock3DRegionReader WaveletBlock3DRegionWriter \
	WaveletBlock3DWriter WaveletBlock3DBufReader WaveletBlock3DBufWriter \
	LayeredIO AMRIO  WRF Transpose DataMgr DataMgrWB DataMgrLayered vdf \
	DataMgrFactory \
	WaveFiltBase  WaveFiltBior  WaveFiltDaub  WaveFiltCoif \
	WaveFiltHaar MatWaveBase  MatWaveDwt MatWaveWavedec  \
	SignificanceMap Compressor WaveCodecIO DataMgrWC

HEADER_FILES = \
	BlkMemMgr XmlNode ExpatParseMgr ImpExp Metadata MetadataVDC \
	MetadataSpherical \
	WaveletBlock1D  WaveletBlock2D WaveletBlock3D \
	AMRTreeBranch AMRTree AMRData \
	VDFIOBase WaveletBlockIOBase WaveletBlock3DReader \
	WaveletBlock3DRegionReader WaveletBlock3DRegionWriter \
	WaveletBlock3DWriter WaveletBlock3DBufReader WaveletBlock3DBufWriter \
	LayeredIO AMRIO  WRF DataMgr DataMgrWB DataMgrLayered \
	DataMgrFactory \
	WaveFiltBase  WaveFiltBior  WaveFiltDaub  WaveFiltCoif \
	WaveFiltHaar MatWaveBase  MatWaveDwt MatWaveWavedec  \
	SignificanceMap Compressor WaveCodecIO DataMgrWC


#LIB_DEFS = vdf.def
  
ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"VDF_EXPORTS" 
MAKEFILE_CXXFLAGS += /D"DLL_NETCDF"
endif

include ${TOP}/make/config/base.mk
