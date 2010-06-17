#ifdef BUILD_GUI
#include <Python.h>
#include <arrayobject.h>
#include "vapor/PythonControl.h"
#endif
#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <map>
#include "vapor/DataMgr.h"
#include "vaporinternal/common.h"
#include "vapor/errorcodes.h"
#include "vapor/Metadata.h"
using namespace VetsUtil;
using namespace VAPoR;


int	DataMgr::_DataMgr(
	size_t mem_size
) {
	SetClassName("DataMgr");

	_blk_mem_mgr = NULL;

	_quantizationRangeMap.clear();
	_regionsList.clear();
	_dataRangeMinMap.clear();
	_dataRangeMaxMap.clear();
	_validRegMinMaxMap.clear();
	UpdateDerivedMappings(0,0,0,0,0);
	_mem_size = mem_size;

	_timestamp = 0;
	derivedMethodMapPtr = 0;
	derived2DInputMapPtr=0;
	derived3DInputMapPtr=0;
	derived2DOutputMapPtr=0;
	derived3DOutputMapPtr=0;

	return(0);
}

DataMgr::DataMgr(
	size_t mem_size
) {

	SetDiagMsg("DataMgr::DataMgr(,%d)", mem_size);

	if (_DataMgr(mem_size) < 0) return;
#ifdef BUILD_GUI
	pyControl = new PythonControl(this);
#endif

}


