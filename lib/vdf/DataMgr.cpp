#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <map>
#include "vapor/DataMgr.h"
#include "vaporinternal/common.h"
#include "vapor/LayeredIO.h"
#include "vapor/errorcodes.h"
using namespace VetsUtil;
using namespace VAPoR;

int	DataMgr::_DataMgr(
	size_t mem_size,
	unsigned int nthreads
) {
	size_t block_size;
	size_t num_blks;

	SetClassName("DataMgr");

	_blk_mem_mgr = NULL;

	_quantizationRangeMap.clear();
	_regionsList.clear();
	_dataRangeMinMap.clear();
	_dataRangeMaxMap.clear();
	_validRegMinMaxMap.clear();

	if (_metadata->GetGridType().compare("block_amr") == 0) {
		_regionReader3D = new AMRIO(_metadata, nthreads);
		_regionReader2D = NULL;
	} 
	else if (_metadata->GetGridType().compare("layered") == 0) {
		_regionReader2D = new WaveletBlock2DRegionReader(_metadata);
		if (_regionReader2D->GetErrCode() != 0) return(-1);

		_regionReader3D = new LayeredIO(_metadata, nthreads);
	} 
	else {
		_regionReader2D = new WaveletBlock2DRegionReader(_metadata);
		if (_regionReader2D->GetErrCode() != 0) return(-1);

		_regionReader3D = new WaveletBlock3DRegionReader(_metadata, nthreads);
	}
	if (_regionReader3D->GetErrCode() != 0) return(-1);


	_timestamp = 0;

	const size_t *bs = _metadata->GetBlockSize();

	block_size = bs[0] * bs[1] * bs[2];

	num_blks = (long long) (mem_size * 1024 * 1024) / block_size;

	BlkMemMgr::RequestMemSize((unsigned int)block_size, (unsigned int)num_blks);
	_blk_mem_mgr = new BlkMemMgr();
	if (BlkMemMgr::GetErrCode() != 0) {
		return(-1);
	}

	// Initialize default quantization ranges for each variable
	//
	const vector <string> &varnames = _metadata->GetVariableNames();
	for(int i=0; i<varnames.size(); i++) {
		float *range = new float[2];
		assert(range != NULL);

		range[0] = 0.0;
		range[1] = 0.0;

		// Use of []'s creates an entry in map
		_quantizationRangeMap[varnames[i]] = range;
	}
	return(0);
}

DataMgr::DataMgr(
	const Metadata *metadata,
	size_t mem_size,
	unsigned int nthreads
) {
	_objInitialized = 0;

	SetDiagMsg("DataMgr::DataMgr(,%d,%d)", mem_size, nthreads);

	_metadata = metadata;

	if (_DataMgr(mem_size, nthreads) < 0) return;

	_objInitialized = 1;
}

DataMgr::DataMgr(
	const char	*metafile,
	size_t mem_size,
	unsigned int nthreads
) {
	_objInitialized = 0;
	SetDiagMsg("DataMgr::DataMgr(%s,%d,%d)", metafile, mem_size, nthreads);

	_metadata = new Metadata(metafile);
	if (Metadata::GetErrCode() != 0) return;

	if (_DataMgr(mem_size, nthreads) < 0) return;

	_objInitialized = 1;
}


DataMgr::~DataMgr(
) {
	SetDiagMsg("DataMgr::~DataMgr()");
	if (! _objInitialized) return;

	if (_regionReader3D) delete _regionReader3D;
	if (_regionReader2D) delete _regionReader2D;

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

	_regionReader2D = NULL;
	_regionReader3D = NULL;
	_blk_mem_mgr = NULL;

	_objInitialized = 0;
}

