TOP = ../..

include ${TOP}/make/config/arch.mk

PROGRAM = testimpexp
FILES = testimpexp


LDFLAGS += $(EXPATLIB)

LIBRARIES = common vdf

CXXFLAGS += -I$(EXPAT_INC_PATH)

include ${TOP}/make/config/base.mk

