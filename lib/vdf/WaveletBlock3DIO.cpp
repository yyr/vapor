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

int    VDF_API mkdirhier(const string &dir);
void	VDF_API mkpath(const string &basename, int level, string &path, int v);
void	VDF_API dirname(const string &path, string &dir);

const string WaveletBlock3DIO::_blockSizeXName = "BlockSizeNx";
const string WaveletBlock3DIO::_blockSizeYName = "BlockSizeNy";
const string WaveletBlock3DIO::_blockSizeZName = "BlockSizeNz";
const string WaveletBlock3DIO::_nBlocksDimName = "NumCoefBlocks";
const string WaveletBlock3DIO::_blockDimXName = "BlockDimNx";
const string WaveletBlock3DIO::_blockDimYName = "BlockDimNy";
const string WaveletBlock3DIO::_blockDimZName = "BlockDimNz";
const string WaveletBlock3DIO::_fileVersionName = "FileVersion";
const string WaveletBlock3DIO::_refLevelName = "RefinementLevel";
const string WaveletBlock3DIO::_nativeResName = "NativeResolution";
const string WaveletBlock3DIO::_refLevelResName = "refinementLevelResolution";
const string WaveletBlock3DIO::_filterCoeffName = "NumFilterCoeff";
const string WaveletBlock3DIO::_liftingCoeffName = "NumLiftingCoeff";
const string WaveletBlock3DIO::_scalarRangeName = "ScalarDataRange";
const string WaveletBlock3DIO::_minsName = "BlockMinValues";
const string WaveletBlock3DIO::_maxsName = "BlockMaxValues";
const string WaveletBlock3DIO::_lambdaName = "LambdaCoefficients";
const string WaveletBlock3DIO::_gammaName = "GammaCoefficients";

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


int	WaveletBlock3DIO::_WaveletBlock3DIO(
) {
	int	j;

	SetClassName("WaveletBlock3DIO");

	if (_num_reflevels > MAX_LEVELS) {
		SetErrMsg("Too many refinement levels");
		return(-1);
	}

	_xform_timer = 0.0;

	for(j=0; j<MAX_LEVELS; j++) {
		file_ptrs_c[j] = NULL;
		_mins[j] = NULL;
		_maxs[j] = NULL;
		_fileOffsets[j] = 0;
		_ncids[j] = -1;
		_ncvars[j] = -1;
		_ncminvars[j] = -1;
		_ncmaxvars[j] = -1;
		_ncpaths[j].clear();
		_ncoffsets[j] = 0;
	}

	super_block_c = NULL;
	block_c = NULL;
	wb3d_c = NULL;


	ntilde_c = _metadata->GetLiftingCoef();
	n_c = _metadata->GetFilterCoef();
	_msbFirst = _metadata->GetMSBFirst();

	// Backwords compatibility for pre version 1 files
	//
	if (_version < 1) {
		for (j=0; j<_num_reflevels; j++) {
			_fileOffsets[j] = get_file_offset(j);
		}
	}

	if (this->my_alloc() < 0) {
		this->my_free();
		return (-1);
	}

	is_open_c = 0;

	return(0);
}

WaveletBlock3DIO::WaveletBlock3DIO(
	Metadata	*metadata,
	unsigned int	nthreads
) : VDFIOBase(metadata, nthreads) {

	_objInitialized = 0;
	if (VDFIOBase::GetErrCode()) return;

	if (_WaveletBlock3DIO() < 0) return;

	_objInitialized = 1;
}

WaveletBlock3DIO::WaveletBlock3DIO(
	const char *metafile,
	unsigned int	nthreads
) : VDFIOBase(metafile, nthreads) {
	_objInitialized = 0;

	if (VDFIOBase::GetErrCode()) return;

	if (_WaveletBlock3DIO() < 0) return;

	_objInitialized = 1;
}

WaveletBlock3DIO::~WaveletBlock3DIO() {

	if (! _objInitialized) return;

	my_free();

	_objInitialized = 0;
}

