TOP = ../..

include ${TOP}/make/config/prebase.mk

LIBRARY = vdf
FILES = \
	BlkMemMgr XmlNode ExpatParseMgr ImpExp Metadata MetadataVDC \
	MetadataSpherical MetadataWRF \
	WaveletBlock1D  WaveletBlock2D WaveletBlock3D \
	AMRTreeBranch AMRTree AMRData \
	VDFIOBase WaveletBlockIOBase WaveletBlock3DReader \
	WaveletBlock3DRegionReader WaveletBlock3DRegionWriter \
	WaveletBlock3DWriter WaveletBlock3DBufReader WaveletBlock3DBufWriter \
	LayeredIO AMRIO  WRF WRFReader Transpose DataMgr DataMgrWB \
	DataMgrLayered DataMgrLayered2 DataMgrWRF vdf \
	WaveFiltBase  WaveFiltBior  WaveFiltDaub  WaveFiltCoif \
	WaveFiltHaar MatWaveBase  MatWaveDwt MatWaveWavedec  \
	SignificanceMap Compressor WaveCodecIO DataMgrWC DataMgrAMR \
	DataMgrFactory 

HEADER_FILES = \
	BlkMemMgr XmlNode ExpatParseMgr ImpExp Metadata MetadataVDC \
	MetadataSpherical MetadataWRF \
	WaveletBlock1D  WaveletBlock2D WaveletBlock3D \
	AMRTreeBranch AMRTree AMRData \
	VDFIOBase WaveletBlockIOBase WaveletBlock3DReader \
	WaveletBlock3DRegionReader WaveletBlock3DRegionWriter \
	WaveletBlock3DWriter WaveletBlock3DBufReader WaveletBlock3DBufWriter \
	LayeredIO AMRIO  WRF WRFReader DataMgr DataMgrWB DataMgrLayered \
	DataMgrLayered2 DataMgrWRF \
	WaveFiltBase  WaveFiltBior  WaveFiltDaub  WaveFiltCoif \
	WaveFiltHaar MatWaveBase  MatWaveDwt MatWaveWavedec  \
	SignificanceMap Compressor WaveCodecIO DataMgrWC DataMgrAMR \
	DataMgrFactory Lifting1D Transpose 


#LIB_DEFS = vdf.def

ifeq ($(BUILD_GUI),1)
MAKEFILE_CXXFLAGS += -D"BUILD_GUI"
endif

ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"VDF_EXPORTS" 
MAKEFILE_CXXFLAGS += /D"DLL_NETCDF"
endif

include ${TOP}/make/config/base.mk
