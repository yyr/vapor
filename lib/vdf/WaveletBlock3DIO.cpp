#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <cassert>
#ifndef WIN32
#include <unistd.h>
#else
#include "windows.h"
#include "vaporinternal/common.h"
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include "vapor/WaveletBlock3DIO.h"
#include "vapor/MyBase.h"

using namespace VetsUtil;
using namespace VAPoR;


#define NC_ERR_WRITE(rc, path) \
	if (rc != NC_NOERR) { \
		SetErrMsg( \
			"Error writing netCDF file \"%s\" : %s",  \
			path.c_str(), nc_strerror(rc) \
		); \
		return(-1); \
	}

#define NC_ERR_READ(rc, path) \
	if (rc != NC_NOERR) { \
		SetErrMsg( \
			"Error reading netCDF file \"%s\" : %s",  \
			path.c_str(), nc_strerror(rc) \
		); \
		return(-1); \
	}


int	WaveletBlock3DIO::_WaveletBlock3DIO(
) {
	SetClassName("WaveletBlock3DIO");

	wb3d_c = NULL;
	super_block_c = NULL;

	if (this->my_alloc() < 0) {
		this->my_free();
		return (-1);
	}
	return(0);
}

WaveletBlock3DIO::WaveletBlock3DIO(
	const Metadata	*metadata,
	unsigned int	nthreads
) : WaveletBlockIOBase(metadata, nthreads) {

	_objInitialized = 0;
	if (WaveletBlockIOBase::GetErrCode()) return;

	if (_WaveletBlock3DIO() < 0) return;

	_objInitialized = 1;
}

WaveletBlock3DIO::WaveletBlock3DIO(
	const char *metafile,
	unsigned int	nthreads
) : WaveletBlockIOBase(metafile, nthreads) {
	_objInitialized = 0;

	if (WaveletBlockIOBase::GetErrCode()) return;

	if (_WaveletBlock3DIO() < 0) return;

	_objInitialized = 1;
}

WaveletBlock3DIO::~WaveletBlock3DIO() {

	if (! _objInitialized) return;

	my_free();

	_objInitialized = 0;
}


int	WaveletBlock3DIO::OpenVariableWrite(
	size_t timestep,
	const char *varname,
	int reflevel
) {

	int min;

	min = Min((int)_bdim[0],(int) _bdim[1]);
	min = Min((int)min, (int)_bdim[2]);
	if (LogBaseN(min, 2.0) < 0) {
		SetErrMsg("Too many levels (%d) in transform ", reflevel);
		return(-1);
	}
	return(WaveletBlockIOBase::OpenVariableWrite(timestep,varname,reflevel));
}


int WaveletBlock3DIO::ncDefineDimsVars(
	int j, const string &path, const int bs_dim_ids[3], const int dim_ids[3]
) {

	//
	// Define netCDF dimensions
	//
		
	size_t bdim[3];

	int nb;
	if (j==0) {
		VDFIOBase::GetDimBlk(bdim,j);
		nb = bdim[0]*bdim[1]*bdim[2];
	}
	else {
		VDFIOBase::GetDimBlk(bdim,j-1);
		nb = bdim[0]*bdim[1]*bdim[2] * 7;
	}

	int nblk_dim_id;
	int rc = nc_def_dim(_ncids[j], _nBlocksDimName.c_str(), nb, &nblk_dim_id);
	NC_ERR_WRITE(rc,path)

	//
	// Define netCDF variables
	//
	{
		int ids[] = {dim_ids[2], dim_ids[1], dim_ids[0]};
		rc = nc_def_var(
			_ncids[j], _minsName.c_str(), NC_FLOAT, 
			sizeof(ids)/sizeof(ids[0]), ids, &_ncminvars[j]
		);
		NC_ERR_WRITE(rc,path)

		rc = nc_def_var(
			_ncids[j], _maxsName.c_str(), NC_FLOAT, 
			sizeof(ids)/sizeof(ids[0]), ids, &_ncmaxvars[j]
		);
		NC_ERR_WRITE(rc,path)
	}

	{
		int ids[] = {nblk_dim_id, bs_dim_ids[2], bs_dim_ids[1], bs_dim_ids[0]};

		if (j==0) {
			rc = nc_def_var(
				_ncids[j], _lambdaName.c_str(), NC_FLOAT, 
				sizeof(ids)/sizeof(ids[0]), ids, &_ncvars[j]
			);
		}
		else {
			rc = nc_def_var(
				_ncids[j], _gammaName.c_str(), NC_FLOAT, 
				sizeof(ids)/sizeof(ids[0]), ids, &_ncvars[j]
			);
		}

		NC_ERR_WRITE(rc,path)
	}

	return(0);
}


int	WaveletBlock3DIO::OpenVariableRead(
	size_t timestep,
	const char *varname,
	int reflevel
) {
	SetDiagMsg(
		"WaveletBlock3DIO::OpenVariableRead(%d, %s, %d)",
		timestep, varname, reflevel
	);
	return(WaveletBlockIOBase::OpenVariableRead(timestep,varname,reflevel));
}

