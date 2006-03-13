# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.


PROJECT = vapor

###########################
# LEAVE THESE THINGS ALONE!
###########################

include $(TOP)/make/config/arch.mk

PLATFORM = $(ARCH)_$(MACHTYPE)

#
#	Prevent local makefiles which source this file from overwriting 
#	these variables
#
CXXFLAGS :=
CFLAGS :=
LDFLAGS :=

include $(TOP)/make/config/$(ARCH).mk


ifdef PROGRAM
BUILDDIR := $(TOP)/targets/$(PLATFORM)/built/$(PROGRAM)
else
ifdef LIBRARY
BUILDDIR := $(TOP)/targets/$(PLATFORM)/built/$(LIBRARY)
else
BUILDDIR := dummy_builddir
endif
endif


CR_CC = $(CC)
CR_CXX = $(CXX)



OBJDIR := $(BUILDDIR)
DEPDIR := $(BUILDDIR)/dependencies
BINDIR := $(TOP)/targets/$(PLATFORM)/bin
INCDIR := $(TOP)/include
DSO_DIR := $(BINDIR)
#DSO_DIR := $(TOP)/targets/$(PLATFORM)/lib/
DOCDIR := $(TOP)/targets/common/doc

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


ifdef QT
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
FILES += $(UI_FILES)
QTTEMPS += $(UI_FILES:%=$(UI_DIR)/%.cpp)
QTTEMPS += $(UI_FILES:%=$(UI_DIR)/%.h)
QTTEMPS += $(UI_FILES:%=$(MOC_DIR)/moc_%.cpp)
#rules to generate .cpp from UI_FILES, using UIC:
 
%.h: %.ui
	@$(UIC) $< -o $@
%.cpp: %.h %.ui
	@$(UIC) $*.ui  -i $< -o $@

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
MOC_UIS := $(UI_FILES:%=moc_%)
FILES += $(MOC_UIS)

#Provide rules to do the MOCing of the files generated by UIC:

$(MOC_DIR)/moc_%.cpp : $(UI_DIR)/%.h
	@$(MAKE_MOCDIR)
	@$(MOC) $< -o $@ 

INCLUDE_DIRS += -I$(QTDIR)/include -I$(UI_DIR)
#END OF ifdef QT
endif

ifdef TEST
FILES := $(TEST)
endif

FILES_NOTDIR := $(notdir $(FILES))
DEPS    := $(addprefix $(DEPDIR)/, $(FILES_NOTDIR))
DEPS    := $(addsuffix .depend, $(DEPS))
OBJS    := $(addprefix $(OBJDIR)/, $(FILES_NOTDIR))
OBJS    := $(addsuffix $(OBJSUFFIX), $(OBJS))
INCS    := $(addprefix $(INCDIR)/$(PROJECT)/, $(HEADER_FILES))
INCS    := $(addsuffix .h, $(INCS))
HEADER_FILES    := $(addsuffix .h, $(HEADER_FILES))
ifdef LIBRARY
ifdef SHARED
	LIB_TARGET := $(addprefix $(DSO_DIR)/, $(LIBPREFIX)$(LIBRARY)$(DLLSUFFIX))
	AIXDLIBNAME := $(addprefix $(OBJDIR)/, $(LIBPREFIX)$(LIBRARY)$(OBJSUFFIX))
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


PRINT_COMMAND := lpr


ifdef LESSWARN
WARN_STRING = (NOWARN)
else
CFLAGS += $(FULLWARN)
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

ifdef WINDOWS
LDFLAGS += /incremental:no 
#LDFLAGS += /pdb:none
ifeq ($(RELEASE), 0)
LDFLAGS += /debug
endif
LDFLAGS := /link $(LDFLAGS)
endif

INCLUDE_DIRS += $(MAKEFILE_INCLUDE_DIRS)
CFLAGS += -D$(ARCH) $(INCLUDE_DIRS)
CXXFLAGS += -D$(ARCH) -DQT_THREAD_SUPPORT $(INCLUDE_DIRS)

#
#	Append flags which may have been set from the makefile
#
CXXFLAGS += $(MAKEFILE_CXXFLAGS)
CFLAGS += $(MAKEFILE_CFLAGS)

ifndef SUBDIRS
all: arch $(PRECOMP) headers dep
recurse: $(PROG_TARGET) $(LIB_TARGET) done
else
ifdef PROGRAM
#if both subdirs and program, need to just link at the top level directory
all: subdirs arch $(PRECOMP) dep
recurse: $(PROG_TARGET) $(LIB_TARGET) done
else

