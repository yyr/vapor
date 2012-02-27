#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libpiovdc, 1.0, vapor@ucar.edu)
AC_CONFIG_FILES([Makefile vdf/Makefile common/Makefile])
AM_INIT_AUTOMAKE([no-dependencies])
AC_PROG_RANLIB
AC_FC_WRAPPERS
#check if debug mode is enabled
AC_ARG_ENABLE([debug],[--enable-debug		Enable debug outputs])

#check to see if netcdf or pnetcdf was chose for the output

AC_ARG_WITH([netcdf],[--with-netcdf	     Use NetCDF + HDF5 for output], [with_netcdf="$withval"], [with_netcdf=no])

AC_ARG_WITH([pnetcdf],[--with-pnetcdf		 Use PNetCDF for output (RECOMMENDED)], [with_pnetcdf="$withval"], [with_pnetcdf=no])

if test "$with_pnetcdf" = "no" -a "$with_netcdf" = "no"
then
	AC_MSG_WARN([Neither PNetCDF nor NetCDF4 were chosen for output, defaulting to standard PnetCDF location (it should be in the standard path)])
fi

if test "$with_pnetcdf" != "no" -a "$with_netcdf" != "no"
then
	    AC_MSG_ERROR([Both PnetCDF and NetCDF4 were chosen for output, only one can be used])
fi

m4_include([m4/expat.m4])
m4_include([m4/mpi.m4])

INCLUDES="-I../../include"

# Checks for programs used in building libraries

AC_PROG_CC
AC_PROG_CXX
AX_MPI

# Checks for the netcdf/pnetcdf libraries

#NETCDF has a lot of dependencies, check for them only if netcdf was requested
if test "$with_netcdf" != "no"
then
	AC_MSG_WARN([NetCDF4 support is provided, but not recommended. Locations for the szip, zlib, and hdf5 libraries must be manually provided using a --with-lib=<location> option])

#m4_include([m4/zlib.m4])
m4_include([m4/hdf5.m4])
m4_include([m4/szip.m4])

#AM_WITH_ZLIB
AM_WITH_SZIP
AM_WITH_HDF5

dnl	verify libnetcdf is in the lib directory

	#AC_CHECKING([for libnetcdf in $with_netcdf/lib])
#	if test ! -f "$with_netcdf/lib/libnetcdf.a" && ! -f "$with_netcdf/lib/libnetcdf.so" && ! -f "$with_netcdf/lib/libnetcdf.dylib" ; then
	#	AC_MSG_ERROR([Could not find libnetcdf in "$with_netcdf/lib"])	
#  	else
	#	AC_MSG_RESULT([yes])
		AC_CHECKING([for netcdf.h in $with_ncdf/include])
dnl		test for header file
		if test ! -f "$with_netcdf/include/netcdf.h"
		then		
			AC_MSG_ERROR([Could not find netcdf.h in "$with_netcdf/include])
		fi
		INCLUDES="$INCLUDES -I$with_netcdf/include"
		#LDFLAGS="$LDFLAGS -L$with_netcdf/lib"
		have_netcdf=yes
#		LIBS="-lnetcdf $LIBS"
		AC_MSG_RESULT([yes])
	#fi
fi

dnl PNETCDF check, by default we look for PNetCDF in the standard path if no --with arg is given

if test "$with_pnetcdf" = "no" -a "$with_netcdf" = "no"
then
	#no custom path, try to find in the standard paths
	AC_CHECK_HEADER(pnetcdf.h,
	AC_SEARCH_LIBS([ncmpi_close], [pnetcdf] ,[] , AC_MSG_ERROR([Could not find libpnetcdf in standard library paths])),
	AC_MSG_ERROR([Could not find pnetcdf.h in standard header paths]))
fi

if test "x$with_pnetcdf" != "xno"
then
dnl	verify libpnetcdf is in the lib directory
#	AC_CHECKING([for libpnetcdf in $with_pnetcdf/lib])
#	if test ! -f "$with_pnetcdf/lib/libpnetcdf.a" && ! -f "$with_pnetcdf/lib/libpnetcdf.so" && ! -f "$with_pnetcdf/lib/libpnetcdf.dylib" ; then
	#	AC_MSG_ERROR([Could not find libpnetcdf in "$with_pnetcdf/lib"])	
#  	else
	#	AC_MSG_RESULT([yes])
		AC_CHECKING([for pnetcdf.h in $with_pnetcdf/include])
dnl		test for header file
		if test ! -f "$with_pnetcdf/include/pnetcdf.h"
		then		
			AC_MSG_ERROR([Could not find pnetcdf.h in "$with_pnetcdf/include])
		fi
		INCLUDES="$INCLUDES -I$with_pnetcdf/include"
		LDFLAGS="$LDFLAGS -L$with_pnetcdf/lib"
		LIBS="-lpnetcdf $LIBS"
		have_pnetcdf=yes
		AC_MSG_RESULT([yes])
#	fi
fi


AC_CHECKING([for libpiovdc build requirements])

AM_WITH_EXPAT

AC_MSG_RESULT([yes])

AC_SUBST(INCLUDES)
AM_CONDITIONAL([DEBUG], test "$enable_debug" == "yes")
AM_CONDITIONAL([PNETCDF], test "$have_pnetcdf" == "yes")
AM_CONDITIONAL([NETCDF], test "$have_netcdf" == "yes")
AC_OUTPUT
