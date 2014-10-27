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
	AMRIO  Transpose DataMgr DataMgrWB \
	DataMgrWC DataMgrWRF DataMgrAMR \
	vdf WaveFiltBase  WaveFiltBior  WaveFiltDaub  WaveFiltCoif \
	WaveFiltHaar MatWaveBase  MatWaveDwt MatWaveWavedec  \
	SignificanceMap Compressor WaveCodecIO \
	DataMgrFactory  NCBuf \
	LayeredGrid RegularGrid SphericalGrid StretchedGrid NetCDFSimple \
	NetCDFCollection NetCDFCFCollection WeightTable \
	DCReaderNCDF  DCReaderMOM DCReaderROMS VDCFactory \
	DataMgrROMS DataMgrMOM DCReaderWRF UDUnitsClass Copy2VDF vdfcreate \
	WrfVDCcreator WrfVDFcreator  Proj4API 

HEADER_FILES = \
	BlkMemMgr XmlNode ExpatParseMgr ImpExp Metadata MetadataVDC \
	MetadataSpherical \
	WaveletBlock1D  WaveletBlock2D WaveletBlock3D \
	AMRTreeBranch AMRTree AMRData \
	VDFIOBase WaveletBlockIOBase WaveletBlock3DReader \
	WaveletBlock3DRegionReader WaveletBlock3DRegionWriter \
	WaveletBlock3DWriter WaveletBlock3DBufReader WaveletBlock3DBufWriter \
	AMRIO  DataMgr DataMgrWB \
	DataMgrWC DataMgrWRF DataMgrAMR \
	WaveFiltBase  WaveFiltBior  WaveFiltDaub  WaveFiltCoif \
	WaveFiltHaar MatWaveBase  MatWaveDwt MatWaveWavedec  \
	SignificanceMap Compressor WaveCodecIO \
	DataMgrFactory Lifting1D Transpose NCBuf \
	LayeredGrid RegularGrid SphericalGrid StretchedGrid NetCDFSimple \
	NetCDFCollection NetCDFCFCollection WeightTable \
	DCReader DCReaderNCDF DCReaderMOM DCReaderROMS VDCFactory \
	DataMgrROMS DataMgrMOM DCReaderWRF UDUnitsClass Copy2VDF vdfcreate \
	WrfVDCcreator WrfVDFcreator  Proj4API 

#LIBRARIESS += grib_api

#LIB_DEFS = vdf.def

ifeq ($(BUILD_GUI),1)
MAKEFILE_CXXFLAGS += -D"BUILD_GUI"
endif

ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"VDF_EXPORTS" 
MAKEFILE_CXXFLAGS += /D"DLL_NETCDF"
endif

include ${TOP}/make/config/base.mk
