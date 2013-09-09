#include <limits>
#include <cassert>
#include <vapor/DataMgrMOM.h>

using namespace VetsUtil;
using namespace VAPoR;

DataMgrMOM::DataMgrMOM(
	const vector <string> &files,
    size_t mem_size
) : DataMgr(mem_size), DCReaderMOM(files) 
{
}
