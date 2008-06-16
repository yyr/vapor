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
#include "vapor/WaveletBlock2DIO.h"
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


int	WaveletBlock2DIO::_WaveletBlock2DIO(
) {
	SetClassName("WaveletBlock2DIO");

	_wb2dXY = NULL;
	_wb2dXZ = NULL;
	_wb2dYZ = NULL;
	_vtype = VARUNKNOWN;
	_super_tile = NULL;

	if (this->my_alloc() < 0) {
		this->my_free();
		return (-1);
	}
	return(0);
}

WaveletBlock2DIO::WaveletBlock2DIO(
	const Metadata	*metadata
) : WaveletBlockIOBase(metadata, 1) {

	_objInitialized = 0;
	if (WaveletBlockIOBase::GetErrCode()) return;

	if (_WaveletBlock2DIO() < 0) return;

	_objInitialized = 1;
}

WaveletBlock2DIO::WaveletBlock2DIO(
	const char *metafile
) : WaveletBlockIOBase(metafile, 1) {
	_objInitialized = 0;

	if (WaveletBlockIOBase::GetErrCode()) return;

	if (_WaveletBlock2DIO() < 0) return;

	_objInitialized = 1;
}

WaveletBlock2DIO::~WaveletBlock2DIO() {

	if (! _objInitialized) return;

	my_free();

	_objInitialized = 0;
}


