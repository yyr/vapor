#
# NCAR local site config file for VAPOR. 
#

ifneq ($(ARCH), WIN_NT)
ifneq ($(ARCH), WIN_98)
HOST=$(shell hostname)
endif
endif

BUILD_AMR_SUPPORT = 0
BUILD_VOLUMIZER = 1

#
#	get VAPOR_INSTALL_DIR from the enviornment
#
ifdef	VAPOR_INSTALL_DIR
INSTALL_PREFIX_DIR = $(VAPOR_INSTALL_DIR)
else
INSTALL_PREFIX_DIR = /tmp
endif

#
# Windows
#
ifeq ($(ARCH),WIN32)

EXPAT_INC_PATH="C:/Expat-1.95.8/Source/lib/"
EXPAT_LIB_PATH="C:/Expat-1.95.8/libs/"
BUILD_IDL_WRAPPERS = 1
IDL_INC_PATH="C:/RSI/IDL61/external/include"
NETCDF_INC_PATH="C:/NCDF/include"
NETCDF_LIB_PATH="C:/NCDF/lib"
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
NETCDF_INC_PATH=/fs/local/include
NETCDF_LIB_PATH=/fs/local/lib
BUILD_IDL_WRAPPERS = 1

IDL_INC_PATH = /fs/local/apps/rsi/idl_6.1/external/include/
QTDIR=/fs/local/apps/qt-3.3.4

ifdef BUILD64

EXPAT_LIB_PATH=/usr/freeware/lib64
NETCDF_LIB_PATH=/fs/local/64/lib

IDL_INC_PATH = /fs/local/64/apps/rsi/idl_6.1/external/include/
QTDIR=/fs/local/64/apps/qt-3.3.4

endif

endif

#
# Linux
#
ifeq ($(ARCH),Linux)


EXPAT_INC_PATH=/usr/include
EXPAT_LIB_PATH=/usr/lib
NETCDF_INC_PATH=/fs/local/include
NETCDF_LIB_PATH=/fs/local/lib
BUILD_IDL_WRAPPERS = 1
IDL_INC_PATH=/fs/local/apps/rsi/idl_6.1/external/include
QTDIR = /fs/local/apps/qt-3.3.4

ifeq ($(MACHTYPE),ia64)
BUILD64 = 1
BUILD_IDL_WRAPPERS = 0
endif

ifeq ($(MACHTYPE),x86_64)
BUILD64 = 1
BUILD_GUI = 0
endif

ifdef	BUILD64
IDL_INC_PATH=/fs/local/apps/rsi/idl_6.1/external/include
QTDIR = /fs/local/64/apps/qt-3.3.4
NETCDF_INC_PATH=/fs/local/64/include
NETCDF_LIB_PATH=/fs/local/64/lib
endif

endif

#
# AIX
#
ifeq ($(ARCH),AIX)

EXPAT_INC_PATH=/home/bluesky/clyne/include
EXPAT_LIB_PATH=/home/bluesky/clyne/lib
NETCDF_INC_PATH=/usr/local/include
NETCDF_LIB_PATH=/usr/local/lib
BUILD_IDL_WRAPPERS = 0
BUILD_GUI	= 0
IDL_INC_PATH=/dev/null
QTDIR = /dev/null

endif
