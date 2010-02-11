#include <cstdio>
#include <cstring>
#include <cerrno>
#include "vapor/WaveletBlock3DWriter.h"
#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace VAPoR;

void	WaveletBlock3DWriter::_WaveletBlock3DWriter()
{
	int	j;

	SetClassName("WaveletBlock3DWriter");
	
	slab_cntr_c = 0;
	is_open_c = 0;
	zero_block_c = NULL;

	const size_t *bs = GetBlockSize();
	_block_size = bs[0]*bs[1]*bs[2];

	for(j=0; j<MAX_LEVELS; j++) {
		lambda_blks_c[j] = NULL;
	}
	zero_block_c = new float[_block_size];
	memset(zero_block_c, 0, _block_size*sizeof(zero_block_c[0]));
}

WaveletBlock3DWriter::WaveletBlock3DWriter(
	const MetadataVDC &metadata
) : WaveletBlockIOBase(metadata) {

	if (WaveletBlockIOBase::GetErrCode()) return;

	SetDiagMsg("WaveletBlock3DWriter::WaveletBlock3DWriter()");

	_WaveletBlock3DWriter();
}

WaveletBlock3DWriter::WaveletBlock3DWriter(
	const string &metafile
) : WaveletBlockIOBase(metafile) {

	if (WaveletBlockIOBase::GetErrCode()) return;

	SetDiagMsg("WaveletBlock3DWriter::WaveletBlock3DWriter(%s)", metafile.c_str());

	_WaveletBlock3DWriter();

}


WaveletBlock3DWriter::~WaveletBlock3DWriter(
) {

	SetDiagMsg(
		"WaveletBlock3DWriter::~WaveletBlock3DWriter()"
	);

	WaveletBlock3DWriter::CloseVariable();

	my_free();

	if (zero_block_c) delete [] zero_block_c;
	zero_block_c = NULL;

}

int	WaveletBlock3DWriter::OpenVariableWrite(
	size_t timestep,
	const char *varname,
	int reflevel
) {
	int	rc;
	slab_cntr_c = 0;
	is_open_c = 1;

	SetDiagMsg(
		"WaveletBlock3DWriter::OpenVariableWrite(%d, %s, %d)",
		timestep, varname, reflevel
	);

	if (reflevel < 0) reflevel = GetNumTransforms();

	rc = WaveletBlockIOBase::OpenVariableWrite(timestep, varname, reflevel);
	if (rc<0) return(rc);
	return(my_realloc());
}

int     WaveletBlock3DWriter::CloseVariable(
) {

	SetDiagMsg(
		"WaveletBlock3DWriter::CloseVariable()"
	);

	if (! is_open_c) return(0);

	//
	// We've already computed the block min & max at the native refinement
	// level. Now go back and compute the block min & max at refinement 
	// levels [0..GetNumTransforms()-1]
	//
	for (int l=GetNumTransforms()-1; l>=0; l--) {
		size_t bdim[3];		// block dim at level l
		size_t bdimlp1[3];	// block dim at level l+1
		int blkidx;
		int blkidxlp1;

		VDFIOBase::GetDimBlk(bdim, l);
		VDFIOBase::GetDimBlk(bdimlp1, l+1);

		for(int z=0; z<bdim[2]; z++) {
		for(int y=0; y<bdim[1]; y++) {
		for(int x=0; x<bdim[0]; x++) {

			blkidx = z * bdim[1] * bdim[0] + y * bdim[0] + x;
			blkidxlp1 = (z<<1) * bdimlp1[1] * bdimlp1[0] + (y<<1) * bdimlp1[0] + (x<<1);
			_mins3d[l][blkidx] = _mins3d[l+1][blkidxlp1];
			_maxs3d[l][blkidx] = _maxs3d[l+1][blkidxlp1];

			for(int zz=z<<1; zz < (z<<1)+2 && zz<bdimlp1[2]; zz++) {
			for(int yy=y<<1; yy < (y<<1)+2 && yy<bdimlp1[1]; yy++) {
			for(int xx=x<<1; xx < (x<<1)+2 && xx<bdimlp1[0]; xx++) {

				blkidxlp1 = zz * bdimlp1[1] * bdimlp1[0] + yy * bdimlp1[0] + xx;

				if (_mins3d[l+1][blkidxlp1] < _mins3d[l][blkidx]) {
					_mins3d[l][blkidx] = _mins3d[l+1][blkidxlp1];
				}
				if (_maxs3d[l+1][blkidxlp1] > _maxs3d[l][blkidx]) {
					_maxs3d[l][blkidx] = _maxs3d[l+1][blkidxlp1];
				}
			}
			}
			}

		}
		}
		}
	}


	is_open_c = 0;

	return(WaveletBlockIOBase::CloseVariable());
}

