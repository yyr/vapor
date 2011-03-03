# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.


PROJECT = vapor

###########################
# LEAVE THESE THINGS ALONE!
###########################

ifneq ($(ARCH), WIN32)
PLATFORM = $(ARCH)_$(MACHTYPE)
else
PLATFORM = $(ARCH)
endif

ifdef PROGRAM
BUILDDIR := $(TOP)/targets/$(PLATFORM)/built/$(PROGRAM)
else
ifdef LIBRARY
BUILDDIR := $(TOP)/targets/$(PLATFORM)/built/$(LIBRARY)
else
BUILDDIR := dummy_builddir
endif
endif

VERSION :=  $(shell tr -d '[:space:]' < $(TOP)/Version | sed 's/\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\)/\1/')
VERSION_MAJOR :=  $(shell tr -d '[:space:]' < $(TOP)/Version | sed 's/\([0-9][0-9]*\)\.[0-9][0-9]*\.[0-9][0-9]*/\1/')
VERSION_MINOR :=  $(shell tr -d '[:space:]' < $(TOP)/Version | sed 's/[0-9][0-9]*\.\([0-9][0-9]*\)\.[0-9][0-9]*/\1/')
VERSION_RELEASE :=  $(shell tr -d '[:space:]' < $(TOP)/Version | sed 's/[0-9][0-9]*\.[0-9][0-9]*\.\([0-9][0-9]*\)/\1/')

VERSION_APP := $(PROJECT)-$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_RELEASE)


CR_CC = $(CC)
CR_CXX = $(CXX)
OGL_LIB = GL
GLU_LIB = GLU

ifndef  NETCDF_LIBS
NETCDF_LIBS = netcdf
endif


ifneq ($(QT_FRAMEWORK), 1) 
QT_LIB = QtOpenGL QtGui QtCore  
endif

OBJDIR := $(BUILDDIR)
DEPDIR := $(BUILDDIR)/dependencies
INCDIR := $(TOP)/include
DOCDIR := $(TOP)/targets/common/doc

#
# For make install we relink libraries and executables in the installation
# directory. This way embedded library paths (e.g. rpath) are correct
#
ifdef MAKE_INSTALL
BINDIR = $(INSTALL_PREFIX_DIR)/bin
DSO_DIR = $(INSTALL_PREFIX_DIR)/lib
else
DSO_DIR := $(ABS_TOP)/targets/$(PLATFORM)/bin
BINDIR := $(TOP)/targets/$(PLATFORM)/bin
endif

INCLUDE_DIRS := -I$(INCDIR) -I. -I$(INCDIR)/vaporinternal

define MAKE_MOCDIR
	if test ! -d $(MOC_DIR); then $(MKDIR) $(MOC_DIR); fi
endef

define MAKE_OBJDIR
	if test ! -d $(OBJDIR); then $(MKDIR) $(OBJDIR); fi
endef

define MAKE_BINDIR
	if test ! -d $(BINDIR); then $(MKDIR) $(BINDIR); fi
endef

define MAKE_DEPDIR
	if test ! -d $(DEPDIR); then $(MKDIR) $(DEPDIR); fi
endef

define MAKE_DSODIR
	if test ! -d $(DSO_DIR); then $(MKDIR) $(DSO_DIR); fi
endef

define MAKE_INCDIR
	if test ! -d $(INCDIR)/$(PROJECT); then $(MKDIR) $(INCDIR)/$(PROJECT); fi
endef

define MAKE_DOCDIR
	if test ! -d $(DOCDIR); then $(MKDIR) $(DOCDIR); fi
endef


ifeq ($(QT), 1)
# Whenever QT is used in a directory, the Makefile must
# define MOC_DIR and UI_DIR.  MOC_DIR specifies the directory
# where the MOC intermediate files will go and UI_DIR contains *.ui files 
# as well as the resulting .cpp, .h files.
# in the UI_DIR
# All the names of .ui files must be specified in $(UI_FILES),
# and the names of .h files containing the Q_OBJECT macro must
# be specified in $(QT_HEADERS)
MOC = $(QTDIR)/bin/moc
UIC = $(QTDIR)/bin/uic


