#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cerrno>
#include "vapor/MyBase.h"
#ifdef WIN32
#include "windows.h"
#endif

#include <iostream>

using namespace VetsUtil;

char 	*MyBase::ErrMsg = NULL;
int	MyBase::ErrMsgSize = 0;
int	MyBase::ErrCode = 0;

MyBase::MyBase() {
}


void	MyBase::SetErrMsg(
	const char *format, 
	...
) {
	va_list args;
	int	done = 0;
	const int alloc_size = 256;
	int rc;
	char *s;
#ifdef WIN32
	CHAR szBuf[80]; 
	
	
    DWORD dw = GetLastError(); 
 
	sprintf(szBuf, "Reporting error message: GetLastError returned %u\n", dw); 
    MessageBox(NULL, szBuf, "Error", MB_OK); 
#endif
	vfprintf(stderr, format, args);
	if (!ErrMsg) {
		ErrMsg = new char[alloc_size];
		assert(ErrMsg != NULL);
		ErrMsgSize = alloc_size;
	}

	// Loop until we've successfully buffered the error message, growing
	// the message buffer as needed
	//
	while (! done) {
		va_start(args, format);
#ifdef WIN32
		rc = _vsnprintf(ErrMsg, ErrMsgSize, format, args);

#else
		rc = vsnprintf(ErrMsg, ErrMsgSize, format, args);
#endif
		va_end(args);

		if (rc < (ErrMsgSize-1)) {
			done = 1;
		} else {
			if (ErrMsg) delete [] ErrMsg;
			ErrMsg = new char[ErrMsgSize + alloc_size];
			assert(ErrMsg != NULL);
			ErrMsgSize += alloc_size;
		}
	}

	// Now handle any %M format specificers
	//
	while (s = strstr("%M", ErrMsg)) {
		s++;	
		*s = 's';	// Ugh. Change %M to %s.
		done = 0;
		while (! done) {
#ifdef WIN32
			rc = _snprintf(ErrMsg, ErrMsgSize, ErrMsg, strerror(errno));
#else
			rc = snprintf(ErrMsg, ErrMsgSize, ErrMsg, strerror(errno));
#endif
			if (rc < (ErrMsgSize-1)) {
				done = 1;
			} else {
				char *sptr;
				sptr = new char[ErrMsgSize + alloc_size];
				assert(sptr != NULL);
				ErrMsgSize += alloc_size;
				if (ErrMsg) {
					strncpy(s, ErrMsg, rc);
					delete [] ErrMsg;
				}
				ErrMsg = s;
			}
		}
	}
		

	ErrCode = 1;
}

int	VetsUtil::IsPowerOfTwo(
	unsigned int x
) {
	if( !x ) return 1;
	while( !(x & 1) ) x >>= 1;
	return (x == 1);
} 

int VetsUtil::StrCmpNoCase(const string &s, const string &t) {
    string::const_iterator sptr = s.begin();
    string::const_iterator tptr = t.begin();

    while (sptr != s.end() && tptr != t.end()) {
        if (toupper(*sptr) != toupper(*tptr)) {
            return(toupper(*sptr) < toupper(*tptr) ? -1 : 1);
        }
        *sptr++; *tptr++;
    }

    return((s.size() == t.size()) ? 0 : ((s.size()<t.size()) ? -1 : 1));
}


void	VetsUtil::StrRmWhiteSpace(string &s) {
	string::size_type	i;
	
	if (s.length() < 1) return;

	i = 0;
	while (isspace(s[i])) i++;
	
	if (i>0) {
		s.replace(0, i, "", 0);
	}

	
	i = s.length() - 1;
	assert(i >= 0);
	while (isspace(s[i])) i--;

	if (i<(s.length() - 1)) {
		s.replace(i+1, s.length()-i+1, "", 0);
	}
}

