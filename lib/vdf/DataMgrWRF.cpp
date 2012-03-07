#include <cassert>
#include <vapor/DataMgrWRF.h>

using namespace VetsUtil;
using namespace VAPoR;

DataMgrWRF::DataMgrWRF(
	const vector <string> &files,
    size_t mem_size
) : LayeredIO(mem_size), WRFReader(files) 
{
	size_t dim[3];
    GetDimNative(dim, -1);
    SetGridHeight(dim[2]);

	_blkptrs = new float*[1];
}

DataMgrWRF::DataMgrWRF(
    const MetadataWRF &metadata,
    size_t mem_size
) : LayeredIO(mem_size), WRFReader(metadata) 
{
	size_t dim[3];
    GetDimNative(dim, -1);
    SetGridHeight(dim[2]);

	_blkptrs = new float*[1];
}

void DataMgrWRF::GetValidRegionNative(
    size_t min[3], size_t max[3], int reflevel
) const {
	size_t dim[3];

	GetDimNative(dim, -1);
	for (int i=0; i<3; i++) {
		min[i] = 0;
		max[i] = dim[i] - 1;
	}
}


RegularGrid *DataMgrWRF::MakeGrid(
	size_t ts, string varname, int reflevel, int lod,
    const size_t bmin[3], const size_t bmax[3], float *blocks
) {
	assert(bmin[0] == 0 && bmin[1] == 0 && bmin[2] == 0);
	assert(bmax[0] == 0 && bmax[1] == 0 && bmax[2] == 0);

	size_t bs[3];
	WRFReader::GetBlockSize(bs,-1);

	size_t bdim[3] = {1,1,1};

	size_t dim[3];
	WRFReader::GetDim(dim,reflevel);

	size_t min[3] = {0,0,0};
	size_t max[3] = {dim[0]-1,dim[1]-1,dim[2]-1};

    //
    // Make sure 2D variables have valid 3rd dimensions
    //
    VarType_T vtype = WRFReader::GetVarType(varname);
    switch (vtype) {
    case VAR2D_XY:
		bs[2] = 1;
		min[2] = max[2] = 0;
        break;
    case VAR2D_XZ:
		bs[1] = 1;
		min[1] = max[1] = 0;
        break;
    case VAR2D_YZ:
		bs[0] = 1;
		min[0] = max[0] = 0;
        break;
    default:
        break;
    }

	_blkptrs[0] = blocks;

	double extents[6];
    WRFReader::MapVoxToUser(ts,min, extents, reflevel);
    WRFReader::MapVoxToUser(ts,max, extents+3, reflevel);

	//
	// Determine which dimensions are periodic, if any. For a dimension to
	// be periodic the data set must be periodic, and the requested
	// blocks must be boundary blocks
	//
	const vector <long> &periodic_vec = WRFReader::GetPeriodicBoundary();

	bool periodic[3];
	for (int i=0; i<3; i++) {
		if (periodic_vec[i] && bmin[i]==0 && bmax[i]==bdim[i]-1) {
			periodic[i] = true;
		}
		else {
			periodic[i] = false;
		}
	}

	if (varname.compare("ELEVATION") == 0) {
		return(new RegularGrid(bs,min,max,extents,periodic,_blkptrs));
	}
	else {

		RegularGrid *elevation = GetGrid(
			ts, "ELEVATION", reflevel, lod, min, max, 0
		);
		if (! elevation) return (NULL);
		float **coords = elevation->GetBlks();
		_blkptrs[0] = blocks;

		delete elevation;

		return(new LayeredGrid(bs,min, max, extents, periodic, _blkptrs, coords,2));
	}
}

RegularGrid *DataMgrWRF::ReadGrid(
	size_t ts, string varname, int reflevel, int lod,
    const size_t bmin[3], const size_t bmax[3], float *blocks
) {
	assert(bmin[0] == 0 && bmin[1] == 0 && bmin[2] == 0);
	assert(bmax[0] == 0 && bmax[1] == 0 && bmax[2] == 0);

    int rc = WRFReader::OpenVariableRead(
		ts, varname.c_str()
	);
    if (rc < 0) return(NULL);

	rc = WRFReader::ReadVariable(blocks);
    if (rc < 0) return(NULL);

	WRFReader::CloseVariable();

	return(MakeGrid(ts,varname,reflevel,lod,bmin,bmax,blocks));
}
