#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cerrno>
#include <cctype>

#include "vapor/MyBase.h"
#ifdef WIN32
#include "windows.h"
#endif

#include <iostream>

using namespace VetsUtil;

char 	*MyBase::ErrMsg = NULL;
int	MyBase::ErrMsgSize = 0;
int	MyBase::ErrCode = 0;
FILE	*MyBase::ErrMsgFilePtr = NULL;
void (*MyBase::ErrMsgCB) (const char *msg, int err_code) = NULL;

char 	*MyBase::DiagMsg = NULL;
int	MyBase::DiagMsgSize = 0;
#ifdef	DEBUG
FILE	*MyBase::DiagMsgFilePtr = stderr;
#else
FILE	*MyBase::DiagMsgFilePtr = NULL;
#endif
void (*MyBase::DiagMsgCB) (const char *msg) = NULL;

MyBase::MyBase() {
	SetClassName("MyBase");
}


void	MyBase::_SetErrMsg(
	const char *format, 
	va_list args
) {
	int	done = 0;
	const int alloc_size = 256;
	int rc;
	char *s;
//#ifdef WIN32
	//CHAR szBuf[80]; 
	
	
    //DWORD dw = GetLastError(); 
 
	//sprintf(szBuf, "Reporting error message: GetLastError returned %u\n", dw); 
    //MessageBox(NULL, szBuf, "Error", MB_OK); 
//#endif
	if (!ErrMsg) {
		ErrMsg = new char[alloc_size];
		assert(ErrMsg != NULL);
		ErrMsgSize = alloc_size;
	}
	// Loop until we've successfully buffered the error message, growing
	// the message buffer as needed
	//
	while (! done) {
#ifdef WIN32
		rc = _vsnprintf(ErrMsg, ErrMsgSize, format, args);

#else
		rc = vsnprintf(ErrMsg, ErrMsgSize, format, args);
#endif

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
	while ((s = strstr(ErrMsg, "%M"))) {
		s++;	
		*s = 's';	// Ugh. Change %M to %s.

		char *fmt = strdup(ErrMsg);
		assert(fmt != NULL);
		done = 0;
		while (! done) {
#ifdef WIN32
			rc = _snprintf(ErrMsg, ErrMsgSize, fmt, strerror(errno));
#else
			rc = snprintf(ErrMsg, ErrMsgSize, fmt, strerror(errno));
#endif
			if (rc < (ErrMsgSize-1)) {
				done = 1;
			} else {
				char *sptr;
				sptr = new char[ErrMsgSize + alloc_size];
				assert(sptr != NULL);
				ErrMsgSize += alloc_size;
				if (ErrMsg) {
					strncpy(sptr, ErrMsg, rc);
					delete [] ErrMsg;
				}
				ErrMsg = sptr;
			}
		}
		if (fmt) free(fmt);
	}


	if (ErrMsgCB) (*ErrMsgCB) (ErrMsg, ErrCode);

	if (ErrMsgFilePtr) {
		(void) fprintf(ErrMsgFilePtr, "%s\n", ErrMsg);
	}
}

void	MyBase::SetErrMsg(
	const char *format, 
	...
) {
	va_list args;

	ErrCode = 1;

	va_start(args, format);
	_SetErrMsg(format, args);
	va_end(args);
}

void	MyBase::SetErrMsg(
	int errcode,
	const char *format, 
	...
) {
	va_list args;

	ErrCode = errcode;

	va_start(args, format);
	_SetErrMsg(format, args);
	va_end(args);
}

void	MyBase::SetDiagMsg(
	const char *format, 
	...
) {
	va_list args;
	int	done = 0;
	const int alloc_size = 256;
	int rc;
	char *s;

	if (!DiagMsg) {
		DiagMsg = new char[alloc_size];
		assert(DiagMsg != NULL);
		DiagMsgSize = alloc_size;
	}
	// Loop until we've successfully buffered the error message, growing
	// the message buffer as needed
	//
	while (! done) {
		va_start(args, format);
#ifdef WIN32
		rc = _vsnprintf(DiagMsg, DiagMsgSize, format, args);

#else
		rc = vsnprintf(DiagMsg, DiagMsgSize, format, args);
#endif
		va_end(args);

		if (rc < (DiagMsgSize-1)) {
			done = 1;
		} else {
			if (DiagMsg) delete [] DiagMsg;
			DiagMsg = new char[DiagMsgSize + alloc_size];
			assert(DiagMsg != NULL);
			DiagMsgSize += alloc_size;
		}
	}

	// Now handle any %M format specificers
	//
	while ((s = strstr("%M", DiagMsg))) {
		s++;	
		*s = 's';	// Ugh. Change %M to %s.

		char *fmt = strdup(DiagMsg);
		assert(fmt != NULL);
		done = 0;
		while (! done) {
#ifdef WIN32
			rc = _snprintf(DiagMsg, DiagMsgSize, fmt, strerror(errno));
#else
			rc = snprintf(DiagMsg, DiagMsgSize, fmt, strerror(errno));
#endif
			if (rc < (DiagMsgSize-1)) {
				done = 1;
			} else {
				char *sptr;
				sptr = new char[DiagMsgSize + alloc_size];
				assert(sptr != NULL);
				DiagMsgSize += alloc_size;
				if (DiagMsg) {
					strncpy(sptr, DiagMsg, rc);
					delete [] DiagMsg;
				}
				DiagMsg = sptr;
			}
		}
	}

	if (DiagMsgCB) (*DiagMsgCB) (DiagMsg);

	if (DiagMsgFilePtr) {
		(void) fprintf(DiagMsgFilePtr, "%s\n", DiagMsg);
	}
}

int	VetsUtil::IsPowerOfTwo(
	unsigned int x
) {
	if( !x ) return 1;
	while( !(x & 1) ) x >>= 1;
	return (x == 1);
} 

//Find integer log, base 2, of a 32-bit positive integer:
int VetsUtil::ILog2(int n){
	int i;
	for (i = 0; i<31;i++){
		if (n <= (1<<i)) break;
	}
	return i;
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

	if (s.length() < 1) return;
	i = s.length() - 1;

	while (isspace(s[i])) i--;

	if (i<(s.length() - 1)) {
		s.replace(i+1, s.length()-i+1, "", 0);
	}
}

unsigned long long VetsUtil::GetBits64(
    unsigned long long targ,
    int pos,
    int n
) {
    return ((targ >> (pos+1-n)) & ~(~0ULL <<n));
}

unsigned long long VetsUtil::SetBits64(
    unsigned long long targ,
    int pos,
    int n,
    unsigned long long src
) {
        targ &= ~(~(~0ULL << n) << (pos+1 - n));
        targ |= (src & ~(~0ULL << n)) << (pos+1-n);
        return(targ);
}
