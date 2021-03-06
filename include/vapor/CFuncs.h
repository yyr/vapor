//
//      $Id$
//
//************************************************************************
//								*
//		     Copyright (C)  2004			*
//     University Corporation for Atmospheric Research		*
//		     All Rights Reserved			*
//								*
//************************************************************************/
//
//	File:		
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Mon Nov 29 11:49:03 MST 2004
//
//	Description:	A collection of common C system routines that 
//					aren't always portable across OS platforms
//

#ifndef	_CFuncs_h_
#define	_CFuncs_h_

#include <cmath>
#include <string>
#include <vapor/common.h>


using namespace std;

namespace VetsUtil {


COMMON_API const char	*Basename(const char *path);
COMMON_API string Basename(const string &path);


//! Return the directory component of a UNIX path name
//!
//! Re-implements the UNIX 'dirname' function. However, in this version
//! the original pathname is not modfied -- the directory component of
//! the path name is copied into caller-provided storage.
//!
//! \param[in] pathname Pointer to a null-terminated file path name
//! \param[out] directory Contains parent directory of \p pathname
//! \retval directory Returns \p directory parameter
//!
COMMON_API char   *Dirname(const char *pathname, char *directory);
COMMON_API string Dirname(const string &path);
COMMON_API double GetTime();

COMMON_API int    MkDirHier(const string &dir);
COMMON_API void   DirName(const string &path, string &dir);

};

#endif	// _CFuncs_h_