int	WaveletBlock3DWriter::WriteSlabs(
//	const float *two_slabs
	float *two_slabs
) {
	SetDiagMsg(
		"WaveletBlock3DWriter::WriteSlabs()"
	);

	float *fptr = two_slabs;
	int	l = GetNumTransforms();

	int blkidx;
	

	// Initialize min & max stats
	//
	if (slab_cntr_c == 0) {
		_dataRange[0] = _dataRange[1] = *two_slabs;
	}

	const size_t *bs = GetBlockSize();
	size_t bdim[3];
	GetDimBlk(bdim, GetNumTransforms());
	const size_t *dim = GetDimension();

	// calculate the data value's range (min and max)
	//
	for(int bz = 0; bz<2 && bz+slab_cntr_c < bdim[2]; bz++) {
	for(int by = 0; by<bdim[1]; by++) {
	for(int bx = 0; bx<bdim[0]; bx++) {

		float *blkptr = two_slabs + 
			_block_size * ((bz * bdim[0] * bdim[1]) + (by * bdim[0]) + bx);

		blkidx = (bz+slab_cntr_c)*bdim[1]*bdim[0] + by*bdim[0] + bx;

		_mins3d[l][blkidx] = *blkptr;	// initialize with first block element
		_maxs3d[l][blkidx] = *blkptr;

		for(int vz=0; vz < bs[2] && (bz+slab_cntr_c)*bs[2] + vz <dim[2]; vz++){
		for(int vy=0; vy < bs[1] && by*bs[1] + vy <dim[1]; vy++) {
		for(int vx=0; vx < bs[0] && bx*bs[0] + vx <dim[0]; vx++) {

			fptr = blkptr + vz*bs[1]*bs[0] + vy*bs[0] + vx;

			if (*fptr < _dataRange[0]) _dataRange[0] = *fptr; 
			if (*fptr > _dataRange[1]) _dataRange[1] = *fptr; 

			// compute min and max for each block
			//
			if (*fptr < _mins3d[l][blkidx]) _mins3d[l][blkidx] = *fptr;
			if (*fptr > _maxs3d[l][blkidx]) _maxs3d[l][blkidx] = *fptr;

		}
		}
		}
	}
	}
	}

	//
	// Pad boundary blocks that are only partially full
	//

	// first pad along X dimension
	//
	if (dim[0] % bs[0]) {
		float pad;

		for(int bz = 0; bz<2 && bz+slab_cntr_c < bdim[2]; bz++) {
		for(int by = 0; by<bdim[1]; by++) {
			int bx = bdim[0] - 1;

			float *blkptr = two_slabs + 
				_block_size * ((bz * bdim[0] * bdim[1]) + (by * bdim[0]) + bx);

			for(int vz=0; vz < bs[2] && (bz+slab_cntr_c)*bs[2] + vz <dim[2]; vz++){
			for(int vy=0; vy < bs[1] && by*bs[1] + vy <dim[1]; vy++) {

				int vx = dim[0] % bs[0] -1;

				fptr = blkptr + vz*bs[1]*bs[0] + vy*bs[0] + vx;
				pad = *fptr;
				vx++;

				for(; vx < bs[0]; vx++) {
					fptr = blkptr + vz*bs[1]*bs[0] + vy*bs[0] + vx;
					*fptr = pad;
				}
			}
			}
		}
		}
	}

	// now pad along Y dimension
	if (dim[1] % bs[1]) {
		float pad;

		for(int bz = 0; bz<2 && bz+slab_cntr_c < bdim[2]; bz++) {
		for(int bx = 0; bx<bdim[0]; bx++) {
			int by = bdim[1] - 1;

			float *blkptr = two_slabs + 
				_block_size * ((bz * bdim[0] * bdim[1]) + (by * bdim[0]) + bx);

			for(int vz=0; vz < bs[2] && (bz+slab_cntr_c)*bs[2] + vz <dim[2]; vz++){
			for(int vx=0; vx < bs[0]; vx++) {

				int vy = dim[1] % bs[1] -1;

				fptr = blkptr + vz*bs[1]*bs[0] + vy*bs[0] + vx;
				pad = *fptr;
				vy++;

				for(; vy < bs[1]; vy++) {
					fptr = blkptr + vz*bs[1]*bs[0] + vy*bs[0] + vx;
					*fptr = pad;
				}
			}
			}
		}
		}
	}

	// now pad along Z dimension
	if (slab_cntr_c + 2 >= bdim[2] && dim[2] % bs[2]) {
		float pad;

		for(int by = 0; by<bdim[1]; by++) {
		for(int bx = 0; bx<bdim[0]; bx++) {
			int bz = 1 - (bdim[2] % 2);

			float *blkptr = two_slabs + 
				_block_size * ((bz * bdim[0] * bdim[1]) + (by * bdim[0]) + bx);

			for(int vy=0; vy < bs[1]; vy++) {
			for(int vx=0; vx < bs[0]; vx++) {

				int vz = dim[2] % bs[2] -1;

				fptr = blkptr + vz*bs[1]*bs[0] + vy*bs[0] + vx;
				pad = *fptr;
				vz++;

				for(; vz < bs[2]; vz++){
					fptr = blkptr + vz*bs[1]*bs[0] + vy*bs[0] + vx;
					*fptr = pad;
				}
			}
			}
		}
		}
	}
				

	int rc = this->write_slabs(two_slabs, GetNumTransforms());
	if (rc < 0) return(rc);

	slab_cntr_c += 2;

	return(0);
}

