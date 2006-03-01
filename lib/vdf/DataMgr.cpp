#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <map>
#include "vapor/DataMgr.h"
#include "vaporinternal/common.h"
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
	_regionsMap.clear();
	_lockedRegionsMap.clear();
	_dataRangeMap.clear();
	_validRegMinMaxMap.clear();

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

	if (_metadata->GetGridType().compare("block_amr") == 0) {
		_regionReader = new AMRIO(_metadata, nthreads);
	}
	else {
		_regionReader = new WaveletBlock3DRegionReader(_metadata, nthreads);
	}
	if (_regionReader->GetErrCode() != 0) return;

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

	if (_metadata->GetGridType().compare("block_amr") == 0) {
		_regionReader = new AMRIO(_metadata, nthreads);
	}
	else {
		_regionReader = new WaveletBlock3DRegionReader(_metadata, nthreads);
	}
	if (_regionReader->GetErrCode() != 0) return;

	if (_DataMgr(mem_size, nthreads) < 0) return;

	_objInitialized = 1;
}


DataMgr::~DataMgr(
) {
	SetDiagMsg("DataMgr::~DataMgr()");
	if (! _objInitialized) return;

	if (_regionReader) delete _regionReader;

	_regionsMap.clear();
	_lockedRegionsMap.clear();
	_timestamp = 0;

	free_all();
	if (_blk_mem_mgr) delete _blk_mem_mgr;

	map <size_t, map<string, float *> >::iterator p0;
	for(p0 = _dataRangeMap.begin(); p0!=_dataRangeMap.end(); p0++) {

		map<string, float * > &vmap = p0->second;
		map <string, float * >::iterator t;

		for(t = vmap.begin(); t!=vmap.end(); t++) {
			if (t->second) delete [] t->second;
		}
	}
	_dataRangeMap.clear();

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

	_regionReader = NULL;
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
		ts,varname,reflevel,min[0],min[1],min[2],max[0],max[1],max[2],lock
	);

	// See if region is already in cache. If so, return it.
	blks = (float *) get_region_from_cache(
		ts, varname, reflevel, DataMgr::FLOAT32, min, max, lock
	);
	if (blks) return(blks);


	// Else, read it from disk
	//
	rc = _regionReader->OpenVariableRead(ts, varname, reflevel);
	if (rc < 0) return (NULL);


	if (_metadata->GetGridType().compare("block_amr") == 0) {
		AMRIO *amrio = (AMRIO *) _regionReader;
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

cerr << "The max ref level " << amrtree.GetRefinementLevel() << endl;

		(void) amrio->CloseTree();

cerr << "The max ref level " << amrtree.GetRefinementLevel() << endl;
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
cerr << "The max ref level " << amrtree.GetRefinementLevel() << endl;
			
		AMRData amrdata(&amrtree, bs, minbase, maxbase, reflevel);
		if (AMRData::GetErrCode() != 0)  return(NULL);
cerr << "The max ref level " << amrtree.GetRefinementLevel() << endl;

		rc = amrio->VariableRead(&amrdata);
		if (rc < 0) return (NULL);

		(void) amrio->CloseVariable();

		blks = (float *) alloc_region(
			ts,varname,reflevel,DataMgr::FLOAT32,min,max,lock
		);

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
		WaveletBlock3DRegionReader *wbreader = 
			(WaveletBlock3DRegionReader *) _regionReader;

		blks = (float *) alloc_region(
			ts,varname,reflevel,DataMgr::FLOAT32,min,max,lock
		);
		if (! blks) return(NULL);

		rc = wbreader->BlockReadRegion(min, max, blks, 1);
		if (rc < 0) {
			string s = wbreader->GetErrMsg();
			SetErrMsg(
				"Failed to read region from disk : %s", s.c_str()
			);
			(void) free_region(ts,varname,reflevel,FLOAT32,min,max);
			wbreader->CloseVariable();
			return (NULL);
		}
	}


	const float *r = get_cached_data_range(ts, varname);
	if (! r) {

		r = _regionReader->GetDataRange();

		float *range = new float[2];
		assert(range != NULL);

		range[0] = r[0];
		range[1] = r[1];

		// Use of []'s creates an entry in map
		_dataRangeMap[ts][varname] = range;

	}


	size_t *minmax = get_cached_reg_min_max(ts, varname, reflevel);
	if (! minmax) {

		minmax = new size_t[6];
		assert(minmax != NULL);

		_regionReader->GetValidRegion(minmax, &minmax[3], reflevel);

		// Use of []'s creates an entry in map
		_validRegMinMaxMap[ts][varname][reflevel] = minmax;
	}
	

	_regionReader->CloseVariable();

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
	unsigned char	*ublks = NULL;
	const float	*fptr;
	unsigned char *ucptr;
	int	x,y,z;

	SetDiagMsg(
		"DataMgr::GetRegionUInt8(%d,%s,%d,[%d,%d,%d],[%d,%d,%d],[%f,%f],%d)",
		ts,varname,reflevel,min[0],min[1],min[2],max[0],max[1],max[2],
		range[0], range[1], lock
	);

	// 
	// Set the data range for the quantization mapping. This operation 
	// is a no-op if the value specified does not differ from the
	// current mapping for this variable.
	//
	if (set_quantization_range(varname, range) < 0) return (NULL);

	ublks = (unsigned char *) get_region_from_cache(
		ts, varname, reflevel, DataMgr::UINT8, min, max, lock
	);

	if (ublks) return(ublks);

	float	*blks = NULL;

	blks = GetRegion(ts, varname, reflevel, min, max, 1);
	if (! blks) return (NULL);

	ublks = (unsigned char *) alloc_region(
		ts,varname,reflevel,DataMgr::UINT8,min,max,lock
	);
    if (! ublks) return(NULL);

	// Quantize the floating point data;

	fptr = blks;
	ucptr = ublks;

	const size_t *bs = _metadata->GetBlockSize();

	int	nz = (int)((max[2]-min[2]+1) * bs[2]);
	int	ny = (int)((max[1]-min[1]+1) * bs[1]);
	int	nx = (int)((max[0]-min[0]+1) * bs[0]);

	for(z=0;z<nz;z++) {
	for(y=0;y<ny;y++) {
	for(x=0;x<nx;x++) {
		double	f;

		if (*fptr < range[0]) *ucptr = 0;
		else if (*fptr > range[1]) *ucptr = 255;
		else {
			f = (*fptr - range[0]) / (range[1] - range[0]) * 255;
			*ucptr = (unsigned char) rint(f);
		}
		ucptr++;
		fptr++;
	}
	}
	}

	// Unlock the floating point data
	//
	UnlockRegion(blks);

	return(ublks);
}

