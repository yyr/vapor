#include <limits>
#include <cassert>
#include <vapor/DataMgrWRF.h>

using namespace VetsUtil;
using namespace VAPoR;

DataMgrWRF::DataMgrWRF(
	const vector <string> &files,
    size_t mem_size
) : DataMgr(mem_size), WRFReader(files) 
{
}

DataMgrWRF::DataMgrWRF(
    const MetadataWRF &metadata,
    size_t mem_size
) : DataMgr(mem_size), WRFReader(metadata) 
{
}

