#include <cstdio>
#include <cstring>
#include <cassert>
#include <cfloat>
#include <vector>
#include <map>
#include <vapor/DataMgr.h>
#include <vapor/common.h>
#include <vapor/errorcodes.h>
#include <vapor/Metadata.h>
#ifdef WIN32
#include <float.h>
#endif
using namespace VetsUtil;
using namespace VAPoR;


int	DataMgr::_DataMgr(
	size_t mem_size
) {
	SetClassName("DataMgr");

	_blk_mem_mgr = NULL;

	_PipeLines.clear();

	_quantizationRangeMap.clear();
	_regionsList.clear();
	_dataRangeMinMap.clear();
	_dataRangeMaxMap.clear();
	_validRegMinMaxMap.clear();
	
	_mem_size = mem_size;

	_timestamp = 0;
	
	return(0);
}

DataMgr::DataMgr(
	size_t mem_size
) {

	SetDiagMsg("DataMgr::DataMgr(,%d)", mem_size);

	if (_DataMgr(mem_size) < 0) return;


}


DataMgr::~DataMgr(
) {
	SetDiagMsg("DataMgr::~DataMgr()");

	_timestamp = 0;

	Clear();
	if (_blk_mem_mgr) delete _blk_mem_mgr;

	_dataRangeMinMap.clear();
	_dataRangeMaxMap.clear();

	_validRegMinMaxMap.clear();

	
	map <string, float * >::iterator p;
	for(p = _quantizationRangeMap.begin(); p!=_quantizationRangeMap.end(); p++){
		if (p->second) delete [] p->second;
	}
	_quantizationRangeMap.clear();

	_blk_mem_mgr = NULL;

}



float	*DataMgr::GetRegion(
	size_t ts,
	const char *varname,
	int reflevel,
	int lod,
	const size_t min[3],
	const size_t max[3],
	int	lock
) {

	float	*blks = NULL;
	int	rc;

	SetDiagMsg(
		"DataMgr::GetRegion(%d,%s,%d,%d,[%d,%d,%d],[%d,%d,%d],%d)",
		ts,varname,reflevel,lod,min[0],min[1],min[2],max[0],max[1],max[2], lock
	);

	// See if region is already in cache. If so, return it.
	//
	blks = (float *) get_region_from_cache(
		ts, varname, reflevel, lod, DataMgr::FLOAT32, min, max, lock
	);
	if (blks) {
		SetDiagMsg("DataMgr::GetRegion() - data in cache %xll\n", blks);
		return(blks);
	}

	//
	// See if the variable is derived from another variable 
	//
	if (IsVariableDerived(varname)) {
		return (execute_pipeline(
			ts, string(varname), reflevel, lod, min, max, lock
		));
	}

	// Else, read it from disk
	//
	VarType_T vtype = GetVarType(varname);

	rc = OpenVariableRead(ts, varname, reflevel, lod);
	if (rc < 0) {
		SetErrMsg(
			"Failed to read variable/timestep/level/lod (%s, %d, %d, %d)",
			varname, ts, reflevel, lod
		);
		return(NULL);
	}

	blks = (float *) alloc_region(
		ts,varname,vtype, reflevel, lod, DataMgr::FLOAT32,min,max,lock,false
	);
	if (! blks) return(NULL);

	rc = BlockReadRegion(min, max, blks);
	if (rc < 0) {
		SetErrMsg(
			"Failed to read region from variable/timestep/level/lod (%s, %d, %d, %d)",
			varname, ts, reflevel, lod
		);
		free_region(ts,varname,reflevel,lod,FLOAT32,min,max);
		CloseVariable();
		return (NULL);
	}

	CloseVariable();
	SetDiagMsg("DataMgr::GetRegion() - data not in cache %xll\n", blks);
	
	//
	// Make sure we have a valid floating point value
	//

	size_t bs[3];
	GetBlockSize(bs, reflevel);

	size_t size;
	switch (vtype) {
	case VAR2D_XY:
		size = ((max[0]-min[0]+1)*bs[0]) * ((max[1]-min[1]+1)*bs[1]);
		break;
	case VAR2D_XZ:
		size = ((max[0]-min[0]+1)*bs[0]) * ((max[1]-min[1]+1)*bs[2]);
		break;
	case VAR2D_YZ:
		size = ((max[0]-min[0]+1)*bs[1]) * ((max[1]-min[1]+1)*bs[2]);
		break;
	case VAR3D:
		size = ((max[0]-min[0]+1)*bs[0]) * ((max[1]-min[1]+1)*bs[1]) *
			((max[2]-min[2]+1)*bs[2]);
		break;
	default: 
		size = 0;
	}
	for (size_t i=0; i<size; i++) {
#ifdef WIN32
		if (! _finite(blks[i]) || _isnan(blks[i])) blks[i] = FLT_MAX;
#else
		if (! finite(blks[i]) || isnan(blks[i])) blks[i] = FLT_MAX;
#endif
	}

	return(blks);
}


unsigned char	*DataMgr::GetRegionUInt8(
	size_t ts,
	const char *varname,
	int reflevel,
	int lod,
	const size_t min[3],
	const size_t max[3],
	const float range[2],
	int lock
) {
	SetDiagMsg(
		"DataMgr::GetRegionUInt8(%d,%s,%d,%d,[%d,%d,%d],[%d,%d,%d],[%f,%f],%d)",
		ts,varname,reflevel,lod,min[0],min[1],min[2],max[0],max[1],max[2],
		range[0], range[1], lock
	);

	return(get_quantized_region(
		ts, varname, reflevel, lod, min, max, range, 
		lock, DataMgr::UINT8
	));
}

