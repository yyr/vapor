#include <limits>
#include <cassert>
#include <vapor/DataMgrGRIB.h>

using namespace VetsUtil;
using namespace VAPoR;

DataMgrGRIB::DataMgrGRIB(
	const vector <string> &files,
    size_t mem_size
) : DataMgr(mem_size), DCReaderGRIB(files) 
{
}
