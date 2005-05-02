#
# NCAR local site config file for VAPOR. 
#


ifneq ($(ARCH), WIN_NT)
ifneq ($(ARCH), WIN_98)
HOST=$(shell hostname)
endif
endif

#
#	get VAPOR_INSTALL_DIR from the enviornment
#
ifdef	VAPOR_INSTALL_DIR
INSTALL_PREFIX_DIR = $(VAPOR_INSTALL_DIR)
else
INSTALL_PREFIX_DIR = /tmp
endif

DO_FS_PATH=0
ifeq ($(HOST), nomad)
DO_FS_PATH=1
endif
ifeq ($(HOST), newcyclone)
DO_FS_PATH=1
endif

#
# Windows
#
ifeq ($(ARCH),WIN32)

EXPAT_INC_PATH="B:/Expat-1.95.8/Source/lib/"
EXPAT_LIB_PATH="B:/Expat-1.95.8/libs/libexpat.lib"
HAVE_IDL = 1
IDL_INC_PATH="C:/RSI/IDL61/external/include"
QTDIR = C:/Qt/3.3.4

endif


#
# IRIX
#
ifeq ($(ARCH),IRIX64)

ifdef	BUILD64
BUILD_64_BIT = 1
endif

EXPAT_INC_PATH=/usr/freeware/include
EXPAT_LIB_PATH=/usr/freeware/lib32
HAVE_IDL = 1

ifneq ($(DO_FS_PATH), 1)
IDL_INC_PATH = /usr/local/apps/rsi/idl_6.0/external/include/
QTDIR=/usr/local/apps/qt
else
IDL_INC_PATH = /fs/local/apps/rsi/idl_6.1/external/include/
QTDIR=/fs/local/apps/qt-3.3.2
endif

ifdef BUILD64

EXPAT_LIB_PATH=/usr/freeware/lib64

ifneq ($(DO_FS_PATH), 1)
IDL_INC_PATH = /usr/local/apps/rsi/idl_6.0/external/include/
QTDIR=/usr/local/apps64/qt
else
IDL_INC_PATH = /fs/local/64/apps/rsi/idl_6.1/external/include/
QTDIR=/fs/local/64/apps/qt-3.3.2
endif

endif

endif

#
# Linux
#
ifeq ($(ARCH),Linux)

EXPAT_INC_PATH=/usr/include
EXPAT_LIB_PATH=/usr/lib
HAVE_IDL = 1
IDL_INC_PATH=/fs/local/apps/rsi/idl_6.0/external/include
QTDIR = /fs/local/apps/qt-3.3.2
HAVE_INTEL_COMPILERS = 1


ifdef	BUILD64
IDL_INC_PATH=/fs/local/64/apps/rsi/idl_6.0/external/include
QTDIR = /fs/local/64/apps/qt-3.3.2
endif

endif