unsigned char	*DataMgr::GetRegionUInt8(
	size_t ts,
	const char *varname1,
	const char *varname2,
	int reflevel,
	int lod,
	const size_t min[3],
	const size_t max[3],
	const float range1[2],
	const float range2[2],
	int lock
) {
	SetDiagMsg(
		"DataMgr::GetRegionUInt8(%d,%s,%s,%d,%d,[%d,%d,%d],[%d,%d,%d],[%f,%f],[%f,%f],%d)",
		ts,varname1, varname2, reflevel,lod, min[0],min[1],min[2],
		max[0],max[1],max[2], range1[0], range1[1], 
		range2[0], range2[1], lock
	);
	VarType_T vtype1 = GetVarType(varname1);
	VarType_T vtype2 = GetVarType(varname2);
	if (vtype1 != vtype2) {
		SetErrMsg("Variable type mismatch");
		return(NULL);
	}

	string varname = varname1;
	varname += "+";
	varname += varname2;
	unsigned char *ublks = NULL;

	// 
	// Verify that the quantization range hasn't changed for
	// either variable. If it hasn't, attempt to get the
	// interleaved field array from the cache.
	//
	if ((set_quantization_range(varname1, range1) == 0) && 
		(set_quantization_range(varname2, range2) == 0)) {

		ublks = (unsigned char *) get_region_from_cache(
			ts, varname.c_str(), reflevel, lod, DataMgr::UINT16, 
			min, max, lock
		);
	}
	else {
		// set_quantization_range() will free the variable if 
		// the range is dirty, but we have to explicity free the two-field
		// 'varname', which has two sets of data ranges.
		//  
		free_var(varname, 0);
	}

	if (ublks) return(ublks);

	// Interleaved array is not in cache so we'll need to
	// construct it
	//
	size_t bs[3];
	GetBlockSize(bs, reflevel);

	int	nz = (int)((max[2]-min[2]+1) * bs[2]);
	int	ny = (int)((max[1]-min[1]+1) * bs[1]);
	int	nx = (int)((max[0]-min[0]+1) * bs[0]);
	switch (vtype1) {
	case VAR2D_XY:
		nz = 1;
		break;
	case VAR2D_XZ:
		ny = 1;
		break;
	case VAR2D_YZ:
		nx = 1;
		break;
	default:
		break;
	}
	size_t size = nx*ny*nz;

	unsigned char *ublks1, *ublks2;

	ublks = (unsigned char *) alloc_region(
		ts,varname.c_str(),vtype1, reflevel,lod, DataMgr::UINT16,
		min,max, lock,false
	);
	if (! ublks) return(NULL);

	ublks1 = get_quantized_region(
		ts, varname1, reflevel, lod, min, max, range1, 
		0, DataMgr::UINT8
	);
	if (! ublks1) return (NULL);

	for (size_t i = 0; i<size; i++) {
		ublks[2*i] = ublks1[i];
	}

	ublks2 = get_quantized_region(
		ts, varname2, reflevel, lod, min, max, range2, 
		0, DataMgr::UINT8
	);
	if (! ublks2) return (NULL);

	for (size_t i = 0; i<size; i++) {
		ublks[2*i+1] = ublks2[i];
	}
	return(ublks);
}

unsigned char	*DataMgr::GetRegionUInt16(
	size_t ts,
	const char *varname,
	int reflevel,
	int lod,
	const size_t min[3],
	const size_t max[3],
	const float range[2],
	int lock
) {
	SetDiagMsg(
		"DataMgr::GetRegionUInt16(%d,%s,%d,%d,[%d,%d,%d],[%d,%d,%d],[%f,%f],%d)",
		ts,varname,reflevel,lod, min[0],min[1],min[2],max[0],max[1],max[2],
		range[0], range[1], lock
	);

	return(get_quantized_region(
		ts, varname, reflevel, lod, min, max, range, 
		lock, DataMgr::UINT16
	));
}

unsigned char	*DataMgr::GetRegionUInt16(
	size_t ts,
	const char *varname1,
	const char *varname2,
	int reflevel,
	int lod,
	const size_t min[3],
	const size_t max[3],
	const float range1[2],
	const float range2[2],
	int lock
) {
	SetDiagMsg(
		"DataMgr::GetRegionUInt16(%d,%s,%s,%d,%d,[%d,%d,%d],[%d,%d,%d],[%f,%f],[%f,%f],%d)",
		ts,varname1, varname2, reflevel,lod, min[0],min[1],min[2],
		max[0],max[1],max[2], range1[0], range1[1], 
		range2[0], range2[1], lock
	);

	VarType_T vtype1 = GetVarType(varname1);
	VarType_T vtype2 = GetVarType(varname2);
	if (vtype1 != vtype2) {
		SetErrMsg("Variable type mismatch");
		return(NULL);
	}

	string varname = varname1;
	varname += "+";
	varname += varname2;
	unsigned char *ublks = NULL;

	// 
	// Verify that the quantization range hasn't changed for
	// either variable. If it hasn't, attempt to get the
	// interleaved field array from the cache.
	//
	if ((set_quantization_range(varname1, range1) == 0) && 
		(set_quantization_range(varname2, range2) == 0)) {

		ublks = (unsigned char *) get_region_from_cache(
			ts, varname.c_str(), reflevel, lod, DataMgr::UINT32, 
			min, max, lock
		);
	}
	else {
		// set_quantization_range() will free the variable if 
		// the range is dirty, but we have to explicity free the two-field
		// 'varname', which has two sets of data ranges.
		//  
		free_var(varname, 0);
	}

	if (ublks) return(ublks);

	// Interleaved array is not in cache so we'll need to
	// construct it
	//
	size_t bs[3];
	GetBlockSize(bs, reflevel);

	int	nz = (int)((max[2]-min[2]+1) * bs[2]);
	int	ny = (int)((max[1]-min[1]+1) * bs[1]);
	int	nx = (int)((max[0]-min[0]+1) * bs[0]);
	switch (vtype1) {
	case VAR2D_XY:
		nz = 1;
		break;
	case VAR2D_XZ:
		ny = 1;
		break;
	case VAR2D_YZ:
		nx = 1;
		break;
	default:
		break;
	}
	size_t size = nx*ny*nz;

	unsigned char *ublks1, *ublks2;

	ublks = (unsigned char *) alloc_region(
		ts,varname.c_str(),vtype1, reflevel,lod, DataMgr::UINT32,
		min,max, lock,false
	);
	if (! ublks) return(NULL);

	ublks1 = get_quantized_region(
		ts, varname1, reflevel, lod, min, max, range1, 
		0, DataMgr::UINT16
	);
	if (! ublks1) return (NULL);

	for (size_t i = 0; i<size; i++) {
		ublks[2*2*i+0] = ublks1[2*i+0];
		ublks[2*2*i+1] = ublks1[2*i+1];
	}

	ublks2 = get_quantized_region(
		ts, varname2, reflevel, lod, min, max, range2, 
		0, DataMgr::UINT16
	);
	if (! ublks2) return (NULL);

	for (size_t i = 0; i<size; i++) {
		ublks[2*2*i+2] = ublks2[2*i+0];
		ublks[2*2*i+3] = ublks2[2*i+1];
	}
	return(ublks);
}

