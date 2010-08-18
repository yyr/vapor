#include <vapor/DataMgrLayered.h>

using namespace VetsUtil;
using namespace VAPoR;


 VAPoR::DataMgrLayered::DataMgrLayered(
    const string &metafile,
    size_t mem_size
 ) : LayeredIO(mem_size), WaveletBlock3DRegionReader(metafile) 
{
	size_t dim[3];
	GetDimNative(dim, -1);
	SetGridHeight(dim[2]);
}

 VAPoR::DataMgrLayered::DataMgrLayered(
    const MetadataVDC &metadata,
    size_t mem_size
 ) : LayeredIO(mem_size), WaveletBlock3DRegionReader(metadata) 
{
	size_t dim[3];
	GetDimNative(dim, -1);
	SetGridHeight(dim[2]);
}

