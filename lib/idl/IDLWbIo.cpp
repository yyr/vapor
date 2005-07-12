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
//	Date:		Wed Feb 16 16:42:37 MST 2005
//
//	Description:	
//
#include <cstdarg>
#include <cstdio>
#include <cassert>
#include <idl_export.h>
#include <vapor/WaveletBlock3DBufReader.h>
#include <vapor/WaveletBlock3DRegionReader.h>
#include <vapor/WaveletBlock3DBufWriter.h>
#include "IDLCommon.h"


namespace {	// un-named namespace


WaveletBlock3DIO *varGetIO(
	IDL_VPTR var
) {

	IDL_ENSURE_SCALAR(var);


	if (var->type != IDL_TYP_MEMINT) {
		errFatal("VDC IO handle must be of type IDL_TYP_MEMINT");
	}

	WaveletBlock3DIO	*io = (WaveletBlock3DIO *) var->value.memint;
	const string	&classname = io->GetClassName();

	if (! ((classname.compare("WaveletBlock3DBufReader") == 0) ||
		(classname.compare("WaveletBlock3DRegionReader") == 0) ||
		(classname.compare("WaveletBlock3DBufWriter") == 0))) { 

		errFatal("Invalid VDC IO handle type for operation");
	}
	return((WaveletBlock3DIO *) var->value.memint);
}



IDL_VPTR vdcBufReaderCreate(int argc, IDL_VPTR *argv)
{
	IDL_VPTR arg = argv[0];
	WaveletBlock3DBufReader *reader;

	IDL_ENSURE_SCALAR(arg);

	if (arg->type == IDL_TYP_MEMINT) {
		Metadata *metadata = varGetMetadata(arg);
		reader = new WaveletBlock3DBufReader(metadata);
	}
	else {
		char *path = IDL_VarGetString(arg);
		reader = new WaveletBlock3DBufReader(path);
	}

	myBaseErrChk();


//	IDL_VPTR result = IDL_GettmpMEMINT((IDL_MEMINT) reader);
	IDL_VPTR result = IDL_Gettmp();
	result->value.memint = (IDL_MEMINT) reader;
	result->type = IDL_TYP_MEMINT;

	return(result);
}

void vdcBufReaderDestroy(int argc, IDL_VPTR *argv)
{

	WaveletBlock3DIO	*io = (WaveletBlock3DIO *) varGetIO(argv[0]);
	const string	&classname = io->GetClassName();

	if (! (classname.compare("WaveletBlock3DBufReader") == 0)) {
		errFatal("Invalid VDC IO handle type for operation");
	}

	WaveletBlock3DBufReader	*obj = (WaveletBlock3DBufReader *) io;

	delete obj;
}

void vdcReadSlice(int argc, IDL_VPTR *argv)
{
	WaveletBlock3DIO	*io = (WaveletBlock3DIO *) varGetIO(argv[0]);
	IDL_VPTR	slice_var = argv[1];

	const string	&classname = io->GetClassName();

	if (! (classname.compare("WaveletBlock3DBufReader") == 0)) {
		errFatal("Invalid VDC IO handle type for operation");
	}

	WaveletBlock3DBufReader	*obj = (WaveletBlock3DBufReader *) io;

	IDL_ENSURE_SIMPLE(slice_var);
	IDL_ENSURE_ARRAY(slice_var);

	if (slice_var->flags & IDL_V_TEMP) {
		errFatal("Input slice not modifiable");
	}

	if (slice_var->value.arr->n_dim != 2) {
		errFatal("Input slice must be a 2D array");
	}

	if (slice_var->type != IDL_TYP_FLOAT) {
		errFatal("Input slice must be of type float");
	}

	obj->ReadSlice((float *)  slice_var->value.arr->data);

	myBaseErrChk();
}



IDL_VPTR vdcRegionReaderCreate(int argc, IDL_VPTR *argv)
{
	IDL_VPTR arg = argv[0];
	WaveletBlock3DRegionReader *reader;

	IDL_ENSURE_SCALAR(arg);

	if (arg->type == IDL_TYP_MEMINT) {
		Metadata *metadata = varGetMetadata(arg);
		reader = new WaveletBlock3DRegionReader(metadata);
	}
	else {
		char *path = IDL_VarGetString(arg);
		reader = new WaveletBlock3DRegionReader(path);
	}

	myBaseErrChk();


//	IDL_VPTR result = IDL_GettmpMEMINT((IDL_MEMINT) reader);
	IDL_VPTR result = IDL_Gettmp();
	result->value.memint = (IDL_MEMINT) reader;
	result->type = IDL_TYP_MEMINT;

	return(result);
}

void vdcRegionReaderDestroy(int argc, IDL_VPTR *argv)
{
	WaveletBlock3DIO	*io = (WaveletBlock3DIO *) varGetIO(argv[0]);
	const string	&classname = io->GetClassName();

	if (! (classname.compare("WaveletBlock3DRegionReader") == 0)) {
		errFatal("Invalid VDC IO handle type for operation");
	}

	WaveletBlock3DRegionReader	*obj = (WaveletBlock3DRegionReader *) io;

	delete obj;
}

void vdcReadRegion(int argc, IDL_VPTR *argv)
{
	WaveletBlock3DIO	*io = (WaveletBlock3DIO *) varGetIO(argv[0]);
	IDL_VPTR min_var = IDL_BasicTypeConversion(1, &argv[1],IDL_TYP_LONG);
	IDL_VPTR max_var = IDL_BasicTypeConversion(1, &argv[2],IDL_TYP_LONG);
	IDL_VPTR	region_var = argv[3];

	const string	&classname = io->GetClassName();

	if (! (classname.compare("WaveletBlock3DRegionReader") == 0)) {
		errFatal("Invalid VDC IO handle type for operation");
	}

	WaveletBlock3DRegionReader	*obj = (WaveletBlock3DRegionReader *) io;

	IDL_ENSURE_SIMPLE(region_var);
	IDL_ENSURE_ARRAY(region_var);

	if (region_var->flags & IDL_V_TEMP) {
		errFatal("Input region not modifiable");
	}

	if (region_var->value.arr->n_dim != 3) {
		errFatal("Input region must be a 3D array");
	}

	if (region_var->type != IDL_TYP_FLOAT) {
		errFatal("Input region must be of type float");
	}

	IDL_LONG    *minptr = (IDL_LONG *) min_var->value.arr->data;
	IDL_LONG    *maxptr = (IDL_LONG *) max_var->value.arr->data;


	size_t min[3];
	size_t max[3];

	for(int i=0; i<3; i++) {
		min[i] = (size_t) minptr[i];
		max[i] = (size_t) maxptr[i];
	}

	obj->ReadRegion(min, max, (float *) region_var->value.arr->data);

	myBaseErrChk();

	if (min_var != argv[1]) IDL_Deltmp(min_var);
	if (max_var != argv[2]) IDL_Deltmp(max_var);
}

IDL_VPTR vdcBufWriterCreate(int argc, IDL_VPTR *argv)
{
	IDL_VPTR arg = argv[0];
	WaveletBlock3DBufWriter *writer;

	IDL_ENSURE_SCALAR(arg);

	if (arg->type == IDL_TYP_MEMINT) {
		Metadata *metadata = varGetMetadata(arg);
		writer = new WaveletBlock3DBufWriter(metadata);
	}
	else {
		char *path = IDL_VarGetString(arg);
		writer = new WaveletBlock3DBufWriter(path);
	}

	myBaseErrChk();


//	IDL_VPTR result = IDL_GettmpMEMINT((IDL_MEMINT) writer);
	IDL_VPTR result = IDL_Gettmp();
	result->value.memint = (IDL_MEMINT) writer;
	result->type = IDL_TYP_MEMINT;

	return(result);
}

void vdcBufWriterDestroy(int argc, IDL_VPTR *argv)
{
	WaveletBlock3DIO	*io = (WaveletBlock3DIO *) varGetIO(argv[0]);
	const string	&classname = io->GetClassName();

	if (! (classname.compare("WaveletBlock3DBufWriter") == 0)) {
		errFatal("Invalid VDC IO handle type for operation");
	}

	WaveletBlock3DBufWriter	*obj = (WaveletBlock3DBufWriter *) io;

	delete obj;
}

void vdcWriteSlice(int argc, IDL_VPTR *argv)
{
	WaveletBlock3DIO	*io = (WaveletBlock3DIO *) varGetIO(argv[0]);
	IDL_VPTR	slice_var = argv[1];

	const string	&classname = io->GetClassName();

	if (! (classname.compare("WaveletBlock3DBufWriter") == 0)) {
		errFatal("Invalid VDC IO handle type for operation");
	}

	WaveletBlock3DBufWriter	*obj = (WaveletBlock3DBufWriter *) io;

	IDL_ENSURE_SIMPLE(slice_var);
	IDL_ENSURE_ARRAY(slice_var);

	if (slice_var->value.arr->n_dim != 2) {
		errFatal("Input slice must be a 2D array");
	}

	if (slice_var->type != IDL_TYP_FLOAT) {
		errFatal("Input slice must be of type float");
	}

	obj->WriteSlice((float *)  slice_var->value.arr->data);
	myBaseErrChk();
}

void vdcOpenVarRead(int argc, IDL_VPTR *argv)
{
	WaveletBlock3DIO	*io = varGetIO(argv[0]);
	IDL_LONG ts = IDL_LongScalar(argv[1]);
	char *varname = IDL_VarGetString(argv[2]);
	IDL_LONG num_xforms = IDL_LongScalar(argv[3]);

	const string &classname = io->GetClassName();
	if (classname.compare("WaveletBlock3DBufReader") == 0) { 
		WaveletBlock3DBufReader *reader = (WaveletBlock3DBufReader *) io;
		reader->OpenVariableRead((size_t) ts, varname, num_xforms);
	}
	else {
		WaveletBlock3DRegionReader *reader = (WaveletBlock3DRegionReader *) io;
		reader->OpenVariableRead((size_t) ts, varname, num_xforms);
	}

	myBaseErrChk();
}


void vdcOpenVarWrite(int argc, IDL_VPTR *argv)
{
	WaveletBlock3DIO	*io = (WaveletBlock3DIO *) varGetIO(argv[0]);
	IDL_LONG ts = IDL_LongScalar(argv[1]);
	char *varname = IDL_VarGetString(argv[2]);


	const string	&classname = io->GetClassName();


	if (! (classname.compare("WaveletBlock3DBufWriter") == 0)) {
		errFatal("Invalid VDC IO handle type for operation");
	}

	WaveletBlock3DBufWriter	*obj = (WaveletBlock3DBufWriter *) io;

	if (argc == 3) {
		obj->OpenVariableWrite((size_t) ts, varname);
	}
	else {
		IDL_LONG num_xforms = IDL_LongScalar(argv[3]);
		obj->OpenVariableWrite((size_t) ts, varname, num_xforms);
	}
	myBaseErrChk();
}


void vdcCloseVar(int argc, IDL_VPTR *argv)
{
	WaveletBlock3DIO	*io = varGetIO(argv[0]);

	const string &classname = io->GetClassName();
	if (classname.compare("WaveletBlock3DBufReader") == 0) { 
		WaveletBlock3DBufReader *obj = (WaveletBlock3DBufReader *) io;
		obj->CloseVariable();
	}
	else if (classname.compare("WaveletBlock3DRegionReader") == 0) { 
		WaveletBlock3DRegionReader *obj = (WaveletBlock3DRegionReader *) io;
		obj->CloseVariable();
	}
	else {
		WaveletBlock3DBufWriter *obj = (WaveletBlock3DBufWriter *) io;
		obj->CloseVariable();
	}

	myBaseErrChk();
}

IDL_VPTR vdcVarExists(int argc, IDL_VPTR *argv)
{
	WaveletBlock3DIO	*io = varGetIO(argv[0]);
	IDL_LONG ts = IDL_LongScalar(argv[1]);
	char *varname = IDL_VarGetString(argv[2]);
	IDL_LONG num_xforms = IDL_LongScalar(argv[3]);

	int rc = io->VariableExists((size_t) ts, varname, (size_t) num_xforms);

	myBaseErrChk();

	return(IDL_GettmpLong((IDL_LONG) rc));

}

IDL_VPTR vdcGetDim(int argc, IDL_VPTR *argv)
{
	WaveletBlock3DIO	*io = varGetIO(argv[0]);
	IDL_LONG num_xforms = IDL_LongScalar(argv[1]);


	size_t dim[3];
	io->GetDim(num_xforms, dim);

	myBaseErrChk();

	IDL_VPTR result;
	IDL_LONG *result_ptr = (IDL_LONG *) IDL_MakeTempVector(
		IDL_TYP_LONG, 3, IDL_ARR_INI_NOP, &result
	);

	for(int i=0; i<3; i++) {
		result_ptr[i] = (IDL_LONG) dim[i];
	}

    return(result);

}

IDL_VPTR vdcTransformCoord(int argc, IDL_VPTR *argv)
{
	WaveletBlock3DIO	*io = varGetIO(argv[0]);
	IDL_LONG num_xforms = IDL_LongScalar(argv[1]);
	IDL_VPTR vcoord_var = IDL_BasicTypeConversion(1, &argv[2],IDL_TYP_LONG);

	if (vcoord_var->value.arr->n_dim != 1 || vcoord_var->value.arr->n_elts != 3) {
		errFatal("Input dimension must be a 3-element, 1D array");
	}
	IDL_LONG    *vcoord0ptr = (IDL_LONG *) vcoord_var->value.arr->data;


	size_t vcoord0[3];
	size_t vcoord1[3];

	for(int i=0; i<3; i++) {
		vcoord0[i] = (size_t) vcoord0ptr[i];
	}

	io->TransformCoord(num_xforms, vcoord0, vcoord1);

	myBaseErrChk();

	IDL_VPTR result;
	IDL_LONG *result_ptr = (IDL_LONG *) IDL_MakeTempVector(
		IDL_TYP_LONG, 3, IDL_ARR_INI_NOP, &result
	);

	for(int i=0; i<3; i++) {
		result_ptr[i] = (IDL_LONG) vcoord1[i];
	}

	if (vcoord_var != argv[2]) IDL_Deltmp(vcoord_var);

    return(result);

}

IDL_VPTR vdcMapVoxToUser(int argc, IDL_VPTR *argv)
{
	WaveletBlock3DIO	*io = varGetIO(argv[0]);
	IDL_LONG num_xforms = IDL_LongScalar(argv[1]);
	IDL_LONG ts = IDL_LongScalar(argv[2]);
	IDL_VPTR vcoord_var = IDL_BasicTypeConversion(1, &argv[3],IDL_TYP_LONG);

	if (vcoord_var->value.arr->n_dim != 1 || vcoord_var->value.arr->n_elts != 3) {
		errFatal("Input dimension must be a 3-element, 1D array");
	}
	IDL_LONG    *vcoord0ptr = (IDL_LONG *) vcoord_var->value.arr->data;


	size_t vcoord0[3];
	double vcoord1[3];

	for(int i=0; i<3; i++) {
		vcoord0[i] = (size_t) vcoord0ptr[i];
	}

	io->MapVoxToUser(num_xforms, ts, vcoord0, vcoord1);

	myBaseErrChk();

	IDL_VPTR result;
	double *result_ptr = (double *) IDL_MakeTempVector(
		IDL_TYP_DOUBLE, 3, IDL_ARR_INI_NOP, &result
	);

	for(int i=0; i<3; i++) {
		result_ptr[i] = (double) vcoord1[i];
	}

	if (vcoord_var != argv[3]) IDL_Deltmp(vcoord_var);

    return(result);

}

IDL_VPTR vdcMapUserToVox(int argc, IDL_VPTR *argv)
{
	WaveletBlock3DIO	*io = varGetIO(argv[0]);
	IDL_LONG num_xforms = IDL_LongScalar(argv[1]);
	IDL_LONG ts = IDL_LongScalar(argv[2]);
	IDL_VPTR vcoord_var = IDL_BasicTypeConversion(1, &argv[3],IDL_TYP_DOUBLE);

	if (vcoord_var->value.arr->n_dim != 1 || vcoord_var->value.arr->n_elts != 3) {
		errFatal("Input dimension must be a 3-element, 1D array");
	}
	double	*vcoord0ptr = (double *) vcoord_var->value.arr->data;


	double vcoord0[3];
	size_t vcoord1[3];

	for(int i=0; i<3; i++) {
		vcoord0[i] = (size_t) vcoord0ptr[i];
	}

	io->MapUserToVox(num_xforms, ts, vcoord0, vcoord1);

	myBaseErrChk();

	IDL_VPTR result;
	IDL_LONG *result_ptr = (IDL_LONG *) IDL_MakeTempVector(
		IDL_TYP_LONG, 3, IDL_ARR_INI_NOP, &result
	);

	for(int i=0; i<3; i++) {
		result_ptr[i] = (IDL_LONG) vcoord1[i];
	}

	if (vcoord_var != argv[3]) IDL_Deltmp(vcoord_var);

    return(result);

}

IDL_VPTR vdcIsValidRegion(int argc, IDL_VPTR *argv)
{
	WaveletBlock3DIO	*io = varGetIO(argv[0]);
	IDL_LONG num_xforms = IDL_LongScalar(argv[1]);
	IDL_VPTR min_var = IDL_BasicTypeConversion(1, &argv[2],IDL_TYP_LONG);
	IDL_VPTR max_var = IDL_BasicTypeConversion(1, &argv[3],IDL_TYP_LONG);

	if (min_var->value.arr->n_dim != 1 || min_var->value.arr->n_elts != 3) {
		errFatal("Input dimension must be a 3-element, 1D array");
	}
	if (max_var->value.arr->n_dim != 1 || max_var->value.arr->n_elts != 3) {
		errFatal("Input dimension must be a 3-element, 1D array");
	}

	IDL_LONG    *minptr = (IDL_LONG *) min_var->value.arr->data;
	IDL_LONG    *maxptr = (IDL_LONG *) max_var->value.arr->data;


	size_t min[3];
	size_t max[3];

	for(int i=0; i<3; i++) {
		min[i] = (size_t) minptr[i];
		max[i] = (size_t) maxptr[i];
	}

	int rc = io->IsValidRegion(num_xforms, min, max);

	myBaseErrChk();

	if (min_var != argv[2]) IDL_Deltmp(min_var);
	if (max_var != argv[3]) IDL_Deltmp(max_var);

	return(IDL_GettmpLong((IDL_LONG) rc));
}

IDL_VPTR vdcGetMetadata(int argc, IDL_VPTR *argv)
{
	WaveletBlock3DIO	*io = varGetIO(argv[0]);

	const Metadata *metadata = io->GetMetadata();
	myBaseErrChk();


//	IDL_VPTR result = IDL_GettmpMEMINT((IDL_MEMINT) io);
	IDL_VPTR result = IDL_Gettmp();
	result->value.memint = (IDL_MEMINT) io;
	result->type = IDL_TYP_MEMINT;

	return(result);
}

};



