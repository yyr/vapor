//
//      $Id$
//
//************************************************************************
//								*
//		     Copyright (C)  2005			*
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
//	Date:		Wed Feb 16 11:16:16 MST 2005
//
//	Description:	
//
#include <cstdarg>
#include <cstdio>
#include <string>
#include <idl_export.h>
#include "IDLCommon.h"

void    errFatal(
    const char *format,
    ...
) {
    va_list args;

    char    msg[1024];

    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);

    IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_LONGJMP, msg);
}

void	myBaseErrChk() {

	if (Metadata::GetErrCode()) {
		string msg(Metadata::GetErrMsg());
		Metadata::SetErrCode(0);
		errFatal(msg.c_str());
	}
}

Metadata *varGetMetadata(
	IDL_VPTR var
) {

	IDL_ENSURE_SCALAR(var);

	if (var->type != IDL_TYP_MEMINT) {
        errFatal("Metadata handle must be of type IDL_TYP_MEMINT");
    }
	return((Metadata *) var->value.memint);
}
