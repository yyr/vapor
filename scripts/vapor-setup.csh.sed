#! /bin/csh -f

set arch = SYSTEM_ARCH
set root = INSTALL_PREFIX_DIR
set idl = BUILD_IDL_WRAPPERS
set bindir = INSTALL_BINDIR
set mandir = INSTALL_MANDIR
set lib_search_dirs = LIB_SEARCH_DIRS

setenv VAPOR_HOME $root

if !($?PATH) then
    setenv PATH "$bindir"
else
    setenv PATH "${bindir}:$PATH"
endif

if !($?MANPATH) then
	if ( "$arch" == "AIX" ) then
		setenv MANPATH "$mandir"
	else
		setenv MANPATH "${mandir}":`man -w`
	endif
else
    setenv MANPATH "${mandir}:${MANPATH}"
endif

if ( "$idl" == 1 ) then
	if !($?IDL_DLM_PATH) then
		setenv IDL_DLM_PATH "${lib_search_dirs}:<IDL_DEFAULT>"
	else
		setenv IDL_DLM_PATH "${lib_search_dirs}:$IDL_DLM_PATH"
	endif
endif