float	*DataMgr::GetRegion(
	size_t ts,
	const char *varname,
	int reflevel,
	const size_t min[3],
	const size_t max[3],
	int	lock
) {

	float	*blks = NULL;
	int	rc;

	SetDiagMsg(
		"DataMgr::GetRegion(%d,%s,%d,[%d,%d,%d],[%d,%d,%d],%d)",
		ts,varname,reflevel,min[0],min[1],min[2],max[0],max[1],max[2], lock
	);

	// See if region is already in cache. If so, return it.
	blks = (float *) get_region_from_cache(
		ts, varname, reflevel, DataMgr::FLOAT32, min, max, lock
	);
	if (blks) {
		SetDiagMsg("DataMgr::GetRegion() - data in cache xll\n", blks);
		return(blks);
	}


	// Else, read it from disk
	//

	VDFIOBase::VarType_T vtype = VDFIOBase::GetVarType(_metadata, varname);
	VDFIOBase *baseio_reader;
	if (vtype != VDFIOBase::VAR3D) {
		baseio_reader = _regionReader2D;

		rc = _regionReader2D->OpenVariableRead(ts, varname, reflevel);
		if (rc < 0) return (NULL);

		blks = (float *) alloc_region(
			ts,varname,vtype, reflevel,DataMgr::FLOAT32,min,max,lock
		);
		if (! blks) return(NULL);

		size_t min2d[2], max2d[2];
		switch (vtype) {
		case VDFIOBase::VAR2D_XY:
			min2d[0] = min[0]; min2d[1] = min[1]; 
			max2d[0] = max[0]; max2d[1] = max[1];
			break;
		case VDFIOBase::VAR2D_XZ:
			min2d[0] = min[0]; min2d[1] = min[2]; 
			max2d[0] = max[0]; max2d[1] = max[2];
			break;
		case VDFIOBase::VAR2D_YZ:
			min2d[0] = min[1]; min2d[1] = min[2]; 
			max2d[0] = max[1]; max2d[1] = max[2];
			break;
		default:
			SetErrMsg("Invalid variable type");
			return(NULL);
		}
		
		rc = _regionReader2D->BlockReadRegion(min2d, max2d, blks, 1);
		if (rc < 0) {
			string s = _regionReader2D->GetErrMsg();
			SetErrMsg( "Failed to read region from disk : %s", s.c_str());
			(void) free_region(ts,varname,reflevel,FLOAT32,min,max);
			_regionReader2D->CloseVariable();
			return (NULL);
		}
	}
	else if (_metadata->GetGridType().compare("block_amr") == 0) {
		baseio_reader = _regionReader3D;	// needed for close

		AMRIO *amrio = (AMRIO *) baseio_reader;

		rc = amrio->OpenVariableRead(ts, varname, reflevel);
		if (rc < 0) return (NULL);

		AMRTree amrtree;

		//
		// Read in the AMR tree
		// N.B. Really only need to do this when the time step
		// changes - same tree used for all variables of a given 
		// time step
		//
		rc = amrio->OpenTreeRead(ts);
		if (rc < 0) return (NULL);

		rc = amrio->TreeRead(&amrtree);
		if (rc < 0) return (NULL);

		(void) amrio->CloseTree();

		//
		// Read in the AMR field data
		//
		const size_t *bs = _metadata->GetBlockSize();
		size_t minbase[3];
		size_t maxbase[3];
		for (int i=0; i<3; i++) {
			minbase[i] = min[i] >> reflevel;
			maxbase[i] = max[i] >> reflevel;
		}
			
		AMRData amrdata(&amrtree, bs, minbase, maxbase, reflevel);
		if (AMRData::GetErrCode() != 0)  return(NULL);

		rc = amrio->VariableRead(&amrdata);
		if (rc < 0) return (NULL);

		blks = (float *) alloc_region(
			ts,varname,vtype, reflevel,DataMgr::FLOAT32,min,max,lock
		);
		if (! blks) return(NULL);

		rc = amrdata.ReGrid(min,max, reflevel, blks);
		if (rc < 0) {
			string s = AMRData::GetErrMsg();
			SetErrMsg(
				"Failed to regrid region : %s", s.c_str()
			);
			(void) free_region(ts,varname,reflevel,FLOAT32,min,max);
			return (NULL);
		}
	}
	else { 
		baseio_reader = _regionReader3D;	// needed for close
		WaveletBlock3DRegionReader *reg_reader = 
			(WaveletBlock3DRegionReader *) _regionReader3D;

		rc = reg_reader->OpenVariableRead(ts, varname, reflevel);
		if (rc < 0) return (NULL);

		blks = (float *) alloc_region(
			ts,varname,vtype, reflevel,DataMgr::FLOAT32,min,max,lock
		);
		if (! blks) return(NULL);

		rc = reg_reader->BlockReadRegion(min, max, blks, 1);
		if (rc < 0) {
			string s = reg_reader->GetErrMsg();
			SetErrMsg(
				"Failed to read region from disk : %s", s.c_str()
			);
			(void) free_region(ts,varname,reflevel,FLOAT32,min,max);
			reg_reader->CloseVariable();
			return (NULL);
		}
	}


	float range[2];
	if (get_cached_data_range(ts, varname, range) < 0) {

		const float *r = baseio_reader->GetDataRange();

		// Use of []'s creates an entry in map
		_dataRangeMinMap[ts][varname] = r[0];
		_dataRangeMaxMap[ts][varname] = r[1];
	}


	size_t *minmax = get_cached_reg_min_max(ts, varname, reflevel);
	if (! minmax) {

		minmax = new size_t[6];
		assert(minmax != NULL);

		baseio_reader->GetValidRegion(minmax, &minmax[3], reflevel);

		// Use of []'s creates an entry in map
		_validRegMinMaxMap[ts][varname][reflevel] = minmax;
	}
	baseio_reader->CloseVariable();
	
	SetDiagMsg("DataMgr::GetRegion() - data not in cache %xll\n", blks);

	return(blks);
}


