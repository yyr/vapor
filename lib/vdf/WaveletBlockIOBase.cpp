//
// $Id$
//
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <cassert>
#include <locale>
#ifndef WIN32
#include <unistd.h>
#else
#pragma warning(disable : 4996)
#include "windows.h"
#include "vaporinternal/common.h"
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include "vapor/WaveletBlockIOBase.h"
#include "vapor/MyBase.h"
#include "vapor/errorcodes.h"

using namespace VetsUtil;
using namespace VAPoR;

void	VDF_API mkpath(const string &basename, int level, string &path, int v);

const string WaveletBlockIOBase::_blockSizeXName = "BlockSizeNx";
const string WaveletBlockIOBase::_blockSizeYName = "BlockSizeNy";
const string WaveletBlockIOBase::_blockSizeZName = "BlockSizeNz";
const string WaveletBlockIOBase::_nBlocksDimName = "NumCoefBlocks";
const string WaveletBlockIOBase::_blockDimXName = "BlockDimNx";
const string WaveletBlockIOBase::_blockDimYName = "BlockDimNy";
const string WaveletBlockIOBase::_blockDimZName = "BlockDimNz";
const string WaveletBlockIOBase::_fileVersionName = "FileVersion";
const string WaveletBlockIOBase::_refLevelName = "RefinementLevel";
const string WaveletBlockIOBase::_nativeResName = "NativeResolution";
const string WaveletBlockIOBase::_refLevelResName = "refinementLevelResolution";
const string WaveletBlockIOBase::_nativeMinValidRegionName = "NativeMinValidRegion";
const string WaveletBlockIOBase::_nativeMaxValidRegionName = "NativeMaxValidRegion";
const string WaveletBlockIOBase::_refLevMinValidRegionName = "RefLevMinValidRegion";
const string WaveletBlockIOBase::_refLevMaxValidRegionName = "RefLevMaxValidRegion";
const string WaveletBlockIOBase::_filterCoeffName = "NumFilterCoeff";
const string WaveletBlockIOBase::_liftingCoeffName = "NumLiftingCoeff";
const string WaveletBlockIOBase::_scalarRangeName = "ScalarDataRange";
const string WaveletBlockIOBase::_minsName = "BlockMinValues";
const string WaveletBlockIOBase::_maxsName = "BlockMaxValues";
const string WaveletBlockIOBase::_lambdaName = "LambdaCoefficients";
const string WaveletBlockIOBase::_gammaName = "GammaCoefficients";

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

//
// IO size for netCDF 
//
#define	NC_CHUNKSIZEHINT	1024*1024


int	WaveletBlockIOBase::_WaveletBlockIOBase(
) {
	SetClassName("WaveletBlockIOBase");

	if (GetNumTransforms() >= MAX_LEVELS) {
		SetErrMsg("Too many refinement levels");
		return(-1);
	}

	_wb2dXY = NULL;
	_wb2dXZ = NULL;
	_wb2dYZ = NULL;
	_wb3d = NULL;
	_super_block = NULL;
    _vtype = VARUNKNOWN;
    _super_tile = NULL;

	_is_alloc2d = false;
	_is_alloc3d = false;

	for(int j=0; j<MAX_LEVELS; j++) {
		_mins2d[j] = NULL;
		_maxs2d[j] = NULL;
		_mins3d[j] = NULL;
		_maxs3d[j] = NULL;
		_ncids[j] = -1;
		_ncvars[j] = -1;
		_ncminvars[j] = -1;
		_ncmaxvars[j] = -1;
		_ncpaths[j].clear();
		_ncoffsets[j] = 0;
	}

	_reflevel = 0;

	is_open_c = 0;

	return(0);
}

WaveletBlockIOBase::WaveletBlockIOBase(
	const MetadataVDC	&metadata
) : VDFIOBase(metadata) {

	if (VDFIOBase::GetErrCode()) return;

	if (_WaveletBlockIOBase() < 0) return;
}

WaveletBlockIOBase::WaveletBlockIOBase(
	const string &metafile
) : VDFIOBase(metafile) {

	if (VDFIOBase::GetErrCode()) return;

	if (_WaveletBlockIOBase() < 0) return;

}

WaveletBlockIOBase::~WaveletBlockIOBase() {

	if (_is_alloc2d) my_free2d();
	if (_is_alloc3d) my_free3d();
}

int    WaveletBlockIOBase::VariableExists(
    size_t timestep,
    const char *varname,
    int reflevel,
	int 
) const {
	string basename;

    if (reflevel < 0) reflevel = GetNumTransforms();

	if (ConstructFullVBase(timestep, varname, &basename) < 0) {
		SetErrCode(0);
		return (0);
	}

	for (int j = 0; j<=reflevel; j++){

		struct STAT64 statbuf;

		string path;
        mkpath(basename, j, path, GetVDFVersion());

		if (STAT64(path.c_str(), &statbuf) < 0) return (0);
	}
	return(1);
}

