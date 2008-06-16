#include <cstdio>
#include <cstdlib>
#include "vapor/WaveletBlock1D.h"

using namespace VetsUtil;
using namespace VAPoR;

WaveletBlock1D::WaveletBlock1D(
	unsigned int bs,
	unsigned int n,
	unsigned int ntilde
) {
	_lift = NULL;
	_liftbuf = NULL;

	SetClassName("WaveletBlock1D");

	if (n<1 || (IsOdd(n) && n != 1)) {
		SetErrMsg("Invalid # of filter coefficients : n=%d", n);
		return;
	}

	if (ntilde<1 || (IsOdd(ntilde) && ntilde != 1)) {
		SetErrMsg("Invalid # of lifting coefficients : ntilde=%d", ntilde);
		return;
	}

	_bs = bs;
	_n = n;
	_ntilde = ntilde;

	if (_n>1) {
		_lift = new Lifting1D<float> (_n, _ntilde, _bs);
		if (_lift->GetErrCode()) {
			SetErrMsg("Lifting1D() : %s", _lift->GetErrMsg());
			return;
		}
		_liftbuf = new float[_bs];
	}
}


WaveletBlock1D::~WaveletBlock1D() {

	if (_lift) delete _lift;
	if (_liftbuf) delete [] _liftbuf;

	_lift = NULL;
	_liftbuf = NULL;
}




void	WaveletBlock1D::ForwardTransform(
	const float *src_ptr,
	float *lambda_ptr,
	float *gamma_ptr
) {

	if (_n == 1) {
		forward_transform1d_haar(src_ptr, lambda_ptr, gamma_ptr, _bs);
	}
	else {
		int	i,j;
		for(i=0;i<_bs;i++) _liftbuf[i] = src_ptr[i];

		_lift->ForwardTransform(_liftbuf);

		// Lifting1D object class operates on data in place, 
		// interleaving coefficients
		//
		for(i=0,j=0;i<_bs;i+=2,j++) {
			lambda_ptr[j] = _liftbuf[i];
			gamma_ptr[j] = _liftbuf[i+1];
		}
	}
}

void	WaveletBlock1D::forward_transform1d_haar(
	const float *src_ptr,
	float *lambda_ptr,
	float *gamma_ptr,
	int size
) {
	int	i;

	int	nG;	// # gamma coefficients
	int	nL;	// # lambda coefficients
	double	lsum = 0.0;	// sum of lambda values
	double	lave = 0.0;	// average of lambda values

    nG = (size >> 1);
    nL = size - nG;

	//
	// Need to preserve average for odd sizes
	//
	if (IsOdd(size)) {
		double	t = 0.0;

		for(i=0;i<size;i++) {
			t += src_ptr[i];
		}
		lave = t / (double) size;
	}

	for (i=0; i<nG; i++) {
		*gamma_ptr = src_ptr[1] - src_ptr[0];
		*lambda_ptr = (float)(*src_ptr + (*gamma_ptr /2.0));
		lsum += *lambda_ptr;

		src_ptr += 2;
		lambda_ptr++;
		gamma_ptr++;
	}

    // If IsOdd(size), then we have one additional case for */
    // the Lambda calculations. This is a boundary case  */
    // and, therefore, has different filter values.      */
	//
	if (IsOdd(size)) {
		*lambda_ptr = (float)((lave * (double) nL) - lsum);
	}
}

void	WaveletBlock1D::InverseTransform(
	const float *lambda_ptr,
	const float *gamma_ptr,
	float *dst_ptr
) {

	if (_n == 1) {
		inverse_transform1d_haar(lambda_ptr, gamma_ptr, dst_ptr, _bs);
	}
	else {
		int	i,j;

		// Lifting1D object class expects interleaved coefficients
		// and operates on data in place
		//
		for(i=0,j=0;i<_bs;i+=2,j++) {
			dst_ptr[i] = lambda_ptr[j];
			dst_ptr[i+1] = gamma_ptr[j];
		}

		_lift->InverseTransform(dst_ptr);
	}
}

void	WaveletBlock1D::inverse_transform1d_haar(
	const float *lambda_ptr,
	const float *gamma_ptr,
	float *dst_ptr,
	int size
) {
	int	i;
	int	nG;	// # gamma coefficients
	int	nL;	// # lambda coefficients
	double	lsum = 0.0;	// sum of lambda values
	double	lave = 0.0;	// average of lambda values

	nG = (size >> 1);
	nL = size - nG;

    // Odd # of coefficients require special handling at boundary
	// Calculate Lambda average 
	//
    if (IsOdd(size) ) {
        double  t = 0.0;

		for(i=0;i<nL;i++) {
            t += lambda_ptr[i];
        }
        lave = t/(double)nL;   // average we've to maintain
    }

	for (i=0; i<nG; i++) {
		dst_ptr[0] = (float)(*lambda_ptr - (*gamma_ptr * 0.5));
		dst_ptr[1] = *gamma_ptr + dst_ptr[0];
		lsum += dst_ptr[0] +  dst_ptr[1];

		dst_ptr += 2;
		lambda_ptr++;
		gamma_ptr++;
	}

    // If ODD(len), then we have one additional case for */
    // the Lambda calculations. This is a boundary case  */
    // and, therefore, has different filter values.      */
	//
    if (IsOdd(size)) {
        *dst_ptr = (float)((lave * (double) size) - lsum);
    }
}
