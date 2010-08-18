//
// $Id$
//
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cassert>
#include <vapor/LayeredIO.h>

using namespace VetsUtil;
using namespace VAPoR;

int	LayeredIO::_LayeredIO()
{
	_elevBlkBuf = NULL;
	_varBlkBuf = NULL;
	_blkBufSize = 0;

	cache_clear();
	_lowValMap.clear();
	_highValMap.clear();

	SetGridHeight(100);
	SetInterpolateOnOff(true);

	return(0);
}

LayeredIO::LayeredIO(
	size_t mem_size
) : DataMgr(mem_size) {

	SetDiagMsg("LayeredIO::LayeredIO()");

	if (_LayeredIO() < 0) return;
}

LayeredIO::~LayeredIO() {

	SetDiagMsg("LayeredIO::~LayeredIO()");

	if (_varBlkBuf) delete _varBlkBuf;
	_varBlkBuf = NULL;

	if (_elevBlkBuf) delete _elevBlkBuf;
	_elevBlkBuf = NULL;

}

int	LayeredIO::OpenVariableRead(
	size_t	timestep,
	const char	*varname,
	int reflevel,
	int lod
) {
	int	rc;

	SetDiagMsg(
		"OpenVariableRead(%d, %s, %d)",
		timestep, varname, reflevel
	);

	CloseVariable();	// close any previously opened files.
	rc = OpenVariableReadNative(timestep, varname, reflevel);
	if (rc < 0) return(rc);

	_reflevel = reflevel;
	_lod = lod;
	_timeStep = timestep;
	_varName = varname;
	_vtype = GetVarType(varname);

	if (_vtype == VAR2D_XY) return(0);
	if (_vtype != VAR3D) {
		SetErrMsg("Variable type not supported");
		return(-1);
	}

	// Can't handle layered data not defined on full Z domain
	//
	size_t dim[3];
	GetDimNative(dim, -1);

	size_t regmin[3];
	size_t regmax[3];

	GetValidRegionNative(regmin, regmax, -1);
	assert(regmin[2] == 0);
	assert(regmax[2] == dim[2]-1);

	return(rc);
}

int	LayeredIO::CloseVariable(
) {
	SetDiagMsg("CloseVariable()");

	return(CloseVariableNative());
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
	float *region
) {

	SetDiagMsg(
		"LayeredIO::BlockReadRegion((%d,%d,%d),(%d,%d,%d))",
		bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]
	);

	if (! _interpolateOn || (_vtype == VAR2D_XY)) {
		return(BlockReadRegionNative( bmin, bmax, region));
	}
	if (_vtype != VAR3D) {
		SetErrMsg("Variable type not supported");
		return(-1);
	}

	// Get block dimensions of region in native coordinates with
	// full Z extent
	//
	size_t nativeBlkDims[3];
	GetDimBlkNative(nativeBlkDims, _reflevel);
	size_t blkMinFullZ[3], blkMaxFullZ[3];	// Native coords
	for (int i=0; i<2; i++) {
		blkMinFullZ[i] = bmin[i];
		blkMaxFullZ[i] = bmax[i];
	}
	blkMinFullZ[2] = 0;
	blkMaxFullZ[2] = nativeBlkDims[2]-1;

	const size_t *bs = GetBlockSize();

	// Size of interpolated region in voxels (with full Z extent)
	//
	size_t size = 1;
	for(int i=0; i<3; i++) {
		size *= (blkMaxFullZ[i] - blkMinFullZ[i] + 1) * bs[i];
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
		string my_var_name = _varName;

		// Close currently opened variable
		//
		CloseVariableNative();

		//
		// open and read the ELEVATION variable
		//
		rc = OpenVariableReadNative(
			_timeStep, "ELEVATION", _reflevel, _lod
		);
		if (rc<0) return(-1);

		rc = BlockReadRegionNative(
			blkMinFullZ, blkMaxFullZ, _elevBlkBuf
		);
		if (rc<0) return(-1);

		CloseVariableNative();

		// update cache
		//
		cache_set(_timeStep, blkMinFullZ, blkMaxFullZ);

		// Reopen the previously opened field variable
		//
		rc = OpenVariableReadNative(
			_timeStep, my_var_name.c_str(), _reflevel, _lod
		);
	}

	rc = BlockReadRegionNative(
		blkMinFullZ, blkMaxFullZ, _varBlkBuf
	);
	if (rc<0) return(-1);

	// Finally interpolate layered data to uniform grid
	//

	
	float *lowValPtr = NULL;
	map <string, float>::iterator itr;
	if ((itr = _lowValMap.find(_varName)) != _lowValMap.end()) lowValPtr = &(itr->second);

	float *highValPtr = NULL;
	if ((itr = _highValMap.find(_varName)) != _highValMap.end()) highValPtr = &(itr->second);

	_interpolateRegion(
		region, _elevBlkBuf, _varBlkBuf, blkMinFullZ, blkMaxFullZ, 
		bmin[2], bmax[2], lowValPtr, highValPtr
	);

	return(0);
}