int	DataMgr::NewPipeline(PipeLine *pipeline) {

	//
	// Delete any pipeline stage with the same name as the new one. This
	// is a no-op if the stage doesn't exist.
	//
	RemovePipeline(pipeline->GetName());

	// 
	// Make sure outputs don't collide with existing outputs
	//
	const vector <pair <string, VarType_T> > &my_outputs = pipeline->GetOutputs();

	for (int i=0; i<_PipeLines.size(); i++) {
		const vector <pair <string,VarType_T> > &outputs = _PipeLines[i]->GetOutputs();
		for (int j=0; j<my_outputs.size(); j++) {
		for (int k=0; k<outputs.size(); k++) {
			if (my_outputs[j].first.compare(outputs[k].first)==0) {
				SetErrMsg(
					"Pipeline output %s already in use", 
					my_outputs[j].first.c_str()
				);
				return(-1);
			}
		}
		}
	}

	//
	// Now make sure outputs don't match any native variables
	//
	vector <string> native_vars = get_native_variables();

	for (int i=0; i<native_vars.size(); i++) {
		for (int j=0; j<my_outputs.size(); j++) {
			if (native_vars[i].compare(my_outputs[j].first) == 0) {
				SetErrMsg(
					"Pipeline output %s matches native variable name", 
					my_outputs[i].first.c_str()
				);
				return(-1);
			}
		}
	}
		

	//
	// Add the new stage to a temporary pipeline. Generate a hash
	// table with all the dependencies of the temporary pipeline. And
	// then check for cycles in the graph (e.g. a -> b -> c -> a).
	//
	map <string, vector <string> > graph;
	vector <PipeLine *> tmp_pipe = _PipeLines;
	tmp_pipe.push_back(pipeline);

	for (int i=0; i<tmp_pipe.size(); i++) {
		vector <string> depends;
		for (int j=0; j<tmp_pipe.size(); j++) {
	
			// 
			// See if inputs to tmp_pipe[i] match outputs 
			// of tmp_pipe[j]
			//
			if (depends_on(tmp_pipe[i], tmp_pipe[j])) {
				depends.push_back(tmp_pipe[j]->GetName());
			}
		}
		graph[tmp_pipe[i]->GetName()] = depends;
	}

	//
	// Finally check for cycles in the graph
	//
	if (cycle_check(graph, pipeline->GetName(), graph[pipeline->GetName()])) {
		SetErrMsg("Invalid pipeline : circular dependency detected");
		return(-1);
	}

	_PipeLines.push_back(pipeline);
	return(0);
}

void	DataMgr::RemovePipeline(string name) {

	vector <PipeLine *>::iterator itr;
	for (itr = _PipeLines.begin(); itr != _PipeLines.end(); itr++) {
		if (name.compare((*itr)->GetName()) == 0) {
			_PipeLines.erase(itr);
			break;
		}
	}
}

int DataMgr::VariableExists(
    size_t ts, const char *varname, int reflevel, int lod
) const {

	if (IsVariableNative(varname)) {
		return(_VariableExists( ts, varname, reflevel, lod));
	}
	else {
		PipeLine *pipeline = get_pipeline_for_var(varname);
		assert(pipeline != NULL);

		const vector <pair <string, VarType_T> > &ovars = pipeline->GetOutputs();
		//
		// Recursively test existence of all dependencies
		//
		for (int i=0; i<ovars.size(); i++) {
			if (! VariableExists(ts, ovars[i].first.c_str(), reflevel, lod)) {
				return(0);
			}
		}
	}
	return(1);
}


bool DataMgr::IsVariableNative(string name) const {
	vector <string> svec = get_native_variables();

	for (int i=0; i<svec.size(); i++) {
		if (name.compare(svec[i]) == 0) return (true);
	}
	return(false);
}

bool DataMgr::IsVariableDerived(string name) const {
	vector <string> svec = get_derived_variables();

	for (int i=0; i<svec.size(); i++) {
		if (name.compare(svec[i]) == 0) return (true);
	}
	return(false);
}

vector <string> DataMgr::GetVariables3D() const {

	// Get native variables (variables contained in the data set)
	//
	vector <string> svec = _GetVariables3D();

	// Now get derived variables 
	//
	for (int i=0; i<_PipeLines.size(); i++) {
		const vector <pair <string,VarType_T> > &outputs = _PipeLines[i]->GetOutputs();
		for (int j=0; j<outputs.size(); j++) {
			if (outputs[j].second == VAR3D) svec.push_back(outputs[j].first);
		}
	}
	return(svec);
}

