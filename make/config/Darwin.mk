G++-INCLUDE-DIR = /usr/include/g++
CXX = c++ -fno-common
CC = cc -fno-common

CXXFLAGS          += -DDARWIN -Wall -Wno-format -Wno-sign-compare  -fPIC -arch i386
CXX_RELEASE_FLAGS += -O3 -DNDEBUG
CXX_DEBUG_FLAGS   += -g

CFLAGS            += -DDARWIN -Wall -Wno-format -fPIC -arch i386
C_RELEASE_FLAGS   += -O3 -DNDEBUG
C_DEBUG_FLAGS     += -g

LDFLAGS           += 
LD_RELEASE_FLAGS  += 
LD_DEBUG_FLAGS    += 

PROFILEFLAGS = -pg -a

CAT = cat
AS = as
LEX = flex -t
LEXLIB = -ll
YACC = bison -y -d
LD = $(CXX)
AR = ar
ARCREATEFLAGS = cr
RANLIB = ranlib
LN = ln -s
MKDIR = mkdir -p
RM = rm -f
CP = cp
MAKE = make
NOWEB = noweb
LATEX = latex
BIBTEX = bibtex
DVIPS = dvips -t letter
GHOSTSCRIPT = gs
LIBPREFIX = lib
ifndef	DLLSUFFIX
DLLSUFFIX = .dylib
endif
LIBSUFFIX = .a
OBJSUFFIX = .o
MV = mv
SHARED_LDFLAGS += -dynamiclib -current_version $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_RELEASE) -compatibility_version $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_RELEASE) -undefined dynamic_lookup
PERL = perl
PYTHON = python

INSTALL_EXEC = /usr/bin/install -m 0755
INSTALL_NONEXEC = /usr/bin/install -m 0644

CLD_EXCLUDE_LIBS =  -exclude ^/usr/lib -exclude ^/usr/X11 -exclude ^/System
#CLD_INCLUDE_LIBS =  -include libexpat -include libXinerama
CLD_INCLUDE_LIBS =  -include libexpat 
