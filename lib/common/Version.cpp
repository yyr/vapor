#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vapor/Version.h>


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
	if (GetRC().length()) oss << "." << GetRC(); 
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
int Version::Compare(std::string string1, std::string string2){
	int pos1 = string1.find(".");
	string maj1 = string1.substr(0,pos1);
	int pos2 = string2.find(".");
	string maj2 = string2.substr(0,pos2);
	int val1 = atoi(maj1.c_str());
	int val2 = atoi(maj2.c_str());
	if (val1 < val2 ) return -1;
	if (val1 > val2) return 1;
	int pos1a = string1.find(".", pos1+1);
	int pos2a = string1.find(".", pos2+1);
	string min1 = string1.substr(pos1+1,pos1a-pos1-1);
	string min2 = string2.substr(pos2+1,pos2a-pos2-1);
	val1 = atoi(min1.c_str());
	val2 = atoi(min2.c_str());
	if (val1 < val2 ) return -1;
	if (val1 > val2) return 1;
	string minmin1 = string1.substr(pos1a+1,string1.length()-pos1a-1);
	string minmin2 = string2.substr(pos2a+1,string2.length()-pos2a-1);
	val1 = atoi(minmin1.c_str());
	val2 = atoi(minmin2.c_str());
	if (val1 < val2 ) return -1;
	if (val1 > val2) return 1;
	return 0;
}




