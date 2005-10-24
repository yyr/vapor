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
//	File:		IDLMetadata.cpp
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Wed Feb 16 11:18:04 MST 2005
//
//	Description:	
//
#include <cstdarg>
#include <cstdio>
#include <cassert>
#include <idl_export.h>
#include "IDLCommon.h"


namespace {	// un-named namespace

IDL_VPTR vdfMetadataCreate(int argc, IDL_VPTR *argv, char *argk)
{
	typedef struct {
		IDL_KW_RESULT_FIRST_FIELD;  //
		IDL_LONG bs;
		IDL_LONG nFilterCoef;
		IDL_LONG nLiftingCoef;
		IDL_LONG msbFirst;
	} KW_RESULT;

	static IDL_KW_PAR kw_pars[] = {
		{
			"BS", IDL_TYP_LONG, 1, 0, 0, (char *) IDL_KW_OFFSETOF(bs)
		},
		{
			"NFILTERCOEF", IDL_TYP_LONG, 1, 0, 0, 
			(char *) IDL_KW_OFFSETOF(nFilterCoef)
		},
		{
			"NLIFTINGCOEF",IDL_TYP_LONG, 1, 0, 0, 
			(char *) IDL_KW_OFFSETOF(nLiftingCoef)
		},
		{
			"MSBFIRST",IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, 
			(char *) IDL_KW_OFFSETOF(msbFirst)
		},
		{NULL}
	};

	KW_RESULT kw;


	kw.bs = 32;
	kw.nFilterCoef = 1;
	kw.nLiftingCoef = 1;
	kw.msbFirst = 1;

	// Process keywords which are mixed in with positional arguments
	// in argv. N.B. the number of positional arguments (non keywords)
	// is returned, **and** positional arguments are moved to the front
	// of argv.
	//
	argc = IDL_KWProcessByOffset(argc, argv, argk, kw_pars,NULL,1,&kw);

	Metadata *metadata;

	if (argc == 1) {
		IDL_ENSURE_SCALAR(argv[0]);

		char *path = IDL_VarGetString(argv[0]);

		metadata = new Metadata(path);
	}
	else if (argc == 2) {
		size_t dim[3];
		
		IDL_ENSURE_ARRAY(argv[0]);
		IDL_ENSURE_SCALAR(argv[1]);

		// Ensure variables are of correct type (convert if needed)

		IDL_VPTR dim_var = IDL_BasicTypeConversion(1, &argv[0],IDL_TYP_LONG);
		IDL_LONG nxforms = IDL_LongScalar(argv[1]);


		if (dim_var->value.arr->n_dim != 1 || dim_var->value.arr->n_elts != 3) {
			errFatal("Input dimension must be a 3-element, 1D array");
		}
		IDL_LONG	*dimptr = (IDL_LONG *) dim_var->value.arr->data;
		dim[0] = dimptr[0];
		dim[1] = dimptr[1];
		dim[2] = dimptr[2];

		size_t bs[3] = {kw.bs,kw.bs,kw.bs};

		metadata = new Metadata(
			dim, nxforms,bs, kw.nFilterCoef,kw.nLiftingCoef, kw.msbFirst
		);

		if (dim_var != argv[0]) IDL_Deltmp(dim_var);
    }
	else {
		errFatal("Wrong number of arguments");
	}

	myBaseErrChk();

//	IDL_VPTR result = IDL_GettmpMEMINT((IDL_MEMINT) metadata);

	IDL_VPTR result = IDL_Gettmp();
	result->value.memint = (IDL_MEMINT) metadata;
	result->type = IDL_TYP_MEMINT;


	// Free keyword storage.
	IDL_KW_FREE;

	return(result);
}

void vdfMetadataDestroy(int argc, IDL_VPTR *argv)
{
	Metadata	*metadata = varGetMetadata(argv[0]);

	delete metadata;

}

void vdfMetadataWrite(int argc, IDL_VPTR *argv)
{
	Metadata *metadata = varGetMetadata(argv[0]);
	char *path = IDL_VarGetString(argv[1]);

	metadata->Write(path);
	myBaseErrChk();
}
	
IDL_VPTR vdfMetadataGetBlockSize(int argc, IDL_VPTR *argv)
{
	Metadata *metadata = varGetMetadata(argv[0]);

	const size_t *bs = metadata->GetBlockSize();

	IDL_VPTR result;
	IDL_LONG *lptr;

	lptr = (IDL_LONG *) IDL_MakeTempVector(
		IDL_TYP_LONG, 3, IDL_ARR_INI_NOP, &result
	);
	lptr[0] = bs[0];
	lptr[1] = bs[1];
	lptr[2] = bs[2];

	return(result);
}
	
IDL_VPTR vdfMetadataGetDimension(int argc, IDL_VPTR *argv)
{
	Metadata *metadata = varGetMetadata(argv[0]);

	const size_t *dim = metadata->GetDimension();

	IDL_VPTR result;
	IDL_LONG *lptr;

	lptr = (IDL_LONG *) IDL_MakeTempVector(
		IDL_TYP_LONG, 3, IDL_ARR_INI_NOP, &result
	);
	lptr[0] = dim[0];
	lptr[1] = dim[1];
	lptr[2] = dim[2];

	return(result);
}

IDL_VPTR vdfMetadataGetFilterCoef(int argc, IDL_VPTR *argv)
{
	Metadata *metadata = varGetMetadata(argv[0]);

	int value = metadata->GetFilterCoef();
	myBaseErrChk();

	return(IDL_GettmpLong((IDL_LONG) value));
}

IDL_VPTR vdfMetadataGetLiftingCoef(int argc, IDL_VPTR *argv)
{
	Metadata *metadata = varGetMetadata(argv[0]);

	int value = metadata->GetLiftingCoef();
	myBaseErrChk();

	return(IDL_GettmpLong((IDL_LONG) value));
}

IDL_VPTR vdfMetadataGetNumTransforms(int argc, IDL_VPTR *argv)
{
	Metadata *metadata = varGetMetadata(argv[0]);

	int value = metadata->GetNumTransforms();
	myBaseErrChk();

	return(IDL_GettmpLong((IDL_LONG) value));
}

IDL_VPTR vdfMetadataGetMSBFirst(int argc, IDL_VPTR *argv)
{
	Metadata *metadata = varGetMetadata(argv[0]);

	int value = metadata->GetMSBFirst();
	myBaseErrChk();

	return(IDL_GettmpLong((IDL_LONG) value));
}


};

