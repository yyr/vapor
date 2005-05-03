#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <iostream>
#ifndef WIN32
#include <unistd.h>
#endif

#include "vapor/BlkMemMgr.h"

using namespace VetsUtil;
using namespace VAPoR;

BlkMemMgr::BlkMemMgr(
	unsigned int blk_size,
	unsigned int mem_size,
	int page_aligned
) {
	_objInitialized = 0;

	int	i;
	size_t	size;

	SetClassName("BlkMemMgr");

	free_table_c = NULL;
	blks_c = NULL;

	page_aligned_c = page_aligned;
	mem_size_c = mem_size;
	blk_size_c = blk_size;

	free_table_c = new(nothrow) int[mem_size_c];
	if (! free_table_c) {
		SetErrMsg("new(nothrow) int[%lu] : alloc failed", mem_size_c);
		return;
	}
	for(i=0; i<mem_size_c; i++) free_table_c[i] = 0;

	if (page_aligned_c) {
#ifdef WIN32
		page_size_c = 4096;
#else
		page_size_c = sysconf(_SC_PAGESIZE);
		if (page_size_c < 0) {
			SetErrMsg("sysconf(_SC_PAGESIZE) : %s",strerror(errno));
			return;
		}

		if ((blk_size_c % page_size_c) && (page_size_c % blk_size_c)) {
			SetErrMsg("Can't align pages");
			page_aligned_c = 0;
		}
#endif
	}


	size = (size_t) blk_size_c * (size_t) mem_size_c;
	size += (size_t) page_size_c;

	blks_c = new(nothrow) unsigned char[size];
	if (! blks_c) {
		SetErrMsg("new(nothrow) unsigned char [%u] : alloc failed", size);
		if (free_table_c) delete [] free_table_c;
		return;
	}
	blkptr_c = blks_c;
	if (page_size_c) {
		blkptr_c += page_size_c - ((long) blks_c % page_size_c);
	}

	_objInitialized = 1;

}

BlkMemMgr::~BlkMemMgr() {
	if (! _objInitialized) return;

	if (free_table_c) delete [] free_table_c;
	if (blks_c) delete [] blks_c;

	free_table_c = NULL;
	blks_c = NULL;

	_objInitialized = 0;
}

void	*BlkMemMgr::Alloc(
	unsigned int n
) {
	int	i,j;
	int	index = -1;

	i = 0;
	while(i<mem_size_c && index == -1) { 
		if (free_table_c[i] == 0) {
			for(j=0; j<(int)n && i<mem_size_c && free_table_c[i+j]==0; j++);
			if (j>=(int) n) index = i;
			i += j;
		}
		else {
			i += free_table_c[i];
		}
	}

	if (index < 0) return(NULL);
				
	free_table_c[index] = n;

	return(blkptr_c + ((long) index * (long) blk_size_c));
}

void	BlkMemMgr::FreeMem(
	void *ptr
) {
	long index = (long) ((unsigned char *) ptr - blkptr_c) / blk_size_c;

	free_table_c[index] = 0;
}
