AC_DEFUN([AM_WITH_EXPAT],
[ AC_ARG_WITH(expat,
	      [  --with-expat=<location> [Use to specify non-standard expat root location]],
	      with_expat="$withval", with_expat=no)

if test "$with_expat" = "no"
then
dnl	no custom path, try to find in the standard paths
	AC_CHECK_HEADER(expat.h,
	AC_SEARCH_LIBS(XML_Parse, expat , , AC_MSG_ERROR([Could not find libexpat in standard library paths]))
	AC_MSG_ERROR([Could not find expat.h in standard header paths]))
else
	dnl verify libexpat is in the given directory
	AC_CHECKING([for expat.h in $with_expat/include])
dnl		test for header file
	if test ! -f "$with_expat/include/expat.h" 
	then		
		AC_MSG_ERROR([Could not find expat.h in "$with_expat/include])
	fi
	AC_MSG_RESULT([yes])		
	INCLUDES="$INCLUDES -I$with_expat/include "

fi
	AC_SUBST(INCLUDES)
])
