AC_DEFUN([AM_WITH_SZIP],
[ AC_ARG_WITH(szip,
	      [  --with-szip=PREFIX     Use system Szip library],
	    with_szip=$withval , with_szip=no)

if test "$with_szip" = "no"
then
dnl	no custom path, try to find in the standard paths
	AC_CHECK_HEADER([szlib.h],
	AC_SEARCH_LIBS([SZ_CompressInit], [sz] , , AC_MSG_ERROR([Could not find Szip in standard library paths])),
	AC_MSG_ERROR([Could not find szlib.h in standard header paths]))
else
dnl verify libszip is in the directory
#	AC_CHECKING([for libsz in $with_szip/lib and szlib.h in $with_szip/include])
#	if test ! -f "$with_szip/lib/libsz.a" && ! -f "$with_szip/lib/libsz.so" && ! -f "with_szip/lib/libsz.dylib"
#	then

	#	AC_MSG_ERROR([Could not find Szip in "$with_szip/lib"])
#	else
	#	AC_MSG_RESULT([yes])
		AC_CHECKING([for szlib.h in $with_szip/lib])
dnl		test for header file
		if test ! -f "$with_szip/include/szlib.h"
		then		
			AC_MSG_ERROR([Could not find szlib.h in "$with_szip/include])
		fi
		AC_MSG_RESULT([yes])
		#LIBS="-lsz $LIBS"
		INCLUDES="$INCLUDES -I$with_szip/include "
	#	LDFLAGS="$LDFLAGS -L$with_szip/lib"
		#HAVE_LIBSZIP=1
#  	fi
fi
	AC_SUBST(INCLUDES)
#	AC_SUBST(LDFLAGS)
#	AC_SUBST(LIBS)
#	AC_SUBST(HAVE_LIBSZIP)
])