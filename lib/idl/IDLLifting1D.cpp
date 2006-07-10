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
	Lifting1D<float>	**flt_liftings;
	Lifting1D<double>	**dbl_liftings;
	unsigned char	*buf;
	unsigned int nxforms;
	unsigned int width;
	int dbl;
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
        IDL_LONG dbl;
		IDL_LONG nFilterCoef;
		IDL_LONG nLiftingCoef;
	} KW_RESULT;

	static IDL_KW_PAR kw_pars[] = {
		{
			"DOUBLE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0,
			(char *) IDL_KW_OFFSETOF(dbl)
		},
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
	kw.dbl = 0;

	int normalize = 1;

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


	Lifting1D<double>	**dbl_liftings = NULL;
	Lifting1D<float>	**flt_liftings = NULL;
	unsigned char *buf = NULL;
	unsigned int w = (unsigned int) width;
	if (kw.dbl) {
		// Allocate one Lifting1D for each transform
		//
		dbl_liftings = new Lifting1D<double>* [nxforms];
		for(int l=0; l<nxforms; l++) dbl_liftings[l] = NULL;
		for(int l=0; l<nxforms; l++) {
			dbl_liftings[l] = new Lifting1D<double> (
				(unsigned int) kw.nFilterCoef,
				(unsigned int) kw.nLiftingCoef,
				(unsigned int) w, normalize
			);

			if (MyBase::GetErrCode() != 0) {
				for(int ll = 0; ll<l; ll++) delete dbl_liftings[ll];
				string msg(Lifting1D<double>::GetErrMsg());
				Lifting1D<double>::SetErrCode(0);
				errFatal(msg.c_str());
			}
			w -= (w>>1);
		}
		buf = new unsigned char[sizeof(double) * width];
	}
	else {
		// Allocate one Lifting1D for each transform
		//
		flt_liftings = new Lifting1D<float>* [nxforms];
		for(int l=0; l<nxforms; l++) flt_liftings[l] = NULL;
		for(int l=0; l<nxforms; l++) {
			flt_liftings[l] = new Lifting1D<float> (
				kw.nFilterCoef,
				kw.nLiftingCoef,
				w, normalize
			);

			if (MyBase::GetErrCode() != 0) {
				for(int ll = 0; ll<l; ll++) delete flt_liftings[ll];
				string msg(Lifting1D<float>::GetErrMsg());
				Lifting1D<float>::SetErrCode(0);
				errFatal(msg.c_str());
			}
			w -= (w>>1);
		}
		buf = new unsigned char[sizeof(float) * width];
	}

    Lifting1DHandle_t *lh = new Lifting1DHandle_t();
    lh->buf = buf;
    lh->nxforms = nxforms;
    lh->width = width;
    lh->flt_liftings = flt_liftings;
    lh->dbl_liftings = dbl_liftings;
	lh->dbl = kw.dbl;


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

	if (lh->flt_liftings) {
		for(int l=0; l<lh->nxforms; l++) {
			if (lh->flt_liftings[l]) delete lh->flt_liftings[l];
		}
		delete [] lh->flt_liftings;
	}
	if (lh->dbl_liftings) {
		for(int l=0; l<lh->nxforms; l++) {
			if (lh->dbl_liftings[l]) delete lh->dbl_liftings[l];
		}
		delete [] lh->dbl_liftings;
	}
	if (lh->buf) delete [] lh->buf;
	delete lh;
}

