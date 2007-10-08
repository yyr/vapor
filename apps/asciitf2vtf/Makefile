TOP = ../..

include ${TOP}/make/config/prebase.mk

PROGRAM = asciitf2vtf
FILES = asciitf2vtf

ifdef   EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

#LIB_DEFS = vdf.def

LIBRARIES = params vdf common 

include ${TOP}/make/config/base.mk