DataMgr::~DataMgr(
) {
	SetDiagMsg("DataMgr::~DataMgr()");

	_timestamp = 0;

	Clear();
	if (_blk_mem_mgr) delete _blk_mem_mgr;

	_dataRangeMinMap.clear();
	_dataRangeMaxMap.clear();

	map <size_t, map<string, map<int, size_t *> > >::iterator p1;
	for(p1 = _validRegMinMaxMap.begin(); p1!=_validRegMinMaxMap.end(); p1++) {

		map <string, map <int, size_t * > > &vmap = p1->second;
		map <string, map <int, size_t * > >::iterator t;

		for(t = vmap.begin(); t!=vmap.end(); t++) {
			map <int, size_t * > &imap = t->second;
			map <int, size_t * >::iterator u;

			for(u=imap.begin(); u!=imap.end(); u++) {
				if (u->second) delete [] u->second;
			}
		}
	}
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
#ifdef BUILD_GUI
	//Else, if it's derived, calculate it:
	int scriptId = getDerivedScriptId(string(varname));
	if (scriptId >= 0){
		// Invoke the method in its c++ -t wrapper.
		//This will truncate/reform the (blocked)region extents
		//to specify true extents of a region that fits within the full data bounds.
		//Then calls the python method with the truncated region, plus ts and reflevel
		//as arguments.
		if (0 != pyControl->python_wrapper(scriptId, ts,reflevel,min,max)){
			SetErrMsg("DataMgr::GetRegion() - python derivation failure\n");
			return NULL;
		}
				
		blks = (float *) get_region_from_cache(ts, varname, reflevel, -1, DataMgr::FLOAT32, min, max, lock);
		if (blks) {
			printf(" data returned from python script execution\n");
			SetDiagMsg("DataMgr::GetRegion() - python-derived data in cache %xll\n", blks);
			return(blks);
		} else {
			printf(" data NOT returned from python script execution\n");
			SetErrMsg("DataMgr::GetRegion() - derived data not in cache\n");
			return(NULL);
		}
	}
#endif
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
		ts,varname,vtype, reflevel, lod, DataMgr::FLOAT32,min,max,lock
	);
	if (! blks) return(NULL);

	rc = BlockReadRegion(min, max, blks);
	if (rc < 0) {
		SetErrMsg(
			"Failed to read region from variable/timestep/level/lod (%s, %d, %d, %d)",
			varname, ts, reflevel, lod
		);
		(void) free_region(ts,varname,reflevel,lod,FLOAT32,min,max);
		CloseVariable();
		return (NULL);
	}

	CloseVariable();
	
	SetDiagMsg("DataMgr::GetRegion() - data not in cache %xll\n", blks);

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
	const size_t *bs = GetBlockSize();

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
		min,max, lock
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
	const size_t *bs = GetBlockSize();

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
		min,max, lock
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
		ts,varname,vtype, reflevel,lod, type,min,max, lock
	);
	if (! ublks) {
		UnlockRegion(blks);
		return(NULL);
	}

	// Quantize the floating point data;

	const size_t *bs = GetBlockSize();

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
	float *range
) {
	int	rc;

	SetDiagMsg("DataMgr::GetDataRange(%d,%s)", ts, varname);


	// See if we've already cache'd it.
	//
	if (get_cached_data_range(ts, varname, range) == 0) return(0);

	// Range isn't cache'd. Need to read it from the file
	//
	rc = OpenVariableRead(ts, varname, 0,0);
	if (rc < 0) {
		SetErrMsg(
			"Failed to read variable/timestep/level/lod (%s, %d, %d, %d)",
			varname, ts, 0,0
		);
		return(-1);
	}

	const float *r = GetDataRange();

	range[0] = r[0];
	range[1] = r[1];

	// Use of []'s creates an entry in map
	_dataRangeMinMap[ts][varname] = range[0];
	_dataRangeMaxMap[ts][varname] = range[1];

	CloseVariable();

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


	// See if we've already cache'd it.
	//
	size_t *minmax = get_cached_reg_min_max(ts, varname, reflevel);

	if (! minmax) {
		
		// Range isn't cache'd. Need to read it from the file
		//
		minmax = new size_t[6];
		assert(minmax != NULL);

		rc = OpenVariableRead(ts, varname, reflevel, 0);
		if (rc < 0) {
			SetErrMsg(
				"Failed to read variable/timestep/level/lod (%s, %d, %d %d)",
				varname, ts, reflevel, 0
			);
			return(-1);
		}

		GetValidRegion(minmax, &minmax[3], reflevel);

		// Use of []'s creates an entry in map
		_validRegMinMaxMap[ts][varname][reflevel] = minmax;

		CloseVariable();
	}

	for (int i=0; i<3; i++) {
		min[i] = minmax[i];
		max[i] = minmax[i+3];
	}

	return(0);
}

	
int	DataMgr::UnlockRegion(
	void *blks
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
	int	lock
) {

	size_t mem_block_size;
	if (! _blk_mem_mgr) {


		const size_t *bs = GetBlockSize();
		mem_block_size = bs[0]*bs[1]*bs[2];

		size_t num_blks = (_mem_size * 1024 * 1024) / mem_block_size;

		BlkMemMgr::RequestMemSize(mem_block_size, num_blks);
		_blk_mem_mgr = new BlkMemMgr();
		if (BlkMemMgr::GetErrCode() != 0) {
			return(NULL);
		}
	}
	mem_block_size = BlkMemMgr::GetBlkSize();

	// See if requested region already exists
	//
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

			// Reset the lock counter
			region.lock_counter = lock;

			// Move region to front of list
			region_t tmp_region = region;
			_regionsList.erase(itr);
			_regionsList.push_back(tmp_region);

			return(tmp_region.blks);
		}
	}

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
	while (! (blks = (float *) _blk_mem_mgr->Alloc(nblocks))) {
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

int	DataMgr::free_region(
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
				return(0);
			}
		}
	}

	return(-1);
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

size_t *DataMgr::get_cached_reg_min_max(
	size_t ts,
	const char *varname,
	int reflevel
) {

	// See if we've already cache'd it.
	//
	if (! _validRegMinMaxMap.empty()) {

		map <size_t, map<string, map<int, size_t *> > >::iterator p;

		p = _validRegMinMaxMap.find(ts);

		if (! (p == _validRegMinMaxMap.end())) {

			map <string, map <int, size_t *> > &vmap = p->second;
			map <string, map <int, size_t *> >::iterator t;

			t = vmap.find(varname);
			if (! (t == vmap.end())) {

				map <int, size_t *> &imap = t->second;
				map <int, size_t *>::iterator s;

				s = imap.find(reflevel);
				if (! (s == imap.end())) {
					return(s->second);
				}
			}
		}
	}

	// not cached
	return(NULL);
}

