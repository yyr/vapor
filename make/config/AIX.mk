
ifdef   QTDIR
QTLIB = -L$(QTDIR)/lib
endif
QTLIB += -lqt

ifdef   OGL_LIB_PATH
OGLLIBS = -L$(OGL_LIB_PATH)
endif
OGLLIBS += -lGLU -lGL  -lX11

ifndef EXPAT_LIB_PATH
EXPAT_LIB_PATH = /usr/freeware/lib64
endif

EXPATLIB += -L$(EXPAT_LIB_PATH)
EXPATLIB += -lexpat


G++-INCLUDE-DIR = /usr/include/g++
#CXX = g++
CXX=xlC_r
#CC = gcc
CC=xlc_r
CXXFLAGS += -DAIX  -I $(TOP)/include
CFLAGS += -qcpluscmt -DAIX -I $(TOP)/include 
#LDFLAGS += -L/usr/X11R6/lib -lX11 -lpthread


DEBUGFLAGS = -g
RELEASEFLAGS = -DNDEBUG
PROFILEFLAGS = -pg -a

AS = as
LEX = flex -t
LEXLIB = -ll
#YACC = bison -y -d
YACC = yacc
LD = $(CXX)
AR = ar
ARCREATEFLAGS = cr
RANLIB = true
LN = ln -s
MKDIR = mkdir -p
RM = rm -f
CP = cp
#MAKE = /scratch/gnu/bin/make -s
MAKE = /usr/local/bin/gmake
NOWEB = noweb
LATEX = latex
BIBTEX = bibtex
DVIPS = dvips -t letter
GHOSTSCRIPT = gs
LIBPREFIX = lib
DLLSUFFIX = .a
LIBSUFFIX = .a
OBJSUFFIX = .o
MV = mv
PERL = perl
PYTHON = python
JGRAPH = /u/eldridge/bin/IRIX/jgraph
PS2TIFF = pstotiff.pl
PS2TIFFOPTIONS = -alpha -mag 2
PS2PDF = ps2pdf

CAT = cat
MPI_CC = mpicc
MPI_CXX = mpiCC
MPI_LDFLAGS =
AIXSHAREDLIB=y
# SHARED=y
SHARED_LDFLAGS = -L $(TOP)/lib/$(ARCH) -lX11 -lXmu -lpthread
ifdef OPENGL
    SHARED_LDFLAGS += -lGL
endif

INSTALL_EXEC = $(TOP)/buildutils/install-sh -c -m 0755
INSTALL_NONEXEC = $(TOP)/buildutils/install-sh -c -m 0644
