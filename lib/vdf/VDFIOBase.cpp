#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <cassert>
#ifndef WIN32
#include <unistd.h>
#else
#include "windows.h"
#include "Winbase.h"
#include <vapor/common.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <netcdf.h>

#ifdef	Darwin
#include <mach/mach_time.h>
#endif

#include <vapor/VDFIOBase.h>
#include <vapor/MyBase.h>

using namespace VetsUtil;
using namespace VAPoR;

#define NC_ERR(rc, path) \
    if (rc != NC_NOERR) { \
        SetErrMsg( \
            "Error operating on netCDF file \"%s\" : %s",  \
            path.c_str(), nc_strerror(rc) \
        ); \
        return(-1); \
    }

//
// ncdf Dimension names
//
const string _volumeDimNbxName = "VolumeDimNbx";    // dim of volume in blocks
const string _volumeDimNbyName = "VolumeDimNby";
const string _volumeDimNbzName = "VolumeDimNbz";
const string _maskDimName = "MaskDim";    // size of mask in bytes per block
const string _maskName = "Mask";    // size of mask in bytes per block
const string _fileVersionName = "FileVersion";
const string _refinementLevelName = "RefinementLevel";


int	VDFIOBase::_VDFIOBase(
) {

	SetClassName("VDFIOBase");

	_read_timer_acc = 0.0;
	_write_timer_acc = 0.0;
	_seek_timer_acc = 0.0;
	_xform_timer_acc = 0.0;

	_read_timer = 0.0;
	_write_timer = 0.0;
	_seek_timer = 0.0;
	_xform_timer = 0.0;

	_ncid_mask = 0;
	_varname.clear();

	return(0);
}

VDFIOBase::VDFIOBase(
	const MetadataVDC	&metadata
) : MetadataVDC (metadata) {

	if (_VDFIOBase() < 0) return;

}

VDFIOBase::VDFIOBase(
	const string &metafile
) : MetadataVDC (metafile) {

	if (_VDFIOBase() < 0) return;
}

VDFIOBase::~VDFIOBase() {
}

double VDFIOBase::GetTime() const {
	double t;
#ifdef WIN32 //Windows does not have a nanosecond time function...
	SYSTEMTIME sTime;
	FILETIME fTime;
	GetSystemTime(&sTime);
	SystemTimeToFileTime(&sTime,&fTime);
    //Resulting system time is in 100ns increments
	__int64 longlongtime = fTime.dwHighDateTime;
	longlongtime <<= 32;
	longlongtime += fTime.dwLowDateTime;
	t = (double)longlongtime;
	t *= 1.e-7;

#endif
#ifndef WIN32
	struct timespec ts;
	ts.tv_sec = ts.tv_nsec = 0;
#endif

#if defined(Linux) || defined(AIX)
	clock_gettime(CLOCK_REALTIME, &ts);
	t = (double) ts.tv_sec + (double) ts.tv_nsec*1.0e-9;
#endif

#ifdef	Darwin
	uint64_t tmac = mach_absolute_time();
	mach_timebase_info_data_t info = {0,0};
	mach_timebase_info(&info);
	ts.tv_sec = tmac * 1e-9;
	ts.tv_nsec = tmac - (ts.tv_sec * 1e9);
	t = (double) ts.tv_sec + (double) ts.tv_nsec*1.0e-9;
#endif
	
	return(t);
}

