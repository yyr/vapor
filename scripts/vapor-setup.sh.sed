#! /bin/sh 

arch=ARCH
build64=BUILD_64_BIT
root=INSTALL_PREFIX_DIR
share=INSTALL_SHAREDIR
expat=EXPAT_LIB_PATH
netcdf=NETCDF_LIB_PATH
qt=QT_LIB_PATH
idl=BUILD_IDL_WRAPPERS

auxlib=""
if [ -n "$expat" ]
then
    auxlib="${auxlib}:$expat"
fi

if [ -n "$netcdf" ]
then
    auxlib="${auxlib}:$netcdf"
fi

if [ -n "$qt" ]
then
    auxlib="${auxlib}:$qt"
fi

VAPOR_HOME="$root"; export VAPOR_HOME
VAPOR_SHARE="$share"; export VAPOR_SHARE

if [ -z "${PATH}" ]
then
    PATH="$root/bin"; export PATH
else
    PATH="$root/bin:$PATH"; export PATH
fi

if [ "$arch" = "Darwin" ]
then
	if [ -z "${DYLD_LIBRARY_PATH}" ]
	then
	    DYLD_LIBRARY_PATH="$root/lib${auxlib}"; export DYLD_LIBRARY_PATH
	else
	    DYLD_LIBRARY_PATH="$root/lib${auxlib}:$DYLD_LIBRARY_PATH"; export DYLD_LIBRARY_PATH
	fi
else
if [ "$arch" = "AIX" ]
then
	if [ -z "${LIBPATH}" ]
	then
	    LIBPATH="$root/lib${auxlib}"; export LIBPATH
	else
	    LIBPATH="$root/lib${auxlib}:$LIBPATH"; export LIBPATH
	fi
else
	if [ -z "${LD_LIBRARY_PATH}" ]
	then
	    LD_LIBRARY_PATH="$root/lib${auxlib}"; export LD_LIBRARY_PATH
	else
	    LD_LIBRARY_PATH="$root/lib${auxlib}:$LD_LIBRARY_PATH"; export LD_LIBRARY_PATH
	fi
fi

if [ "$arch" = "IRIX64" ]
then
	if [ "$build64" -eq 1 ]
	then
		if [ -z "${LD_LIBRARY64_PATH}" ]
		then
			LD_LIBRARY64_PATH="$root/lib${auxlib}"; export LD_LIBRARY64_PATH
		else
			LD_LIBRARY64_PATH="$root/lib${auxlib}:$LD_LIBRARY64_PATH"; export LD_LIBRARY64_PATH
		fi
	else
		if [ -z "${LD_LIBRARYN32_PATH}" ]
		then
			LD_LIBRARYN32_PATH="$root/lib${auxlib}"; export LD_LIBRARYN32_PATH
		else
			LD_LIBRARYN32_PATH="$root/lib${auxlib}:$LD_LIBRARYN32_PATH"; export LD_LIBRARYN32_PATH
		fi
	fi
fi

#if [ -z "${MANPATH}" ]
#then
#	if [ "$arch" = "Linux" ]
#	then
#		MANPATH="$root/man":$(man -w); export MANPATH
#	else
#		MANPATH="$root/man"; export MANPATH
#	fi
#else
#    MANPATH="$root/man:${MANPATH}"; export MANPATH
#fi

if [ "$idl" -eq 1 ]
then
	if [ -z "${IDL_DLM_PATH}" ]
	then
		IDL_DLM_PATH="$root/lib:<IDL_DEFAULT>"; export IDL_DLM_PATH
	else
		IDL_DLM_PATH="$root/lib:$IDL_DLM_PATH"; export IDL_DLM_PATH
	fi
fi