all: subdirs

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
#LIBRARIES := $(foreach lib,$(LIBRARIES),$(TOP)/targets/$(PLATFORM)/bin/$(LIBPREFIX)$(lib)$(LIBSUFFIX))
LIBRARIES := $(foreach lib,$(LIBRARIES),$(lib)$(LIBSUFFIX))
LIBRARIES += $(foreach lib,$(PERSONAL_LIBRARIES),$(TOP)/targets/$(PLATFORM)/lib/$(LIBPREFIX)$(SHORT_TARGET_NAME)_$(lib)_copy$(LIBSUFFIX))
#LIBRARIES := $(LIBRARIES:$(DLLSUFFIX)=$(LIBSUFFIX))
STATICLIBRARIES :=

LDFLAGS += "/LIBPATH:$(DSO_DIR)" 
else

ifeq ($(ARCH), IRIX64)
LDFLAGS += -L$(DSO_DIR) -rpath $(DSO_DIR)
else
ifeq ($(ARCH), Linux)
LDFLAGS += -L$(DSO_DIR) -Xlinker -rpath -Xlinker $(DSO_DIR)
else
LDFLAGS += -L$(DSO_DIR) 
endif
endif


STATICLIBRARIES := $(foreach lib,$(LIBRARIES),$(wildcard $(TOP)/lib/$(PLATFORM)/lib$(lib)$(LIBSUFFIX)))
LIBRARIES := $(foreach lib,$(LIBRARIES),-l$(lib))
LIBRARIES += $(foreach lib,$(PERSONAL_LIBRARIES),-l$(SHORT_TARGET_NAME)_$(lib)_copy)
P_LIB_FILES := $(foreach lib,$(PERSONAL_LIBRARIES),$(TOP)/lib/$(PLATFORM)/$(LIBPREFIX)$(SHORT_TARGET_NAME)_$(lib)_copy$(DLLSUFFIX) )
endif

LDFLAGS += $(MAKEFILE_LDFLAGS)

dep: $(DEPS)
	$(MAKE) $(PARALLELMAKEFLAGS) recurse INCLUDEDEPS=1

headers: $(HEADER_FILES)
ifdef SUBDIRS
	for i in $(SUBDIRS); do $(MAKE) -C $$i headers; done
else
ifdef	HEADER_FILES
$(HEADER_FILES): 
	@$(RM) $@
	@$(LN) $(INCDIR)/$(PROJECT)/$@ $@

install:: all
	@$(ECHO) "Installing header files $(HEADER_FILES) in $(INSTALL_INCDIR)."
	@$(MAKE_INSTALL_INCDIR)
	@for i in $(HEADER_FILES); do $(INSTALL_NONEXEC) $(INCDIR)/$(PROJECT)/$$i $(INSTALL_INCDIR); done
endif
endif

# XXX this target should also have a dependency on all static Cr libraries.
# For example: crserver depends on libcrstate.a and libcrserverlib.a
$(PROG_TARGET): $(OBJS) $(STATICLIBRARIES)
ifdef PROGRAM
	@$(ECHO) "Linking $(PROGRAM) for $(PLATFORM)"
ifdef WINDOWS
	@$(CR_CXX) $(OBJS) /Fe$(PROG_TARGET)$(EXESUFFIX) $(LIBRARIES) $(LDFLAGS)
else
ifdef BINUTIL_LINK_HACK
ifdef PERSONAL_LIBRARIES
	@$(PERL) $(TOP)/buildutils/trans_undef_symbols.pl $(PROGRAM) $(TOP)/built/$(PROGRAM)/$(PLATFORM) $(P_LIB_FILES)
endif
endif
	$(CR_CXX) $(OBJS) -o $(PROG_TARGET)$(EXESUFFIX) $(LDFLAGS) $(LIBRARIES)
endif

endif

$(LIB_TARGET): $(OBJS) $(LIB_DEFS) $(STATICLIBRARIES)
ifdef LIBRARY
	@$(ECHO) "Linking $@"
ifdef WINDOWS
ifdef SHARED
	@$(LD) $(SHARED_LDFLAGS) /Fe$(LIB_TARGET) $(OBJS) $(LIBRARIES) $(LIB_DEFS) $(LDFLAGS)
else
	@LIB.EXE /nologo $(OBJS) $(LIBRARIES) /OUT:$(LIB_TARGET)
endif #shared
else #windows
ifdef SHARED
ifdef AIXSHAREDLIB
	@$(ECHO) "AIX shared obj link "
	rm -f $(AIXDLIBNAME)
	rm -f $(LIB_TARGET)
	$(CXX) -qmkshrobj -G -o shr.o $(OBJS) $(LDFLAGS) $(LIBRARIES)
	ar $(ARCREATEFLAGS) $(LIB_TARGET) shr.o
	rm -f shr.o 