const float	*DataMgr::GetDataRange(
	size_t ts,
	const char *varname
) {
	int	rc;

	SetDiagMsg("DataMgr::GetDataRange(%d,%s)", ts, varname);


	// See if we've already cache'd it.
	//
	float *range = get_cached_data_range(ts, varname);
	if (range) return(range);
		

	// Range isn't cache'd. Need to read it from the file
	//
	range = new float[2];
	assert(range != NULL);

	rc = _regionReader->OpenVariableRead(ts, varname, 0);
	if (rc < 0) return (NULL);

	const float *r = _regionReader->GetDataRange();


	range[0] = r[0];
	range[1] = r[1];

	// Use of []'s creates an entry in map
	_dataRangeMap[ts][varname] = range;

	_regionReader->CloseVariable();

	return(range);
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

		rc = _regionReader->OpenVariableRead(ts, varname, reflevel);
		if (rc < 0) return(-1);

		_regionReader->GetValidRegion(minmax, &minmax[3], reflevel);

		// Use of []'s creates an entry in map
		_validRegMinMaxMap[ts][varname][reflevel] = minmax;

		_regionReader->CloseVariable();
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
	region_t *regptr;
	map <void *, region_t *>::iterator p;

	SetDiagMsg("DataMgr::UnlockRegion()");

	p = _lockedRegionsMap.find((void *) blks);
	if (p == _lockedRegionsMap.end()) {
		SetErrMsg("Couldn't unlock region - not found");
		return(-1);
	}

	regptr = p->second;
	regptr->lock = 0;

	_lockedRegionsMap.erase(p);

	return(0);
}

