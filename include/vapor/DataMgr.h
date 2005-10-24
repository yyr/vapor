//
//      $Id$
//

#ifndef	_DataMgr_h_
#define	_DataMgr_h_


#include <vapor/MyBase.h>
#include "vapor/BlkMemMgr.h"
#include "vapor/WaveletBlock3DRegionReader.h"
#include "vaporinternal/common.h"

namespace VAPoR {

//
//! \class DataMgr
//! \brief A cache based data reader
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! This class provides a wrapper to the \b WaveletBlock3DRegionReader
//! class that includes memory cache. Data regions read from disk through 
//! this
//! interface are stored in a cache in main memory, where they may be
//! be available for future access without reading from disk.
//
class VDF_API DataMgr : public VetsUtil::MyBase {

public:

 //! Constructor for the DataMgr class. 
 //! \param[in] metadata Pointer to a metadata class object for which all 
 //! future class operations will apply
 //! \param[in] mem_size Size of memory cache to be created, specified 
 //! in MEGABYTES!!
 //! \param[in] nthreads Number of execution threads that may be used by
 //! the class for parallel execution.
 //! \note The success or failure of this constructor can be checked
 //! with the GetErrCode() method.
 //! 
 //! \sa Metadata, WaveletBlock3DRegionReader, GetErrCode(), 
 //
 DataMgr(
	const Metadata *metadata,
	size_t mem_size,
	unsigned int nthreads = 1
 );

 //! Constructor for the DataMgr class. 
 //! \param[in] metadata Path to a metadata file for which all 
 //! future class operations will apply
 //! \param[in] mem_size Size of memory cache to be created, specified 
 //! in MEGABYTES!!
 //! \param[in] nthreads Number of execution threads that may be used by
 //! the class for parallel execution.
 //! \note The success or failure of this constructor can be checked
 //! with the GetErrCode() method.
 //! 
 //! \sa Metadata, WaveletBlock3DRegionReader, GetErrCode(), 
 //
 DataMgr(
	const char	*metafile,
	size_t mem_size,
	unsigned int nthreads = 1
 );


 ~DataMgr(); 

 //! Read in and return a subregion from the multiresolution dataset.
 //!
 //! GetRegion() will first check to see if the requested region resides
 //! in cache. If so, no reads are performed.
 //! The \p ts, \p varname, and \p level pararmeter tuple identifies 
 //! the time step, variable name, and refinement level, 
 //! respectively, of the requested volume.
 //! The \p min and \p max vectors identify the minium and
 //! maximum extents, in block coordinates, of the subregion of interest. The
 //! minimum valid value of \p min is (0,0,0), the maximum valid value of
 //! \p max is (nbx-1,nby-1,nbz-1), where nbx, nby, and nbz are the block
 //! dimensions
 //! of the volume at the resolution indicated by \p level. I.e.
 //! the coordinates are specified relative to the desired volume
 //! resolution. If the requested region is available, GetRegion() returns 
 //! a pointer to memory containing the region. Subsequent calls to GetRegion()
 //! may invalidate the memory space returned by previous calls unless
 //! the \p lock parameter is set, in which case the array returned by
 //! GetRegion() is locked into memory until freed by a call the
 //! UnlockRegion() method (or the class is destroyed).
 //!
 //! GetRegion will fail if the requested data are not present. The
 //! VariableExists() method may be used to determine if the data
 //! identified by a (resolution,timestep,variable) tupple are
 //! available on disk.
 //!
 //! \param[in] ts A valid time step from the Metadata object used 
 //! to initialize the class
 //! \param[in] varname A valid variable name 
 //! \param[in] level Refinement level requested
 //! \param[in] min Minimum region bounds in blocks
 //! \param[in] max Maximum region bounds in blocks
 //! \param[in] lock If true, the memory region will be locked into the 
 //! cache (i.e. valid after subsequent GetRegion() calls).
 //! \retval ptr A pointer to a region containing the desired data, or NULL
 //! if the region can not be extracted.
 //! \sa WaveletBlock3DRegionReader, GetErrMsg()
 //
 float   *GetRegion(
    size_t ts,
    const char *varname,
    int reflevel,
    const size_t min[3],
    const size_t max[3],
    int lock = 0
 );

