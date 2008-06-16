#include <cstdio>
#include <cstdlib>
#include "vapor/WaveletBlock3D.h"

using namespace VetsUtil;
using namespace VAPoR;

WaveletBlock3D::WaveletBlock3D(
	unsigned int bs,
	unsigned int n,
	unsigned int ntilde,
	unsigned int nthreads
) {
	_objInitialized = 0;

	temp_blks1_c = NULL;
	temp_blks2_c = NULL;
	et_c = NULL;
	threads_c = NULL;
	deallocate_c = 0;
	_wb1d = NULL;

	SetClassName("WaveletBlock3D");

	if (! IsPowerOfTwo(bs)) {
		SetErrMsg("Block dimension is not a power of two: bs=%d", bs);
		return;
	}

	bs_c = bs;

	// work decompostion boundaries for threads
	//
	z0_c = 0;
	zr_c = bs_c;

	nthreads_c = nthreads;

	_wb1d = new WaveletBlock1D(bs_c, n, ntilde);
	if (_wb1d->GetErrCode()) {
		SetErrMsg("WaveletBlock1D() : %s", _wb1d->GetErrMsg());
		return;
	}

	temp_blks1_c = new float*[4];
	temp_blks2_c = new float*[4];
	int	i;
	for(i=0;i<4;i++) {
		temp_blks1_c[i] = new float[bs_c*bs_c*bs_c];
		temp_blks2_c[i] = new float[bs_c*bs_c*bs_c];
	}

	src_s_blk_ptr_c = &src_super_blk_c;
	dst_s_blk_ptr_c = &dst_super_blk_c;
	if (nthreads_c > 1) {
		threads_c = new WaveletBlock3D*[nthreads_c];

		et_c = new EasyThreads(nthreads_c);
		if (et_c->GetErrCode() != 0) return;

		for(i=0; i<nthreads_c; i++) {
			threads_c[i] = new WaveletBlock3D(this, i);
		}
	}
	deallocate_c = 1;
	_objInitialized = 1;
}

WaveletBlock3D::WaveletBlock3D(
	WaveletBlock3D *X,
	int	index
) {
	_objInitialized = 0;
	*this = *X;
	this->src_s_blk_ptr_c = &(X->src_super_blk_c);
	this->dst_s_blk_ptr_c = &(X->dst_super_blk_c);

	// member initialization specific to this thread
	//
	this->deallocate_c = 0;
	et_c->Decompose(bs_c, nthreads_c, index, &z0_c, &zr_c);
	_objInitialized = 1;
}

WaveletBlock3D::~WaveletBlock3D() {

	if (! _objInitialized) return;

	if (deallocate_c) {
		int	i;

		if (temp_blks1_c) {
			for(i=0;i<4;i++) delete [] temp_blks1_c[i];
			delete [] temp_blks1_c;
			temp_blks1_c = NULL;
		}
		if (temp_blks2_c) {
			for(i=0;i<4;i++) delete [] temp_blks2_c[i];
			delete [] temp_blks2_c;
			temp_blks2_c = NULL;
		}


		if (threads_c) delete [] threads_c;
		if (et_c) delete et_c;
		if (_wb1d) delete _wb1d;

		threads_c = NULL;
		et_c = NULL;
		_wb1d = NULL;
	}
	_objInitialized = 0;
}

#include "Transpose.h"

void	WaveletBlock3D::ForwardTransform(
	const float *src_super_blk[8],
	float *dst_super_blk[8]
) {
	int	i = 0;
	int	iz = 1;
	int	iy = 2;
	int	ix = 4;

	int	z;

	// X coodinate transform
	//
	forward_transform3d_blocks(
		src_super_blk, temp_blks1_c, &dst_super_blk[ix], 8
	);
		
	// Transpose X & Y
	//
	for(i=0; i<4; i++) {
		float *s;
		float *d;

		for(z=z0_c;z<z0_c+zr_c;z++) {
			s = temp_blks1_c[i]+z*bs_c*bs_c;
			d = temp_blks2_c[i]+z*bs_c*bs_c;
			Transpose(s,d,bs_c,bs_c);
		}
	}

	// Y coodinate transform
	//
	forward_transform3d_blocks(
		(const float **) temp_blks2_c,temp_blks1_c,&dst_super_blk[iy],4
	);

	// Transpose X&Z coord axis
	//
	for(i=0;i<2;i++) {
		Transpose(
			temp_blks1_c[i], temp_blks2_c[i],
			0, bs_c*bs_c, bs_c*bs_c, z0_c, zr_c, bs_c
		);
	}

	// Z coodinate transform
	//
	forward_transform3d_blocks(
		(const float **) temp_blks2_c,&dst_super_blk[0],&dst_super_blk[iz], 2
	);

	// now transpose back to x,y,z order
	//
	Transpose(
		dst_super_blk[0], temp_blks1_c[0],
		0, bs_c, bs_c, z0_c*bs_c, bs_c*zr_c, bs_c*bs_c
	);
	float *s;
	float *d;

	for(z=z0_c;z<z0_c+zr_c;z++) {
		s = temp_blks1_c[0]+z*bs_c*bs_c;
		d = dst_super_blk[0]+z*bs_c*bs_c;
		Transpose(s,d,bs_c,bs_c);
	}
}

