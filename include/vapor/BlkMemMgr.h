//
//      $Id$
//

#ifndef	_BlkMemMgr_h_
#define	_BlkMemMgr_h_

#include <vapor/MyBase.h>

namespace VAPoR {

//
//! \class BlkMemMgr
//! \brief The VetsUtil BlkMemMgr class
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! A block-based memory allocator. Allocates memory contiguous runs of
//! memory blocks from a memory pool of user defined size.
// 
class BlkMemMgr : public VetsUtil::MyBase {

public:
 //! Initialize a memory allocator
 //
 //! Initialize a block-based memory allocator
 //! \param[in] blk_size Size of a single memory block in bytes
 //! \param[in] num_blks Size of memory pool in blocks. This is the
 //! maximum amount that will be available through subsequent 
 //! \b Alloc() calls.
 //! \param[in] page_aligned If true, start address of memory pool 
 //! will be page aligned
 //
 BlkMemMgr(unsigned int blk_size, unsigned int num_blks, int page_aligned);
 ~BlkMemMgr();

 //! Alloc space from memory pool
 //
 //! Return a pointer to the specified amount of memory from the memory pool
 //! \param[in] num_blks Size of memory region requested in blocks
 //! \retval ptr A pointer to the requested memory pool
 //
 void	*Alloc(unsigned int num_blks);

 //! Free memory
 //
 //! Frees memory previosly allocated with the \b Alloc() method.
 //! \param[in] ptr Pointer to memory returned by previous call to 
 //! \b Alloc().
 void	FreeMem(void *ptr);

private:
 unsigned char	*base_c;
 unsigned char	*aligned_base_c;
 int	page_aligned_c;
 int	mem_size_c;	// size of mem in blocks
 int	blk_size_c;	// size of block in bytes
 int	*free_table_c;	// free block table
 int	page_size_c;	// page size in bytes of OS
 unsigned char	*blks_c;	// memory pool
 unsigned char	*blkptr_c;	// page-aligned memory pool

};
};

#endif	//	_BlkMemMgr_h_
