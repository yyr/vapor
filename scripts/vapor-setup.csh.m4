#! /bin/csh 

set arch = ARCH
set build64 = BUILD_64_BIT
set root = INSTALL_PREFIX_DIR
set expat = EXPAT_LIB_PATH
set netcdf = NETCDF_LIB_PATH

set auxlib = ""
if ("$expat" != "") then
	set auxlib = "${auxlib}:$expat"
endif
if ("$netcdf" != "") then
	set auxlib = "${auxlib}:$netcdf"
endif

if !($?PATH) then
    setenv PATH "$root/bin"
else
    setenv PATH "$root/bin:$PATH"
endif

if !($?LD_LIBRARY_PATH) then
    setenv LD_LIBRARY_PATH "$root/lib$auxlib"
else
    setenv LD_LIBRARY_PATH "$root/lib${auxlib}:$LD_LIBRARY_PATH"
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

if !($?MANPATH) then
	if ( "$arch" == "Linux" ) then
		setenv MANPATH "$root/man":$(man -w)
	else
		setenv MANPATH "$root/man"
	endif
else
    setenv MANPATH "$root/man:${MANPATH}"
endif
