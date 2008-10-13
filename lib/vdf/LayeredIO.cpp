#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cassert>
#include "vapor/LayeredIO.h"

using namespace VetsUtil;
using namespace VAPoR;

int	LayeredIO::_LayeredIO()
{
	SetClassName("LayeredIO");

	_elevReader = NULL;

	_elevBlkBuf = NULL;
	_varBlkBuf = NULL;
	_blkBufSize = 0;

	_elevReader = new WaveletBlock3DRegionReader(_metadata, _nthreads);
	if (_elevReader->GetErrCode()) return(-1);

	cache_clear();
	_setDefaultHighLowVals();

	SetGridHeight(_dim[2]);
	SetInterpolateOnOff(true);

	return(0);
}

LayeredIO::LayeredIO(
	const Metadata *metadata,
	unsigned int	nthreads
) : WaveletBlock3DRegionReader(metadata, nthreads) {

	_objInitialized = 0;
	if (LayeredIO::GetErrCode()) return;

	SetDiagMsg("LayeredIO::LayeredIO()");

	if (_LayeredIO() < 0) return;

	_objInitialized = 1;
}

LayeredIO::LayeredIO(
	const char *metafile,
	unsigned int	nthreads
) : WaveletBlock3DRegionReader(metafile, nthreads) {

	_objInitialized = 0;
	if (LayeredIO::GetErrCode()) return;

	SetDiagMsg("LayeredIO::LayeredIO(%s)", metafile);

	if (_LayeredIO() < 0) return;

	_objInitialized = 1;
}

LayeredIO::~LayeredIO(
) {

	SetDiagMsg("LayeredIO::~LayeredIO()");
	if (! _objInitialized) return;

	LayeredIO::CloseVariable();

	if (_elevReader) delete _elevReader;
	_elevReader = NULL;

	if (_varBlkBuf) delete _varBlkBuf;
	_varBlkBuf = NULL;

	if (_elevBlkBuf) delete _elevBlkBuf;
	_elevBlkBuf = NULL;

	_objInitialized = 0;
}

int	LayeredIO::OpenVariableRead(
	size_t	timestep,
	const char	*varname,
	int reflevel
) {
	int	rc;

	SetDiagMsg(
		"LayeredIO::OpenVariableRead(%d, %s, %d)",
		timestep, varname, reflevel
	);

	LayeredIO::CloseVariable();	// close any previously opened files.
	rc = WaveletBlock3DRegionReader::OpenVariableRead(
			timestep, varname, reflevel
		);
	if (rc < 0) return(rc);

	// Can't handle layered data not defined on full Z domain
	//
	size_t dim[3];
	WaveletBlock3DRegionReader::GetDim(dim, -1);
	assert(_validRegMin[2] == 0);
	assert(_validRegMax[2] == dim[2]-1);

	rc = _elevReader->OpenVariableRead(timestep, "ELEVATION", reflevel);

	return(rc);
}

int	LayeredIO::CloseVariable(
) {
	SetDiagMsg("LayeredIO::CloseVariable()");

	WaveletBlock3DRegionReader::CloseVariable();

	return(_elevReader->CloseVariable()); 
}


int	LayeredIO::ReadRegion(
	const size_t min[3],
	const size_t max[3],
	float *region
) {
	SetDiagMsg(
		"LayeredIO::ReadRegion((%d,%d,%d),(%d,%d,%d))",
		min[0], min[1], min[2], max[0], max[1], max[2]
	);

	if (! _interpolateOn) {
		return(WaveletBlock3DRegionReader::ReadRegion(min, max, region));
	}

	if (! LayeredIO::IsValidRegion(min, max, _reflevel)) {
		SetErrMsg(
			"Invalid region : (%d %d %d) (%d %d %d)", 
			min[0], min[1], min[2], max[0], max[1], max[2]
		);
		return(-1);
	}


	SetErrMsg("Not supported");
	return(-1);

}

bool LayeredIO::cache_check(
	size_t timestep,
	const size_t bmin[3],
	const size_t bmax[3]
) {
	if (_cacheEmpty) return(false);
	if (timestep != _cacheTimeStep) return(false);
	for (int i=0; i<3; i++) {
		if (bmin[i] != _cacheBMin[i]) return(false);
		if (bmax[i] != _cacheBMax[i]) return(false);
	}
	return(true);
}

void LayeredIO::cache_set(
	size_t timestep,
	const size_t bmin[3],
	const size_t bmax[3]
) {
	_cacheTimeStep = timestep;
	for (int i=0; i<3; i++) {
		_cacheBMin[i] = bmin[i];
		_cacheBMax[i] = bmax[i];
	}
	_cacheEmpty = false;
}

void LayeredIO::cache_clear(
) {
	_cacheEmpty = true;
}
		
