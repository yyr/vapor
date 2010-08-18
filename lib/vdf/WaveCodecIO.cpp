

#include <iostream>
#include <sstream>
#include <netcdf.h>
#include <sys/stat.h>
#include <vapor/WaveCodecIO.h>
#ifdef WIN32
#include <windows.h>
#endif

using namespace std;
using namespace VAPoR;

//
// ncdf Dimension names
//
const string _volumeDimNbxName = "VolumeDimNbx";	// dim of volume in blocks
const string _volumeDimNbyName = "VolumeDimNby";
const string _volumeDimNbzName = "VolumeDimNbz";
const string _coeffDimName = "CoeffDim";	// num. coeff per block
const string _sigMapDimName = "SigMapDim";	// size of sig map per block

//
// ncdf attribute names
//
const string _blockSizeName = "BlockSize";	// dim of block in voxels
const string _waveletName = "Wavelet";	// wavelet family name
const string _boundaryModeName = "BoundaryMode";	// wavelet boundary handling
const string _fileVersionName = "FileVersion";
const string _scalarRangeName = "ScalarDataRange";
const string _minValidRegionName = "MinValidRegion";
const string _maxValidRegionName = "MaxValidRegion";
const string _nativeResName = "NativeResolution";	// vol dim in voxels
const string _compressionLevelName = "CompressionLevel";

//
// ncdf variable names
//
const string _minsName = "BlockMinValues";
const string _maxsName = "BlockMaxValues";
const string _waveletCoeffName = "WaveletCoefficients";
const string _sigMapName = "SigMaps";

const size_t NC_CHUNKSIZEHINT = 4*1024*1024;

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



namespace {
	bool rcmp (size_t a, size_t b) {return a > b;}
};



int WaveCodecIO::_WaveCodecIO() {

	
	_compressor = NULL;
	_compressor3D = NULL;
	_compressor2DXY = NULL;
	_compressor2DXZ = NULL;
	_compressor2DYZ = NULL;
	_compressorType = VARUNKNOWN;
	_vtype = VARUNKNOWN;

	_cvector = NULL;
	_cvectorsize = 0;
	_svector = NULL;
	_svectorsize = 0;
	_sliceCount = 0;
	_sliceBufferSize = 0;
	_sliceBuffer = NULL;
	_isOpen = false;
	_pad = true;

	_cratios3D = GetCRatios();
	sort(_cratios3D.begin(), _cratios3D.end(), rcmp);
	_cratios = _cratios3D;

	//
	// Try to compute "reasonable" 2D compression ratios from 3D 
	// compression ratios
	//
	_cratios2D.clear();
	for (int i=0; i<_cratios3D.size(); i++) {
		size_t c = (size_t) pow((double) _cratios3D[i], (double) (2.0 / 3.0));
		_cratios2D.push_back(c);
	}

	_sigmaps = new SignificanceMap* [_cratios.size()];
	for (int i=0; i<_cratios.size(); i++) {
		_sigmaps[i] = new SignificanceMap();
	}
	const size_t *bs =  VDFIOBase::GetBlockSize();

	_block = NULL;
	_blockReg = NULL;

	//
	// Set up the default compressor - a 3D one.
	//
	vector <size_t> bdims;
	bdims.push_back(bs[0]);
	bdims.push_back(bs[1]);
	bdims.push_back(bs[2]);
	string wavename = GetWaveName();
	string boundary_mode = GetBoundaryMode();
	_compressor3D = new Compressor(bdims, wavename, boundary_mode);
	if (Compressor::GetErrCode() != 0) {
		SetErrMsg("Failed to allocate compressor");
		return(-1);
	}
	_compressor = _compressor3D;

	return(0);
}

WaveCodecIO::WaveCodecIO(
	const MetadataVDC &metadata
) : VDFIOBase(metadata) {

	if (VDFIOBase::GetErrCode()) return;

	if (_WaveCodecIO() < 0) return;
}

WaveCodecIO::WaveCodecIO(
	const string &metafile
) : VDFIOBase(metafile) {

	if (VDFIOBase::GetErrCode()) return;

	if (_WaveCodecIO() < 0) return;
}


#ifdef	DEAD
WaveCodecIO::WaveCodecIO(
	const size_t dim[3], const size_t bs[3], int numTransforms, 
	const vector <size_t> cratios,
	const string &wname, const string &filebase
) {
	// cratios need to be sorted, largest to smallest. 
	_cratios3D = GetCRatios();
	sort(_cratios3D.begin(), _cratios3D.end(), rcmp);

	// validity check on cratios
}

WaveCodecIO(const vector <string> &files);
#endif

WaveCodecIO::~WaveCodecIO() {
	if (_sigmaps) {
		for (int i =0; i<_cratios.size(); i++) {
			delete _sigmaps[i];
		}
		delete [] _sigmaps;
	}
	if (_cvector) delete [] _cvector;
	if (_svector) delete [] _svector;
	if (_compressor3D) delete _compressor3D;
	if (_compressor2DXY) delete _compressor2DXY;
	if (_compressor2DXZ) delete _compressor2DXZ;
	if (_compressor2DYZ) delete _compressor2DYZ;
	if (_block) delete [] _block;
	if (_blockReg) delete [] _blockReg;
	if (_sliceBuffer) delete [] _sliceBuffer;
}


