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
#include "vapor/VDFIOBase.h"
#include "vapor/LayeredIO.h"
#include "vapor/WaveletBlock3DRegionReader.h"
#include "vapor/MyBase.h"

using namespace VetsUtil;
using namespace VAPoR;



int	LayeredIO::_LayeredIO(
) {
	
	SetClassName("LayeredIO");

	if (_num_reflevels > MAX_LEVELS) {
		SetErrMsg("Too many refinement levels");
		return(-1);
	}
	_xform_timer = 0.0;
	_reflevel = 0;
	is_open_c = 0;
	//dimensions will be reset when file is opened; 
	//Initially use number of vertical layers.
	

	return(0);
}

LayeredIO::LayeredIO(
	const Metadata	*metadata,
	unsigned int	nthreads
) : VDFIOBase(metadata, nthreads) {
	
	_objInitialized = 0;
	if (VDFIOBase::GetErrCode()) return;
	wbRegionReader = new WaveletBlock3DRegionReader(metadata,nthreads);
	if (_LayeredIO() < 0) return;
	_objInitialized = 1;
}

LayeredIO::LayeredIO(
	const char *metafile,
	unsigned int	nthreads
) : VDFIOBase(metafile, nthreads) {
	_objInitialized = 0;
	if (VDFIOBase::GetErrCode()) return;
	wbRegionReader = new WaveletBlock3DRegionReader(metafile,nthreads);
	if (_LayeredIO() < 0) return;
	_objInitialized = 1;
}

LayeredIO::~LayeredIO() {

	if (wbRegionReader) delete wbRegionReader;
	if (! _objInitialized) return;
	_objInitialized = 0;
}

int    LayeredIO::VariableExists(
    size_t timestep,
    const char *varname,
    int reflevel
) const {
	return wbRegionReader->VariableExists(timestep, varname, reflevel);
}

int	LayeredIO::OpenVariableWrite(
	size_t timestep,
	const char *varname,
	int reflevel
) {
	//Not implemented yet
	return(0);
}

void	LayeredIO::SetGridHeight(size_t full_height) {
	if (full_height != 0) _dim[2] = full_height;
	GetDimBlk(_bdim, _num_reflevels-1);
}



int	LayeredIO::OpenVariableRead(
	size_t timestep,
	const char *varname,
	int reflevel,
	size_t full_height
) {
	_timeStep = timestep;
	_varName.assign(varname);
	_reflevel = reflevel;
	
	if (full_height != 0) _dim[2] = full_height;
	GetDimBlk(_bdim, _num_reflevels-1);
	
	return 0;
}

