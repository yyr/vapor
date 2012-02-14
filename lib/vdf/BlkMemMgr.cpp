#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <new>
#ifndef WIN32
#include <unistd.h>
#endif

#include <vapor/BlkMemMgr.h>

using namespace VetsUtil;
using namespace VAPoR;

//
//	Static member initialization
//
bool BlkMemMgr::_page_aligned_req = true;
size_t BlkMemMgr::_mem_size_max_req = 32768;
size_t BlkMemMgr::_blk_size_req = 32*32*32;

bool BlkMemMgr::_page_aligned = false;
size_t BlkMemMgr::_mem_size_max = 0;
size_t BlkMemMgr::_blk_size = 0;
size_t BlkMemMgr::_max_regions = 8;

vector <size_t>	BlkMemMgr::_mem_size;
vector <size_t *> BlkMemMgr::_free_table;
vector <unsigned char *> BlkMemMgr::_blks;
vector <unsigned char *> BlkMemMgr::_blkptr;

int	BlkMemMgr::_ref_count = 0;

int	BlkMemMgr::_Reinit(bool restart, size_t n)
{
	long page_size = 0;
	size_t size = 0;

	if (_mem_size_max_req == 0 || _blk_size_req == 0) return(false);

	_page_aligned = _page_aligned_req;
	_mem_size_max = _mem_size_max_req;
	_blk_size = _blk_size_req;

	//
	// Calculate starting region size
	//
	size_t mem_size = _mem_size_max;
	size_t max_regions = _max_regions;
	if (restart) {
		while (mem_size > 1 && mem_size*_blk_size > 128*1024*1024 && max_regions>1) {
			mem_size = mem_size >> 1;
			max_regions--;
		}
	}
	else {
		size_t total_size = 0;
		int r;
		for (r=0; r<_free_table.size(); r++) total_size += _mem_size[r];

		//
		// New region size is double preceding one
		//
		if (r>0) mem_size = _mem_size[r-1] << 1;

		// Make sure region size will be large enough, and not too large
		//
		if (mem_size < n) mem_size = n;
		if ((mem_size + total_size) > _mem_size_max) mem_size = _mem_size_max - total_size;

		if (mem_size < n) return(false);
	}


	if (_page_aligned) {
#ifdef WIN32
		page_size = 4096;
#else
		page_size = sysconf(_SC_PAGESIZE);
		if (page_size < 0) page_size = 0;
#endif
	}

	unsigned char *blks;
	do {
		size = (size_t) _blk_size * (size_t) mem_size;
		size += (size_t) page_size;

		blks = new(nothrow) unsigned char[size];
		if (! blks) {
			SetDiagMsg(
				"BlkMemMgr::_Reinit() : failed to allocate %d blocks, retrying",
				 mem_size
			);
			mem_size = mem_size >> 1;
		}
	} while (blks == NULL && mem_size > 0 && _blk_size > 0);

	if (! blks && mem_size > 0 && _blk_size > 0) {
		SetDiagMsg("Memory allocation of %lu bytes failed", size);
		return(false);
	}
	else {
		SetDiagMsg("BlkMemMgr() : allocated %lu bytes", size);
	}

	unsigned char *blkptr = blks;

	if (page_size) {
		blkptr += page_size - (((size_t) blks) % page_size);
	}

	size_t *free_table = new(nothrow) size_t[mem_size];
	if (! free_table) {
		SetDiagMsg("Memory allocation of %lu ints failed", mem_size);
		return(false);
	}

	for(size_t i=0; i<mem_size; i++) free_table[i] = 0;

	_free_table.push_back(free_table);
	_mem_size.push_back(mem_size);
	_blks.push_back(blks);
	_blkptr.push_back(blkptr);

	return(true);
}

