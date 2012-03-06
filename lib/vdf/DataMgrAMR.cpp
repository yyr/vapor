

#include <cassert>
#include <vapor/DataMgrAMR.h>

using namespace VetsUtil;
using namespace VAPoR;
// Maximum number of allocable blocks
#define MAX_NBLOCKS 1024*1024 

void   DataMgrAMR::_GetDimBlk(size_t bdim[3], int reflevel) const {
    size_t dim[3];

    AMRIO::GetDim(dim, reflevel);

    size_t bs[3];
    AMRIO::GetBlockSize(bs, reflevel);
    for (int i=0; i<3; i++) {
        bdim[i] = (size_t) ceil ((double) dim[i] / (double) bs[i]);
    }
}

int DataMgrAMR::_DataMgrAMR()
{
	AMRIO::GetBlockSize(_bs, -1);

	size_t bdim[3];
	_GetDimBlk(bdim, -1);

	//
	// Native AMR block size can be too small for efficient memory mgt
	//
	for (int i=0; i<3; i++) {
		_bsshift[i] = 0;
		while (_bs[i] < 64) {
			_bs[i] *= 2;
			_bsshift[i] += 1;
		}
		bdim[i] = bdim[i] >> _bsshift[i];
	}

	_nblocks = 1;
	for (int i=0; i<3; i++) {
		_nblocks *= bdim[i];
	}
	if (_nblocks > MAX_NBLOCKS) _nblocks = MAX_NBLOCKS;
	_blkptrs = new float*[_nblocks];

	_ts = -1;
	_ts_current = (size_t) -1;


	return(0);
}

DataMgrAMR::DataMgrAMR(
    const string &metafile,
    size_t mem_size
 ) : DataMgr(mem_size), AMRIO(metafile) 
{
	(void) _DataMgrAMR();
}

DataMgrAMR::DataMgrAMR(
    const MetadataVDC &metadata,
    size_t mem_size
 ) : DataMgr(mem_size), AMRIO(metadata) 
{
	(void) _DataMgrAMR();
}

int	DataMgrAMR::OpenVariableRead(
    size_t timestep,
    const char *varname,
    int reflevel,
    int lod
) {

    if (reflevel < 0) reflevel = GetNumTransforms();

	int rc = AMRIO::OpenVariableRead(timestep, varname, reflevel);
	if (rc < 0) return (-1);

	_reflevel = reflevel;

	//
	// Read in the AMR tree
	// N.B. Really only need to do this when the time step
	// changes - same tree used for all variables of a given
	// time step
	//
	if (timestep != _ts) {
		rc = AMRIO::OpenTreeRead(timestep);
		if (rc < 0) return (-1);

		rc = AMRIO::TreeRead(&_amrtree);
		if (rc < 0) return (-1);

		(void) AMRIO::CloseTree();
		_ts = timestep;
	}

	return(0);
}

int	DataMgrAMR::_ReadBlocks(
	const AMRTree *amrtree, int reflevel,
    const size_t bmin[3], const size_t bmax[3],
    float *blocks
) {

	//
	// Read in the AMR field data
	//
	const size_t *bs_native = AMRIO::GetBlockSize();
	size_t bdim_native_base[3];
	_GetDimBlk(bdim_native_base, 0);
	size_t minbase[3];
	size_t maxbase[3];
	size_t bmin_native[3];
	size_t bmax_native[3];
	for (int i=0; i<3; i++) {
		bmin_native[i] = bmin[i] << _bsshift[i];
		bmax_native[i] = ((bmax[i]+1) << _bsshift[i]) - 1;
		minbase[i] = bmin_native[i] >> reflevel;
		maxbase[i] = bmax_native[i] >> reflevel;
		if (maxbase[i] >= bdim_native_base[i]) maxbase[i] = bdim_native_base[i]-1;
	}
		
	AMRData amrdata(amrtree, bs_native, minbase, maxbase, reflevel);
	if (AMRData::GetErrCode() != 0)  return(-1);

	int rc = AMRIO::VariableRead(&amrdata);
	if (rc < 0) return (-1);

	
	const size_t *bs = DataMgrAMR::GetBlockSize();
	size_t block_size = bs[0]*bs[1]*bs[2];

	size_t bdim_native[3];
	_GetDimBlk(bdim_native, reflevel);

	int index = 0;
	for (int k=0; k<(bmax[2]-bmin[2]+1); k++) {
	for (int j=0; j<(bmax[1]-bmin[1]+1); j++) {
	for (int i=0; i<(bmax[0]-bmin[0]+1); i++) {
		size_t mybmin[] = {
			bmin_native[0] + i*(1<<_bsshift[0]),
			bmin_native[1] + j*(1<<_bsshift[1]),
			bmin_native[2] + k*(1<<_bsshift[2]),
		};
		size_t mybmax[] = {
			mybmin[0] + (1<<_bsshift[0]) - 1,
			mybmin[1] + (1<<_bsshift[1]) - 1,
			mybmin[2] + (1<<_bsshift[2]) - 1,
		};

		if (mybmax[0] >= bdim_native[0]) mybmax[0] = bdim_native[0]-1;
		if (mybmax[1] >= bdim_native[1]) mybmax[1] = bdim_native[1]-1;
		if (mybmax[2] >= bdim_native[2]) mybmax[2] = bdim_native[2]-1;

		rc = amrdata.ReGrid(
			mybmin,mybmax, reflevel, blocks+(index*block_size), bs
		);
		if (rc < 0) {
			string s = AMRData::GetErrMsg();
			SetErrMsg(
				"Failed to regrid region : %s", s.c_str()
			);
			return (-1);
		}
		index++;
	}
	}
	}
	return(0);
}
 