int	WaveletBlockIOBase::OpenVariableWrite(
	size_t timestep,
	const char *varname,
	int reflevel
) {

	int min;
	_vtype = GetVarType(varname);

	size_t dim[3];
	GetDim(dim, -1);
	for(int i=0; i<3; i++) {
		_validRegMin[i] = 0;
		_validRegMax[i] = dim[i]-1;
	}

	size_t bdim[3];
	VDFIOBase::GetDimBlk(bdim, GetNumTransforms());

	switch (_vtype) {
	case VAR2D_XY:
		min = Min((int) bdim[0],(int) bdim[1]);
	break;
	case VAR2D_XZ:
		min = Min((int) bdim[0],(int) bdim[2]);
	break;
	case VAR2D_YZ:
		min = Min((int) bdim[1],(int) bdim[2]);
	break;
	case VAR3D:
		min = Min((int) bdim[0],(int) bdim[1]);
		min = Min((int)min, (int) bdim[2]);
	break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	if (LogBaseN(min, 2.0) < 0) {
		SetErrMsg("Too many levels (%d) in transform ", reflevel);
		return(-1);
	}

	if (_vtype == VAR3D) {
		if (! _is_alloc3d) {
			if (this->my_alloc3d() < 0) {
				this->my_free3d();
				return (-1);
			}
			_is_alloc3d = true;
		}
	}
	else { 
		if (! _is_alloc2d) { 
			if (this->my_alloc2d() < 0) {
				this->my_free2d();
				return (-1);
			}
			_is_alloc2d = true;
		}
	}

	string dir;
	string basename;

	_dataRange[0] = _dataRange[1] = 0.0;

    if (reflevel < 0) reflevel = GetNumTransforms();

	write_mode_c = 1;

	_timeStep = timestep;
	_varName.assign(varname);
	_reflevel = reflevel;

	WaveletBlockIOBase::CloseVariable(); // close any previously opened files

	if (reflevel > GetNumTransforms()) {
		SetErrMsg("Requested refinement level out of range (%d)", reflevel);
		return(-1);
	}

	if (ConstructFullVBase(timestep, varname, &basename) < 0) {
		return (-1);
	}

	DirName(basename, dir);
	if (MkDirHier(dir) < 0) return(-1);
	

	int rc = open_var_write(basename);
	if (rc<0) return(-1);


	is_open_c = 1;

	return(0);
}


int WaveletBlockIOBase::open_var_write(
	const string &basename
) {

	//
	// Create a netCDF file for each refinement level
	//
	const size_t *bs = GetBlockSize();
	size_t dim[3];
	GetDim(dim, -1);
	for(int j=0; j<=_reflevel; j++) {
		string path;
		int rc;

		_ncoffsets[j] = 0;

		mkpath(basename, j, path, GetVDFVersion());
		_ncpaths[j] = path;

		size_t chsz = NC_CHUNKSIZEHINT;
		rc = nc__create(path.c_str(), NC_64BIT_OFFSET, 0, &chsz, &_ncids[j]);
		NC_ERR_WRITE(rc,path)
	
		// Disable data filling - may not be necessary
		//
		int mode;
		nc_set_fill(_ncids[j], NC_NOFILL, &mode);

		//
		// Define netCDF dimensions
		//
		int bs_dim_ids[3];

		rc = nc_def_dim(_ncids[j],_blockSizeXName.c_str(),bs[0],&bs_dim_ids[0]);
		NC_ERR_WRITE(rc,path)

		rc = nc_def_dim(_ncids[j],_blockSizeYName.c_str(),bs[1],&bs_dim_ids[1]);
		NC_ERR_WRITE(rc,path)

		rc = nc_def_dim(_ncids[j],_blockSizeZName.c_str(),bs[2],&bs_dim_ids[2]);
		NC_ERR_WRITE(rc,path)

		int dim_ids[3];
		size_t bdim[3];

		VDFIOBase::GetDimBlk(bdim,j);
		rc = nc_def_dim(_ncids[j],_blockDimXName.c_str(),bdim[0],&dim_ids[0]);
		NC_ERR_WRITE(rc,path)

		rc = nc_def_dim(_ncids[j],_blockDimYName.c_str(),bdim[1],&dim_ids[1]);
		NC_ERR_WRITE(rc,path)

		rc = nc_def_dim(_ncids[j],_blockDimZName.c_str(),bdim[2],&dim_ids[2]);
		NC_ERR_WRITE(rc,path)

		// Call child class method to define dimensions and variables
		//
		if (_vtype == VAR3D) {
			if (ncDefineDimsVars3D(j,path, bs_dim_ids, dim_ids) < 0) return(-1);
		}
		else {
			if (ncDefineDimsVars2D(j,path, bs_dim_ids, dim_ids) < 0) return(-1);
		}

		//
		// Define netCDF global attributes
		//
		int version = GetVDFVersion();
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_fileVersionName.c_str(),NC_INT, 1, &version
		);
		NC_ERR_WRITE(rc,path)

		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_refLevelName.c_str(),NC_INT, 1, &j
		);
		NC_ERR_WRITE(rc,path)

		int dim_int[] = {dim[0], dim[1], dim[2]};
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_nativeResName.c_str(),NC_INT, 3, dim_int
		);
		NC_ERR_WRITE(rc,path)

		size_t dimj[3];
		VDFIOBase::GetDim(dimj,j);
		int dimj_int[] = {dimj[0], dimj[1], dimj[2]};
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_refLevelResName.c_str(),NC_INT, 3, dimj_int
		);
		NC_ERR_WRITE(rc,path)

		int minreg_int[3] = {_validRegMin[0],_validRegMin[1],_validRegMin[2]};
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_nativeMinValidRegionName.c_str(),NC_INT, 3, 
			minreg_int
		);
		NC_ERR_WRITE(rc,path)

		int maxreg_int[3] = {_validRegMax[0],_validRegMax[1],_validRegMax[2]};
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_nativeMaxValidRegionName.c_str(),NC_INT, 3, 
			maxreg_int
		);
		NC_ERR_WRITE(rc,path)

		size_t rminreg[3], rmaxreg[3];
		WaveletBlockIOBase::GetValidRegion(rminreg, rmaxreg,j);
		int rminreg_int[] = {rminreg[0], rminreg[1], rminreg[2]};
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_refLevMinValidRegionName.c_str(),NC_INT, 3, 
			rminreg_int
		);
		NC_ERR_WRITE(rc,path)

		int rmaxreg_int[] = {rmaxreg[0], rmaxreg[1], rmaxreg[2]};
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_refLevMaxValidRegionName.c_str(),NC_INT, 3, 
			rmaxreg_int
		);
		NC_ERR_WRITE(rc,path)

		int n = GetFilterCoef();
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_filterCoeffName.c_str(),NC_INT, 1, &n
		);
		NC_ERR_WRITE(rc,path)

		int ntilde = GetLiftingCoef();
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_liftingCoeffName.c_str(),NC_INT, 1, &ntilde
		);
		NC_ERR_WRITE(rc,path)

		rc = nc_put_att_float(
			_ncids[j],NC_GLOBAL,_scalarRangeName.c_str(),NC_FLOAT, 2, _dataRange
		);

		NC_ERR_WRITE(rc,path)


		rc = nc_enddef(_ncids[j]);
		NC_ERR_WRITE(rc,path)

	}
	return(0);
		
}

