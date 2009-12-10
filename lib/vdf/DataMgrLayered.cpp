

#include <vapor/DataMgrLayered.h>

using namespace VetsUtil;
using namespace VAPoR;


 VAPoR::DataMgrLayered::DataMgrLayered(
    const string &metafile,
    size_t mem_size
 ) : DataMgr(mem_size), LayeredIO(metafile) 
{}

 VAPoR::DataMgrLayered::DataMgrLayered(
    const MetadataVDC &metadata,
    size_t mem_size
 ) : DataMgr(mem_size), LayeredIO(metadata) 
{}

