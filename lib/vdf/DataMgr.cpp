#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <map>
#include "vapor/DataMgr.h"
#include "vaporinternal/common.h"
using namespace VetsUtil;
using namespace VAPoR;

void	DataMgr::_DataMgr(
	size_t mem_size,
	unsigned int nthreads
) {
	size_t bs;
	size_t block_size;
	size_t num_blks;

	_blk_mem_mgr = NULL;

	_dataRange[0] = 0.0;
	_dataRange[1] = 1.0;


	_regionsMap.clear();
	_lockedRegionsMap.clear();

	_timestamp = 0;

	bs = _metadata->GetBlockSize();

	block_size = bs * bs * bs;

	num_blks = (long long) (mem_size * 1024 * 1024) / block_size;

	_blk_mem_mgr = new BlkMemMgr((unsigned int)block_size, (unsigned int)num_blks, 1);

}

DataMgr::DataMgr(
	const Metadata *metadata,
	size_t mem_size,
	unsigned int nthreads
) {
	_metadata = metadata;

	_wbreader = new WaveletBlock3DRegionReader(_metadata, nthreads);
	if (_wbreader->GetErrCode() != 0) return;

	_DataMgr(mem_size, nthreads);
}

DataMgr::DataMgr(
	const char	*metafile,
	size_t mem_size,
	unsigned int nthreads
) {
	if (_metadata->GetErrCode() != 0) return;

	_wbreader = new WaveletBlock3DRegionReader(metafile, nthreads);
	if (_wbreader->GetErrCode() != 0) return;

	_metadata = _wbreader->GetMetadata();

	_DataMgr(mem_size, nthreads);
}


DataMgr::~DataMgr(
) {

	if (_wbreader) delete _wbreader;
	if (_blk_mem_mgr) delete _blk_mem_mgr;
	if (_doFreeMeta && _metadata) delete _metadata;

	free_all(1);
	_regionsMap.clear();
	_lockedRegionsMap.clear();
	_timestamp = 0;

	_wbreader = NULL;
	_blk_mem_mgr = NULL;

}

float	*DataMgr::GetRegion(
	size_t ts,
	const char *varname,
	size_t num_xforms,
	const size_t min[3],
	const size_t max[3],
	int	lock
) {
	float	*blks = NULL;
	int	rc;

	// See if region is already in cache. If so, return it.
	blks = (float *) get_region_from_cache(
		ts, varname, num_xforms, DataMgr::FLOAT32, min, max, lock
	);
	if (blks) return(blks);


	// Else, read it from disk
	//
	rc = _wbreader->OpenVariableRead(ts, varname, num_xforms);
	if (rc < 0) return (NULL);

	blks = (float *) alloc_region(
		ts,varname,num_xforms,DataMgr::FLOAT32,min,max,lock
	);
    if (! blks) return(NULL);

	rc = _wbreader->BlockReadRegion(min, max, blks, 1);
	if (rc < 0) {
		string s = _wbreader->GetErrMsg();
		SetErrMsg(
			"Failed to read region from disk : %s", s.c_str()
		);
		(void) free_region(ts,varname,num_xforms,FLOAT32,min,max);
		_wbreader->CloseVariable();
		return (NULL);
	}

	_wbreader->CloseVariable();

	return(blks);
}


unsigned char	*DataMgr::GetRegionUInt8(
	size_t ts,
	const char *varname,
	size_t num_xforms,
	const size_t min[3],
	const size_t max[3],
	int lock
) {
	unsigned char	*ublks = NULL;
	const float	*fptr;
	unsigned char *ucptr;
	size_t bs;
	int	x,y,z;


	ublks = (unsigned char *) get_region_from_cache(
		ts, varname, num_xforms, DataMgr::UINT8, min, max, lock
	);

	if (ublks) return(ublks);

	float	*blks = NULL;

	blks = GetRegion(ts, varname, num_xforms, min, max, 1);
	if (! blks) return (NULL);

	ublks = (unsigned char *) alloc_region(
		ts,varname,num_xforms,DataMgr::UINT8,min,max,lock
	);
    if (! blks) return(NULL);

	// Quantize the floating point data;

	fptr = blks;
	ucptr = ublks;

	bs = _metadata->GetBlockSize();

	int	nz = (int)((max[2]-min[2]+1) * bs);
	int	ny = (int)((max[1]-min[1]+1) * bs);
	int	nx = (int)((max[0]-min[0]+1) * bs);

cerr << "quantizing" << endl;

	for(z=0;z<nz;z++) {
	for(y=0;y<ny;y++) {
	for(x=0;x<nx;x++) {
		double	f;

		if (*fptr < _dataRange[0]) *ucptr = 0;
		else if (*fptr > _dataRange[1]) *ucptr = 255;
		else {
			f = (*fptr - _dataRange[0]) / (_dataRange[1] - _dataRange[0]) * 255;
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

void	DataMgr::SetDataRange(float range[2]) {

	_dataRange[0] = range[0] < range[1] ? range[0] : range[1];
	_dataRange[1] = range[1] > range[0] ? range[1] : range[0];

	// Invalidate the cache of quantized quantities
	//
	free_all(0);
}
	
int	DataMgr::UnlockRegion(
	float *blks
) {
	region_t *regptr;
	map <void *, region_t *>::iterator p;

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
	size_t num_xforms,
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
			regptr->num_xforms == num_xforms &&
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
	size_t num_xforms,
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
			regptr->num_xforms == num_xforms &&
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
		regptr->num_xforms = num_xforms;
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
	size_t num_xforms,
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
			regptr->num_xforms == num_xforms &&
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

void	DataMgr::free_all(int do_native) {

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