int	LayeredIO::BlockReadRegion(
	const size_t bmin[3],
	const size_t bmax[3],
	float *region,
	int unblock
) {

	SetDiagMsg(
		"LayeredIO::BlockReadRegion((%d,%d,%d),(%d,%d,%d))",
		bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]
	);

	if (! _interpolateOn) {
		return(
			WaveletBlock3DRegionReader::BlockReadRegion(
			bmin, bmax, region, unblock)
		);
	}
	assert(unblock = 1);

	if (! LayeredIO::IsValidRegionBlk(bmin, bmax, _reflevel)) {
		SetErrMsg(
			"Invalid region : (%d %d %d) (%d %d %d)", 
			bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]
		);
		return(-1);
	}

	// Get block dimensions of region in native coordinates with
	// full Z extent
	//
	size_t nativeBlkDims[3];
	VDFIOBase::GetDimBlk(nativeBlkDims, _reflevel);
	size_t blkMinFullZ[3], blkMaxFullZ[3];	// Native coords
	for (int i=0; i<2; i++) {
		blkMinFullZ[i] = bmin[i];
		blkMaxFullZ[i] = bmax[i];
	}
	blkMinFullZ[2] = 0;
	blkMaxFullZ[2] = nativeBlkDims[2]-1;

	// Size of interpolated region in voxels (with full Z extent)
	//
	size_t size = 1;
	for(int i=0; i<3; i++) {
		size *= (blkMaxFullZ[i] - blkMinFullZ[i] + 1) * _bs[i];
	}

	// Allocate space for layered data, if needed
	//
	if (size > _blkBufSize) {
		cache_clear();
		if (_varBlkBuf) delete [] _varBlkBuf;
		if (_elevBlkBuf) delete [] _elevBlkBuf;

		_varBlkBuf = new float[size];
		if (! _varBlkBuf) {
			SetErrMsg("new float[%d] : %s", size, strerror(errno));
			return(-1);
		}

		_elevBlkBuf = new float[size];
		if (! _elevBlkBuf) {
			SetErrMsg("new float[%d] : %s", size, strerror(errno));
			return(-1);
		}
		_blkBufSize = size;
	}


	// Read layered (native) data. Presently read entire vertical 
	// extent whether needed or not.
	//

	// See if *elevation* data are already cached. If not, read 
	// from disk.
	//
	int rc;
	if (! cache_check(_timeStep, blkMinFullZ, blkMaxFullZ)) {
		rc = _elevReader->BlockReadRegion(
			blkMinFullZ, blkMaxFullZ, _elevBlkBuf, 1
		);
		if (rc<0) return(-1);

		// update cache
		//
		cache_set(_timeStep, blkMinFullZ, blkMaxFullZ);
	}

	rc = WaveletBlock3DRegionReader::BlockReadRegion(
		blkMinFullZ, blkMaxFullZ, _varBlkBuf, 1
	);
	if (rc<0) return(-1);

	// Finally interpolate layered data to uniform grid
	//

	float lowVal = GetLowValue(_varName);
	float highVal = GetHighValue(_varName);
	_interpolateRegion(
		region, _elevBlkBuf, _varBlkBuf, blkMinFullZ, blkMaxFullZ, 
		bmin[2], bmax[2], lowVal, highVal
	);

	return(0);
}

int LayeredIO::SetGridHeight(size_t height) {

	_gridHeight = height;

	return(0);
}

void LayeredIO::SetInterpolateOnOff(bool on)
{
	_interpolateOn = on;
}

void LayeredIO::SetLowHighVals(
	const vector<string>&varNames, 
	const vector<float>&lowVals, 
	const vector<float>&highVals
) {       
    _lowValMap.clear();
    _highValMap.clear();
    _setDefaultHighLowVals();
    for (int i = 0; i<varNames.size(); i++){
        _lowValMap[varNames[i]] = lowVals[i];
        _highValMap[varNames[i]] = highVals[i];
    } 
}       


void LayeredIO::_setDefaultHighLowVals()
{           
     //Set all variables in the Metadata to defaults:
            
    const vector<string> mdnames = _metadata->GetVariableNames();
    for (int i = 0; i< mdnames.size(); i++){
            
        _lowValMap[mdnames[i]] = BELOW_GRID;
        _highValMap[mdnames[i]] = ABOVE_GRID;
     
    }
}

