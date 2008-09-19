//
// $Id$
//
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
	int	j;

	SetClassName("WaveletBlockIOBase");

	if (_num_reflevels > MAX_LEVELS) {
		SetErrMsg("Too many refinement levels");
		return(-1);
	}

	_xform_timer = 0.0;

	for(j=0; j<MAX_LEVELS; j++) {
		_mins[j] = NULL;
		_maxs[j] = NULL;
		_ncids[j] = -1;
		_ncvars[j] = -1;
		_ncminvars[j] = -1;
		_ncmaxvars[j] = -1;
		_ncpaths[j].clear();
		_ncoffsets[j] = 0;
	}

	_reflevel = 0;


	ntilde_c = _metadata->GetLiftingCoef();
	n_c = _metadata->GetFilterCoef();

	is_open_c = 0;

	return(0);
}

WaveletBlockIOBase::WaveletBlockIOBase(
	const Metadata	*metadata,
	unsigned int	nthreads
) : VDFIOBase(metadata, nthreads) {

	_objInitialized = 0;
	if (VDFIOBase::GetErrCode()) return;

	if (_WaveletBlockIOBase() < 0) return;

	_objInitialized = 1;
}

WaveletBlockIOBase::WaveletBlockIOBase(
	const char *metafile,
	unsigned int	nthreads
) : VDFIOBase(metafile, nthreads) {
	_objInitialized = 0;

	if (VDFIOBase::GetErrCode()) return;

	if (_WaveletBlockIOBase() < 0) return;

	_objInitialized = 1;
}

WaveletBlockIOBase::~WaveletBlockIOBase() {

	if (! _objInitialized) return;

	_objInitialized = 0;
}

int    WaveletBlockIOBase::VariableExists(
    size_t timestep,
    const char *varname,
    int reflevel
) const {
	string basename;

    if (reflevel < 0) reflevel = _num_reflevels - 1;

	if (_metadata->ConstructFullVBase(timestep, varname, &basename) < 0) {
		_metadata->SetErrCode(0);
		return (0);
	}

	for (int j = 0; j<=reflevel; j++){

		struct STAT64 statbuf;

		string path;
        mkpath(basename, j, path, _version);

		if (STAT64(path.c_str(), &statbuf) < 0) return (0);
	}
	return(1);
}

