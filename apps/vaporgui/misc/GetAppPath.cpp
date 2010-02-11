//
// $Id$
//
#include <string>
#include <iostream>
#include <cctype>
#include <cassert>
#ifdef  Darwin
#include <CoreFoundation/CFBundle.h>
#include <CoreFoundation/CFString.h>
#include <CoreServices/CoreServices.h>
#endif
#include "GetAppPath.h"
#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace std;
using namespace VetsUtil;

#ifdef  Darwin
string get_path_from_bundle(const string &app) {
    string path;
	path.clear();

	string bundlename = app + ".app";

    //
    // Get path to document directory from the application "Bundle";
    //

    CFBundleRef mainBundle = CFBundleGetMainBundle();
    if (!mainBundle) return(".");

    CFURLRef url = CFBundleCopyBundleURL(mainBundle);
    if (! url) return(".");

    const int kBufferLength = 1024;
    UInt8 buffer[kBufferLength];
    char componentStr[kBufferLength];

    CFIndex componentLength = CFURLGetBytes(url, buffer, kBufferLength);
    if (componentLength < 0) return(false);
    buffer[componentLength] = 0;

    CFRange range;
    CFRange rangeIncludingSeparators;

    range = CFURLGetByteRangeForComponent(
        url, kCFURLComponentPath, &rangeIncludingSeparators
    );
    if (range.location == kCFNotFound) return(".");

    strncpy(componentStr, (const char *)&buffer[range.location], range.length);
    componentStr[range.length] = 0;
	string s = componentStr;

	if (s.find(bundlename) == string::npos) return(path); 

    path = s;
    return(path);

}
#endif

std::string VetsUtil::GetAppPath(
	const string &app, 
	const string &resource,
	const vector <string> &paths
) {

//cerr << "Begin GetAppPath\n";

#ifdef	WIN32
	const string separator = "\\";
#else
	const string separator = "/";
#endif

	string path;
	path.clear();

	string myapp = app;	// upper case app name
	for (string::iterator itr = myapp.begin(); itr != myapp.end(); itr++) {
		if (islower(*itr)) *itr = toupper(*itr);
	}

	string env(myapp);
	env.append("_");
	env.append("HOME");

	if ( ! (
		(resource.compare("lib") == 0) || 
		(resource.compare("bin") == 0) ||
		(resource.compare("share") == 0) ||
		(resource.compare("plugins") == 0)
	)) {
		return(path);	// empty path, invalid resource
	}


    if (char *s = getenv(env.c_str())) {
		path.assign(s);
		path.append(separator);
		path.append(resource);
	}
#ifdef	Darwin
	else {
		path = get_path_from_bundle(myapp);
		if (! path.empty()) {
			path.append("Contents/");
			if ((resource.compare("lib")==0) || (resource.compare("bin")==0)) {
				path.append("MacOS");
			}
			else if (resource.compare("share") == 0) {
				path.append("SharedSupport");
			}
			else {	// must be plugins
				path.append("Plugins");
			}
		}
	}
#endif
#ifndef WIN32 //For both Linux and Mac:
	if (path.empty()) {
		if (resource.compare("lib") == 0) {
			path.append(DSO_DIR);
		}
		else if (resource.compare("bin") == 0) {
			path.append(BINDIR);
		}
		else if (resource.compare("share") == 0) {
			path.append(ABS_TOP);
			path.append(separator);
			path.append("share");
		}
		else {	// must be plugins
			path.append(QTDIR);
			path.append(separator);
			path.append("plugins");
		}
	}
#endif

	if (path.empty()) return(path);

	for (int i=0; i<paths.size(); i++) {
		path.append(separator);
		path.append(paths[i]);
	}

//cerr << "End GetAppPath " << path << endl;
	return(path);	
}
