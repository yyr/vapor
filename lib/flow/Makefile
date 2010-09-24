TOP = ../..

include ${TOP}/make/config/prebase.mk

LIBRARY = flow
FILES = \
	Field Grid Interpolator Rake Solution flowlinedata\
	VTFieldLine VTPathLine VTStreamLine VTStreakLine VTTimeLine \
	VTTimeVaryingFieldLine VaporFlow VectorMatrix

HEADER_FILES = VaporFlow flowlinedata

#LIB_DEFS = vdf.def
  
ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"FLOW_EXPORTS"
endif

include ${TOP}/make/config/base.mk