vector <string> DataMgr::GetVariables2DXY() const {

	// Get native variables (variables contained in the data set)
	//
	vector <string> svec = _GetVariables2DXY();

	// Now get derived variables 
	//
	for (int i=0; i<_PipeLines.size(); i++) {
		const vector <pair <string,VarType_T> > &outputs = _PipeLines[i]->GetOutputs();
		for (int j=0; j<outputs.size(); j++) {
			if (outputs[j].second == VAR2D_XY) svec.push_back(outputs[j].first);
		}
	}
	return(svec);
}
vector <string> DataMgr::GetVariables2DXZ() const {

	// Get native variables (variables contained in the data set)
	//
	vector <string> svec = _GetVariables2DXZ();

	// Now get derived variables 
	//
	for (int i=0; i<_PipeLines.size(); i++) {
		const vector <pair <string,VarType_T> > &outputs = _PipeLines[i]->GetOutputs();
		for (int j=0; j<outputs.size(); j++) {
			if (outputs[j].second == VAR2D_XZ) svec.push_back(outputs[j].first);
		}
	}
	return(svec);
}

vector <string> DataMgr::GetVariables2DYZ() const {

	// Get native variables (variables contained in the data set)
	//
	vector <string> svec = _GetVariables2DYZ();

	// Now get derived variables 
	//
	for (int i=0; i<_PipeLines.size(); i++) {
		const vector <pair <string,VarType_T> > &outputs = _PipeLines[i]->GetOutputs();
		for (int j=0; j<outputs.size(); j++) {
			if (outputs[j].second == VAR2D_YZ) svec.push_back(outputs[j].first);
		}
	}
	return(svec);
}

	
unsigned char	*DataMgr::get_quantized_region(
	size_t ts,
	const char *varname,
	int reflevel,
	int lod,
	const size_t min[3],
	const size_t max[3],
	const float range[2],
	int lock,
	_dataTypes_t type
) {

	unsigned char	*ublks = NULL;

	// 
	// Set the data range for the quantization mapping. This operation 
	// is a no-op if the value specified does not differ from the
	// current mapping for this variable.
	//
	(void) set_quantization_range(varname, range);

	ublks = (unsigned char *) get_region_from_cache(
		ts, varname, reflevel, lod, type, min, max, lock
	);

	if (ublks) {
		SetDiagMsg(
			"DataMgr::get_quantized_region() - data in cache %xll\n", ublks
		);
		return(ublks);
	}
    
	float	*blks = NULL;

	blks = GetRegion(ts, varname, reflevel, lod, min, max, 1);
	if (! blks) return (NULL);


	VarType_T vtype = GetVarType(varname);
	ublks = (unsigned char *) alloc_region(
		ts,varname,vtype, reflevel,lod, type,min,max, lock, false
	);
	if (! ublks) {
		UnlockRegion(blks);
		return(NULL);
	}

	// Quantize the floating point data;

	size_t bs[3];
	GetBlockSize(bs, reflevel);

	int	nz = (int)((max[2]-min[2]+1) * bs[2]);
	int	ny = (int)((max[1]-min[1]+1) * bs[1]);
	int	nx = (int)((max[0]-min[0]+1) * bs[0]);

	switch (vtype) {
	case VAR2D_XY:
		nz = 1;
		break;
	case VAR2D_XZ:
		ny = 1;
		break;
	case VAR2D_YZ:
		nx = 1;
		break;
	default:
		break;
	}

	if (type == DataMgr::UINT8) {
		quantize_region_uint8(blks, ublks, nx*ny*nz, range);
	}
	else {
		quantize_region_uint16(blks, ublks, nx*ny*nz, range);
	}

	// Unlock the floating point data
	//
	UnlockRegion(blks);
	SetDiagMsg(
		"DataMgr::get_quantized_region () - data not in cache %xll\n,", ublks
	);

	return(ublks);
}

void	DataMgr::quantize_region_uint8(
	const float *fptr,
	unsigned char *ucptr,
	size_t size,
	const float range[2]
) {
	for (size_t i = 0; i<size; i++) {
		unsigned int	v;
		if (*fptr < range[0]) *ucptr = 0;
		else if (*fptr > range[1]) *ucptr = 255;
		else {
			v = (int) rint((*fptr - range[0]) / (range[1] - range[0]) * 255);
			*ucptr = (unsigned char) v;
		}
		ucptr++;
		fptr++;
	}
}

void	DataMgr::quantize_region_uint16(
	const float *fptr,
	unsigned char *ucptr,
	size_t size,
	const float range[2]
) {
	for (size_t i = 0; i<size; i++) {
		unsigned int	v;
		if (*fptr < range[0]) {
			v = 0;
		}
		else if (*fptr > range[1]) {
			v = 65535;
		}
		else {
			v = (int) rint((*fptr - range[0]) / (range[1] - range[0]) * 65535);
		}
		ucptr[0] = (unsigned char) (v & 0xff);
		ucptr[1] = (unsigned char) ((v >> 8) & 0xff);

		ucptr+=2;
		fptr++;
	}
}