int IDL_LoadIO(void)
{
	//
	//These tables contain information on the functions and procedures
	//that make up the TESTMODULE DLM. The information contained in these
	//tables must be identical to that contained in testmodule.dlm.
	//
	static IDL_SYSFUN_DEF2 func_addr[] = {
		{ (IDL_SYSRTN_GENERIC) vdcBufReaderCreate, 
			"VDC_BUFREADCREATE", 1, 1, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdcRegionReaderCreate, 
			"VDC_REGREADCREATE", 1, 1, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdcBufWriterCreate, 
			"VDC_BUFWRITECREATE", 1, 1, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdcVarExists, 
			"VDC_VAREXISTS", 4, 4, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdcGetDim, 
			"VDC_GETDIM", 2, 2, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdcTransformCoord, 
			"VDC_XFORMCOORD", 3, 3, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdcMapVoxToUser, 
			"VDC_MAPVOX2USER", 4, 4, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdcMapUserToVox, 
			"VDC_MAPUSER2VOX", 4, 4, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdcIsValidRegion, 
			"VDC_ISVALIDREG", 4, 4, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdcGetMetadata, 
			"VDC_GETMETADATA", 1, 1, 0, 0
		}
	};

	static IDL_SYSFUN_DEF2 proc_addr[] = {
		{ (IDL_SYSRTN_GENERIC) vdcBufReaderDestroy, 
			"VDC_BUFREADDESTROY", 1, 1, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdcReadSlice, 
			"VDC_BUFREADSLICE", 2, 2, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdcRegionReaderDestroy, 
			"VDC_REGREADDESTROY", 1, 1, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdcReadRegion, 
			"VDC_REGREAD", 4, 4, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdcBufWriterDestroy, 
			"VDC_BUFWRITEDESTROY", 1, 1, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdcWriteSlice, 
			"VDC_BUFWRITESLICE", 2, 2, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdcOpenVarWrite, 
			"VDC_OPENVARWRITE", 3, 4, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdcOpenVarRead, 
			"VDC_OPENVARREAD", 4, 4, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) vdcCloseVar, 
			"VDC_CLOSEVAR", 1, 1, 0, 0
		}
	};

	return IDL_SysRtnAdd(func_addr, TRUE, IDL_CARRAY_ELTS(func_addr)) && 
		IDL_SysRtnAdd(proc_addr, FALSE, IDL_CARRAY_ELTS(proc_addr));

}

