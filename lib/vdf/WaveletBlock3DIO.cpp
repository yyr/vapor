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

const string WaveletBlock3DIO::_blockDimXName = "BlockNx";
const string WaveletBlock3DIO::_blockDimYName = "BlockNy";
const string WaveletBlock3DIO::_blockDimZName = "BlockNz";
const string WaveletBlock3DIO::_nBlocksDimName = "NumBlocks";
const string WaveletBlock3DIO::_fileVersionName = "FileVersion";
const string WaveletBlock3DIO::_refLevelName = "RefinementLevel";
const string WaveletBlock3DIO::_nativeResName = "NativeResolution";
const string WaveletBlock3DIO::_refLevelResName = "refinementLevelResolution";
const string WaveletBlock3DIO::_filterCoeffName = "NumFilterCoeff";
const string WaveletBlock3DIO::_liftingCoeffName = "NumLiftingCoeff";
const string WaveletBlock3DIO::_scalarRangeName = "ScalarDataRange";
const string WaveletBlock3DIO::_lambdaName = "Lambda";
const string WaveletBlock3DIO::_gammaName = "Gamma";


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
		mins_c[j] = NULL;
		maxs_c[j] = NULL;
		_fileOffsets[j] = 0;
		_ncfiles[j] = NULL;
		_ncvars[j] = NULL;
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

	this->VAPoR::VDFIOBase::~VDFIOBase();

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
			"Failed to find variable \"%s\" in metadata object", 
			varname
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


	for(int j=0; j<=_reflevel; j++) {
		string path;
		int e;

		_ncoffsets[j] = 0;

		mkpath(basename, j, path, _version);

		NcError ncerror(NcError::silent_nonfatal);

		_ncfiles[j] = new NcFile(path.c_str(), NcFile::Replace);
		if (! _ncfiles[j]->is_valid()) {
			SetErrMsg("Failed to create NCDF file \"%s\"", path.c_str());
			return(-1);
		}

		_ncfiles[j]->set_fill(NcFile::NoFill);
		if ((e = ncerror.get_err()) != NC_NOERR) {
			SetErrMsg("NcFile::set_fill() : %s", nc_strerror(e));
			return(-1);
		}

		NcDim *bx_dim = _ncfiles[j]->add_dim(_blockDimXName.c_str(), _bs[0]);
		NcDim *by_dim = _ncfiles[j]->add_dim(_blockDimYName.c_str(), _bs[1]);
		NcDim *bz_dim = _ncfiles[j]->add_dim(_blockDimZName.c_str(), _bs[2]);
		if ((e = ncerror.get_err()) != NC_NOERR) {
			SetErrMsg("NcFile::add_dim() : %s", nc_strerror(e));
			return(-1);
		}

		size_t bdim[3];
		GetDimBlk(bdim,j);
		int nb = bdim[0]*bdim[1]*bdim[2];

		if (j>0) nb *= 7;
		NcDim *nblk_dim = _ncfiles[j]->add_dim(_nBlocksDimName.c_str(), nb);
		if ((e = ncerror.get_err()) != NC_NOERR) {
			SetErrMsg("NcFile::add_dim() : %s", nc_strerror(e));
			return(-1);
		}

		_ncfiles[j]->add_att(_fileVersionName.c_str(), _version);

		_ncfiles[j]->add_att(_refLevelName.c_str(), j);

		int dim[] = {_dim[0], _dim[1], _dim[2]};
		_ncfiles[j]->add_att(_nativeResName.c_str(), 3, dim);

		size_t dimj[3];
		GetDim(dimj,j);
		int dimj_int[] = {dimj[0], dimj[1], dimj[2]};
		_ncfiles[j]->add_att(_refLevelResName.c_str(), 3, dimj_int);

		_ncfiles[j]->add_att(_filterCoeffName.c_str(), n_c);
		_ncfiles[j]->add_att(_liftingCoeffName.c_str(), ntilde_c);

		_ncfiles[j]->add_att(_scalarRangeName.c_str(), 2, _dataRange);

		if ((e = ncerror.get_err()) != NC_NOERR) {
			SetErrMsg("NcFile::add_att() : %s", nc_strerror(e));
			return(-1);
		}

		if (j==0) {
			_ncvars[j] = _ncfiles[j]->add_var(
				_lambdaName.c_str(), ncFloat, nblk_dim, bz_dim, by_dim, bx_dim
			);
		}
		else {
			_ncvars[j] = _ncfiles[j]->add_var(
				_gammaName.c_str(), ncFloat, nblk_dim, bz_dim, by_dim, bx_dim
			);
		}
		if ((e = ncerror.get_err()) != NC_NOERR) {
			SetErrMsg("NcFile::add_var() : %s", nc_strerror(e));
			return(-1);
		}
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
			"Failed to find variable \"%s\" in metadata object", 
			varname
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

#define	NC_ERR_CHK(f) \
	{ \
	int e; \
	if ((e = ncerror.get_err()) != NC_NOERR) { \
		SetErrMsg("Error reading netcdf file %s : %s",f.c_str(),nc_strerror(e)); \
		return(-1); \
	}} 

int WaveletBlock3DIO::open_var_read(
	const string &basename
) {

	if (_version < 2) {
		for(int j=0; j<=_reflevel; j++) {
			string path;

			mkpath(basename, j, path, _version);

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

		NcAtt	*ncatt;
		NcDim	*ncdim;
		string path;

		_ncoffsets[j] = 0;

		mkpath(basename, j, path, _version);

		NcError ncerror(NcError::silent_nonfatal);

		_ncfiles[j] = new NcFile(path.c_str(), NcFile::ReadOnly);
		if (! _ncfiles[j]->is_valid()) {
			SetErrMsg("Failed to open NCDF file \"%s\"", path.c_str());
			return(-1);
		}

		//
		// Verify coefficient file metadata matches VDF metadata
		//
		
		ncdim = _ncfiles[j]->get_dim(_blockDimXName.c_str());
		NC_ERR_CHK(path)
		if (ncdim->size() != _bs[0]) {
			SetErrMsg("Metadata file and data file mismatch");
			return(-1);
		}

		ncdim = _ncfiles[j]->get_dim(_blockDimYName.c_str());
		NC_ERR_CHK(path)
		if (ncdim->size() != _bs[1]) {
			SetErrMsg("Metadata file and data file mismatch");
			return(-1);
		}

		ncdim = _ncfiles[j]->get_dim(_blockDimZName.c_str());
		NC_ERR_CHK(path)
		if (ncdim->size() != _bs[2]) {
			SetErrMsg("Metadata file and data file mismatch");
			return(-1);
		}

		size_t bdim[3];
		GetDimBlk(bdim,j);
		int nb = bdim[0]*bdim[1]*bdim[2];

		if (j>0) nb *= 7;

		NcDim *nblk_dim = _ncfiles[j]->get_dim(_nBlocksDimName.c_str());
		NC_ERR_CHK(path)
		if (nb != nblk_dim->size()) {
			SetErrMsg("Metadata file and data file mismatch");
			return(-1);
		}


		ncatt = _ncfiles[j]->get_att(_fileVersionName.c_str());
		NC_ERR_CHK(path)

		ncatt = _ncfiles[j]->get_att(_refLevelName.c_str());
		NC_ERR_CHK(path)
		if (ncatt->as_int(0) != j) {
			SetErrMsg("Metadata file and data file mismatch");
			return(-1);
		}

		ncatt = _ncfiles[j]->get_att(_nativeResName.c_str());
		NC_ERR_CHK(path)
		for(int i = 0; i<3; i++) {
			if (ncatt->as_int(i) != _dim[i]) {
				SetErrMsg("Metadata file and data file mismatch");
				return(-1);
			}
		}

		size_t dimj[3];
		GetDim(dimj,j);

		ncatt = _ncfiles[j]->get_att(_refLevelResName.c_str());
		NC_ERR_CHK(path)
		for(int i = 0; i<3; i++) {
			if (ncatt->as_int(i) != dimj[i]) {
				SetErrMsg("Metadata file and data file mismatch");
				return(-1);
			}
		}

		ncatt = _ncfiles[j]->get_att(_filterCoeffName.c_str());
		NC_ERR_CHK(path)
		if (ncatt->as_int(0) != n_c) {
			SetErrMsg("Metadata file and data file mismatch");
			return(-1);
		}

		ncatt = _ncfiles[j]->get_att(_liftingCoeffName.c_str());
		NC_ERR_CHK(path)
		if (ncatt->as_int(0) != ntilde_c) {
			SetErrMsg("Metadata file and data file mismatch");
			return(-1);
		}

		ncatt = _ncfiles[j]->get_att(_scalarRangeName.c_str());
		NC_ERR_CHK(path)
		for (int i=0; i<2; i++) {
			_dataRange[i] = ncatt->as_float(i);
		}

		if (j==0) {
			_ncvars[j] = _ncfiles[j]->get_var(_lambdaName.c_str());
		}
		else {
			_ncvars[j] = _ncfiles[j]->get_var(_gammaName.c_str());
		}
		NC_ERR_CHK(path)

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

			if (_ncfiles[j]) {
				if (write_mode_c) {
					_ncfiles[j]->add_att(_scalarRangeName.c_str(),2,_dataRange);
					_ncfiles[j]->sync();
				}
				delete _ncfiles[j];
			}
			_ncfiles[j] = NULL;
			_ncvars[j] = NULL;
			_ncoffsets[j] = 0;
		}
		if (write_mode_c) TIMER_STOP(t0, _write_timer)
		else TIMER_STOP(t0, _read_timer)
	}
				
	is_open_c = 0;

	return(0);
}


int	WaveletBlock3DIO::GetBlockMins(
	const float	**mins,
	int reflevel
) {
	if (reflevel >= _num_reflevels) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}

	*mins = mins_c[reflevel];
	return(0);
}