int	WaveletBlockIOBase::OpenVariableRead(
	size_t timestep,
	const char *varname,
	int reflevel,
	int
) {
	string basename;

	write_mode_c = 0;
	_dataRange[0] = _dataRange[1] = 0.0;

	_timeStep = timestep;
	_varName.assign(varname);
	_reflevel = reflevel;

	size_t dim[3];
	GetDim(dim, -1);
	for(int i=0; i<3; i++) {
		_validRegMin[i] = 0;
		_validRegMax[i] = dim[i]-1;
	}

	WaveletBlockIOBase::CloseVariable(); // close previously opened files

	if (reflevel > GetNumTransforms()) {
		SetErrMsg("Requested refinement level out of range (%d)", reflevel);
		return(-1);
	}

	if (!VariableExists(timestep, varname, reflevel)) {
		SetErrMsg(VAPOR_ERROR_VDF,
			"Variable \"%s\" not present \nat requested timestep or level",
			varname
		);
		return(-1);
	}

	if (ConstructFullVBase(timestep, varname, &basename) < 0) {
		SetErrCode(0);
		return (0);
	}

	_vtype = GetVarType(varname);
    if (_vtype == VAR3D) {
		if (! _is_alloc3d) {
			if (this->my_alloc3d() < 0) {
				this->my_free3d();
				return (-1);
			}
			_is_alloc3d = true;
		}
	}
	else { 
		if (! _is_alloc2d) { 
			if (this->my_alloc2d() < 0) {
				this->my_free2d();
				return (-1);
			}
			_is_alloc2d = true;
		}
	}


	int rc = open_var_read(timestep, varname, basename);
	if (rc<0) return(-1);


	is_open_c = 1;

	return(0);
}