int    WaveletBlock3DIO::VariableExists(
    size_t timestep,
    const char *varname,
    int reflevel
) {
	string basename;

    if (reflevel < 0) reflevel = _num_reflevels - 1;

	const string &bp = _metadata->GetVBasePath(timestep, varname);
	if (_metadata->GetErrCode() != 0 || bp.length() == 0) {
		_metadata->SetErrCode(0);
		return (0);
	}

	// Path to variable file is relative to xml file, if it exists
	if (_metadata->GetParentDir() && bp[0] != '/') {
		basename.append(_metadata->GetParentDir());
		basename.append("/");
	}

	// Path to variable file is relative to xml file, if it exists
	if (_metadata->GetMetafileName() && bp[0] != '/') {
		string s = _metadata->GetMetafileName();
		string t;
		size_t p = s.find_first_of(".");
		if (p != string::npos) {
			t = s.substr(0, p);
		}
		else {
			t = s;
		}
		basename.append(t);
		basename.append("_data");
		basename.append("/");

    }
	
	basename.append(bp);

	for (int j = 0; j<=reflevel; j++){
#ifdef WIN32
		struct _stat statbuf;
#else
		struct stat64 statbuf;
#endif
		string path;
        mkpath(basename, j, path, _version);

#ifndef WIN32
		if (stat64(path.c_str(), &statbuf) < 0) return(0);
#else
		if (_stat(path.c_str(), &statbuf) < 0) return (0);
#endif
	}
	return(1);
}

int	WaveletBlock3DIO::OpenVariableWrite(
	size_t timestep,
	const char *varname,
	int reflevel
) {
	string dir;
	int	min;
	string basename;

	_dataRange[0] = _dataRange[1] = 0.0;

    if (reflevel < 0) reflevel = _num_reflevels - 1;

	write_mode_c = 1;

	_timeStep = timestep;
	_varName.assign(varname);
	_reflevel = reflevel;

	WaveletBlock3DIO::CloseVariable(); // close any previously opened files

	min = Min((int)_bdim[0],(int) _bdim[1]);
	min = Min((int)min, (int)_bdim[2]);

	if (reflevel >= _num_reflevels) {
		SetErrMsg("Requested refinement level out of range (%d)", reflevel);
		return(-1);
	}

	if (LogBaseN(min, 2.0) < 0) {
		SetErrMsg("Too many levels (%d) in transform ", reflevel);
		return(-1);
	}

	const string &bp = _metadata->GetVBasePath(timestep, varname);
	if (_metadata->GetErrCode() != 0 || bp.length() == 0) {
		SetErrMsg(
			"Failed to find variable \"%s\" in metadata object at time step %d",
			varname, (int) timestep
		);
		return(-1);
	}

	// Path to variable file is relative to xml file, if it exists
	if (_metadata->GetParentDir() && bp[0] != '/') {
		basename.append(_metadata->GetParentDir());
		basename.append("/");
	}

	// Path to variable file is relative to xml file, if it exists
	if (_metadata->GetMetafileName() && bp[0] != '/') {
		string s = _metadata->GetMetafileName();
		string t;
		size_t p = s.find_first_of(".");
		if (p != string::npos) {
			t = s.substr(0, p);
		}
		else {
			t = s;
		}
		basename.append(t);
		basename.append("_data");
		basename.append("/");

    }

	basename.append(bp);

	dirname(basename, dir);
	if (mkdirhier(dir) < 0) return(-1);
	

	int rc = open_var_write(basename);
	if (rc<0) return(-1);


	is_open_c = 1;

	return(0);
}


