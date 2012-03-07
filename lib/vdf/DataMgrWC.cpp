

#include <vapor/DataMgrWC.h>

using namespace VetsUtil;
using namespace VAPoR;

int VAPoR::DataMgrWC::_DataMgrWC(
) {
	size_t bdim[3];
	WaveCodecIO::GetDimBlk(bdim, -1);

	int nblocks = 1;
	for (int i=0; i<3; i++) {
		nblocks *= bdim[i];
	}
	_blkptrs = new float*[nblocks];
	return(0);
}

 VAPoR::DataMgrWC::DataMgrWC(
    const string &metafile,
    size_t mem_size
 ) : DataMgr(mem_size), WaveCodecIO(metafile) 
{
	(void) _DataMgrWC();
}

 VAPoR::DataMgrWC::DataMgrWC(
    const MetadataVDC &metadata,
    size_t mem_size
 ) : DataMgr(mem_size), WaveCodecIO(metadata) 
{
	(void) _DataMgrWC();
}

RegularGrid *DataMgrWC::MakeGrid(
	size_t ts, string varname, int reflevel, int lod,
    const size_t bmin[3], const size_t bmax[3], float *blocks
) {

	size_t bs[3];
	WaveCodecIO::GetBlockSize(bs,reflevel);

	size_t bdim[3];
	WaveCodecIO::GetDimBlk(bdim,reflevel);

	size_t dim[3];
	WaveCodecIO::GetDim(dim,reflevel);

    //
    // Make sure 2D variables have valid 3rd dimensions
    //
    VarType_T vtype = WaveCodecIO::GetVarType(varname);
    switch (vtype) {
    case VAR2D_XY:
        dim[2] = bdim[2] = bs[2] = 1;
        break;
    case VAR2D_XZ:
        dim[1] = bdim[1] = bs[1] = 1;
        break;
    case VAR2D_YZ:
        dim[0] = bdim[0] = bs[0] = 1;
        break;
    default:
        break;
    }

	int nblocks = 1;
	size_t block_size = 1;
	size_t min[3], max[3];
	for (int i=0; i<3; i++) {
		nblocks *= bmax[i]-bmin[i]+1;
		block_size *= bs[i];
		min[i] = bmin[i]*bs[i];
		max[i] = bmax[i]*bs[i] + bs[i] - 1;
		if (max[i] >= dim[i]) max[i] = dim[i]-1;
	}
	for (int i=0; i<nblocks; i++) {
		_blkptrs[i] = blocks + i*block_size;
	}

	double extents[6];
    WaveCodecIO::MapVoxToUser(ts,min, extents, reflevel);
    WaveCodecIO::MapVoxToUser(ts,max, extents+3, reflevel);

	//
	// Determine which dimensions are periodic, if any. For a dimension to
	// be periodic the data set must be periodic, and the requested
	// blocks must be boundary blocks
	//
	const vector <long> &periodic_vec = WaveCodecIO::GetPeriodicBoundary();

	bool periodic[3];
	for (int i=0; i<3; i++) {
		if (periodic_vec[i] && bmin[i]==0 && bmax[i]==bdim[i]-1) {
			periodic[i] = true;
		}
		else {
			periodic[i] = false;
		}
	}


	if (GetCoordSystemType().compare("spherical")==0) { 
		vector <long> permv = GetGridPermutation();
		size_t perm[] = {permv[0], permv[1], permv[2]};
		return(new SphericalGrid(bs,min,max,extents,perm,periodic,_blkptrs));
	}
	else if (
		! (WaveCodecIO::GetGridType().compare("layered")==0) ||
		varname.compare("ELEVATION") == 0 || vtype != VAR3D
	) {
		return(new RegularGrid(bs,min,max,extents,periodic,_blkptrs));
	}
	else {
		RegularGrid *elevation = GetGrid(
			ts, "ELEVATION", reflevel, lod, min, max, 0
		);
		if (! elevation) return (NULL);
		float **coords = elevation->GetBlks();
		for (int i=0; i<nblocks; i++) {
			_blkptrs[i] = blocks + i*block_size;
		}
		return(new LayeredGrid(bs,min, max, extents, periodic, _blkptrs, coords,2));
	}
}

RegularGrid *DataMgrWC::ReadGrid(
	size_t ts, string varname, int reflevel, int lod,
    const size_t bmin[3], const size_t bmax[3], float *blocks
) {

    int rc = WaveCodecIO::OpenVariableRead(
		ts, varname.c_str(), reflevel, lod
	);
    if (rc < 0) return(NULL);

	rc = WaveCodecIO::BlockReadRegion(
		bmin, bmax, blocks, false
	);
    if (rc < 0) return(NULL);

	WaveCodecIO::CloseVariable();

	return(MakeGrid(ts,varname,reflevel,lod,bmin,bmax,blocks));
}