//	prototype for IDL_Load 
extern "C" int IDL_Load( void );
int IDL_Load(void)
{
	extern int	IDL_LoadMeta();
	extern int	IDL_LoadIO();
	extern int	IDL_LoadVaporImport();
	extern int	IDL_LoadLifting();
	//
	//These tables contain information on the functions and procedures
	//that make up the TESTMODULE DLM. The information contained in these
	//tables must be identical to that contained in testmodule.dlm.
	//
	static IDL_SYSFUN_DEF2 func_addr[] = {
		{ (IDL_SYSRTN_GENERIC) vdfMetadataCreate, 
			"VDF_CREATE", 1, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdfMetadataGetBlockSize, 
			"VDF_GETBLOCKSIZE", 1, 1, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdfMetadataGetDimension, 
			"VDF_GETDIMENSION", 1, 1, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdfMetadataGetFilterCoef, 
			"VDF_GETFILTERCOEF", 1, 1, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdfMetadataGetLiftingCoef, 
			"VDF_GETLIFTINGCOEF", 1, 1, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdfMetadataGetNumTransforms, 
			"VDF_GETNUMTRANSFORMS", 1, 1, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdfMetadataGetMSBFirst, 
			"VDF_GETMSBFIRST", 1, 1, 0, 0
		},
	};

	static IDL_SYSFUN_DEF2 proc_addr[] = {
		{ (IDL_SYSRTN_GENERIC) vdfMetadataDestroy, 
			"VDF_DESTROY", 1, 1, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdfMetadataWrite, 
			"VDF_WRITE", 2, 2, 0, 0
		},
	};

	if (! IDL_LoadMeta()) return(0);
	if (! IDL_LoadIO()) return(0);
	if (! IDL_LoadVaporImport()) return(0);
	if (! IDL_LoadLifting()) return(0);

	return IDL_SysRtnAdd(func_addr, TRUE, IDL_CARRAY_ELTS(func_addr)) && 
		IDL_SysRtnAdd(proc_addr, FALSE, IDL_CARRAY_ELTS(proc_addr));

}