void LayeredIO::SetLowVals(
	const vector<string>&varNames, 
	const vector<float>&vals
) {       
    _lowValMap.clear();
    for (int i = 0; i<varNames.size(); i++){
        _lowValMap[varNames[i]] = vals[i];
    } 
}       

void LayeredIO::SetHighVals(
	const vector<string>&varNames, 
	const vector<float>&vals
) {       
    _highValMap.clear();
    for (int i = 0; i<varNames.size(); i++){
        _highValMap[varNames[i]] = vals[i];
    } 
}       

void LayeredIO::GetLowVals(
	vector<string>&varNames, 
	vector<float>&vals
) {       
	varNames.clear(); 
	vals.clear();

	map <string, float>::iterator itr;
	for (itr = _lowValMap.begin(); itr != _lowValMap.end(); itr++) {
		varNames.push_back(itr->first);
		vals.push_back(itr->second);
	}
}

void LayeredIO::GetHighVals(
	vector<string>&varNames, 
	vector<float>&vals
) {       
	varNames.clear(); 
	vals.clear();

	map <string, float>::iterator itr;
	for (itr = _lowValMap.begin(); itr != _lowValMap.end(); itr++) {
		varNames.push_back(itr->first);
		vals.push_back(itr->second);
	}
}

int LayeredIO::SetGridHeight(size_t height) 
{
	_gridHeight = height; 
	return(0);
};