int WaveCodecIO::OpenVariableRead(
	size_t timestep, const char *varname, int reflevel, int lod
) {
	string basename;

	if (CloseVariable() < 0) return(-1); 

	_vtype = GetVarType(varname);
	if (_vtype == VARUNKNOWN) {
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	if (_SetupCompressor()<0) return(-1);

	//
	// Ugh. We indicate successful open so various methods called
	// below don't fail (notably GetNumTransforms(), which is called by
	// GetDimBlk()
	//
	_isOpen = true;

	_writeMode = false;

	_timeStep = timestep;
	_varName.assign(varname);

	if (reflevel < 0) reflevel = GetNumTransforms();
	if (reflevel < 0) {
		_isOpen = false; return(-1);
	}

	if (reflevel > GetNumTransforms()) {
		SetErrMsg("Requested refinement level out of range (%d)", reflevel);
		_isOpen = false;
		return(-1);
	}
	_reflevel = reflevel;

	_lod = lod;
	if (_lod < 0) _lod = _cratios.size() - 1;

	if (_lod >= _cratios.size()) {
		SetErrMsg("Requested compression level out of range (%d)", _lod);
		_isOpen = false;
		return(-1);
	}

	if (!VariableExists(timestep, varname, reflevel, lod)) {
		SetErrMsg(
			"Variable \"%s\" not present at requested timestep or level",
			_varName.c_str()
		);
		_isOpen = false;
		return(-1);
	}

	if (ConstructFullVBase(timestep, varname, &basename) < 0) {
		_isOpen = false;
		return (-1);
	}

	int rc = _OpenVarRead(basename);
	if (rc<0) { 
		_isOpen = false;
		return(-1);
	}

	return(0);
}


int WaveCodecIO::OpenVariableWrite(
	size_t timestep, 
	const char *varname, 
	int reflevel, /*ignored*/
	int lod
) {

	if (CloseVariable() < 0) return(-1); 

	_vtype = GetVarType(varname);
	if (_vtype == VARUNKNOWN) {
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	if (_SetupCompressor()<0) return(-1);

	//
	// Ugh. We indicate successful open so various methods called
	// below don't fail (notably GetNumTransforms(), which is called by
	// GetDimBlk()
	//
	_isOpen = true;

	if (lod < 0)  lod = _cratios.size() - 1;

	if (lod >= _cratios.size()) {
		SetErrMsg("Requested compression level out of range (%d)", lod);
		_isOpen = false;
		return(-1);
	}
	_lod = lod;

	size_t dim[3];
	GetDim(dim, -1);
	for(int i=0; i<3; i++) {
		_validRegMin[i] = dim[i]-1;
		_validRegMax[i] = 0;	// impossible values
	}
	// 
	// For 2D data initialize bounds of unused dimension to full
	// volume extents (dimensions)
	//
	switch (_vtype) {
	case VAR2D_XY:
		_validRegMin[2] = 0;
		_validRegMax[2] = dim[2]-1;
		break;
	case VAR2D_XZ:
		_validRegMin[1] = 0;
		_validRegMax[1] = dim[1]-1;
		break;
	case VAR2D_YZ:
		_validRegMin[0] = 0;
			_validRegMax[0] = dim[0]-1;
		break;
	default:
		break;
	}

	string dir;
	string basename;

	_dataRange[0] = _dataRange[1] = 0.0;

	_writeMode = true;

	_timeStep = timestep;
	_varName.assign(varname);
	_reflevel = reflevel;
	_firstWrite = true;
	_sliceCount = 0;

	if (ConstructFullVBase(timestep, varname, &basename) < 0) {
		_isOpen = false;
		return (-1);
	}

	DirName(basename, dir);
	if (MkDirHier(dir) < 0) {
		_isOpen = false;
		return(-1);
	}


	int rc = _OpenVarWrite(basename);
	if (rc<0) {
		_isOpen = false;
		return(-1);
	}

	return(0);
}

int WaveCodecIO::CloseVariable() {

	if (! _isOpen) return(0);

	for(int j=0; j<=_lod; j++) {

		if (_ncids[j] > -1) {
			int rc; 

			if (_writeMode) {

				rc = nc_put_att_float(
					_ncids[j], NC_GLOBAL, _scalarRangeName.c_str(), 
					NC_FLOAT, 2,_dataRange
				);
				NC_ERR_WRITE(rc,_ncpaths[j])

				int minreg_int[] = {
					_validRegMin[0], _validRegMin[1], _validRegMin[2]
				};
				rc = nc_put_att_int(
					_ncids[j],NC_GLOBAL,_minValidRegionName.c_str(),
					NC_INT, 3, minreg_int
				);
				NC_ERR_WRITE(rc,_ncpaths[j])

				int maxreg_int[] = {
					_validRegMax[0], _validRegMax[1], _validRegMax[2]
				};
				rc = nc_put_att_int(
					_ncids[j],NC_GLOBAL,_maxValidRegionName.c_str(),
					NC_INT, 3, maxreg_int
				);
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
		_nc_wave_vars[j] = -1;
		_nc_sig_vars[j] = -1;
	}
				
	_isOpen = false;
    _vtype = VARUNKNOWN;

	return(0);

}

int WaveCodecIO::BlockReadRegion(
	const size_t bmin[3], const size_t bmax[3], float *region, int unblock
) {
	if (! _isOpen || _writeMode) {
		SetErrMsg("Variable not open for reading\n");
		return(-1);
	}

	size_t bmin_p[3], bmax_p[3];	// filled copies of bmin, bmax
	size_t block_size;
	size_t bs[3];
	size_t bs_p[3];	// packed copy of bs

	if (unblock && ! _block) {
		GetBlockSize(bs, -1);
		size_t sz = bs[0]*bs[1]*bs[2];
		_block = new float [sz];
	}

	GetBlockSize(bs, _reflevel);

	_FillPackedCoord(_vtype, bmin, bmin_p, 0);
	_FillPackedCoord(_vtype, bmax, bmax_p, 0);
	_PackCoord(_vtype, bs, bs_p, 1);

	block_size = bs_p[0]*bs_p[1]*bs_p[2];

	size_t bmin_up[3], bmax_up[3];	// unpacked copy of bmin, bmax
	_UnpackCoord(_vtype, bmin, bmin_up, 0);
	_UnpackCoord(_vtype, bmax, bmax_up, 0);
	if (! IsValidRegionBlk(bmin_up, bmax_up, _reflevel)) {
		SetErrMsg(
			"Invalid region : (%d %d %d) (%d %d %d)", 
			bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]
		);
		return(-1);
	}


	// dimensions of region in voxels
	//
	size_t nx = (bmax_p[0] - bmin_p[0] + 1) * bs_p[0];
	size_t ny = (bmax_p[1] - bmin_p[1] + 1) * bs_p[1];

	size_t nbx = (bmax_p[0] - bmin_p[0] + 1);
	size_t nby = (bmax_p[1] - bmin_p[1] + 1);

	int rc;
	for (int bz=bmin_p[2]; bz<=bmax_p[2]; bz++) {
	for (int by=bmin_p[1]; by<=bmax_p[1]; by++) {
	for (int bx=bmin_p[0]; bx<=bmax_p[0]; bx++) {

		rc = _FetchBlock(bx, by, bz);
		if (rc<0) return(-1);

		// Block coordinates relative to region origin
		//
		int bxx = bx - bmin_p[0];
		int byy = by - bmin_p[1];
		int bzz = bz - bmin_p[2];

		float *blockptr;

		if (unblock) {
			blockptr = _block;

			_XFormTimerStart();
			rc = _compressor->Reconstruct(
				_cvector, blockptr, _sigmaps, _lod+1, _reflevel
			); 
			if (rc<0) return(-1);
			_XFormTimerStop();

			// Starting coordinate of current block in voxels
			// relative to region origin
			//
			size_t x0 = bxx * bs_p[0];
			size_t y0 = byy * bs_p[1];
			size_t z0 = bzz * bs_p[2];

			for (int z = 0; z<bs_p[2]; z++) {
			for (int y = 0; y<bs_p[1]; y++) {
			for (int x = 0; x<bs_p[0]; x++) {
				float v = _block[z*bs_p[0]*bs_p[1] + y*bs_p[0] + x];

				region[nx*ny*(z0+z) + nx*(y0+y) + (x0+x)]  = v;

			}
			}
			}
		}
		else {

			blockptr = region + bzz*nbx*nby*block_size + 
				byy*nbx*block_size + bxx*block_size;

			_XFormTimerStart();
			int rc = _compressor->Reconstruct(
				_cvector, blockptr, _sigmaps, _lod+1, _reflevel
			); 
			if (rc<0) return(-1);
			_XFormTimerStop();

		}

	}
	}
	}

	return(0);
}

int WaveCodecIO::ReadRegion(
	const size_t min[3], const size_t max[3], float *region
) {
	if (! _isOpen || _writeMode) {
		SetErrMsg("Variable not open for reading\n");
		return(-1);
	}

	size_t min_up[3], max_up[3];	// unpacked copy of min, max
	_UnpackCoord(_vtype, min, min_up, 0);
	_UnpackCoord(_vtype, max, max_up, 0);
	if (! IsValidRegion(min_up, max_up, _reflevel)) {
		SetErrMsg(
			"Invalid region : (%d %d %d) (%d %d %d)", 
			min[0], min[1], min[2], max[0], max[1], max[2]
		);
		return(-1);
	}


	//
	// unpacked and packed coordinates, respectively, of region in blocks
	//
	size_t bmin_up[3], bmax_up[3];
	size_t bmin_p[3], bmax_p[3];

	// packed coordinates of region 
	size_t min_p[3], max_p[3];

	// MapVoxToBlk operates on unpacked coordinates
	//
	MapVoxToBlk(min_up, bmin_up, _reflevel);
	MapVoxToBlk(max_up, bmax_up, _reflevel);

	_PackCoord(_vtype, bmin_up, bmin_p, 0);
	_PackCoord(_vtype, bmax_up, bmax_p, 0);

	//
	// fill in 3rd dimension (no-op if 3D data)
	//
	_FillPackedCoord(_vtype, min, min_p, 0);
	_FillPackedCoord(_vtype, max, max_p, 0);

	//
	// Dimension of region in voxels
	//
	size_t nx = max_p[0] - min_p[0] + 1; 
	size_t ny = max_p[1] - min_p[1] + 1; 

	size_t bs[3];
	size_t bs_p[3];	// packed copy of bs

	if (! _blockReg) {
		GetBlockSize(bs, -1);
		size_t sz = bs[0]*bs[1]*bs[2];
		_blockReg = new float [sz];
	}

	GetBlockSize(bs, _reflevel);
	_PackCoord(_vtype, bs, bs_p, 1);


	int rc;
	for (int bz=bmin_p[2]; bz<=bmax_p[2]; bz++) {
	for (int by=bmin_p[1]; by<=bmax_p[1]; by++) {
	for (int bx=bmin_p[0]; bx<=bmax_p[0]; bx++) {
		size_t b_p[] = {bx,by,bz};

		//
		// Read current block
		//
		rc = BlockReadRegion(b_p, b_p, _blockReg, 0);
		if (rc<0) return(-1);

		//
		// Now copy current block to region, clipping block to 
		// region bounds
		//

		// Packed voxel coordinates of subregion within current 
		// block relative to volume
		//
		size_t minv_p[3], maxv_p[3];

		// Packed voxel coordinates of subregion within current 
		// block relative to current block
		//
		size_t minb_p[3], maxb_p[3];

		//
		// Set up coordinates for copy
		//
		for (int i=0; i<3; i++) {

			minb_p[i] = 0;
			maxb_p[i] = bs_p[i]-1;

			minv_p[i] = b_p[i] * bs_p[i];
			maxv_p[i] = (b_p[i]+1) * bs_p[i] - 1;

			// Clip to region bounds
			//
			if (minv_p[i] < min_p[i]) {
				minb_p[i] += min_p[i] - minv_p[i];
				minv_p[i] = min_p[i];
			}
			if (maxv_p[i] > max_p[i]) {
				maxb_p[i] -= maxv_p[i] - max_p[i];
				maxv_p[i] = max_p[i];
			}
		}

		//
		// Finally, copy data from block to region
		//
		for (int z=minb_p[2], zr=minv_p[2]-min_p[2]; z<=maxb_p[2]; z++,zr++){
		for (int y=minb_p[1], yr=minv_p[1]-min_p[1]; y<=maxb_p[1]; y++,yr++){
		for (int x=minb_p[0], xr=minv_p[0]-min_p[0]; x<=maxb_p[0]; x++,xr++){

			region[zr*nx*ny + yr*nx + xr] = 
				_blockReg[z*bs_p[0]*bs_p[1] + y*bs_p[0] + x];
		}
		}
		}

	}
	}
	}
	return(0);
}

int WaveCodecIO::BlockWriteRegion(
	const float *region, const size_t bmin[3], const size_t bmax[3], int block
) {

	if (! _isOpen || ! _writeMode) {
		SetErrMsg("Variable not open for writing\n");
		return(-1);
	}

	size_t block_size;

	size_t bdim[3];
	GetDimBlk(bdim,-1);

	size_t bdim_p[3];	// packed version of bdim
	_PackCoord(_vtype, bdim, bdim_p, 1);

	size_t dim[3];
	GetDim(dim, -1);

	size_t dim_p[3];
	_PackCoord(_vtype, dim, dim_p, 1);

	size_t bmin_p[3], bmax_p[3];	// filled copies of bmin, bmax
	_FillPackedCoord(_vtype, bmin, bmin_p, 0);
	_FillPackedCoord(_vtype, bmax, bmax_p, 0);

	const size_t *bs =  VDFIOBase::GetBlockSize();

	if (! _block) {
		size_t sz = bs[0]*bs[1]*bs[2];
		_block = new float [sz];
	}

	size_t bs_p[3];	// packed copy of bs
	_PackCoord(_vtype, bs, bs_p, 1);

	block_size = bs_p[0]*bs_p[1]*bs_p[2];

	if (_firstWrite) {
		_dataRange[0] = _dataRange[1] = *region;


		_firstWrite = false;
	}

	size_t bmin_up[3], bmax_up[3];	// unpacked copy of bmin, bmax
	_UnpackCoord(_vtype, bmin, bmin_up, 0);
	_UnpackCoord(_vtype, bmax, bmax_up, 0);
	if (! IsValidRegionBlk(bmin_up, bmax_up, -1)) {
		SetErrMsg(
			"Invalid region : (%d %d %d) (%d %d %d)", 
			bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]
		);
		return(-1);
	}


	// dimensions of region in voxels
	//
	size_t nx = (bmax_p[0] - bmin_p[0] + 1) * bs_p[0];
	size_t ny = (bmax_p[1] - bmin_p[1] + 1) * bs_p[1];

	size_t nbx = (bmax_p[0] - bmin_p[0] + 1);
	size_t nby = (bmax_p[1] - bmin_p[1] + 1);

	for (int bz=bmin_p[2]; bz<=bmax_p[2]; bz++) {
	for (int by=bmin_p[1]; by<=bmax_p[1]; by++) {
	for (int bx=bmin_p[0]; bx<=bmax_p[0]; bx++) {

		//
		// These flags are true if this is a boundary block && the volume
		// dimensions don't match the blocked-volume dimensions
		//
		bool xbdry = (bx == (bdim_p[0]-1) && (bdim_p[0]*bs_p[0] != dim_p[0]));
		bool ybdry = (by == (bdim_p[1]-1) && (bdim_p[1]*bs_p[1] != dim_p[1]));
		bool zbdry = (bz == (bdim_p[2]-1) && (bdim_p[2]*bs_p[2] != dim_p[2]));

		//
		// the xyzstop limits are the bounds of the valid data for the block
		// (in the case where bdim*bs != dim)
		//
		size_t xstop = bs_p[0];
		size_t ystop = bs_p[1];
		size_t zstop = bs_p[2];

		if (xbdry) xstop -= (bdim_p[0]*bs_p[0] - dim_p[0]);
		if (ybdry) ystop -= (bdim_p[1]*bs_p[1] - dim_p[1]);
		if (zbdry) zstop -= (bdim_p[2]*bs_p[2] - dim_p[2]);

		// Block coordinates relative to region origin
		//
		int bxx = bx - bmin_p[0];
		int byy = by - bmin_p[1];
		int bzz = bz - bmin_p[2];

		const float *blockptr;

		if (block) {
			blockptr = _block;

			// Starting coordinate of current block in voxels
			//
			size_t x0 = bxx * bs_p[0];
			size_t y0 = byy * bs_p[1];
			size_t z0 = bzz * bs_p[2];

			for (int z = 0; z<zstop; z++) {
			for (int y = 0; y<ystop; y++) {
			for (int x = 0; x<xstop; x++) {
				float v = region[nx*ny*(z0+z) + nx*(y0+y) + (x0+x)];

				_block[z*bs_p[0]*bs_p[1] + y*bs_p[0] + x] = v;
				if (v < _dataRange[0]) _dataRange[0] = v;
				if (v > _dataRange[1]) _dataRange[1] = v;
			}
			}
			}
		}
		else {
			blockptr = region + bzz*nbx*nby*block_size + 
				byy*nbx*block_size + bxx*block_size;

			for (int z = 0; z<zstop; z++) {
			for (int y = 0; y<ystop; y++) {
			for (int x = 0; x<xstop; x++) {
				float v = blockptr[bs_p[0]*bs_p[1]*z + bs_p[0]*y + z];

				if (v < _dataRange[0]) _dataRange[0] = v;
				if (v > _dataRange[1]) _dataRange[1] = v;

			}
			}
			}

			//
			// If we need padding we need to copy the data to local space
			//
			if ((xbdry || ybdry || zbdry) && _pad) { 

				for (int z = 0; z<zstop; z++) {
				for (int y = 0; y<ystop; y++) {
				for (int x = 0; x<xstop; x++) {
					_block[bs_p[0]*bs_p[1]*z + bs_p[0]*y + z] = 
						blockptr[bs_p[0]*bs_p[1]*z + bs_p[0]*y + z];

				}
				}
				}
				blockptr = _block;
			}
		}

		if (xbdry && _pad) {

			float *line_start;
			for (int z = 0; z<bs_p[2]; z++) {  
			for (int y = 0; y<bs_p[1]; y++) {  
				line_start = _block + (z*bs_p[1]*bs_p[0] + y*bs_p[0]);

				_pad_line(line_start, xstop, bs_p[0], 1);
			}
			}
		}

		if (ybdry && _pad) {

			float *line_start;
			for (int z = 0; z<bs_p[2]; z++) {  
			for (int x = 0; x<bs_p[0]; x++) {  
				line_start = _block + (z*bs_p[1]*bs_p[0] + x);

				_pad_line(line_start, ystop, bs_p[1], bs_p[0]);
			}
			}
		}

		if (zbdry && _pad) {

			float *line_start;
			for (int y = 0; y<bs_p[1]; y++) {  
			for (int x = 0; x<bs_p[0]; x++) {  
				line_start = _block + (y*bs_p[0] + x);

				_pad_line(line_start, zstop, bs_p[2], bs_p[0]*bs_p[1]);
			}
			}
		}


		_XFormTimerStart();
		int rc = _compressor->Decompose(
			blockptr, _cvector, _ncoeffs, _sigmaps, _ncoeffs.size()
		); 
		_XFormTimerStop();
		if (rc<0) return(-1);

		rc = _WriteBlock(bx, by, bz);
		if (rc<0) return(-1);
		
	}
	}
	}

	//
	// Compute valid region bounds
	// N.B. for 2D data the code below should be a no-op for the 3rd,
	// unused dimension.
	//
	for(int i=0; i<3; i++) {
		size_t t = bmin_up[i] * bs[i];
		if (_validRegMin[i] > t) _validRegMin[i] = t;

		t = min((((bmax_up[i]+1) * bs[i]) - 1), (dim[i]-1));
		if (_validRegMax[i] < t) _validRegMax[i] = t;
	}
	return(0);
}

int WaveCodecIO::WriteRegion(
	const float *region, const size_t min[3], const size_t max[3]
) {
	if (! _isOpen || ! _writeMode) {
		SetErrMsg("Variable not open for writing\n");
		return(-1);
	}

	size_t min_up[3], max_up[3];	// unpacked copy of min, max
	_UnpackCoord(_vtype, min, min_up, 0);
	_UnpackCoord(_vtype, max, max_up, 0);
	if (! IsValidRegion(min_up, max_up, _reflevel)) {
		SetErrMsg(
			"Invalid region : (%d %d %d) (%d %d %d)", 
			min[0], min[1], min[2], max[0], max[1], max[2]
		);
		return(-1);
	}

	//
	// unpacked and packed coordinates, respectively, of region in blocks
	//
	size_t bmin_up[3], bmax_up[3];
	size_t bmin_p[3], bmax_p[3];

	// packed coordinates of region 
	size_t min_p[3], max_p[3];

	// MapVoxToBlk operates on unpacked coordinates
	//
	MapVoxToBlk(min_up, bmin_up, _reflevel);
	MapVoxToBlk(max_up, bmax_up, _reflevel);

	_PackCoord(_vtype, bmin_up, bmin_p, 0);
	_PackCoord(_vtype, bmax_up, bmax_p, 0);

	//
	// fill in 3rd dimension (no-op if 3D data)
	//
	_FillPackedCoord(_vtype, min, min_p, 0);
	_FillPackedCoord(_vtype, max, max_p, 0);

	//
	// Dimension of region in voxels
	//
	size_t nx = max_p[0] - min_p[0] + 1; 
	size_t ny = max_p[1] - min_p[1] + 1; 

	size_t bs[3];
	size_t bs_p[3];	// packed copy of bs

	if (! _blockReg) {
		GetBlockSize(bs, -1);
		size_t sz = bs[0]*bs[1]*bs[2];
		_blockReg = new float [sz];
	}

	GetBlockSize(bs, _reflevel);
	_PackCoord(_vtype, bs, bs_p, 1);

	//
	// local copies of valid region extents
	//
	size_t myValidRegMin[3];
	size_t myValidRegMax[3];
	for(int i=0; i<3; i++) {
		myValidRegMin[i] = _validRegMin[i];
		myValidRegMax[i] = _validRegMax[i];
	}

	int rc;
	for (int bz=bmin_p[2]; bz<=bmax_p[2]; bz++) {
	for (int by=bmin_p[1]; by<=bmax_p[1]; by++) {
	for (int bx=bmin_p[0]; bx<=bmax_p[0]; bx++) {
		size_t b_p[] = {bx,by,bz};

		//
		// These flags are true if this is a boundary block && the region
		// dimensions don't coincide with block boundaries
		//
		bool xbdry = ((bx == bmin_p[0]) && (min_p[0] != 0)) || 
			((bx == bmax_p[0]) && ((bx+1)*bs_p[0] != (max_p[0]+1)));
		bool ybdry = ((by == bmin_p[1]) && (min_p[1] != 0)) || 
			((by == bmax_p[1]) && ((by+1)*bs_p[1] != (max_p[1]+1)));
		bool zbdry = ((bz == bmin_p[2]) && (min_p[2] != 0)) || 
			((bz == bmax_p[2]) && ((bz+1)*bs_p[2] != (max_p[2]+1)));



		//
		// Now copy current block to region, clipping block to 
		// region bounds
		//

		// Packed voxel coordinates of subregion within current 
		// block relative to volume
		//
		size_t minv_p[3], maxv_p[3];

		// Packed voxel coordinates of subregion within current 
		// block relative to current block
		//
		size_t minb_p[3], maxb_p[3];

		//
		// Set up coordinates for copy
		//
		for (int i=0; i<3; i++) {

			minb_p[i] = 0;
			maxb_p[i] = bs_p[i]-1;

			minv_p[i] = b_p[i] * bs_p[i];
			maxv_p[i] = (b_p[i]+1) * bs_p[i] - 1;

			// Clip to region bounds
			//
			if (minv_p[i] < min_p[i]) {
				minb_p[i] += min_p[i] - minv_p[i];
				minv_p[i] = min_p[i];
			}
			if (maxv_p[i] > max_p[i]) {
				maxb_p[i] -= maxv_p[i] - max_p[i];
				maxv_p[i] = max_p[i];
			}
		}

		//
		// Finally, copy data from block to region
		//
		for (int z=minb_p[2], zr=minv_p[2]-min_p[2]; z<=maxb_p[2]; z++,zr++){
		for (int y=minb_p[1], yr=minv_p[1]-min_p[1]; y<=maxb_p[1]; y++,yr++){
		for (int x=minb_p[0], xr=minv_p[0]-min_p[0]; x<=maxb_p[0]; x++,xr++){

			_blockReg[z*bs_p[0]*bs_p[1] + y*bs_p[0] + x] = 
				region[zr*nx*ny + yr*nx + xr];
		}
		}
		}

		//
		// Pad any incomplete dimensions
		//
		if (xbdry && _pad) {

			float *line_start;
			for (int z = 0; z<bs_p[2]; z++) {  
			for (int y = 0; y<bs_p[1]; y++) {  

				size_t l1 = maxb_p[0]-minb_p[0]+1;	// length of valid data
				size_t l2; 

				line_start = _blockReg + (z*bs_p[1]*bs_p[0] + y*bs_p[0]);

				line_start += minb_p[0];
				l2 = bs_p[0] - minb_p[0];
				_pad_line(line_start, l1, l2, 1);

				line_start += l1 - 1;
				l2 = maxb_p[0] + 1;
				_pad_line(line_start, l1, l2, -1);
			}
			}
		}

		if (ybdry && _pad) {

			float *line_start;
			for (int z = 0; z<bs_p[2]; z++) {  
			for (int x = 0; x<bs_p[0]; x++) {  
				line_start = _blockReg + (z*bs_p[1]*bs_p[0] + x);
				long stride = bs_p[0];

				size_t l1 = maxb_p[1]-minb_p[1]+1;	// length of valid data
				size_t l2;

				line_start += minb_p[1] * stride;
				l2 = bs_p[1] - minb_p[1];
				_pad_line(line_start, l1, l2, stride);

				line_start += (l1 - 1) * stride;
				l2 = maxb_p[1] + 1;
				_pad_line(line_start, l1, l2, -stride);
			}
			}
		}

		if (zbdry && _pad) {

			float *line_start;
			for (int y = 0; y<bs_p[1]; y++) {  
			for (int x = 0; x<bs_p[0]; x++) {  
				line_start = _blockReg + (y*bs_p[0] + x);
				long stride = bs_p[0] * bs_p[1];

				size_t l1 = maxb_p[2]-minb_p[2]+1;	// length of valid data
				size_t l2;

				line_start += minb_p[2] * stride;
				l2 = bs_p[2] - minb_p[2];
				_pad_line(line_start, l1, l2, stride);

				line_start += (l1 - 1) * stride;
				l2 = maxb_p[2] + 1;
				_pad_line(line_start, l1, l2, -stride);
			}
			}
		}

		//
		// Write current block
		//
		rc = BlockWriteRegion(_blockReg, b_p, b_p);
		if (rc<0) return(-1);

	}
	}
	}

	//
	// Update valid region. N.B. _validReg* gets written by 
	// BlockRegionWrite(), but this info is incorrect in the general 
	// case when the region extents aren't block aligned 
	// 
	for(int i=0; i<3; i++) {
		if (myValidRegMin[i] > min_p[i]) _validRegMin[i] = min_p[i];
		if (myValidRegMax[i] < max_p[i]) _validRegMax[i] = max_p[i];
	}

	return(0);
}

int WaveCodecIO::ReadSlice(
	float *slice
) {
	if (! _isOpen || _writeMode) {
		SetErrMsg("Variable not open for reading\n");
		return(-1);
	}

	size_t bdim[3];
	GetDimBlk(bdim,_reflevel);
	size_t bdim_p[3];
	_PackCoord(_vtype, bdim, bdim_p, 1);

	size_t dim[3];
	GetDim(dim, _reflevel);
	size_t dim_p[3];
	_PackCoord(_vtype, dim, dim_p, 1);

	size_t bs[3];
	GetBlockSize(bs, _reflevel);
	size_t bs_p[3];
	_PackCoord(_vtype, bs, bs_p, 1);

	size_t nnx, nny, nnz;	// dimensions of blocked slice (2d) or volume (3d)
							// in voxels


	size_t block_size = bs_p[0] * bs_p[1] * bs_p[2];

	nnx = bs_p[0]*bdim_p[0];
	nny = bs_p[1]*bdim_p[1]; 
	nnz = bs_p[2]*bdim_p[2]; 	// nnz==1 if 2D

	size_t bmin_p[] = {0,0,0};	// block coordinates of current slab
	size_t bmax_p[] = {0,0,0};
	for (int i=0; i<3; i++) bmax_p[i] = bdim_p[i] - 1;

	size_t slice_count;	// num slices read from current slab 
	if (_vtype == VAR3D) {
		slice_count = _sliceCount % bs_p[2];
		bmax_p[2] = bmin_p[2] = _sliceCount / bs_p[2];
	}
	else {
		slice_count = 0;
	}

	if (_sliceCount >= dim_p[2]) {
		return(0);	// no data to read
	}

	size_t sz = (bmax_p[0]+1) * (bmax_p[1]+1) * block_size;
	if (_sliceBufferSize < sz) {
		if (_sliceBuffer) delete [] _sliceBuffer;
		_sliceBuffer = new float[sz];
		_sliceBufferSize = sz;
	}

	if (_sliceCount % bs_p[2] == 0) {
		int rc = BlockReadRegion(bmin_p, bmax_p, _sliceBuffer, 1);
		if (rc<0) return(-1);
		if (_sliceCount == dim_p[2]) _sliceCount = 0;
	}

	for (int y = 0; y<dim_p[1]; y++) { 
		float *ptr = _sliceBuffer + (slice_count * nnx*nny + y*nnx);
		for (int x = 0; x<dim_p[0]; x++) { 
			*slice++ = *ptr++;
		}
	}

	_sliceCount++;

	return(0);	// read one slice
}

int WaveCodecIO::WriteSlice(
	const float *slice
) {
	if (! _isOpen || ! _writeMode) {
		SetErrMsg("Variable not open for writing\n");
		return(-1);
	}

	size_t bdim[3];
	GetDimBlk(bdim,-1);
	size_t bdim_p[3];
	_PackCoord(_vtype, bdim, bdim_p, 1);

	size_t bs[3]; 
	GetBlockSize(bs, -1);
	size_t bs_p[3];
	_PackCoord(_vtype, bs, bs_p, 1);

	size_t dim[3];
	GetDim(dim,-1);
	size_t dim_p[3];	// packed version of dim
	_PackCoord(_vtype, dim, dim_p, 1);

	size_t nnx, nny, nnz;	// dimensions of blocked slice in voxels
	size_t slice_count;	// num slices written from current slab 

	size_t block_size;

	nnx = bs_p[0]*bdim_p[0];
	nny = bs_p[1]*bdim_p[1]; 
	nnz = bs_p[2]*bdim_p[2]; 	// nnz==1 if 2D

	block_size = bs_p[0] * bs_p[1] * bs_p[2];

	size_t bmin_p[] = {0,0,0};
	size_t bmax_p[] = {0,0,0};
	for (int i=0; i<3; i++) bmax_p[i] = bdim_p[i] - 1;

	if (_vtype == VAR3D) {
		slice_count = _sliceCount % bs_p[2];
		bmax_p[2] = bmin_p[2] = _sliceCount / bs_p[2];
	}
	else {
		slice_count = 0;
	}

	if (_sliceCount >= dim_p[2]) {
		return(0);
	}

	size_t sz = (bmax_p[0]+1) * (bmax_p[1]+1) * block_size;
	if (_sliceBufferSize < sz) {
		if (_sliceBuffer) delete [] _sliceBuffer;
		_sliceBuffer = new float[sz];
		_sliceBufferSize = sz;
	}

	float *ptr;
	float *line_start;
	for (int y = 0; y<dim_p[1]; y++) {  
		line_start = _sliceBuffer + (slice_count * nnx*nny + y*nnx);
		ptr = line_start;

		for (int x = 0; x<dim_p[0]; x++) { 
			*ptr++ = *slice++;
		}
		// Pad if necessary (noop if not)
		//
		_pad_line(line_start, dim_p[0], nnx, 1);
	}

	for (int x = 0; x<nnx; x++) { 
		line_start = _sliceBuffer + (slice_count * nnx*nny + x);
		_pad_line(line_start, dim_p[1], nny, nnx);
	}
	_sliceCount++;

	if (_vtype != VAR3D) {
		bool pad = _pad; _pad = false;

		int rc = BlockWriteRegion(_sliceBuffer, bmin_p, bmax_p, 1);

		_pad = pad;
		if (rc<0) return(rc);
		return(0);
	}


	//
	// Pad Z dimension if last slice
	//
	if (_sliceCount == dim_p[2]) {
		line_start = _sliceBuffer;
		for (int y = 0; y<nny; y++) {
			for (int x = 0; x<nnx; x++) {
				_pad_line(line_start, dim_p[2]-(bmax_p[2]*bs_p[2]), bs_p[2], nnx*nny);
				line_start++;
			}
		}
	}
		
	if (_sliceCount % bs_p[2] == 0 || _sliceCount == dim_p[2]) {
		bool pad = _pad; _pad = false;

		int rc = BlockWriteRegion(_sliceBuffer, bmin_p, bmax_p, 1);

		_pad = pad;
		if (rc<0) return(-1);
	}
	return(0);
}

void    WaveCodecIO::GetValidRegion(
	size_t min[3], size_t max[3], int reflevel
) const {

	if (reflevel < 0) reflevel = GetNumTransforms();

	int  ldelta = GetNumTransforms() - reflevel;

	for (int i=0; i<3; i++) {
		min[i] = _validRegMin[i] >> ldelta;
		max[i] = _validRegMax[i] >> ldelta;
	}
}

int    WaveCodecIO::VariableExists(
	size_t timestep,
	const char *varname,
	int ,
	int lod
) const  {
	string basename;

//    if (reflevel < 0) reflevel = GetNumTransforms();

	if (lod < 0) lod = _cratios.size() - 1;

	if (lod >= _cratios.size()) return(0);

	if (ConstructFullVBase(timestep, varname, &basename) < 0) {
		SetErrCode(0);
		return (0);
	}

	for (int j = 0; j<=lod; j++){
		ostringstream oss;
		oss << basename << ".nc" << j;
		string path = oss.str();

		struct STAT64 statbuf;

		if (STAT64(path.c_str(), &statbuf) < 0) return (0);
	}
	return(1);
}


int WaveCodecIO::GetNumTransforms() const {

	size_t ntotal = _compressor3D->GetNumWaveCoeffs();
	const size_t *bs =  VDFIOBase::GetBlockSize();

	//
	// If a non-symmetric wavelet/boundary pair or a non-period 
	// boundary are used the number of approximation coefficients
	// at each each level will be greater than half the number of coefficients
	// at the preceding level
	//
	if (ntotal == bs[0]*bs[1]*bs[2]) {
		return(_compressor3D->GetNumLevels());
	}
	else {
		return(0);
	}
}

void WaveCodecIO::GetBlockSize(size_t bs[3], int reflevel) const {

	if (reflevel < 0) reflevel = GetNumTransforms();
	if (reflevel > GetNumTransforms()) reflevel = GetNumTransforms();

	vector <size_t> bsvec;
	_compressor3D->GetDimension(bsvec, reflevel);
	for (int i=0; i<3; i++) bs[i] = bsvec[i];

}

size_t WaveCodecIO::GetMaxCRatio(
	const size_t bs[3], 
	string wavename, string wmode
) {
	vector <size_t> dims;
	for (int i=0; i<3; i++) {
		dims.push_back(bs[i]);
	}
	Compressor *cmp = new Compressor(dims, wavename, wmode);
	if (Compressor::GetErrCode() != 0) return(0);

	// Total number of wavelet coefficients in a forward transform
	//
	size_t ntotal = cmp->GetNumWaveCoeffs();

	size_t mincoeff = cmp->GetMinCompression();
	if (mincoeff == 0) return(0);

	return(ntotal / mincoeff);
}

int WaveCodecIO::_SetupCompressor() {

	if (_vtype == VARUNKNOWN) {
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	if (_compressorType == _vtype) return(0);

	string wavename = GetWaveName();
	string boundary_mode = GetBoundaryMode();
	const size_t *bs =  VDFIOBase::GetBlockSize();

	vector <size_t> bdims;

	switch (_vtype) {
	case VAR2D_XY:
		bdims.push_back(bs[0]);
		bdims.push_back(bs[1]);
		_cratios = _cratios2D;
		if (! _compressor2DXY) {
			_compressor2DXY = new Compressor(bdims, wavename, boundary_mode);
		}
		_compressor = _compressor2DXY;
	break;

	case VAR2D_XZ:
		bdims.push_back(bs[0]);
		bdims.push_back(bs[2]);
		_cratios = _cratios2D;
		if (! _compressor2DXZ) {
			_compressor2DXZ = new Compressor(bdims, wavename, boundary_mode);
		}
		_compressor = _compressor2DXZ;
	break;

	case VAR2D_YZ:
		bdims.push_back(bs[1]);
		bdims.push_back(bs[2]);
		_cratios = _cratios2D;
		if (! _compressor2DYZ) {
			_compressor2DYZ = new Compressor(bdims, wavename, boundary_mode);
		}
		_compressor = _compressor2DYZ;
	break;

	case VAR3D:
		bdims.push_back(bs[0]);
		bdims.push_back(bs[1]);
		bdims.push_back(bs[2]);
		_cratios = _cratios3D;
		if (! _compressor3D) {
			_compressor3D = new Compressor(bdims, wavename, boundary_mode);
		}
		_compressor = _compressor3D;
	break;

	default:
		assert(0);
	}

	if (Compressor::GetErrCode() != 0) {
		SetErrMsg("Failed to allocate compressor");
		return(-1);
	}
	_compressorType = _vtype;


	// Total number of wavelet coefficients in a forward transform
	//
	size_t ntotal = _compressor->GetNumWaveCoeffs();

	size_t mincoeff = _compressor->GetMinCompression();
	assert(mincoeff >0);

	for (int i=0; i<_cratios.size(); i++) {
		size_t n = ntotal / _cratios[i];
		if (n < mincoeff) {
			SetErrMsg(
				"Invalid compression ratio for configuration : %d", 
				_cratios[i]
			);
			return(-1);
		}
	}

	_ncoeffs.clear();
	_sigmapsizes.clear();
	size_t naccum = 0;
	size_t saccum = 0;
	for (int i = 0; i < _cratios.size(); i++) {
		if (_cratios[i] > ntotal) break;

		size_t n = ntotal / _cratios[i] - naccum;  
		// what happens when n == 0?

		_ncoeffs.push_back(n);
		naccum += n;

		size_t s = _compressor->GetSigMapSize(n);
		_sigmapsizes.push_back(s);
		saccum += s;

		vector <size_t> sigdims;
		_compressor->GetSigMapShape(sigdims);
		_sigmaps[i]->Reshape(sigdims);
		
	}
	assert(naccum <= ntotal); 

	if (_cvectorsize < naccum) {
		if (_cvector) delete [] _cvector;
		_cvector = new float[naccum];
		_cvectorsize = naccum;
	}

	if (_svectorsize < saccum) {
		if (_svectorsize) delete [] _svector;
		_svector = new unsigned char[saccum];
		_svectorsize = saccum;
	}

	return(0);
}


int WaveCodecIO::_OpenVarRead(
	const string &basename
) {

	//
	// Create a netCDF file for each refinement level
	//
	string wname = GetWaveName();
	string wmode = GetBoundaryMode();

	_ncpaths.clear();
	_ncids.clear();
	_nc_wave_vars.clear();
	_nc_sig_vars.clear();

	int ncdimid;
	size_t ncdim;

	for(int j=0; j<=_lod; j++) {
		int rc;

		ostringstream oss;
        oss << basename << ".nc" << j;
		string path = oss.str();

		_ncpaths.push_back(path);


		size_t chsz = NC_CHUNKSIZEHINT;
		int ncid;
		int ii = 0;
		do {
			rc = nc__open(path.c_str(), NC_NOWRITE, &chsz, &ncid);
#ifdef WIN32
			if (rc == EAGAIN) Sleep(100);//milliseconds
#else
			if (rc == EAGAIN) sleep(1);//seconds
#endif
			ii++;

		} while (rc != NC_NOERR && ii < 10);
		NC_ERR_READ(rc,path)
		_ncids.push_back(ncid);


		//
		// Verify coefficient file metadata matches VDF metadata
		//
		size_t bdim[3];
		VDFIOBase::GetDimBlk(bdim,-1);

        rc = nc_inq_dimid(_ncids[j], _volumeDimNbxName.c_str(), &ncdimid);
		NC_ERR_READ(rc,path)
        rc = nc_inq_dimlen(_ncids[j], ncdimid, &ncdim);
        NC_ERR_READ(rc,path)

        if (ncdim != bdim[0]) {
            SetErrMsg("Metadata file and data file mismatch");
            return(-1);
        }

        rc = nc_inq_dimid(_ncids[j], _volumeDimNbyName.c_str(), &ncdimid);
		NC_ERR_READ(rc,path)
        rc = nc_inq_dimlen(_ncids[j], ncdimid, &ncdim);
        NC_ERR_READ(rc,path)

        if (ncdim != bdim[1]) {
            SetErrMsg("Metadata file and data file mismatch");
            return(-1);
		}

        rc = nc_inq_dimid(_ncids[j], _volumeDimNbzName.c_str(), &ncdimid);
		NC_ERR_READ(rc,path)
        rc = nc_inq_dimlen(_ncids[j], ncdimid, &ncdim);
        NC_ERR_READ(rc,path)

        if (ncdim != bdim[2]) {
            SetErrMsg("Metadata file and data file mismatch");
            return(-1);
		}

        rc = nc_inq_dimid(_ncids[j], _coeffDimName.c_str(), &ncdimid);
		NC_ERR_READ(rc,path)
        rc = nc_inq_dimlen(_ncids[j], ncdimid, &ncdim);
        NC_ERR_READ(rc,path)
        if (ncdim != _ncoeffs[j]) {
            SetErrMsg("Metadata file and data file mismatch");
            return(-1);
		}

        rc = nc_inq_dimid(_ncids[j], _sigMapDimName.c_str(), &ncdimid);
		NC_ERR_READ(rc,path)
        rc = nc_inq_dimlen(_ncids[j], ncdimid, &ncdim);
        NC_ERR_READ(rc,path)
        if (ncdim != _sigmapsizes[j]) {
            SetErrMsg("Metadata file and data file mismatch");
            return(-1);
		}

		rc = nc_get_att_float(
			_ncids[j],NC_GLOBAL,_scalarRangeName.c_str(),_dataRange
		);
		NC_ERR_READ(rc,path)

		int minreg_int[3];
		rc = nc_get_att_int(
			_ncids[j],NC_GLOBAL,_minValidRegionName.c_str(),minreg_int
		);
		if (rc == NC_NOERR) {
			for(int i=0; i<3; i++) _validRegMin[i] = minreg_int[i];
		}

		int maxreg_int[3];
		rc = nc_get_att_int(
			_ncids[j],NC_GLOBAL,_maxValidRegionName.c_str(),maxreg_int
		);
		if (rc == NC_NOERR) {
			for(int i=0; i<3; i++) _validRegMax[i] = maxreg_int[i];
		}

		int ncvar;
		rc = nc_inq_varid(_ncids[j], _waveletCoeffName.c_str(), &ncvar);
		NC_ERR_READ(rc,path)
		_nc_wave_vars.push_back(ncvar);

		bool reconstruct_sigmap = 
			 ((_cratios.size() == (j+1)) && (_cratios[j] == 1));

		if (! reconstruct_sigmap) {
			rc = nc_inq_varid(_ncids[j], _sigMapName.c_str(), &ncvar);
			NC_ERR_READ(rc,path)
			_nc_sig_vars.push_back(ncvar);
		}

	}
	return(0);
		
}

int WaveCodecIO::_OpenVarWrite(
	const string &basename
) {

	//
	// Create a netCDF file for each refinement level
	//
	size_t dim[3];
	GetDim(dim, -1);
	size_t bs[3]; 
	GetBlockSize(bs, -1);

	string wname = GetWaveName();
	string wmode = GetBoundaryMode();

	_ncpaths.clear();
	_ncids.clear();
	_nc_wave_vars.clear();
	_nc_sig_vars.clear();
	for(int j=0; j<=_lod; j++) {
		int rc;

		ostringstream oss;
        oss << basename << ".nc" << j;
		string path = oss.str();

		_ncpaths.push_back(path);

		size_t chsz = NC_CHUNKSIZEHINT;
		int ncid;
		rc = nc__create(path.c_str(), NC_64BIT_OFFSET, 0, &chsz, &ncid);
		NC_ERR_WRITE(rc,path)
		_ncids.push_back(ncid);
	
		// Disable data filling - may not be necessary
		//
		int mode;
		nc_set_fill(_ncids[j], NC_NOFILL, &mode);

		//
		// Define netCDF dimensions
		//
		int vol_dim_ids[3];
		size_t bdim[3];
		VDFIOBase::GetDimBlk(bdim,-1);
		rc = nc_def_dim(
			_ncids[j],_volumeDimNbxName.c_str(),bdim[0],&vol_dim_ids[0]
		);
		NC_ERR_WRITE(rc,path)

		rc = nc_def_dim(
			_ncids[j],_volumeDimNbyName.c_str(),bdim[1],&vol_dim_ids[1]
		);
		NC_ERR_WRITE(rc,path)

		rc = nc_def_dim(
			_ncids[j],_volumeDimNbzName.c_str(),bdim[2],&vol_dim_ids[2]
		);
		NC_ERR_WRITE(rc,path)

		int coeff_dim_id;
		rc = nc_def_dim(
			_ncids[j],_coeffDimName.c_str(),_ncoeffs[j], &coeff_dim_id
		);
		NC_ERR_WRITE(rc,path)

		int sig_map_dim_id;
		rc = nc_def_dim(
			_ncids[j],_sigMapDimName.c_str(),_sigmapsizes[j], &sig_map_dim_id
		);
		NC_ERR_WRITE(rc,path)

		int wave_dim_ids[4];
		int sig_dim_ids[4];
		int ndims;
		bool reconstruct_sigmap = 
				 ((_cratios.size() == (j+1)) && (_cratios[j] == 1));

		switch (_vtype) {
		case VAR2D_XY:
			ndims = 3;
			wave_dim_ids[0] = vol_dim_ids[1];
			wave_dim_ids[1] = vol_dim_ids[0];
			wave_dim_ids[2] = coeff_dim_id;

			sig_dim_ids[0] = vol_dim_ids[1];
			sig_dim_ids[1] = vol_dim_ids[0];
			sig_dim_ids[2] = sig_map_dim_id;
		break;
		case VAR2D_XZ:
			ndims = 3;
			wave_dim_ids[0] = vol_dim_ids[2];
			wave_dim_ids[1] = vol_dim_ids[0];
			wave_dim_ids[2] = coeff_dim_id;

			sig_dim_ids[0] = vol_dim_ids[2];
			sig_dim_ids[1] = vol_dim_ids[0];
			sig_dim_ids[2] = sig_map_dim_id;
		break;
		case VAR2D_YZ:
			ndims = 3;
			wave_dim_ids[0] = vol_dim_ids[2];
			wave_dim_ids[1] = vol_dim_ids[1];
			wave_dim_ids[2] = coeff_dim_id;

			sig_dim_ids[0] = vol_dim_ids[2];
			sig_dim_ids[1] = vol_dim_ids[1];
			sig_dim_ids[2] = sig_map_dim_id;
		break;
		case VAR3D:
			ndims = 4;
			wave_dim_ids[0] = vol_dim_ids[2];
			wave_dim_ids[1] = vol_dim_ids[1];
			wave_dim_ids[2] = vol_dim_ids[0];
			wave_dim_ids[3] = coeff_dim_id;

			sig_dim_ids[0] = vol_dim_ids[2];
			sig_dim_ids[1] = vol_dim_ids[1];
			sig_dim_ids[2] = vol_dim_ids[0];
			sig_dim_ids[3] = sig_map_dim_id;
		break;
		default:
			assert(0);
		}

		rc = nc_def_var(
			_ncids[j], _waveletCoeffName.c_str(), NC_FLOAT,
			ndims, wave_dim_ids, &ncid
		);
		NC_ERR_WRITE(rc,path)
		_nc_wave_vars.push_back(ncid);

		if (! reconstruct_sigmap) {
			rc = nc_def_var(
				_ncids[j], _sigMapName.c_str(), NC_BYTE,
				ndims, sig_dim_ids, &ncid
			);
			NC_ERR_WRITE(rc,path)
			_nc_sig_vars.push_back(ncid);
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
			_ncids[j],NC_GLOBAL,_compressionLevelName.c_str(),NC_INT, 1, &j
		);
		NC_ERR_WRITE(rc,path)


		int dim_int[] = {(int) dim[0], (int) dim[1], (int) dim[2]};
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_nativeResName.c_str(),NC_INT, 3, dim_int
		);
		NC_ERR_WRITE(rc,path)

		int bs_int[] = {(int) bs[0],(int) bs[1],(int) bs[2]};
        rc = nc_put_att_int(
            _ncids[j],NC_GLOBAL,_blockSizeName.c_str(),NC_INT, 3, bs_int
        );
        NC_ERR_WRITE(rc,path)

		int minreg_int[3] = {_validRegMin[0],_validRegMin[1],_validRegMin[2]};
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_minValidRegionName.c_str(),NC_INT, 3, 
			minreg_int
		);
		NC_ERR_WRITE(rc,path)

		int maxreg_int[3] = {_validRegMax[0],_validRegMax[1],_validRegMax[2]};
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_maxValidRegionName.c_str(),NC_INT, 3, 
			maxreg_int
		);
		NC_ERR_WRITE(rc,path)

		rc = nc_put_att_float(
			_ncids[j],NC_GLOBAL,_scalarRangeName.c_str(),NC_FLOAT, 2, _dataRange
		);
		NC_ERR_WRITE(rc,path)

		rc = nc_put_att_text(
			_ncids[j],NC_GLOBAL,_waveletName.c_str(), wname.length(), 
			wname.c_str()
		);
		NC_ERR_WRITE(rc,path)

		rc = nc_put_att_text(
			_ncids[j],NC_GLOBAL,_boundaryModeName.c_str(), wmode.length(), 
			wmode.c_str()
		);
		NC_ERR_WRITE(rc,path)

		rc = nc_enddef(_ncids[j]);
		NC_ERR_WRITE(rc,path)

	}
	return(0);
		
}