else #aixsharedlib
ifdef BINUTIL_LINK_HACK
ifdef PERSONAL_LIBRARIES
	@$(PERL) $(TOP)/buildutils/trans_undef_symbols.pl $(SHORT_TARGET_NAME) $(TOP)/built/$(SHORT_TARGET_NAME)/$(PLATFORM) $(P_LIB_FILES)
endif
endif
	$(LD) $(SHARED_LDFLAGS) -o $(LIB_TARGET) $(OBJS) $(LDFLAGS) $(LIBRARIES)
endif #aixsharedlib
else #shared
	@$(AR) $(ARCREATEFLAGS) $@ $(OBJS)
	@$(RANLIB) $@
endif #shared
endif #windows

#	@$(CP) $(LIB_TARGET) $(DSO_DIR)
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

$(DEPDIR)/%.depend: $(UI_DIR)/%.cpp
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

$(OBJDIR)/%.o: $(UI_DIR)/%.cpp Makefile
	@$(ECHO) "UIC and Compiling $<"
	$(CR_CXX) -o $@ -c $(CXXFLAGS) $<

$(OBJDIR)/%.obj: $(UI_DIR)/%.cpp Makefile
	@$(ECHO) "UIC and Compiling $<"
	$(CR_CXX) /Fo$@ -c $(CXXFLAGS) $<

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
ifdef LIBRARY
	@$(ECHO) "Removing $(LIB_TARGET) for $(PLATFORM)."
	@$(RM) $(LIB_TARGET) $(QTTEMPS)
else
ifdef PROGRAM
	@$(ECHO) "Removing $(PROGRAM) for $(PLATFORM)."
	@$(RM) $(PROGRAM)
	@$(RM) $(BINDIR)/$(PROGRAM) $(QTTEMPS)
endif
	@$(ECHO) "Removing dependency files (if any)"
	@$(RM) $(DEPDIR)/*.depend
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
INSTALL_DOCDIR = $(INSTALL_PREFIX_DIR)/doc
INSTALL_INCDIR = $(INSTALL_PREFIX_DIR)/include/$(PROJECT)

define MAKE_INSTALL_BINDIR
	if test ! -d $(INSTALL_BINDIR); then $(MKDIR) $(INSTALL_BINDIR); fi
endef

define MAKE_INSTALL_LIBDIR
	if test ! -d $(INSTALL_LIBDIR); then $(MKDIR) $(INSTALL_LIBDIR); fi
endef

define MAKE_INSTALL_DOCDIR
	if test ! -d $(INSTALL_DOCDIR); then $(MKDIR) $(INSTALL_DOCDIR); fi
endef

define MAKE_INSTALL_INCDIR
	if test ! -d $(INSTALL_INCDIR); then $(MKDIR) $(INSTALL_INCDIR); fi
endef

ifdef SUBDIRS
install:: 
	@for i in $(SUBDIRS); do $(MAKE) -C $$i install; done
endif

ifdef COMPILE_ONLY
install:: all
else
install:: all
ifdef LIBRARY
	@$(ECHO) "Installing library $(LIBRARY) in $(INSTALL_LIBDIR)."
	@$(MAKE_INSTALL_LIBDIR)
	$(INSTALL_EXEC) $(LIB_TARGET) $(INSTALL_LIBDIR)
else
ifdef PROGRAM
	@$(ECHO) "Installing program $(PROGRAM) in $(INSTALL_BINDIR)."
	@$(MAKE_INSTALL_BINDIR)
	$(INSTALL_EXEC) $(PROG_TARGET) $(INSTALL_BINDIR)
endif
endif
endif


doc: headers FRC
ifdef SUBDIRS
	@for i in $(SUBDIRS); do $(MAKE) -C $$i doc; done

endif

FRC:


# Make CRNAME.tar.gz and CRNAME.zip files
CRNAME = cr-1.1
tarball: clean
#	remove old files
	-rm -rf ../$(CRNAME)
	-rm -f ../$(CRNAME).tar.gz
	-rm -f ../$(CRNAME).zip
#	make copy of cr directory
	cp -r ../cr ../$(CRNAME)
#	remove CVS files and other unneeded files
	-find ../$(CRNAME) -name CVS -exec rm -rf '{}' \;
	rm -f ../$(CRNAME)/mothership/server/crconfig.py
	rm -rf ../$(CRNAME)/built
	rm -rf ../$(CRNAME)/bin
	rm -rf ../$(CRNAME)/lib
#	make tarball and zip file
	cd .. ; tar cvf $(CRNAME).tar $(CRNAME) ; gzip $(CRNAME).tar
	cd .. ; zip -r $(CRNAME).zip $(CRNAME)