//
// initializes members:
//	_vtype_mask
//	_reflevel_mask
//	_ncpath_mask
//	_ncpath_mask_tmp
//	_ncid_mask = 0
//	_mv_mask
//	_bs_p_mask : packed version of block size
//	_bdim_p_mask : packed version of grid dimension in block coordinates
//
int VDFIOBase::_mask_open(
    size_t timestep,
    string varname,
    int reflevel,
	size_t &bitmasksz
) {
	_ncpath_mask.clear();
	_ncpath_mask_tmp.clear();
	bitmasksz = 0;

    if (VDFIOBase::_MaskClose() < 0) return(-1);

    _vtype_mask = GetVarType(varname);
    if (_vtype_mask == VARUNKNOWN) {
        SetErrMsg("Invalid variable type");
        return(-1);
    }

	if (reflevel < 0) reflevel = GetNumTransforms();
	_reflevel_mask = reflevel;

	size_t bs[3];
	GetBlockSize(bs, _reflevel_mask);
	_PackCoord(_vtype_mask, bs, _bs_p_mask, 1);

	bitmasksz = _bs_p_mask[0] * _bs_p_mask[1] * _bs_p_mask[2];

	size_t bdim[3];
	VDFIOBase::GetDimBlk(bdim,_reflevel_mask);
	_PackCoord(_vtype_mask, bdim, _bdim_p_mask, 1);

	//
	// Most of the code below is used to compute the path name to the 
	// bit mask file based on the variable type (3d, 2dxy, etc), and 
	// whether the missing value locations are the same for all
	// time steps and all variables, vary from time step to time step, 
	// or vary with each time step and variable. 
	//
	string vtypestr;
	switch (_vtype_mask) {
	case VAR2D_XY:
		vtypestr = "2dxy";
		break;
	case VAR2D_XZ:
		vtypestr = "2dxz";
		break;
	case VAR2D_YZ:
		vtypestr = "2dyz";
		break;
	default:
		vtypestr = "3d";
		break;
	}

	vector <double> mv_vec;
	string s0, s1;
	string basename;
	//
	// Global missing value (same for all times and variables)
	//
	if (GetMissingValue().size() == 1) {
		mv_vec = GetMissingValue();

		ConstructFullVBase(0, varname, &basename);
		s0 = varname + "/" + varname + ".0000";
		s1 = (string) "mask/mask" + (string) "_" + vtypestr;
		size_t pos = basename.find(s0);
		basename.replace(pos, s0.size(), s1);
		
	}

	//
	// Per time step missing value
	//
	if (GetTSMissingValue(timestep).size() == 1) {
		mv_vec = GetTSMissingValue(timestep);

		ConstructFullVBase(timestep, varname, &basename);
		s0 = varname + "/" + varname;
		s1 = (string) "mask/mask" + (string) "_" + vtypestr;
		size_t pos = basename.find(s0);
		basename.replace(pos, s0.size(), s1);
	}

	//
	// Finally, check if a missing value is set for this specific variable and
	// time step.
	//
	if (GetVMissingValue(timestep, varname).size() == 1) {
		mv_vec = GetVMissingValue(timestep, varname);

		ConstructFullVBase(timestep, varname, &basename);
		s0 = varname + "/" + varname;
		s1 = varname + "/" + varname + "_mask";
		size_t pos = basename.find(s0);
		basename.replace(pos, s0.size(), s1);
	}
	if (mv_vec.size() == 0) return(0);	// No missing value for these data
	_mv_mask = (float) mv_vec[0];

	_ncpath_mask = basename + ".nc";
	_ncpath_mask_tmp = basename + "_tmp" + ".nc";
	_ncid_mask = 0;

	return(0);
}

