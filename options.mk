# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

#
#
#

# This file lets one set various compile-time options.


# Set RELEASE to 1 to compile with optimizations and without debug info.
RELEASE=0


# Set THREADSAFE to 1 if you want thread safety for parallel applications.
THREADSAFE=0

# Set DEBUG to 1 if you want diagnostic messages turned on
DEBUG=0


# Set EXPAT_INC_PATH to the directory where expat.h may be found if not
# in a standard location
#
ifeq ($(ARCH),WIN32)
EXPAT_INC_PATH="B:/Expat-1.95.8/Source/lib/"
endif
ifeq ($(ARCH),IRIX64)
EXPAT_INC_PATH=/usr/freeware/include
endif
ifeq ($(ARCH),Linux)
EXPAT_INC_PATH=/usr/include
endif

# Set EXPAT_LIB_PATH to the directory where libexpat.* may be found if not
# in a standard location
#
ifeq ($(ARCH),WIN32)
EXPAT_LIB_PATH="B:/Expat-1.95.8/libs/libexpat.lib"
endif
ifeq ($(ARCH),IRIX64)
EXPAT_LIB_PATH=/usr/freeware/lib32
endif
ifeq ($(ARCH),Linux)
EXPAT_LIB_PATH=/usr/lib
endif


# Set QTDIR to the root of the QT directory
#
ifndef $(QTDIR)
ifeq ($(ARCH),WIN32)
QTDIR = C:/Qt/3.3.4
endif
ifeq ($(ARCH),Linux)
QTDIR = /fs/local/apps/qt
#QTDIR = /usr/local/apps/qt
endif 
ifeq ($(ARCH),IRIX64)
QTDIR=/usr/local/apps/qt
endif
endif

# Set VOLUMIZER_ROOT to the root of the volumizer directory
#VOLUMIZER_ROOT = 

# Set IDL_INC_PATH to the top of the IDL external include directory
IDL_INC_PATH = /usr/local/apps/rsi/idl_5.6/external/include


# Set to 1 if intel compilers are available on linux systems
HAVE_INTEL_COMPILERS = 1
