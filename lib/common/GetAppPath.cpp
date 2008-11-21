//
// $Id$
//
#include <string>
#include <cctype>
#include <cassert>
#ifdef  Darwin
#include <CoreFoundation/CFBundle.h>
#include <CoreServices/CoreServices.h>
#endif
#include <vapor/GetAppPath.h>

using namespace VetsUtil;

#ifdef  Darwin
string get_path_from_bundle(const string &app, const string & name) {
    string path;
	path.clear();

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

    path = componentStr;
    path.append("/Contents/SharedSupport/");
    path.append(app);
    path.append("-");
    path.append(Version::GetVersionString());
    return(path);

}
#endif

string VetsUtil::GetAppPath(const string &app, const string &name) {
	string path;
	path.clear();

	string env(app);
	env.append("_");
	env.append(name);

	for (string::iterator itr = env.begin(); itr != env.end(); itr++) {
		if (islower(*itr)) *itr = toupper(*itr);
	}

    if (char *s = getenv(env.c_str())) {
		path.assign(s);
		return(path);
	}
#ifdef	Darwin
	else {
		return(get_path_from_bundle(app, name));
	}
#endif
	path.assign(".");
	return(path);	
}
