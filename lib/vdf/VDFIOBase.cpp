#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <cassert>
#ifndef WIN32
#include <unistd.h>
#else
#include "windows.h"
#include "Winbase.h"
#include <vapor/common.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#ifdef	Darwin
#include <mach/mach_time.h>
#endif

#include <vapor/VDFIOBase.h>
#include <vapor/MyBase.h>

using namespace VetsUtil;
using namespace VAPoR;


int	VDFIOBase::_VDFIOBase(
) {

	SetClassName("VDFIOBase");

	_read_timer_acc = 0.0;
	_write_timer_acc = 0.0;
	_seek_timer_acc = 0.0;
	_xform_timer_acc = 0.0;

	_read_timer = 0.0;
	_write_timer = 0.0;
	_seek_timer = 0.0;
	_xform_timer = 0.0;

	return(0);
}

VDFIOBase::VDFIOBase(
	const MetadataVDC	&metadata
) : MetadataVDC (metadata) {

	if (_VDFIOBase() < 0) return;

}

VDFIOBase::VDFIOBase(
	const string &metafile
) : MetadataVDC (metafile) {

	if (_VDFIOBase() < 0) return;
}

VDFIOBase::~VDFIOBase() {
}

double VDFIOBase::GetTime() const {
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



int    VAPoR::MkDirHier(const string &dir) {

    stack <string> dirs;

    string::size_type idx;
    string s = dir;

	dirs.push(s);
    while ((idx = s.find_last_of("/")) != string::npos) {
        s = s.substr(0, idx);
		if (! s.empty()) dirs.push(s);
    }

    while (! dirs.empty()) {
        s = dirs.top();
		dirs.pop();
#ifndef WIN32
        if ((mkdir(s.c_str(), 0777) < 0) && dirs.empty() && errno != EEXIST) {
			MyBase::SetErrMsg("mkdir(%s) : %M", s.c_str());
            return(-1);
        }
#else 
		//Windows version of mkdir:
		//If it succeeds, return value is nonzero
		if (!CreateDirectory(( LPCSTR)s.c_str(), 0)){
			DWORD dw = GetLastError();
			if (dw != 183){ //183 means file already exists
				MyBase::SetErrMsg("mkdir(%s) : %M", s.c_str());
				return(-1);
			}
		}
#endif
    }
    return(0);
}

void    VAPoR::DirName(const string &path, string &dir) {
	
	string::size_type idx = path.find_last_of('/');
#ifdef WIN32
	if (idx == string::npos)
		idx = path.find_last_of('\\');
#endif
	if (idx == string::npos) {
#ifdef WIN32
		dir.assign(".\\");
#else
		dir.assign("./");
#endif
	}
	else {
		dir = path.substr(0, idx+1);
	}
}
