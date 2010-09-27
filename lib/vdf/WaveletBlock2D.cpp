#include <cstdio>
#include <cstdlib>
#include <vapor/WaveletBlock2D.h>

using namespace VetsUtil;
using namespace VAPoR;

WaveletBlock2D::WaveletBlock2D(
	size_t bs[2],
	unsigned int n,
	unsigned int ntilde
) {
	_objInitialized = 0;

	_temp_tiles1 = NULL;
	_temp_tiles2 = NULL;
	_wb1d0 = NULL;
	_wb1d1 = NULL;

	SetClassName("WaveletBlock2D");

	_bs[0] = bs[0];
	_bs[1] = bs[1];

	_wb1d0 = new WaveletBlock1D(bs[0], n, ntilde);
	if (_wb1d0->GetErrCode()) {
		SetErrMsg("WaveletBlock1D() : %s", _wb1d0->GetErrMsg());
		return;
	}

	_wb1d1 = new WaveletBlock1D(bs[1], n, ntilde);
	if (_wb1d1->GetErrCode()) {
		SetErrMsg("WaveletBlock1D() : %s", _wb1d1->GetErrMsg());
		return;
	}

	_temp_tiles1 = new float*[4];
	_temp_tiles2 = new float*[4];
	int	i;
	for(i=0;i<4;i++) {
		_temp_tiles1[i] = new float[_bs[0]*_bs[1]];
		_temp_tiles2[i] = new float[_bs[0]*_bs[1]];
	}


	_objInitialized = 1;
}

WaveletBlock2D::~WaveletBlock2D() {

	if (! _objInitialized) return;

	int	i;

	if (_temp_tiles1) {
		for(i=0;i<4;i++) delete [] _temp_tiles1[i];
		delete [] _temp_tiles1;
		_temp_tiles1 = NULL;
	}
	if (_temp_tiles2) {
		for(i=0;i<4;i++) delete [] _temp_tiles2[i];
		delete [] _temp_tiles2;
		_temp_tiles2 = NULL;
	}


	if (_wb1d0) delete _wb1d0;
	if (_wb1d1) delete _wb1d1;

	_wb1d0 = NULL;
	_wb1d1 = NULL;

	_objInitialized = 0;
}

#include <vapor/Transpose.h>

void	WaveletBlock2D::ForwardTransform(
	const float *src_super_tile[4],
	float *dst_super_tile[4]
) {
	int	iy = 1;
	int	ix = 2;
	float *s, *d;

	// X coodinate transform
	//
	forward_transform2D_tiles(
		src_super_tile, _temp_tiles1, &dst_super_tile[ix], 4, 
		_bs[0], _bs[1], _wb1d0
	);
		
	// Transpose X & Y
	//
	for(int i=0; i<2; i++) {
		s = _temp_tiles1[i];
		d = _temp_tiles2[i];

		Transpose(s,d,_bs[0],_bs[1]);
	}

	// Y coodinate transform
	//
	forward_transform2D_tiles(
		(const float **) _temp_tiles2,_temp_tiles1,&dst_super_tile[iy],2,
		_bs[1], _bs[0], _wb1d1
	);


	// now transpose lambda coefficients back to x,y order
	//
	s = _temp_tiles1[0];
	d = dst_super_tile[0];
	Transpose(s,d,_bs[1],_bs[0]);

}

void	WaveletBlock2D::InverseTransform(
	const float *src_super_tile[4],
	float *dst_super_tile[4]
) {

	int	iy = 1;
	int	ix = 2;

	float *s;
	float *d;


	// First transpose lambda tiles so they are in y,x order
	//
	s = (float *) src_super_tile[0];
	d = _temp_tiles1[0];
	Transpose(s,d,_bs[0],_bs[1]);

    inverse_transform2D_tiles(
        (const float **) _temp_tiles1,&src_super_tile[iy], _temp_tiles2, 2,
        _bs[1], _bs[0], _wb1d1
    );

	// Transpose X & Y
	//
	for(int i=0; i<2; i++) {
		s = _temp_tiles2[i];
		d = _temp_tiles1[i];

		Transpose(s,d,_bs[0],_bs[1]);
	}

	// X coodinate transform
	//
	inverse_transform2D_tiles(
		(const float **) _temp_tiles1, &src_super_tile[ix], dst_super_tile, 4, 
		_bs[0], _bs[1], _wb1d0
	);

}


void	WaveletBlock2D::forward_transform2D_tiles(
	const float **src_tiles,
	float **lambda_tiles,
	float **gamma_tiles,
	int ntiles,
	size_t nx,
	size_t ny,
	WaveletBlock1D *wb1d
) {
	const float	*src_tileptr;
	float	*lambda_tileptr;
	float	*gamma_tileptr;

	int	gamma_offset = 0;
	int	lambda_offset = 0;
	int	nG = (nx >> 1);	// # gamma coefficients
	int	nL = nx - nG;	// # lambda coefficients

	int	i,x;

	for(i=0; i<ntiles/2; i++) {

		lambda_tileptr = lambda_tiles[i];
		lambda_offset = 0;
		gamma_tileptr = gamma_tiles[i];
		gamma_offset = 0;

		for(x=0;x<2;x++) {
			src_tileptr = src_tiles[(i*2) + x];

			forward_transform2D(
				src_tileptr, lambda_tileptr, gamma_tileptr,
				lambda_offset, gamma_offset, nx, ny, wb1d
			);

			lambda_offset += nL;
			gamma_offset += nG;
		}
	}
}


