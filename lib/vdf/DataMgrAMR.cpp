

#include <cassert>
#include <vapor/DataMgrAMR.h>

using namespace VetsUtil;
using namespace VAPoR;
// Maximum number of allocable blocks
#define MAX_NBLOCKS 1024*1024 

int DataMgrAMR::_DataMgrAMR()
{
	AMRIO::GetBlockSize(_bs, -1);

	size_t bdim[3];
	AMRIO::GetDimBlk(bdim, -1);

	//
	// Native AMR block size can be too small for efficient memory mgt. So
	// we make sure the block size is at least 64^3 (N.B. of course this
	// doesn't change the size of the AMR blocks stored on disk, only
	// the access size allowed by the DataMgr)
	//
	for (int i=0; i<3; i++) {
		_bsshift[i] = 0;
		while (_bs[i] < 64) {
			_bs[i] *= 2;
			_bsshift[i] += 1;
		}
		bdim[i] = bdim[i] >> _bsshift[i];
	}

	_ts = -1;

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

int	DataMgrAMR::_OpenVariableRead(
    size_t timestep,
    const char *varname,
    int reflevel,
    int lod
) {

    if (reflevel < 0) reflevel = AMRIO::GetNumTransforms();

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
	AMRIO::GetDimBlk(bdim_native_base, 0);
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

	
	//
	// bs is the block size returned to the DataMgr, but this may be
	// larger than the actual AMR block size (bs_native) stored on disk
	//
	size_t bs[3];
	DataMgrAMR::_GetBlockSize(bs, reflevel);
	size_t block_size = bs[0]*bs[1]*bs[2];

	size_t bdim_native[3];
	AMRIO::GetDimBlk(bdim_native, reflevel);

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
 
int	DataMgrAMR::_BlockReadRegion(
    const size_t bmin[3], const size_t bmax[3],
    float *region
) {
	return(_ReadBlocks(&_amrtree, _reflevel, bmin, bmax, region));
}


