#include <cstdio>
#include <cstring>
#include <cerrno>
#include "vapor/WaveletBlock3DReader.h"

using namespace VetsUtil;
using namespace VAPoR;

void	WaveletBlock3DReader::_WaveletBlock3DReader(
) {
	int	j;
	SetClassName("WaveletBlock3DReader");

	_xform_timer = 0.0;
	slab_cntr_c = 0;
	scratch_block_c = NULL;

	for(j=0; j<MAX_LEVELS; j++) {
		lambda_blks_c[j] = NULL;
	}
}

WaveletBlock3DReader::WaveletBlock3DReader(
	const Metadata *metadata,
	unsigned int	nthreads
) : WaveletBlock3DIO(metadata, nthreads) {

	_objInitialized = 0;
	if (WaveletBlock3DIO::GetErrCode()) return;

	_WaveletBlock3DReader();
	
	_objInitialized = 1;
}

WaveletBlock3DReader::WaveletBlock3DReader(
	const char *metafile,
	unsigned int	nthreads
) : WaveletBlock3DIO(metafile, nthreads) {

	_objInitialized = 0;
	if (WaveletBlock3DIO::GetErrCode()) return;
	
	_WaveletBlock3DReader();

	_objInitialized = 1;
}

WaveletBlock3DReader::~WaveletBlock3DReader(
) {
	if (! _objInitialized) return;

	WaveletBlock3DReader::CloseVariable();

	_objInitialized = 0;
}

int	WaveletBlock3DReader::OpenVariableRead(
	size_t timestep,
	const char	*varname,
	int reflevel
) {
	int	rc;

	slab_cntr_c = 0;

    if (reflevel < 0) reflevel = _num_reflevels - 1;

	rc = WaveletBlock3DIO::OpenVariableRead(timestep, varname, reflevel);
	if (rc<0) return(rc);
	rc = my_alloc();
	if (rc<0) return(rc);

	return(0);
}

int     WaveletBlock3DReader::CloseVariable(
) {
	my_free();
	return(WaveletBlock3DIO::CloseVariable());
}

int	WaveletBlock3DReader::ReadSlabs(
	float *two_slabs,
	int unblock
) {
	float		*dst_buf;
	float		*src_buf;

	size_t dst_nb[3]; // # of lambda blocks in each dimension
	size_t lambda_nb[3]; // # of lambda blocks in each dimension
	int	offset;
	int	j;
	int	rc;

	static int	first = 1;

	// if reading from the coarsest level
	if (_reflevel==0) {
		int	nb;

		GetDimBlk(lambda_nb, 0);

		if (slab_cntr_c+2 <= (int)lambda_nb[2]) nb = (int)(lambda_nb[0] * lambda_nb[1] * 2);
		else nb = (int)(lambda_nb[0] * lambda_nb[1]); // odd # of slabs


		memset(two_slabs, 0, nb*_block_size*4);
		if (! first) return(0);

		if (! unblock) {
			slab_cntr_c += 2;
			return(readLambdaBlocks(nb, two_slabs));
		}
		else {
			int x,y,z;
			float *srcptr = lambda_blks_c[0];

			rc = readLambdaBlocks(nb, srcptr);
			if (rc < 0) return(rc);

			for(z=0; z<2; z++) {
			for(y=0; y<(int)lambda_nb[1]; y++) {
			for(x=0; x<(int)lambda_nb[0]; x++) {
				const size_t bcoord[] = {x,y,z};
				const size_t min[] = {0,0,0};
				const size_t max[] = {
					lambda_nb[0]*_bs[0]-1,lambda_nb[1]*_bs[1]-1,2*_bs[2]-1
				};

				Block2NonBlock(
					srcptr, bcoord, min, max, two_slabs
				);
				srcptr += _block_size;
			}
			}
			}
			slab_cntr_c += 2;
		}
		return(0);
	}

	// Transform blocks at each level as long as there are enough 
	// blocks to process. 
	// Read a slab from the coarsest file every 2^levels times

	// Calculate how far down in the tree to go. I.e. find the
	// largest 2^(levels-j), that is a factor of the current slab counter
	//
	for(
		j=_reflevel;
		((slab_cntr_c % (1 << (_reflevel-j+1))) == 0) && j>0; 
		j--
	);


	if (j==0) { // top of tree => read lambda blocks

		GetDimBlk(lambda_nb, 0);

		// altenate between slab 0 and slab 1
		if (((slab_cntr_c / (1<<_reflevel)) % 2) == 1) {
			offset = (int)(lambda_nb[0] * lambda_nb[1] * _block_size);
		}
		else {
			offset = 0;
		}

		rc = readLambdaBlocks(
			lambda_nb[0] * lambda_nb[1], &lambda_blks_c[0][offset]
		);

		if (rc<0) return(rc);

	}

	while (j<_reflevel) {

		GetDimBlk(lambda_nb, j);
		GetDimBlk(dst_nb, j+1);

		// Calculate offset into temp storage for lambda blocks
		// at the current level. Alternate between "left" and
		// "right" side of the tree at every level.
		//
		if (((slab_cntr_c / (1<<(_reflevel-j))) % 2) == 1) {
			offset = (int)(lambda_nb[0] * lambda_nb[1] * _block_size);
		}
		else {
			offset = 0;
		}
		src_buf = &lambda_blks_c[j][offset];

		if (j==_reflevel-1 && !unblock) {
			dst_buf = two_slabs;
		} else {
			dst_buf = lambda_blks_c[j+1];
		}

		rc = read_slabs(
			j+1, src_buf, (int)lambda_nb[0], (int)lambda_nb[1], 
			dst_buf, (int)dst_nb[0], (int)dst_nb[1]
		);

		if (j==_reflevel-1 && unblock) {
			int x,y,z;
			float *srcptr = lambda_blks_c[j+1];

			for(z=0; z<2; z++) {
			for(y=0; y<(int)dst_nb[1]; y++) {
			for(x=0; x<(int)dst_nb[0]; x++) {
				const size_t bcoord[3] = {x,y,z};
				const size_t min[] = {0,0,0};
				const size_t max[] = {
					dst_nb[0]*_bs[0]-1,dst_nb[1]*_bs[1]-1,2*_bs[2]-1
				};
				Block2NonBlock(
					srcptr, bcoord, min, max, two_slabs
				);
				srcptr += _block_size;
			}
			}
			}
		}

		if (rc < 0) return(-1);

		j++;
	}
	slab_cntr_c += 2;

	return(0);
}