#Specify that the .ui files will result in objects
#FILES += $(UI_FILES)
#QTTEMPS += $(UI_FILES:%=$(UI_DIR)/%.cpp)
QTTEMPS += $(UI_FILES:%=$(UI_DIR)/%.h)
#QTTEMPS += $(UI_FILES:%=$(MOC_DIR)/moc_%.cpp)
#rules to generate .cpp from UI_FILES, using UIC:
 
%.h: %.ui
	@$(UIC) $< -o $@
#%.cpp: %.h %.ui
#	@$(UIC) $*.ui  -i $< -o $@

#specify that QT_HEADERS result in compiled moc_* files 
#Remove the directory, then add moc_
QT_HEAD_NOTDIR = $(notdir $(QT_HEADERS))
MOCS := $(QT_HEAD_NOTDIR:%=moc_%)
FILES += $(MOCS)

#rule to generate moc*.cpp from QT_HEADERS,
#placing intermediate result in MOC_DIR
QTTEMPS += $(MOCS:%=$(MOC_DIR)/%.cpp)

.SECONDARY: $(QTTEMPS) 

$(MOC_DIR)/moc_%.cpp : %.h
	@$(MAKE_MOCDIR)
	@$(MOC) $< -o $@ 

#specify that the UI_FILES also result in MOC intermediates: 
#MOC_UIS := $(UI_FILES:%=moc_%)
#FILES += $(MOC_UIS)

#Provide rules to do the MOCing of the files generated by UIC:

#$(MOC_DIR)/moc_%.cpp : $(UI_DIR)/%.h
#	@$(MAKE_MOCDIR)
#	@$(MOC) $< -o $@ 

QT_INCLUDE_DIRS += $(QTDIR)/include
QT_INCLUDE_DIRS += $(addprefix $(QTDIR)/include/, QtCore QtGui QtOpenGL)
QT_INCLUDE_DIRS += $(UI_DIR)

ifneq ($(QT_FRAMEWORK), 1)
QT_LIB_DIRS = $(QTDIR)/lib
endif

#END OF ifdef QT
endif

ifeq ($(BUILD_PYTHON), 1)
PY_INCLUDE_DIRS = $(addprefix $(PYTHONDIR)/include/python, $(PYTHONVERSION))
PY_INCLUDE_DIRS += $(addprefix $(join $(PYTHONDIR)/lib/python, $(PYTHONVERSION)), /site-packages/numpy/core/include)
endif

ifdef TEST
FILES := $(TEST)
endif

FILES_NOTDIR := $(notdir $(FILES))
DEPS    := $(addprefix $(DEPDIR)/, $(FILES_NOTDIR))
DEPS	+= $(addprefix $(DEPDIR)/, $(UI_FILES))
DEPS    := $(addsuffix .depend, $(DEPS))
OBJS    := $(addprefix $(OBJDIR)/, $(FILES_NOTDIR))
OBJS    := $(addsuffix $(OBJSUFFIX), $(OBJS))
INCS    := $(addprefix $(INCDIR)/$(PROJECT)/, $(HEADER_FILES))
INCS    := $(addsuffix .h, $(INCS))
HEADER_FILES    := $(addsuffix .h, $(HEADER_FILES))
ifdef LIBRARY
ifdef SHARED
ifeq ($(ARCH), Darwin)
	LIB_LINKERNAME = $(LIBPREFIX)$(LIBRARY)$(DLLSUFFIX)
	LIB_SONAME = $(LIBPREFIX)$(LIBRARY).$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_RELEASE)$(DLLSUFFIX)
else
	LIB_LINKERNAME = $(LIBPREFIX)$(LIBRARY)$(DLLSUFFIX)
	LIB_SONAME = $(LIB_LINKERNAME).$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_RELEASE)
endif
	LIB_REALNAME = $(LIB_SONAME)
	LIB_TARGET := $(addprefix $(DSO_DIR)/, $(LIB_REALNAME))
else
	LIB_TARGET := $(addprefix $(DSO_DIR)/, $(LIBPREFIX)$(LIBRARY)$(LIBSUFFIX))
endif
else
	LIB_TARGET := dummy_libname
endif

TEMPFILES := *~ \\\#*\\\# so_locations *.pyc tmpAnyDX.a tmpAnyDX.exp load.map shr.o *.pdb *.idb

