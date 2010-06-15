

#include <vapor/DataMgrAMR.h>

using namespace VetsUtil;
using namespace VAPoR;


 VAPoR::DataMgrAMR::DataMgrAMR(
    const string &metafile,
    size_t mem_size
 ) : DataMgr(mem_size), AMRIO(metafile) 
{
}

 VAPoR::DataMgrAMR::DataMgrAMR(
    const MetadataVDC &metadata,
    size_t mem_size
 ) : DataMgr(mem_size), AMRIO(metadata) 
{
}

int	DataMgrAMR::OpenVariableRead(
    size_t timestep,
    const char *varname,
    int reflevel,
    int lod
) {
	int rc = AMRIO::OpenVariableRead(timestep, varname, reflevel);
	if (rc < 0) return (-1);

	_reflevel = reflevel;

	//
	// Read in the AMR tree
	// N.B. Really only need to do this when the time step
	// changes - same tree used for all variables of a given
	// time step
	//
	rc = AMRIO::OpenTreeRead(timestep);
	if (rc < 0) return (-1);

	rc = AMRIO::TreeRead(&_amrtree);
	if (rc < 0) return (-1);

	(void) AMRIO::CloseTree();

	return(0);
}
 
int	DataMgrAMR::BlockReadRegion(
    const size_t bmin[3], const size_t bmax[3],
    float *region
) {

	//
	// Read in the AMR field data
	//
	const size_t *bs = GetBlockSize();
	size_t minbase[3];
	size_t maxbase[3];
	for (int i=0; i<3; i++) {
		minbase[i] = bmin[i] >> _reflevel;
		maxbase[i] = bmax[i] >> _reflevel;
	}
		
	AMRData amrdata(&_amrtree, bs, minbase, maxbase, _reflevel);
	if (AMRData::GetErrCode() != 0)  return(-1);

	int rc = AMRIO::VariableRead(&amrdata);
	if (rc < 0) return (-1);

	rc = amrdata.ReGrid(bmin,bmax, _reflevel, region);
	if (rc < 0) {
		string s = AMRData::GetErrMsg();
		SetErrMsg(
			"Failed to regrid region : %s", s.c_str()
		);
		return (-1);
	}
	return(0);
}
