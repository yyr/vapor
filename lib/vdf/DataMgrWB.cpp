

#include <vapor/DataMgrWB.h>

using namespace VetsUtil;
using namespace VAPoR;

int VAPoR::DataMgrWB::_DataMgrWB(
) {
	size_t bdim[3];
	WaveletBlock3DRegionReader::GetDimBlk(bdim, -1);

	int nblocks = 1;
	for (int i=0; i<3; i++) {
		nblocks *= bdim[i];
	}
	_blkptrs = new float*[nblocks];
	return(0);
}

VAPoR::DataMgrWB::DataMgrWB(
    const string &metafile,
    size_t mem_size
 ) : DataMgr(mem_size), WaveletBlock3DRegionReader(metafile) 
{
	(void) _DataMgrWB();
}

VAPoR::DataMgrWB::DataMgrWB(
    const MetadataVDC &metadata,
    size_t mem_size
) : DataMgr(mem_size), WaveletBlock3DRegionReader(metadata) 
{
	(void) _DataMgrWB();
}

RegularGrid *DataMgrWB::MakeGrid(
	size_t ts, string varname, int reflevel, int lod,
    const size_t bmin[3], const size_t bmax[3], float *blocks
) {

	size_t bs[3];
	WaveletBlock3DRegionReader::GetBlockSize(bs,-1);

	size_t bdim[3];
	WaveletBlock3DRegionReader::GetDimBlk(bdim,reflevel);

	size_t dim[3];
	WaveletBlock3DRegionReader::GetDim(dim,reflevel);

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
    WaveletBlock3DRegionReader::MapVoxToUser(ts,min, extents, reflevel);
    WaveletBlock3DRegionReader::MapVoxToUser(ts,max, extents+3, reflevel);

	//
	// Determine which dimensions are periodic, if any. For a dimension to
	// be periodic the data set must be periodic, and the requested
	// blocks must be boundary blocks
	//
	const vector <long> &periodic_vec = WaveletBlock3DRegionReader::GetPeriodicBoundary();

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
	} else if (
		! (WaveletBlock3DRegionReader::GetGridType().compare("layered")==0) ||
		varname.compare("ELEVATION") == 0
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
//		return(new LayeredGrid(bs,min, max, extents, periodic, _blkptrs, coords,2));
cerr << "Hard code missing value\n";
		return(new LayeredGrid(bs,min, max, extents, periodic, _blkptrs, coords,2, -1e20));
	}
}

RegularGrid *DataMgrWB::ReadGrid(
	size_t ts, string varname, int reflevel, int lod,
    const size_t bmin[3], const size_t bmax[3], float *blocks
) {

    int rc = WaveletBlock3DRegionReader::OpenVariableRead(
		ts, varname.c_str(), reflevel, 0
	);
    if (rc < 0) return(NULL);

	rc = WaveletBlock3DRegionReader::BlockReadRegion(
		bmin, bmax, blocks, false
	);
    if (rc < 0) return(NULL);

	WaveletBlock3DRegionReader::CloseVariable();

	return(MakeGrid(ts,varname,reflevel,lod,bmin,bmax,blocks));
}
