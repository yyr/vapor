# If you want 64 bit support, then uncomment the following line:
#IRIX_64BIT = 1


ifdef   QTDIR
QTLIB = -L$(QTDIR)/lib
endif
QTLIB += -lqt

ifdef   OGL_LIB_PATH
OGLLIBS = -L$(OGL_LIB_PATH)
endif
OGLLIBS += -lGLU -lGL  -lX11

ifdef   EXPAT_LIB_PATH
EXPATLIB = -L$(EXPAT_LIB_PATH)
endif
EXPATLIB += -lexpat

G++-INCLUDE-DIR = /usr/include/g++
CXX = CC
CC = cc

CXX_RELEASE_FLAGS += -O2 -DNDEBUG
CXX_DEBUG_FLAGS   += -g

ifdef IRIX_64BIT
CXXFLAGS          += -DIRIX_64BIT -64 -DIRIX 
CFLAGS            += -DIRIX_64BIT -64 -DIRIX
LDFLAGS           += -64 -lm 
SHARED_LDFLAGS = -shared -64
else
CXXFLAGS          += -n32 -DIRIX 
CFLAGS            += -n32 -DIRIX
LDFLAGS           += -n32 -lm 
SHARED_LDFLAGS = -shared -n32
endif
C_RELEASE_FLAGS   += -O2 -DNDEBUG
C_DEBUG_FLAGS     += -g

LD_RELEASE_FLAGS  += 
LD_DEBUG_FLAGS    +=


PROFILEFLAGS = -pg -a

CAT = cat
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
PARALLELMAKEFLAGS =
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
PERL = perl
PYTHON = python
JGRAPH = /u/eldridge/bin/IRIX/jgraph
PS2TIFF = pstotiff.pl
PS2TIFFOPTIONS = -alpha -mag 2
PS2PDF = ps2pdf

MPI_CC = cc
MPI_CXX = CC
MPI_LDFLAGS = -lmpi
SLOP += so_locations