int WaveletBlockIOBase::open_var_read(
	size_t ts,
	const char *varname,
	const string &basename
) {
	const size_t *bs = GetBlockSize();



	for(int j=0; j<=_reflevel; j++) {

		int	ncdimid;
		size_t	ncdim;
		string path;
		int	rc;


		_ncoffsets[j] = 0;

		mkpath(basename, j, path, GetVDFVersion());
		_ncpaths[j] = path;

		size_t chsz = NC_CHUNKSIZEHINT;

		int ii = 0;
		do {
			rc = nc__open(path.c_str(), NC_NOWRITE, &chsz, &_ncids[j]);
#ifdef WIN32
			if (rc == EAGAIN) Sleep(100);//milliseconds
#else
			if (rc == EAGAIN) sleep(1);//seconds
#endif
			ii++;

		} while (rc != NC_NOERR && ii < 10);
		NC_ERR_READ(rc,path)


		//
		// Verify coefficient file metadata matches VDF metadata
		//
		rc = nc_inq_dimid(_ncids[j], _blockSizeXName.c_str(), &ncdimid);
		if (rc==NC_EBADDIM) {
			// Ugh. Deal with old data format from trial version
			// of vapor
			//
			rc = nc_inq_dimid(_ncids[j], "BlockNx", &ncdimid);
		}
		NC_ERR_READ(rc,path)
		rc = nc_inq_dimlen(_ncids[j], ncdimid, &ncdim);
		NC_ERR_READ(rc,path)
		if (ncdim != bs[0]) {
			SetErrMsg("Metadata file and data file mismatch");
			return(-1);
		}

		rc = nc_inq_dimid(_ncids[j], _blockSizeYName.c_str(), &ncdimid);
		if (rc==NC_EBADDIM) {
			rc = nc_inq_dimid(_ncids[j], "BlockNy", &ncdimid);
		}
		NC_ERR_READ(rc,path)
		rc = nc_inq_dimlen(_ncids[j], ncdimid, &ncdim);
		NC_ERR_READ(rc,path)
		if (ncdim != bs[1]) {
			SetErrMsg("Metadata file and data file mismatch");
			return(-1);
		}

		rc = nc_inq_dimid(_ncids[j], _blockSizeZName.c_str(), &ncdimid);
		if (rc==NC_EBADDIM) {
			rc = nc_inq_dimid(_ncids[j], "BlockNz", &ncdimid);
		}
		NC_ERR_READ(rc,path)
		rc = nc_inq_dimlen(_ncids[j], ncdimid, &ncdim);
		NC_ERR_READ(rc,path)
		if (ncdim != bs[2]) {
			SetErrMsg("Metadata file and data file mismatch");
			return(-1);
		}

		// Call child class method to verify dimensions and variables
		//
		if (_vtype == VAR3D) {
			if (ncVerifyDimsVars3D(j,path) < 0) return(-1);
		}
		else {
			if (ncVerifyDimsVars2D(j,path) < 0) return(-1);
		}

		rc = nc_get_att_float(
			_ncids[j],NC_GLOBAL,_scalarRangeName.c_str(),_dataRange
		);
		NC_ERR_READ(rc,path)

		int minreg_int[3];
		rc = nc_get_att_int(
			_ncids[j],NC_GLOBAL,_nativeMinValidRegionName.c_str(),minreg_int
		);
		if (rc == NC_NOERR) {
			for(int i=0; i<3; i++) _validRegMin[i] = minreg_int[i];
		}

		int maxreg_int[3];
		rc = nc_get_att_int(
			_ncids[j],NC_GLOBAL,_nativeMaxValidRegionName.c_str(),maxreg_int
		);
		if (rc == NC_NOERR) {
			for(int i=0; i<3; i++) _validRegMax[i] = maxreg_int[i];
		}

				
		rc = nc_inq_varid(_ncids[j], _minsName.c_str(), &_ncminvars[j]);
		if (rc==NC_ENOTVAR) { 
			rc = NC_NOERR;
			_ncminvars[j] = -1;
		}
		NC_ERR_READ(rc,path)

		rc = nc_inq_varid(_ncids[j], _maxsName.c_str(), &_ncmaxvars[j]);
		if (rc==NC_ENOTVAR) { 
			rc = NC_NOERR;
			_ncmaxvars[j] = -1;
		}
		NC_ERR_READ(rc,path)

		if (j==0) {
			rc = nc_inq_varid(_ncids[j], _lambdaName.c_str(), &_ncvars[j]);
			if (rc==NC_ENOTVAR) {
				rc = nc_inq_varid(_ncids[j], "Lambda", &_ncvars[j]);
			}
		}
		else {
			rc = nc_inq_varid(_ncids[j], _gammaName.c_str(), &_ncvars[j]);
			if (rc==NC_ENOTVAR) {
				rc = nc_inq_varid(_ncids[j], "Gamma", &_ncvars[j]);
			}
		}
		NC_ERR_READ(rc,path)


		// Go ahead and read the block min & max
		//
		float *mins = NULL;
		float *maxs = NULL;
		if (_vtype == VAR3D) {
			mins = _mins3d[j];
			maxs = _maxs3d[j];
		}
		else {
			mins = _mins2d[j];
			maxs = _maxs2d[j];
		}
		if (_ncminvars[j] != -1) {
			rc = nc_get_var_float(_ncids[j], _ncminvars[j], mins);
			NC_ERR_READ(rc,path)
		}

		if (_ncmaxvars[j] != -1) {
			rc = nc_get_var_float(_ncids[j], _ncmaxvars[j], maxs);
			NC_ERR_READ(rc,path)
		}
	}
	return(0);
}

