TOP = ../..

include ${TOP}/make/config/arch.mk

PROGRAM = vdf2raw
FILES = vdf2raw


LDFLAGS += $(EXPATLIB)

LIBRARIES = common vdf

ifdef   EXPAT_INC_PATH
CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

include ${TOP}/make/config/base.mk

