#include <cstdio>
#include <cstring>
#include <cerrno>
#include "vapor/WaveletBlock3DWriter.h"

using namespace VAPoR;

void	WaveletBlock3DWriter::_WaveletBlock3DWriter()
{
	int	j;

	SetClassName("WaveletBlock3DWriter");
	
	_xform_timer = 0.0;
	slab_cntr_c = 0;
	is_open_c = 0;
	zero_block_c = NULL;

	for(j=0; j<MAX_LEVELS; j++) {
		lambda_blks_c[j] = NULL;
	}
}

WaveletBlock3DWriter::WaveletBlock3DWriter(
	Metadata *metadata,
	unsigned int    nthreads
) : WaveletBlock3DIO(metadata, nthreads) {

	_objInitialized = 0;
	if (WaveletBlock3DIO::GetErrCode()) return;

	SetDiagMsg(
		"WaveletBlock3DWriter::WaveletBlock3DWriter(,%d)",
		nthreads
	);

	_metafile.clear();
	_WaveletBlock3DWriter();

	_objInitialized = 1;
}

WaveletBlock3DWriter::WaveletBlock3DWriter(
	const char *metafile,
	unsigned int    nthreads
) : WaveletBlock3DIO(metafile, nthreads) {

	_objInitialized = 0;
	if (WaveletBlock3DIO::GetErrCode()) return;

	SetDiagMsg(
		"WaveletBlock3DWriter::WaveletBlock3DWriter(%s,%d)",
		metafile, nthreads
	);

	_metafile.assign(metafile);
	_WaveletBlock3DWriter();

	_objInitialized = 1;
}


WaveletBlock3DWriter::~WaveletBlock3DWriter(
) {
	if (! _objInitialized) return;

	SetDiagMsg(
		"WaveletBlock3DWriter::~WaveletBlock3DWriter()"
	);

	if (_version < 2 && _metafile.length()) {
		_metadata->Write(_metafile.c_str());
	}
	WaveletBlock3DWriter::CloseVariable();

	_objInitialized = 0;
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

	if (reflevel < 0) reflevel = _num_reflevels - 1;

	rc = WaveletBlock3DIO::OpenVariableWrite(timestep, varname, reflevel);
	if (rc<0) return(rc);
	return(my_alloc());
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
	// levels [0.._num_reflevels-2]
	//
	for (int l=_num_reflevels-2; l>=0; l--) {
		size_t bdim[3];		// block dim at level l
		size_t bdimlp1[3];	// block dim at level l+1
		int blkidx;
		int blkidxlp1;

		GetDimBlk(bdim, l);
		GetDimBlk(bdimlp1, l+1);

		for(int z=0; z<bdim[2]; z++) {
		for(int y=0; y<bdim[1]; y++) {
		for(int x=0; x<bdim[0]; x++) {

			blkidx = z * bdim[1] * bdim[0] + y * bdim[0] + x;
			blkidxlp1 = (z<<1) * bdimlp1[1] * bdimlp1[0] + (y<<1) * bdimlp1[0] + (x<<1);
			_mins[l][blkidx] = _mins[l+1][blkidxlp1];
			_maxs[l][blkidx] = _maxs[l+1][blkidxlp1];


			for(int zz=z<<1; zz < (z<<1)+2; zz++) {
			for(int yy=y<<1; yy < (y<<1)+2; yy++) {
			for(int xx=x<<1; xx < (x<<1)+2; xx++) {
				blkidxlp1 = zz * bdimlp1[1] * bdimlp1[0] + yy * bdimlp1[0] + xx;

				if (_mins[l+1][blkidxlp1] < _mins[l][blkidx]) {
					_mins[l][blkidx] = _mins[l+1][blkidxlp1];
				}
				if (_maxs[l+1][blkidxlp1] > _maxs[l][blkidx]) {
					_maxs[l][blkidx] = _maxs[l+1][blkidxlp1];
				}
			}
			}
			}

		}
		}
		}
	}

	my_free();

	is_open_c = 0;

	return(WaveletBlock3DIO::CloseVariable());
}