ifdef COMPILE_ONLY
TARGET := $(OBJS)
else
ifdef PROGRAM
PROG_TARGET := $(BINDIR)/$(PROGRAM)
TARGET := $(PROGRAM)
SHORT_TARGET_NAME = $(PROGRAM)
else
PROG_TARGET := dummy_prog_target
endif
endif

ifdef LIBRARY
SHORT_TARGET_NAME = $(LIBRARY)
ifdef SHARED
TARGET := $(LIBPREFIX)$(LIBRARY)$(DLLSUFFIX)
else
TARGET := $(LIBPREFIX)$(LIBRARY)$(LIBSUFFIX)
endif
endif

ifndef TARGET
TARGET := NOTHING
endif

ifeq ($(INCLUDEDEPS), 1)
ifneq ($(DEPS)HACK, HACK)
include $(DEPS)
endif
endif


ifeq ($(RELEASE), 1)
CFLAGS += $(C_RELEASE_FLAGS)
CXXFLAGS += $(CXX_RELEASE_FLAGS)
LDFLAGS += $(LD_RELEASE_FLAGS)
RELEASE_STRING = (RELEASE)
RELEASE_FLAGS = "RELEASE=1"
else
ifdef PROFILE
CFLAGS += $(C_DEBUG_FLAGS) $(PROFILE_FLAGS)
CXXFLAGS += $(CXX_DEBUG_FLAGS) $(PROFILE_FLAGS)
LDFLAGS += $(LD_DEBUG_FLAGS) $(PROFILE_LAGS)
RELEASE_STRING = (PROFILE)
else
CFLAGS += $(C_DEBUG_FLAGS)
CXXFLAGS += $(CXX_DEBUG_FLAGS)
LDFLAGS += $(LD_DEBUG_FLAGS)
RELEASE_STRING = (DEBUG)
endif
endif

ifeq ($(DEBUG), 1)
CFLAGS += -DDEBUG
CXXFLAGS += -DDEBUG
endif

ifeq ($(BENCHMARK),1)
CFLAGS += -DBENCHMARKING
CXXFLAGS += -DBENCHMARKING
endif

ifeq ($(PROFILING),1)
CFLAGS += -DPROFILING
CXXFLAGS += -DPROFILING
endif

ifdef WINDOWS
LDFLAGS += /incremental:no 
ifeq ($(RELEASE), 0)
LDFLAGS += /debug
endif
LDFLAGS := /link $(LDFLAGS)
endif

INCLUDE_DIRS += $(addprefix -I, $(INC_SEARCH_DIRS))

ifdef QT_INCLUDE_DIRS
INCLUDE_DIRS += $(addprefix -I, $(QT_INCLUDE_DIRS))
endif

ifdef	PY_INCLUDE_DIRS
INCLUDE_DIRS += $(addprefix -I, $(PY_INCLUDE_DIRS))
endif

INCLUDE_DIRS += $(MAKEFILE_INCLUDE_DIRS)
CFLAGS += -D$(ARCH) $(INCLUDE_DIRS)
#CXXFLAGS += -D$(ARCH) -DQT_THREAD_SUPPORT $(INCLUDE_DIRS)
CXXFLAGS += -D$(ARCH) -DQT_OPENGL_LIB -DQT_GUI_GUI_LIB -DQT_CORE_LIB -DQT_SHARED $(INCLUDE_DIRS)

#
#	Append flags which may have been set from the makefile
#
CXXFLAGS += $(MAKEFILE_CXXFLAGS)
CFLAGS += $(MAKEFILE_CFLAGS)

ifndef SUBDIRS
all:: arch $(PRECOMP) headers dep
recurse: $(PROG_TARGET) $(LIB_TARGET) done
else
ifdef PROGRAM
#if both subdirs and program, need to just link at the top level directory
all:: subdirs arch $(PRECOMP) dep
recurse: $(PROG_TARGET) $(LIB_TARGET) done
else

all:: subdirs

endif

SUBDIRS_ALL = $(foreach dir, $(SUBDIRS), $(dir).subdir)

subdirs: $(SUBDIRS_ALL)

$(SUBDIRS_ALL):
	@$(MAKE) -C $(basename $@) $(RELEASE_FLAGS)
endif

release:
	@$(MAKE) RELEASE=1