int	DataMgr::UnlockRegionUInt8(
	unsigned char *blks
) {
	region_t *regptr;
	map <void *, region_t *>::iterator p;

	SetDiagMsg("DataMgr::UnlockRegionUInt8()");

	p = _lockedRegionsMap.find((void *) blks);
	if (p == _lockedRegionsMap.end()) {
		SetErrMsg("Couldn't unlock region - not found");
		return(-1);
	}

	regptr = p->second;
	regptr->lock = 0;

	_lockedRegionsMap.erase(p);

	return(0);
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

	if (_regionsMap.empty()) return(NULL);

	map <size_t, map<string, vector<region_t *> > >::iterator p;

	p = _regionsMap.find(ts);
	if (p == _regionsMap.end()) return(NULL);

	map<string, vector<region_t *> > &vmap = p->second;
	map <string, vector<region_t *> >::iterator t;

	t = vmap.find(varname);
	if (t == vmap.end()) return(NULL);

	vector<region_t *> &regvec = t->second;

	for(int i = 0; i<(int)regvec.size(); i++) {
		region_t *regptr;

		regptr = regvec[i];

		if (regptr->type == type &&
			regptr->reflevel == reflevel &&
			regptr->min[0] == min[0] && 
			regptr->min[1] == min[1] &&
			regptr->min[2] == min[2] &&
			regptr->max[0] == max[0] && 
			regptr->max[1] == max[1] &&
			regptr->max[2] == max[2]) {

			regptr->lock = lock;
			regptr->timestamp = _timestamp;
			_timestamp++;
			assert(_timestamp > 0); // overflow -- should us a q

			if (lock) {
				_lockedRegionsMap[(void *) regptr->blks] = regptr;
			}
			return(regptr->blks);
		}
	}

	return(NULL);
}

void	*DataMgr::alloc_region(
	size_t ts,
	const char *varname,
	int reflevel,
	_dataTypes_t type,
	const size_t min[3],
	const size_t max[3],
	int	lock
) {
	region_t *regptr = NULL;
	int	nblocks;
	int	vs;
	int	i;

	// accessing the map with the '[]' syntax creates an empty 
	// element if the key does not exist
	//
    map <string, vector<region_t *> > &varsMap = _regionsMap[ts];

    vector<region_t *> &regvec = varsMap[varname];

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
		
	nblocks = (int)((max[0]-min[0]+1) * (max[1]-min[1]+1) * (max[2]-min[2]+1) * vs);

	// See if already have a block allocation for this region
	//
	for(i = 0,regptr=NULL; i<(int)regvec.size() && regptr==NULL; i++) {

		regptr = regvec[i];

		if (regptr->type == type &&
			regptr->reflevel == reflevel &&
			regptr->min[0] == min[0] && 
			regptr->min[1] == min[1] &&
			regptr->min[2] == min[2] &&
			regptr->max[0] == max[0] && 
			regptr->max[1] == max[1] &&
			regptr->max[2] == max[2]) {


			if (regptr->blks) _blk_mem_mgr->FreeMem(regptr->blks);
			regptr->blks = NULL;

		}
		else {
			regptr = NULL;
		}
	}

	if (! regptr) {
		regptr = new region_t;
		regptr->reflevel = reflevel;
		regptr->type = type;
		regptr->min[0] = min[0];
		regptr->min[1] = min[1];
		regptr->min[2] = min[2];
		regptr->max[0] = max[0];
		regptr->max[1] = max[1];
		regptr->max[2] = max[2];

		regvec.push_back(regptr);
	}
	regptr->timestamp = _timestamp;
	_timestamp++;
	assert(_timestamp > 0); // overflow -- should us a q
	regptr->lock = lock;

	while (! (regptr->blks = _blk_mem_mgr->Alloc(nblocks))) {
		if (free_lru() < 0) {
			SetErrMsg("Failed to allocate requested memory");
			return(NULL);
		}
	}

	if (lock) {
		_lockedRegionsMap[(void *) regptr->blks] = regptr;
	}

	return(regptr->blks);
}

int	DataMgr::free_region(
	size_t ts,
	const char *varname,
	int reflevel,
	_dataTypes_t type,
	const size_t min[3],
	const size_t max[3]
) {
	if (_regionsMap.empty()) return(-1);

	map <size_t, map<string, vector<region_t *> > >::iterator p;

	p = _regionsMap.find(ts);
	if (p == _regionsMap.end()) return(-1);

	map<string, vector<region_t *> > &vmap = p->second;
	map <string, vector<region_t *> >::iterator t;

	t = vmap.find(varname);
	if (t == vmap.end()) return(-1);

	vector<region_t *> &regvec = t->second;

	for(int i = 0; i<(int)regvec.size(); i++) {
		region_t *regptr = regvec[i];

		if (regptr->type == type &&
			regptr->reflevel == reflevel &&
			regptr->min[0] == min[0] && 
			regptr->min[1] == min[1] &&
			regptr->min[2] == min[2] &&
			regptr->max[0] == max[0] && 
			regptr->max[1] == max[1] &&
			regptr->max[2] == max[2]) {

			if (! regptr->lock) {
				if (regptr->blks) _blk_mem_mgr->FreeMem(regptr->blks);
				
				delete regptr;
				regvec.erase(regvec.begin()+i);
				return(0);
			}
		}
	}
	return(-1);
}

