//
// $Id$
//
#ifndef	_GetAppPath_h_
#define	_GetAppPath_h_
#include <vapor/MyBase.h>
#include <vapor/Version.h>

namespace VetsUtil {

PARAMS_API std::string GetAppPath(
	const string &app, const string &name, const vector <string> &paths);

};

#endif