int WaveCodecIO::_FetchBlock(
	size_t bx, size_t by, size_t bz
) {
	float *cvectorptr = _cvector;
	unsigned char *svectorptr = _svector;
	int rc;

	_ReadTimerStart();

	size_t start[] = {0,0,0,0};
	size_t wcount[] = {1,1,1,1};
	size_t scount[] = {1,1,1,1};

	for(int j=0; j<=_lod; j++) {

		bool reconstruct_sigmap = 
				 ((_cratios.size() == (j+1)) && (_cratios[j] == 1));

		if (_vtype == VAR3D) {
			start[0] = bz;
			start[1] = by;
			start[2] = bx;
			wcount[3] = _ncoeffs[j];
			scount[3] = _sigmapsizes[j];
		}
		else {
			start[0] = by;
			start[1] = bx;
			wcount[2] = _ncoeffs[j];
			scount[2] = _sigmapsizes[j];
		}

		rc = nc_get_vars_float(
			_ncids[j], _nc_wave_vars[j], start, wcount, NULL, cvectorptr
		);
		NC_ERR_READ(rc, _ncpaths[j]);
		cvectorptr += _ncoeffs[j];

		if (! reconstruct_sigmap) {
			rc = nc_get_vars_uchar(
				_ncids[j], _nc_sig_vars[j], start, scount, NULL, svectorptr
			);
			NC_ERR_READ(rc, _ncpaths[j]);

			rc = _sigmaps[j]->SetMap(svectorptr);
			if (rc<0) {
				SetErrMsg("Error reading data");
				return (-1);
			}
		}
		else {
			// 
			// Reconstruct the last signficiance map from all the previous
			// ones.
			//
			_sigmaps[j]->Clear();
			for (int i=0; i<j; i++) {
				_sigmaps[j]->Append(*_sigmaps[i]);
			}
			_sigmaps[j]->Sort();
			_sigmaps[j]->Invert();
		}

		svectorptr += _sigmapsizes[j];
	}

	_ReadTimerStop();

	return(0);
}

