#include <limits>
#include <cassert>
#include <vapor/DataMgrROMS.h>

using namespace VetsUtil;
using namespace VAPoR;

DataMgrROMS::DataMgrROMS(
	const vector <string> &files,
    size_t mem_size
) : DataMgr(mem_size), DCReaderROMS(files) 
{
}
