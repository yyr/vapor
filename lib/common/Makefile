TOP = ../..

include ${TOP}/make/config/arch.mk

SHARED = 0
LIBRARY = common
FILES = MyBase OptionParser EasyThreads CFuncs
HEADER_FILES = MyBase OptionParser EasyThreads CFuncs 

ifeq ($(ARCH), WIN32)
MAKEFILE_CXXFLAGS += /D"COMMON_EXPORTS"
endif

#LIB_DEFS = common.def

include ${TOP}/make/config/base.mk

