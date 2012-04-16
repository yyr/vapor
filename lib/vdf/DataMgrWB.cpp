

#include <vapor/DataMgrWB.h>

using namespace VetsUtil;
using namespace VAPoR;

VAPoR::DataMgrWB::DataMgrWB(
    const string &metafile,
    size_t mem_size
 ) : DataMgr(mem_size), WaveletBlock3DRegionReader(metafile) 
{ }

VAPoR::DataMgrWB::DataMgrWB(
    const MetadataVDC &metadata,
    size_t mem_size
) : DataMgr(mem_size), WaveletBlock3DRegionReader(metadata) 
{ }
