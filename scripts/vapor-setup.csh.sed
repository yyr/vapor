#! /bin/csh -f

set arch = SYSTEM_ARCH
set root = INSTALL_PREFIX_DIR
set idl = BUILD_IDL_WRAPPERS
set bindir = INSTALL_BINDIR
set mandir = INSTALL_MANDIR
set sharedir = INSTALL_SHAREDIR
set lib_search_dirs = LIB_SEARCH_DIRS

setenv VAPOR_HOME $root
setenv GRIB_DEFINITION_PATH $sharedir/grib_api/definitions
setenv PROJ_LIB $sharedir/proj

if ($?LD_LIBRARY_PATH) then
	echo "######################## WARNING ##############################"
	echo ""
	echo "The LD_LIBRARY_PATH environment variable is set."
	echo "Some VAPOR applications may fail to run correctly."
	echo "To un-set LD_LIBRARY_PATH execute: unsetenv LD_LIBRARY_PATH"
	echo ""
	echo "######################## WARNING ##############################"
endif
	


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
