#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cerrno>
#include <vapor/CFuncs.h>

#include <iostream>
#ifdef WIN32
#pragma warning( disable : 4996 )
#endif

using namespace VetsUtil;

const char	*VetsUtil::Basename(const char *path) {
    const char *last;
    last = strrchr(path, '/');
    if ( !last ) return path;
    else return last + 1;
}

string VetsUtil::Basename(const string &path) {
	string separator = "/";
	string separator2 = "";

#ifdef WIN32
	separator2 = "\\";
#endif

	string::size_type pos = path.rfind(separator);
	if (pos == string::npos) {
        if (separator2.empty()) return(path);

		pos = path.rfind(separator2);
		if (pos == string::npos) return(path);
	}

	return(path.substr(pos+1));
}

string VetsUtil::Dirname(const string &path) {
	string separator = "/";
	string separator2 = "";

#ifdef WIN32
	separator2 = "\\";
#endif

	string::size_type pos = path.rfind(separator);
	if (pos == string::npos) {
        if (separator2.empty()) return(".");

		pos = path.rfind(separator2);
		if (pos == string::npos) return(".");
	}

	return(path.substr(0, pos));
}

char   *VetsUtil::Dirname(const char *pathname, char *directory)
{
	char	*s;

	if (!pathname || (strlen(pathname) == 0))  return(NULL);

	strcpy(directory, pathname);

	s = directory + strlen(directory) - 1;

	// remove trailing slashes
	//
	while (*s == '/' && s != directory) {
		*s = '\0'; 
		s--; 
	}

	s = strrchr(directory, '/');
	if (s) {
		
		if (s != directory) { 
			*s = '\0';
		}
	}
	else {
		strcpy(directory, ".");
	}
	return (directory);
}

double VetsUtil::GetTime(){
	double t;
#ifdef WIN32 //Windows does not have a nanosecond time function...
	SYSTEMTIME sTime;
	FILETIME fTime;
	GetSystemTime(&sTime);
	SystemTimeToFileTime(&sTime,&fTime);
    //Resulting system time is in 100ns increments
	__int64 longlongtime = fTime.dwHighDateTime;
	longlongtime <<= 32;
	longlongtime += fTime.dwLowDateTime;
	t = (double)longlongtime;
	t *= 1.e-7;

#endif
#ifndef WIN32
	struct timespec ts;
	ts.tv_sec = ts.tv_nsec = 0;
#endif

#if defined(Linux) || defined(AIX)
	clock_gettime(CLOCK_REALTIME, &ts);
	t = (double) ts.tv_sec + (double) ts.tv_nsec*1.0e-9;
#endif

#ifdef	Darwin
	uint64_t tmac = mach_absolute_time();
	mach_timebase_info_data_t info = {0,0};
	mach_timebase_info(&info);
	ts.tv_sec = tmac * 1e-9;
	ts.tv_nsec = tmac - (ts.tv_sec * 1e9);
	t = (double) ts.tv_sec + (double) ts.tv_nsec*1.0e-9;
#endif
	
	return(t);
}