void	Lifting1DForwardTransform(int argc, IDL_VPTR *argv)
{
    Lifting1DHandle_t   *lh = varGetLiftingHandle(argv[0]);
	unsigned int width = lh->width;
	int	l;
	int	i,j;

	IDL_VPTR    data_var = argv[1];

	IDL_ENSURE_SIMPLE(data_var);
	IDL_ENSURE_ARRAY(data_var);

	if (data_var->flags & IDL_V_TEMP) {
		errFatal("Input array not modifiable");
	}

	if (data_var->value.arr->n_dim != 1) {
		errFatal("Input array must be a 1D array");
	}



	if (lh->dbl) {
		if (data_var->type != IDL_TYP_DOUBLE) {
			errFatal("Input array must be of type double");
		}

		double *data = (double *) data_var->value.arr->data;

		double	*lambda_ptr, *gamma_ptr;
		double *dblbuf = (double *) lh->buf;

		for(l=0; l<lh->nxforms; l++) {

			memcpy(dblbuf, data, width * sizeof(data[0]));

			lh->dbl_liftings[l]->ForwardTransform(dblbuf);


			// Lifting1D object class operates on data in place,
			// interleaving coefficients
			//
			lambda_ptr = data;
			gamma_ptr = data + (width - (width>>1));
			for(i=0,j=0; j<width>>1; i+=2,j++) {
				lambda_ptr[j] = dblbuf[i];
				gamma_ptr[j] = dblbuf[i+1];
			}
			if (IsOdd(width)) lambda_ptr[j] = dblbuf[i];

			width -= (width >> 1);
		}
	}
	else {
		if (data_var->type != IDL_TYP_FLOAT) {
			errFatal("Input array must be of type float");
		}

		float *data = (float *) data_var->value.arr->data;

		float	*lambda_ptr, *gamma_ptr;
		float *fltbuf = (float *) lh->buf;

		for(l=0; l<lh->nxforms; l++) {

			memcpy(fltbuf, data, width * sizeof(data[0]));

			lh->flt_liftings[l]->ForwardTransform(fltbuf);

			// Lifting1D object class operates on data in place,
			// interleaving coefficients
			//
			lambda_ptr = data;
			gamma_ptr = data + (width - (width>>1));
			for(i=0,j=0; j<width>>1; i+=2,j++) {
				lambda_ptr[j] = fltbuf[i];
				gamma_ptr[j] = fltbuf[i+1];
			}
			if (IsOdd(width)) lambda_ptr[j] = fltbuf[i];

			width -= (width >> 1);
		}
	}
}

void	Lifting1DInverseTransform(int argc, IDL_VPTR *argv)
{
    Lifting1DHandle_t   *lh = varGetLiftingHandle(argv[0]);
	unsigned int width = lh->width;
	int	l;
	int	i,j;

	IDL_VPTR    data_var = argv[1];

	IDL_ENSURE_SIMPLE(data_var);
	IDL_ENSURE_ARRAY(data_var);

	if (data_var->flags & IDL_V_TEMP) {
		errFatal("Input array not modifiable");
	}

	if (data_var->value.arr->n_dim != 1) {
		errFatal("Input array must be a 1D array");
	}

	if (lh->dbl) {
		if (data_var->type != IDL_TYP_DOUBLE) {
			errFatal("Input array must be of type double");
		}

		double	*lambda_ptr, *gamma_ptr;
		double *dblbuf = (double *) lh->buf;

		double *data = (double *) data_var->value.arr->data;

		for(l=lh->nxforms-1; l>=0; l--) {

			width = lh->width;
			for(int k=0; k<l; k++) width -= (width >> 1);


			lambda_ptr = data;
			gamma_ptr = data + (width - (width>>1));
			for(i=0,j=0; j<width>>1; i+=2,j++) {
				dblbuf[i] = lambda_ptr[j];
				dblbuf[i+1] = gamma_ptr[j];
			}
			if (IsOdd(width)) dblbuf[i] = lambda_ptr[j];

			lh->dbl_liftings[l]->InverseTransform(dblbuf);

			memcpy(data, dblbuf, width * sizeof(data[0]));

		}
	}
	else {
		if (data_var->type != IDL_TYP_FLOAT) {
			errFatal("Input array must be of type float");
		}

		float	*lambda_ptr, *gamma_ptr;
		float *fltbuf = (float *) lh->buf;

		float *data = (float *) data_var->value.arr->data;

		for(l=lh->nxforms-1; l>=0; l--) {

			width = lh->width;
			for(int k=0; k<l; k++) width -= (width >> 1);

			lambda_ptr = data;
			gamma_ptr = data + (width - (width>>1));
			for(i=0,j=0; j<width>>1; i+=2,j++) {
				fltbuf[i] = lambda_ptr[j];
				fltbuf[i+1] = gamma_ptr[j];
			}
			if (IsOdd(width)) fltbuf[i] = lambda_ptr[j];

			lh->flt_liftings[l]->InverseTransform(fltbuf);

			memcpy(data, fltbuf, width * sizeof(data[0]));

		}
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
		}
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
		}
	};

	return IDL_SysRtnAdd(func_addr, TRUE, IDL_CARRAY_ELTS(func_addr)) && 
		IDL_SysRtnAdd(proc_addr, FALSE, IDL_CARRAY_ELTS(proc_addr));

}