unsigned char	*DataMgr::GetRegionUInt8(
	size_t ts,
	const char *varname,
	int reflevel,
	const size_t min[3],
	const size_t max[3],
	const float range[2],
	int lock
) {
	SetDiagMsg(
		"DataMgr::GetRegionUInt8(%d,%s,%d,[%d,%d,%d],[%d,%d,%d],[%f,%f],%d)",
		ts,varname,reflevel,min[0],min[1],min[2],max[0],max[1],max[2],
		range[0], range[1], lock
	);

	return(get_quantized_region(
		ts, varname, reflevel, min, max, range, 
		lock, DataMgr::UINT8
	));
}

unsigned char	*DataMgr::GetRegionUInt8(
	size_t ts,
	const char *varname1,
	const char *varname2,
	int reflevel,
	const size_t min[3],
	const size_t max[3],
	const float range1[2],
	const float range2[2],
	int lock
) {
	SetDiagMsg(
		"DataMgr::GetRegionUInt8(%d,%s,%s,%d,[%d,%d,%d],[%d,%d,%d],[%f,%f],[%f,%f],%d)",
		ts,varname1, varname2, reflevel,min[0],min[1],min[2],
		max[0],max[1],max[2], range1[0], range1[1], 
		range2[0], range2[1], lock
	);
	VDFIOBase::VarType_T vtype1 = VDFIOBase::GetVarType(_metadata, varname1);
	VDFIOBase::VarType_T vtype2 = VDFIOBase::GetVarType(_metadata, varname2);
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
			ts, varname.c_str(), reflevel, DataMgr::UINT16, 
			min, max, lock
		);
	}
	if (ublks) return(ublks);

	// Interleaved array is not in cache so we'll need to
	// construct it
	//
	const size_t *bs = _metadata->GetBlockSize();

	int	nz = (int)((max[2]-min[2]+1) * bs[2]);
	int	ny = (int)((max[1]-min[1]+1) * bs[1]);
	int	nx = (int)((max[0]-min[0]+1) * bs[0]);
	switch (vtype1) {
	case VDFIOBase::VAR2D_XY:
		nz = 1;
		break;
	case VDFIOBase::VAR2D_XZ:
		ny = 1;
		break;
	case VDFIOBase::VAR2D_YZ:
		nx = 1;
		break;
	default:
		break;
	}
	size_t size = nx*ny*nz;

	unsigned char *ublks1, *ublks2;

	ublks = (unsigned char *) alloc_region(
		ts,varname.c_str(),vtype1, reflevel,DataMgr::UINT16,
		min,max, lock
	);
	if (! ublks) return(NULL);

	ublks1 = get_quantized_region(
		ts, varname1, reflevel, min, max, range1, 
		0, DataMgr::UINT8
	);
	if (! ublks1) return (NULL);

	for (size_t i = 0; i<size; i++) {
		ublks[2*i] = ublks1[i];
	}

	ublks2 = get_quantized_region(
		ts, varname2, reflevel, min, max, range2, 
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
	const size_t min[3],
	const size_t max[3],
	const float range[2],
	int lock
) {
	SetDiagMsg(
		"DataMgr::GetRegionUInt16(%d,%s,%d,[%d,%d,%d],[%d,%d,%d],[%f,%f],%d)",
		ts,varname,reflevel,min[0],min[1],min[2],max[0],max[1],max[2],
		range[0], range[1], lock
	);

	return(get_quantized_region(
		ts, varname, reflevel, min, max, range, 
		lock, DataMgr::UINT16
	));
}