const size_t NC_CHUNKSIZEHINT = 4*1024*1024;
int VDFIOBase::_MaskOpenWrite(
    size_t timestep,
    string varname,
    int reflevel
) {
	if (IsCoordinateVariable(_varname)) return(0);

	size_t bitmasksz;
	if (_mask_open(timestep, varname, reflevel, bitmasksz) < 0) return(-1);

	//
	// if _ncpath_mask is empty there is not missing data mask file
	//
	if (_ncpath_mask_tmp.length() == 0) return(0);

	string dir;
    DirName(_ncpath_mask_tmp, dir);
    if (MkDirHier(dir) < 0) {
        return(-1);
    }
	int rc;

	size_t chsz = NC_CHUNKSIZEHINT;
	rc = nc__create(_ncpath_mask_tmp.c_str(), NC_64BIT_OFFSET, 0, &chsz, &_ncid_mask);
	NC_ERR(rc,_ncpath_mask_tmp)

	// Disable data filling - may not be necessary
	//
	int mode;
	nc_set_fill(_ncid_mask, NC_NOFILL, &mode);

	size_t bdim[3];
	VDFIOBase::GetDimBlk(bdim,_reflevel_mask);

	//
	// Define netCDF dimensions
	//
	int all_dim_ids[4];
	rc = nc_def_dim(
		_ncid_mask,_volumeDimNbxName.c_str(),bdim[0],&all_dim_ids[0]
	);
	NC_ERR(rc,_ncpath_mask_tmp)

	rc = nc_def_dim(
		_ncid_mask,_volumeDimNbyName.c_str(),bdim[1],&all_dim_ids[1]
	);
	NC_ERR(rc,_ncpath_mask_tmp)

	rc = nc_def_dim(
		_ncid_mask,_volumeDimNbzName.c_str(),bdim[2],&all_dim_ids[2]
	);
	NC_ERR(rc,_ncpath_mask_tmp);

	rc = nc_def_dim(
		_ncid_mask,_maskDimName.c_str(),BitMask::getSize(bitmasksz),
		&all_dim_ids[3]
	);
   NC_ERR(rc,_ncpath_mask_tmp)

	int ndims = 0;
	int mask_dim_ids[4];
	switch (_vtype_mask) {
	case VAR2D_XY:
		ndims = 3;
		mask_dim_ids[0] = all_dim_ids[1];
		mask_dim_ids[1] = all_dim_ids[0];
		mask_dim_ids[2] = all_dim_ids[3];
	break;
	case VAR2D_XZ:
		ndims = 3;
		mask_dim_ids[0] = all_dim_ids[2];
		mask_dim_ids[1] = all_dim_ids[0];
		mask_dim_ids[2] = all_dim_ids[3];
	break;
	case VAR2D_YZ:
		ndims = 3;
		mask_dim_ids[0] = all_dim_ids[2];
		mask_dim_ids[1] = all_dim_ids[1];
		mask_dim_ids[2] = all_dim_ids[3];
	break;
	case VAR3D:
		ndims = 4;
		mask_dim_ids[0] = all_dim_ids[2];
		mask_dim_ids[1] = all_dim_ids[1];
		mask_dim_ids[2] = all_dim_ids[0];
		mask_dim_ids[3] = all_dim_ids[3];

	break;
	default:
		assert(0);
	}


	rc = nc_def_var(
		_ncid_mask, _maskName.c_str(), NC_CHAR,
		ndims, mask_dim_ids, &_varid_mask
	);
	NC_ERR(rc,_ncpath_mask_tmp)

	//
	// Define netCDF global attributes
	//
	int version = GetVDFVersion();
	rc = nc_put_att_int(
		_ncid_mask,NC_GLOBAL,_fileVersionName.c_str(),NC_INT, 1, &version
	);
	NC_ERR(rc,_ncpath_mask_tmp)

	rc = nc_put_att_int(
		_ncid_mask,NC_GLOBAL,_refinementLevelName.c_str(),NC_INT, 1,
		&_reflevel_mask
	);
	NC_ERR(rc,_ncpath_mask_tmp)

	rc = nc_enddef(_ncid_mask);
	_open_write_mask = true;
	return(0);
		
}

