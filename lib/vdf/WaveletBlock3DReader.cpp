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

	xform_timer_c = 0.0;
	slab_cntr_c = 0;
	scratch_block_c = NULL;

	for(j=0; j<MAX_LEVELS; j++) {
		lambda_blks_c[j] = NULL;
	}
}

WaveletBlock3DReader::WaveletBlock3DReader(
	const Metadata *metadata,
	unsigned int	nthreads
) : WaveletBlock3DIO((Metadata *) metadata, nthreads) {

	_objInitialized = 0;

	_WaveletBlock3DReader();
	
	_objInitialized = 1;
}

WaveletBlock3DReader::WaveletBlock3DReader(
	const char *metafile,
	unsigned int	nthreads
) : WaveletBlock3DIO(metafile, nthreads) {

	_objInitialized = 0;
	
	_WaveletBlock3DReader();

	_objInitialized = 1;
}

WaveletBlock3DReader::~WaveletBlock3DReader(
) {
	if (! _objInitialized) return;

	CloseVariable();

	_objInitialized = 0;
}

int	WaveletBlock3DReader::OpenVariableRead(
	size_t timestep,
	const char	*varname,
	size_t num_xforms
) {
	int	rc;

	slab_cntr_c = 0;

	rc = WaveletBlock3DIO::OpenVariableRead(timestep, varname, num_xforms);
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
	int levels = max_xforms_c - num_xforms_c;

	size_t dst_nb[3]; // # of lambda blocks in each dimension
	size_t lambda_nb[3]; // # of lambda blocks in each dimension
	int	offset;
	int	j;
	int	rc;

	static int	first = 1;

	// if reading from the coarsest level
	if (levels==0) {
		int	nb;

		TIMER_START(t0)

		GetDimBlk(max_xforms_c, lambda_nb);

		if (slab_cntr_c+2 <= (int)lambda_nb[2]) nb = (int)(lambda_nb[0] * lambda_nb[1] * 2);
		else nb = (int)(lambda_nb[0] * lambda_nb[1]); // odd # of slabs


		memset(two_slabs, 0, nb*block_size_c*4);
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
					lambda_nb[0]*bs_c-1,lambda_nb[1]*bs_c-1,2*bs_c-1
				};

				Block2NonBlock(
					srcptr, bcoord, min, max, two_slabs
				);
				srcptr += block_size_c;
			}
			}
			}
			slab_cntr_c += 2;
		}
		TIMER_STOP(t0, xform_timer_c)
		return(0);
	}

	TIMER_START(t0)

	// Transform blocks at each level as long as there are enough 
	// blocks to process. 
	// Read a slab from the coarsest file every 2^levels times

	// Calculate how far down in the tree to go. I.e. find the
	// largest 2^(levels-j), that is a factor of the current slab counter
	//
	for(
		j=levels;
		((slab_cntr_c % (1 << (levels-j+1))) == 0) && j>0; 
		j--
	);


	if (j==0) { // top of tree => read lambda blocks

		GetDimBlk(max_xforms_c, lambda_nb);

		// altenate between slab 0 and slab 1
		if (((slab_cntr_c / (1<<levels)) % 2) == 1) {
			offset = (int)(lambda_nb[0] * lambda_nb[1] * block_size_c);
		}
		else {
			offset = 0;
		}

		rc = readLambdaBlocks(
			lambda_nb[0] * lambda_nb[1], &lambda_blks_c[0][offset]
		);

		if (rc<0) return(rc);

	}

	while (j<levels) {

		GetDimBlk(max_xforms_c-j, lambda_nb);
		GetDimBlk(max_xforms_c-j-1, dst_nb);

		// Calculate offset into temp storage for lambda blocks
		// at the current level. Alternate between "left" and
		// "right" side of the tree at every level.
		//
		if (((slab_cntr_c / (1<<(levels-j))) % 2) == 1) {
			offset = (int)(lambda_nb[0] * lambda_nb[1] * block_size_c);
		}
		else {
			offset = 0;
		}
		src_buf = &lambda_blks_c[j][offset];

		if (j==levels-1 && !unblock) {
			dst_buf = two_slabs;
		} else {
			dst_buf = lambda_blks_c[j+1];
		}

		rc = read_slabs(
			max_xforms_c-j-1, src_buf, (int)lambda_nb[0], (int)lambda_nb[1], 
			dst_buf, (int)dst_nb[0], (int)dst_nb[1]
		);

		if (j==levels-1 && unblock) {
			int x,y,z;
			float *srcptr = lambda_blks_c[j+1];

			for(z=0; z<2; z++) {
			for(y=0; y<(int)dst_nb[1]; y++) {
			for(x=0; x<(int)dst_nb[0]; x++) {
				const size_t bcoord[3] = {x,y,z};
				const size_t min[] = {0,0,0};
				const size_t max[] = {
					dst_nb[0]*bs_c-1,dst_nb[1]*bs_c-1,2*bs_c-1
				};
				Block2NonBlock(
					srcptr, bcoord, min, max, two_slabs
				);
				srcptr += block_size_c;
			}
			}
			}
		}

		if (rc < 0) return(-1);

		j++;
	}
	slab_cntr_c += 2;

	TIMER_STOP(t0, xform_timer_c)
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
	float	*slab2 = slab1 + (block_size_c * dst_nbx * dst_nby);

	const float	*src_super_blk[8];
	float		*dst_super_blk[8];

	src_super_blk[0] = src_lambda_buf;
	for(i=1; i<8; i++) {
		src_super_blk[i] = super_block_c + (block_size_c * (i-1));
	}

	//
	// process blocks in groups of 8 (8 blocks == 1 super_block_c)
	//
	for(y=0; y<src_nby; y++) {
	for(x=0; x<src_nbx; x++) {

		rc = readGammaBlocks(1, level, super_block_c);
		if (rc<0) return(rc);

		dst_super_blk[0] = slab1;
		dst_super_blk[1] = slab1 + block_size_c;
		dst_super_blk[2] = slab1 + (block_size_c * dst_nbx);
		dst_super_blk[3] = slab1 + (block_size_c * dst_nbx) + block_size_c;
		dst_super_blk[4] = slab2;
		dst_super_blk[5] = slab2 + block_size_c;
		dst_super_blk[6] = slab2 + (block_size_c * dst_nbx);
		dst_super_blk[7] = slab2 + (block_size_c * dst_nbx) + block_size_c;

		slab1 += 2*block_size_c;
		slab2 += 2*block_size_c;

        // Deal with boundary conditions for slabs with odd dimensions
        //
        if ((y*2)+2 > dst_nby) {
            dst_super_blk[2] = dst_super_blk[3] = dst_super_blk[6] =
            dst_super_blk[7] = scratch_block_c;
        }
        if ((x*2)+2 > dst_nbx) {
            dst_super_blk[1] = dst_super_blk[3] = dst_super_blk[5] =
            dst_super_blk[7] = scratch_block_c;
			slab1 -= block_size_c;
			slab2 -= block_size_c;
        }

		wb3d_c->InverseTransform(src_super_blk,dst_super_blk);

		src_super_blk[0] += block_size_c;
	}
	slab1 += (block_size_c * dst_nbx);
	slab2 += (block_size_c * dst_nbx);
	}
	return(0);
}

int	WaveletBlock3DReader::my_alloc(
) {

	int     size;
	int	j;

	// alloc space from coarsest (j==0) to finest level
	for(j=0; j<=max_xforms_c; j++) {
		size_t nb_j[3];

		GetDimBlk(max_xforms_c-j, nb_j);

		size = (int)(nb_j[0] * nb_j[1] * block_size_c * 2);

		lambda_blks_c[j] = new float[size];
		if (! lambda_blks_c[j]) {
			SetErrMsg("new float[%d] : %s", size, strerror(errno));
			return(-1);
		}

	}
	scratch_block_c = new float[block_size_c];

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
