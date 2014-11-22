#! /bin/sh 

arch=SYSTEM_ARCH
root=INSTALL_PREFIX_DIR
idl=BUILD_IDL_WRAPPERS
bindir=INSTALL_BINDIR
mandir=INSTALL_MANDIR
lib_search_dirs=LIB_SEARCH_DIRS

VAPOR_HOME="$root"; export VAPOR_HOME

if [ -z "${PATH}" ]
then
    PATH="$bindir"; export PATH
else
    PATH="$bindir:$PATH"; export PATH
fi

if [ -z "${MANPATH}" ]
then
	if [ "$arch" = "AIX" ]
	then
		MANPATH="$mandir"; export MANPATH
	else
		MANPATH="$mandir":$(man -w); export MANPATH
	fi
else
    MANPATH="$mandir:${MANPATH}"; export MANPATH
fi

if [ "$idl" -eq 1 ]
then
	if [ -z "${IDL_DLM_PATH}" ]
	then
		IDL_DLM_PATH="${lib_search_dirs}:<IDL_DEFAULT>"; export IDL_DLM_PATH
	else
		IDL_DLM_PATH="${lib_search_dirs}:$IDL_DLM_PATH"; export IDL_DLM_PATH
	fi
fi