int VDFIOBase::_MaskOpenRead(
    size_t timestep,
    string varname,
    int reflevel
) {
	if (IsCoordinateVariable(_varname)) return(0);

	size_t dummy;
	if (_mask_open(timestep, varname, reflevel, dummy) < 0) return(-1);

	//
	// if _ncpath_mask is empty there is not missing data mask file
	//
	if (_ncpath_mask.length() == 0) return(0);

	int rc;

	size_t chsz = NC_CHUNKSIZEHINT;

	int ii = 0;
	do {

		rc = nc__open(_ncpath_mask.c_str(), NC_NOWRITE, &chsz, &_ncid_mask);
#ifdef WIN32
		if (rc == EAGAIN) Sleep(100);//milliseconds
#else
		if (rc == EAGAIN) sleep(1);//seconds
#endif

	} while (rc != NC_NOERR && ii < 10);
	NC_ERR(rc,_ncpath_mask)

	rc = nc_get_att_int(
		_ncid_mask, NC_GLOBAL, _refinementLevelName.c_str(), 
		&_reflevel_file_mask
	);
	NC_ERR(rc,_ncpath_mask)

	// dimensions of mask stored in file
	//
	size_t bdim[3];	
	VDFIOBase::GetDimBlk(bdim,_reflevel_file_mask);

	size_t ncdim;

	int ncdimid;
	rc = nc_inq_dimid(_ncid_mask, _volumeDimNbxName.c_str(), &ncdimid);
	NC_ERR(rc,_ncpath_mask)

	rc = nc_inq_dimlen(_ncid_mask, ncdimid, &ncdim);
	NC_ERR(rc,_ncpath_mask)

	if (ncdim != bdim[0]) {
		SetErrMsg("Metadata file and data file mismatch");
		return(-1);
	}

	rc = nc_inq_dimid(_ncid_mask, _volumeDimNbyName.c_str(), &ncdimid);
	NC_ERR(rc,_ncpath_mask)

	rc = nc_inq_dimlen(_ncid_mask, ncdimid, &ncdim);
	NC_ERR(rc,_ncpath_mask)

	if (ncdim != bdim[1]) {
		SetErrMsg("Metadata file and data file mismatch");
		return(-1);
	}

	rc = nc_inq_dimid(_ncid_mask, _volumeDimNbzName.c_str(), &ncdimid);
	NC_ERR(rc,_ncpath_mask)

	rc = nc_inq_dimlen(_ncid_mask, ncdimid, &ncdim);
	NC_ERR(rc,_ncpath_mask)

	if (ncdim != bdim[2]) {
		SetErrMsg("Metadata file and data file mismatch");
		return(-1);
	}

	rc = nc_inq_dimid(_ncid_mask, _maskDimName.c_str(), &ncdimid);
	NC_ERR(rc,_ncpath_mask)

	rc = nc_inq_dimlen(_ncid_mask, ncdimid, &ncdim);
	NC_ERR(rc,_ncpath_mask)

	nc_inq_varid(_ncid_mask, _maskName.c_str(), &_varid_mask);
	NC_ERR(rc,_ncpath_mask)

	_bitmasks.clear();
	for (int z = 0; z<_bdim_p_mask[2]; z++) {
	for (int y = 0; y<_bdim_p_mask[1]; y++) {
	for (int x = 0; x<_bdim_p_mask[0]; x++) {
		BitMask bm(0);
		_bitmasks.push_back(bm);
	}
	}
	}

	_open_write_mask = false;
	return(0);
}

int VDFIOBase::_MaskClose() 
{
	if (_ncid_mask == 0 || IsCoordinateVariable(_varname)) return(0);
	int rc = nc_close(_ncid_mask);
	NC_ERR(rc,_ncpath_mask)


	if (_open_write_mask) {
		rc = _unlink(_ncpath_mask.c_str());
		rc = rename(_ncpath_mask_tmp.c_str(), _ncpath_mask.c_str());
		if (rc<0) {
			MyBase::SetErrMsg(
				"rename(%s, %s) : %M", _ncpath_mask_tmp.c_str(), 
				_ncpath_mask.c_str()
			);
			return(-1);
		}
	}
	_bitmasks.clear();
	_ncid_mask = 0;
	_ncpath_mask.clear();
	_ncpath_mask_tmp.clear();

	return(0);
}

void VDFIOBase::_MaskRemove(
	float *blk,
	bool &valid_data
) const {
	valid_data = true;
	if (_ncid_mask == 0 || IsCoordinateVariable(_varname)) return;

	//
	// First, compute the global average for the block
	//
	size_t n = 0;
	double total = 0.0;
	for (size_t z = 0; z<_bs_p_mask[2]; z++) {
	for (size_t y = 0; y<_bs_p_mask[1]; y++) {
	for (size_t x = 0; x<_bs_p_mask[0]; x++) {
		float v = blk[_bs_p_mask[0]*_bs_p_mask[1]*z + _bs_p_mask[0]*y + x];
		if (v != _mv_mask) {
			n++;
			total += v;
		}
	}
	}
	}

	float ave = 0.0;
	if (n) {
		ave = total / (double) n;
		valid_data = true;
	}
	else {
		valid_data = false;
	}

	//
	// Now replace missing values with block average
	//
	for (size_t z = 0; z<_bs_p_mask[2]; z++) {
	for (size_t y = 0; y<_bs_p_mask[1]; y++) {
	for (size_t x = 0; x<_bs_p_mask[0]; x++) {
		float v = blk[_bs_p_mask[0]*_bs_p_mask[1]*z + _bs_p_mask[0]*y + x];
		if (v == _mv_mask) {
			blk[_bs_p_mask[0]*_bs_p_mask[1]*z + _bs_p_mask[0]*y + x] = ave;
		}
	}
	}
	}
}