int DataMgr::GetDataRange(
	size_t ts,
	const char *varname,
	float *range,
	int reflevel,
	int lod
) {
	int	rc;

	SetDiagMsg("DataMgr::GetDataRange(%d,%s)", ts, varname);


	// See if we've already cache'd it.
	//
	if (get_cached_data_range(ts, varname, range) == 0) return(0);

	// Range isn't cache'd. Need to get it from derived class
	//

	if (IsVariableNative(varname)) {
		rc = OpenVariableRead(ts, varname, reflevel,lod);
		if (rc < 0) {
			SetErrMsg(
				"Failed to read variable/timestep/level/lod (%s, %d, %d, %d)",
				varname, ts, 0,0
			);
			return(-1);
		}

		const float *r = GetDataRange();
		if (r) {
			range[0] = r[0];
			range[1] = r[1];

#ifdef WIN32
			if (! _finite(range[0]) || _isnan(range[0])) range[0] = FLT_MIN;
#else
			if (! finite(range[0]) || isnan(range[0])) range[0] = FLT_MIN;
#endif
#ifdef WIN32
			if (! _finite(range[1]) || _isnan(range[1])) range[1] = FLT_MIN;
#else
			if (! finite(range[1]) || isnan(range[1])) range[1] = FLT_MIN;
#endif

			// Use of []'s creates an entry in map
			_dataRangeMinMap[ts][varname] = range[0];
			_dataRangeMaxMap[ts][varname] = range[1];

			CloseVariable();
			return(0);
		}
		CloseVariable();
	}

	//
	// Argh. Child class doesn't know data range (or this is a
	// derived variable). Have to caculate range
	// ourselves
	//
	size_t min[3], max[3];
	size_t bmin[3], bmax[3];
	rc = GetValidRegion(ts, varname, reflevel, min, max);
	if (rc<0) return(-1);
	MapVoxToBlk(min, bmin, -1);
	MapVoxToBlk(max, bmax, -1);

	
	// Get data volume 
	//
	float *blks = GetRegion(ts, varname, reflevel, lod,bmin, bmax, 0); 
	if (! blks) return(-1);

	VarType_T vtype = GetVarType(varname);

	size_t bs[3];
	GetBlockSize(bs, reflevel);

	size_t size;
	switch (vtype) {
	case VAR2D_XY:
		size = ((bmax[0]-bmin[0]+1)*bs[0]) * ((bmax[1]-bmin[1]+1)*bs[1]);
		break;
	case VAR2D_XZ:
		size = ((bmax[0]-bmin[0]+1)*bs[0]) * ((bmax[1]-bmin[1]+1)*bs[2]);
		break;
	case VAR2D_YZ:
		size = ((bmax[0]-bmin[0]+1)*bs[1]) * ((bmax[1]-bmin[1]+1)*bs[2]);
		break;
	case VAR3D:
		size = ((bmax[0]-bmin[0]+1)*bs[0]) * ((bmax[1]-bmin[1]+1)*bs[1]) *
			((bmax[2]-bmin[2]+1)*bs[2]);
		break;
	default: 
		size = 0;
	}

	//
	// Finally calculate the range. Note this code probably should crop
	// the extents to the valid extents range returned by GetValidExtents()
	//
	range[0] = blks[0];
	range[1] = blks[0];
	for (size_t i=0; i<size; i++) {
		if (blks[i] < range[0]) range[0] = blks[i];
		if (blks[i] > range[1]) range[1] = blks[i];
	}
	// Use of []'s creates an entry in map
	_dataRangeMinMap[ts][varname] = range[0];
	_dataRangeMaxMap[ts][varname] = range[1];

	return(0);
}
	
int DataMgr::GetValidRegion(
	size_t ts,
	const char *varname,
	int reflevel,
	size_t min[3],
	size_t max[3]
) {
	int	rc;

	SetDiagMsg("DataMgr::GetValidRegion(%d,%s,%d)",ts,varname,reflevel);
	for (int i=0; i<3; i++) {
		min[i] = max[i] = 0;
	}

	// See if we've already cache'd it.
	//
	vector <size_t> minmax = get_cached_reg_min_max(ts, varname, reflevel);
	if (minmax.size()) {
		for (int i=0; i<3; i++) {
			min[i] = minmax[i];
			max[i] = minmax[i+3];
		}
		return(0);
	}

	if (IsVariableNative(varname)) {

		// Range isn't cache'd. Need to read it from the file
		//
		rc = OpenVariableRead(ts, varname, reflevel, 0);
		if (rc < 0) {
			SetErrMsg(
				"Failed to read variable/timestep/level/lod (%s, %d, %d %d)",
				varname, ts, reflevel, 0
			);
			return(-1);
		}

		GetValidRegion(min, max, reflevel);

		CloseVariable();
	}
	else if (IsVariableDerived(varname)) {

		//
		// Initialize min1 and max1 to maximum extents
		//
		size_t dim[3], min1[3], max1[3];
		GetDim(dim, reflevel);
		for (int i=0; i<3; i++) {
			min1[i] = 0;
			max1[i] = dim[i]-1;
		}

		//
		// Get the pipline stage for computing this variable
		//
		PipeLine *pipeline = get_pipeline_for_var(varname);
		assert(pipeline != NULL);

		const vector  <string> ivars = pipeline->GetInputs();

		//
		// Recursively compute the intersection of valid regions for 
		// all variables that a dependencies of this variable
		//
		for (int i=0; i<ivars.size(); i++) {
			size_t min2[3], max2[3];
			
			int rc = GetValidRegion(
				ts, ivars[i].c_str(), reflevel, min2, max2
			);
			if (rc<0) return(-1);

			for (int j=0; j<3; j++) {
				if (min2[j] > min1[j]) min1[j] = min2[j];
				if (max2[j] < max1[j]) max1[j] = max2[j];
			}
		}
		for (int i=0; i<3; i++) {
			min[i] = min1[i];
			max[i] = max1[i];
		}
	}
	else {
		SetErrMsg("Invalid variable : %s", varname);
		return(-1);
	}

	//
	// Cache results
	//
	for (int i=0; i<3; i++) minmax.push_back(min[i]);
	for (int i=0; i<3; i++) minmax.push_back(max[i]);

	// Use of []'s creates an entry in map
	_validRegMinMaxMap[ts][varname][reflevel] = minmax;
		
	return(0);
}

	
int	DataMgr::UnlockRegion(
	const void *blks
) {
	SetDiagMsg("DataMgr::UnlockRegion()");

	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); itr++) {
		region_t &region = *itr;

		if (region.blks == blks && region.lock_counter>0) {
			region.lock_counter--;
			return(0);
		}
	}

	SetErrMsg("Couldn't unlock region - not found");
	return(-1);
}