int	WaveletBlockIOBase::CloseVariable()
{
	int	j;

	if (! is_open_c) return(0);

	if (write_mode_c) {

		// Invoke derived class method to determine region bounds 
		// and data range
		//
		_GetDataRange(_dataRange);
		_GetValidRegion(_validRegMin, _validRegMax);

		_WriteTimerStart();
	}
	else {
		_ReadTimerStart();
	}
	for(j=0; j<MAX_LEVELS; j++) {

		if (_ncids[j] > -1) {
			int rc; 

			if (write_mode_c) {

				rc = nc_put_att_float(
					_ncids[j], NC_GLOBAL, _scalarRangeName.c_str(), 
					NC_FLOAT, 2,_dataRange
				);
				NC_ERR_WRITE(rc,_ncpaths[j])

				int minreg_int[] = {
					_validRegMin[0], _validRegMin[1], _validRegMin[2]
				};
				rc = nc_put_att_int(
					_ncids[j],NC_GLOBAL,_nativeMinValidRegionName.c_str(),
					NC_INT, 3, minreg_int
				);
				NC_ERR_WRITE(rc,_ncpaths[j])

				int maxreg_int[] = {
					_validRegMax[0], _validRegMax[1], _validRegMax[2]
				};
				rc = nc_put_att_int(
					_ncids[j],NC_GLOBAL,_nativeMaxValidRegionName.c_str(),
					NC_INT, 3, maxreg_int
				);
				NC_ERR_WRITE(rc,_ncpaths[j])

				size_t rminreg[3], rmaxreg[3];
				WaveletBlockIOBase::GetValidRegion(rminreg, rmaxreg,j);
				int rminreg_int[] = {rminreg[0], rminreg[1], rminreg[2]};
				rc = nc_put_att_int(
					_ncids[j],NC_GLOBAL,_refLevMinValidRegionName.c_str(),
					NC_INT, 3, rminreg_int
				);
				NC_ERR_WRITE(rc,_ncpaths[j])

				int rmaxreg_int[] = {rmaxreg[0], rmaxreg[1], rmaxreg[2]};
				rc = nc_put_att_int(
					_ncids[j],NC_GLOBAL,_refLevMaxValidRegionName.c_str(),
					NC_INT, 3, rmaxreg_int
				);
				NC_ERR_WRITE(rc,_ncpaths[j])

				float *mins = NULL;
				float *maxs = NULL;
				if (_vtype == VAR3D) {
					mins = _mins3d[j];
					maxs = _maxs3d[j];
				}
				else {
					mins = _mins2d[j];
					maxs = _maxs2d[j];
				}
				rc = nc_put_var_float(_ncids[j], _ncminvars[j], mins);
				NC_ERR_WRITE(rc,_ncpaths[j])

				rc = nc_put_var_float(_ncids[j], _ncmaxvars[j], maxs);
				NC_ERR_WRITE(rc,_ncpaths[j])
			}
			rc = nc_close(_ncids[j]);
			if (rc != NC_NOERR) { 
				SetErrMsg( 
					"Error writing netCDF file : %s",  
					nc_strerror(rc) 
				); 
				return(-1); 
			}
		}
		_ncids[j] = -1;
		_ncvars[j] = -1;
		_ncminvars[j] = -1;
		_ncmaxvars[j] = -1;
		_ncoffsets[j] = 0;
	}
	if (write_mode_c) {
		_WriteTimerStop();
	}
	else {
		_ReadTimerStop();
	}
				
	is_open_c = 0;

    _vtype = VARUNKNOWN;

	return(0);
}

int	WaveletBlockIOBase::readBlocks(
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
		SetErrMsg("WaveletBlockIOBase : object created for writing");
		return(-1);
	}

	if (reflevel > _reflevel) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}

	const size_t *bs = GetBlockSize();

	int i1 = 0, i2 = 0;
	switch (_vtype) {
	case VAR2D_XY:
		i1 = bs[1];
		i2 = bs[0];
		break;
	case VAR2D_XZ:
		i1 = bs[2];
		i2 = bs[0];
		break;
	case VAR2D_YZ:
		i1 = bs[2];
		i2 = bs[1];
		break;
	case VAR3D:
		break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	_ReadTimerStart();
	if (_vtype == VAR3D) {
		size_t start[] = {_ncoffsets[reflevel],0,0,0};
		size_t count[] = {n,bs[2],bs[1],bs[0]};

		rc = nc_get_vars_float(
			_ncids[reflevel], _ncvars[reflevel], start, count, NULL, blks
		);
	}
	else {
		size_t start[] = {_ncoffsets[reflevel],0,0};
		size_t count[] = {n,i1,i2};

		rc = nc_get_vars_float(
				_ncids[reflevel], _ncvars[reflevel], start, count, NULL, blks
		);
	}

	if (rc != NC_NOERR) { 
		SetErrMsg( "Error reading netCDF file : %s",  nc_strerror(rc) ); 
		return(-1); 
	}
	_ReadTimerStop();

	_ncoffsets[reflevel] += n;

	return(0);
}

int WaveletBlockIOBase::readLambdaBlocks(
    size_t n,
    float *blks
) {
    return(readBlocks(n, blks, 0));
}