profile:
	@$(MAKE) PROFILE=1

done:
	@$(ECHO) "  Done!"
	@$(ECHO) ""

arch: 
	@$(ECHO) "-------------------------------------------------------------------------------"
ifdef BANNER
	@$(ECHO) "              $(BANNER)"
else
ifdef PROGRAM
	@$(ECHO) "              Building $(TARGET) for $(PLATFORM) $(RELEASE_STRING) $(STATE_STRING) $(PACK_STRING) $(UNPACK_STRING) $(VTK_STRING) $(WARN_STRING)"
endif
ifdef LIBRARY
	@$(ECHO) "              Building $(TARGET) for $(PLATFORM) $(RELEASE_STRING) $(STATE_STRING) $(PACK_STRING) $(UNPACK_STRING) $(VTK_STRING) $(WARN_STRING)"
endif
endif
	@$(ECHO) "-------------------------------------------------------------------------------"
ifneq ($(BUILDDIR), dummy_builddir)
	@$(MAKE_BINDIR)
	@$(MAKE_OBJDIR)
	@$(MAKE_DEPDIR)
	@$(MAKE_DSODIR)
	@$(MAKE_INCDIR)
	@$(MAKE_DOCDIR)
endif

ifdef WINDOWS
LIBRARIES := $(foreach lib,$(LIBRARIES),$(lib)$(LIBSUFFIX))
LIBRARIES += $(foreach lib,$(PERSONAL_LIBRARIES),$(TOP)/targets/$(PLATFORM)/lib/$(LIBPREFIX)$(SHORT_TARGET_NAME)_$(lib)_copy$(LIBSUFFIX))
STATICLIBRARIES :=

LD_SEARCH_FLAGS += $(addprefix /LIBPATH:, $(DSO_DIR))
ifdef	QT_LIB_DIRS
LD_SEARCH_FLAGS += $(addprefix /LIBPATH:, $(QT_LIB_DIRS))
endif
LD_SEARCH_FLAGS += $(addprefix /LIBPATH:, $(LIB_SEARCH_DIRS))

else

STATICLIBRARIES := $(foreach lib,$(LIBRARIES),$(wildcard $(TOP)/lib/$(PLATFORM)/lib$(lib)$(LIBSUFFIX)))
LIBRARIES := $(foreach lib,$(LIBRARIES),-l$(lib))
LIBRARIES += $(foreach lib,$(PERSONAL_LIBRARIES),-l$(SHORT_TARGET_NAME)_$(lib)_copy)

LD_SEARCH_FLAGS += $(addprefix -L, $(DSO_DIR))
ifdef	QT_LIB_DIRS
LD_SEARCH_FLAGS += $(addprefix -L, $(QT_LIB_DIRS))
endif
LD_SEARCH_FLAGS += $(addprefix -L, $(LIB_SEARCH_DIRS))

ifdef	RPATHFLAG
RPATHS += $(addprefix $(RPATHFLAG), $(DSO_DIR))
ifdef	QT_LIB_DIRS
RPATHS += $(addprefix $(RPATHFLAG), $(QT_LIB_DIRS))
endif
RPATHS += $(addprefix $(RPATHFLAG), $(LIB_SEARCH_DIRS))
endif

endif


LDFLAGS += $(MAKEFILE_LDFLAGS)

dep: $(DEPS)
	$(MAKE) -f $(word 1, $(MAKEFILE_LIST)) recurse INCLUDEDEPS=1

headers: $(HEADER_FILES)
ifdef SUBDIRS
	for i in $(SUBDIRS); do $(MAKE) -C $$i headers; done
else
ifdef	HEADER_FILES
$(HEADER_FILES)::
	@$(RM) $@
	@$(LN) $(INCDIR)/$(PROJECT)/$@ $@

install:: all
	@$(ECHO) "Installing header files $(HEADER_FILES) in $(INSTALL_INCDIR)."
	@$(MAKE_INSTALL_INCDIR)
	@for i in $(HEADER_FILES); do $(INSTALL_NONEXEC) $(INCDIR)/$(PROJECT)/$$i $(INSTALL_INCDIR); done

install-dep:: install

endif
endif