int	WaveletBlock3DWriter::WriteSlabs(
//	const float *two_slabs
	float *two_slabs
) {
	SetDiagMsg(
		"WaveletBlock3DWriter::WriteSlabs()"
	);

	float *fptr = two_slabs;
	int	l = _num_reflevels - 1;

	int blkidx;
	

	// Initialize min & max stats
	//
	if (slab_cntr_c == 0) {
		_dataRange[0] = _dataRange[0] = *two_slabs;
	}

	// calculate the data value's range (min and max)
	//
	for(int bz = 0; bz<2 && bz+slab_cntr_c < _bdim[2]; bz++) {
	for(int by = 0; by<_bdim[1]; by++) {
	for(int bx = 0; bx<_bdim[0]; bx++) {

		float *blkptr = two_slabs + 
			_block_size * ((bz * _bdim[0] * _bdim[1]) + (by * _bdim[0]) + bx);

		blkidx = (bz+slab_cntr_c)*_bdim[1]*_bdim[0] + by*_bdim[0] + bx;

		_mins[l][blkidx] = *blkptr;	// initialize with first block element
		_maxs[l][blkidx] = *blkptr;

		for(int vz=0; vz < _bs[2] && (bz+slab_cntr_c)*_bs[2] + vz <_dim[2]; vz++){
		for(int vy=0; vy < _bs[1] && by*_bs[1] + vy <_dim[1]; vy++) {
		for(int vx=0; vx < _bs[0] && bx*_bs[0] + vx <_dim[0]; vx++) {

			fptr = blkptr + vz*_bs[1]*_bs[0] + vy*_bs[0] + vx;

			if (*fptr < _dataRange[0]) _dataRange[0] = *fptr; 
			if (*fptr > _dataRange[1]) _dataRange[1] = *fptr; 

			// compute min and max for each block
			//
			if (*fptr < _mins[l][blkidx]) _mins[l][blkidx] = *fptr;
			if (*fptr > _maxs[l][blkidx]) _maxs[l][blkidx] = *fptr;

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
	if (_dim[0] % _bs[0]) {
		float pad;

		for(int bz = 0; bz<2 && bz+slab_cntr_c < _bdim[2]; bz++) {
		for(int by = 0; by<_bdim[1]; by++) {
			int bx = _bdim[0] - 1;

			float *blkptr = two_slabs + 
				_block_size * ((bz * _bdim[0] * _bdim[1]) + (by * _bdim[0]) + bx);

			for(int vz=0; vz < _bs[2] && (bz+slab_cntr_c)*_bs[2] + vz <_dim[2]; vz++){
			for(int vy=0; vy < _bs[1] && by*_bs[1] + vy <_dim[1]; vy++) {

				int vx = _dim[0] % _bs[0] -1;

				fptr = blkptr + vz*_bs[1]*_bs[0] + vy*_bs[0] + vx;
				pad = *fptr;
				vx++;

				for(; vx < _bs[0]; vx++) {
					fptr = blkptr + vz*_bs[1]*_bs[0] + vy*_bs[0] + vx;
					*fptr = pad;
				}
			}
			}
		}
		}
	}

	// now pad along Y dimension
	if (_dim[1] % _bs[1]) {
		float pad;

		for(int bz = 0; bz<2 && bz+slab_cntr_c < _bdim[2]; bz++) {
		for(int bx = 0; bx<_bdim[0]; bx++) {
			int by = _bdim[1] - 1;

			float *blkptr = two_slabs + 
				_block_size * ((bz * _bdim[0] * _bdim[1]) + (by * _bdim[0]) + bx);

			for(int vz=0; vz < _bs[2] && (bz+slab_cntr_c)*_bs[2] + vz <_dim[2]; vz++){
			for(int vx=0; vx < _bs[0]; vx++) {

				int vy = _dim[1] % _bs[1] -1;

				fptr = blkptr + vz*_bs[1]*_bs[0] + vy*_bs[0] + vx;
				pad = *fptr;
				vy++;

				for(; vy < _bs[1]; vy++) {
					fptr = blkptr + vz*_bs[1]*_bs[0] + vy*_bs[0] + vx;
					*fptr = pad;
				}
			}
			}
		}
		}
	}

	// now pad along Z dimension
	if (slab_cntr_c + 2 >= _bdim[2] && _dim[2] % _bs[2]) {
		float pad;

		for(int by = 0; by<_bdim[1]; by++) {
		for(int bx = 0; bx<_bdim[0]; bx++) {
			int bz = 1 - (_bdim[2] % 2);

			float *blkptr = two_slabs + 
				_block_size * ((bz * _bdim[0] * _bdim[1]) + (by * _bdim[0]) + bx);

			for(int vy=0; vy < _bs[1]; vy++) {
			for(int vx=0; vx < _bs[0]; vx++) {

				int vz = _dim[2] % _bs[2] -1;

				fptr = blkptr + vz*_bs[1]*_bs[0] + vy*_bs[0] + vx;
				pad = *fptr;
				vz++;

				for(; vz < _bs[2]; vz++){
					fptr = blkptr + vz*_bs[1]*_bs[0] + vy*_bs[0] + vx;
					*fptr = pad;
				}
			}
			}
		}
		}
	}
				

		

	if (_version < 2) {
		vector <double> r;
		r.push_back(_dataRange[0]);
		r.push_back(_dataRange[1]);
		_metadata->SetVDataRange(_timeStep, _varName.c_str(), r);
	}

	int rc = this->write_slabs(two_slabs, _num_reflevels-1);
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



	// handle case where there is no transform
	//
	if ((_num_reflevels-1) == 0) {
		int	size;

		if ((slab_cntr_c+2) <= (int)_bdim[2]) {
			size = (int)(2 * _bdim[0] * _bdim[1]);
		}
		else {
			size = (int)(_bdim[0] * _bdim[1]);	// odd # of slabs
		}

		return(writeLambdaBlocks(two_slabs, size));
	}

	if (slab_cntr_c+2 >= _bdim[2]) flush = 1;

	// Transform blocks at each level as long as there are enough 
	// blocks to process. 
	j = reflevel;
	while (j>0 && (flush || (((slab_cntr_c+2) % (1 << (_num_reflevels-j))) == 0))) {
		int	offset;


		GetDimBlk(src_nb, j);
		GetDimBlk(lambda_nb, j-1);

		if (((slab_cntr_c / (1<<(_num_reflevels-j))) % 2) == 1) {
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
		dst_super_blk[i] = super_block_c + (_block_size * (i-1));
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

		TIMER_START(t0)
		wb3d_c->ForwardTransform(src_super_blk,dst_super_blk);
		TIMER_STOP(t0, _xform_timer)

		// only save gamma coefficients for requested levels
		//
		if (j <= _reflevel) {
			size =  1;
			rc = writeGammaBlocks(super_block_c, size, j);
			if (rc<0) return(-1);
		}

		dst_super_blk[0] += _block_size;
	}
	slab1 += (_block_size * src_nbx);
	slab2 += (_block_size * src_nbx);
	}
	return(0);
}

int	WaveletBlock3DWriter::my_alloc(
) {
	int	j;
	int	size;
	size_t nb_j[3];

	for(j=0; j<_num_reflevels; j++) {

		GetDimBlk(nb_j, j);

		size = (int)(_block_size * nb_j[0] * nb_j[1] * 2);

		lambda_blks_c[j] = new float[size];
		if (! lambda_blks_c[j]) {
			SetErrMsg("new float[%d] : %s", size, strerror(errno));
			return(-1);
		}
	}
	zero_block_c = new float[_block_size];
	memset(zero_block_c, 0, _block_size*sizeof(zero_block_c[0]));
	return(0);
}

void	WaveletBlock3DWriter::my_free(
) {
	int	j;

	for(j=0; j<MAX_LEVELS; j++) {
		if (lambda_blks_c[j]) delete [] lambda_blks_c[j];

		lambda_blks_c[j] = NULL;
	}
	if (zero_block_c) delete [] zero_block_c;
	zero_block_c = NULL;
}
