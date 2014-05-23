

#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <cstdlib>
#include <cfloat>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#ifdef PARALLEL
#include <mpi.h>
#else
namespace {
double MPI_Wtime() { return(0.0); };
}
#endif

#ifdef PNETCDF
#include "pnetcdf.h"
#else
#include <netcdf.h>
#endif
#include <vapor/WaveCodecIO.h>


using namespace std;
using namespace VAPoR;

//
// ncdf Dimension names
//
const string _volumeDimNbxName = "VolumeDimNbx";	// dim of volume in blocks
const string _volumeDimNbyName = "VolumeDimNby";
const string _volumeDimNbzName = "VolumeDimNbz";
const string _coeffDimName = "CoeffDim";	// num. coeff per block

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

#ifdef PARALLEL
extern "C" int
nc_create_par(const char *path, int cmode, MPI_Comm comm, MPI_Info info,
	      int *ncidp);
#endif


#ifdef PNETCDF
#define NC_ERR_WRITE(rc, path) \
    if (rc != NC_NOERR) { \
        SetErrMsg( \
            "Error writing PnetCDF file \"%s\" : %s",  \
            path.c_str(), ncmpi_strerror(rc) \
        ); \
        return(-1); \
    }

#define NC_ERR_READ(rc, path) \
    if (rc != NC_NOERR) { \
        SetErrMsg( \
            "Error reading PnetCDF file \"%s\" : %s",  \
            path.c_str(), ncmpi_strerror(rc) \
        ); \
        return(-1); \
    }

#else

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

#endif // PNETCDF


#define	NC_FLOAT_SZ 4

void swapbytes(void *ptr, size_t ws, size_t nelem) {

	for (size_t i = 0; i<nelem; i++) {
		unsigned char *uptr = ((unsigned char *) ptr) + (i*ws);

		unsigned char *p1 = uptr;
		unsigned char *p2 = uptr + ws-1;
		unsigned char t;
		for (int j = 0; j< ws >> 1; j++) {
			t = *p1;
			*p1 = *p2;
			*p2 = t;
			p1++;
			p2--;
		}
	}
}


int WaveCodecIO::_WaveCodecIO(int nthreads) {

	if (nthreads < 1) nthreads = EasyThreads::NProc();
	if (nthreads < 1) nthreads = 1;

	_nthreads = nthreads;
	_threadStatus = 0;	// global thread status
	_compressorThread.resize(_nthreads, NULL);
	_compressorThread3D.resize(_nthreads, NULL);
	_compressorThread2DXY.resize(_nthreads, NULL);
	_compressorThread2DXZ.resize(_nthreads, NULL);
	_compressorThread2DYZ.resize(_nthreads, NULL);
	_compressor = NULL;
	_compressor3D = NULL;
	_compressor2DXY = NULL;
	_compressor2DXZ = NULL;
	_compressor2DYZ = NULL;
	_compressorType = VARUNKNOWN;
	_NC_BUF_SIZE = NC_CHUNKSIZEHINT;
	_vtype = VARUNKNOWN;

	_cvector = NULL;
	_cvectorThread.resize(_nthreads, NULL);
	_cvectorsize = 0;
	_svector = NULL;
	_svectorThread.resize(_nthreads, NULL);
	_svectorsize = 0;
	_sliceCount = 0;
	_sliceBufferSize = 0;
	_sliceBuffer = NULL;
	_isOpen = false;
	_pad = true;
	_rw_thread_objs = NULL;

	_cratios3D = GetCRatios();
	_cratios = _cratios3D;
	_collectiveIO = false;
	_xformMPI = 0.0;
	_methodTimer = 0.0;
	_methodThreadTimer = 0.0;
	_ioMPI = 0.0;


	//
	// Try to compute "reasonable" 2D compression ratios from 3D 
	// compression ratios
	//
	_cratios2D.clear();
	for (int i=0; i<_cratios3D.size(); i++) {
		size_t c = (size_t) pow((double) _cratios3D[i], (double) (2.0 / 3.0));
		_cratios2D.push_back(c);
	}

	
	//
	// One collection of sigmaps for each thread, each collection contains
	// a signficance map for each compression level
	//
	_sigmapsThread.resize(_nthreads);
	for (int t=0; t<_nthreads; t++) {
		_sigmapsThread[t].resize(_cratios.size());
	}
	const size_t *bs =  VDFIOBase::GetBlockSize();

	//
	// Don't allocate storage for blocks until later
	//
	_block = NULL;
	_blockThread.resize(_nthreads, NULL);
	_blockReg = NULL;

	//
	// Set up the default compressor - a 3D one for each thread.
	//
	vector <size_t> bdims;
	bdims.push_back(bs[0]);
	bdims.push_back(bs[1]);
	bdims.push_back(bs[2]);
	string wavename = GetWaveName();
	string boundary_mode = GetBoundaryMode();

	for (int t=0; t<_nthreads; t++) {
		_compressor3D = new Compressor(bdims, wavename, boundary_mode);
		if (Compressor::GetErrCode() != 0) {
			SetErrMsg("Failed to allocate compressor");
			return(-1);
		}
		_compressorThread[t] = _compressor3D;
		_compressorThread3D[t] = _compressor3D;
	}
	_compressor = _compressor3D;

	//
	// Finally, create thread read objects
	//
	_rw_thread_objs = new ReadWriteThreadObj * [_nthreads];
	for (int t=0; t<_nthreads; t++) _rw_thread_objs[t] = NULL;


	return(0);
}

WaveCodecIO::WaveCodecIO(
	const MetadataVDC &metadata,
	int nthreads
) : VDFIOBase(metadata), EasyThreads(nthreads) {

	if (VDFIOBase::GetErrCode()) return;

	if (_WaveCodecIO(GetNumThreads()) < 0) return;
}

WaveCodecIO::WaveCodecIO(
	const string &metafile,
	int nthreads
) : VDFIOBase(metafile) , EasyThreads(nthreads) {

	if (VDFIOBase::GetErrCode()) return;

	if (_WaveCodecIO(GetNumThreads()) < 0) return;
}


#ifdef	DEAD
WaveCodecIO::WaveCodecIO(
	const size_t dim[3], const size_t bs[3], int numTransforms, 
	const vector <size_t> cratios,
	const string &wname, const string &filebase
) {
	_cratios3D = GetCRatios();

	// validity check on cratios
}

WaveCodecIO(const vector <string> &files);
#endif

WaveCodecIO::~WaveCodecIO() {
	if (_cvector) delete [] _cvector;
	if (_svector) delete [] _svector;
	if (_compressor3D) delete _compressor3D;
	if (_compressor2DXY) delete _compressor2DXY;
	if (_compressor2DXZ) delete _compressor2DXZ;
	if (_compressor2DYZ) delete _compressor2DYZ;
	if (_block) delete [] _block;
	if (_blockReg) delete [] _blockReg;
	if (_sliceBuffer) delete [] _sliceBuffer;

	//
	// Free all but the last object contained in these vectors, which has
	// already been freed above
	//
	for (int t = 0; t<_nthreads-1; t++) {
		if (_cvectorThread[t]) delete [] _cvectorThread[t];
		if (_svectorThread[t]) delete [] _svectorThread[t];
		if (_compressorThread3D[t]) delete _compressorThread3D[t];
		if (_compressorThread2DXY[t]) delete _compressorThread2DXY[t];
		if (_compressorThread2DXZ[t]) delete _compressorThread2DXZ[t];
		if (_compressorThread2DYZ[t]) delete _compressorThread2DYZ[t];
		if (_blockThread[t]) delete [] _blockThread[t];
	}
	if (_rw_thread_objs) delete [] _rw_thread_objs;
}


