TOP = ../..

include ${TOP}/make/config/arch.mk

PROGRAM = test_amrtree
FILES = test_amrtree


LIBRARIES = common vdf

MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
MAKEFILE_LDFLAGS += $(EXPATLIB)

include ${TOP}/make/config/base.mk

