#include <vector>
#include <vapor/VDC.h>
#include <vapor/MyBase.h>
#include <vapor/DataMgrVDC.h>

using namespace std;
using namespace VAPoR;

DataMgrVDC::DataMgrVDC(
	size_t mem_size, int numthreads
) : DataMgrV3_0("vdc",mem_size), _vdc(numthreads)
{ } 

DataMgrVDC::~DataMgrVDC() {}


int DataMgrVDC::_Initialize(const vector <string> &files) {
	if (files.size() < 1) {
		SetErrMsg("Invalid initialization : not VDC master file");
		return(-1);
	}
	return(_vdc.Initialize(files, VDC::R));
}

int DataMgrVDC::_ReadVariableBlock (
	size_t ts, string varname, int level, int lod, 
	vector <size_t> min, vector <size_t> max, float *blocks
) {
	int rc = _vdc.OpenVariableRead(ts, varname, level, lod);
	if (rc<0) return(-1);

	rc = _vdc.ReadRegionBlock(min, max, blocks);
	if (rc<0) return(-1);

	rc = _vdc.CloseVariable();
	if (rc<0) return(-1);

	return(0);
}

int DataMgrVDC::_ReadVariable(
    string varname, int level, int lod, float *data
) {
	int rc = _vdc.GetVar(varname, level, lod, data);
	if (rc<0) return(-1);

	return(0);
}