void	WaveletBlock2D::forward_transform2D(
	const float	*src_tileptr,
	float	*lambda_tileptr,
	float	*gamma_tileptr,
	int	lambda_offset,
	int	gamma_offset,
	size_t nx,
	size_t ny,
	WaveletBlock1D *wb1d
) {
	const float *src_ptr = src_tileptr;
	float *lambda_ptr = lambda_tileptr + lambda_offset;
	float *gamma_ptr = gamma_tileptr + gamma_offset;

	int	y;

	for(y=0; y<ny; y++) {

		wb1d->ForwardTransform(src_ptr, lambda_ptr, gamma_ptr);
		src_ptr += nx;
		lambda_ptr += nx;
		gamma_ptr += nx;
	}
}



void	WaveletBlock2D::inverse_transform2D_tiles(
	const float **lambda_tiles,
	const float **gamma_tiles,
	float **dst_tiles,
	int ntiles,
	size_t nx,
	size_t ny,
	WaveletBlock1D *wb1d
) {
	const float	*lambda_tileptr;
	const float	*gamma_tileptr;
	float	*dst_ptr;

	int	gamma_offset = 0;
	int	lambda_offset = 0;
	int	nG = (nx >> 1);	// # gamma coefficients
	int	nL = nx - nG;	// # lambda coefficients


	int	i,x;

	for(i=0; i<ntiles/2; i++) {

		lambda_tileptr = lambda_tiles[i];
		lambda_offset = 0;
		gamma_tileptr = gamma_tiles[i];
		gamma_offset = 0;

		for(x=0;x<2;x++) {
			dst_ptr = dst_tiles[(i*2) + x];

			inverse_transform2D(
				lambda_tileptr, gamma_tileptr, dst_ptr,
				lambda_offset, gamma_offset, nx, ny, wb1d
			);

			lambda_offset += nL;
			gamma_offset += nG;

		}
	}
}

void	WaveletBlock2D::inverse_transform2D(
	const float	*lambda_tileptr,
	const float	*gamma_tileptr,
	float	*dst_tileptr,
	int	lambda_offset,
	int	gamma_offset,
	size_t nx,
	size_t ny,
	WaveletBlock1D *wb1d
) {
	const float *lambda_ptr = lambda_tileptr + lambda_offset;
	const float *gamma_ptr = gamma_tileptr + gamma_offset;
	float *dst_ptr = dst_tileptr;

	int	y;

	for(y=0; y<ny; y++) {

		wb1d->InverseTransform(lambda_ptr, gamma_ptr, dst_ptr);
		dst_ptr += nx;
		lambda_ptr += nx;
		gamma_ptr += nx;
	}
}


#ifdef	DEAD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
main(int argc, char **argv) {

	WaveletBlock2D	*wb;
	static const int	bb = 32;
	static const int	nb = 2;
	float		src_super_block[nb*nb*nb][bb][bb][bb];
	float		dst_super_block[nb*nb*nb][bb][bb][bb];
	float		*src_ptr[8];
	float		*dst_ptr[8];
	int		i,k;
	int		x,y,z;
	int		block;
	int		xtile, ytile,ztile;
	int		xx,yy,zz;
	int		index;

	int		bs = bb*bb*bb;

	wb = new WaveletBlock2D(bb,4,4,1);

	for(i=0;i<8;i++) {
		src_ptr[i] = (float *) &src_super_block[i][0][0][0];
		dst_ptr[i] = (float *) &dst_super_block[i][0][0][0];
	}

	memset(src_super_block, 0, sizeof(src_super_block));
	memset(dst_super_block, 0, sizeof(src_super_block));

	for(block=0; block<8; block++) {
		for(zz=0;zz<bb;zz++) {
		for(yy=0;yy<bb;yy++) {
		for(xx=0;xx<bb;xx++) {
			ztile = block / (nb*nb);
			ytile = (block - (ztile * nb * nb)) / nb;
			xtile = block - (ztile * nb * nb) - (ytile*nb);

			z = ztile * bb + zz;
			y = ytile * bb + yy;
			x = xtile * bb + xx;

			index = x + (y*bb*nb) + (z*bb*bb*nb*nb);

	//		src_super_block[block][zz][yy][xx] = 0.0 + (index % 5);
			src_super_block[block][zz][yy][xx] = index;
		}
		}
		}
	}

	wb->ForwardTransform((const float **) src_ptr, dst_ptr);
	memset(src_super_block, 0, sizeof(src_super_block));
	wb->InverseTransform((const float **) dst_ptr, (float **) src_ptr);
	fwrite(src_super_block, 1, sizeof(src_super_block), stdout);


	return(0);
}
#endif	