int WaveletBlock3DIO::open_var_write(
	const string &basename
) {

	if (_version < 2) {
		for(int j=0; j<=_reflevel; j++) {
			string path;

			mkpath(basename, j, path, _version);
			_ncpaths[j] = path;

#ifdef	WIN32
			file_ptrs_c[j] = fopen(path.c_str(), "wb");
#else
			file_ptrs_c[j] = fopen64(path.c_str(), "wb");
#endif
			if (! file_ptrs_c[j]) {
				SetErrMsg("fopen(%s, wb) : %s", path.c_str(), strerror(errno));
				return(-1);
			}
		}
		return(0);
	}


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
		int bx_dim, by_dim, bz_dim;

		rc = nc_def_dim(_ncids[j], _blockSizeXName.c_str(), _bs[0], &bx_dim);
		NC_ERR_WRITE(rc,path)

		rc = nc_def_dim(_ncids[j], _blockSizeYName.c_str(), _bs[1], &by_dim);
		NC_ERR_WRITE(rc,path)

		rc = nc_def_dim(_ncids[j], _blockSizeZName.c_str(), _bs[2], &bz_dim);
		NC_ERR_WRITE(rc,path)
			
		size_t bdim[3];

		int nb;
		if (j==0) {
			GetDimBlk(bdim,j);
			nb = bdim[0]*bdim[1]*bdim[2];
		}
		else {
			GetDimBlk(bdim,j-1);
			nb = bdim[0]*bdim[1]*bdim[2] * 7;
		}

		int nblk_dim;
		rc = nc_def_dim(_ncids[j], _nBlocksDimName.c_str(), nb, &nblk_dim);
		NC_ERR_WRITE(rc,path)

		int nbx_dim, nby_dim, nbz_dim;

		GetDimBlk(bdim,j);
		rc = nc_def_dim(_ncids[j], _blockDimXName.c_str(), bdim[0], &nbx_dim);
		NC_ERR_WRITE(rc,path)

		rc = nc_def_dim(_ncids[j], _blockDimYName.c_str(), bdim[1], &nby_dim);
		NC_ERR_WRITE(rc,path)

		rc = nc_def_dim(_ncids[j], _blockDimZName.c_str(), bdim[2], &nbz_dim);
		NC_ERR_WRITE(rc,path)


		//
		// Define netCDF variables
		//
		{
			int dim_ids[] = {nbz_dim, nby_dim, nbx_dim};
			rc = nc_def_var(
				_ncids[j], _minsName.c_str(), NC_FLOAT, 
				sizeof(dim_ids)/sizeof(dim_ids[0]), dim_ids, &_ncminvars[j]
			);
			NC_ERR_WRITE(rc,path)

			rc = nc_def_var(
				_ncids[j], _maxsName.c_str(), NC_FLOAT, 
				sizeof(dim_ids)/sizeof(dim_ids[0]), dim_ids, &_ncmaxvars[j]
			);
			NC_ERR_WRITE(rc,path)
		}

		{
			int dim_ids[] = {nblk_dim, bz_dim, by_dim, bx_dim};
			if (j==0) {
				rc = nc_def_var(
					_ncids[j], _lambdaName.c_str(), NC_FLOAT, 
					sizeof(dim_ids)/sizeof(dim_ids[0]), dim_ids, &_ncvars[j]
				);
			}
			else {
				rc = nc_def_var(
					_ncids[j], _gammaName.c_str(), NC_FLOAT, 
					sizeof(dim_ids)/sizeof(dim_ids[0]), dim_ids, &_ncvars[j]
				);
			}
			NC_ERR_WRITE(rc,path)
		}

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
		GetDim(dimj,j);
		int dimj_int[] = {dimj[0], dimj[1], dimj[2]};
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_refLevelResName.c_str(),NC_INT, 3, dimj_int
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

int	WaveletBlock3DIO::OpenVariableRead(
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

	WaveletBlock3DIO::CloseVariable(); // close previously opened files

	if (reflevel >= _num_reflevels) {
		SetErrMsg("Requested refinement level out of range (%d)", reflevel);
		return(-1);
	}

	if (!VariableExists(timestep, varname, reflevel)) {
		SetErrMsg(
			"Variable \"%s\" not present at requested timestep or level",
			varname
		);
		return(-1);
	}


	const string &bp = _metadata->GetVBasePath(timestep, varname);
	if (_metadata->GetErrCode() != 0 || bp.length() == 0) {
		SetErrMsg(
			"Failed to find variable \"%s\" in metadata object at time step %d",
			varname, (int) timestep
		);
		return(-1);
	}

	// Path to variable file is relative to xml file, if it exists
	if (_metadata->GetParentDir() && bp[0] != '/') {
		basename.append(_metadata->GetParentDir());
		basename.append("/");
	}

	// Path to variable file is relative to xml file, if it exists
	if (_metadata->GetMetafileName() && bp[0] != '/') {
		string s = _metadata->GetMetafileName();
		string t;
		size_t p = s.find_first_of(".");
		if (p != string::npos) {
			t = s.substr(0, p);
		}
		else {
			t = s;
		}
		basename.append(t);
		basename.append("_data");
		basename.append("/");

    }

	basename.append(bp);

	int rc = open_var_read(basename);
	if (rc<0) return(-1);


	is_open_c = 1;

	return(0);
}



int WaveletBlock3DIO::open_var_read(
	const string &basename
) {

	if (_version < 2) {
		for(int j=0; j<=_reflevel; j++) {
			string path;

			mkpath(basename, j, path, _version);
			_ncpaths[j] = path;

#ifdef	WIN32
			file_ptrs_c[j] = fopen(path.c_str(), "rb");
#else
			file_ptrs_c[j] = fopen64(path.c_str(), "rb");
#endif
			if (! file_ptrs_c[j]) {
				if (errno != ENOENT) {
					SetErrMsg("fopen(%s, rb) : %s", path.c_str(), strerror(errno));
					return(-1);
				}
				else {
					break;
				}
			}
			if (_fileOffsets[j]) {	// seek to start of data
				int rc = fseek(file_ptrs_c[j], _fileOffsets[j], SEEK_SET);
				if (rc<0) {
					SetErrMsg("fseek(%d) : %s",_fileOffsets[j],strerror(errno));
					return(-1);
				}
			}
		}
		return(0);
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
		rc = nc__open(path.c_str(), NC_NOWRITE, &chsz, &_ncids[j]);
		NC_ERR_READ(rc,path)

		//
		// Verify coefficient file metadata matches VDF metadata
		//
		rc = nc_inq_dimid(_ncids[j], _blockSizeXName.c_str(), &ncdimid);
		NC_ERR_READ(rc,path)
		rc = nc_inq_dimlen(_ncids[j], ncdimid, &ncdim);
		NC_ERR_READ(rc,path)
		if (ncdim != _bs[0]) {
			SetErrMsg("Metadata file and data file mismatch");
			return(-1);
		}

		rc = nc_inq_dimid(_ncids[j], _blockSizeYName.c_str(), &ncdimid);
		NC_ERR_READ(rc,path)
		rc = nc_inq_dimlen(_ncids[j], ncdimid, &ncdim);
		NC_ERR_READ(rc,path)
		if (ncdim != _bs[1]) {
			SetErrMsg("Metadata file and data file mismatch");
			return(-1);
		}

		rc = nc_inq_dimid(_ncids[j], _blockSizeZName.c_str(), &ncdimid);
		NC_ERR_READ(rc,path)
		rc = nc_inq_dimlen(_ncids[j], ncdimid, &ncdim);
		NC_ERR_READ(rc,path)
		if (ncdim != _bs[2]) {
			SetErrMsg("Metadata file and data file mismatch");
			return(-1);
		}

		size_t bdim[3];
		int nb;

		if (j==0) {
			GetDimBlk(bdim,j);
			nb = bdim[0]*bdim[1]*bdim[2];
		}
		else {
			GetDimBlk(bdim,j-1);
			nb = bdim[0]*bdim[1]*bdim[2] * 7;
		}

		rc = nc_inq_dimid(_ncids[j], _nBlocksDimName.c_str(), &ncdimid);
		NC_ERR_READ(rc,path)
		rc = nc_inq_dimlen(_ncids[j], ncdimid, &ncdim);
		NC_ERR_READ(rc,path)
		if (ncdim != nb) {
			SetErrMsg("Metadata file and data file mismatch");
			return(-1);
		}


		rc = nc_get_att_float(
			_ncids[j],NC_GLOBAL,_scalarRangeName.c_str(),_dataRange
		);
		NC_ERR_READ(rc,path)

		rc = nc_inq_varid(_ncids[j], _minsName.c_str(), &_ncminvars[j]);
		NC_ERR_READ(rc,path)

		rc = nc_inq_varid(_ncids[j], _maxsName.c_str(), &_ncmaxvars[j]);
		NC_ERR_READ(rc,path)

		if (j==0) {
			rc = nc_inq_varid(_ncids[j], _lambdaName.c_str(), &_ncvars[j]);
		}
		else {
			rc = nc_inq_varid(_ncids[j], _gammaName.c_str(), &_ncvars[j]);
		}
		NC_ERR_READ(rc,path)


		// Go ahead and read the block min & max
		//
		rc = nc_get_var_float(_ncids[j], _ncminvars[j], _mins[j]);
		NC_ERR_READ(rc,path)

		rc = nc_get_var_float(_ncids[j], _ncmaxvars[j], _maxs[j]);
		NC_ERR_READ(rc,path)

	}
	return(0);
}

int	WaveletBlock3DIO::CloseVariable()
{
	int	j;


	if (! is_open_c) return(0);

	if (_version < 2) {
		for(j=0; j<MAX_LEVELS; j++) {
			if (file_ptrs_c[j]) (void) fclose (file_ptrs_c[j]);
			file_ptrs_c[j] = NULL;
		}
	}
	else {
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
	}
				
	is_open_c = 0;

	return(0);
}


int	WaveletBlock3DIO::GetBlockMins(
	const float	**mins,
	int reflevel
) {
    if (reflevel < 0) reflevel = _num_reflevels - 1;

	if (reflevel >= _num_reflevels) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}

	*mins = _mins[reflevel];
	return(0);
}

