TOP = ../..

include ${TOP}/make/config/prebase.mk

SHARED = 0
LIBRARY = common
FILES = MyBase OptionParser EasyThreads CFuncs Base64 Version PVTime
HEADER_FILES = MyBase OptionParser EasyThreads CFuncs Base64 Version PVTime

ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"COMMON_EXPORTS"
endif

#LIB_DEFS = common.def

include ${TOP}/make/config/base.mk