int WaveCodecIO::_WriteBlock(
	size_t bx, size_t by, size_t bz
) {
	const float *cvectorptr = _cvector;
	int rc;

	_WriteTimerStart();

	size_t start[] = {0,0,0,0};
	size_t wcount[] = {1,1,1,1};
	size_t scount[] = {1,1,1,1};

	for(int j=0; j<=_lod; j++) {

		const unsigned char *map;
		size_t maplen;
		_sigmaps[j]->GetMap(&map, &maplen);
		assert(maplen == _sigmapsizes[j]);

		bool reconstruct_sigmap = 
			 ((_cratios.size() == (j+1)) && (_cratios[j] == 1));

		if (_vtype == VAR3D) {
			start[0] = bz;
			start[1] = by;
			start[2] = bx;
			wcount[3] = _ncoeffs[j];
			scount[3] = _sigmapsizes[j];
		}
		else {
			start[0] = by;
			start[1] = bx;
			wcount[2] = _ncoeffs[j];
			scount[2] = _sigmapsizes[j];
		}

		rc = nc_put_vars_float(
			_ncids[j], _nc_wave_vars[j], start, wcount, NULL, cvectorptr
		);
		NC_ERR_WRITE(rc, _ncpaths[j]);
		cvectorptr += _ncoeffs[j];

		//
		// To save space we don't store the final signficiance map
		// if it coresponds to a full reconstuction of the 
		// data (cratio == 1).
		//
		if (! reconstruct_sigmap) {
			rc = nc_put_vars_uchar(
				_ncids[j], _nc_sig_vars[j], start, scount, NULL, map
			);
			NC_ERR_WRITE(rc, _ncpaths[j]);
		}
	}

	_WriteTimerStop();

	return(0);
}