int WaveletBlockIOBase::readGammaBlocks(
    size_t n,
    float *blks,
    int reflevel
) {

    if (reflevel < 0 || reflevel > _reflevel) {
        SetErrMsg("Invalid refinement level : %d", reflevel);
        return(-1);
    }

	size_t sz = 0;
	if (_vtype == VAR3D) {
		sz = n*7;
	}
	else {
		sz = n*3;
	}

    return(readBlocks(sz, blks, reflevel));
}


int	WaveletBlockIOBase::writeBlocks(
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
		SetErrMsg("WaveletBlockIOBase : object created for reading");
		return(-1);
	}

	if (reflevel > _reflevel) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}

	const size_t *bs = GetBlockSize();

	int i1 = 0, i2 = 0;
	switch (_vtype) {
	case VAR2D_XY:
		i1 = bs[1];
		i2 = bs[0];
		break;
	case VAR2D_XZ:
		i1 = bs[2];
		i2 = bs[0];
		break;
	case VAR2D_YZ:
		i1 = bs[2];
		i2 = bs[1];
		break;
	case VAR3D:
		break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	_WriteTimerStart();
	if (_vtype == VAR3D) {

		size_t start[] = {_ncoffsets[reflevel],0,0,0};
		size_t count[] = {n,bs[2],bs[1],bs[0]};

		rc = nc_put_vars_float(
			_ncids[reflevel], _ncvars[reflevel], start, count, NULL, blks
		);

	}
	else {
		size_t start[] = {_ncoffsets[reflevel],0,0};
		size_t count[] = {n,i1,i2};

		rc = nc_put_vars_float(
				_ncids[reflevel], _ncvars[reflevel], start, count, NULL, blks
		);
	}

	if (rc != NC_NOERR) { 
		SetErrMsg( "Error writing netCDF file : %s",  nc_strerror(rc) ); 
		return(-1); 
	}
	_WriteTimerStop();

	_ncoffsets[reflevel] += n;

	return(0);
}

int	WaveletBlockIOBase::writeLambdaBlocks(
	const float *blks, 
	size_t n
) {
	return(writeBlocks(blks, n, 0));
}

int	WaveletBlockIOBase::writeGammaBlocks(
	const float *blks, 
	size_t n, 
	int reflevel
) {

	if (reflevel < 0 || reflevel > _reflevel) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}

	size_t sz;
	if (_vtype == VAR3D) {
		sz = n*7;
	}
	else {
		sz = n*3;
	}
	return(writeBlocks(blks, sz, reflevel));
}


int	WaveletBlockIOBase::seekBlocks(
	unsigned int offset, 
	int reflevel
) {

	if (! is_open_c) {
		SetErrMsg("File not open");
		return(-1);
	}

	if (reflevel > _reflevel) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}

	_ncoffsets[reflevel] = offset;

	return(0);
}


int	WaveletBlockIOBase::seekLambdaBlocks(
	const size_t bcoord[3]
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
	case VAR3D:
		err = bcoord[0]>=bdim[0] || bcoord[1]>=bdim[1] || bcoord[2]>=bdim[2];
		offset = (int)(bcoord[2] * bdim[0] * bdim[1] + (bcoord[1]*bdim[0]) + (bcoord[0]));

		break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}
	if (err) {
		SetErrMsg("Invalid block address");
		return(-1);
	}

	return(seekBlocks(offset, 0));
}


