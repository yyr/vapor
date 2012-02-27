AC_DEFUN([AM_WITH_HDF5],
[ AC_ARG_WITH(hdf5,
	      [  --with-hdf5=<location>      [Use to specify non-standard hdf5 root location]],
	      with_hdf5="$withval", with_hdf5=no)
	if test "$with_hdf5" = "no"
	then
dnl	no custom path, try to find in the standard paths
		AC_CHECK_HEADER(hdf5.h,
		AC_SEARCH_LIBS([H5_open], [hdf5], , AC_MSG_ERROR([Could not find libhdf5.a in standard library paths])),
		AC_MSG_ERROR([Could not find hdf5.h in standard header paths]))

		AC_CHECK_HEADER(hdf5_hl.h,
		AC_SEARCH_LIBS([H5TBmake_table], [hdf5_hl], , AC_MSG_ERROR([Could not find libhdf5_hl.a in standard library paths])),
		AC_MSG_ERROR([Could not find hdf5_hl.h in standard header paths]))
	else
		AC_CHECKING([for hdf5.h and hdf5_hl.h in $with_hdf5/include])
dnl		test for header file
		if test ! -f "$with_hdf5/include/hdf5.h" && ! -f "$with_hdf5/include/hdf5_hl.h"
		then		
			AC_MSG_ERROR([Could not find hdf5.h and hdf5_hl.h in "$with_hdf5/include])
		fi
			INCLUDES="$INCLUDES -I$with_hdf5/include"
	fi
	AC_SUBST(INCLUDES)
 ])