int	DataMgrAMR::BlockReadRegion(
    const size_t bmin[3], const size_t bmax[3],
    float *region, bool unblock
) {
	assert (unblock == true);

	return(_ReadBlocks(&_amrtree, _reflevel, bmin, bmax, region));
}


RegularGrid *DataMgrAMR::MakeGrid(
	size_t ts, string varname, int reflevel, int lod,
    const size_t bmin[3], const size_t bmax[3], float *blocks
) {

	size_t bs[3];
	DataMgrAMR::GetBlockSize(bs,-1);

	size_t bdim[3];
	AMRIO::GetDimBlk(bdim,reflevel);

	size_t dim[3];
	AMRIO::GetDim(dim,reflevel);

	size_t nblocks = 1;
	size_t block_size = 1;
	size_t min[3], max[3];
	for (int i=0; i<3; i++) {
		nblocks *= bmax[i]-bmin[i]+1;
		block_size *= bs[i];
		min[i] = bmin[i]*bs[i];
		max[i] = bmax[i]*bs[i] + bs[i] - 1;
		if (max[i] >= dim[i]) max[i] = dim[i]-1;
	}
	if (nblocks > _nblocks) {
		SetErrMsg("Requested region too large");
		return(NULL);
	}
	for (int i=0; i<nblocks; i++) {
		_blkptrs[i] = blocks + i*block_size;
	}

	double extents[6];
    AMRIO::MapVoxToUser(ts,min, extents, reflevel);
    AMRIO::MapVoxToUser(ts,max, extents+3, reflevel);

	//
	// Determine which dimensions are periodic, if any. For a dimension to
	// be periodic the data set must be periodic, and the requested
	// blocks must be boundary blocks
	//
	const vector <long> &periodic_vec = AMRIO::GetPeriodicBoundary();

	bool periodic[3];
	for (int i=0; i<3; i++) {
		if (periodic_vec[i] && bmin[i]==0 && bmax[i]==bdim[i]-1) {
			periodic[i] = true;
		}
		else {
			periodic[i] = false;
		}
	}

	return(new RegularGrid(bs,min,max,extents,periodic,_blkptrs));
}

RegularGrid *DataMgrAMR::ReadGrid(
	size_t ts, string varname, int reflevel, int lod,
    const size_t bmin[3], const size_t bmax[3], float *blocks
) {

    if (reflevel < 0) reflevel = GetNumTransforms();

	int rc = AMRIO::OpenVariableRead(ts, varname.c_str(), reflevel,0);
	if (rc < 0) return (NULL);

	//
	// Read in the AMR tree
	// N.B. Really only need to do this when the time step
	// changes - same tree used for all variables of a given
	// time step
	//
	if (ts != _ts_current) {
		rc = AMRIO::OpenTreeRead(ts);
		if (rc < 0) return (NULL);

		rc = AMRIO::TreeRead(&_amrtree_current);
		if (rc < 0) return (NULL);

		(void) AMRIO::CloseTree();
		_ts_current = ts;
	}

	rc =  _ReadBlocks(&_amrtree_current, reflevel, bmin, bmax, blocks);
	if (rc<0) return(NULL);

	AMRIO::CloseVariable();

	return(MakeGrid(ts,varname,reflevel,lod,bmin,bmax,blocks));
}