void	*DataMgr::get_region_from_cache(
	size_t ts,
	const char *varname,
	int reflevel,
	int lod,
	_dataTypes_t	type,
	const size_t min[3],
	const size_t max[3],
	int	lock
) {

	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); itr++) {
		region_t &region = *itr;

		if (region.ts == ts &&
			region.varname.compare(varname) == 0 &&
			region.reflevel == reflevel &&
			region.lod == lod &&
			region.min[0] == min[0] &&
			region.min[1] == min[1] &&
			region.min[2] == min[2] &&
			region.max[0] == max[0] &&
			region.max[1] == max[1] &&
			region.max[2] == max[2] &&
			region.type == type) {

			// Increment the lock counter
			region.lock_counter += lock ? 1 : 0;

			// Move region to front of list
			region_t tmp_region = region;
			_regionsList.erase(itr);
			_regionsList.push_back(tmp_region);

			return(tmp_region.blks);
		}
	}

	return(NULL);
}

void	*DataMgr::alloc_region(
	size_t ts,
	const char *varname,
	VarType_T vtype,
	int reflevel,
	int lod,
	_dataTypes_t type,
	const size_t min[3],
	const size_t max[3],
	int	lock,
	bool fill
) {

	size_t mem_block_size;
	if (! _blk_mem_mgr) {


		size_t bs[3];
		GetBlockSize(bs, -1);
		mem_block_size = bs[0]*bs[1]*bs[2];

		size_t num_blks = (_mem_size * 1024 * 1024) / mem_block_size;

		BlkMemMgr::RequestMemSize(mem_block_size, num_blks);
		_blk_mem_mgr = new BlkMemMgr();
		if (BlkMemMgr::GetErrCode() != 0) {
			return(NULL);
		}
	}
	mem_block_size = BlkMemMgr::GetBlkSize();

	// Free region already exists
	//
	free_region(ts,varname,reflevel,lod,type,min,max);

	int	vs;

	switch (type) {
	case UINT8:
		vs = 1;
		break;
	case UINT16:
		vs = 2;
		break;
	default:
		vs = 4;
	};

	size_t bs[3];
	GetBlockSize(bs, reflevel);

	size_t size;
	switch (vtype) {
	case VAR2D_XY:
		size = ((max[0]-min[0]+1)*bs[0]) * ((max[1]-min[1]+1)*bs[1]) * vs;
		break;

	case VAR2D_XZ:
		size = ((max[0]-min[0]+1)*bs[0]) * ((max[1]-min[1]+1)*bs[2]) * vs;
		break;
	case VAR2D_YZ:
		size = ((max[0]-min[0]+1)*bs[1]) * ((max[1]-min[1]+1)*bs[2]) * vs;
		break;

	case VAR3D:
		size = ((max[0]-min[0]+1)*bs[0]) * ((max[1]-min[1]+1)*bs[1]) *
			((max[2]-min[2]+1)*bs[2]) * vs;
		break;

	default:
		SetErrMsg("Invalid variable type");
		return(NULL);
	}
	size_t nblocks = (size_t) ceil((double) size / (double) mem_block_size);
		
	float *blks;
	while (! (blks = (float *) _blk_mem_mgr->Alloc(nblocks, fill))) {
		if (free_lru() < 0) {
			SetErrMsg("Failed to allocate requested memory");
			return(NULL);
		}
	}

	region_t region;

	region.ts = ts;
	region.varname.assign(varname);
	region.reflevel = reflevel;
	region.lod = lod;
	region.min[0] = min[0];
	region.min[1] = min[1];
	region.min[2] = min[2];
	region.max[0] = max[0];
	region.max[1] = max[1];
	region.max[2] = max[2];
	region.type = type;
	region.lock_counter = lock;
	region.blks = blks;

	_regionsList.push_back(region);

	return(region.blks);
}

void	DataMgr::free_region(
	size_t ts,
	const char *varname,
	int reflevel,
	int lod,
	_dataTypes_t type,
	const size_t min[3],
	const size_t max[3]
) {

	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); itr++) {
		const region_t &region = *itr;

		if (region.ts == ts &&
			region.varname.compare(varname) == 0 &&
			region.reflevel == reflevel &&
			region.lod == lod &&
			region.min[0] == min[0] &&
			region.min[1] == min[1] &&
			region.min[2] == min[2] &&
			region.max[0] == max[0] &&
			region.max[1] == max[1] &&
			region.max[2] == max[2] &&
			region.type == type ) {

			if (region.lock_counter == 0) {
				if (region.blks) _blk_mem_mgr->FreeMem(region.blks);
				
				_regionsList.erase(itr);
				return;
			}
		}
	}

	return;
}

void	DataMgr::Clear() {

	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); itr++) {
		const region_t &region = *itr;

		if (region.blks) _blk_mem_mgr->FreeMem(region.blks);
			
	}
	_regionsList.clear();
	_dataRangeMinMap.clear();
	_dataRangeMaxMap.clear();
	_validRegMinMaxMap.clear();
}

void	DataMgr::free_var(const string &varname, int do_native) {

	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); ) {
		const region_t &region = *itr;

		if (region.varname.compare(varname) == 0 &&
			(region.type != DataMgr::FLOAT32 || do_native)) {

			if (region.blks) _blk_mem_mgr->FreeMem(region.blks);
				
			_regionsList.erase(itr);
			itr = _regionsList.begin();
		}
		else itr++;
	}

	// Remove min and max data ranges from cache.
	//
	{
	map <size_t, map<string, float> >::iterator itr1;
	for (itr1=_dataRangeMinMap.begin(); itr1 !=_dataRangeMinMap.end(); itr1++) {
		map<string, float> &m = itr1->second;

		map<string, float>::iterator itr2 = m.find(varname);
		if (itr2 != m.end()) {
			m.erase(itr2);
		}
	}
	}

	{
	map <size_t, map<string, float> >::iterator itr1;
	for (itr1=_dataRangeMaxMap.begin(); itr1 !=_dataRangeMaxMap.end(); itr1++) {
		map<string, float> &m = itr1->second;

		map<string, float>::iterator itr2 = m.find(varname);
		if (itr2 != m.end()) {
			m.erase(itr2);
		}
	}
	}

	// Purge variable from valid min/max region cache
	//
	{
	map <size_t, map<string, map<int, vector <size_t> > > >::iterator itr1;
	for (itr1=_validRegMinMaxMap.begin(); itr1 !=_validRegMinMaxMap.end(); itr1++) {
		map<string, map<int, vector <size_t> > >&m = itr1->second;

		map<string, map<int, vector <size_t> > >::iterator itr2 = m.find(varname);
		if (itr2 != m.end()) {
			m.erase(itr2);
		}
	}
	}

}