//following methods needed for python execution
//Override Metadata virtual method:
//
Metadata::VarType_T DataMgr::GetVarType(const string &varname) const
{
	int scriptid = getDerivedScriptId(varname);
	if (scriptid < 0) return Metadata::GetVarType(varname);
	const vector<string>& outvars3d = getDerived3DOutputs(scriptid);
	for (int i = 0; i< outvars3d.size(); i++)
		if (outvars3d[i] == varname) return (VAR3D);
	const vector<string>& outvars2d = getDerived2DOutputs(scriptid);
	for (int i = 0; i< outvars2d.size(); i++)
		if (outvars2d[i] == varname) return (VAR2D_XY);
	return(VARUNKNOWN);
}
	

 //Obtain the id for a given output variable, return -1 if it does not exist
int DataMgr::getDerivedScriptId(const string& outvar) const{
	map <int, vector<string> > :: const_iterator outIter = derived2DOutputMapPtr->begin();
	while (outIter != derived2DOutputMapPtr->end()){
		vector<string> vars = outIter->second;
		for (int i = 0; i<vars.size(); i++){
			if (vars[i] == outvar) return outIter->first;
		}
		outIter++;
	}
	outIter = derived3DOutputMapPtr->begin();
	while (outIter != derived3DOutputMapPtr->end()){
		vector<string> vars = outIter->second;
		for (int i = 0; i<vars.size(); i++){
			if (vars[i] == outvar) return outIter->first;
		}
		outIter++;
	}
	return -1;
}
const string& DataMgr::getDerivedScript(int id) const{
	map<int,string> :: const_iterator iter = derivedMethodMapPtr->find(id);
	if (iter == derivedMethodMapPtr->end()) return *(new string(""));
	else return iter->second;
}
const vector<string>& DataMgr::getDerived2DInputs(int id) const{
	map<int,vector<string> > :: const_iterator iter = derived2DInputMapPtr->find(id);
	if (iter == derived2DInputMapPtr->end()) return emptyVec;
	else return iter->second;
}
 const vector<string>& DataMgr::getDerived2DOutputs(int id) const{
	map<int,vector<string> > :: const_iterator iter = derived2DOutputMapPtr->find(id);
	if (iter == derived2DOutputMapPtr->end()) return emptyVec;
	else return iter->second;
}
 const vector<string>& DataMgr::getDerived3DInputs(int id) const{
	map<int,vector<string> > :: const_iterator iter = derived3DInputMapPtr->find(id);
	if (iter == derived3DInputMapPtr->end()) return emptyVec;
	else return iter->second;
}
 const vector<string>& DataMgr::getDerived3DOutputs(int id) const{
	map<int,vector<string> > :: const_iterator iter = derived3DOutputMapPtr->find(id);
	if (iter == derived3DOutputMapPtr->end()) return emptyVec;
	else return iter->second;
}
 
void DataMgr::PurgeScriptOutputs(int id){
	map<int,vector<string> > :: const_iterator iter = derived2DOutputMapPtr->find(id);
	if (iter != derived2DOutputMapPtr->end()) {
		vector<string> outputvars = iter->second;
		for (int i = 0; i<outputvars.size(); i++) free_var(outputvars[i],1);
	}
	iter = derived3DOutputMapPtr->find(id);
	if (iter != derived3DOutputMapPtr->end()) {
		vector<string> outputvars = iter->second;
		for (int i = 0; i<outputvars.size(); i++) free_var(outputvars[i],1);
	}
 }