int	WaveletBlock3DWriter::write_slabs(
	const float *two_slabs,
	int reflevel
) {
	float		*dst_buf = NULL;

	size_t	src_nb[3];	// # of transform blocks in each dimension
	size_t	lambda_nb[3];	// # of lambda blocks in each dimension
	int	j;
	int	flush = 0;
	int	rc;


	size_t bdim[3];
	GetDimBlk(bdim, GetNumTransforms());

	// handle case where there is no transform
	//
	if ((GetNumTransforms()) == 0) {
		int	size;

		if ((slab_cntr_c+2) <= (int)bdim[2]) {
			size = (int)(2 * bdim[0] * bdim[1]);
		}
		else {
			size = (int)(bdim[0] * bdim[1]);	// odd # of slabs
		}

		return(writeLambdaBlocks(two_slabs, size));
	}

	if (slab_cntr_c+2 >= bdim[2]) flush = 1;

	// Transform blocks at each level as long as there are enough 
	// blocks to process. 
	j = reflevel;
	while (j>0 && (flush || (((slab_cntr_c+2) % (1 << (GetNumTransforms()+1-j))) == 0))) {
		int	offset;


		VDFIOBase::GetDimBlk(src_nb, j);
		VDFIOBase::GetDimBlk(lambda_nb, j-1);

		if (((slab_cntr_c / (1<<(GetNumTransforms()+1-j))) % 2) == 1) {
			offset = (int)(lambda_nb[0] * lambda_nb[1] * _block_size);
		}
		else {
			offset = 0;
		}
		dst_buf = &lambda_blks_c[j-1][offset];

		if (write_gamma_slabs(
			j, two_slabs, 
			(int)src_nb[0], (int)src_nb[1], dst_buf, (int)lambda_nb[0], (int)lambda_nb[1]) < 0) {

			return(-1);
		}
		two_slabs  = &lambda_blks_c[j-1][0];
		j--;
	}
	if (j == 0) {
		int	size = (int)(lambda_nb[0] * lambda_nb[1]);
		rc = writeLambdaBlocks(dst_buf, size);
		if (rc<0) return(-1);
	}


	return(0);
}