$(PROG_TARGET): $(OBJS) $(STATICLIBRARIES)
ifdef PROGRAM
	@$(ECHO) "Linking $(PROGRAM) for $(PLATFORM)"
ifdef WINDOWS
	@$(CR_CXX) $(OBJS) /Fe$(PROG_TARGET)$(EXESUFFIX) $(LIBRARIES) $(LDFLAGS) $(LD_SEARCH_FLAGS)
else
	$(CR_CXX) $(OBJS) -o $(PROG_TARGET)$(EXESUFFIX) $(LDFLAGS) $(RPATHS) $(LD_SEARCH_FLAGS) $(LIBRARIES)
endif

endif

$(LIB_TARGET): $(OBJS) $(STATICLIBRARIES)
ifdef LIBRARY
	@$(ECHO) "Linking $@"
ifdef WINDOWS
ifdef SHARED
	@$(LD) $(SHARED_LDFLAGS) /Fe$(LIB_TARGET) $(OBJS) $(LIBRARIES) 
else
	@LIB.EXE /nologo $(OBJS) $(LIBRARIES) /OUT:$(LIB_TARGET)
endif #shared
else #windows
ifdef SHARED
	$(LD) $(SHARED_LDFLAGS) -o $(LIB_TARGET) $(OBJS) $(LIBRARIES) $(RPATHS) $(LD_SEARCH_FLAGS)
	cd $(DSO_DIR); $(RM) $(LIB_LINKERNAME); $(LN) $(LIB_REALNAME) $(LIB_LINKERNAME)
else #shared
	$(AR) $(ARCREATEFLAGS) $@ $(OBJS)
	$(RANLIB) $@
endif #shared
endif #windows

endif #library


.SUFFIXES: .cpp .c .cxx .cc .C .s .l .h

%.cpp: %.l
	@$(ECHO) "Creating $@"
	@$(LEX) $< > $@

%.cpp: %.y
	@$(ECHO) "Creating $@"
	@$(YACC) $<
	@$(MV) y.tab.c $@

$(DEPDIR)/%.depend: %.cpp
	@$(MAKE_DEPDIR)
	@$(ECHO) "Rebuilding dependencies for $<"
	@$(PERL) $(TOP)/buildutils/fastdep.pl $(INCLUDE_DIRS) --obj-prefix='$(OBJDIR)/' --extra-target=$@ $< > $@

$(DEPDIR)/%.depend: $(UI_DIR)/%.h
	@$(MAKE_DEPDIR)
	@$(ECHO) "Rebuilding UI dependencies for $<"
	@$(PERL) $(TOP)/buildutils/fastdep.pl $(INCLUDE_DIRS) --obj-prefix='$(OBJDIR)/' --extra-target=$@ $< > $@

$(DEPDIR)/%.depend: $(MOC_DIR)/%.cpp
	@$(MAKE_DEPDIR)
	@$(ECHO) "Rebuilding UI dependencies for $<"
	@$(PERL) $(TOP)/buildutils/fastdep.pl $(INCLUDE_DIRS) --obj-prefix='$(OBJDIR)/' --extra-target=$@ $< > $@

$(DEPDIR)/%.depend: %.cxx
	@$(MAKE_DEPDIR)
	@$(ECHO) "Rebuilding dependencies for $<"
	@$(PERL) $(TOP)/buildutils/fastdep.pl $(INCLUDE_DIRS) --obj-prefix='$(OBJDIR)/' --extra-target=$@ $< > $@

$(DEPDIR)/%.depend: %.cc
	@$(MAKE_DEPDIR)
	@$(ECHO) "Rebuilding dependencies for $<"
	@$(PERL) $(TOP)/buildutils/fastdep.pl $(INCLUDE_DIRS) --obj-prefix='$(OBJDIR)/' --extra-target=$@ $< > $@

$(DEPDIR)/%.depend: %.C
	@$(MAKE_DEPDIR)
	@$(ECHO) "Rebuilding dependencies for $<"
	@$(PERL) $(TOP)/buildutils/fastdep.pl $(INCLUDE_DIRS) --obj-prefix='$(OBJDIR)/' --extra-target=$@ $< > $@