 //! Read in, quantize and return a subregion from the multiresolution dataset
 //!
 //! This method is identical to the GetRegion() method except that the
 //! data are returned as quantized, 8-bit unsigned integers. 
 //! Regions with integer data types are created by quantizing
 //! native floating point representations such that floating values
 //! less than or equal to \p range[0] are mapped to min, and values 
 //! greater than or equal to \p range[1] are mapped to max, where "min" and
 //! "max" are the minimum and maximum values that may be represented 
 //! by the integer type. For example, for 8-bit, unsigned ints min is 0
 //! and max is 255. Floating point values between \p range[0] and \p range[1]
 //! are linearly interpolated between min and max.
 //!
 //! \param[in] ts A valid time step from the Metadata object used 
 //! to initialize the class
 //! \param[in] varname A valid variable name 
 //! \param[in] reflevel Transformation number requested
 //! \param[in] min Minimum region bounds in blocks
 //! \param[in] max Maximum region bounds in blocks
 //! \param[in] range A two-element vector specifying the minimum and maximum
 //! quantization mapping. 
 //! \param[in] lock If true, the memory region will be locked into the 
 //! \retval ptr A pointer to a region containing the desired data, 
 //! quantized to 8 bits, or NULL
 //! if the region can not be extracted.
 //! \sa WaveletBlock3DRegionReader, GetErrMsg(), GetRegion()
 //
 unsigned char   *GetRegionUInt8(
    size_t ts,
    const char *varname,
    int reflevel,
    const size_t min[3],
    const size_t max[3],
	const float range[2],
    int lock = 0
);


#ifdef	DEAD
 //! Return the current data range as a two-element array
 //!
 //! \param[in] varname Name of variable to which data range applies
 //! \retval range A two-element vector containing the current 
 //! minimum and maximum or NULL if the variable is not known
 //! quantization mapping.
 //! \sa SetQuantizationRange()
 //
 const float	*GetQuantizationRange(const char *varname) const;
#endif

 //! Unlock a floating-point region of memory 
 //!
 //! Unlock a floating-point region of memory previously locked GetRegion(). 
 //! The region is simply marked available for
 //! internal garbage collection during subsequent GetRegion() calls
 //!
 //! \param[in] region A pointer to a region of memory previosly 
 //! returned by GetRegion()
 //! \retval status Returns a non-negative value on success
 //!
 //! \sa GetRegion(), GetRegion()
 //
 int	UnlockRegion (
    float *region
 );

 //! Unlock an unsigned-int region of memory 
 //!
 //! Unlock a unsigned-int region of memory previously locked 
 //! GetRegionUInt8(). The region is simply marked available for
 //! internal garbage collection during subsequent GetRegionUInt8() calls
 //!
 //! \param[in] region A pointer to a region of memory previosly 
 //! returned by GetRegion()
 //! \retval status Returns a non-negative value on success
 //!
 //! \sa GetRegion(), GetRegion()
 //
 int	UnlockRegionUInt8 (
    unsigned char *region
 );

 //! Return the current data range as a two-element array
 //!
 //! This method returns the minimum and maximum data values
 //! for the indicated time step and variable
 //!
 //! \param[in] ts A valid time step from the Metadata object used 
 //! to initialize the class
 //! \param[in] varname Name of variable 
 //! \retval range A two-element vector containing the current 
 //! minimum and maximum or NULL if the variable is not known
 //! quantization mapping.
 //!
 //
 const float	*GetDataRange(size_t ts, const char *varname);

 
 //! Return the WaveletBlock3DRegionReader class object associated
 //! with this class instance.
 //!
 //! The WaveletBlock3DRegionReader object instance used to by GetRegion()
 //! methods to read a region from disk - when the region is not present
 //! in the memory cache - is returned. 
 // 
 const WaveletBlock3DRegionReader	*GetRegionReader() const {
	return (_wbreader);
 };

 //! Return the metadata class object associated with this class
 //!
 const  Metadata *GetMetadata() const { return (_metadata); };


private:
 int	_objInitialized;

 enum _dataTypes_t {UINT8,UINT16,UINT32,FLOAT32};

 typedef struct {
	int reflevel;
	size_t min[3];
	size_t max[3];
	_dataTypes_t	type;
	int	timestamp;
	int lock;
	void *blks;
 } region_t;

 // min and max bounds for quantization
 map <string, float *> _quantizationRangeMap;	

 map <size_t, map<string, float *> > _dataRangeMap;

 map <size_t, map<string, vector<region_t *> > > _regionsMap;

 map <void *, region_t *> _lockedRegionsMap;

 const Metadata	*_metadata;

 int	_timestamp;	// access time of most recently accessed region

 WaveletBlock3DRegionReader	*_wbreader;

 BlkMemMgr	*_blk_mem_mgr;

 void	*get_region_from_cache(
	size_t ts,
	const char *varname,
	int reflevel,
	_dataTypes_t    type,
	const size_t min[3],
	const size_t max[3],
	int lock
 );

 void    *alloc_region(
	size_t ts,
	const char *varname,
	int reflevel,
	_dataTypes_t type,
	const size_t min[3],
	const size_t max[3],
	int lock
 ); 

 int	free_region(
	size_t ts,
	const char *varname,
	int reflevel,
	_dataTypes_t type,
	const size_t min[3],
	const size_t max[3]
 );

 int	set_quantization_range(const char *varname, const float range[2]);

 void	free_all();
 void	free_var(const string &, int do_native);

 int	free_lru();

 int	_DataMgr(size_t mem_size, unsigned int nthreads);

};

};

#endif	//	_DataMgr_h_
