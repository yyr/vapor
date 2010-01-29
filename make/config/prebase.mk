ABS_TOP = $(abspath $(TOP))

# abspath doesn't work on older versions of gmake
ifeq ($(ABS_TOP), )
	ABS_TOP = $(shell cd $(TOP) && pwd)
endif

SHARED = 1

include $(TOP)/make/config/arch.mk
include $(TOP)/make/config/$(ARCH).mk
include $(TOP)/options.mk