int	WaveletBlock3DIO::GetBlockMaxs(
	const float	**maxs,
	int reflevel
) {
    if (reflevel < 0) reflevel = _num_reflevels - 1;

	if (reflevel >= _num_reflevels) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}

	*maxs = _maxs[reflevel];
	return(0);
}



int	WaveletBlock3DIO::seekBlocks(
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

	if (_version < 2) {

		TIMER_START(t0);

		long long byteoffset = offset * _block_size * sizeof(float);
		byteoffset += _fileOffsets[reflevel];
		FILE	*fp = file_ptrs_c[reflevel];
		int	rc;

#ifdef	WIN32
		//Note: win32 won't seek beyond 32 bits
		rc = fseek(fp,  byteoffset, SEEK_SET);
#else
#if     defined(Linux) || defined(AIX)
		rc = fseeko64(fp, byteoffset, SEEK_SET);
#else
		rc = fseek64(fp, byteoffset, SEEK_SET);
#endif
#endif
		if (rc<0) {
			SetErrMsg("fseek(%lld) : %s",(long long) byteoffset,strerror(errno));
			return(-1);
		}

		TIMER_STOP(t0, _seek_timer);
	}
	else {
		_ncoffsets[reflevel] = offset;
	}

	return(0);
}

int	WaveletBlock3DIO::seekLambdaBlocks(
	const size_t bcoord[3]
) {

	unsigned int	offset;
	size_t bdim[3];

	GetDimBlk(bdim, 0);

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

	GetDimBlk(bdim, reflevel-1);

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
	unsigned long    lsbFirstTest = 1;


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



	if (_version < 2) {

		FILE	*fp = file_ptrs_c[reflevel];
		TIMER_START(t0)

		rc = (int)fread(blks, sizeof(blks[0]), (_block_size*n), fp);
		if (rc != _block_size*n) {
			SetErrMsg("fread(%d) : %s",_block_size*n,strerror(errno));
			return(-1);
		}
		TIMER_STOP(t0, _read_timer)

		// Deal with endianess
		//
		if ((*(char *) &lsbFirstTest && _msbFirst) || 
			(! *(char *) &lsbFirstTest && ! _msbFirst)) {

			swapbytescopy(blks, blks, sizeof(blks[0]), _block_size*n);
		}
	}
	else {
		int rc;
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
	}

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
	FILE	*fp;
	unsigned long lsbFirstTest = 1;

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

	if (_version < 2) {

		fp = file_ptrs_c[reflevel];


		// Deal with endianess
		//
		if ((*(char *) &lsbFirstTest && _msbFirst) || 
			(! *(char *) &lsbFirstTest && ! _msbFirst)) {
			int	i;

			for(i=0;i<(int)n;i++) {
				swapbytescopy(
					&blks[i*_block_size], block_c, sizeof(blks[0]), _block_size
				);

				TIMER_START(t0)
				rc = (int)fwrite(block_c, sizeof(block_c[0]), _block_size, fp);
				if (rc != _block_size) {
					SetErrMsg("fwrite(%d) : %s",_block_size*n,strerror(errno));
					return(-1);
				}
				TIMER_STOP(t0, _write_timer)
			}
		}
		else {
			TIMER_START(t0)
			rc = (int)fwrite(blks, sizeof(blks[0]), _block_size*n, fp);
			if (rc != _block_size*n) {
				SetErrMsg("fwrite(%d) : %s",_block_size*n,strerror(errno));
				return(-1);
			}
			TIMER_STOP(t0, _write_timer)
		}
	}
	else {
		int rc;

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
	}

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
	unsigned long	lsbFirstTest = 1;
	int	j;


	// alloc space from coarsest (j==0) to finest level
	for(j=0; j<_num_reflevels; j++) {
		size_t nb_j[3];

		GetDimBlk(nb_j, j);

		size = (int)(nb_j[0] * nb_j[1] * nb_j[2]);

		_mins[j] = new float[size];
		_maxs[j] = new float[size];
		if (! _mins[j] || ! _maxs[j]) {
			SetErrMsg("new float[%d] : %s", size, strerror(errno));
			return(-1);
		}
	}

	size = _block_size * 8;
	super_block_c = new float[size];
	if (! super_block_c) {
		SetErrMsg("new float[%d] : %s", size, strerror(errno));
		return(-1);
	}

	// Deal with endianess
	//
	if ((*(char *) &lsbFirstTest && _msbFirst) || 
		(! *(char *) &lsbFirstTest && ! _msbFirst)) {

		block_c = new float[_block_size];
		if (! block_c) {
			SetErrMsg("new float[%d] : %s", _block_size, strerror(errno));
			return(-1);
		}
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
	if (block_c) delete [] block_c;
	if (wb3d_c) delete wb3d_c;

	super_block_c = NULL;
	block_c = NULL;
	wb3d_c = NULL;
}



void    WaveletBlock3DIO::swapbytes(
        void    *vptr,
        int     n
) const {
        unsigned char   *ucptr = (unsigned char *) vptr;
        unsigned char   uc;
        int             i;

        for (i=0; i<n/2; i++) {
                uc = ucptr[i];
                ucptr[i] = ucptr[n-i-1];
                ucptr[n-i-1] = uc;
        }
}

void    WaveletBlock3DIO::swapbytescopy(
        const void    *src,
        void    *dst,
        int	size,
		int	n
) const {
	const unsigned char   *ucsrc = (unsigned char *) src;
	unsigned char   *ucdst = (unsigned char *) dst;
	unsigned char   uc;
	int             i,j;

	for(j=0;j<n;j++) {
		for (i=0; i<size/2; i++) {
			uc = ucsrc[i];
			ucdst[i] = ucsrc[size-i-1];
			ucdst[size-i-1] = uc;
		}
		ucsrc += size;
		ucdst += size;
	}
}

int	WaveletBlock3DIO::get_file_offset(
	size_t level
) {
	int	fixed_part;
	int	var_part;
	size_t bdim[3];
	const int BLOCK_FACTOR = 8192;
	const int STATIC_HEADER_SIZE = 512;

	int	minsize;

	fixed_part = STATIC_HEADER_SIZE;

	GetDimBlk(bdim, level);

	minsize = bdim[0] * bdim[1] * bdim[2] * sizeof(float) * 2;

	for(var_part = -fixed_part; var_part<(minsize + fixed_part); var_part += BLOCK_FACTOR);

	return(fixed_part + var_part);
}

int    mkdirhier(const string &dir) {

    stack <string> dirs;

    string::size_type idx;
    string s = dir;

	dirs.push(s);
    while ((idx = s.find_last_of("/")) != string::npos) {
        s = s.substr(0, idx);
		if (! s.empty()) dirs.push(s);
    }

    while (! dirs.empty()) {
        s = dirs.top();
		dirs.pop();
#ifndef WIN32
        if ((mkdir(s.c_str(), 0777) < 0) && errno != EEXIST) {
			MyBase::SetErrMsg("mkdir(%s) : %M", s.c_str());
            return(-1);
        }
#else 
		//Windows version of mkdir:
		//If it succeeds, return value is nonzero
		if (!CreateDirectory(( LPCSTR)s.c_str(), 0)){
			DWORD dw = GetLastError();
			if (dw != 183){ //183 means file already exists
				MyBase::SetErrMsg("mkdir(%s) : %M", s.c_str());
				return(-1);
			}
		}
#endif
    }
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

void    dirname(const string &path, string &dir) {
	
	string::size_type idx = path.find_last_of('/');
	if (idx == string::npos) {
		dir.assign("./");
	}
	else {
		dir = path.substr(0, idx+1);
	}
}