$(DEPDIR)/%.depend: %.c
	@$(MAKE_DEPDIR)
	@$(ECHO) "Rebuilding dependencies for $<"
	@$(PERL) $(TOP)/buildutils/fastdep.pl $(INCLUDE_DIRS) --obj-prefix='$(OBJDIR)/' --extra-target=$@ $< > $@

$(DEPDIR)/%.depend: %.s
	@$(MAKE_DEPDIR)
	@$(ECHO) "Rebuilding dependencies for $<"
	@$(PERL) $(TOP)/buildutils/fastdep.pl $(INCLUDE_DIRS) --obj-prefix='$(OBJDIR)/' --extra-target=$@ $< > $@

$(OBJDIR)/%.obj: %.cpp Makefile
	@$(ECHO) -n "Compiling "
	$(CR_CXX) /Fo$@ /c $(CXXFLAGS) $<

$(OBJDIR)/%.obj: %.c Makefile
	@$(ECHO) -n "Compiling "
	$(CR_CC) /Fo$@ /c $(CFLAGS) $<

$(OBJDIR)/%.o: %.cpp Makefile
	@$(ECHO) "Compiling $<"
	$(CR_CXX) -o $@ -c $(CXXFLAGS) $<

#$(OBJDIR)/%.o: $(UI_DIR)/%.cpp Makefile
#	@$(ECHO) "UIC and Compiling $<"
#	$(CR_CXX) -o $@ -c $(CXXFLAGS) $<

#$(OBJDIR)/%.obj: $(UI_DIR)/%.cpp Makefile
#	@$(ECHO) "UIC and Compiling $<"
#	$(CR_CXX) /Fo$@ -c $(CXXFLAGS) $<

$(OBJDIR)/%.o: $(MOC_DIR)/%.cpp Makefile
	@$(ECHO) "MOCing and Compiling $<"
	$(CR_CXX) -o $@ -c $(CXXFLAGS) $<

$(OBJDIR)/%.obj: $(MOC_DIR)/%.cpp Makefile
	@$(ECHO) "MOCing and Compiling $<"
	$(CR_CXX) /Fo$@ -c $(CXXFLAGS) $<

$(OBJDIR)/%.o: %.cxx Makefile
	@$(ECHO) "Compiling $<"
	$(CR_CXX) -o $@ -c $(CXXFLAGS) $<

$(OBJDIR)/%.o: %.cc Makefile
	@$(ECHO) "Compiling $<"
	$(CR_CXX) -o $@ -c $(CXXFLAGS) $<

$(OBJDIR)/%.o: %.C Makefile
	@$(ECHO) "Compiling $<"
	$(CR_CXX) -o $@ -c $(CXXFLAGS) $<

$(OBJDIR)/%.o: %.c Makefile
	@$(ECHO) "Compiling $<"
	$(CR_CC) -o $@ -c $(CFLAGS) $<

$(OBJDIR)/%.o: %.s Makefile
	@$(ECHO) "Assembling $<"
	$(AS) -o $@ $<

###############
# Other targets
###############

clean::
ifdef SUBDIRS
	@for i in $(SUBDIRS); do $(MAKE) -C $$i clean; done
endif
ifdef HEADER_FILES
	@$(RM) $(HEADER_FILES)
endif
ifdef LIBRARY
	@$(ECHO) "Removing all $(PLATFORM) object files for $(TARGET)."
else
ifdef PROGRAM
	@$(ECHO) "Removing all $(PLATFORM) object files for $(PROGRAM)."
endif
endif
	@$(RM) $(OBJS) $(TEMPFILES) 
#	@$(RM) $(INCS)
ifneq ($(SLOP)HACK, HACK)
	@$(ECHO) "Also blowing away:    $(SLOP)"
	@$(RM) $(SLOP)
endif


clobber:: clean
ifdef SUBDIRS
	@for i in $(SUBDIRS); do $(MAKE) -C $$i clobber; done