int	WaveletBlock3DReader::read_slabs(
	int level,
	const float *src_lambda_buf,
	int src_nbx,
	int src_nby,
	float *two_slabs,
	int	dst_nbx,
	int	dst_nby
) {
	int	i,rc;
	int	x,y;

	float	*slab1 = two_slabs;
	float	*slab2 = slab1 + (_block_size * dst_nbx * dst_nby);

	const float	*src_super_blk[8];
	float		*dst_super_blk[8];

	src_super_blk[0] = src_lambda_buf;
	for(i=1; i<8; i++) {
		src_super_blk[i] = super_block_c + (_block_size * (i-1));
	}

	//
	// process blocks in groups of 8 (8 blocks == 1 super_block_c)
	//
	for(y=0; y<src_nby; y++) {
	for(x=0; x<src_nbx; x++) {

		rc = readGammaBlocks(1, super_block_c, level);
		if (rc<0) return(rc);

		dst_super_blk[0] = slab1;
		dst_super_blk[1] = slab1 + _block_size;
		dst_super_blk[2] = slab1 + (_block_size * dst_nbx);
		dst_super_blk[3] = slab1 + (_block_size * dst_nbx) + _block_size;
		dst_super_blk[4] = slab2;
		dst_super_blk[5] = slab2 + _block_size;
		dst_super_blk[6] = slab2 + (_block_size * dst_nbx);
		dst_super_blk[7] = slab2 + (_block_size * dst_nbx) + _block_size;

		slab1 += 2*_block_size;
		slab2 += 2*_block_size;

        // Deal with boundary conditions for slabs with odd dimensions
        //
        if ((y*2)+2 > dst_nby) {
            dst_super_blk[2] = dst_super_blk[3] = dst_super_blk[6] =
            dst_super_blk[7] = scratch_block_c;
        }
        if ((x*2)+2 > dst_nbx) {
            dst_super_blk[1] = dst_super_blk[3] = dst_super_blk[5] =
            dst_super_blk[7] = scratch_block_c;
			slab1 -= _block_size;
			slab2 -= _block_size;
        }

		TIMER_START(t0)
		wb3d_c->InverseTransform(src_super_blk,dst_super_blk);
		TIMER_STOP(t0, _xform_timer)

		src_super_blk[0] += _block_size;
	}
	slab1 += (_block_size * dst_nbx);
	slab2 += (_block_size * dst_nbx);
	}
	return(0);
}

int	WaveletBlock3DReader::my_alloc(
) {

	int     size;
	int	j;

	// alloc space from coarsest (j==0) to finest level
	for(j=0; j<_num_reflevels; j++) {
		size_t nb_j[3];

		GetDimBlk(nb_j, j);

		size = (int)(nb_j[0] * nb_j[1] * _block_size * 2);

		lambda_blks_c[j] = new float[size];
		if (! lambda_blks_c[j]) {
			SetErrMsg("new float[%d] : %s", size, strerror(errno));
			return(-1);
		}

	}
	scratch_block_c = new float[_block_size];

	return(0);
}

void	WaveletBlock3DReader::my_free(
) {
	int	j;

	for(j=0; j<MAX_LEVELS; j++) {
		if (lambda_blks_c[j]) delete [] lambda_blks_c[j];

		lambda_blks_c[j] = NULL;
	}
	if (scratch_block_c) delete [] scratch_block_c;
	scratch_block_c = NULL;
}
