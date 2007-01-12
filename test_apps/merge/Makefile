TOP = ../..

include ${TOP}/make/config/prebase.mk

PROGRAM = test_merge
FILES = test_merge


LIBRARIES = common vdf

MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
MAKEFILE_LDFLAGS += $(EXPATLIB)

include ${TOP}/make/config/base.mk