int	WaveletBlockIOBase::seekGammaBlocks(
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

	bool err = false;
	switch (_vtype) {
	case VAR2D_XY:
		err = bcoord[0]>=bdim[0] || bcoord[1]>=bdim[1];
		offset = ((bcoord[1]*bdim[0]) + bcoord[0]) * 3;
		break;
	case VAR2D_XZ:
		err = bcoord[0]>=bdim[0] || bcoord[1]>=bdim[2];
		offset = ((bcoord[1]*bdim[0]) + bcoord[0]) * 3;
		break;
	case VAR2D_YZ:
		err = bcoord[0]>=bdim[1] || bcoord[1]>=bdim[2];
		offset = ((bcoord[1]*bdim[1]) + bcoord[0]) * 3;
		break;
	case VAR3D:
		err = bcoord[0]>=bdim[0] || bcoord[1]>=bdim[1] || bcoord[2]>=bdim[2];
		offset = (unsigned int)((bcoord[2] * bdim[0] * bdim[1]) + (bcoord[1]*bdim[0]) + (bcoord[0])) *  7;
		break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	if (err) {
		SetErrMsg("Invalid block address");
		return(-1);
	}

	return(seekBlocks(offset, reflevel));
}

void    mkpath(const string &basename, int level, string &path, int version) {
	ostringstream oss;

	if (version < 2) {
		oss << basename << ".wb" << level;
	}
	else {
		oss << basename << ".nc" << level;
	}
	path = oss.str();
}

int	WaveletBlockIOBase::my_alloc2d(
) {

	int     size;
	int	j;

	const size_t *bs = GetBlockSize();

	// alloc space from coarsest (j==0) to finest level
	for(j=0; j<=GetNumTransforms(); j++) {
		size_t nb_j[3];

		VDFIOBase::GetDimBlk(nb_j, j);

		// conservatively estimate size since it varies with 
		// 2D plane.
		//
		size = max(nb_j[0],  max(nb_j[1], nb_j[2])) * 
			max(nb_j[0],  max(nb_j[1], nb_j[2]));

		_mins2d[j] = new float[size];
		_maxs2d[j] = new float[size];
		if (! _mins2d[j] || ! _maxs2d[j]) {
			SetErrMsg("new float[%d] : %s", size, strerror(errno));
			return(-1);
		}
		for (int i=0; i<size; i++) _mins2d[j][i]  = _maxs2d[j][i] = 0.0;
	}

	// conservatively estimate size since it varies with 
	// 2D plane.
	//
	size_t tile_size = max(bs[0],  max(bs[1], bs[2])) * 
		max(bs[0],  max(bs[1], bs[2]));

	size = tile_size * 4;

	_super_tile = new float[size];
	if (! _super_tile) {
		SetErrMsg("new float[%d] : %s", size, strerror(errno));
		return(-1);
	}

	int n = GetFilterCoef();
	int ntilde = GetLiftingCoef();
	{
		size_t bs2d[] = {bs[0],bs[1]};
		_wb2dXY = new WaveletBlock2D(bs2d, n, ntilde);
		if (_wb2dXY->GetErrCode() != 0) return(-1);
	}
	{
		size_t bs2d[] = {bs[0],bs[2]};
		_wb2dXZ = new WaveletBlock2D(bs2d, n, ntilde);
		if (_wb2dXZ->GetErrCode() != 0) return(-1);
	}
	{
		size_t bs2d[] = {bs[1],bs[2]};
		_wb2dYZ = new WaveletBlock2D(bs2d, n, ntilde);
		if (_wb2dYZ->GetErrCode() != 0) return(-1);
	}

	return(0);
}

void	WaveletBlockIOBase::my_free2d(
) {
	int	j;

	for(j=0; j<MAX_LEVELS; j++) {
		if (_mins2d[j]) delete [] _mins2d[j];
		if (_maxs2d[j]) delete [] _maxs2d[j];
		_mins2d[j] = NULL;
		_maxs2d[j] = NULL;
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


int	WaveletBlockIOBase::my_alloc3d(
) {

	int     size;
	int	j;


	// alloc space from coarsest (j==0) to finest level
	for(j=0; j<=GetNumTransforms(); j++) {
		size_t nb_j[3];

		VDFIOBase::GetDimBlk(nb_j, j);

		size = (int)(nb_j[0] * nb_j[1] * nb_j[2]);

		_mins3d[j] = new float[size];
		_maxs3d[j] = new float[size];
		if (! _mins3d[j] || ! _maxs3d[j]) {
			SetErrMsg("new float[%d] : %s", size, strerror(errno));
			return(-1);
		}
		for (int i=0; i<size; i++) _mins3d[j][i]  = _maxs3d[j][i] = 0.0;
	}

	const size_t *bs = GetBlockSize();
	size = bs[0]*bs[1]*bs[2] * 8;
	_super_block = new float[size];
	if (! _super_block) {
		SetErrMsg("new float[%d] : %s", size, strerror(errno));
		return(-1);
	}

	int n = GetFilterCoef();
	int ntilde = GetLiftingCoef();
	_wb3d = new WaveletBlock3D(bs[0], n, ntilde, 1);
	if (_wb3d->GetErrCode() != 0) {
		SetErrMsg(
			"WaveletBlock3D(%d,%d,%d,%d) : %s",
			bs[0], n, ntilde, 1, _wb3d->GetErrMsg()
		);
		return(-1);
	}
	return(0);
}

void	WaveletBlockIOBase::my_free3d(
) {
	int	j;

	for(j=0; j<MAX_LEVELS; j++) {
		if (_mins3d[j]) delete [] _mins3d[j];
		if (_maxs3d[j]) delete [] _maxs3d[j];
		_mins3d[j] = NULL;
		_maxs3d[j] = NULL;
	}
	if (_super_block) delete [] _super_block;
	if (_wb3d) delete _wb3d;

	_super_block = NULL;
	_wb3d = NULL;
}



int WaveletBlockIOBase::ncDefineDimsVars2D(
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

int WaveletBlockIOBase::ncDefineDimsVars3D(
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


int WaveletBlockIOBase::ncVerifyDimsVars2D(
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
int WaveletBlockIOBase::ncVerifyDimsVars3D(
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

void	WaveletBlockIOBase::Tile2NonTile(
	const float *blk, 
	const size_t bcoord[2],
	const size_t min[2],
	const size_t max[2],
	VarType_T vtype,
	float	*voxels
) const {

	SetDiagMsg(
		"WaveletBlockIOBase::Tile2NonTile(,[%d,%d],[%d,%d],[%d,%d])",
		bcoord[0], bcoord[1], min[0], min[1], max[0], max[1]
	);

	const size_t *bs = GetBlockSize();

	size_t y;
	int bs2d[2];
	switch (_vtype) {
	case VAR2D_XY:
		bs2d[0] = bs[0];
		bs2d[1] = bs[1];
		break;
	case VAR2D_XZ:
		bs2d[0] = bs[0];
		bs2d[1] = bs[2];
		break;
	case VAR2D_YZ:
		bs2d[0] = bs[1];
		bs2d[1] = bs[2];
		break;
	default:
		return;
	}

	assert((min[0] < bs2d[0]) && (max[0] > min[0]) && (max[0]/bs2d[0] >= (bcoord[0])));
	assert((min[1] < bs2d[1]) && (max[1] > min[1]) && (max[1]/bs2d[1] >= (bcoord[1])));

	size_t dim[] = {max[0]-min[0]+1, max[1]-min[1]+1};
	size_t xsize = bs2d[0];
	size_t ysize = bs2d[1];

	//
	// Address of first destination voxel
	//
	voxels += (bcoord[1]*bs2d[1]*dim[0]) + (bcoord[0]*bs2d[0]);

	if (bcoord[0] == 0) {
		blk += min[0];
		xsize = bs[0] - min[0];
	}
	else {
		voxels -= min[0];
	}
	if (bcoord[0] == max[0]/bs2d[0]) xsize -= bs2d[0] - (max[0] % bs2d[0] + 1);

	if (bcoord[1] == 0) {
		blk += min[1] * bs2d[0];
		ysize = bs2d[1] - min[1];
	}
	else {
		voxels -= dim[0] * min[1];
	}
	if (bcoord[1] == max[1]/bs2d[1]) ysize -= bs2d[1] - (max[1] % bs2d[1] + 1);

	float *voxptr = voxels;
	const float *blkptr = blk;

	for(y=0; y<ysize; y++) {

		memcpy(voxptr,blkptr,xsize*sizeof(blkptr[0]));
		blkptr += bs2d[0];
		voxptr += dim[0];
	}
}

void	WaveletBlockIOBase::Block2NonBlock(
	const float *blk, 
	const size_t bcoord[3],
	const size_t min[3],
	const size_t max[3],
	float	*voxels
) const {

	SetDiagMsg(
		"WaveletBlockIOBase::Block2NonBlock(,[%d,%d,%d],[%d,%d,%d],[%d,%d,%d])",
		bcoord[0], bcoord[1], bcoord[2],
		min[0], min[1], min[2], max[0], max[1], max[2]
	);

	size_t y,z;
	const size_t *bs = GetBlockSize();

	assert((min[0] < bs[0]) && (max[0] > min[0]) && (max[0]/bs[0] >= (bcoord[0])));
	assert((min[1] < bs[1]) && (max[1] > min[1]) && (max[1]/bs[1] >= (bcoord[1])));
	assert((min[2] < bs[2]) && (max[2] > min[2]) && (max[2]/bs[2] >= (bcoord[2])));

	size_t dim[] = {max[0]-min[0]+1, max[1]-min[1]+1, max[2]-min[2]+1};
	size_t xsize = bs[0];
	size_t ysize = bs[1];
	size_t zsize = bs[2];

	//
	// Address of first destination voxel
	//
	voxels += (bcoord[2]*bs[2] * dim[1] * dim[0]) + 
		(bcoord[1]*bs[1]*dim[0]) + 
		(bcoord[0]*bs[0]);

	if (bcoord[0] == 0) {
		blk += min[0];
		xsize = bs[0] - min[0];
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

	if (bcoord[2] == 0) {
		blk += min[2] * bs[1] * bs[0];
		zsize = bs[2] - min[2];
	}
	else {
		voxels -= dim[1] * dim[0] * min[2];
	}
	if (bcoord[2] == max[2]/bs[2]) zsize -= bs[2] - (max[2] % bs[2] + 1);

	float *voxptr;
	const float *blkptr;

	for(z=0; z<zsize; z++) {
		voxptr = voxels + (dim[0]*dim[1]*z); 
		blkptr = blk + (bs[0]*bs[1]*z);

		for(y=0; y<ysize; y++) {

			memcpy(voxptr,blkptr,xsize*sizeof(blkptr[0]));
			blkptr += bs[0];
			voxptr += dim[0];
		}
	}
}

void    WaveletBlockIOBase::GetValidRegion(
    size_t min[3], size_t max[3], int reflevel
) const {

	if (reflevel < 0) reflevel = GetNumTransforms();

	int  ldelta = GetNumTransforms() - reflevel;

	for (int i=0; i<3; i++) {
		min[i] = _validRegMin[i] >> ldelta;
		max[i] = _validRegMax[i] >> ldelta;
	}
}

void WaveletBlockIOBase::_GetValidRegion(
	size_t minreg[3], size_t maxreg[3]
) const {
	
	size_t dim[3];
	GetDim(dim, -1);

	for (int i=0; i<3; i++) {
		minreg[i] = 0;
		maxreg[i] = dim[i]-1;
	}
}
