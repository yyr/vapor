#include <vapor/DataMgrLayered2.h>

using namespace VetsUtil;
using namespace VAPoR;


 VAPoR::DataMgrLayered2::DataMgrLayered2(
    const string &metafile,
    size_t mem_size
 ) : LayeredIO(mem_size), WaveCodecIO(metafile) 
{
	size_t dim[3];
	GetDimNative(dim, -1);
	SetGridHeight(dim[2]);
}

 VAPoR::DataMgrLayered2::DataMgrLayered2(
    const MetadataVDC &metadata,
    size_t mem_size
 ) : LayeredIO(mem_size), WaveCodecIO(metadata) 
{
	size_t dim[3];
	GetDimNative(dim, -1);
	SetGridHeight(dim[2]);
}