// thread helper function
//
static void     *run_inverse_transform_thread(void *object) {
        WaveletBlock3D  *X = (WaveletBlock3D *) object;
        X->inverse_transform_thread();
        return(0);
}

void	WaveletBlock3D::InverseTransform(
	const float *src_super_blk[8],
	float *dst_super_blk[8]
) {
	src_super_blk_c = src_super_blk;
	dst_super_blk_c = dst_super_blk;

	if (nthreads_c <=1) {
		inverse_transform_thread();
	}
	else {
		int	rc;
		rc = et_c->ParRun(run_inverse_transform_thread,(void**)threads_c);
		if (rc < 0) {
			fprintf(
				stderr, "EasyThreads::ParRun() : %s\n",
				et_c->GetErrMsg()
			);
		}
	}
}

void	WaveletBlock3D::TransposeBlks(
	float *super_blk[8]
) {
	int	iy = 2;
	int	iz = 1;
	float *s;
	float *d;
	int i, z;

	for(i=0; i<2; i++) {
		memcpy(
			temp_blks1_c[iy+i], super_blk[iy+i], 
			bs_c*bs_c*bs_c*sizeof(super_blk[0][0])
		);
	}

	// Transpose X&Z coordinates for Z gamma blocks
	//
    Transpose(
        super_blk[iz], temp_blks1_c[iz],
        0, bs_c, bs_c, z0_c*bs_c, bs_c*zr_c, bs_c*bs_c
    );

	// Now transpose X&Y coordinates for Z & Y gamma blocks
	//
    // Transpose X & Y
    //
    for(i=0; i<3; i++) {

        for(z=z0_c;z<z0_c+zr_c;z++) {
            s = temp_blks1_c[iz+i]+z*bs_c*bs_c;
            d = super_blk[iz+i]+z*bs_c*bs_c;
            Transpose(s,d,bs_c,bs_c);
        }
	}
}


void	WaveletBlock3D::inverse_transform_thread() 
{
	int	i = 0;
	int	iz = 1;
	int	iy = 2;
	int	ix = 4;

	int	z;

	float *s;
	float *d;

//fprintf(stderr, "z0_c=%d, zr_c=%d, &src=%d, &dst=%d\n", z0_c, zr_c, (int) *src_s_blk_ptr_c, *dst_s_blk_ptr_c);

	// First transpose lambda blocks so they are in z,y,x order
	//
	for(z=z0_c;z<z0_c+zr_c;z++) {
		s = (float *) *src_s_blk_ptr_c[0]+z*bs_c*bs_c;
		d = temp_blks1_c[0]+z*bs_c*bs_c;
		Transpose(s,d,bs_c,bs_c);
	}
	if (et_c) et_c->Barrier();

	Transpose(
		temp_blks1_c[0], temp_blks2_c[0],
		0, bs_c*bs_c, bs_c*bs_c, z0_c, zr_c, bs_c
	);
	if (et_c) et_c->Barrier();


	// Z coodinate transform
	//
	//inverse_transform3d_blocks(
	//	&src_super_blk_c[0], &src_super_blk_c[iz], temp_blks1_c, 2
	//);
	inverse_transform3d_blocks(
		(const float**) &temp_blks2_c[0], &(*src_s_blk_ptr_c)[iz], temp_blks1_c, 2
	);
	if (et_c) et_c->Barrier();

	// Transpose X&Z coord axis
	//
	for(i=0;i<2;i++) {
		Transpose(
			temp_blks1_c[i], temp_blks2_c[i],
			0, bs_c, bs_c, z0_c*bs_c, bs_c*zr_c, bs_c*bs_c
		);
	}
	if (et_c) et_c->Barrier();

	// Y coodinate transform
	//
	inverse_transform3d_blocks(
		(const float **) temp_blks2_c,&(*src_s_blk_ptr_c)[iy],temp_blks1_c,4
	);
	if (et_c) et_c->Barrier();

	// Transpose X & Y
	//
	for(i=0; i<4; i++) {
		float *s;
		float *d;

		for(z=z0_c;z<z0_c+zr_c;z++) {
			s = temp_blks1_c[i]+z*bs_c*bs_c;
			d = temp_blks2_c[i]+z*bs_c*bs_c;
			Transpose(s,d,bs_c,bs_c);
		}
	}
	if (et_c) et_c->Barrier();

	// X coodinate transform
	//
	inverse_transform3d_blocks(
		(const float **) temp_blks2_c,&(*src_s_blk_ptr_c)[ix],*dst_s_blk_ptr_c,8
	);
}


