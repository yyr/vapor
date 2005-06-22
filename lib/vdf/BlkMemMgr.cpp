#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <new>
#ifndef WIN32
#include <unistd.h>
#endif

#include "vapor/BlkMemMgr.h"

using namespace VetsUtil;
using namespace VAPoR;

//
//	Static member initialization
//
int BlkMemMgr::_page_aligned_req = 1;
int BlkMemMgr::_mem_size_req = 32;
int BlkMemMgr::_blk_size_req = 32*32*32;

int BlkMemMgr::_page_aligned = 0;
int BlkMemMgr::_mem_size = 0;
int BlkMemMgr::_blk_size = 0;

int *BlkMemMgr::_free_table = NULL;
unsigned char   *BlkMemMgr::_blks = NULL;
unsigned char   *BlkMemMgr::_blkptr = NULL;

int	BlkMemMgr::_ref_count = 0;

int	BlkMemMgr::_Reinit()
{
	long page_size;
	size_t size = 0;
	int	i;

	if (_free_table) delete [] _free_table;
	if (_blks) delete [] _blks;

	_free_table = NULL;
	_blks = NULL;


	_page_aligned = _page_aligned_req;
	_mem_size = _mem_size_req;
	_blk_size = _blk_size_req;

	if (_mem_size == 0 || _blk_size == 0) return(0);

	_free_table = new(nothrow) int[_mem_size];
	if (! _free_table) {
		SetErrMsg("Memory allocation of %lu ints failed", _mem_size);
		return(-1);
	}

	for(i=0; i<_mem_size; i++) _free_table[i] = 0;

	if (_page_aligned) {
#ifdef WIN32
		page_size = 4096;
#else
		page_size = sysconf(_SC_PAGESIZE);
		if (page_size < 0) {
			SetErrMsg("sysconf(_SC_PAGESIZE) : %s",strerror(errno));
			if (_free_table) delete [] _free_table;
			return(-1);
		}

		if ((_blk_size % page_size) && (page_size % _blk_size)) {
			SetErrMsg("Can't align pages");
			if (_free_table) delete [] _free_table;
			return(-1);
		}
#endif
	}


	do {
		size = (size_t) _blk_size * (size_t) _mem_size;
		size += (size_t) page_size;

		_blks = new(nothrow) unsigned char[size];
		if (! _blks) {
			SetDiagMsg(
				"BlkMemMgr::_Reinit() : failed to allocate %d blocks, retrying",
				 _mem_size
			);
			_mem_size--;
		}
	} while (_blks == NULL && _mem_size > 0 && _blk_size > 0);

	if (! _blks && _mem_size_req > 0 && _blk_size_req > 0) {
		SetErrMsg("Memory allocation of %lu bytes failed", size);
		if (_free_table) delete [] _free_table;
		return(-1);
	}

	_blkptr = _blks;

	if (page_size) {
		_blkptr += page_size - ((long) _blks % page_size);
	}

	return(0);

}

void	BlkMemMgr::RequestMemSize(
	unsigned int blk_size, 
	unsigned int num_blks, 
	int page_aligned
) {

	SetDiagMsg(
		"BlkMemMgr::RequestMemSize(%u,%u,%d)", blk_size, num_blks, page_aligned
	);

	_blk_size_req = blk_size;
	_mem_size_req = num_blks;
	_page_aligned_req = page_aligned;

	//
	// If there are no instances of this object, re-initialized
	// the static memory pool if needed
	//
	if (_ref_count == 0 && _mem_size_req == 0 || _blk_size_req == 0) {
		if (BlkMemMgr::_Reinit() < 0) return;
	}
}

BlkMemMgr::BlkMemMgr(
) {
	_objInitialized = 0;

	SetDiagMsg("BlkMemMgr::BlkMemMgr()");

	SetClassName("BlkMemMgr");


	//
	// If there are no other instances of this object, re-initialized
	// the static memory pool if needed
	//
	if (_ref_count == 0) {
		if (_page_aligned_req != _page_aligned ||
			_mem_size_req != _mem_size	||
			_blk_size_req != _blk_size)  {

			if (BlkMemMgr::_Reinit() < 0) return;
		}
	}
	_ref_count++;

	_objInitialized = 1;

}

BlkMemMgr::~BlkMemMgr() {
	SetDiagMsg("BlkMemMgr::~BlkMemMgr()");

	if (! _objInitialized) return;

	_ref_count--;
	_objInitialized = 0;

}

void	*BlkMemMgr::Alloc(
	unsigned int n
) {
	int	i,j;
	int	index = -1;

	SetDiagMsg("BlkMemMgr::Alloc(%d)", n);

	i = 0;
	while(i<_mem_size && index == -1) { 
		if (_free_table[i] == 0) {
			for(j=0; j<(int)n && i<_mem_size && _free_table[i+j]==0; j++);
			if (j>=(int) n) index = i;
			i += j;
		}
		else {
			i += _free_table[i];
		}
	}

	if (index < 0) return(NULL);
				
	_free_table[index] = n;

	return(_blkptr + ((long) index * (long) _blk_size));
}

void	BlkMemMgr::FreeMem(
	void *ptr
) {
	long index = (long) ((unsigned char *) ptr - _blkptr) / _blk_size;

	SetDiagMsg("BlkMemMgr::FreeMem()");

	_free_table[index] = 0;
}
