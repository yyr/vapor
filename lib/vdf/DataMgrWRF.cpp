#include <limits>
#include <cassert>
#include <vapor/DataMgrWRF.h>

using namespace VetsUtil;
using namespace VAPoR;

DataMgrWRF::DataMgrWRF(
	const vector <string> &files,
    size_t mem_size
) : DataMgr(mem_size), DCReaderWRF(files) 
{
}
