TOP = ../..

include ${TOP}/make/config/prebase.mk

SHARED = 0
LIBRARY = common
FILES = MyBase OptionParser EasyThreads CFuncs Base64 Version
HEADER_FILES = MyBase OptionParser EasyThreads CFuncs Base64 Version

ifeq ($(ARCH),WIN32)
MAKEFILE_CXXFLAGS += /D"COMMON_EXPORTS"
endif

#LIB_DEFS = common.def

include ${TOP}/make/config/base.mk

