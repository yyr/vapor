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

	// flush any data that may not have been fully transformed and 
	// written out yet
	//
	while (slab_cntr_c % (1 << _num_reflevels-1)) {
		const float *two_slabs;
		int	j, rc;

		// Calculate how far down in the tree to go. I.e. find the
		// largest 2^(levels-j), that is a factor of the current slab counter
		//
		for(j=_num_reflevels-1;((slab_cntr_c % (1<<(_num_reflevels-j))) == 0) && j>0;j--);

		two_slabs = &lambda_blks_c[j][0];
		rc = this->write_slabs(two_slabs, j);
		if (rc < 0) return (rc);
	}

	my_free();

	is_open_c = 0;

	return(WaveletBlock3DIO::CloseVariable());
}

int	WaveletBlock3DWriter::WriteSlabs(
	const float *two_slabs
) {
	SetDiagMsg(
		"WaveletBlock3DWriter::WriteSlabs()"
	);

	const float *fptr = two_slabs;

	if (slab_cntr_c == 0) {
		_dataRange[0] = _dataRange[0] = *two_slabs;
	}

	// calculate the data value's range (min and max)
	//
	for(int z = 0; z<2*_bs[2] && z<(_dim[2]-slab_cntr_c*_bs[2]); z++) {
	for(int y = 0; y<_dim[1]; y++) {
		fptr = two_slabs + 
			(z * _bdim[0]*_bs[0] * _bdim[1]*_bs[1]) + (y*_bdim[0]*_bs[0]);

		for(int x = 0; x<_dim[0]; x++) {
			if (*fptr < _dataRange[0]) _dataRange[0] = *fptr; 
			if (*fptr > _dataRange[1]) _dataRange[1] = *fptr; 
			fptr++;
		}
	}
	}

	if (_version < 2) {
		vector <double> r;
		r.push_back(_dataRange[0]);
		r.push_back(_dataRange[1]);
		_metadata->SetVDataRange(_timeStep, _varName.c_str(), r);
	}

	return(this->write_slabs(two_slabs, _num_reflevels-1));
}

int	WaveletBlock3DWriter::write_slabs(
	const float *two_slabs,
	int reflevel
) {
	float		*dst_buf;

	size_t	src_nb[3];	// # of transform blocks in each dimension
	size_t	lambda_nb[3];	// # of lambda blocks in each dimension
	int	j;
	int	rc;

	slab_cntr_c += 2;


	// handle case where there is no transform
	//
	if (_reflevel == 0) {
		int	size;

		if (slab_cntr_c <= (int)_bdim[2]) size = (int)(2 * _bdim[0] * _bdim[1]);
		else size = (int)(_bdim[0] * _bdim[1]);	// odd # of slabs

		return(writeLambdaBlocks(two_slabs, size));
	}


	// Transform blocks at each level as long as there are enough 
	// blocks to process. 
	j = reflevel;
	while (j>0 && ((slab_cntr_c % (1 << (_num_reflevels-j))) == 0)) {
		int	offset;

		GetDimBlk(src_nb, j);
		GetDimBlk(lambda_nb, j-1);

		if ((((slab_cntr_c-2) / (1<<(_num_reflevels-j))) % 2) == 1) {
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
