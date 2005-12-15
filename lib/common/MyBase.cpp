#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cerrno>
#include <cctype>
#include <string>

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
	char **msgbuf,
	int *msgbufsz,
	const char *format, 
	va_list args
) {
	int	done = 0;
	const int alloc_size = 256;
	int rc;
//#ifdef WIN32
	//CHAR szBuf[80]; 
	
	
    //DWORD dw = GetLastError(); 
 
	//sprintf(szBuf, "Reporting error message: GetLastError returned %u\n", dw); 
    //MessageBox(NULL, szBuf, "Error", MB_OK); 
//#endif
	if (!*msgbuf) {
		*msgbuf = new char[alloc_size];
		assert(*msgbuf != NULL);
		*msgbufsz = alloc_size;
	}

	string formatstr(format);
	int loc;
	while ((loc = formatstr.find("%M", 0)) != string::npos) {
		formatstr.replace(loc, 2, strerror(errno), strlen(strerror(errno)));
	}

	format = formatstr.c_str();


	// Loop until we've successfully buffered the error message, growing
	// the message buffer as needed
	//
	while (! done) {
#ifdef WIN32
		rc = _vsnprintf(*msgbuf, *msgbufsz, format, args);

#else
		rc = vsnprintf(*msgbuf, *msgbufsz, format, args);
#endif

		if (rc < (*msgbufsz-1)) {
			done = 1;
		} else {
			if (*msgbuf) delete [] *msgbuf;
			*msgbuf = new char[*msgbufsz + alloc_size];
			assert(*msgbuf != NULL);
			*msgbufsz += alloc_size;
		}
	}

}

void	MyBase::SetErrMsg(
	const char *format, 
	...
) {
	va_list args;

	ErrCode = 1;

	va_start(args, format);
	_SetErrMsg(&ErrMsg, &ErrMsgSize, format, args);
	va_end(args);

	if (ErrMsgCB) (*ErrMsgCB) (ErrMsg, ErrCode);

	if (ErrMsgFilePtr) {
		(void) fprintf(ErrMsgFilePtr, "%s\n", ErrMsg);
	}
}

void	MyBase::SetErrMsg(
	int errcode,
	const char *format, 
	...
) {
	va_list args;

	ErrCode = errcode;

	va_start(args, format);
	_SetErrMsg(&ErrMsg, &ErrMsgSize, format, args);
	va_end(args);

	if (ErrMsgCB) (*ErrMsgCB) (ErrMsg, ErrCode);

	if (ErrMsgFilePtr) {
		(void) fprintf(ErrMsgFilePtr, "%s\n", ErrMsg);
	}
}

void	MyBase::SetDiagMsg(
	const char *format, 
	...
) {
	va_list args;

	va_start(args, format);
	_SetErrMsg(&DiagMsg, &DiagMsgSize, format, args);
	va_end(args);

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
