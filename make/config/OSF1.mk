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
CXX = cxx
CC = cc

CXXFLAGS          += -DOSF1 -I/usr/local/include -Wall -Werror -pthread
CXX_RELEASE_FLAGS += -O3 -DNDEBUG
CXX_DEBUG_FLAGS   += -g

CFLAGS            += -DOSF1 -I/usr/local/include -Wall -Werror -pthread
C_RELEASE_FLAGS   += -O3 -DNDEBUG
C_DEBUG_FLAGS     += -g

LDFLAGS           += -L/usr/X11R6/lib
LD_RELEASE_FLAGS  += 
LD_DEBUG_FLAGS    += 

ifeq ($(MACHTYPE), alpha)
CXXFLAGS          += -mieee -mcpu=ev67
CFLAGS            += -mieee -mcpu=ev67
endif

PROFILEFLAGS = -pg -a

MATH = 1

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
MAKE = gmake -s
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