int	WaveletBlock3DWriter::write_gamma_slabs(
	int j,
	const float *two_slabs,
	int src_nbx,
	int src_nby,
	float *dst_lambda_buf,
	int	dst_nbx,
	int dst_nby
) {
	int	i,rc;
	int	x,y;

	const float	*slab1 = two_slabs;
	const float	*slab2 = slab1 + (_block_size * src_nbx * src_nby);

	const float	*src_super_blk[8];
	float		*dst_super_blk[8];

	dst_super_blk[0] = dst_lambda_buf;
	for(i=1; i<8; i++) {
		dst_super_blk[i] = _super_block + (_block_size * (i-1));
	}

	//
	// process blocks in groups of 8 (8 blocks == 1 super_block_c)
	//
	for(y=0; y<dst_nby; y++) {
	for(x=0; x<dst_nbx; x++) {
		int	size;

		src_super_blk[0] = slab1;
		src_super_blk[1] = slab1 + _block_size;
		src_super_blk[2] = slab1 + (_block_size * src_nbx);
		src_super_blk[3] = slab1 + (_block_size * src_nbx) + _block_size;
		src_super_blk[4] = slab2;
		src_super_blk[5] = slab2 + _block_size;
		src_super_blk[6] = slab2 + (_block_size * src_nbx);
		src_super_blk[7] = slab2 + (_block_size * src_nbx) + _block_size;

		slab1 += 2*_block_size;
		slab2 += 2*_block_size;

		// Deal with boundary conditions for slabs with odd dimensions
		//
		if ((y*2)+2 > src_nby) {
			src_super_blk[2] = src_super_blk[3] = src_super_blk[6] =
			src_super_blk[7] = zero_block_c;
		}
		if ((x*2)+2 > src_nbx) {
			src_super_blk[1] = src_super_blk[3] = src_super_blk[5] =
			src_super_blk[7] = zero_block_c;
			slab1 -= _block_size;
			slab2 -= _block_size;
		}

		
		_XFormTimerStart();
		_wb3d->ForwardTransform(src_super_blk,dst_super_blk);
		_XFormTimerStop();

		// only save gamma coefficients for requested levels
		//
		if (j <= _reflevel) {
			size =  1;
			rc = writeGammaBlocks(_super_block, size, j);
			if (rc<0) return(-1);
		}

		dst_super_blk[0] += _block_size;
	}
	slab1 += (_block_size * src_nbx);
	slab2 += (_block_size * src_nbx);
	}
	return(0);
}

int	WaveletBlock3DWriter::my_realloc(
) {

	for(int j=0; j<=GetNumTransforms(); j++) {
		if (! lambda_blks_c[j]) {

			size_t	size;
			size_t nb_j[3];

			VDFIOBase::GetDimBlk(nb_j, j);

			size = _block_size * nb_j[0] * nb_j[1] * 2;

			lambda_blks_c[j] = new float[size];
			if (! lambda_blks_c[j]) {
				SetErrMsg("new float[%d] : %s", size, strerror(errno));
				return(-1);
			}
			memset(lambda_blks_c[j], 0, size*sizeof(lambda_blks_c[j][0]));
		}
	}

	return(0);
}

void	WaveletBlock3DWriter::my_free(
) {
	int	j;

	for(j=0; j<MAX_LEVELS; j++) {
		if (lambda_blks_c[j]) delete [] lambda_blks_c[j];

		lambda_blks_c[j] = NULL;
	}
}

void WaveletBlock3DWriter::_GetDataRange(float range[2]) const {
	range[0] = _dataRange[0];
	range[1] = _dataRange[1];
}

