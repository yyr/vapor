TOP = ../..

include ${TOP}/make/config/arch.mk

PROGRAM = raw2vdf
FILES = raw2vdf


LDFLAGS += $(EXPATLIB)

ifdef	EXPAT_INC_PATH
CXXFLAGS += -I$(EXPAT_INC_PATH)
endif

LIBRARIES = common vdf 


include ${TOP}/make/config/base.mk

