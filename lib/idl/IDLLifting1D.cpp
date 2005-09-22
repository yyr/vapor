#include <cstdarg>
#include <cstdio>
#include <cassert>
#include <idl_export.h>
#include <vapor/Lifting1D.h>
#include <vapor/MyBase.h>
#include "IDLCommon.h"

using namespace VAPoR;
using namespace VetsUtil;

namespace {	// un-named namespace

typedef struct  {
	Lifting1D	**liftings;
	float	*buf;
	unsigned int nxforms;
	unsigned int width;
} Lifting1DHandle_t;

Lifting1DHandle_t *varGetLiftingHandle(
    IDL_VPTR var
) {

    IDL_ENSURE_SCALAR(var);

    if (var->type != IDL_TYP_MEMINT) {
        errFatal("Metadata handle must be of type IDL_TYP_MEMINT");
    }
    return((Lifting1DHandle_t *) var->value.memint);
}

IDL_VPTR Lifting1DCreate(int argc, IDL_VPTR *argv, char *argk)
{
	typedef struct {
		IDL_KW_RESULT_FIRST_FIELD;  //
		IDL_LONG nFilterCoef;
		IDL_LONG nLiftingCoef;
	} KW_RESULT;

	static IDL_KW_PAR kw_pars[] = {
		{
			"NFILTERCOEF", IDL_TYP_LONG, 1, 0, 0, 
			(char *) IDL_KW_OFFSETOF(nFilterCoef)
		},
		{
			"NLIFTINGCOEF",IDL_TYP_LONG, 1, 0, 0, 
			(char *) IDL_KW_OFFSETOF(nLiftingCoef)
		},
		{NULL}
	};

	KW_RESULT kw;


	kw.nFilterCoef = 1;
	kw.nLiftingCoef = 1;

	// Process keywords which are mixed in with positional arguments
	// in argv. N.B. the number of positional arguments (non keywords)
	// is returned, **and** positional arguments are moved to the front
	// of argv.
	//
	argc = IDL_KWProcessByOffset(argc, argv, argk, kw_pars,NULL,1,&kw);


	if (argc != 2) {
		errFatal("Wrong number of arguments");
	}

		
	IDL_ENSURE_SCALAR(argv[0]);
	IDL_ENSURE_SCALAR(argv[1]);

		// Ensure variables are of correct type (convert if needed)

	IDL_LONG width = IDL_LongScalar(argv[0]);
	IDL_LONG nxforms = IDL_LongScalar(argv[1]);


	// Allocate one Lifting1D for each transform
    //
    Lifting1D	**liftings = new Lifting1D*[nxforms];
    for(int l=0; l<nxforms; l++) liftings[l] = NULL;
    for(int l=0; l<nxforms; l++) {
        liftings[l] = new Lifting1D(kw.nFilterCoef,kw.nLiftingCoef,width);

        if (MyBase::GetErrCode() != 0) {
            for(int ll = 0; ll<l; ll++) delete liftings[ll];
			string msg(Lifting1D::GetErrMsg());
			Lifting1D::SetErrCode(0);
			errFatal(msg.c_str());
        }
    }

    Lifting1DHandle_t *lh = new Lifting1DHandle_t();
    lh->buf = new float[width];
    lh->nxforms = nxforms;
    lh->width = width;
    lh->liftings = liftings;


	IDL_VPTR result = IDL_Gettmp();
	result->value.memint = (IDL_MEMINT) lh;
	result->type = IDL_TYP_MEMINT;


	// Free keyword storage.
	IDL_KW_FREE;

	return(result);
}


void Lifting1DDestroy(int argc, IDL_VPTR *argv)
{
	
	Lifting1DHandle_t	*lh = varGetLiftingHandle(argv[0]);
	Lifting1D   **liftings = (Lifting1D **) lh->liftings;

	for(int l=0; l<lh->nxforms; l++) {
		if (liftings[l]) delete liftings[l];
	}
	delete [] liftings;
	delete [] lh->buf;
	delete lh;
}

void	Lifting1DForwardTransform(int argc, IDL_VPTR *argv)
{
    Lifting1DHandle_t   *lh = varGetLiftingHandle(argv[0]);
	Lifting1D	**liftings = (Lifting1D **) lh->liftings;
	int	l;
	int	i,j;
	unsigned int width = lh->width;
	float	*lambda_ptr, *gamma_ptr;

	IDL_VPTR    data_var = argv[1];

	IDL_ENSURE_SIMPLE(data_var);
	IDL_ENSURE_ARRAY(data_var);

	if (data_var->flags & IDL_V_TEMP) {
		errFatal("Input array not modifiable");
	}

	if (data_var->value.arr->n_dim != 1) {
		errFatal("Input array must be a 1D array");
	}

	if (data_var->type != IDL_TYP_FLOAT) {
		errFatal("Input array must be of type float");
	}

	float *data = (float *) data_var->value.arr->data;


	for(l=0; l<lh->nxforms; l++) {

		memcpy(lh->buf, data, width * sizeof(data[0]));

		liftings[l]->ForwardTransform(lh->buf);

		// Lifting1D object class operates on data in place,
		// interleaving coefficients
		//
		lambda_ptr = data;
		gamma_ptr = data + (width>>1);
		for(i=0,j=0; i<width; i+=2,j++) {
			lambda_ptr[j] = lh->buf[i];
			gamma_ptr[j] = lh->buf[i+1];
		}

		width = width >> 1;
	}
}

void	Lifting1DInverseTransform(int argc, IDL_VPTR *argv)
{
    Lifting1DHandle_t   *lh = varGetLiftingHandle(argv[0]);
	Lifting1D	**liftings = (Lifting1D **) lh->liftings;
	int	l;
	int	i,j;
	unsigned int width = lh->width >> lh->nxforms;
	float	*lambda_ptr, *gamma_ptr;

	IDL_VPTR    data_var = argv[1];

	IDL_ENSURE_SIMPLE(data_var);
	IDL_ENSURE_ARRAY(data_var);

	if (data_var->flags & IDL_V_TEMP) {
		errFatal("Input array not modifiable");
	}

	if (data_var->value.arr->n_dim != 1) {
		errFatal("Input array must be a 1D array");
	}

	if (data_var->type != IDL_TYP_FLOAT) {
		errFatal("Input array must be of type float");
	}

	float *data = (float *) data_var->value.arr->data;

	for(l=lh->nxforms-1; l>=0; l--) {

		lambda_ptr = data;
		gamma_ptr = data + width;
		for(i=0,j=0; j<width; i+=2,j++) {
			lh->buf[i] = lambda_ptr[j];
			lh->buf[i+1] = gamma_ptr[j];
		}

		liftings[l]->InverseTransform(lh->buf);

		memcpy(data, lh->buf, (width<<1) * sizeof(data[0]));

		width = width << 1;
	}
}



};


int IDL_LoadLifting(void)
{
	//
	//These tables contain information on the functions and procedures
	//that make up the TESTMODULE DLM. The information contained in these
	//tables must be identical to that contained in testmodule.dlm.
	//
	static IDL_SYSFUN_DEF2 func_addr[] = {
		{ (IDL_SYSRTN_GENERIC) Lifting1DCreate,
			"LIFTING1D_CREATE", 1, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0
		},
	};

	static IDL_SYSFUN_DEF2 proc_addr[] = {
		{ (IDL_SYSRTN_GENERIC) Lifting1DDestroy, 
			"LIFTING1D_DESTROY", 1, 1, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) Lifting1DForwardTransform, 
			"LIFTING1D_FORWARDTRANSFORM", 2, 2, 0, 0
		},
		{ (IDL_SYSRTN_GENERIC) Lifting1DInverseTransform, 
			"LIFTING1D_INVERSETRANSFORM", 2, 2, 0, 0
		},
	};

	return IDL_SysRtnAdd(func_addr, TRUE, IDL_CARRAY_ELTS(func_addr)) && 
		IDL_SysRtnAdd(proc_addr, FALSE, IDL_CARRAY_ELTS(proc_addr));

}