void LayeredIO::_interpolateRegion(
	float *region, const float *elevBlks, const float *varBlks, 
	const size_t blkMin[3], const size_t blkMax[3], 
	size_t zmini, size_t zmaxi, float lowVal, float highVal
) const {
	// Dimenions in voxels. nx & ny are the dimensions for both
	// the interpolated and native (layered) ROI. nz is the Z dimension
	// of the native ROI. nzi is the Z dimension of the interpolated
	// ROI
	//
	size_t nx = (blkMax[0] - blkMin[0] + 1) * _bs[0];
	size_t ny = (blkMax[1] - blkMin[1] + 1) * _bs[1];
	size_t nz = (blkMax[2] - blkMin[2] + 1) * _bs[2];
	size_t nzi = (zmaxi-zmini+1) * _bs[2];	// Interpolated Z dimension

	size_t x, y;	// X&Y Coordinates of both interpolated grid
					// and original data grid relative to ROI

	size_t xx, yy;	// X&Y Coordinates of both interpolated grid
					// and original data grid relative to full volume domain

	size_t zi;		// Z coordinate of interpolated grid relative to ROI
	size_t zzi;		// Z coordinate of interpolated grid relative to full vol
	size_t z;		// Z coordinate of original grid relative to ROI

	// Even though we're working with blocks we need the coorindate
	// of the top most valid z voxel within the block ('cause
	// we don't know how the blocks were padded)
	//
	size_t dim[3];
	VDFIOBase::GetDim(dim,_reflevel);

	// coordinate of last valid voxel in Z relativeo to ROI coords
	//
	size_t zztop = dim[2]-1 - (blkMin[2] * _bs[0]);	

	// Vertical extents of interpolated grid in user coords
	// N.B. only get extents at X,Y origin (should be constant for
	// all X & Y
	//
	size_t vcoordi[3] = {0,0,zmini*_bs[2]};
	double ucoordi[3];	// user coorindates of interpolated grid 
	LayeredIO::MapVoxToUser(_timeStep, vcoordi, ucoordi, _reflevel);
	float zbottomi_u = ucoordi[2];

	vcoordi[2] = (zmaxi+1)*_bs[2];
	LayeredIO::MapVoxToUser(_timeStep, vcoordi, ucoordi, _reflevel);
	float ztopi_u = ucoordi[2];
	float zdeltai_u = (ztopi_u-zbottomi_u)/(float) nzi;

	for (x=0, xx=blkMin[0]*_bs[1]; x<nx; x++, xx++) {
	for (y=0, yy=blkMin[1]*_bs[1]; y<ny; y++, yy++) {

		float *regPtr = &region[nx*ny*0 + nx*y + x];
		const float *varPtr0 = &varBlks[nx*ny*0 + nx*y + x];
		const float *varPtr1 = &varBlks[nx*ny*1 + nx*y + x];
		const float *elePtr0 = &elevBlks[nx*ny*0 + nx*y + x];
		const float *elePtr1 = &elevBlks[nx*ny*1 + nx*y + x];

		float zi_u = zbottomi_u;
		for (zi = 0, zzi=zmini*_bs[2], z=0; zi<nzi; zi++, zzi++) {
			// size_t vcoordi[3] = {xx,yy,zzi};
			// double ucoordi[3];	// user coorindates of interpolated grid 
			// LayeredIO::MapVoxToUser(_timeStep, vcoordi, ucoordi, _reflevel);

			// Below the grid
			if (zi_u < elevBlks[nx*ny*0 + nx*y + x]) {
				*regPtr = lowVal;
			}
			// Above the grid
			else if (zi_u > elevBlks[nx*ny*zztop + nx*y + x]) {
				*regPtr = highVal;
			}
			else {
				while (zi_u > *elePtr1 && z < nz) {
					z++;
					elePtr0 = elePtr1;
					elePtr1 += nx*ny;
					varPtr0 = varPtr1;
					varPtr1 += nx*ny;
				}
				assert(zi_u >= *elePtr0 && zi_u <= *elePtr1);

				float frac = (zi_u - *elePtr0) / (*elePtr1 - *elePtr0);
				*regPtr = ((1.0f - frac) * *varPtr0) + (frac * *varPtr1);
			}

			regPtr += nx*ny;
			zi_u += zdeltai_u;
		}

	}
	}
}


void    LayeredIO::GetDim(
    size_t dim[3], int reflevel
) const {

	if (! _interpolateOn) { 
		VDFIOBase::GetDim(dim, reflevel);
		return;
	}
 
	// X & Y dims are same as for parent class
	//
	VDFIOBase::GetDim(dim, reflevel);

	// Now deal with Z dim
	//
    if (reflevel < 0) reflevel = _num_reflevels - 1;
    int  ldelta = _num_reflevels - 1 - reflevel; 
 
	dim[2] = _gridHeight >> ldelta;

	// Deal with odd dimensions 
	if ((dim[2] << ldelta) < _gridHeight) dim[2]++;
}
 
void    LayeredIO::GetDimBlk(
    size_t bdim[3], int reflevel
) const {

	if (! _interpolateOn) { 
		VDFIOBase::GetDimBlk(bdim, reflevel);
		return;
	}

    size_t dim[3];
 
    LayeredIO::GetDim(dim, reflevel); 

    for (int i=0; i<3; i++) {
        bdim[i] = (size_t) ceil ((double) dim[i] / (double) _bs[i]);
    }
}