void VDFIOBase::_MaskReplace(
	size_t bx, size_t by, size_t bz, float *blk
) const {
	if (_ncid_mask == 0 || IsCoordinateVariable(_varname)) return;

	size_t bidx = _bdim_p_mask[0]*_bdim_p_mask[1]*bz + _bdim_p_mask[0]*by + bx;

	const BitMask &bm = _bitmasks[bidx];

	size_t idx = 0;
	for (size_t z=0; z<_bs_p_mask[2]; z++) {
	for (size_t y=0; y<_bs_p_mask[1]; y++) {
	for (size_t x=0; x<_bs_p_mask[0]; x++) {
		if (bm.getBit(idx)) {
			blk[idx] = _mv_mask;
		}
		idx++;
	}
	}
	}
}

int VDFIOBase::_MaskWrite(
	const float *region,
	const size_t bmin_p[3], const size_t bmax_p[3], bool block
) {
	if (_ncid_mask == 0 || IsCoordinateVariable(_varname)) return(0);

	size_t blocksize = _bs_p_mask[0]*_bs_p_mask[1]*_bs_p_mask[2];

	BitMask bm(blocksize);


	size_t nx = (bmax_p[0] - bmin_p[0] + 1) * _bs_p_mask[0];
	size_t ny = (bmax_p[1] - bmin_p[1] + 1) * _bs_p_mask[1];

	size_t nbx = (bmax_p[0] - bmin_p[0] + 1);
	size_t nby = (bmax_p[1] - bmin_p[1] + 1);

	for (size_t bz=bmin_p[2]; bz<=bmax_p[2]; bz++) {
	for (size_t by=bmin_p[1]; by<=bmax_p[1]; by++) {
	for (size_t bx=bmin_p[0]; bx<=bmax_p[0]; bx++) {

		// Block coordinates relative to region origin
		//
		size_t bxx = bx - bmin_p[0];
		size_t byy = by - bmin_p[1];
		size_t bzz = bz - bmin_p[2];

		bm.clear();
		if (block) {
			// Starting coordinate of current block in voxels
			//
			size_t x0 = bxx * _bs_p_mask[0];
			size_t y0 = byy * _bs_p_mask[1];
			size_t z0 = bzz * _bs_p_mask[2];

			size_t idx = 0;
			for (int z = 0; z<_bs_p_mask[2]; z++) {
			for (int y = 0; y<_bs_p_mask[1]; y++) {
			for (int x = 0; x<_bs_p_mask[0]; x++) {
				float v = region[nx*ny*(z0+z) + nx*(y0+y) + (x0+x)];
				if (v == _mv_mask) {
					bm.setBit(idx, true);
				}
				idx++;
			}
			}
			}
		}
		else {
			const float *blockptr = region + bzz*nbx*nby*blocksize +
				byy*nbx*blocksize + bxx*blocksize;

			size_t idx = 0;
			for (int z = 0; z<_bs_p_mask[2]; z++) {
			for (int y = 0; y<_bs_p_mask[1]; y++) {
			for (int x = 0; x<_bs_p_mask[0]; x++) {
				float v = blockptr[_bs_p_mask[0]*_bs_p_mask[1]*z + _bs_p_mask[0]*y + x];
				if (v == _mv_mask) {
					bm.setBit(idx, true);
				}
				idx++;
			}
			}
			}
		}

		size_t start[] = {0,0,0,0};
		size_t count[] = {1,1,1,1};

		if (_vtype_mask == VAR3D) {
			start[0] = bz;
			start[1] = by;
			start[2] = bx;
			count[3] = BitMask::getSize(blocksize);
		}
		else {
			start[0] = by;
			start[1] = bx;
			count[2] = BitMask::getSize(blocksize);
		}

		int rc = nc_put_vara(_ncid_mask, _varid_mask, start, count, bm.getStorage());
		NC_ERR(rc,_ncpath_mask_tmp)
	}
	}
	}

	return(0);
}