int WaveCodecIO::OpenVariableRead(
	size_t timestep, const char *varname, int reflevel, int lod
) {
	string basename;

	if (CloseVariable() < 0) return(-1); 

	(void) VDFIOBase::OpenVariableRead(timestep, varname, reflevel, lod);


	//
	// no-op unless missing data are present
	//
	if (_MaskOpenRead(timestep, varname, reflevel) < 0) return(-1);

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

	//
	// Clamp the range of the reconstructed values to the range of the
	// original data values
	//
	for (int t=0; t<_nthreads; t++) {
		_compressorThread[t]->ClampMinOnOff() = true;
		_compressorThread[t]->ClampMaxOnOff() = true;

		_compressorThread[t]->ClampMin() = _dataRange[0];
		_compressorThread[t]->ClampMax() = _dataRange[1];
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

	(void) VDFIOBase::OpenVariableWrite(timestep, varname, reflevel, lod);

	//
	// no-op unless missing data are present
	//
	if (_MaskOpenWrite(timestep, varname, reflevel) < 0) return(-1);

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
	Metadata::GetDim(dim, -1);
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

	_dataRange[0] = FLT_MAX;
	_dataRange[1] = -FLT_MAX;

	_writeMode = true;

	_timeStep = timestep;
	_varName.assign(varname);
	_reflevel = reflevel;
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

// Collective call: all io are expected to call WITH SAME ARGS
// Calculates the appropriate buffer size for each IO task,
// assuming that the entire global block is being written out
void WaveCodecIO::EnableBuffering(size_t count[3], size_t divisor, int rank){

  _NC_BUF_SIZE = count[0] * count[1] * count[2] * sizeof(float) / divisor;
#ifdef PIOVDC_DEBUG
   if(_NC_BUF_SIZE <= 0)
    {
      printf("Enable @Buffering rank = %d NC_BUF_SIZE = %d count = %d-%d-%d divisor: %d\n",rank,  _NC_BUF_SIZE, count[0], count[1], count[2], divisor);
    }
#endif
}

int WaveCodecIO::CloseVariable() {

	if (! _isOpen) return(0);

	for (int j=0; j<_ncbufs.size(); j++) {
		int rc = _ncbufs[j]->Flush();
		NC_ERR_WRITE(rc, _ncpaths[j]);
		delete _ncbufs[j];
	}
	_ncbufs.clear();

	_WriteTimerStart();
#ifdef PARALLEL
	int rank;
	MPI_Comm_rank(_IO_Comm, &rank);
int x[] = {0,0,0};
 float *y = (float*) malloc(sizeof(float));
 int *reduce = (int*) malloc(sizeof(int) *3);
int *value = (int*) malloc(sizeof(int) *3);

 value[0] = _validRegMin[0];
 value[1] = _validRegMin[1];
 value[2] = _validRegMin[2];
MPI_Allreduce(value, reduce, 3, MPI_INT, MPI_MIN, _IO_Comm);
_validRegMin[0] = reduce[0]; _validRegMin[1] = reduce[1]; _validRegMin[2] = reduce[2];

 value[0] = _validRegMax[0];
 value[1] = _validRegMax[1];
 value[2] = _validRegMax[2];
MPI_Allreduce(value, reduce, 3, MPI_INT, MPI_MAX, _IO_Comm);
_validRegMax[0] = reduce[0]; _validRegMax[1] = reduce[1]; _validRegMax[2] = reduce[2];

 free(reduce);
 free(value);

MPI_Allreduce(&_dataRange[1], y, 1, MPI_FLOAT, MPI_MAX, _IO_Comm);
 _dataRange[1] = *y;
  MPI_Allreduce(&_dataRange[0], y, 1, MPI_FLOAT, MPI_MIN, _IO_Comm);
_dataRange[0] = *y;

SetDiagMsg("@MPI validreg min: %d %d %d, max: %d %d %d",_validRegMin[0],_validRegMin[1],_validRegMin[2],_validRegMax[0],_validRegMax[1],_validRegMax[2]);

#endif

#ifndef NOIO
	for(int j=0; j<=_lod && j<_ncids.size(); j++) {

		

		if (_ncids[j] > -1) {
			int rc; 

#ifndef NO_NC_ATTS

			if (_writeMode) {
#ifdef PNETCDF
			  rc = ncmpi_put_att_float(_ncids[j], NC_GLOBAL, _scalarRangeName.c_str(),	NC_FLOAT, (MPI_Offset)2,_dataRange);
#else
				rc = nc_put_att_float(
					_ncids[j], NC_GLOBAL, _scalarRangeName.c_str(), 
					NC_FLOAT, 2,_dataRange
				);
#endif
				NC_ERR_WRITE(rc,_ncpaths[j])

				int minreg_int[] = {
					_validRegMin[0], _validRegMin[1], _validRegMin[2]
				};
#ifdef PNETCDF
				rc = ncmpi_put_att_int(_ncids[j],NC_GLOBAL,_minValidRegionName.c_str(), NC_INT, (MPI_Offset)3, minreg_int);

#else
				rc = nc_put_att_int(
					_ncids[j],NC_GLOBAL,_minValidRegionName.c_str(),
					NC_INT, 3, minreg_int
				);
#endif
				NC_ERR_WRITE(rc,_ncpaths[j])

				int maxreg_int[] = {
					_validRegMax[0], _validRegMax[1], _validRegMax[2]
				};
#ifdef PNETCDF
				rc = ncmpi_put_att_int(_ncids[j],NC_GLOBAL,_maxValidRegionName.c_str(), NC_INT, (MPI_Offset)3, maxreg_int);

#else
				rc = nc_put_att_int(
					_ncids[j],NC_GLOBAL,_maxValidRegionName.c_str(),
					NC_INT, 3, maxreg_int
				);
#endif							   
		
				NC_ERR_WRITE(rc,_ncpaths[j])
#endif
			}
#ifdef PNETCDF
			rc = ncmpi_close(_ncids[j]);
			NC_ERR_WRITE(rc,_ncpaths[j])
#else
			rc = nc_close(_ncids[j]);
#endif
			NC_ERR_WRITE(rc,_ncpaths[j])
			
			  }
		_ncids[j] = -1;
		_nc_wave_vars[j] = -1;
	}
#endif
	_WriteTimerStop();
				
	_isOpen = false;
    _vtype = VARUNKNOWN;

	_MaskClose();
	return(0);

}

namespace VAPoR {

	// thread helper function
	//
	void     *RunBlockReadRegionThread(void *object) {
	WaveCodecIO::ReadWriteThreadObj  *X = (WaveCodecIO::ReadWriteThreadObj *) object;
	X->BlockReadRegionThread();
	return(0);
	}

	void     *RunBlockWriteRegionThread(void *object) {
	WaveCodecIO::ReadWriteThreadObj  *X = (WaveCodecIO::ReadWriteThreadObj *) object;
	X->BlockWriteRegionThread();
	return(0);
	}
};

void _pad_line(string mode,float *line_start,size_t l1,size_t l2,long stride);

int WaveCodecIO::BlockReadRegion(
	const size_t bmin[3], const size_t bmax[3], float *region, bool unblock
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
		for (int t=0; t<_nthreads; t++) {
			_block = new float [sz];
			_blockThread[t] = _block;
		}
	}

	GetBlockSize(bs, _reflevel);

	VDFIOBase::_FillPackedCoord(_vtype, bmin, bmin_p, 0);
	VDFIOBase::_FillPackedCoord(_vtype, bmax, bmax_p, 0);
	VDFIOBase::_PackCoord(_vtype, bs, bs_p, 1);

	block_size = bs_p[0]*bs_p[1]*bs_p[2];

	size_t bmin_up[3], bmax_up[3];	// unpacked copy of bmin, bmax
	VDFIOBase::_UnpackCoord(_vtype, bmin, bmin_up, 0);
	VDFIOBase::_UnpackCoord(_vtype, bmax, bmax_up, 0);
	if (! IsValidRegionBlk(bmin_up, bmax_up, _reflevel)) {
		SetErrMsg(
			"Invalid region : (%d %d %d) (%d %d %d)", 
			bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]
		);
		return(-1);
	}

	size_t bdim[3];
	Metadata::GetDimBlk(bdim,-1);

	size_t bdim_p[3];	// packed version of bdim
	VDFIOBase::_PackCoord(_vtype, bdim, bdim_p, 1);

	size_t dim[3];
	Metadata::GetDim(dim, -1);

	size_t dim_p[3];
	VDFIOBase::_PackCoord(_vtype, dim, dim_p, 1);

	//
	// Read bitmask for missing data values, if any
	//
	_MaskRead(bmin_p, bmax_p);

	//
	// Created threaded read object
	//
	for (int t=0; t<_nthreads; t++) {
		_rw_thread_objs[t] = new ReadWriteThreadObj(
			this, t, region, bmin_p, bmax_p, bdim_p, dim_p, 
			bs_p, (bool) unblock, _pad
		);
	}

	_next_block = 0;	// serialize data reads
	_threadStatus = 0;
	if (_nthreads <= 1) {
		_rw_thread_objs[0]->BlockReadRegionThread();
	}
	else {
		int rc = ParRun(RunBlockReadRegionThread, (void **) _rw_thread_objs);
        if (rc < 0) {
			SetErrMsg("Error spawning threads");
			return(-1);
		}
	}

	for (int t=0; t<_nthreads; t++) {
		delete _rw_thread_objs[t];
	}

	return(_threadStatus);
}

