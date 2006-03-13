#
# NCAR local site config file for VAPOR. 
#

ifneq ($(ARCH), WIN_NT)
ifneq ($(ARCH), WIN_98)
HOST=$(shell hostname)
endif
endif

BUILD_VOLUMIZER = 1

ifeq ($(RELEASE_ENV), 1)
RELEASE = 1
endif

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

ifdef	BUILD_64_BIT_ENV
BUILD_64_BIT = 1
endif

EXPAT_INC_PATH=/usr/freeware/include
EXPAT_LIB_PATH=/usr/freeware/lib32
NETCDF_INC_PATH=/fs/local/include
NETCDF_LIB_PATH=/fs/local/lib
HDF5_INC_PATH=/fs/local/include
HDF5_LIB_PATH=/fs/local/lib
BUILD_IDL_WRAPPERS = 1

IDL_INC_PATH = /fs/local/apps/rsi/idl/external/include/
QTDIR=/fs/local/apps/qt-3.3.4

ifeq ($(BUILD_64_BIT), 1)

EXPAT_LIB_PATH=/usr/freeware/lib64
NETCDF_LIB_PATH=/fs/local/64/lib
HDF5_LIB_PATH=/fs/local/64/lib

IDL_INC_PATH = /fs/local/64/apps/rsi/idl/external/include/
QTDIR=/fs/local/64/apps/qt-3.3.4

endif

endif

#
# Linux
#
ifeq ($(ARCH),Linux)

ifdef	HAVE_INTEL_COMPILERS_ENV
HAVE_INTEL_COMPILERS = 1
endif

EXPAT_INC_PATH=/usr/include
EXPAT_LIB_PATH=/usr/lib
NETCDF_INC_PATH=/fs/local/include
NETCDF_LIB_PATH=/fs/local/lib
HDF5_INC_PATH=/fs/local/include
HDF5_LIB_PATH=/fs/local/lib
BUILD_IDL_WRAPPERS = 1
IDL_INC_PATH=/fs/local/apps/rsi/idl/external/include
QTDIR = /fs/local/apps/qt-3.3.4

ifeq ($(MACHTYPE),ia64)
BUILD_64_BIT = 1
BUILD_IDL_WRAPPERS = 0
endif

ifeq ($(MACHTYPE),x86_64)
BUILD_64_BIT = 1
endif

ifeq ($(BUILD_64_BIT), 1)
IDL_INC_PATH=/fs/local/64/apps/rsi/idl/external/include
QTDIR = /fs/local/64/apps/qt-3.3.4
NETCDF_INC_PATH=/fs/local/64/include
NETCDF_LIB_PATH=/fs/local/64/lib
HDF5_INC_PATH=/fs/local/64/include
HDF5_LIB_PATH=/fs/local/64/lib
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
HDF5_INC_PATH=/usr/local/include
HDF5_LIB_PATH=/usr/local/lib
BUILD_IDL_WRAPPERS = 0
BUILD_GUI	= 0
IDL_INC_PATH=/dev/null
QTDIR = /dev/null

endif
