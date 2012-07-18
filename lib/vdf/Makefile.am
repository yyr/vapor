#specify libs that this make file will build
lib_LIBRARIES = libpiovdc.a

#setup lib source files

libpiovdc_a_SOURCES = \
	BlkMemMgr.cpp XmlNode.cpp ExpatParseMgr.cpp ImpExp.cpp Metadata.cpp MetadataVDC.cpp \
	WaveletBlock1D.cpp  WaveletBlock2D.cpp WaveletBlock3D.cpp \
	VDFIOBase.cpp NCBuf.cpp \
	Transpose.cpp  vdf.cpp \
	WaveFiltBase.cpp  WaveFiltBior.cpp WaveFiltDaub.cpp  WaveFiltCoif.cpp \
	WaveFiltHaar.cpp MatWaveBase.cpp  MatWaveDwt.cpp MatWaveWavedec.cpp  \
	SignificanceMap.cpp Compressor.cpp WaveCodecIO.cpp  \
	DataMgr.cpp vdfbridge.cpp

libpiovdc_a_CPPFLAGS = -DPARALLEL 

if PNETCDF

libpiovdc_a_CPPFLAGS += -DPNETCDF

endif

if DEBUG

libpiovdc_a_CPPFLAGS += -DPIOVDC_DEBUG

endif

if NETCDF

libpiovdc_a_CPPFLAGS += -DNETCDF

endif

libpiovdc_a_CPPFLAGS += -O2


all: libpiovdc.a
