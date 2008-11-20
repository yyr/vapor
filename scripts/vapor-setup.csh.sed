#! /bin/csh -f

set arch = ARCH
set build64 = BUILD_64_BIT
set root = INSTALL_PREFIX_DIR
set share = INSTALL_SHAREDIR
set expat = EXPAT_LIB_PATH
set netcdf = NETCDF_LIB_PATH
set qt = QT_LIB_PATH
set idl = BUILD_IDL_WRAPPERS

set auxlib = ""
if ("$expat" != "") then
	set auxlib = "${auxlib}:$expat"
endif
if ("$netcdf" != "") then
	set auxlib = "${auxlib}:$netcdf"
endif
if ("$qt" != "") then
	set auxlib = "${auxlib}:$qt"
endif

setenv VAPOR_HOME $root
setenv VAPOR_SHARE $share

if !($?PATH) then
    setenv PATH "$root/bin"
else
    setenv PATH "$root/bin:$PATH"
endif

if ( "$arch" == "Darwin" ) then
	if !($?DYLD_LIBRARY_PATH) then
	    setenv DYLD_LIBRARY_PATH "$root/lib$auxlib"
	else
	    setenv DYLD_LIBRARY_PATH "$root/lib${auxlib}:$DYLD_LIBRARY_PATH"
	endif
else if ( "$arch" == "AIX" ) then
	if !($?LIBPATH) then
	    setenv LIBPATH "$root/lib$auxlib"
	else
	    setenv LIBPATH "$root/lib${auxlib}:$LIBPATH"
	endif
else
	if !($?LD_LIBRARY_PATH) then
	    setenv LD_LIBRARY_PATH "$root/lib$auxlib"
	else
	    setenv LD_LIBRARY_PATH "$root/lib${auxlib}:$LD_LIBRARY_PATH"
	endif
endif

if ( "$arch" == "IRIX64" ) then
	if ( "$build64" == 1 ) then
		if !($?LD_LIBRARY64_PATH) then
			setenv LD_LIBRARY64_PATH "$root/lib${auxlib}"
		else
			setenv LD_LIBRARY64_PATH "$root/lib${auxlib}:$LD_LIBRARY64_PATH"
		endif
	else
		if !($?LD_LIBRARYN32_PATH) then
			setenv LD_LIBRARYN32_PATH "$root/lib${auxlib}"
		else
			setenv LD_LIBRARYN32_PATH "$root/lib${auxlib}:$LD_LIBRARYN32_PATH"
		endif
	endif
endif

#if !($?MANPATH) then
#	if ( "$arch" == "Linux" ) then
#		setenv MANPATH "$root/man":`man -w`
#	else
#		setenv MANPATH "$root/man"
#	endif
#else
#    setenv MANPATH "$root/man:${MANPATH}"
#endif

if ( "$idl" == 1 ) then
	if !($?IDL_DLM_PATH) then
		setenv IDL_DLM_PATH "$root/lib:<IDL_DEFAULT>"
	else
		setenv IDL_DLM_PATH "$root/lib:$IDL_DLM_PATH"
	endif
endif
