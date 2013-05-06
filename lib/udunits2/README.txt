Note:  The udunits2 code has been ported to Windows for use in VAPOR.  However, whenever udunits2 is updated, the code will again need to be ported.  This file documents the conversion process that should be performed whenever the udunits source code that is used by VAPOR is updated.

Build the latest version (currently 2.1.24) of udunits on Mac (run configure, then make then make install)
Copy over the following files into lib/udunits2:

unitcore.c, formatter.c, parser.c, converter.c, idToUnitMap.c, udunits-1.c, unitToIdMap.c, xml.c, scanner.c, ut_free_system.c, unitAndId.c, systemMap.c, status.c, prefix.c, error.c

parser.y, scanner.l

udunits.h, unitToIdMap.h, unitAndId.h, converter.h, idToUnitMap.h,systemMap.h

add udunits2.h to include/vapor directory

Add all these files (except for parser.y and scanner.l and scanner.c) to the VS2010 DLL project udunits2.
Add the include directory where expat.h is located.

In converter.c, and formatter.c and scanner.l define _USE_MATH_DEFINES before including math.h 
Also at the start of parser.c

Replace snprintf with _snprintf (in converter.c, formatter.c, xml.c)

In formatter.c, make size_t arguments of asciiPrintProduct, latin1PrintProduct, and utf8PrintProduct not const.

In idToUnitMap.c, parser.c, unitcore.c, and xml.c, remove #include <strings.h>
In scanner.c and xml.c, remove #include <unistd.h>

Replace strcasecmp with stricmp in idToUnitMap.c, parser.c, xml.c

In status.c replace <udunits2.h> with "vapor/udunits2.h"

In unitcore.c, remove "include inttypes.h"
Insert "#define int32_t __int32"

In xml.c remove "include libgen.h"
#define _POSIX_PATH_MAX 256
comment out
#define DEFAULT_UDUNITS2_XML_PATH 
Where DEFAULT_UDUNITS2_XML_PATH is used, replace it with
getenv(VAPOR_HOME)\share\udunits2\udunits2.xml

add tsearch.c and tsearch.h to project
change <search.h> to "tsearch.h" in include statements in 6 files.

Add libexpat.lib to project linker inputs, and its directory to linker additional library directives

In xml.c replace close with _close, open with _open, read with _read.  Insert #include <io.h> at top.  Instead of using dirname() as an argument to memmove(), do the following:
char dir[_POSIX_PATH_MAX];
char drv[_POSIX_PATH_MAX];
_splitpath(base,drv,dir,NULL,NULL);
strcat(dir,drv);
memmove(base,drv,sizeof(base));

In common.h insert:
#ifdef UDUNITS2_EXPORTS
#define UDUNITS2_API __declspec(dllexport)
#else
#define UDUNITS2_API __declspec(dllimport)
#endif

then put UDUNITS2_API in front of all the methods defined in udunits2.h
as well as in front of cv_free in converter.h 

insert #include "vapor/udunits2.h" in converter.h and converter.c
replace "udunits2.h" with "vapor/udunits2.h" wherever it occurs in #includes
insert #include "vapor/common.h" in udunits2.h