int WaveletBlock3DIO::ncVerifyDimsVars(
	int j, const string &path
) {

	//
	// Verify coefficient file metadata matches VDF metadata
	//
	size_t bdim[3];
	int nb;

	if (j==0) {
		VDFIOBase::GetDimBlk(bdim,j);
		nb = bdim[0]*bdim[1]*bdim[2];
	}
	else {
		VDFIOBase::GetDimBlk(bdim,j-1);
		nb = bdim[0]*bdim[1]*bdim[2] * 7;
	}

	int ncdimid;
	int rc = nc_inq_dimid(_ncids[j], _nBlocksDimName.c_str(), &ncdimid);
	if (rc==NC_EBADDIM) {
		rc = nc_inq_dimid(_ncids[j], "NumBlocks", &ncdimid);
	}
	NC_ERR_READ(rc,path)

	size_t ncdim;
	rc = nc_inq_dimlen(_ncids[j], ncdimid, &ncdim);
	NC_ERR_READ(rc,path)
	if (ncdim != nb) {
		SetErrMsg("Metadata file and data file mismatch");
		return(-1);
	}

	return(0);
}

int	WaveletBlock3DIO::CloseVariable()
{
	return(WaveletBlockIOBase::CloseVariable());
}


int	WaveletBlock3DIO::seekLambdaBlocks(
	const size_t bcoord[3]
) {

	unsigned int	offset;
	size_t bdim[3];

	VDFIOBase::GetDimBlk(bdim, 0);

	for(int i=0; i<3; i++) {
		if (bcoord[i] >= bdim[i]) {
			SetErrMsg(
				"Invalid block address : %dx%dx%d", 
				bcoord[0], bcoord[1], bcoord[2]
			);
			return(-1);
		}
	}

	offset = (int)(bcoord[2] * bdim[0] * bdim[1] + (bcoord[1]*bdim[0]) + (bcoord[0]));

	return(seekBlocks(offset, 0));
}

int	WaveletBlock3DIO::seekGammaBlocks(
	const size_t bcoord[3],
	int reflevel
) {
	size_t bdim[3];

	unsigned int	offset;


	if (reflevel < 0 || reflevel > _reflevel) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}

	VDFIOBase::GetDimBlk(bdim, reflevel-1);

	for(int i=0; i<3; i++) {
		if (bcoord[i] >= bdim[i]) {
			SetErrMsg(
				"Invalid block address : %dx%dx%d", 
				bcoord[0], bcoord[1], bcoord[2]
			);
			return(-1);
		}
	}

	offset = (unsigned int)((bcoord[2] * bdim[0] * bdim[1]) + (bcoord[1]*bdim[0]) + (bcoord[0])) *  7;

	return(seekBlocks(offset, reflevel));
}

int	WaveletBlock3DIO::readBlocks(
	size_t n,
	float *blks,
	int reflevel
) {
	int	rc;


	if (! is_open_c) {
		SetErrMsg("File not open");
		return(-1);
	}

	if (write_mode_c) {
		SetErrMsg("WaveletBlock3DIO : object created for writing");
		return(-1);
	}

	if (reflevel > _reflevel) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}

	TIMER_START(t0)

	size_t start[] = {_ncoffsets[reflevel],0,0,0};
	size_t count[] = {n,_bs[2],_bs[1],_bs[0]};
	rc = nc_get_vars_float(
			_ncids[reflevel], _ncvars[reflevel],
			start, count, NULL, blks
	);
	if (rc != NC_NOERR) { 
		SetErrMsg( "Error reading netCDF file : %s",  nc_strerror(rc) ); 
		return(-1); 
	}
	TIMER_STOP(t0, _read_timer)

	_ncoffsets[reflevel] += n;

	return(0);
}

int	WaveletBlock3DIO::readLambdaBlocks(
	size_t n,
	float *blks
) {
	return(readBlocks(n, blks, 0));
}

int	WaveletBlock3DIO::readGammaBlocks(
	size_t n, 
	float *blks,
	int reflevel
) {

	if (reflevel < 0 || reflevel > _reflevel) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}

	return(readBlocks(n*7, blks, reflevel));
}


int	WaveletBlock3DIO::writeBlocks(
	const float *blks, 
	size_t n,
	int reflevel
) {
	int	rc;

	if (! is_open_c) {
		SetErrMsg("File not open");
		return(-1);
	}

	if (! write_mode_c) {
		SetErrMsg("WaveletBlock3DIO : object created for reading");
		return(-1);
	}

	if (reflevel > _reflevel) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}

	TIMER_START(t0)

	size_t start[] = {_ncoffsets[reflevel],0,0,0};
	size_t count[] = {n,_bs[2],_bs[1],_bs[0]};

	rc = nc_put_vars_float(
			_ncids[reflevel], _ncvars[reflevel], start, count, NULL, blks
	);
	if (rc != NC_NOERR) { 
		SetErrMsg( "Error writing netCDF file : %s",  nc_strerror(rc) ); 
		return(-1); 
	}

	TIMER_STOP(t0, _write_timer)

	_ncoffsets[reflevel] += n;

	return(0);
}

