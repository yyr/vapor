TOP = ../..

include ${TOP}/make/config/arch.mk

PROGRAM = vdfcreate
FILES = vdfcreate

LDFLAGS += $(EXPATLIB)

LIBRARIES = common vdf

ifdef   EXPAT_INC_PATH
CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

include ${TOP}/make/config/base.mk