void VDFIOBase::_downsample(
	const BitMask &bm0, BitMask &bm1, int l0, int l1
) const {

	size_t bs0[3];
	GetBlockSize(bs0, l0);
	size_t bs1[3];
	GetBlockSize(bs1, l1);

	size_t bs0_p[3];
	_PackCoord(_vtype_mask, bs0, bs0_p, 1);
	size_t bs1_p[3];
	_PackCoord(_vtype_mask, bs1, bs1_p, 1);

	bm1.clear();
	bm1.resize(bs1_p[2]*bs1_p[1]*bs1_p[0]);

	assert (l0 >= l1);
	int stride = 1 << (l0-l1);

	for (int z0=0, z1=0; z0<bs0_p[2] &&  z1<bs1_p[2]; z0 += stride, z1++) {
	for (int y0=0, y1=0; y0<bs0_p[1] &&  y1<bs1_p[1]; y0 += stride, y1++) {
	for (int x0=0, x1=0; x0<bs0_p[0] &&  x1<bs1_p[0]; x0 += stride, x1++) {
		size_t idx1 = bs1_p[0]*bs1_p[1]*z1 + bs1_p[0]*y1 + x1;

		bool skip = false;
		for (int zz0 = z0; zz0<z0+stride && zz0 < bs0_p[2] && ! skip; zz0++) {
		for (int yy0 = y0; yy0<y0+stride && yy0 < bs0_p[1] && ! skip; yy0++) {
		for (int xx0 = x0; xx0<x0+stride && xx0 < bs0_p[0] && ! skip; xx0++) {
			size_t idx0 = bs0_p[0]*bs0_p[1]*zz0 + bs0_p[0]*yy0 + xx0;
			if (bm0.getBit(idx0)) {
				bm1.setBit(idx1, true);
				skip = true;
			}
		}
		}
		}
	}
	}
	}
}


	
int VDFIOBase::_MaskRead(
	const size_t bmin_p[3], const size_t bmax_p[3]
) {
	if (_ncid_mask == 0 || IsCoordinateVariable(_varname)) return(0);

	size_t bdim_file[3];
	VDFIOBase::GetDimBlk(bdim_file, _reflevel_file_mask);
	size_t bdim_p_file[3];   // packed version of bdim
	_PackCoord(_vtype_mask, bdim_file, bdim_p_file, 1);

	size_t bs_file[3];
	GetBlockSize(bs_file, _reflevel_file_mask);
	size_t bs_p_file[3];
	_PackCoord(_vtype_mask, bs_file, bs_p_file, 1);
	size_t blocksize = bs_p_file[0]*bs_p_file[1]*bs_p_file[2];

	for (size_t bz=bmin_p[2]; bz<=bmax_p[2]; bz++) {
	for (size_t by=bmin_p[1]; by<=bmax_p[1]; by++) {
	for (size_t bx=bmin_p[0]; bx<=bmax_p[0]; bx++) {
		size_t idx = _bdim_p_mask[0]*_bdim_p_mask[1]*bz + _bdim_p_mask[0]*by + bx;

		size_t start[] = {0,0,0,0};
		size_t count[] = {1,1,1,1};

		if (_vtype_mask == VAR3D) {
			start[0] = bz;
			start[1] = by;
			start[2] = bx;
			count[3] = BitMask::getSize(blocksize);
		}
		else {
			start[0] = by;
			start[1] = bx;
			count[2] = BitMask::getSize(blocksize);
		}

		BitMask bm(blocksize);
		int rc = nc_get_vara(_ncid_mask, _varid_mask, start, count, bm.getStorage());
		NC_ERR(rc,_ncpath_mask)

		assert(idx < _bitmasks.size());
		if (_reflevel_mask < _reflevel_file_mask) {
			_downsample(bm, _bitmasks[idx],_reflevel_file_mask, _reflevel_mask);
		}
		else {
			_bitmasks[idx] = bm;
		}

	}
	}
	}

	return(0);
}


void VDFIOBase::BitMask::_BitMask(size_t nbits) 
{
	_nbits = 0;
	_bitmask = NULL;
	_bitmask_sz = BitMask::getSize(nbits);
	if (_bitmask_sz) {
		_bitmask = new unsigned char [_bitmask_sz];
		for (int i=0; i<_bitmask_sz; i++) _bitmask[i] = 0;
	}
	_nbits = nbits;
}

VDFIOBase::BitMask::BitMask() 
{
	_BitMask(0);
}

VDFIOBase::BitMask::BitMask(size_t nbits) 
{
	_BitMask(nbits);
}