int	WaveletBlock3DIO::writeLambdaBlocks(
	const float *blks, 
	size_t n
) {
	return(writeBlocks(blks, n, 0));
}

int	WaveletBlock3DIO::writeGammaBlocks(
	const float *blks, 
	size_t n, 
	int reflevel
) {

	if (reflevel < 0 || reflevel > _reflevel) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}
	return(writeBlocks(blks, n*7, reflevel));
}

void	WaveletBlock3DIO::Block2NonBlock(
	const float *blk, 
	const size_t bcoord[3],
	const size_t min[3],
	const size_t max[3],
	float	*voxels
) const {

	SetDiagMsg(
		"WaveletBlock3DIO::Block2NonBlock(,[%d,%d,%d],[%d,%d,%d],[%d,%d,%d])",
		bcoord[0], bcoord[1], bcoord[2],
		min[0], min[1], min[2], max[0], max[1], max[2]
	);

	size_t y,z;

	assert((min[0] < _bs[0]) && (max[0] > min[0]) && (max[0]/_bs[0] >= (bcoord[0])));
	assert((min[1] < _bs[1]) && (max[1] > min[1]) && (max[1]/_bs[1] >= (bcoord[1])));
	assert((min[2] < _bs[2]) && (max[2] > min[2]) && (max[2]/_bs[2] >= (bcoord[2])));

	size_t dim[] = {max[0]-min[0]+1, max[1]-min[1]+1, max[2]-min[2]+1};
	size_t xsize = _bs[0];
	size_t ysize = _bs[1];
	size_t zsize = _bs[2];

	//
	// Address of first destination voxel
	//
	voxels += (bcoord[2]*_bs[2] * dim[1] * dim[0]) + 
		(bcoord[1]*_bs[1]*dim[0]) + 
		(bcoord[0]*_bs[0]);

	if (bcoord[0] == 0) {
		blk += min[0];
		xsize = _bs[0] - min[0];
	}
	else {
		voxels -= min[0];
	}
	if (bcoord[0] == max[0]/_bs[0]) xsize -= _bs[0] - (max[0] % _bs[0] + 1);

	if (bcoord[1] == 0) {
		blk += min[1] * _bs[0];
		ysize = _bs[1] - min[1];
	}
	else {
		voxels -= dim[0] * min[1];
	}
	if (bcoord[1] == max[1]/_bs[1]) ysize -= _bs[1] - (max[1] % _bs[1] + 1);

	if (bcoord[2] == 0) {
		blk += min[2] * _bs[1] * _bs[0];
		zsize = _bs[2] - min[2];
	}
	else {
		voxels -= dim[1] * dim[0] * min[2];
	}
	if (bcoord[2] == max[2]/_bs[2]) zsize -= _bs[2] - (max[2] % _bs[2] + 1);

	float *voxptr;
	const float *blkptr;

	for(z=0; z<zsize; z++) {
		voxptr = voxels + (dim[0]*dim[1]*z); 
		blkptr = blk + (_bs[0]*_bs[1]*z);

		for(y=0; y<ysize; y++) {

			memcpy(voxptr,blkptr,xsize*sizeof(blkptr[0]));
			blkptr += _bs[0];
			voxptr += dim[0];
		}
	}
}



int	WaveletBlock3DIO::my_alloc(
) {

	int     size;
	int	j;


	// alloc space from coarsest (j==0) to finest level
	for(j=0; j<_num_reflevels; j++) {
		size_t nb_j[3];

		VDFIOBase::GetDimBlk(nb_j, j);

		size = (int)(nb_j[0] * nb_j[1] * nb_j[2]);

		_mins[j] = new float[size];
		_maxs[j] = new float[size];
		if (! _mins[j] || ! _maxs[j]) {
			SetErrMsg("new float[%d] : %s", size, strerror(errno));
			return(-1);
		}
		for (int i=0; i<size; i++) _mins[j][i]  = _maxs[j][i] = 0.0;
	}

	size = _block_size * 8;
	super_block_c = new float[size];
	if (! super_block_c) {
		SetErrMsg("new float[%d] : %s", size, strerror(errno));
		return(-1);
	}

	wb3d_c = new WaveletBlock3D(_bs[0], n_c, ntilde_c, _nthreads);
	if (wb3d_c->GetErrCode() != 0) {
		SetErrMsg(
			"WaveletBlock3D(%d,%d,%d,%d) : %s",
			_bs[0], n_c, ntilde_c, _nthreads, wb3d_c->GetErrMsg()
		);
		return(-1);
	}
	return(0);
}

void	WaveletBlock3DIO::my_free(
) {
	int	j;

	for(j=0; j<MAX_LEVELS; j++) {
		if (_mins[j]) delete [] _mins[j];
		if (_maxs[j]) delete [] _maxs[j];
		_mins[j] = NULL;
		_maxs[j] = NULL;
	}
	if (super_block_c) delete [] super_block_c;
	if (wb3d_c) delete wb3d_c;

	super_block_c = NULL;
	wb3d_c = NULL;
}