void LayeredIO::_interpolateRegion(
	float *region, const float *elevBlks, const float *varBlks, 
	const size_t blkMin[3], const size_t blkMax[3], 
	size_t zmini, size_t zmaxi, float *lowValPtr, float *highValPtr
) const {
	// Dimenions in voxels. nx & ny are the dimensions for both
	// the interpolated and native (layered) ROI. nz is the Z dimension
	// of the native ROI. nzi is the Z dimension of the interpolated
	// ROI
	//
	const size_t *bs = GetBlockSize();

	size_t nx = (blkMax[0] - blkMin[0] + 1) * bs[0];
	size_t ny = (blkMax[1] - blkMin[1] + 1) * bs[1];
	size_t nz = (blkMax[2] - blkMin[2] + 1) * bs[2];
	size_t nzi = (zmaxi-zmini+1) * bs[2];	// Interpolated Z dimension

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
	GetDimNative(dim,_reflevel);

	// coordinate of last valid voxel in Z relativeo to ROI coords
	//
	size_t zztop = dim[2]-1 - (blkMin[2] * bs[0]);	

	// Vertical extents of interpolated grid in user coords
	// N.B. only get extents at X,Y origin (should be constant for
	// all X & Y
	//
	size_t vcoordi[3] = {0,0,zmini*bs[2]};
	double ucoordi[3];	// user coorindates of interpolated grid 
	MapVoxToUser(_timeStep, vcoordi, ucoordi, _reflevel);
	float zbottomi_u = ucoordi[2];

	vcoordi[2] = (zmaxi+1)*bs[2];
	MapVoxToUser(_timeStep, vcoordi, ucoordi, _reflevel);
	float ztopi_u = ucoordi[2];
	float zdeltai_u = (ztopi_u-zbottomi_u)/(float) nzi;

	for (x=0, xx=blkMin[0]*bs[1]; x<nx; x++, xx++) {
	for (y=0, yy=blkMin[1]*bs[1]; y<ny; y++, yy++) {

		float *regPtr = &region[nx*ny*0 + nx*y + x];
		const float *varPtr0 = &varBlks[nx*ny*0 + nx*y + x];
		const float *varPtr1 = &varBlks[nx*ny*1 + nx*y + x];
		const float *elePtr0 = &elevBlks[nx*ny*0 + nx*y + x];
		const float *elePtr1 = &elevBlks[nx*ny*1 + nx*y + x];

		float zi_u = zbottomi_u;
		for (zi = 0, zzi=zmini*bs[2], z=0; zi<nzi; zi++, zzi++) {

			// Below the grid
			if (zi_u < elevBlks[nx*ny*0 + nx*y + x]) {
				if (lowValPtr) *regPtr = *lowValPtr;
				else *regPtr = varBlks[nx*ny*0 + nx*y + x];
			}
			// Above the grid
			else if (zi_u > elevBlks[nx*ny*zztop + nx*y + x]) {
				if (highValPtr) *regPtr = *highValPtr;
				else *regPtr = varBlks[nx*ny*zztop + nx*y + x];
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

void    LayeredIO::GetGridDim(
    size_t dim[3]
) const {
	return(GetDim(dim, -1));
}

void    LayeredIO::GetDim(
    size_t dim[3], int reflevel
) const {

	GetDimNative(dim, reflevel);

	if (! _interpolateOn) { 
		return;
	}
 
	// X & Y dims are same as for parent class
	//
	// Now deal with Z dim
	//
    if (reflevel < 0) reflevel = GetNumTransforms();
    int  ldelta = GetNumTransforms() - reflevel; 
 
	dim[2] = _gridHeight >> ldelta;

	// Deal with odd dimensions 
	if ((dim[2] << ldelta) < _gridHeight) dim[2]++;
}
 
void    LayeredIO::GetDimBlk(
    size_t bdim[3], int reflevel
) const {

	if (! _interpolateOn) { 
		GetDimBlkNative(bdim, reflevel);
		return;
	}

    size_t dim[3];
 
    GetDimNative(dim, reflevel); 

	const size_t *bs = GetBlockSize();

    for (int i=0; i<3; i++) {
        bdim[i] = (size_t) ceil ((double) dim[i] / (double) bs[i]);
    }
}

void    LayeredIO::GetValidRegion(
    size_t min[3], size_t max[3], int reflevel
) const {

	if (! _interpolateOn) {
		GetValidRegionNative(min, max, reflevel);
		return;
	}

	// X & Y dims are same as for parent class
	//
	GetValidRegionNative(min, max, reflevel);

	// Now deal with Z dim
	//
	// N.B. presently full Z dimension is required. I.e. Z extent of
	// ROI must be full domain
	//
    if (reflevel < 0) reflevel = GetNumTransforms();
    int  ldelta = GetNumTransforms() - reflevel;

	min[2] = 0;
	max[2] = (_gridHeight-1) >> ldelta;
}

void    LayeredIO::MapVoxToUser(
	size_t timestep,
	const size_t vcoord0[3], double vcoord1[3],
	int reflevel
) const {

	if (! _interpolateOn) {
		MapVoxToUserNative(timestep, vcoord0, vcoord1, reflevel);
		return;
	}

	if (reflevel < 0) reflevel = GetNumTransforms();
	int  ldelta = GetNumTransforms() - reflevel;

	size_t  dim[3];

	vector <double> extents = GetTSExtents(timestep);
	if (! extents.size()) extents = GetExtents();

	GetDim(dim, -1);   // finest dimension
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
		MapUserToVoxNative(timestep, vcoord0, vcoord1, reflevel);
		return;
	}

	if (reflevel < 0) reflevel = GetNumTransforms();
	int	 ldelta = GetNumTransforms() - reflevel;

	vector <double> extents = GetTSExtents(timestep);
	if (! extents.size()) extents = GetExtents();
	// assert(GetErrCode() == 0);

	size_t	my_dim[3];
	GetDim(my_dim, reflevel);	

	size_t dim[3]; 
	GetDimNative(dim, -1);

	for(int i = 0; i<3; i++) {
		double a;

		// distance between voxels along dimension 'i' in user coords at full resolution
		double deltax;
		if (i == 2) deltax = (extents[i+3] - extents[i]) / (_gridHeight - 1);
		else deltax = (extents[i+3] - extents[i]) / (dim[i] - 1);

		// coordinate of first voxel in user space
		double x0 = extents[i];

		// Boundary shrinks and step size increases with each transform
		
		for(int j=0; j<(int)ldelta; j++) {
			x0 += 0.5 * deltax;
			deltax *= 2.0;
		}
		extents[i] = x0;
		extents[i+3] = extents[i] + (deltax * (my_dim[i]-1));

		a = (vcoord0[i] - extents[i]) / (extents[i+3]-extents[i]);
		if (a < 0.0) vcoord1[i] = 0;
		else vcoord1[i] = (size_t) rint(a * (double) (my_dim[i]-1));

		if (vcoord1[i] > (my_dim[i]-1)) vcoord1[i] = my_dim[i]-1;
	}
}