int	WaveletBlockIOBase::OpenVariableWrite(
	size_t timestep,
	const char *varname,
	int reflevel
) {
	string dir;
	string basename;

	_dataRange[0] = _dataRange[1] = 0.0;

    if (reflevel < 0) reflevel = _num_reflevels - 1;

	write_mode_c = 1;

	_timeStep = timestep;
	_varName.assign(varname);
	_reflevel = reflevel;

	WaveletBlockIOBase::CloseVariable(); // close any previously opened files

	if (reflevel >= _num_reflevels) {
		SetErrMsg("Requested refinement level out of range (%d)", reflevel);
		return(-1);
	}

	if (_metadata->ConstructFullVBase(timestep, varname, &basename) < 0) {
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
	for(int j=0; j<=_reflevel; j++) {
		string path;
		int rc;

		_ncoffsets[j] = 0;

		mkpath(basename, j, path, _version);
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

		rc = nc_def_dim(_ncids[j],_blockSizeXName.c_str(),_bs[0],&bs_dim_ids[0]);
		NC_ERR_WRITE(rc,path)

		rc = nc_def_dim(_ncids[j],_blockSizeYName.c_str(),_bs[1],&bs_dim_ids[1]);
		NC_ERR_WRITE(rc,path)

		rc = nc_def_dim(_ncids[j],_blockSizeZName.c_str(),_bs[2],&bs_dim_ids[2]);
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
		if (ncDefineDimsVars(j,path, bs_dim_ids, dim_ids) < 0) return(-1);


		//
		// Define netCDF global attributes
		//
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_fileVersionName.c_str(),NC_INT, 1, &_version
		);
		NC_ERR_WRITE(rc,path)

		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_refLevelName.c_str(),NC_INT, 1, &j
		);
		NC_ERR_WRITE(rc,path)

		int dim[] = {_dim[0], _dim[1], _dim[2]};
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_nativeResName.c_str(),NC_INT, 3, dim
		);
		NC_ERR_WRITE(rc,path)

		size_t dimj[3];
		VDFIOBase::GetDim(dimj,j);
		int dimj_int[] = {dimj[0], dimj[1], dimj[2]};
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_refLevelResName.c_str(),NC_INT, 3, dimj_int
		);
		NC_ERR_WRITE(rc,path)

		size_t minreg[3];
		size_t maxreg[3];
		VDFIOBase::GetValidRegion(minreg, maxreg,_num_reflevels-1);
		int minreg_int[] = {minreg[0], minreg[1], minreg[2]};
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_nativeMinValidRegionName.c_str(),NC_INT, 3, 
			minreg_int
		);
		NC_ERR_WRITE(rc,path)

		int maxreg_int[] = {maxreg[0], maxreg[1], maxreg[2]};
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_nativeMaxValidRegionName.c_str(),NC_INT, 3, 
			maxreg_int
		);
		NC_ERR_WRITE(rc,path)

		VDFIOBase::GetValidRegion(minreg, maxreg,j);
		int rminreg_int[] = {minreg[0], minreg[1], minreg[2]};
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_refLevMinValidRegionName.c_str(),NC_INT, 3, 
			rminreg_int
		);
		NC_ERR_WRITE(rc,path)

		int rmaxreg_int[] = {maxreg[0], maxreg[1], maxreg[2]};
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_refLevMaxValidRegionName.c_str(),NC_INT, 3, 
			rmaxreg_int
		);
		NC_ERR_WRITE(rc,path)

		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_filterCoeffName.c_str(),NC_INT, 1, &n_c
		);
		NC_ERR_WRITE(rc,path)

		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_liftingCoeffName.c_str(),NC_INT, 1, &ntilde_c
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
	int reflevel
) {
	string basename;

	write_mode_c = 0;
	_dataRange[0] = _dataRange[1] = 0.0;

	_timeStep = timestep;
	_varName.assign(varname);
	_reflevel = reflevel;

	WaveletBlockIOBase::CloseVariable(); // close previously opened files

	if (reflevel >= _num_reflevels) {
		SetErrMsg("Requested refinement level out of range (%d)", reflevel);
		return(-1);
	}

	if (!VariableExists(timestep, varname, reflevel)) {
		SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,
			"Variable \"%s\" not present at requested timestep or level",
			varname
		);
		return(-1);
	}

	if (_metadata->ConstructFullVBase(timestep, varname, &basename) < 0) {
		_metadata->SetErrCode(0);
		return (0);
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

	for(int i=0; i<3; i++) {
		_validRegMin[i] = 0;
		_validRegMax[i] = _dim[i]-1;
	}

	for(int j=0; j<=_reflevel; j++) {

		int	ncdimid;
		size_t	ncdim;
		string path;
		int	rc;


		_ncoffsets[j] = 0;

		mkpath(basename, j, path, _version);
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
		if (ncdim != _bs[0]) {
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
		if (ncdim != _bs[1]) {
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
		if (ncdim != _bs[2]) {
			SetErrMsg("Metadata file and data file mismatch");
			return(-1);
		}

		// Call child class method to verify dimensions and variables
		//
		if (ncVerifyDimsVars(j,path) < 0) return(-1);

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
		if (_ncminvars[j] != -1) {
			rc = nc_get_var_float(_ncids[j], _ncminvars[j], _mins[j]);
			NC_ERR_READ(rc,path)
		}

		if (_ncmaxvars[j] != -1) {
			rc = nc_get_var_float(_ncids[j], _ncmaxvars[j], _maxs[j]);
			NC_ERR_READ(rc,path)
		}
	}
	return(0);
}

int	WaveletBlockIOBase::CloseVariable()
{
	int	j;


	if (! is_open_c) return(0);

	TIMER_START(t0)
	for(j=0; j<MAX_LEVELS; j++) {

		if (_ncids[j] > -1) {
			int rc; 

			if (write_mode_c) {
				rc = nc_put_att_float(
					_ncids[j], NC_GLOBAL, _scalarRangeName.c_str(), 
					NC_FLOAT, 2,_dataRange
				);
				NC_ERR_WRITE(rc,_ncpaths[j])

				size_t minreg[3];
				size_t maxreg[3];
				VDFIOBase::GetValidRegion(minreg, maxreg,_num_reflevels-1);
				int minreg_int[] = {minreg[0], minreg[1], minreg[2]};
				rc = nc_put_att_int(
					_ncids[j],NC_GLOBAL,_nativeMinValidRegionName.c_str(),
					NC_INT, 3, minreg_int
				);
				NC_ERR_WRITE(rc,_ncpaths[j])

				int maxreg_int[] = {maxreg[0], maxreg[1], maxreg[2]};
				rc = nc_put_att_int(
					_ncids[j],NC_GLOBAL,_nativeMaxValidRegionName.c_str(),
					NC_INT, 3, maxreg_int
				);
				NC_ERR_WRITE(rc,_ncpaths[j])

				VDFIOBase::GetValidRegion(minreg, maxreg,j);
				int rminreg_int[] = {minreg[0], minreg[1], minreg[2]};
				rc = nc_put_att_int(
					_ncids[j],NC_GLOBAL,_refLevMinValidRegionName.c_str(),
					NC_INT, 3, rminreg_int
				);
				NC_ERR_WRITE(rc,_ncpaths[j])

				int rmaxreg_int[] = {maxreg[0], maxreg[1], maxreg[2]};
				rc = nc_put_att_int(
					_ncids[j],NC_GLOBAL,_refLevMaxValidRegionName.c_str(),
					NC_INT, 3, rmaxreg_int
				);
				NC_ERR_WRITE(rc,_ncpaths[j])

				rc = nc_put_var_float(_ncids[j], _ncminvars[j], _mins[j]);
				NC_ERR_WRITE(rc,_ncpaths[j])

				rc = nc_put_var_float(_ncids[j], _ncmaxvars[j], _maxs[j]);
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
		TIMER_STOP(t0, _write_timer)
	}
	else {
		TIMER_STOP(t0, _read_timer)
	}
				
	is_open_c = 0;

	return(0);
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