int	WaveletBlock3DIO::GetBlockMaxs(
	const float	**maxs,
	int reflevel
) {
	if (reflevel >= _num_reflevels) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}

	*maxs = maxs_c[reflevel];
	return(0);
}



int	WaveletBlock3DIO::seekBlocks(
	unsigned int offset, 
	int reflevel
) {
	//cerr << reflevel << " seeking " << offset << " blocks" << endl;

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
		rc = fseek(fp, (long) byteoffset, SEEK_SET);
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

	//cerr << reflevel << " reading " << n << " blocks" << endl;

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
		int e;
		TIMER_START(t0)
		NcError ncerror(NcError::silent_nonfatal);

		TIMER_START(t1)
		_ncvars[reflevel]->set_cur(_ncoffsets[reflevel], 0,0,0);
		if ((e = ncerror.get_err()) != NC_NOERR) {
			SetErrMsg("NcVar::set_cur() : %s", nc_strerror(e));
			return(-1);
		}
		TIMER_STOP(t0, _seek_timer)

		_ncvars[reflevel]->get(blks, (int) n,_bs[2],_bs[1],_bs[0]);
		if ((e = ncerror.get_err()) != NC_NOERR) {
			SetErrMsg("NcVar::get() : %s", nc_strerror(e));
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
	//cerr << "Read lambda ";
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

	//cerr << reflevel << " writing " << n << " blocks" << endl;

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
		int e;
		TIMER_START(t0)
		NcError ncerror(NcError::silent_nonfatal);

		TIMER_START(t1)
		_ncvars[reflevel]->set_cur(_ncoffsets[reflevel], 0,0,0);
		if ((e = ncerror.get_err()) != NC_NOERR) {
			SetErrMsg("NcVar::set_cur() : %s", nc_strerror(e));
			return(-1);
		}
		TIMER_STOP(t0, _seek_timer)

		_ncvars[reflevel]->put(blks, (int) n,_bs[2],_bs[1],_bs[0]);
		if ((e = ncerror.get_err()) != NC_NOERR) {
			SetErrMsg("NcVar::put() : %s", nc_strerror(e));
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
	//cerr << "Write lambda ";
	return(writeBlocks(blks, n, 0));
}

int	WaveletBlock3DIO::writeGammaBlocks(
	const float *blks, 
	size_t n, 
	int reflevel
) {
	//cerr << "Write gamma ";
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

		mins_c[j] = new float[size];
		maxs_c[j] = new float[size];
		if (! mins_c[j] || ! maxs_c[j]) {
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
		if (mins_c[j]) delete [] mins_c[j];
		if (maxs_c[j]) delete [] maxs_c[j];
		mins_c[j] = NULL;
		maxs_c[j] = NULL;
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