void	DataMgr::free_all() {

	map <size_t, map<string, vector<region_t *> > >::iterator p;
	for(p = _regionsMap.begin(); p!=_regionsMap.end(); p++) {

		map<string, vector<region_t *> > &vmap = p->second;
		map <string, vector<region_t *> >::iterator t;

		for(t = vmap.begin(); t!=vmap.end(); t++) {
			vector <region_t *> &regvec = t->second;

			// Erase matching elements 
			//
			vector <region_t *>::iterator itr;
			itr = regvec.begin();
			while (itr != regvec.end()) {
				region_t *regptr = *itr;

				if (regptr->blks) _blk_mem_mgr->FreeMem(regptr->blks);
				delete regptr;
				++itr;
			}
			regvec.clear();
		}
	}
}

void	DataMgr::free_var(const string &varname, int do_native) {

	map <size_t, map<string, vector<region_t *> > >::iterator p;
	for(p = _regionsMap.begin(); p!=_regionsMap.end(); p++) {

		map<string, vector<region_t *> > &vmap = p->second;
		map <string, vector<region_t *> >::iterator t;

		t = vmap.find(varname);
		if (t == vmap.end()) return;

		vector <region_t *> &regvec = t->second;

		// Erase matching elements 
		//
		vector <region_t *>::iterator itr;
		itr = regvec.begin();
		while (itr != regvec.end()) {
			region_t *regptr = *itr;

			if (regptr->type != DataMgr::FLOAT32 || do_native) {
				if (regptr->blks) _blk_mem_mgr->FreeMem(regptr->blks);
				delete regptr;
				regvec.erase(itr);

				// Changed the vector. Need to reset the pointer
				// to the beginning.
				//
				itr = regvec.begin();
			}
			else {
				++itr;
			}
		}
	}
}


int	DataMgr::free_lru(
) {
	region_t *regptr;

	int	lrutime = _timestamp;
	int	lruindex = -1;	
	vector <region_t *> *lruvec = NULL;

	// search for lowest time stamp
	//
	map <size_t, map<string, vector<region_t *> > >::iterator p;

	for(p = _regionsMap.begin(); p!=_regionsMap.end(); p++) {

		map<string, vector<region_t *> > &vmap = p->second;
		map <string, vector<region_t *> >::iterator t;

		for(t = vmap.begin(); t!=vmap.end(); t++) {
			vector <region_t *> &regvec = t->second;

			for(int i=0; i<(int)regvec.size(); i++) {
				regptr = regvec[i];

				if (regptr->timestamp < lrutime && !regptr->lock) {

					lrutime = regptr->timestamp;
					lruvec = &regvec;
					lruindex = i;
				}
				
			}
		}
	}

	if (lruindex < 0) return(-1);	// nothing to free;

	regptr = lruvec->at(lruindex);
	lruvec->erase(lruvec->begin()+lruindex);


	if (regptr->blks) _blk_mem_mgr->FreeMem(regptr->blks);
	regptr->blks = NULL;

	delete regptr;

	return(0);
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
	return(0);
}

float *DataMgr::get_cached_data_range(
	size_t ts,
	const char *varname
) {

	// See if we've already cache'd it.
	//
	if (! _dataRangeMap.empty()) {

		map <size_t, map<string, float *> >::iterator p;

		p = _dataRangeMap.find(ts);

		if (! (p == _dataRangeMap.end())) {

			map <string, float *> &vmap = p->second;
			map <string, float *>::iterator t;

			t = vmap.find(varname);
			if (! (t == vmap.end())) {
				return (t->second);
			}
		}
	}

	// Not cached
	return(NULL);
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
					return (s->second);
				}
			}
		}
	}

	// Not cached
	return(NULL);
}