int LayeredIO::IsValidRegion(
	const size_t min[3],
	const size_t max[3],
	int reflevel
) const {

	if (! _interpolateOn) {
		return(VDFIOBase::IsValidRegion(min, max, reflevel));
	}

	size_t dim[3];
	LayeredIO::GetDim(dim, reflevel);

	for(int i=0; i<3; i++) {
		if (min[i] > max[i]) return (0);
		if (max[i] >= dim[i]) return (0);
	}
	return(1);
}

int LayeredIO::IsValidRegionBlk( 
	const size_t min[3],
	const size_t max[3],
	int reflevel
) const {

	if (! _interpolateOn) {
		return(VDFIOBase::IsValidRegionBlk(min, max, reflevel));
	}
	size_t dim[3];
	LayeredIO::GetDimBlk(dim, reflevel);

	for(int i=0; i<3; i++) {
		if (min[i] > max[i]) return (0);
		if (max[i] >= dim[i]) return (0);
	}
	return(1);
}

void    LayeredIO::GetValidRegion(
    size_t min[3], size_t max[3], int reflevel
) const {

	if (! _interpolateOn) {
		VDFIOBase::GetValidRegion(min, max, reflevel);
		return;
	}

	// X & Y dims are same as for parent class
	//
	VDFIOBase::GetValidRegion(min, max, reflevel);

	// Now deal with Z dim
	//
	// N.B. presently full Z dimension is required. I.e. Z extent of
	// ROI must be full domain
	//
    if (reflevel < 0) reflevel = _num_reflevels - 1;
    int  ldelta = _num_reflevels - 1 - reflevel;

	min[2] = 0;
	max[2] = _gridHeight >> ldelta;
}

void    LayeredIO::MapVoxToUser(
	size_t timestep,
	const size_t vcoord0[3], double vcoord1[3],
	int reflevel
) const {

	if (! _interpolateOn) {
		VDFIOBase::MapVoxToUser(timestep, vcoord0, vcoord1, reflevel);
		return;
	}

	if (reflevel < 0) reflevel = _num_reflevels - 1;
	int  ldelta = _num_reflevels - 1 - reflevel;

	size_t  dim[3];

	const vector <double> &extents = _metadata->GetExtents();
	LayeredIO::GetDim(dim, -1);   // finest dimension
	for(int i = 0; i<3; i++) {

		// distance between voxels along dimension 'i' in user coords
		double deltax = (extents[i+3] - extents[i]) / (dim[i] - 1);

		// coordinate of first voxel in user space
		double x0 = extents[i];

		// Boundary shrinks and step size increases with each transform
		for(int j=0; j<(int)ldelta; j++) {
			x0 += 0.5 * deltax;
			deltax *= 2.0;
		}
		vcoord1[i] = x0 + (vcoord0[i] * deltax);
	}
}


void	LayeredIO::MapUserToVox(
    size_t timestep, const double vcoord0[3], size_t vcoord1[3],
	int	reflevel
) const {

	if (! _interpolateOn) {
		VDFIOBase::MapUserToVox(timestep, vcoord0, vcoord1, reflevel);
		return;
	}

	if (reflevel < 0) reflevel = _num_reflevels - 1;
	int	 ldelta = _num_reflevels - 1 - reflevel;

	size_t	dim[3];
	const vector <double> &extents = _metadata->GetExtents();
	// assert(_metadata->GetErrCode() == 0);

	vector <double> lextents = extents;

	LayeredIO::GetDim(dim, reflevel);	
	for(int i = 0; i<3; i++) {
		double a;

		// distance between voxels along dimension 'i' in user coords at full resolution
		double deltax;
		if (i == 2) deltax = (lextents[i+3] - lextents[i]) / (_gridHeight - 1);
		else deltax = (lextents[i+3] - lextents[i]) / (_dim[i] - 1);

		// coordinate of first voxel in user space
		double x0 = lextents[i];

		// Boundary shrinks and step size increases with each transform
		
		for(int j=0; j<(int)ldelta; j++) {
			x0 += 0.5 * deltax;
			deltax *= 2.0;
		}
		lextents[i] = x0;
		lextents[i+3] = lextents[i] + (deltax * (dim[i]-1));

		a = (vcoord0[i] - lextents[i]) / (lextents[i+3]-lextents[i]);
		if (a < 0.0) vcoord1[i] = 0;
		else vcoord1[i] = (size_t) rint(a * (double) (dim[i]-1));

		if (vcoord1[i] > (dim[i]-1)) vcoord1[i] = dim[i]-1;
	}
}
