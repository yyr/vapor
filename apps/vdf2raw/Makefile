TOP = ../..

include ${TOP}/make/config/arch.mk

PROGRAM = vdf2raw
FILES = vdf2raw


MAKEFILE_LDFLAGS += $(EXPATLIB)

LIBRARIES = common vdf

ifdef   EXPAT_INC_PATH
MAKEFILE_CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

include ${TOP}/make/config/base.mk

