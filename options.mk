# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

# This file lets one set various compile-time options.

#
#
#


# Installation prefix directory. Vapor executables will be installed
# in $(INSTALL_PREFIX_DIR)/bin, libraries in $(INSTALL_PREFIX_DIR)/lib
# etc,.
#
INSTALL_PREFIX_DIR=/usr/local

# Set RELEASE to 1 to compile with optimizations and without debug info.
RELEASE=0

# Set DEBUG to 1 if you want diagnostic messages turned on
DEBUG=0

# Set BENCHMARK or PROFILE to 1 for volume rendering framerate diagnostics
BENCHMARK=0
PROFILING=0

# Set SPHERICAL_GRID to 1 to enable spherical grid rendering proof-of-concept
# code.
SPHERICAL_GRID=0

# Set EXPAT_INC_PATH to the directory where 'expat.h' may be found if not
# in a standard location. expat.h is part of the Expat XML Parser, available
# from http://expat.sourceforge.net/
#
EXPAT_INC_PATH=


# Set EXPAT_LIB_PATH to the directory where 'libexpat.*' may be found 
# if not in a standard location. libexpat is part of the Expat XML 
# Parser, available from http://expat.sourceforge.net/
#
EXPAT_LIB_PATH=


# Set NETCDF_INC_PATH to the directory where 'netcdf.h' may be found if not
# in a standard location. netcdf.h is part of Unidata's NetCDF (network
# Common Data Form), available from 
# http://www.unidata.ucar.edu/software/netcdf/
#
NETCDF_INC_PATH=

# Set NETCDF_LIB_PATH to the directory where 'libnetcdf.*' may be found 
# if not in a standard location. libnetcdf is part of Unidata's NetCDF (network
# Common Data Form), available from 
# http://www.unidata.ucar.edu/software/netcdf/
#
NETCDF_LIB_PATH=


# Set QTDIR to the root of the QT directory where the directories 'bin',
# 'lib', and 'include' may be found, if not in a standard location. Qt 
# refers to Trolltech's Qt, available (with some amount of hunting) from
# http://www.trolltech.com, and when possible, from the vapor
# web site: www.vapor.ucar.edu.
#
QTDIR =

# Set QT_LIB_PATH to the directory where "libqt-mt.*" may be found if 
# ***NOT*** under $(QTDIR)/lib. This macro, if set, overrides the 
# the default location, $(QTDIR)/lib, and may be needed on platforms
# with mixed word size support (Linux32 and Linux64).
#
QT_LIB_PATH = 


# Set to 1 if you have SGI Volumizer installed on your system and you would
# like to compile vapor's Volumizer rendering engine. Volumizer is a 
# licensed product available, for fee, from www.sgi.com. Volumizer is NOT
# required by vapor.
#
BUILD_VOLUMIZER = 1

# Set VOLUMIZER_ROOT to the root of the volumizer directory
# (Only needed if BUILD_VOLUMIZER is 1)
#
VOLUMIZER_ROOT = 


# Set to 1 if you have IDL installed on your system and you would
# like to build the VAPoR IDL commands. IDL refers to RSI's IDL, available
# for fee from www.rsinc.com. IDL is not required by vapor, however the 
# analysis and data processing capabilities of vapor are greatly limitted
# without IDL
#
BUILD_IDL_WRAPPERS = 0

# Set to path to IDL include directory. 
# This is the # path to the directory that contains the file "idl_export.h"
# (Only needed if BUILD_IDL_WRAPPERS is 1)
#
IDL_INC_PATH=

# Set to 1 if you want the VAPoR GUI to be built. Otherwise only the 
# VAPoR libraries and support utilities are compiled
#
BUILD_GUI = 1

# Set to 1 if you want to add support for Adaptive Mesh Refinement grids
#
BUILD_AMR_SUPPORT = 0

# Set HDF5_INC_PATH to the directory where 'hdf5.h' may be found if not
# in a standard location (Only needed if BUILD_AMR_SUPPORT is 1). hdf5.h is
# part of NCSA's HDF5 package, available from http://hdf.ncsa.uiuc.edu/HDF5/.
#
HDF5_INC_PATH=


# Set HDF5_LIB_PATH to the directory where 'libhdf5.*' may be found 
# if not in a standard location (Only needed if BUILD_AMR_SUPPORT is 
# 1). libhdf5 is part of NCSA's HDF5 package, available 
# from http://hdf.ncsa.uiuc.edu/HDF5/.
#
HDF5_LIB_PATH=




##
##
##	PLATFORM SPECIFIC MACROS
##
##


##
##	Irix
##

# SET BUILD_64_BIT to 1 if you want to force the building of 64 bit binaries.
# Currently only has effect under the IRIX OS
#
BUILD_64_BIT=0


##
##	Linux
##

# Set to 1 if intel compilers are available on linux systems and you
# wish to compile with them instead of gcc
#
HAVE_INTEL_COMPILERS = 

#
#	If the file `site.mk' exists, include it. It contains site-specific
#	(host or platform specific) make variables that may override 
#	values defined above. The site.mk file is NOT part of the vapor
#	distribution. But you may define one that sets the variables above
#	based on host name, OS, etc.
#
-include $(TOP)/site.mk