int	BlkMemMgr::RequestMemSize(
	size_t blk_size, 
	size_t num_blks, 
	bool page_aligned
) {

	SetDiagMsg(
		"BlkMemMgr::RequestMemSize(%u,%u,%d)", blk_size, num_blks, page_aligned
	);

	//
	// If there are no instances of this object, re-initialized
	// the static memory pool if needed
	//
	if (blk_size == 0 || num_blks == 0) {
		SetErrMsg("Invalid request");
		return(-1);
	}

	_blk_size_req = blk_size;
	_mem_size_max_req = num_blks;
	_page_aligned_req = page_aligned;

	return(0);
}

BlkMemMgr::BlkMemMgr(
) {

	SetDiagMsg("BlkMemMgr::BlkMemMgr()");


	//
	// If there are no other instances of this object, re-initialized
	// the static memory pool if needed
	//
	if (_ref_count != 0) {
		_ref_count++;
		return;
	}

	for (int i=0; i<_free_table.size(); i++) {
		if (_free_table[i]) delete [] _free_table[i];
		if (_blks[i]) delete [] _blks[i];
	}
	_free_table.clear();
	_blks.clear();
	_blkptr.clear();
	_mem_size.clear();


	(void) BlkMemMgr::_Reinit(true,0);
	_ref_count = 1;

}

BlkMemMgr::~BlkMemMgr() {
	SetDiagMsg("BlkMemMgr::~BlkMemMgr()");

	if (_ref_count > 0) _ref_count--;

	if (_ref_count != 0) return;

	for (int i=0; i<_free_table.size(); i++) {
		if (_free_table[i]) delete [] _free_table[i];
		if (_blks[i]) delete [] _blks[i];
	}
	_free_table.clear();
	_blks.clear();
	_blkptr.clear();
	_mem_size.clear();

}

void	*BlkMemMgr::Alloc(
	size_t n,
	bool fill
) {
	size_t	i,j;
	long long	index = -1;

	SetDiagMsg("BlkMemMgr::Alloc(%d)", n);

	int r;
	for (r=0; r<_free_table.size() && index == -1; r++) {
		i = 0;
		while(i<_mem_size[r] && index == -1) { 
			if (_free_table[r][i] == 0) {
				for(j=0; j<n && i+j<_mem_size[r] && _free_table[r][i+j]==0; j++);
				if (j>=n) index = i;
				i += j;
			}
			else {
				i += _free_table[r][i];
			}
		}
	}
	r--;

	if (index < 0) {
		// Couldn't find space in existing memory pool.
		// Try to allocate more memory.
		//
		if (! BlkMemMgr::_Reinit(false, n)) 
			return(NULL);

		return(Alloc(n,fill));
	}
				
	_free_table[r][index] = n;

	if (fill) {
		unsigned char *ptr = _blkptr[r] + ((long) index * (long) _blk_size);
		for (size_t i=0; i<n*_blk_size; i++) ptr[i] = 0;
	}

	return(_blkptr[r] + ((long) index * (long) _blk_size));
}

void	BlkMemMgr::FreeMem(
	void *ptr
) {
	SetDiagMsg("BlkMemMgr::FreeMem()");


	int r;
	bool found = false;
	for (r = 0; r<_free_table.size() && ! found; r++) {
		if (ptr >= _blkptr[r] && ptr < (_blkptr[r] + _mem_size[r])) {
			found = true;
			long index = (long) ((unsigned char *) ptr - _blkptr[r]) / _blk_size;
			_free_table[r][index] = 0;

			// Garbage collection. Delete region if it is completely
			// empty.
			//
			bool empty = true;
			for (size_t i=0; i<_mem_size[r] && empty; i++) {
				if (_free_table[r][i] != 0) empty = false;
			}
			if (empty) {
				if (_blks[r]) delete [] _blks[r];
				if (_free_table[r]) delete [] _free_table[r];

				_free_table.erase(_free_table.begin() + r);
				_mem_size.erase(_mem_size.begin() + r);
				_blks.erase(_blks.begin() + r);
				_blkptr.erase(_blkptr.begin() + r);
			}
		}
	}
}