int	DataMgr::free_lru(
) {

	// The least recently used region is at the front of the list
	//
	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); itr++) {
		const region_t &region = *itr;

		if (region.lock_counter == 0) {
			if (region.blks) _blk_mem_mgr->FreeMem(region.blks);
			_regionsList.erase(itr);
			return(0);
		}
	}

	// nothing to free
	return(-1);
}
	

int	DataMgr::set_quantization_range(const char *varname, const float range[2]) {
	string varstr = varname;
	float *rangeptr;

	map <string, float *>::iterator p;
	p = _quantizationRangeMap.find(varname);

	// 
	// See if this is a new variable
	//
	if (p == _quantizationRangeMap.end()) {
		float *range = new float[2];
		assert(range != NULL);

		range[0] = 0.0;
		range[1] = 0.0;

		// Use of []'s creates an entry in map
		_quantizationRangeMap[varname] = range;
	}
	p = _quantizationRangeMap.find(varname);
	assert (p != _quantizationRangeMap.end());

	rangeptr = p->second;
	if (range[0] <= range[1]) {
		if (range[0] == rangeptr[0] && range[1] == rangeptr[1]) return(0);
		rangeptr[0] = range[0];
		rangeptr[1] = range[1];
	}
	else {
		if (range[0] == rangeptr[1] && range[1] == rangeptr[0]) return(0);
		rangeptr[0] = range[1];
		rangeptr[1] = range[0];
	}
	
	// Invalidate the cache of quantized quantities
	//
	free_var(varstr, 0);
	return(1);
}

int DataMgr::get_cached_data_range(
	size_t ts,
	const char *varname,
	float *range
) {

	// See if we've already cache'd it.
	//

	// Min value
	{
		if (_dataRangeMinMap.empty()) return(-1);

		map <size_t, map<string, float> >::iterator p;


		p = _dataRangeMinMap.find(ts);

		if (p == _dataRangeMinMap.end()) return(-1);

		map <string, float> &vmap = p->second;
		map <string, float>::iterator t;

		t = vmap.find(varname);
		if (t == vmap.end()) return(-1);
		range[0] = t->second;
	}

	// Max value
	{
		if (_dataRangeMaxMap.empty()) return(-1);

		map <size_t, map<string, float> >::iterator p;

		p = _dataRangeMaxMap.find(ts);

		if (p == _dataRangeMaxMap.end()) return(-1);

		map <string, float> &vmap = p->second;
		map <string, float>::iterator t;

		t = vmap.find(varname);
		if (t == vmap.end()) return(-1);
		range[1] = t->second;
	}

	return(0);
}

vector <size_t> DataMgr::get_cached_reg_min_max(
	size_t ts,
	const char *varname,
	int reflevel
) {

	// See if we've already cache'd it.
	//
	if (! _validRegMinMaxMap.empty()) {

		map <size_t, map<string, map<int, vector <size_t> > > >::iterator p;

		p = _validRegMinMaxMap.find(ts);

		if (! (p == _validRegMinMaxMap.end())) {

			map <string, map <int, vector <size_t> > >&vmap = p->second;
			map <string, map <int, vector <size_t> > >::iterator t;

			t = vmap.find(varname);
			if (! (t == vmap.end())) {

				map <int, vector <size_t> > &imap = t->second;
				map <int, vector <size_t> >::iterator s;

				s = imap.find(reflevel);
				if (! (s == imap.end())) {
					return(s->second);
				}
			}
		}
	}

	// not cached
	vector <size_t> empty;
	return(empty);
}

PipeLine *DataMgr::get_pipeline_for_var(string varname) const {

	for (int i=0; i<_PipeLines.size(); i++) {
		const vector <pair <string, VarType_T> > &output_vars = _PipeLines[i]->GetOutputs();

		for (int j=0; j<output_vars.size(); j++) {
			if (output_vars[j].first.compare(varname) == 0) {
				return(_PipeLines[i]);
			}
		}
	}
	return(NULL);
}

