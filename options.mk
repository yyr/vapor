# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

# This file lets one set various compile-time options.

#
#
#


# Installation prefix directory. Vapor executables will be installed
# in INSTALL_PREFIX_DIR/bin, libraries in INSTALL_PREFIX_DIR/lib
# etc,.
INSTALL_PREFIX_DIR=/usr/local


# Set RELEASE to 1 to compile with optimizations and without debug info.
RELEASE=0


# Set THREADSAFE to 1 if you want thread safety for parallel applications.
THREADSAFE=0

# Set DEBUG to 1 if you want diagnostic messages turned on
DEBUG=0

# SET BUILD64 to 1 if you want to force the building of 64 bit binaries.
# Currently only has effect under the IRIX OS
BUILD_64_BIT=0


# Set EXPAT_INC_PATH to the directory where 'expat.h' may be found if not
# in a standard location
#
EXPAT_INC_PATH=


# Set EXPAT_LIB_PATH to the directory where 'libexpat.*' may be found 
# if not in a standard location
#
EXPAT_LIB_PATH=


# Set QTDIR to the root of the QT directory where the directories 'bin',
# 'lib', and 'include' may be found, if not in a standard location.
#
QTDIR = 

# Set VOLUMIZER_ROOT to the root of the volumizer directory
VOLUMIZER_ROOT = 


# Set to 1 if intel compilers are available on linux systems
#HAVE_INTEL_COMPILERS = 


# Set to 1 if you have IDL installed on your system and you would
# like to build the VAPoR IDL commands
#
HAVE_IDL = 0

# If HAVE_IDL is 1, set to path to IDL include directory. This is the 
# path to the directory that contains the file "idl_export.h"
#
IDL_INC_PATH=

#
#	If the file `site.mk' exists, include it. It contains site-specific
#	(host or platform specific) make variables that may override 
#	values defined above
#
-include $(TOP)/site.mk