int WaveCodecIO::ReadRegion(
	const size_t min[3], const size_t max[3], float *region
) {
	if (! _isOpen || _writeMode) {
		SetErrMsg("Variable not open for reading\n");
		return(-1);
	}

	size_t min_up[3], max_up[3];	// unpacked copy of min, max
	VDFIOBase::_UnpackCoord(_vtype, min, min_up, 0);
	VDFIOBase::_UnpackCoord(_vtype, max, max_up, 0);
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

	VDFIOBase::_PackCoord(_vtype, bmin_up, bmin_p, 0);
	VDFIOBase::_PackCoord(_vtype, bmax_up, bmax_p, 0);

	//
	// fill in 3rd dimension (no-op if 3D data)
	//
	VDFIOBase::_FillPackedCoord(_vtype, min, min_p, 0);
	VDFIOBase::_FillPackedCoord(_vtype, max, max_p, 0);

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
	VDFIOBase::_PackCoord(_vtype, bs, bs_p, 1);


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

int WaveCodecIO::ReadRegion(
    float *region
) {
    SetDiagMsg( "WaveCodecIO::ReadRegion()");

    size_t dim3d[3];
    Metadata::GetDim(dim3d,_reflevel);

    size_t min[] = {0,0,0};
    size_t max[3];
    switch (_vtype) {
    case VAR2D_XY:
        max[0] = dim3d[0]-1; max[1] = dim3d[1]-1;
    break;
    case VAR2D_XZ:
        max[0] = dim3d[0]-1; max[1] = dim3d[2]-1;
    break;
    case VAR2D_YZ:
        max[0] = dim3d[1]-1; max[1] = dim3d[2]-1;
    break;
    case VAR3D:
        max[0] = dim3d[0]-1; max[1] = dim3d[1]-1; max[2] = dim3d[2]-1;
    break;
    default:
        SetErrMsg("Invalid variable type");
        return(-1);
    }

    return(ReadRegion(min, max, region));
}

int WaveCodecIO::BlockWriteRegion(
	const float *region, const size_t bmin[3], const size_t bmax[3], bool block
) {
#ifdef PIOVDC_DEBUG
	cout << "BlockWriteRegion(" << bmin[0] << " ";
	cout << bmin[1] << ",";
	cout << bmin[2] << ", ";
	cout << bmax[0] << ",";
	cout << bmax[1] << ",";
	cout << bmax[2] << ",";
	cout << ") : # blocks " << (bmax[0]-bmin[0]+1) * (bmax[1]-bmin[1]+1) * (bmax[2]-bmin[2]+1) << endl;
#endif

	if (! _isOpen || ! _writeMode) {
		SetErrMsg("Variable not open for writing\n");
		return(-1);
	}

	double starttime = MPI_Wtime();
	size_t bdim[3];
	Metadata::GetDimBlk(bdim,-1);

	size_t bdim_p[3];	// packed version of bdim
	VDFIOBase::_PackCoord(_vtype, bdim, bdim_p, 1);

	size_t dim[3];
	Metadata::GetDim(dim, -1);

	size_t dim_p[3];
	VDFIOBase::_PackCoord(_vtype, dim, dim_p, 1);

	size_t bmin_p[3], bmax_p[3];	// filled copies of bmin, bmax
	VDFIOBase::_FillPackedCoord(_vtype, bmin, bmin_p, 0);
	VDFIOBase::_FillPackedCoord(_vtype, bmax, bmax_p, 0);

	size_t bs[3];
	GetBlockSize(bs, -1);

	if (! _block) {
		size_t sz = bs[0]*bs[1]*bs[2];
		for (int t=0; t<_nthreads; t++) {
			_block = new float [sz];
			_blockThread[t] = _block;
		}
	}

	size_t bs_p[3];	// packed copy of bs
	VDFIOBase::_PackCoord(_vtype, bs, bs_p, 1);

#ifdef PARALLEL
	int rank;
	MPI_Comm_rank(_IO_Comm, &rank);
#endif

	size_t bmin_up[3], bmax_up[3];	// unpacked copy of bmin, bmax
	VDFIOBase::_UnpackCoord(_vtype, bmin, bmin_up, 0);
	VDFIOBase::_UnpackCoord(_vtype, bmax, bmax_up, 0);
	if (! IsValidRegionBlk(bmin_up, bmax_up, -1)) {
		SetErrMsg(
			"Invalid region : (%d %d %d) (%d %d %d)", 
			bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]
		);
		return(-1);
	}

	//
	// Write missing value mask, if any
	//
	_MaskWrite(region, bmin_p, bmax_p, block);

	//
	// Created threaded write object
	//
	for (int t=0; t<_nthreads; t++) {
		_rw_thread_objs[t] = new ReadWriteThreadObj(
			this, t, (float *) region, bmin_p, bmax_p, 
			bdim_p, dim_p, bs_p, block, _pad
		);
	}

	_next_block = 0;    // serialize data writes
	_threadStatus = 0;
	if (_nthreads <= 1) {
		_rw_thread_objs[0]->BlockWriteRegionThread();
	}
	else {
	  double threadTime = MPI_Wtime();
	  int rc = ParRun(RunBlockWriteRegionThread, (void **) _rw_thread_objs);
	  _methodThreadTimer += (MPI_Wtime() - threadTime);
		if (rc < 0) {
			SetErrMsg("Error spawning threads");
			return(-1);
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

	//
	// Clean up threads and reduce data range
	//

	for (int t=0; t<_nthreads; t++) {
		float v = _rw_thread_objs[t]->GetDataRange()[0];
		if (v < _dataRange[0]) _dataRange[0] = v;

		v = _rw_thread_objs[t]->GetDataRange()[1];
		if (v > _dataRange[1]) _dataRange[1] = v;
		delete _rw_thread_objs[t];
	}
	_methodTimer += (MPI_Wtime() - starttime);

	return(_threadStatus);
}

void WaveCodecIO::ReadWriteThreadObj::BlockWriteRegionThread() {


	// dimensions of region in voxels
	//
	size_t nx = (_bmax_p[0] - _bmin_p[0] + 1) * _bs_p[0];
	size_t ny = (_bmax_p[1] - _bmin_p[1] + 1) * _bs_p[1];

	size_t nbx = (_bmax_p[0] - _bmin_p[0] + 1);
	size_t nby = (_bmax_p[1] - _bmin_p[1] + 1);

	size_t block_size = _bs_p[0]*_bs_p[1]*_bs_p[2];

	int nblocks = (_bmax_p[0] - _bmin_p[0] + 1)  * (_bmax_p[1] - _bmin_p[1] + 1) * (_bmax_p[2] - _bmin_p[2] + 1);
	
	double begin = MPI_Wtime();
	for (int index = _id; index<nblocks; index+=_wc->_nthreads) {
        int bx = (index % nbx) + _bmin_p[0];
        int by = (index % (nbx*nby) / nbx) + _bmin_p[1];
        int bz = (index / (nbx*nby)) + _bmin_p[2];


		//
		// These flags are true if this is a boundary block && the volume
		// dimensions don't match the blocked-volume dimensions
		//
		bool xbdry = (bx==(_bdim_p[0]-1) && (_bdim_p[0]*_bs_p[0] != _dim_p[0]));
		bool ybdry = (by==(_bdim_p[1]-1) && (_bdim_p[1]*_bs_p[1] != _dim_p[1]));
		bool zbdry = (bz==(_bdim_p[2]-1) && (_bdim_p[2]*_bs_p[2] != _dim_p[2]));

		//
		// the xyzstop limits are the bounds of the valid data for the block
		// (in the case where bdim*bs != dim)
		//
		size_t xstop = _bs_p[0];
		size_t ystop = _bs_p[1];
		size_t zstop = _bs_p[2];

		if (xbdry && _pad) xstop -= (_bdim_p[0]*_bs_p[0] - _dim_p[0]);
		if (ybdry && _pad) ystop -= (_bdim_p[1]*_bs_p[1] - _dim_p[1]);
		if (zbdry && _pad) zstop -= (_bdim_p[2]*_bs_p[2] - _dim_p[2]);

		// Block coordinates relative to region origin
		//
		int bxx = bx - _bmin_p[0];
		int byy = by - _bmin_p[1];
		int bzz = bz - _bmin_p[2];

		float *blockptr;

		if (_reblock) {
			blockptr = _wc->_blockThread[_id];

			// Starting coordinate of current block in voxels
			//
			size_t x0 = bxx * _bs_p[0];
			size_t y0 = byy * _bs_p[1];
			size_t z0 = bzz * _bs_p[2];

			for (int z = 0; z<zstop; z++) {
			for (int y = 0; y<ystop; y++) {
			for (int x = 0; x<xstop; x++) {
				float v = _region[nx*ny*(z0+z) + nx*(y0+y) + (x0+x)];

				_wc->_blockThread[_id][z*_bs_p[0]*_bs_p[1] + y*_bs_p[0] + x] = v;
			}
			}
			}
		}
		else {
			blockptr = _region + bzz*nbx*nby*block_size + 
				byy*nbx*block_size + bxx*block_size;

			for (int z = 0; z<zstop; z++) {
			for (int y = 0; y<ystop; y++) {
			for (int x = 0; x<xstop; x++) {
				float v = blockptr[_bs_p[0]*_bs_p[1]*z + _bs_p[0]*y + x];
				_wc->_blockThread[_id][_bs_p[0]*_bs_p[1]*z + _bs_p[0]*y + x] =v;

			}
			}
			}
			blockptr = _wc->_blockThread[_id];
		}

		if (xbdry && _pad) {
			string mode = _wc->GetBoundaryMode();

			float *line_start;
			for (int z = 0; z<_bs_p[2]; z++) {  
			for (int y = 0; y<_bs_p[1]; y++) {  
				line_start = _wc->_blockThread[_id] + (z*_bs_p[1]*_bs_p[0] + y*_bs_p[0]);

				_pad_line(mode, line_start, xstop, _bs_p[0], 1);
			}
			}
		}

		if (ybdry && _pad) {
			string mode = _wc->GetBoundaryMode();

			float *line_start;
			for (int z = 0; z<_bs_p[2]; z++) {  
			for (int x = 0; x<_bs_p[0]; x++) {  
				line_start = _wc->_blockThread[_id] + (z*_bs_p[1]*_bs_p[0] + x);

				_pad_line(mode, line_start, ystop, _bs_p[1], _bs_p[0]);
			}
			}
		}

		if (zbdry && _pad) {
			string mode = _wc->GetBoundaryMode();

			float *line_start;
			for (int y = 0; y<_bs_p[1]; y++) {  
			for (int x = 0; x<_bs_p[0]; x++) {  
				line_start = _wc->_blockThread[_id] + (y*_bs_p[0] + x);

				_pad_line(mode, line_start, zstop, _bs_p[2], _bs_p[0]*_bs_p[1]);
			}
			}
		}

		//
		// Handle any missing values
		//
		bool valid_data = true;
		_wc->_MaskRemove(blockptr, valid_data);

		if (valid_data) {
			for (int z = 0; z<_bs_p[2]; z++) {
			for (int y = 0; y<_bs_p[1]; y++) {
			for (int x = 0; x<_bs_p[0]; x++) {
				float v = blockptr[_bs_p[0]*_bs_p[1]*z + _bs_p[0]*y + x];
#ifdef	WIN32
				if (_isnan(v)) {
#else
				if (isnan(v)) {
#endif
					blockptr[_bs_p[0]*_bs_p[1]*z + _bs_p[0]*y + x] = 0.0;
				}

				if (v < _dataRange[0]) _dataRange[0] = v;
				if (v > _dataRange[1]) _dataRange[1] = v;

			}
			}
			}
		}


		double starttime = MPI_Wtime();
		if (_id==0) _wc->_XFormTimerStart();
		int rc = 0;
		rc = _wc->_compressorThread[_id]->Decompose(
			blockptr, _wc->_cvectorThread[_id], _wc->_ncoeffs, 
			_wc->_sigmapsThread[_id]
		); 
		_wc->_xformMPI += (MPI_Wtime() - starttime);
		if (_id==0) _wc->_XFormTimerStop();
		if (rc<0) {
			_wc->_threadStatus = -1;
			return;
		}

		bool myturn = false;
		while (! myturn) {
			_wc->MutexLock();
				if (index == _wc->_next_block) {
				  starttime = MPI_Wtime();
					rc = _WriteBlock(bx, by, bz);
					_wc->_ioMPI += (MPI_Wtime() - starttime);
					if (rc<0) _wc->_threadStatus = -1;
					_wc->_next_block++;
					myturn = true;
				}
			_wc->MutexUnlock();
		}

		if (_wc->_threadStatus != 0) return;
		
	}
	_wc->_methodThreadTimer += (MPI_Wtime() - begin);
}

int WaveCodecIO::WriteRegion(
	const float *region, const size_t min[3], const size_t max[3]
) {
	if (! _isOpen || ! _writeMode) {
		SetErrMsg("Variable not open for writing\n");
		return(-1);
	}

	size_t min_up[3], max_up[3];	// unpacked copy of min, max
	VDFIOBase::_UnpackCoord(_vtype, min, min_up, 0);
	VDFIOBase::_UnpackCoord(_vtype, max, max_up, 0);
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

	VDFIOBase::_PackCoord(_vtype, bmin_up, bmin_p, 0);
	VDFIOBase::_PackCoord(_vtype, bmax_up, bmax_p, 0);

	//
	// fill in 3rd dimension (no-op if 3D data)
	//
	VDFIOBase::_FillPackedCoord(_vtype, min, min_p, 0);
	VDFIOBase::_FillPackedCoord(_vtype, max, max_p, 0);

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
	VDFIOBase::_PackCoord(_vtype, bs, bs_p, 1);

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
				_pad_line(GetBoundaryMode(), line_start, l1, l2, 1);

				line_start += l1 - 1;
				l2 = maxb_p[0] + 1;
				_pad_line(GetBoundaryMode(), line_start, l1, l2, -1);
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
				_pad_line(GetBoundaryMode(), line_start, l1, l2, stride);

				line_start += (l1 - 1) * stride;
				l2 = maxb_p[1] + 1;
				_pad_line(GetBoundaryMode(), line_start, l1, l2, -stride);
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
				_pad_line(GetBoundaryMode(), line_start, l1, l2, stride);

				line_start += (l1 - 1) * stride;
				l2 = maxb_p[2] + 1;
				_pad_line(GetBoundaryMode(), line_start, l1, l2, -stride);
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

int WaveCodecIO::WriteRegion(
    const float *region
) {

    SetDiagMsg( "WaveCodecIO::WriteRegion()" );

    size_t dim3d[3];
    Metadata::GetDim(dim3d,_reflevel);

    size_t min[] = {0,0,0};
    size_t max[3];
    switch (_vtype) {
    case VAR2D_XY:
        max[0] = dim3d[0]-1; max[1] = dim3d[1]-1;
    break;
    case VAR2D_XZ:
        max[0] = dim3d[0]-1; max[1] = dim3d[2]-1;
    break;
    case VAR2D_YZ:
        max[0] = dim3d[1]-1; max[1] = dim3d[2]-1;
    break;
    case VAR3D:
        max[0] = dim3d[0]-1; max[1] = dim3d[1]-1; max[2] = dim3d[2]-1;
    break;
    default:
        SetErrMsg("Invalid variable type");
        return(-1);
    }

    return(WriteRegion(region, min, max));
}



int WaveCodecIO::ReadSlice(
	float *slice
) {
	if (! _isOpen || _writeMode) {
		SetErrMsg("Variable not open for reading\n");
		return(-1);
	}

	size_t bdim[3];
	Metadata::GetDimBlk(bdim,_reflevel);
	size_t bdim_p[3];
	VDFIOBase::_PackCoord(_vtype, bdim, bdim_p, 1);

	size_t dim[3];
	Metadata::GetDim(dim, _reflevel);
	size_t dim_p[3];
	VDFIOBase::_PackCoord(_vtype, dim, dim_p, 1);

	size_t bs[3];
	GetBlockSize(bs, _reflevel);
	size_t bs_p[3];
	VDFIOBase::_PackCoord(_vtype, bs, bs_p, 1);

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
	Metadata::GetDimBlk(bdim,-1);
	size_t bdim_p[3];
	VDFIOBase::_PackCoord(_vtype, bdim, bdim_p, 1);

	size_t bs[3]; 
	GetBlockSize(bs, -1);
	size_t bs_p[3];
	VDFIOBase::_PackCoord(_vtype, bs, bs_p, 1);

	size_t dim[3];
	Metadata::GetDim(dim,-1);
	size_t dim_p[3];	// packed version of dim
	VDFIOBase::_PackCoord(_vtype, dim, dim_p, 1);

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
		_pad_line(GetBoundaryMode(), line_start, dim_p[0], nnx, 1);
	}

	for (int x = 0; x<nnx; x++) { 
		line_start = _sliceBuffer + (slice_count * nnx*nny + x);
		_pad_line(GetBoundaryMode(), line_start, dim_p[1], nny, nnx);
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
				_pad_line(
					GetBoundaryMode(), line_start, 
					dim_p[2]-(bmax_p[2]*bs_p[2]), bs_p[2], nnx*nny
				);
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
		size_t dim = (_validRegMax[i] + 1) >> ldelta;
		min[i] = _validRegMin[i] >> ldelta;
		max[i] = _validRegMax[i] >> ldelta;

		//
		// hack to deal with boundary conditions when dimensions are odd
		//
		if (max[i] >= dim) max[i] = dim-1;
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
	delete cmp;
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
		for (int t=0; t<_nthreads; t++) {
			if (! _compressorThread2DXY[t]) {
				_compressor2DXY = new Compressor(bdims,wavename,boundary_mode);
				_compressorThread2DXY[t] = _compressor2DXY;
			}
			_compressorThread[t] = _compressorThread2DXY[t];
		}
		_compressor = _compressor2DXY;
	break;

	case VAR2D_XZ:
		bdims.push_back(bs[0]);
		bdims.push_back(bs[2]);
		_cratios = _cratios2D;
		for (int t=0; t<_nthreads; t++) {
			if (! _compressorThread2DXZ[t]) {
				_compressor2DXZ = new Compressor(bdims,wavename,boundary_mode);
				_compressorThread2DXZ[t] = _compressor2DXZ;
			}
			_compressorThread[t] = _compressorThread2DXZ[t];
		}
		_compressor = _compressor2DXZ;
	break;

	case VAR2D_YZ:
		bdims.push_back(bs[1]);
		bdims.push_back(bs[2]);
		_cratios = _cratios2D;
		for (int t=0; t<_nthreads; t++) {
			if (! _compressorThread2DYZ[t]) {
				_compressor2DYZ = new Compressor(bdims,wavename,boundary_mode);
				_compressorThread2DYZ[t] = _compressor2DYZ;
			}
			_compressorThread[t] = _compressorThread2DYZ[t];
		}
		_compressor = _compressor2DYZ;
	break;

	case VAR3D:
		bdims.push_back(bs[0]);
		bdims.push_back(bs[1]);
		bdims.push_back(bs[2]);
		_cratios = _cratios3D;
		for (int t=0; t<_nthreads; t++) {
			if (! _compressorThread3D[t]) {
				_compressor3D = new Compressor(bdims,wavename,boundary_mode);
				_compressorThread3D[t] = _compressor3D;
			}
			_compressorThread[t] = _compressorThread3D[t];
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
		for (int t=0; t<_nthreads; t++) {
			_sigmapsThread[t][i].Reshape(sigdims);
		}
		
	}
	assert(naccum <= ntotal); 

	if (_cvectorsize < naccum) {
		if (_cvector) delete [] _cvector;
		for (int t=0; t<_nthreads; t++) {
			_cvector = new float[naccum];
			_cvectorThread[t] = _cvector;
		}
		_cvectorsize = naccum;
	}

	if (_svectorsize < saccum) {
		if (_svectorsize) delete [] _svector;
		for (int t=0; t<_nthreads; t++) {
			_svector = new unsigned char[saccum];
			_svectorThread[t] = _svector;
		}
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

	int ncdimid;
#ifdef PNETCDF
	MPI_Offset ncdim;
#else
	size_t ncdim;
#endif

	_sigmapsizes.clear();
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
#ifdef PARALLEL
		MPI_Info info;
		MPI_Info_create(&info);
	//	MPI_Info_set(info,"ibm_largeblock_io","true");
#endif

#ifdef PNETCDF
		  rc = ncmpi_open(_IO_Comm, path.c_str(), NC_NOWRITE, info, &ncid);
#else
			rc = nc__open(path.c_str(), NC_NOWRITE, &chsz, &ncid);
#endif

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
		Metadata::GetDimBlk(bdim,-1);
#ifdef PNETCDF
		rc = ncmpi_inq_dimid(_ncids[j], _volumeDimNbxName.c_str(), &ncdimid);
#else
		rc = nc_inq_dimid(_ncids[j], _volumeDimNbxName.c_str(), &ncdimid);
#endif
		NC_ERR_READ(rc,path)
#ifdef PNETCDF
		  rc = ncmpi_inq_dimlen(_ncids[j], ncdimid, &ncdim);
#else
        rc = nc_inq_dimlen(_ncids[j], ncdimid, &ncdim);
#endif
        NC_ERR_READ(rc,path)

        if (ncdim != bdim[0]) {
            SetErrMsg("Metadata file and data file mismatch");
            return(-1);
        }

#ifdef PNETCDF
		rc = ncmpi_inq_dimid(_ncids[j], _volumeDimNbyName.c_str(), &ncdimid);
#else
        rc = nc_inq_dimid(_ncids[j], _volumeDimNbyName.c_str(), &ncdimid);
#endif
	NC_ERR_READ(rc,path)
#ifdef PNETCDF
		  rc = ncmpi_inq_dimlen(_ncids[j], ncdimid, &ncdim);
#else
        rc = nc_inq_dimlen(_ncids[j], ncdimid, &ncdim);
#endif
        NC_ERR_READ(rc,path)

        if (ncdim != bdim[1]) {
            SetErrMsg("Metadata file and data file mismatch");
            return(-1);
		}
#ifdef PNETCDF
		rc = ncmpi_inq_dimid(_ncids[j], _volumeDimNbzName.c_str(), &ncdimid);
#else
        rc = nc_inq_dimid(_ncids[j], _volumeDimNbzName.c_str(), &ncdimid);
#endif
	NC_ERR_READ(rc,path)
#ifdef PNETCDF
		  rc = ncmpi_inq_dimlen(_ncids[j], ncdimid, &ncdim);
#else
        rc = nc_inq_dimlen(_ncids[j], ncdimid, &ncdim);
#endif
        NC_ERR_READ(rc,path)

        if (ncdim != bdim[2]) {
            SetErrMsg("Metadata file and data file mismatch");
            return(-1);
		}

		//
		// Final dimension is the number of wavelet coefficients
		// + the sigmap size / 4
		//
#ifdef PNETCDF
		rc = ncmpi_inq_dimid(_ncids[j], _coeffDimName.c_str(), &ncdimid);
#else
        rc = nc_inq_dimid(_ncids[j], _coeffDimName.c_str(), &ncdimid);
#endif
	NC_ERR_READ(rc,path)
#ifdef PNETCDF
		  rc = ncmpi_inq_dimlen(_ncids[j], ncdimid, &ncdim);
#else
        rc = nc_inq_dimlen(_ncids[j], ncdimid, &ncdim);
#endif
        NC_ERR_READ(rc,path)

		//
		// Encoded sigmap is concatenated to wavelet coefficients
		//
		_sigmapsizes.push_back(ncdim - _ncoeffs[j]);

#ifdef PNETCDF
	rc = ncmpi_get_att_float(_ncids[j],NC_GLOBAL,_scalarRangeName.c_str(),_dataRange);
#else
		rc = nc_get_att_float(
			_ncids[j],NC_GLOBAL,_scalarRangeName.c_str(),_dataRange
		);
#endif
		NC_ERR_READ(rc,path)

		int minreg_int[3];
#ifdef PNETCDF
		rc = ncmpi_get_att_int(_ncids[j],NC_GLOBAL,_minValidRegionName.c_str(),minreg_int);
#else
		rc = nc_get_att_int(
			_ncids[j],NC_GLOBAL,_minValidRegionName.c_str(),minreg_int
		);
#endif
		if (rc == NC_NOERR) {
			for(int i=0; i<3; i++) _validRegMin[i] = minreg_int[i];
		}

		int maxreg_int[3];
#ifdef PNETCDF
		rc = ncmpi_get_att_int(_ncids[j],NC_GLOBAL,_maxValidRegionName.c_str(),maxreg_int);
#else
		rc = nc_get_att_int(
			_ncids[j],NC_GLOBAL,_maxValidRegionName.c_str(),maxreg_int
		);
#endif
		if (rc == NC_NOERR) {
			for(int i=0; i<3; i++) _validRegMax[i] = maxreg_int[i];
		}

		int ncvar;
#ifdef PNETCDF
		rc = ncmpi_inq_varid(_ncids[j], _waveletCoeffName.c_str(), &ncvar);
#else
		rc = nc_inq_varid(_ncids[j], _waveletCoeffName.c_str(), &ncvar);
#endif
		NC_ERR_READ(rc,path)
		_nc_wave_vars.push_back(ncvar);
#ifdef PNETCDF
			  if(!_collectiveIO){
			    ncmpi_begin_indep_data(_ncids[j]);
			  }
	
#endif

	}

	size_t maxsmap= 0;
	for (int i = 0; i<_sigmapsizes.size(); i++) {
		if (_sigmapsizes[i] > maxsmap) maxsmap = _sigmapsizes[i];
	}

	if (_svectorsize < maxsmap) {
		if (_svectorsize) delete [] _svector;
		for (int t=0; t<_nthreads; t++) {
			_svector = new unsigned char[maxsmap * NC_FLOAT_SZ];
			_svectorThread[t] = _svector;
		}
		_svectorsize = maxsmap;
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
	Metadata::GetDim(dim, -1);
	size_t bs[3]; 
	GetBlockSize(bs, -1);

	string wname = GetWaveName();
	string wmode = GetBoundaryMode();

	_ncpaths.clear();
	_ncids.clear();
	_nc_wave_vars.clear();
	_sigmapsizes.clear();
	_ncbufs.clear();
	for(int j=0; j<=_lod; j++) {
		int rc;

		if (_ncoeffs[j] < 1) break;

		ostringstream oss;
        oss << basename << ".nc" << j;
		string path = oss.str();

		_ncpaths.push_back(path);

		size_t chsz = NC_CHUNKSIZEHINT;
		int ncid;

	_WriteTimerStart();

#ifdef PARALLEL
		MPI_Info info;
		MPI_Info_create(&info);
	//	MPI_Info_set(info,"ibm_largeblock_io","true");
#endif
#ifndef NOIO
#ifdef PARALLEL
#ifdef PNETCDF
	rc = ncmpi_create(_IO_Comm, path.c_str(), NC_WRITE | NC_64BIT_OFFSET,
#else //PNETCDF
		rc = nc_create_par(path.c_str(), NC_MPIIO | NC_NETCDF4, _IO_Comm, 
#endif //PNETCDF
		info, &ncid);
#else //PARALLEL
		rc = nc__create(path.c_str(), NC_64BIT_OFFSET, 0, &chsz, &ncid);
#endif //PARALLEL
#endif //NOIO

		NC_ERR_WRITE(rc,path)
		_ncids.push_back(ncid);
	
		// Disable data filling - may not be necessary
		//
		int mode;
#ifdef PNETCDF

		ncmpi_set_fill(_ncids[j], NC_NOFILL, &mode);
#else
	   	nc_set_fill(_ncids[j], NC_NOFILL, &mode);
#endif

		//
		// Define netCDF dimensions
		//
		int vol_dim_ids[3];
		size_t bdim[3];
		Metadata::GetDimBlk(bdim,-1);
#ifndef NOIO
#ifdef PNETCDF
	       rc = ncmpi_def_dim(_ncids[j],_volumeDimNbxName.c_str(),bdim[0],&vol_dim_ids[0]
		);
				   NC_ERR_WRITE(rc,path)

		rc = ncmpi_def_dim(_ncids[j],_volumeDimNbyName.c_str(),bdim[1],&vol_dim_ids[1]
		);
				   NC_ERR_WRITE(rc,path)

		rc = ncmpi_def_dim(_ncids[j],_volumeDimNbzName.c_str(),bdim[2],&vol_dim_ids[2]
		);
				   NC_ERR_WRITE(rc,path)
#else
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
				   NC_ERR_WRITE(rc,path);
#endif
#endif		

		bool reconstruct_sigmap = 
				 ((_cratios.size() == (j+1)) && (_cratios[j] == 1));

		//
		// Final dimension is the number of wavelet coefficients
		// + the sigmap size / 4
		//
		if (reconstruct_sigmap) {
			_sigmapsizes.push_back(0);
		}
		else {
			size_t sms = _sigmapsThread[0][j].GetMapSize(_ncoeffs[j]);
			_sigmapsizes.push_back((sms+(NC_FLOAT_SZ-1)) / NC_FLOAT_SZ);
		}
#ifdef PNETCDF
			  MPI_Offset l = _ncoeffs[j] + _sigmapsizes[j];
#else
			  size_t l = _ncoeffs[j] + _sigmapsizes[j]; //bad var name
#endif

		int coeff_dim_id;
#ifdef PNETCDF
		rc = ncmpi_def_dim(_ncids[j], _coeffDimName.c_str(), l, &coeff_dim_id);
#else
		rc = nc_def_dim(
			_ncids[j],_coeffDimName.c_str(),l, &coeff_dim_id
		);
#endif
				   NC_ERR_WRITE(rc,path)

		int wave_dim_ids[4];
		int ndims = 0;

		vector <size_t> ncdims;
		switch (_vtype) {
		case VAR2D_XY:
			ndims = 3;
			wave_dim_ids[0] = vol_dim_ids[1];
			wave_dim_ids[1] = vol_dim_ids[0];
			wave_dim_ids[2] = coeff_dim_id;
			ncdims.push_back(bdim[1]);
			ncdims.push_back(bdim[0]);
			ncdims.push_back(l);
		break;
		case VAR2D_XZ:
			ndims = 3;
			wave_dim_ids[0] = vol_dim_ids[2];
			wave_dim_ids[1] = vol_dim_ids[0];
			wave_dim_ids[2] = coeff_dim_id;
			ncdims.push_back(bdim[2]);
			ncdims.push_back(bdim[0]);
			ncdims.push_back(l);
		break;
		case VAR2D_YZ:
			ndims = 3;
			wave_dim_ids[0] = vol_dim_ids[2];
			wave_dim_ids[1] = vol_dim_ids[1];
			wave_dim_ids[2] = coeff_dim_id;
			ncdims.push_back(bdim[2]);
			ncdims.push_back(bdim[1]);
			ncdims.push_back(l);

		break;
		case VAR3D:
			ndims = 4;
			wave_dim_ids[0] = vol_dim_ids[2];
			wave_dim_ids[1] = vol_dim_ids[1];
			wave_dim_ids[2] = vol_dim_ids[0];
			wave_dim_ids[3] = coeff_dim_id;
			ncdims.push_back(bdim[2]);
			ncdims.push_back(bdim[1]);
			ncdims.push_back(bdim[0]);
			ncdims.push_back(l);

		break;
		default:
			assert(0);
		}

#ifndef NOIO
#ifdef PNETCDF
		rc = ncmpi_def_var(_ncids[j], _waveletCoeffName.c_str(), NC_FLOAT, ndims, wave_dim_ids, &ncid);
#else
		rc = nc_def_var(
			_ncids[j], _waveletCoeffName.c_str(), NC_FLOAT,
			ndims, wave_dim_ids, &ncid
		);
#endif
				   NC_ERR_WRITE(rc,path)
#endif		
		_nc_wave_vars.push_back(ncid);

		  int rank = -1;
#ifdef PARALLEL

			  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif
		NCBuf *ncbuf = new NCBuf(_ncids[j], ncid, NC_FLOAT, ncdims, _collectiveIO, rank, _NC_BUF_SIZE);
		_ncbufs.push_back(ncbuf);

#ifndef NOIO
		oss.str("");
		oss << "Floating point wavelet coefficients (" << _ncoeffs[j] <<
		") byte encoded signficant map (" << _sigmapsizes[j] << ")";
#ifdef PNETCDF
		rc = ncmpi_put_att_text(_ncids[j],ncid,"description", oss.str().length(), oss.str().c_str());
#else
		rc = nc_put_att_text(
			_ncids[j],ncid,"description", oss.str().length(), 
			oss.str().c_str()
		);
#endif
				   NC_ERR_WRITE(rc,path)

		//
		// Define netCDF global attributes
		//
		int version = GetVDFVersion();
#ifdef PNETCDF
		rc = ncmpi_put_att_int(_ncids[j],NC_GLOBAL,_fileVersionName.c_str(),NC_INT, 1, &version);
#else
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_fileVersionName.c_str(),NC_INT, 1, &version
		);
#endif
				   NC_ERR_WRITE(rc,path)

#ifdef PNETCDF
		rc = ncmpi_put_att_int(_ncids[j],NC_GLOBAL,_compressionLevelName.c_str(),NC_INT, 1, &j);
#else
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_compressionLevelName.c_str(),NC_INT, 1, &j
		);
#endif
				   NC_ERR_WRITE(rc,path)


		int dim_int[] = {(int) dim[0], (int) dim[1], (int) dim[2]};
#ifdef PNETCDF
		rc = ncmpi_put_att_int(_ncids[j],NC_GLOBAL,_nativeResName.c_str(),NC_INT, 3, dim_int);
#else
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_nativeResName.c_str(),NC_INT, 3, dim_int
		);
#endif
				   NC_ERR_WRITE(rc,path)

		int bs_int[] = {(int) bs[0],(int) bs[1],(int) bs[2]};
#ifdef PNETCDF
		rc = ncmpi_put_att_int(_ncids[j],NC_GLOBAL,_blockSizeName.c_str(),NC_INT, 3, bs_int);
#else
        rc = nc_put_att_int(
            _ncids[j],NC_GLOBAL,_blockSizeName.c_str(),NC_INT, 3, bs_int
        );
#endif
				   NC_ERR_WRITE(rc,path)

		int minreg_int[3] = {_validRegMin[0],_validRegMin[1],_validRegMin[2]};
#ifdef PNETCDF
		rc = ncmpi_put_att_int(_ncids[j],NC_GLOBAL,_minValidRegionName.c_str(),NC_INT, 3, minreg_int);
#else
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_minValidRegionName.c_str(),NC_INT, 3, 
			minreg_int
		);
#endif
				   NC_ERR_WRITE(rc,path)

		int maxreg_int[3] = {_validRegMax[0],_validRegMax[1],_validRegMax[2]};
#ifdef PNETCDF
		rc = ncmpi_put_att_int(_ncids[j],NC_GLOBAL,_maxValidRegionName.c_str(),NC_INT, 3, maxreg_int);
#else
		rc = nc_put_att_int(
			_ncids[j],NC_GLOBAL,_maxValidRegionName.c_str(),NC_INT, 3, 
			maxreg_int
		);
#endif
				   NC_ERR_WRITE(rc,path)

#ifdef PNETCDF
		rc = ncmpi_put_att_float(_ncids[j],NC_GLOBAL,_scalarRangeName.c_str(),NC_FLOAT, 2, _dataRange);
#else
		rc = nc_put_att_float(
			_ncids[j],NC_GLOBAL,_scalarRangeName.c_str(),NC_FLOAT, 2, _dataRange
		);
#endif
				   NC_ERR_WRITE(rc,path);

#ifdef PNETCDF
	        rc = ncmpi_put_att_text(_ncids[j],NC_GLOBAL,_waveletName.c_str(), wname.length(),wname.c_str());
#else
		rc = nc_put_att_text(
			_ncids[j],NC_GLOBAL,_waveletName.c_str(), wname.length(), 
			wname.c_str()
		);
#endif
				   NC_ERR_WRITE(rc,path)

#ifdef PNETCDF
		rc = ncmpi_put_att_text(_ncids[j],NC_GLOBAL,_boundaryModeName.c_str(), wmode.length(), wmode.c_str());
#else
		rc = nc_put_att_text(
			_ncids[j],NC_GLOBAL,_boundaryModeName.c_str(), wmode.length(), 
			wmode.c_str()
		);
#endif

				   NC_ERR_WRITE(rc,path)
#endif
			  
#ifdef PNETCDF
				   rc = ncmpi_enddef(_ncids[j]);
#else   
				   rc = nc_enddef(_ncids[j]);
#endif
#ifdef PNETCDF
			  if(!_collectiveIO){
			    ncmpi_begin_indep_data(_ncids[j]);
			  }
#endif
				   NC_ERR_WRITE(rc,path)

	_WriteTimerStop();

	}
	return(0);
		
}


int WaveCodecIO::ReadWriteThreadObj::_WriteBlock(
	size_t bx, size_t by, size_t bz
) {
	unsigned long LSBTest = 1;
	bool do_swapbytes = false;
	if (! (*(char *) &LSBTest)) {
		// swap to MSBFirst
		do_swapbytes = true;
	}

	const float *cvectorptr = _wc->_cvectorThread[_id];
	int rc;

	//
	// This method is serialized. OK for all threads to call Timer methods
	//
	_wc->_WriteTimerStart();


	for(int j=0; j<=_wc->_lod; j++) {
		if (_wc->_ncoeffs[j] < 1) break;

		size_t start[] = {0,0,0,0};
		size_t wcount[] = {1,1,1,1};
		size_t scount[] = {1,1,1,1};
		size_t maplen;
		const unsigned char *map;

#ifdef PNETCDF
		_wc->_sigmapsThread[_id][j].GetMap(&map, (size_t *)&maplen);
#else
		_wc->_sigmapsThread[_id][j].GetMap(&map, &maplen);
#endif
//		assert(maplen == _wc->_sigmapsizes[j]);

		bool reconstruct_sigmap = 
		     ((_wc->_cratios.size() == (j+1)) && (_wc->_cratios[j] == 1));

		if (_wc->_vtype == VAR3D) {
			start[0] = bz;
			start[1] = by;
			start[2] = bx;
			wcount[3] = _wc->_ncoeffs[j];
			scount[3] = _wc->_sigmapsizes[j];
		}
		else {
			start[0] = by;
			start[1] = bx;
			wcount[2] = _wc->_ncoeffs[j];
			scount[2] = _wc->_sigmapsizes[j];
		}

		rc = _wc->_ncbufs[j]->PutVara(start, wcount, cvectorptr);
		NC_ERR_WRITE(rc, _wc->_ncpaths[j]);

		cvectorptr += _wc->_ncoeffs[j];

		//
		// To save space we don't store the final signficiance map
		// if it coresponds to a full reconstuction of the 
		// data (cratio == 1).
		//
		if (! reconstruct_sigmap) {
			if (_wc->_vtype == VAR3D) 
				start[3] += _wc->_ncoeffs[j];
			else
				start[2] += _wc->_ncoeffs[j];

			//
			// Signficance map is concatenated to the wavelet coefficients
			// variable to improve IO performance
			//
#ifndef NOIO
			if (do_swapbytes) {
				// Ugh. We're violating const-ness of map					// to avoid another data copy.
				//
				swapbytes(
					(void *) map, NC_FLOAT_SZ, 
					_wc->_sigmapsizes[j]
				);
			}
			rc = _wc->_ncbufs[j]->PutVara(start, scount, map);
			NC_ERR_WRITE(rc, _wc->_ncpaths[j])
		}
	}
#endif

	_wc->_WriteTimerStop();

	return(0);
}

void _pad_line(
	string mode,
	float *line_start, 
	size_t l1,	// length of valid data
	size_t l2,	// total length of array
	long stride
) {
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
		return;
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


WaveCodecIO::ReadWriteThreadObj::ReadWriteThreadObj(
	WaveCodecIO *wc,
	int id,
	float *region,
	const size_t *bmin_p,
	const size_t *bmax_p,
	const size_t *bdim_p,
	const size_t *dim_p,
	const size_t *bs_p, 
	bool reblock,
	bool pad
) {
	_wc = wc;
	_id = id;
	_region = region;
	_bmin_p = bmin_p;
	_bmax_p = bmax_p;
	_bdim_p = bdim_p;
	_dim_p = dim_p;
	_bs_p = bs_p;
	_dataRange[0] = FLT_MAX;
	_dataRange[1] = -FLT_MAX;
	_reblock = reblock;
	_pad = pad;
}

void WaveCodecIO::ReadWriteThreadObj::BlockReadRegionThread(
) {



	// dimensions of region in voxels
	//
	size_t nx = (_bmax_p[0] - _bmin_p[0] + 1) * _bs_p[0];
	size_t ny = (_bmax_p[1] - _bmin_p[1] + 1) * _bs_p[1];

	size_t nbx = (_bmax_p[0] - _bmin_p[0] + 1);
	size_t nby = (_bmax_p[1] - _bmin_p[1] + 1);
	
	size_t block_size = _bs_p[0]*_bs_p[1]*_bs_p[2];

	int nblocks = (_bmax_p[0] - _bmin_p[0] + 1)  * (_bmax_p[1] - _bmin_p[1] + 1)  * (_bmax_p[2] - _bmin_p[2] + 1);

	for (int index = _id; index<nblocks; index+=_wc->_nthreads) {
		int bx = (index % nbx) + _bmin_p[0];
		int by = (index % (nbx*nby) / nbx) + _bmin_p[1];
		int bz = (index / (nbx*nby)) + _bmin_p[2];

		//
		// This ugly bit of code serializes the data reads to improve
		// performance of the underlying storage system and also
		// because the netcdf API is not thread safe. The code might
		// be made more efficient by using a higher level synchronization
		// mechanism (conditional wait) to avoid the spin lock
		//
		int rc;
#define COORDINATED_IO
#ifdef	COORDINATED_IO
		bool myturn = false;
		while (! myturn) {
			_wc->MutexLock();
				if (index == _wc->_next_block) {
					rc = _FetchBlock(bx, by, bz);
					if (rc<0) _wc->_threadStatus = -1;
					_wc->_next_block++;
					myturn = true;
				}
			_wc->MutexUnlock();
		}
#else
		_wc->MutexLock();
			rc = _FetchBlock(bx, by, bz);
			if (rc<0) _wc->_threadStatus = -1;
		_wc->MutexUnlock();
#endif
		if (_wc->_threadStatus != 0) return;

		// Block coordinates relative to region origin
		//
		int bxx = bx - _bmin_p[0];
		int byy = by - _bmin_p[1];
		int bzz = bz - _bmin_p[2];

		float *blockptr;

		if (_reblock) {
			blockptr = _wc->_blockThread[_id];

			if (_id==0) _wc->_XFormTimerStart();
			int rc = _wc->_compressorThread[_id]->Reconstruct(
				_wc->_cvectorThread[_id], blockptr, _wc->_sigmapsThread[_id], 
				_wc->_reflevel
			); 
			if (rc<0) {
				_wc->_threadStatus = -1;
				return;
			}
			if (_id==0) _wc->_XFormTimerStop();

			_wc->_MaskReplace(bx, by, bz, blockptr);

			// Starting coordinate of current block in voxels
			// relative to region origin
			//
			size_t x0 = bxx * _bs_p[0];
			size_t y0 = byy * _bs_p[1];
			size_t z0 = bzz * _bs_p[2];

			for (int z = 0; z<_bs_p[2]; z++) {
			for (int y = 0; y<_bs_p[1]; y++) {
			for (int x = 0; x<_bs_p[0]; x++) {
				float v = blockptr[z*_bs_p[0]*_bs_p[1] + y*_bs_p[0] + x];

				_region[nx*ny*(z0+z) + nx*(y0+y) + (x0+x)]  = v;

			}
			}
			}
		}
		else {

			blockptr = _region + bzz*nbx*nby*block_size + 
				byy*nbx*block_size + bxx*block_size;

			if (_id==0) _wc->_XFormTimerStart();
			int rc = _wc->_compressorThread[_id]->Reconstruct(
				_wc->_cvectorThread[_id], blockptr, _wc->_sigmapsThread[_id], 
				_wc->_reflevel
			); 
			if (rc<0) {
				_wc->_threadStatus = -1;
				return;
			}
			if (_id==0) _wc->_XFormTimerStop();

			_wc->_MaskReplace(bx, by, bz, blockptr);

		}

	}
}

int WaveCodecIO::ReadWriteThreadObj::_FetchBlock(
	size_t bx, size_t by, size_t bz
) {
	float *cvectorptr = _wc->_cvectorThread[_id];
	unsigned char *svectorptr = _wc->_svectorThread[_id];
	int rc;

	unsigned long LSBTest = 1;
	bool do_swapbytes = false;
	if (! (*(char *) &LSBTest)) {
		// swap to MSBFirst
		do_swapbytes = true;
	}

	//
	// This method is serialized. OK for all threads to call Timer methods
	//
	_wc->_ReadTimerStart();


	for (int j=0; j<_wc->_sigmapsThread[_id].size(); j++) {
		_wc->_sigmapsThread[_id][j].Clear();
	}
	for(int j=0; j<=_wc->_lod; j++) {
#ifdef PNETCDF
		MPI_Offset start[] = {0,0,0,0};
		MPI_Offset wcount[] = {1,1,1,1};
		MPI_Offset scount[] = {1,1,1,1};
#else
		size_t start[] = {0,0,0,0};
		size_t wcount[] = {1,1,1,1};
		size_t scount[] = {1,1,1,1};
#endif

		bool reconstruct_sigmap = 
				 ((_wc->_cratios.size() == (j+1)) && (_wc->_cratios[j] == 1));

		if (_wc->_vtype == VAR3D) {
			start[0] = bz;
			start[1] = by;
			start[2] = bx;
			wcount[3] = _wc->_ncoeffs[j];
			scount[3] = _wc->_sigmapsizes[j];
		}
		else {
			start[0] = by;
			start[1] = bx;
			wcount[2] = _wc->_ncoeffs[j];
			scount[2] = _wc->_sigmapsizes[j];
		}
		/*		if(start[0] == 2)
		  wcount[0] = 0;
		if(start[1] == 2)
		  wcount[1] = 0;
		if(start[2] == 2)
		wcount[2] = 0;*/
#ifdef PNETCDF
		//ncmpi_begin_indep_data(_wc->_ncids[j]);
		rc = ncmpi_get_vars_float_all(_wc->_ncids[j], _wc->_nc_wave_vars[j], start, wcount, NULL, cvectorptr);
#else
		rc = nc_get_vars_float(
			_wc->_ncids[j], _wc->_nc_wave_vars[j], start, wcount, NULL, cvectorptr
		);
#endif
		if(rc != NC_NOERR)

		NC_ERR_READ(rc, _wc->_ncpaths[j]);
		cvectorptr += _wc->_ncoeffs[j];

		//
		// Signficance map is concatenated to the end of the wavelet
		// coefficients for each block. This improves write
		// performance signficantly over having two separate 
		// variables (one for coefficients and one for sig map)
		//
		if (! reconstruct_sigmap) {
			if (_wc->_vtype == VAR3D) 
				start[3] += _wc->_ncoeffs[j];
			else
				start[2] += _wc->_ncoeffs[j];

			// 
			// Read sig map as untyped quantity to avoid data conversion
			//
#ifdef PNETCDF
			rc = ncmpi_get_vars_float_all(_wc->_ncids[j], _wc->_nc_wave_vars[j], start, scount, NULL,(float*) svectorptr);
#else
			rc = nc_get_vars(
				_wc->_ncids[j], _wc->_nc_wave_vars[j], start, scount, NULL, svectorptr
	  );
			//ncmpi_end_indep_data(_wc->_ncids[j]);
#endif
		if(rc != NC_NOERR)

	if (do_swapbytes) {
				swapbytes(
					(void *) svectorptr, NC_FLOAT_SZ, 
					_wc->_sigmapsizes[j]
				);
			}

			NC_ERR_READ(rc, _wc->_ncpaths[j]);

			rc = _wc->_sigmapsThread[_id][j].SetMap(svectorptr);
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
			_wc->_sigmapsThread[_id][j].Clear();
			for (int i=0; i<j; i++) {
				_wc->_sigmapsThread[_id][j].Append(_wc->_sigmapsThread[_id][i]);
			}
			_wc->_sigmapsThread[_id][j].Sort();
			_wc->_sigmapsThread[_id][j].Invert();
		}

	}

	_wc->_ReadTimerStop();

	return(0);
}