endif
	@$(ECHO) "Removing dependency files (if any)"
	@if test -d $(DEPDIR); then $(RM) $(DEPDIR)/*.depend; fi
ifdef LIBRARY
	@$(ECHO) "Removing $(LIB_TARGET) for $(PLATFORM)."
	@$(RM) $(LIB_TARGET) $(QTTEMPS)
else
ifdef PROGRAM
	@$(ECHO) "Removing $(PROGRAM) for $(PLATFORM)."
	@$(RM) $(PROGRAM)
	@$(RM) $(BINDIR)/$(PROGRAM) $(QTTEMPS)
endif
ifdef	HEADER_FILES
	@$(ECHO) "Removing links to header files"
	@$(RM) $(HEADER_FILES)
endif
endif

ifndef INSTALL_PREFIX_DIR
INSTALL_PREFIX_DIR = /usr/local
endif

INSTALL_BINDIR = $(INSTALL_PREFIX_DIR)/bin
INSTALL_LIBDIR = $(INSTALL_PREFIX_DIR)/lib
INSTALL_INCDIR = $(INSTALL_PREFIX_DIR)/include/$(PROJECT)
INSTALL_SHAREDIR = $(INSTALL_PREFIX_DIR)/share
INSTALL_MANDIR = $(INSTALL_PREFIX_DIR)/share/man
INSTALL_PLUGINSDIR = $(INSTALL_PREFIX_DIR)/plugins

define MAKE_INSTALL_BINDIR
	if test ! -d $(INSTALL_BINDIR); then $(MKDIR) $(INSTALL_BINDIR); fi
endef

define MAKE_INSTALL_LIBDIR
	if test ! -d $(INSTALL_LIBDIR); then $(MKDIR) $(INSTALL_LIBDIR); fi
endef

define MAKE_INSTALL_INCDIR
	if test ! -d $(INSTALL_INCDIR); then $(MKDIR) $(INSTALL_INCDIR); fi
endef

define MAKE_INSTALL_PLUGINSDIR
	if test ! -d $(INSTALL_PLUGINSDIR); then $(MKDIR) $(INSTALL_PLUGINSDIR); fi
endef

ifdef SUBDIRS

install:: 
	@for i in $(SUBDIRS); do $(MAKE) -C $$i install; done

install-dep:: install
	@for i in $(SUBDIRS); do $(MAKE) -C $$i install-dep; done

endif

ifdef COMPILE_ONLY
install:: all
install-dep:: install
else
install-dep:: 
install:: 
ifdef LIBRARY
	@$(ECHO) "Installing library $(LIBRARY) in $(INSTALL_LIBDIR)."
	@$(MAKE_INSTALL_LIBDIR)
	$(MAKE) -f $(word 1, $(MAKEFILE_LIST)) all MAKE_INSTALL=1
install-dep:: install
else
ifdef PROGRAM
	@$(ECHO) "Installing program $(PROGRAM) in $(INSTALL_BINDIR)."
	@$(MAKE_INSTALL_BINDIR)
	$(MAKE) -f $(word 1, $(MAKEFILE_LIST)) all MAKE_INSTALL=1

LDLIBPATHS += -ldlibpath $(DSO_DIR)
LDLIBPATHS += $(addprefix -ldlibpath , $(LIB_SEARCH_DIRS))

CLD_INCLUDE_FLAGS = $(addprefix -include , $(CLD_INCLUDE_LIBS))
CLD_EXCLUDE_FLAGS = $(addprefix -exclude , $(CLD_EXCLUDE_LIBS))
CLD_INCLUDE_FLAGS += $(addprefix -include ^, $(DSO_DIR))
CLD_INCLUDE_FLAGS += $(addprefix -include ^, $(LIB_SEARCH_DIRS))

install-dep:: install
	@$(ECHO) "Installing program $(PROGRAM) library dependencies in $(INSTALL_LIBDIR)."
	$(MAKE) -f $(word 1, $(MAKEFILE_LIST)) install-dep-helper MAKE_INSTALL=1

install-dep-helper::
	$(PERL) $(TOP)/buildutils/copylibdeps.pl -arch $(ARCH) $(LDLIBPATHS) $(CLD_EXCLUDE_FLAGS) $(CLD_INCLUDE_FLAGS) $(PROG_TARGET) $(INSTALL_LIBDIR)
ifeq ($(ARCH), Linux)
	@$(ECHO) "Removing rpaths from $(PROG_TARGET)"
	/usr/bin/patchelf --set-rpath "" $(PROG_TARGET)
endif

endif
endif
endif


doc: headers FRC
ifdef SUBDIRS
	@for i in $(SUBDIRS); do $(MAKE) -C $$i doc; done

endif

FRC:
