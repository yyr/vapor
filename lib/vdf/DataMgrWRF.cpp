

#include <vapor/DataMgrWRF.h>

using namespace VetsUtil;
using namespace VAPoR;


DataMgrWRF::DataMgrWRF(
	const vector <string> &files,
    size_t mem_size
) : LayeredIO(mem_size), WRFReader(files) 
{
	size_t dim[3];
    GetDimNative(dim, -1);
    SetGridHeight(dim[2]);
}

DataMgrWRF::DataMgrWRF(
    const MetadataWRF &metadata,
    size_t mem_size
) : LayeredIO(mem_size), WRFReader(metadata) 
{
	size_t dim[3];
    GetDimNative(dim, -1);
    SetGridHeight(dim[2]);
}

void DataMgrWRF::GetValidRegionNative(
    size_t min[3], size_t max[3], int reflevel
) const {
	size_t dim[3];

	GetDimNative(dim, -1);
	for (int i=0; i<3; i++) {
		min[i] = 0;
		max[i] = dim[i] - 1;
	}
}
