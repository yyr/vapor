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
//	Date:		Mon Mar  7 14:31:12 MST 2005
//
//	Description:	
//
#include <cstdarg>
#include <cstdio>
#include <cassert>
#include <idl_export.h>
#include <vapor/ImpExp.h>
#include "IDLCommon.h"


namespace {	// un-named namespace



IDL_VPTR vaporImport(int argc, IDL_VPTR *argv)
{
	UCHAR	*ucptr;
	IDL_LONG64	*long64ptr;	// x86_64 systems corrupted memory when plain	
							// old long pointers were used for arrays.
	IDL_LONG	*longptr;
	IDL_STRING	*stringptr;

	string vdfpath;
	size_t timestep;
	string varname;
	size_t minrange[3];
	size_t maxrange[3];
	size_t timeseg[2];

	IDL_MEMINT range_dims[] = {1,3};
	IDL_MEMINT timeseg_dims[] = {1,2};

	IDL_STRUCT_TAG_DEF s_tags[] = {
		{"VDFPATH", 0, (void *) IDL_TYP_STRING, 0},
		{"TIMESTEP", 0, (void *) IDL_TYP_LONG, 0},
		{"VARNAME", 0, (void *) IDL_TYP_STRING, 0},
		{"MINRANGE", range_dims, (void *) IDL_TYP_LONG64, 0},
		{"MAXRANGE", range_dims, (void *) IDL_TYP_LONG64, 0},
		{"TIMESEG", timeseg_dims, (void *) IDL_TYP_LONG64, 0},
		{NULL}
	};
	void	*s;

	ImpExp  *impexp = new ImpExp();
	myBaseErrChk();


	impexp = new ImpExp();

	impexp->Import(&vdfpath, &timestep, &varname, minrange, maxrange, timeseg);
	myBaseErrChk();

	s = IDL_MakeStruct(0, s_tags);

	IDL_VPTR result;
	IDL_MakeTempStructVector(s,1,&result,1);

	ucptr = result->value.s.arr->data + IDL_StructTagInfoByName(
		s, "VDFPATH",IDL_MSG_LONGJMP, NULL
	);
	stringptr = (IDL_STRING *) ucptr;
	IDL_StrStore(stringptr, (char *) vdfpath.c_str());

	ucptr = result->value.s.arr->data + IDL_StructTagInfoByName(
		s, "TIMESTEP",IDL_MSG_LONGJMP, NULL
	);
	longptr = (IDL_LONG *) ucptr;
	*longptr = (IDL_LONG) timestep;

	ucptr = result->value.s.arr->data + IDL_StructTagInfoByName(
		s, "VARNAME",IDL_MSG_LONGJMP, NULL
	);
	stringptr = (IDL_STRING *) ucptr;
	IDL_StrStore(stringptr, (char *) varname.c_str());


	ucptr = result->value.s.arr->data + IDL_StructTagInfoByName(
		s, "MINRANGE",IDL_MSG_LONGJMP, NULL
	);
	long64ptr = (IDL_LONG64 *) ucptr;
	long64ptr[0] = (IDL_LONG64) minrange[0];
	long64ptr[1] = (IDL_LONG64) minrange[1];
	long64ptr[2] = (IDL_LONG64) minrange[2];

	ucptr = result->value.s.arr->data + IDL_StructTagInfoByName(
		s, "MAXRANGE",IDL_MSG_LONGJMP, NULL
	);
	long64ptr = (IDL_LONG64 *) ucptr;
	long64ptr[0] = (IDL_LONG64) maxrange[0];
	long64ptr[1] = (IDL_LONG64) maxrange[1];
	long64ptr[2] = (IDL_LONG64) maxrange[2];

	ucptr = result->value.s.arr->data + IDL_StructTagInfoByName(
		s, "TIMESEG",IDL_MSG_LONGJMP, NULL
	);
	long64ptr = (IDL_LONG64 *) ucptr;
	long64ptr[0] = (IDL_LONG64) timeseg[0];
	long64ptr[1] = (IDL_LONG64) timeseg[1];


	return(result);
}

};



int IDL_LoadVaporImport(void)
{
	//
	//These tables contain information on the functions and procedures
	//that make up the TESTMODULE DLM. The information contained in these
	//tables must be identical to that contained in testmodule.dlm.
	//

	static IDL_SYSFUN_DEF2 func_addr[] = {
		{ (IDL_SYSRTN_GENERIC) vaporImport, 
			"VAPORIMPORT", 0, 0, 0, 0
		},
	};

	return IDL_SysRtnAdd(func_addr, TRUE, IDL_CARRAY_ELTS(func_addr));

}

