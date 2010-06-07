

#include <vapor/DataMgrWC.h>

using namespace VetsUtil;
using namespace VAPoR;


 VAPoR::DataMgrWC::DataMgrWC(
    const string &metafile,
    size_t mem_size
 ) : DataMgr(mem_size), WaveCodecIO(metafile) 
{}

 VAPoR::DataMgrWC::DataMgrWC(
    const MetadataVDC &metadata,
    size_t mem_size
 ) : DataMgr(mem_size), WaveCodecIO(metadata) 
{}