unsigned char	*DataMgr::GetRegionUInt16(
	size_t ts,
	const char *varname1,
	const char *varname2,
	int reflevel,
	const size_t min[3],
	const size_t max[3],
	const float range1[2],
	const float range2[2],
	int lock
) {
	SetDiagMsg(
		"DataMgr::GetRegionUInt16(%d,%s,%s,%d,[%d,%d,%d],[%d,%d,%d],[%f,%f],[%f,%f],%d)",
		ts,varname1, varname2, reflevel,min[0],min[1],min[2],
		max[0],max[1],max[2], range1[0], range1[1], 
		range2[0], range2[1], lock
	);

	VDFIOBase::VarType_T vtype1 = VDFIOBase::GetVarType(_metadata, varname1);
	VDFIOBase::VarType_T vtype2 = VDFIOBase::GetVarType(_metadata, varname2);
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
			ts, varname.c_str(), reflevel, DataMgr::UINT32, 
			min, max, lock
		);
	}
	if (ublks) return(ublks);

	// Interleaved array is not in cache so we'll need to
	// construct it
	//
	const size_t *bs = _metadata->GetBlockSize();

	int	nz = (int)((max[2]-min[2]+1) * bs[2]);
	int	ny = (int)((max[1]-min[1]+1) * bs[1]);
	int	nx = (int)((max[0]-min[0]+1) * bs[0]);
	switch (vtype1) {
	case VDFIOBase::VAR2D_XY:
		nz = 1;
		break;
	case VDFIOBase::VAR2D_XZ:
		ny = 1;
		break;
	case VDFIOBase::VAR2D_YZ:
		nx = 1;
		break;
	default:
		break;
	}
	size_t size = nx*ny*nz;

	unsigned char *ublks1, *ublks2;

	ublks = (unsigned char *) alloc_region(
		ts,varname.c_str(),vtype1, reflevel,DataMgr::UINT32,
		min,max, lock
	);
	if (! ublks) return(NULL);

	ublks1 = get_quantized_region(
		ts, varname1, reflevel, min, max, range1, 
		0, DataMgr::UINT16
	);
	if (! ublks1) return (NULL);

	for (size_t i = 0; i<size; i++) {
		ublks[2*2*i+0] = ublks1[2*i+0];
		ublks[2*2*i+1] = ublks1[2*i+1];
	}

	ublks2 = get_quantized_region(
		ts, varname2, reflevel, min, max, range2, 
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
	if (set_quantization_range(varname, range) < 0) return (NULL);

	ublks = (unsigned char *) get_region_from_cache(
		ts, varname, reflevel, type, min, max, lock
	);

	if (ublks) {
		SetDiagMsg(
			"DataMgr::get_quantized_region() - data in cache %xll\n", ublks
		);
		return(ublks);
	}
    
	float	*blks = NULL;

	blks = GetRegion(ts, varname, reflevel, min, max, 1);
	if (! blks) return (NULL);


	VDFIOBase::VarType_T vtype = VDFIOBase::GetVarType(_metadata, varname);
	ublks = (unsigned char *) alloc_region(
		ts,varname,vtype, reflevel,type,min,max, lock
	);
	if (! ublks) {
		UnlockRegion(blks);
		return(NULL);
	}

	// Quantize the floating point data;

	const size_t *bs = _metadata->GetBlockSize();

	int	nz = (int)((max[2]-min[2]+1) * bs[2]);
	int	ny = (int)((max[1]-min[1]+1) * bs[1]);
	int	nx = (int)((max[0]-min[0]+1) * bs[0]);

	switch (vtype) {
	case VDFIOBase::VAR2D_XY:
		nz = 1;
		break;
	case VDFIOBase::VAR2D_XZ:
		ny = 1;
		break;
	case VDFIOBase::VAR2D_YZ:
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

	VDFIOBase *baseio_reader;
	VDFIOBase::VarType_T vtype = VDFIOBase::GetVarType(_metadata, varname);
	switch (vtype) {
	case VDFIOBase::VAR2D_XY:
	case VDFIOBase::VAR2D_XZ:
	case VDFIOBase::VAR2D_YZ:
		baseio_reader = _regionReader2D;
		break;
	case VDFIOBase::VAR3D:
		baseio_reader = _regionReader3D;
		break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}
	rc = baseio_reader->OpenVariableRead(ts, varname, 0);
	if (rc < 0) return (-1);

	// Ugh! For AMR data we have to read some data to get the
	// range. i.e. simply opening the file doesn't provide
	// the range info.
	//
	if (_metadata->GetGridType().compare("block_amr") == 0) {
		AMRIO *amrio = (AMRIO *) baseio_reader;
		AMRTree amrtree;

		rc = amrio->OpenTreeRead(ts);
		if (rc < 0) return (-1);

		rc = amrio->TreeRead(&amrtree);
		if (rc < 0) return (-1);
		(void) amrio->CloseTree();

		//
		// Read in small amount of AMR field data
		//
		const size_t *bs = _metadata->GetBlockSize();
		size_t minbase[3] = {0,0,0};
		size_t maxbase[3] = {1,1,1};
		AMRData amrdata(&amrtree, bs, minbase, maxbase, 0);

		if (AMRData::GetErrCode() != 0)  return(-1);

		rc = amrio->VariableRead(&amrdata);
		if (rc < 0) return (-1);

		(void) amrio->CloseVariable();
	} 

	const float *r = baseio_reader->GetDataRange();


	range[0] = r[0];
	range[1] = r[1];

	// Use of []'s creates an entry in map
	_dataRangeMinMap[ts][varname] = range[0];
	_dataRangeMaxMap[ts][varname] = range[1];

	baseio_reader->CloseVariable();

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

	SetDiagMsg("DataMgr::GetValidRegion(%d,%s,%d)", ts, varname, reflevel);


	// See if we've already cache'd it.
	//
	size_t *minmax = get_cached_reg_min_max(ts, varname, reflevel);

	if (! minmax) {
		

		// Range isn't cache'd. Need to read it from the file
		//
		minmax = new size_t[6];
		assert(minmax != NULL);

		VDFIOBase *baseio_reader;
		VDFIOBase::VarType_T vtype = VDFIOBase::GetVarType(_metadata, varname);
		switch (vtype) {
		case VDFIOBase::VAR2D_XY:
		case VDFIOBase::VAR2D_XZ:
		case VDFIOBase::VAR2D_YZ:
			baseio_reader = _regionReader2D;
			break;
		case VDFIOBase::VAR3D:
			baseio_reader = _regionReader3D;
			break;
		default:
			SetErrMsg("Invalid variable type");
			return(-1);
		}

		rc = baseio_reader->OpenVariableRead(ts, varname, reflevel);
		if (rc < 0) return(-1);

		baseio_reader->GetValidRegion(minmax, &minmax[3], reflevel);

		// Use of []'s creates an entry in map
		_validRegMinMaxMap[ts][varname][reflevel] = minmax;

		baseio_reader->CloseVariable();
	}

	for (int i=0; i<3; i++) {
		min[i] = minmax[i];
		max[i] = minmax[i+3];
	}

	return(0);
}


#ifdef	DEAD
const float	*DataMgr::GetQuantizationRange(const char *varname) const {

	map <string, float *>::const_iterator p;
	p = _quantizationRangeMap.find(varname);

	if (p == _quantizationRangeMap.end()) {
		SetErrMsg("Unknown variable : %s", varname);
		return(NULL);
	}

	return(p->second);
}
#endif
	
int	DataMgr::UnlockRegion(
	float *blks
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

int	DataMgr::UnlockRegionUInt8(
	unsigned char *blks
) {

	SetDiagMsg("DataMgr::UnlockRegionUInt8()");

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
	VDFIOBase::VarType_T vtype,
	int reflevel,
	_dataTypes_t type,
	const size_t min[3],
	const size_t max[3],
	int	lock
) {

	// See if requested region already exists
	//
	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); itr++) {
		region_t &region = *itr;

		if (region.ts == ts &&
			region.varname.compare(varname) == 0 &&
			region.reflevel == reflevel &&
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

	int	nblocks;
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

	const size_t *bs = _metadata->GetBlockSize();
	switch (vtype) {
	case VDFIOBase::VAR2D_XY:
		nblocks = (int) ceil((double) ((max[0]-min[0]+1) * (max[1]-min[1]+1) * vs) / (double) bs[2]);
		break;
	case VDFIOBase::VAR2D_XZ:
		nblocks = (int) ceil((double) ((max[0]-min[0]+1) * (max[2]-min[2]+1) * vs) / (double) bs[1]);
		break;
	case VDFIOBase::VAR2D_YZ:
		nblocks = (int) ceil((double) ((max[1]-min[1]+1) * (max[2]-min[2]+1) * vs) / (double) bs[0]);
		break;
	case VDFIOBase::VAR3D:
		nblocks = (int) (max[0]-min[0]+1) * (max[1]-min[1]+1) * (max[2]-min[2]+1) * vs;
		break;
	default:
		SetErrMsg("Invalid variable type");
		return(NULL);
	}
		

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

	if (p == _quantizationRangeMap.end()) {
		SetErrMsg("Unknown variable : %s", varname);
		return(-1);
	}

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

	// Not cached
	return(NULL);
}