int	LayeredIO::CloseVariable()
{
	
	if (! is_open_c) return(0);

	//Currently  a no-op.  may have to do something when we support writing.
				
	is_open_c = 0;

	return(0);
}
//Produce interpolated region, using elev grid (elevBlks) and variable data on levels (levVarBlks)
// minblks, maxblks specify the region in block coords
void LayeredIO::
InterpolateRegion(float* interpBlks, const float* elevBlks, const float* levVarBlks, 
				  const size_t minblks[3], const size_t maxblks[3], size_t full_height,
				  float lowVal, float highVal)
{
	//First, find the actual coord extents that we are going to interpolate:
	size_t minInterpGrid[3], maxInterpGrid[3];
	size_t minLayerGrid[3], maxLayerGrid[3];
	size_t minRel[2], maxRel[2];
	//Find the valid region in the layered data:
	GetValidRegion(minLayerGrid, maxLayerGrid, 0, _reflevel);
	//Find min and max valid x, y coordinates relative to block boundary.
	for (int i = 0; i<2; i++){
		//Start with absolute grid coords
		minRel[i] = minblks[i]*_bs[i];
		maxRel[i] = (maxblks[i]+1)*_bs[i]-1;
		//modify to put them inside valid data
		if (minRel[i] < minLayerGrid[i]) minRel[i] = minLayerGrid[i];
		if (maxRel[i] > maxLayerGrid[i]) maxRel[i] = maxLayerGrid[i];
		//then translate back relative to block boundary:
		minRel[i] -= minblks[i]*_bs[i];
		maxRel[i] -= minblks[i]*_bs[i];
		assert(minRel[i] >= 0 && (minRel[i] < (maxblks[i]+1)*_bs[i] ) );
		assert(maxRel[i] >= 0 && (maxRel[i] < (maxblks[i]+1)*_bs[i] ) );
	}
	
	//Note that in z, the full numbers of layers at the current ref level
	//must be available for interpolation.  These are
	//mn[2] = 0 thru mx[2] = numlayers-1.
	assert(minLayerGrid[2] == 0);
	size_t dims[3];
	GetWBReader()->GetDim(dims,_reflevel);
	assert(maxLayerGrid[2] == dims[2]-1);
	//Need to know what is the full min and max elevation that we are interpolating full_height 
	//into.  These are the z-extents of the data set, at the current ref level, 
	//from the metadata
	double minExt[3], maxExt[3];
	//Find the region in the interpolated data
	
	GetValidRegion(minInterpGrid, maxInterpGrid, full_height, _reflevel);
	
	MapVoxToUser(_timeStep, minInterpGrid, minExt, _reflevel);
	MapVoxToUser(_timeStep, maxInterpGrid, maxExt, _reflevel);
	float minElevation = minExt[2];
	float maxElevation = maxExt[2];
	

	//Determine the vertical size of the interpolated grid 
	int interpGridHeight = full_height >> (_num_reflevels - 1 - _reflevel);

	//Restrict the interpolated region based on blocks requested:
	minInterpGrid[2] = _bs[2]*minblks[2];
	if (_bs[2]*(maxblks[2]+1) -1 < maxInterpGrid[2])
		maxInterpGrid[2] = _bs[2]*(maxblks[2]+1) -1;
	
	
	//identify the interval we are interpolating:
	
	int zmin = minInterpGrid[2];
	int zmax = maxInterpGrid[2];
	for (int i = minRel[0]; i<= maxRel[0]; i++){
		for (int j = minRel[1]; j<= maxRel[1]; j++){
	
			//Keep track of the index of the lower level of the elev layer for the
			//current grid point.
			int currentLevel = 0;
			
			
			//Calculate total index associated with i and j, for indexing inside the
			//block region bounds
			int relIndex = i + j*(maxblks[0]-minblks[0]+1)*_bs[0];
			//Determine the increment between adjacent z-values in both leveled and
			//interpolated grids
			int zincrem = (maxblks[0]-minblks[0]+1)*_bs[0]*(maxblks[1]-minblks[1]+1)*_bs[1];
			currentLevel = maxLayerGrid[2] - 1;
			float lowerHeight = elevBlks[relIndex+zincrem*currentLevel];
			float upperHeight = elevBlks[relIndex+zincrem*(currentLevel+1)];
			for (int z = zmax; z >= zmin; z--) {
				// convert the grid index to an elevation.
				// ht is the height of a cell in the grid:
				float ht = (maxElevation - minElevation)/(float)interpGridHeight;
				float vertHeight = minElevation + ht*(float)z;
			
				//compare with top and bottom layers of elevation
				if (vertHeight < elevBlks[relIndex]){
					interpBlks[relIndex + (z-zmin)*zincrem] = lowVal;
					continue;
				}
				else if(vertHeight > elevBlks[relIndex+zincrem*maxLayerGrid[2]]) {
					interpBlks[relIndex + (z-zmin)*zincrem] = highVal;
					continue;
				}
			
				// want vert height between currentLevel and currentLevel+1
				//  If currentLevel is too high, lower it.  It should never be too low.
				while (vertHeight < elevBlks[relIndex + zincrem*currentLevel]){
					currentLevel--;
					assert (currentLevel >= 0);
					upperHeight = lowerHeight;
					lowerHeight = elevBlks[relIndex+zincrem*currentLevel];
				}
				
				
				//Now we should be inside the interval:
				assert (lowerHeight <= vertHeight && upperHeight >= vertHeight);
				assert (currentLevel < maxLayerGrid[2] && currentLevel >= 0);
				//Perform interpolation:
				float frac = (vertHeight - lowerHeight)/(upperHeight - lowerHeight);
				float val = (1.f - frac)*levVarBlks[relIndex+zincrem*currentLevel] +
					frac*levVarBlks[relIndex+zincrem*(currentLevel+1)];
				interpBlks[relIndex + (z-zmin)*zincrem] = val;
			} // end loop over z
			
		} //End loop over j
	} //End loop over i
	return;
}
