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
MAKEFILE_LDFLAGS += ../../targets/win32/bin/common.lib 
MAKEFILE_LDFLAGS += "mpr.lib"
MAKEFILE_LDFLAGS += ../../targets/win32/bin/vdf.lib
endif

MAKEFILE_LDFLAGS += $(EXPATLIB)


ifdef	EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

include ${TOP}/make/config/base.mk