void WaveCodecIO::_pad_line(
	float *line_start, 
	size_t l1,	// length of valid data
	size_t l2,	// total length of array
	long stride
) const {
	string mode = GetBoundaryMode();
	float *ptr = line_start + ((long) l1 * stride);

	long index;
	int inc;

	assert(l1>0 && stride != 0);
	if (l1==l2) return;

	if (l1 == 1) {
		int dir = stride < 0 ? -1 : 1;
		for (size_t l=l1; l<l2; l+=dir) {
			*ptr = *line_start;
			ptr += stride;
		}
	}
		

	//
	// Symmetric-halfpoint. If a signal looks like ABCDE, the extended signal
	// looks like:
	//
	//	EEDCBAABCDEEDCBAA
	//	      ^^^^^
	//
	if (mode.compare("symh") == 0) {
		index = (long) l1 - 1;
		inc = 0;

		for (size_t l=l1; l<l2; l++) {
			*ptr = line_start[(size_t) index * stride];
			ptr += stride;
			if (index == 0) {
				if (inc == 0) inc = 1;
				else inc = 0;
			}
			else if (index == (long) l1 - 1) {
				if (inc == 0) inc = -1;
				else inc = 0;
			}
			index += inc;
		}
	}
	//
	// Symmetric-wholepoint. If a signal looks like ABCDE, the extended signal
	// looks like:
	//
	//	DEDCBABCDEDCBAB
	//	     ^^^^^
	else if (mode.compare("symw") == 0) {
		index = (long) l1 - 2;
		inc = -1;

		for (size_t l=l1; l<l2; l++) {
			*ptr = line_start[(size_t) index * stride];
			ptr += stride;
			if (index == 0) {
				inc = 1;
			}
			else if (index == (long) l1 - 1) {
				inc = -1;
			}
			index += inc;
		}
	}
	//
	// Periodic. If a signal looks like ABCDE, the extended signal
	// looks like:
	//
	//	...ABCDABCDEABCDABCD...
	//	       ^^^^^
	else if (mode.compare("per") == 0) {
		index = (int) 0;

		for (size_t l=l1; l<l2; l++) {
			*ptr = line_start[(size_t) index * stride];
			ptr += stride;
			index++;
		}
	}
	//
	// SP0. If a signal looks like ABCDE, the extended signal
	// looks like:
	//
	//	...AAAAABCDEEEEE...
	//	       ^^^^^
	else if (mode.compare("sp0") == 0) {
		index = (long) l1-1;

		for (size_t l=l1; l<l2; l++) {
			*ptr = line_start[(size_t) index * stride];
			ptr += stride;
		}
	}
	// Default to sp0
	//
	else {
		index = (long) l1-1;

		for (size_t l=l1; l<l2; l++) {
			*ptr = line_start[(size_t) index * stride];
			ptr += stride;
		}
	}
}

void WaveCodecIO::_UnpackCoord(
	VarType_T vtype, const size_t src[3], size_t dst[3], size_t fill
) const {

	switch (_vtype) { 
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

void WaveCodecIO::_PackCoord(
	VarType_T vtype, const size_t src[3], size_t dst[3], size_t fill
) const {
	switch (_vtype) { 
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

void WaveCodecIO::_FillPackedCoord(
	VarType_T vtype, const size_t src[3], size_t dst[3], size_t fill
) const {
	switch (_vtype) { 
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