int	WaveletBlock2DIO::OpenVariableWrite(
	size_t timestep,
	const char *varname,
	int reflevel
) {

	int min;
	_vtype = GetVarType(_metadata, varname);

	switch (_vtype) {
	case VAR2D_XY:
		min = Min((int)_bdim[0],(int) _bdim[1]);
		break;
	case VAR2D_XZ:
		min = Min((int)_bdim[0],(int) _bdim[2]);
		break;
	case VAR2D_YZ:
		min = Min((int)_bdim[1],(int) _bdim[2]);
		break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	if (LogBaseN(min, 2.0) < 0) {
		SetErrMsg("Too many levels (%d) in transform ", reflevel);
		return(-1);
	}
	return(WaveletBlockIOBase::OpenVariableWrite(timestep,varname,reflevel));
}


int WaveletBlock2DIO::ncDefineDimsVars(
	int j, const string &path, const int bs_dim_ids[3], const int dim_ids[3]
) {

	//
	// Define netCDF dimensions
	//
		
	size_t bdim[3];
	int nb;
	if (j==0) {
		VDFIOBase::GetDimBlk(bdim,j);
		nb = 1;
	}
	else {
		VDFIOBase::GetDimBlk(bdim,j-1);
		nb = 3;
	}

	switch (_vtype) {
	case VAR2D_XY:
		nb *= bdim[0]*bdim[1];
		break;
	case VAR2D_XZ:
		nb *= bdim[0]*bdim[2];
		break;
	case VAR2D_YZ:
		nb *= bdim[1]*bdim[2];
		break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	int nblk_dim_id;
	int rc = nc_def_dim(_ncids[j], _nBlocksDimName.c_str(), nb, &nblk_dim_id);
	NC_ERR_WRITE(rc,path)

	int block_dim_ids[2];
	int coeff_dim_ids[3];
	switch (_vtype) {
	case VAR2D_XY:
		block_dim_ids[0] = dim_ids[1];
		block_dim_ids[1] = dim_ids[0];
		coeff_dim_ids[0]= nblk_dim_id;
		coeff_dim_ids[1]= bs_dim_ids[1];
		coeff_dim_ids[2]= bs_dim_ids[0];
		break;
	case VAR2D_XZ:
		block_dim_ids[0] = dim_ids[2];
		block_dim_ids[1] = dim_ids[0];
		coeff_dim_ids[0]= nblk_dim_id;
		coeff_dim_ids[1]= bs_dim_ids[2];
		coeff_dim_ids[2]= bs_dim_ids[0];
		break;
	case VAR2D_YZ:
		block_dim_ids[0] = dim_ids[2];
		block_dim_ids[1] = dim_ids[1];
		coeff_dim_ids[0]= nblk_dim_id;
		coeff_dim_ids[1]= bs_dim_ids[2];
		coeff_dim_ids[2]= bs_dim_ids[1];
		break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}


	//
	// Define netCDF variables
	//
	rc = nc_def_var(
		_ncids[j], _minsName.c_str(), NC_FLOAT, 
		sizeof(block_dim_ids)/sizeof(block_dim_ids[0]), 
		block_dim_ids, &_ncminvars[j]
	);
	NC_ERR_WRITE(rc,path)

	rc = nc_def_var(
		_ncids[j], _maxsName.c_str(), NC_FLOAT, 
		sizeof(block_dim_ids)/sizeof(block_dim_ids[0]), 
		block_dim_ids, &_ncmaxvars[j]
	);
	NC_ERR_WRITE(rc,path)


	if (j==0) {
		rc = nc_def_var(
			_ncids[j], _lambdaName.c_str(), NC_FLOAT, 
			sizeof(coeff_dim_ids)/sizeof(coeff_dim_ids[0]), 
			coeff_dim_ids, &_ncvars[j]
		);
	}
	else {
		rc = nc_def_var(
			_ncids[j], _gammaName.c_str(), NC_FLOAT, 
			sizeof(coeff_dim_ids)/sizeof(coeff_dim_ids[0]), 
			coeff_dim_ids, &_ncvars[j]
		);
	}

	NC_ERR_WRITE(rc,path)

	return(0);
}


int	WaveletBlock2DIO::OpenVariableRead(
	size_t timestep,
	const char *varname,
	int reflevel
) {
	_vtype = GetVarType(_metadata, varname);
	return(WaveletBlockIOBase::OpenVariableRead(timestep,varname,reflevel));
}

int WaveletBlock2DIO::ncVerifyDimsVars(
	int j, const string &path
) {

	//
	// Verify coefficient file metadata matches VDF metadata
	//
	size_t bdim[3];
	int nb;
	if (j==0) {
		VDFIOBase::GetDimBlk(bdim,j);
		nb = 1;
	}
	else {
		VDFIOBase::GetDimBlk(bdim,j-1);
		nb = 3;
	}

	switch (_vtype) {
	case VAR2D_XY:
		nb *= bdim[0]*bdim[1];
		break;
	case VAR2D_XZ:
		nb *= bdim[0]*bdim[2];
		break;
	case VAR2D_YZ:
		nb *= bdim[1]*bdim[2];
		break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
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

int	WaveletBlock2DIO::CloseVariable()
{
	_vtype = VARUNKNOWN;
	return(WaveletBlockIOBase::CloseVariable());
}


int	WaveletBlock2DIO::seekLambdaBlocks(
	const size_t bcoord[2]
) {

	unsigned int	offset;
	size_t bdim[3];

	VDFIOBase::GetDimBlk(bdim, 0);

	bool err = false;
	switch (_vtype) {
	case VAR2D_XY:
		err = bcoord[0]>=bdim[0] || bcoord[1]>=bdim[1];
		offset = (bcoord[1]*bdim[0]) + (bcoord[0]);
		break;
	case VAR2D_XZ:
		err = bcoord[0]>=bdim[0] || bcoord[1]>=bdim[2];
		offset = (bcoord[1]*bdim[0]) + (bcoord[0]);
		break;
	case VAR2D_YZ:
		err = bcoord[0]>=bdim[1] || bcoord[1]>=bdim[2];
		offset = (bcoord[1]*bdim[1]) + (bcoord[0]);
		break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}
	if (err) {
		SetErrMsg("Invalid block address : %dx%dx", bcoord[0], bcoord[1]);
		return(-1);
	}

	return(seekBlocks(offset, 0));
}

int	WaveletBlock2DIO::seekGammaBlocks(
	const size_t bcoord[2],
	int reflevel
) {
	size_t bdim[3];

	unsigned int	offset;


	if (reflevel < 0 || reflevel > _reflevel) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}

	VDFIOBase::GetDimBlk(bdim, reflevel-1);

	bool err = false;
	switch (_vtype) {
	case VAR2D_XY:
		err = bcoord[0]>=bdim[0] || bcoord[1]>=bdim[1];
		offset = (bcoord[1]*bdim[0]) + (bcoord[0]);
		break;
	case VAR2D_XZ:
		err = bcoord[0]>=bdim[0] || bcoord[1]>=bdim[2];
		offset = (bcoord[1]*bdim[0]) + (bcoord[0]);
		break;
	case VAR2D_YZ:
		err = bcoord[0]>=bdim[1] || bcoord[1]>=bdim[2];
		offset = (bcoord[1]*bdim[1]) + (bcoord[0]);
		break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	if (err) {
		SetErrMsg("Invalid block address : %dx%dx", bcoord[0], bcoord[1]);
		return(-1);
	}

	offset *= 3;
	return(seekBlocks(offset, reflevel));
}

int	WaveletBlock2DIO::readBlocks(
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
		SetErrMsg("WaveletBlock2DIO : object created for writing");
		return(-1);
	}

	if (reflevel > _reflevel) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}

	int i1, i2;
	switch (_vtype) {
	case VAR2D_XY:
		i1 = _bs[1];
		i2 = _bs[0];
		break;
	case VAR2D_XZ:
		i1 = _bs[2];
		i2 = _bs[0];
		break;
	case VAR2D_YZ:
		i1 = _bs[2];
		i2 = _bs[1];
		break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	size_t start[] = {_ncoffsets[reflevel],0,0};
	size_t count[] = {n,i1,i2};

	TIMER_START(t0)
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

int	WaveletBlock2DIO::readLambdaBlocks(
	size_t n,
	float *blks
) {
	return(readBlocks(n, blks, 0));
}

int	WaveletBlock2DIO::readGammaBlocks(
	size_t n, 
	float *blks,
	int reflevel
) {

	if (reflevel < 0 || reflevel > _reflevel) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}

	return(readBlocks(n*3, blks, reflevel));
}


int	WaveletBlock2DIO::writeBlocks(
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
		SetErrMsg("WaveletBlock2DIO : object created for reading");
		return(-1);
	}

	if (reflevel > _reflevel) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}

	int i1, i2;
	switch (_vtype) {
	case VAR2D_XY:
		i1 = _bs[1];
		i2 = _bs[0];
		break;
	case VAR2D_XZ:
		i1 = _bs[2];
		i2 = _bs[0];
		break;
	case VAR2D_YZ:
		i1 = _bs[2];
		i2 = _bs[1];
		break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	size_t start[] = {_ncoffsets[reflevel],0,0};
	size_t count[] = {n,i1,i2};

	TIMER_START(t0)
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

int	WaveletBlock2DIO::writeLambdaBlocks(
	const float *blks, 
	size_t n
) {
	return(writeBlocks(blks, n, 0));
}

int	WaveletBlock2DIO::writeGammaBlocks(
	const float *blks, 
	size_t n, 
	int reflevel
) {

	if (reflevel < 0 || reflevel > _reflevel) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}
	return(writeBlocks(blks, n*3, reflevel));
}

void	WaveletBlock2DIO::Tile2NonTile(
	const float *blk, 
	const size_t bcoord[2],
	const size_t min[2],
	const size_t max[2],
	VarType_T vtype,
	float	*voxels
) const {

	SetDiagMsg(
		"WaveletBlock2DIO::Tile2NonTile(,[%d,%d],[%d,%d],[%d,%d])",
		bcoord[0], bcoord[1], min[0], min[1], max[0], max[1]
	);

	size_t y;
	int bs[2];
	switch (_vtype) {
	case VAR2D_XY:
		bs[0] = _bs[0];
		bs[1] = _bs[1];
		break;
	case VAR2D_XZ:
		bs[0] = _bs[0];
		bs[1] = _bs[2];
		break;
	case VAR2D_YZ:
		bs[0] = _bs[1];
		bs[1] = _bs[2];
		break;
	default:
		return;
	}

	assert((min[0] < bs[0]) && (max[0] > min[0]) && (max[0]/bs[0] >= (bcoord[0])));
	assert((min[1] < bs[1]) && (max[1] > min[1]) && (max[1]/bs[1] >= (bcoord[1])));

	size_t dim[] = {max[0]-min[0]+1, max[1]-min[1]+1};
	size_t xsize = bs[0];
	size_t ysize = bs[1];

	//
	// Address of first destination voxel
	//
	voxels += (bcoord[1]*bs[1]*dim[0]) + (bcoord[0]*bs[0]);

	if (bcoord[0] == 0) {
		blk += min[0];
		xsize = _bs[0] - min[0];
	}
	else {
		voxels -= min[0];
	}
	if (bcoord[0] == max[0]/bs[0]) xsize -= bs[0] - (max[0] % bs[0] + 1);

	if (bcoord[1] == 0) {
		blk += min[1] * bs[0];
		ysize = bs[1] - min[1];
	}
	else {
		voxels -= dim[0] * min[1];
	}
	if (bcoord[1] == max[1]/bs[1]) ysize -= bs[1] - (max[1] % bs[1] + 1);

	float *voxptr = voxels;
	const float *blkptr = blk;

	for(y=0; y<ysize; y++) {

		memcpy(voxptr,blkptr,xsize*sizeof(blkptr[0]));
		blkptr += bs[0];
		voxptr += dim[0];
	}
}



int	WaveletBlock2DIO::my_alloc(
) {

	int     size;
	int	j;


	// alloc space from coarsest (j==0) to finest level
	for(j=0; j<_num_reflevels; j++) {
		size_t nb_j[3];

		VDFIOBase::GetDimBlk(nb_j, j);

		// conservatively estimate size since it varies with 
		// 2D plane.
		//
		size = max(nb_j[0],  max(nb_j[1], nb_j[2])) * 
			max(nb_j[0],  max(nb_j[1], nb_j[2]));

		_mins[j] = new float[size];
		_maxs[j] = new float[size];
		if (! _mins[j] || ! _maxs[j]) {
			SetErrMsg("new float[%d] : %s", size, strerror(errno));
			return(-1);
		}
		for (int i=0; i<size; i++) _mins[j][i]  = _maxs[j][i] = 0.0;
	}

	// conservatively estimate size since it varies with 
	// 2D plane.
	//
	size_t tile_size = max(_bs[0],  max(_bs[1], _bs[2])) * 
		max(_bs[0],  max(_bs[1], _bs[2]));

	size = tile_size * 4;

	_super_tile = new float[size];
	if (! _super_tile) {
		SetErrMsg("new float[%d] : %s", size, strerror(errno));
		return(-1);
	}

	{
		size_t bs[] = {_bs[0],_bs[1]};
		_wb2dXY = new WaveletBlock2D(bs, n_c, ntilde_c);
		if (_wb2dXY->GetErrCode() != 0) return(-1);
	}
	{
		size_t bs[] = {_bs[0],_bs[2]};
		_wb2dXZ = new WaveletBlock2D(bs, n_c, ntilde_c);
		if (_wb2dXZ->GetErrCode() != 0) return(-1);
	}
	{
		size_t bs[] = {_bs[1],_bs[2]};
		_wb2dYZ = new WaveletBlock2D(bs, n_c, ntilde_c);
		if (_wb2dYZ->GetErrCode() != 0) return(-1);
	}

	return(0);
}

void	WaveletBlock2DIO::my_free(
) {
	int	j;

	for(j=0; j<MAX_LEVELS; j++) {
		if (_mins[j]) delete [] _mins[j];
		if (_maxs[j]) delete [] _maxs[j];
		_mins[j] = NULL;
		_maxs[j] = NULL;
	}
	if (_super_tile) delete [] _super_tile;
	if (_wb2dXY) delete _wb2dXY;
	if (_wb2dXZ) delete _wb2dXZ;
	if (_wb2dYZ) delete _wb2dYZ;

	_super_tile = NULL;
	_wb2dXY = NULL;
	_wb2dXZ = NULL;
	_wb2dYZ = NULL;
}
