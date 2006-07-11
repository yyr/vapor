#include <string>
#include <iostream>
#include <sstream>
#include "vapor/Version.h"


using namespace VetsUtil;
using namespace std;

//
// If these aren't defined here, the module won't link!
//
string Version::_formatString;
string Version::_dateString;

const string &Version::GetVersionString() {

	ostringstream oss;
	oss << _majorVersion << "." << _minorVersion << "." << _minorMinorVersion;
	_formatString = oss.str();
	return(_formatString);

}

int Version::Compare(int major, int minor, int minorminor) {

	if (major != _majorVersion) {
		return(major > _majorVersion ? -1 : 1);
	}
	if (minor != _minorVersion) {
		return(minor > _minorVersion ? -1 : 1);
	}
	if (minorminor != _minorMinorVersion) {
		return(minorminor > _minorMinorVersion ? -1 : 1);
	}

	return(0);
}



