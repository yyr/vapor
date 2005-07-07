# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

ifdef	QTDIR
QTLIB = -L$(QTDIR)/lib
endif
QTLIB += -lqt-mt

ifdef	OGL_LIB_PATH
OGLLIBS = -L$(OGL_LIB_PATH)
else
OGLLIBS = -L/usr/lib
endif
OGLLIBS += -lGLU -lGL  -lX11

ifdef   EXPAT_LIB_PATH
EXPATLIB := -L$(EXPAT_LIB_PATH)
endif
EXPATLIB += -lexpat


G++-INCLUDE-DIR = /usr/include/g++

ifeq	($(HAVE_INTEL_COMPILERS),1)
CXX = icpc
CC = icc
else
CXX = g++
CC = gcc
endif

# Mike Houston reports 20-30% speed-ups with these compiler flags on
# P4/Xeon systems:
#-O3 -DNDEBUG -fno-strict-aliasing -fomit-frame-pointer -fexpensive-optimizations -falign-functions=4 -funroll-loops -malign-double -fprefetch-loop-arrays -march=pentium4 -mcpu=pentium4 -msse2 -mfpmath=sse 

#CXXFLAGS          += -DLINUX -Wall -Werror

ifdef	HAVE_INTEL_COMPILERS

CXXFLAGS          += -DLINUX -D__USE_LARGEFILE64
CXX_RELEASE_FLAGS += -O -DNDEBUG
CXX_DEBUG_FLAGS   += -g

CFLAGS            += -DLINUX -D__USE_LARGEFILE64
C_RELEASE_FLAGS   += -O
C_DEBUG_FLAGS     += -g

else

CXXFLAGS          += -DLINUX -Wall -Wno-sign-compare -D__USE_LARGEFILE64
CXX_RELEASE_FLAGS += -O3 -DNDEBUG -fno-strict-aliasing
CXX_DEBUG_FLAGS   += -g

#CFLAGS            += -DLINUX -Wall -Werror -Wmissing-prototypes -Wsign-compare
CFLAGS            += -DLINUX -Wall -Wmissing-prototypes -Wno-sign-compare -D__USE_LARGEFILE64
C_RELEASE_FLAGS   += -O3 -DNDEBUG -fno-strict-aliasing
C_DEBUG_FLAGS     += -g

endif

ifeq ($(MACHTYPE),ia64)
CXXFLAGS          += -fPIC
CFLAGS          += -fPIC

endif

ifeq ($(MACHTYPE),x86_64)
LDFLAGS           += -L/usr/X11R6/lib64
CXXFLAGS          += -fPIC
CFLAGS            += -fPIC
else
LDFLAGS           += -L/usr/X11R6/lib
endif

LDFLAGS			+= -lrt

LD_RELEASE_FLAGS  += 
LD_DEBUG_FLAGS    += 

ifeq ($(MACHTYPE), alpha)
CXXFLAGS          += -mieee 
CFLAGS            += -mieee
endif

ifeq ($(MACHTYPE), mips)
ifeq ($(shell ls /proc/ps2pad), /proc/ps2pad)
PLAYSTATION2      =   1
CXXFLAGS          += -DPLAYSTATION2
CFLAGS            += -DPLAYSTATION2
endif
endif

PROFILEFLAGS = -pg -a

CAT = cat
AS = as
LEX = flex -t
LEXLIB = -ll
YACC = bison -y -d
LD = $(CXX)
AR = ar
ARCREATEFLAGS = cr
RANLIB = true
LN = ln -s
MKDIR = mkdir -p
RM = rm -f
CP = cp
#MAKE = gmake -s
MAKE = gmake 
NOWEB = noweb
LATEX = latex
BIBTEX = bibtex
DVIPS = dvips -t letter
GHOSTSCRIPT = gs
LIBPREFIX = lib
DLLSUFFIX = .so
LIBSUFFIX = .a
OBJSUFFIX = .o
MV = mv
SHARED_LDFLAGS += -shared 
PERL = perl
PYTHON = python
JGRAPH = /u/eldridge/bin/IRIX/jgraph
PS2TIFF = pstotiff.pl
PS2TIFFOPTIONS = -alpha -mag 2
PS2PDF = ps2pdf

MPI_CC = mpicc
MPI_CXX = mpiCC
MPI_LDFLAGS =

INSTALL_EXEC = /usr/bin/install -m 0755
INSTALL_NONEXEC = /usr/bin/install -m 0644
