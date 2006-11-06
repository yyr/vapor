# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.


ifneq ($(ARCH), WIN_NT)
ifneq ($(ARCH), WIN_98)
ARCH=$(shell uname | sed -e 's/-//g')

ifeq ($(ARCH), IRIX64)
MACHTYPE=$(shell uname -p)
endif

ifeq ($(ARCH), Darwin)
MACHTYPE=$(shell uname -p)
endif

ifndef	MACHTYPE
MACHTYPE=$(shell uname -m)
endif

endif
endif

ifeq ($(ARCH), WIN_NT)
ARCH=WIN32
endif
ifeq ($(ARCH), WIN_98)
ARCH=WIN32
endif
ifeq ($(ARCH),WIN32)
WIN32 = 1
endif


ifeq ($(ARCH), CYGWIN_NT5.0)
ARCH=WIN32
endif

ifeq ($(ARCH), CYGWIN_NT5.1)
ARCH=WIN32
endif

ifeq ($(MACHTYPE),i686)
MACHTYPE=i386
endif

ifeq ($(MACHTYPE),i586)
MACHTYPE=i386
endif

ifeq ($(MACHTYPE),i486)
MACHTYPE=i386
endif

include $(TOP)/options.mk

ECHO := echo
