TOP = ../..

include ${TOP}/make/config/arch.mk

SHARED = 0
LIBRARY = flow
FILES = \
	Field Grid Interpolator Rake Solution \
	VTFieldLine VTPathLine VTStreamLine VTStreakLine VTTimeLine \
	VTTimeVaryingFieldLine VaporFlow VectorMatrix

#LIB_DEFS = vdf.def
  
ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"FLOW_EXPORTS"
endif


ifdef   EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH) -I$(NETCDF_INC_PATH)
endif

LIBRARIES = common vdf


include ${TOP}/make/config/base.mk

