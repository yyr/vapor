//
//      $Id$
//
//************************************************************************
//								*
//		     Copyright (C)  2004			*
//     University Corporation for Atmospheric Research		*
//		     All Rights Reserved			*
//								*
//************************************************************************/
//
//	File:		
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Wed Sep 29 15:40:23 MDT 2004
//
//	Description:	A collection of general purpose utilities - things that
//					probably should be in the STL but aren't.
//

//! \class MyBase
//! \brief VetsUtil base class
//! \author John Clyne
//! \version 0.1
//! \date    Mon Dec 13 17:15:12 MST 2004
//! A collection of general purpose utilities - things that
//!                  probably should be in the STL but aren't.
//!


#ifndef	_MyBase_h_
#define	_MyBase_h_

#include <cmath>
#include <string>
#include "vaporinternal/common.h"


using namespace std;

namespace VetsUtil {

//
// The MyBase base class provides a simple error reporting mechanism 
// that can be used by derrived classes. N.B. the error messages/codes
// are stored in static class members.
//
class COMMON_API MyBase {
public:

 typedef void (*ErrMsgCB_T) (const char *msg, int err_code);
 typedef void (*DiagMsgCB_T) (const char *msg);

 MyBase();

 //! Record a formatted error message. 
 // 
 //! Formats and records an error message. Subsequent calls will overwrite
 //! the stored error message. The method will also set the error
 //! code to 1.
 //! \param[in] format A 'C' style sprintf format string.
 //! \param[in] arg... Arguments to format 
 //! \sa GetErrMsg(), GetErrCode()
 //
 static void	SetErrMsg(const char *format, ...);

 //! Retrieve the current error message
 //!
 //! Retrieves the last error message set with \b SetErrMsg(). It is the 
 //! caller's responsibility to copy the message returned to user space.
 //! \sa SetErrMsg(), SetErrCode()
 //! \retval msg A pointer to null-terminated string.
 //
 static const char	*GetErrMsg() {return(ErrMsg);}

 //! Record an error code
 // 
 //! Sets the error code to the indicated value. 
 //! \param[in] format A 'C' style sprintf format string.
 //! \param[in] arg... Arguments to format 
 //! \sa GetErrMsg(), GetErrCode(), SetErrMsg()
 //
 static void	SetErrCode(int err_code) { ErrCode = err_code; }

 //! Retrieve the current error code
 // 
 //! Retrieves the last error code set either explicity with \bSetErrCode()
 //! or indirectly with a call to \bSetErrMsg()
 //! \sa SetErrMsg(), SetErrCode()
 //! \retval code An erroor code
 //
 static int	GetErrCode() { return (ErrCode); }

 //! Set a callback function for error messages
 //!
 //! Set the callback function to be called whenever SetErrMsg() 
 //! is called. The callback function, \p cb, will be called and passed 
 //! the formatted error message and the error code as an argument. The 
 //! default callback function is NULL, i.e. no function is called
 //!
 //! \param[in] cb A callback function or NULL
 //
 static void SetErrMsgCB(ErrMsgCB_T cb) { ErrMsgCB = cb; };

 //! Set the file pointer to whence error messages are written
 //!
 //! This method permits the specification of a file pointer to which
 //! all messages logged with SetErrMsg() will be written. The default
 //! file pointer is NULL. I.e. by default error messages logged by 
 //! SetErrMsg() are not written.
 //! \param[in] A file pointer opened for writing or NULL
 //! \sa SetErrMsg()
 //
 static void SetErrMsgFilePtr(FILE *fp) { ErrMsgFilePtr = fp; };

 //! Record a formatted diagnostic message. 
 // 
 //! Formats and records a diagnostic message. Subsequent calls will overwrite
 //! the stored error message. This method differs from SetErrMsg() only
 //! in that no associated error code is set - the message is considered
 //! diagnostic only, not an error.
 //! \param[in] format A 'C' style sprintf format string.
 //! \param[in] arg... Arguments to format 
 //! \sa GetDiagMsg()
 //
 static void	SetDiagMsg(const char *format, ...);

 //! Retrieve the current diagnostic message
 //!
 //! Retrieves the last error message set with \b SetDiagMsg(). It is the 
 //! caller's responsibility to copy the message returned to user space.
 //! \sa SetDiagMsg()
 //! \retval msg A pointer to null-terminated string.
 //
 static const char	*GetDiagMsg() {return(DiagMsg);}


 //! Set a callback function for diagnostic messages
 //!
 //! Set the callback function to be called whenever SetDiagMsg() 
 //! is called. The callback function, \p cb, will be called and passed 
 //! the formatted error message as an argument. The 
 //! default callback function is NULL, i.e. no function is called
 //!
 //! \param[in] cb A callback function or NULL
 //
 static void SetDiagMsgCB(DiagMsgCB_T cb) { DiagMsgCB = cb; };

 //! Set the file pointer to whence diagnostic messages are written
 //!
 //! This method permits the specification of a file pointer to which
 //! all messages logged with SetDiagMsg() will be written. The default
 //! file pointer is NULL. I.e. by default error messages logged by 
 //! SetDiagMsg() are not written.
 //! \param[in] A file pointer opened for writing or NULL
 //! \sa SetDiagMsg()
 //
 static void SetDiagMsgFilePtr(FILE *fp) { DiagMsgFilePtr = fp; };

 // N.B. the error codes/messages are stored in static class members!!!
 static char 	*ErrMsg;
 static int	ErrMsgSize;
 static int	ErrCode;
 static FILE	*ErrMsgFilePtr;
 static ErrMsgCB_T ErrMsgCB;

 static char 	*DiagMsg;
 static int	DiagMsgSize;
 static FILE	*DiagMsgFilePtr;
 static DiagMsgCB_T DiagMsgCB;


};

 COMMON_API inline int    IsOdd(int x) { return(x % 2); };

 //! Return true if power of two
 //
 //! Returns a non-zero value if the input parameter is a power of two
 //! \param[in] x An integer
 //! \retval status 
 COMMON_API int IsPowerOfTwo(unsigned int x);

 COMMON_API inline int	Min(int a, int b) { return(a<b ? a : b); };
 COMMON_API inline int	Max(int a, int b) { return(a>b ? a : b); };

 COMMON_API inline float	Min(float a, float b) { return(a<b ? a : b); };
 COMMON_API inline float	Max(float a, float b) { return(a>b ? a : b); };

 COMMON_API inline double	Min(double a, double b) { return(a<b ? a : b); };
 COMMON_API inline double	Max(double a, double b) { return(a>b ? a : b); };


 COMMON_API inline double	LogBaseN(double x, double n) { return(log(x) / log(n)); };

 //! Case-insensitive string comparison
 //
 //! Performs a case-insensitive comparison of two C++ strings. Behaviour
 //! is otherwise identical to the C++ std::string.compare() method.
 //
 COMMON_API int	StrCmpNoCase(const string &s, const string &t);


 //! Remove white space from a string
 //
 //! Performs in-place removal of all white space from a string.
 //! \param[in,out] s The string.
 COMMON_API void	StrRmWhiteSpace(string &s);

#ifdef WIN32
 COMMON_API inline int rint(float X) {
	 return ((X)>0)?((int)(X+0.5)):((int)(X-0.5));
 }
#endif
};

#endif //MYBASE_H