VDFIOBase::BitMask::BitMask(const BitMask &bm) 
{
	_BitMask(bm.size());

	if (_bitmask_sz) {
		for (int i=0; i<BitMask::getSize(_nbits); i++) {
			_bitmask[i] = bm._bitmask[i];
		}
	}
}

VDFIOBase::BitMask::~BitMask() 
{
	if (_bitmask) delete [] _bitmask;
	_bitmask = NULL;
}

void VDFIOBase::BitMask::clear() {
	for (size_t i=0; i<getSize(_nbits); i++) {
		_bitmask[i] = 0;
	}
}

void VDFIOBase::BitMask::resize(size_t nbits) {
	clear();

	if (getSize(nbits) > _bitmask_sz) {
		if (_bitmask) delete [] _bitmask;
		_bitmask_sz = getSize(nbits); 
		_bitmask = new unsigned char [_bitmask_sz];
		for (int i=0; i<_bitmask_sz; i++) _bitmask[i] = 0;
	}
	_nbits = nbits;
}

void VDFIOBase::BitMask::setBit(size_t idx, bool value) {
	size_t byte_idx = idx / 8;
	if (byte_idx >= getSize(_nbits)) return;
	unsigned char mask = (unsigned char) (0x80 >> idx % 8);

	unsigned char &byte = _bitmask[byte_idx];

	if( value == true ) {
		 byte |= mask;
	}
	else {
		 byte &= 0xFF ^ mask;
	}
}

bool VDFIOBase::BitMask::getBit(size_t idx) const {
	size_t byte_idx = idx / 8;
	if (byte_idx >= getSize(_nbits)) return (false);;
	unsigned char byte = _bitmask[byte_idx];

	byte &= (unsigned char) ( 0x80 >> idx % 8 );

	return(byte != 0);
}

unsigned char *VDFIOBase::BitMask::getStorage() const {
	return(_bitmask);
}

VDFIOBase::BitMask& VDFIOBase::BitMask::operator=(const BitMask &bm) {
	if (this == &bm)  return(*this);

	this->resize(bm.size());
	for (int i=0; i<getSize(_nbits); i++) {
		_bitmask[i] = bm._bitmask[i];
	}
	return(*this);
}


int    VAPoR::MkDirHier(const string &dir) {

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
        if ((mkdir(s.c_str(), 0777) < 0) && dirs.empty() && errno != EEXIST) {
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

void    VAPoR::DirName(const string &path, string &dir) {
	
	string::size_type idx = path.find_last_of('/');
#ifdef WIN32
	if (idx == string::npos)
		idx = path.find_last_of('\\');
#endif
	if (idx == string::npos) {
#ifdef WIN32
		dir.assign(".\\");
#else
		dir.assign("./");
#endif
	}
	else {
		dir = path.substr(0, idx+1);
	}
}


void VDFIOBase::_UnpackCoord(
	VarType_T vtype, const size_t src[3], size_t dst[3], size_t fill
) {

	switch (vtype) { 
	case VAR2D_XY:
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = fill;
	break;
	case VAR2D_XZ:
		dst[0] = src[0];
		dst[1] = fill;
		dst[2] = src[1];
	break; 
	case VAR2D_YZ:
		dst[0] = fill;
		dst[1] = src[0];
		dst[2] = src[1];
	break;
	case VAR3D:
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
	break;
		default:
		assert(0);
	}
}

void VDFIOBase::_PackCoord(
	VarType_T vtype, const size_t src[3], size_t dst[3], size_t fill
) {

	for (int i=0; i<3; i++) dst[i] = fill;

	switch (vtype) { 
	case VAR2D_XY:
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = fill;
	break;
	case VAR2D_XZ:
		dst[0] = src[0];
		dst[1] = src[2];
		dst[2] = fill;
	break; 
	case VAR2D_YZ:
		dst[0] = src[1];
		dst[1] = src[2];
		dst[0] = fill;
	break;
	case VAR3D:
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
	break;
		default:
		assert(0);
	}
}

void VDFIOBase::_FillPackedCoord(
	VarType_T vtype, const size_t src[3], size_t dst[3], size_t fill
) {

	for (int i=0; i<3; i++) dst[i] = fill;

	switch (vtype) { 
	case VAR2D_XY:
	case VAR2D_XZ:
	case VAR2D_YZ:
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = fill;
	break;
	case VAR3D:
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
	break;
		default:
		assert(0);
	}
}