float *DataMgr::execute_pipeline(
	size_t ts,
	string varname,
	int reflevel,
	int lod,
	const size_t min[3],
	const size_t max[3],
	int	lock
) {
	PipeLine *pipeline = get_pipeline_for_var(varname);
 
	assert(pipeline != NULL);

	const vector <string> &input_varnames = pipeline->GetInputs();
	const vector <pair <string, VarType_T> > &output_vars = pipeline->GetOutputs();

	//
	// Ptrs to space for input and output variables
	//
	vector <const float *> in_blkptrs;
	vector <float *> out_blkptrs;

	//
	// Get input variables, and lock them into memory
	//
	for (int i=0; i<input_varnames.size(); i++) {
		int my_lock = 1;
		float *blks = GetRegion(
						ts, input_varnames[i].c_str(), reflevel, lod, 
						min, max, my_lock
		);
		if (! blks) {
			// Unlock any locked variables and abort
			//
			for (int j=0; j<in_blkptrs.size(); j++) UnlockRegion(in_blkptrs[j]);
			return(NULL);
		}
		in_blkptrs.push_back(blks);
	}

	//
	// Get space for all output variables generated by the pipeline,
	// including the single variable that we will return.
	//
	int output_index = -1;
	for (int i=0; i<output_vars.size(); i++) {
		int my_lock = 1;

		string v = output_vars[i].first;
		VarType_T vtype = output_vars[i].second;

		//
		// if output variable i is the one we are interested in record
		// the index and use the lock value passed in to this method
		//
		if (v.compare(varname) == 0) {
			output_index = i;
			my_lock = lock;
		}

		float *blks = (float *) alloc_region(
			ts,v.c_str(),vtype, reflevel, lod, DataMgr::FLOAT32,min,max,my_lock,
			true
		);
		if (! blks) {
			// Unlock any locked variables and abort
			//
			for (int j=0;j<in_blkptrs.size();j++) UnlockRegion(in_blkptrs[j]);
			for (int j=0;j<out_blkptrs.size();j++) UnlockRegion(out_blkptrs[j]);
			return(NULL);
		}
		out_blkptrs.push_back(blks);
	}
	assert(output_index >= 0);

	size_t bs[3];
	GetBlockSize(bs, reflevel);

	int rc = pipeline->Calculate(
		in_blkptrs, out_blkptrs, ts, reflevel, lod, bs, min, max
	);

	//
	// Unlock input variables and output variables that are not 
	// being returned.
	//
	// N.B. unlocking a variable doesn't necessarily free it, but
	// makes the space available if needed later
	//
	for (int i=0; i<in_blkptrs.size(); i++) UnlockRegion(in_blkptrs[i]);

	for (int i=0; i<out_blkptrs.size(); i++) {
		if (i != output_index) UnlockRegion(out_blkptrs[i]);
	}

	if (rc < 0) return(NULL);

	for (int i=0; i<output_vars.size(); i++) {

		string v = output_vars[i].first;
		VarType_T vtype = output_vars[i].second;
		float *blks = out_blkptrs[i];

		size_t size;
		switch (vtype) {
		case VAR2D_XY:
			size = ((max[0]-min[0]+1)*bs[0]) * ((max[1]-min[1]+1)*bs[1]);
			break;
		case VAR2D_XZ:
			size = ((max[0]-min[0]+1)*bs[0]) * ((max[1]-min[1]+1)*bs[2]);
			break;
		case VAR2D_YZ:
			size = ((max[0]-min[0]+1)*bs[1]) * ((max[1]-min[1]+1)*bs[2]);
			break;
		case VAR3D:
			size = ((max[0]-min[0]+1)*bs[0]) * ((max[1]-min[1]+1)*bs[1]) *
				((max[2]-min[2]+1)*bs[2]);
			break;
		default: 
			size = 0;
		}
		for (size_t i=0; i<size; i++) {
#ifdef WIN32
			if (! _finite(blks[i]) || _isnan(blks[i])) blks[i] = FLT_MAX;
#else
			if (! finite(blks[i]) || isnan(blks[i])) blks[i] = FLT_MAX;
#endif
		}
	}

	return(out_blkptrs[output_index]);
}

bool DataMgr::cycle_check(
	const map <string, vector <string> > &graph,
	const string &node, 
	const vector <string> &depends
) const {

	if (depends.size() == 0) return(false);

	for (int i=0; i<depends.size(); i++) {
		if (node.compare(depends[i]) == 0) return(true);
	}

	for (int i=0; i<depends.size(); i++) {
		const map <string, vector <string> >::const_iterator itr = 
			graph.find(depends[i]);
		assert(itr != graph.end());

		if (cycle_check(graph, node, itr->second)) return(true);
	}

	return(false);
}

//
// return true iff 'a' depends on 'b' - true if a has inputs that
// match b's outputs.
//
bool DataMgr::depends_on(
	const PipeLine *a, const PipeLine *b
) const {
	const vector <string> &input_varnames = a->GetInputs();
	const vector <pair <string, VarType_T> > &output_vars = b->GetOutputs();

	for (int i=0; i<input_varnames.size(); i++) {
	for (int j=0; j<output_vars.size(); j++) {
		if (input_varnames[i].compare(output_vars[j].first) == 0) {
			return(true);
		}
	}
	}
	return(false);
}

//
// return complete list of native variables
//
vector <string> DataMgr::get_native_variables() const {
    vector <string> svec1, svec2;

    svec1 = _GetVariables3D();
    for(int i=0; i<svec1.size(); i++) svec2.push_back(svec1[i]);
    svec1 = _GetVariables2DXY();
    for(int i=0; i<svec1.size(); i++) svec2.push_back(svec1[i]);
    svec1 = _GetVariables2DXZ();
    for(int i=0; i<svec1.size(); i++) svec2.push_back(svec1[i]);
    svec1 = _GetVariables2DYZ();
    for(int i=0; i<svec1.size(); i++) svec2.push_back(svec1[i]);

    return(svec2);
}

//
// return complete list of derived variables
//
vector <string> DataMgr::get_derived_variables() const {

    vector <string> svec;

	for (int i=0; i<_PipeLines.size(); i++) {
		const vector <pair <string, VarType_T> > &ovars = _PipeLines[i]->GetOutputs();
		for (int j=0; j<ovars.size(); j++) {
			svec.push_back(ovars[j].first);
		}
	}
    return(svec);
}

Metadata::VarType_T DataMgr::GetVarType(const string &varname) const {
	if (!IsVariableDerived(varname)) return Metadata::GetVarType(varname);
	for (int i = 0; i< _PipeLines.size(); i++){
		const vector<pair<string, VarType_T> > &ovars = _PipeLines[i]->GetOutputs();
		for (int j = 0; j<ovars.size(); j++){
			if(ovars[j].first == varname){
				return ovars[j].second;
			}
		}
	}
	assert(0);
	return VAR3D;
}
	

 
void DataMgr::PurgeVariable(string varname){
	free_var(varname,1);
}