void	WaveletBlock3D::forward_transform3d_blocks(
	const float **src_blks,
	float **lambda_blks,
	float **gamma_blks,
	int nblocks
) {
	const float	*src_blkptr;
	float	*lambda_blkptr;
	float	*gamma_blkptr;

	int	gamma_offset = 0;
	int	lambda_offset = 0;
	int	nG = (bs_c >> 1);	// # gamma coefficients
	int	nL = bs_c - nG;	// # lambda coefficients

	int	i,x;

	for(i=0; i<nblocks/2; i++) {

		lambda_blkptr = lambda_blks[i];
		lambda_offset = 0;
		gamma_blkptr = gamma_blks[i];
		gamma_offset = 0;

		for(x=0;x<2;x++) {
			src_blkptr = src_blks[(i*2) + x];

			forward_transform3d(
				src_blkptr, lambda_blkptr, gamma_blkptr,
				lambda_offset, gamma_offset
			);

//			lambda_offset += bs_c / 2;
//			gamma_offset += bs_c / 2;

			lambda_offset += nL;
			gamma_offset += nG;
		}
	}
}


void	WaveletBlock3D::forward_transform3d(
	const float	*src_blkptr,
	float	*lambda_blkptr,
	float	*gamma_blkptr,
	int	lambda_offset,
	int	gamma_offset
) {
	const float *src_ptr = src_blkptr + z0_c * bs_c * bs_c;
	float *lambda_ptr = lambda_blkptr + lambda_offset;
	float *gamma_ptr = gamma_blkptr + gamma_offset;

	int	y,z;

	for(z=z0_c; z<z0_c+zr_c; z++) {
	for(y=0; y<bs_c; y++) {

		_wb1d->ForwardTransform(src_ptr, lambda_ptr, gamma_ptr);
		src_ptr += bs_c;
		lambda_ptr += bs_c;
		gamma_ptr += bs_c;
	}
	}
}



void	WaveletBlock3D::inverse_transform3d_blocks(
	const float **lambda_blks,
	const float **gamma_blks,
	float **dst_blks,
	int nblocks
) {
	const float	*lambda_blkptr;
	const float	*gamma_blkptr;
	float	*dst_ptr;

	int	gamma_offset = 0;
	int	lambda_offset = 0;
	int	nG = (bs_c >> 1);	// # gamma coefficients
	int	nL = bs_c - nG;	// # lambda coefficients


	int	i,x;

	for(i=0; i<nblocks/2; i++) {

		lambda_blkptr = lambda_blks[i];
		lambda_offset = 0;
		gamma_blkptr = gamma_blks[i];
		gamma_offset = 0;

		for(x=0;x<2;x++) {
			dst_ptr = dst_blks[(i*2) + x];

			inverse_transform3d(
				lambda_blkptr, gamma_blkptr, dst_ptr,
				lambda_offset, gamma_offset
			);

			lambda_offset += nL;
			gamma_offset += nG;

//			lambda_offset += bs_c / 2;
//			gamma_offset += bs_c / 2;
		}
	}
}

void	WaveletBlock3D::inverse_transform3d(
	const float	*lambda_blkptr,
	const float	*gamma_blkptr,
	float	*dst_blkptr,
	int	lambda_offset,
	int	gamma_offset
) {
	const float *lambda_ptr = lambda_blkptr + lambda_offset;
	const float *gamma_ptr = gamma_blkptr + gamma_offset;
	float *dst_ptr = dst_blkptr + z0_c * bs_c * bs_c;

	int	y,z;

	for(z=z0_c; z<z0_c+zr_c; z++) {
	for(y=0; y<bs_c; y++) {

		_wb1d->InverseTransform(lambda_ptr, gamma_ptr, dst_ptr);
		dst_ptr += bs_c;
		lambda_ptr += bs_c;
		gamma_ptr += bs_c;
	}
	}
}


#ifdef	DEAD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
main(int argc, char **argv) {

	WaveletBlock3D	*wb;
	static const int	bb = 32;
	static const int	nb = 2;
	float		src_super_block[nb*nb*nb][bb][bb][bb];
	float		dst_super_block[nb*nb*nb][bb][bb][bb];
	float		*src_ptr[8];
	float		*dst_ptr[8];
	int		i,k;
	int		x,y,z;
	int		block;
	int		xblk, yblk,zblk;
	int		xx,yy,zz;
	int		index;

	int		bs = bb*bb*bb;

	wb = new WaveletBlock3D(bb,4,4,1);

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
			zblk = block / (nb*nb);
			yblk = (block - (zblk * nb * nb)) / nb;
			xblk = block - (zblk * nb * nb) - (yblk*nb);

			z = zblk * bb + zz;
			y = yblk * bb + yy;
			x = xblk * bb + xx;

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
