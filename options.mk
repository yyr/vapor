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

# Set BENCHMARK or PROFILE to 1 for framerate diagnostics
BENCHMARK=0
PROFILING=0

# Set SPHERICAL_GRID to 1 to enable spherical grid rendering proof-of-concept
# code.
SPHERICAL_GRID=1

# Set LIB_SEARCH_DIRS to a list of directories containing libraries
# not on the default search path for the linker. Typically 3rd party 
# dependencies (e.g. netCDF) are not installed in a location where 
# linker normally checks. The linker will search the directories
# in the order specified.
#
LIB_SEARCH_DIRS = 

# Set INC_SEARCH_DIRS to a list of directories containing include files
# not on the default search path for the compiler. Typically 3rd party 
# dependencies (e.g. netCDF, IDL) are not installed in a location where 
# compiler normally checks. The compiler will search the directories
# in the order specified.
#
INC_SEARCH_DIRS = 

# Set to 1 if you want the VAPoR GUI to be built. Otherwise only the 
# VAPoR libraries and support utilities are compiled
#
BUILD_GUI = 1

# If BUILD_GUI is set to 1, set QTDIR to the root of the QT directory 
# where the sub directories 'bin', 'lib', and 'include' may be found. Qt 
# refers to Trolltech's Qt, available (with some amount of hunting) from
# http://www.trolltech.com, and when possible, from the vapor
# web site: www.vapor.ucar.edu.
#
QTDIR =

# If BUILD_GUI is set to 1 **and** this is a Mac system, set HAVE_QT_FRAMEWORK
# to 1 if your Qt libraries are built as Mac Frameworks, or to 0 if 
# your Qt libraries are built as older UNIX style dynamic libraries.
#
HAVE_QT_FRAMEWORK = 1

# Set to 1 if you have IDL installed on your system and you would
# like to build the VAPoR IDL commands. IDL refers to RSI's IDL, available
# for fee from www.rsinc.com. IDL is not required by vapor, however the 
# analysis and data processing capabilities of vapor are greatly limitted
# without IDL
#
BUILD_IDL_WRAPPERS = 0

# If BUILD_IDL_WRAPPERS is set to 1, set IDLDIR to the root of the 
# IDL directory 
# where the sub directories 'bin', 'lib', and 'include' may be found. 
#
IDLDIR =



# Set to 1 if you want to add support for Adaptive Mesh Refinement grids
#
BUILD_AMR_SUPPORT = 1


##
##
##	PLATFORM SPECIFIC MACROS
##
##


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
