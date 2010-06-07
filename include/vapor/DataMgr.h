//
//      $Id$
//

#ifndef	_DataMgr_h_
#define	_DataMgr_h_


#include <list>
#include <vapor/MyBase.h>
#include "vapor/BlkMemMgr.h"
#include "vapor/Metadata.h"
#include "vaporinternal/common.h"

namespace VAPoR {

//
//! \class DataMgr
//! \brief A cache based data reader
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! This class provides a wrapper to the WaveletBlock3DRegionReader()
//! and WaveletBlock2DRegionReader()
//! classes that includes a memory cache. Data regions read from disk through 
//! this
//! interface are stored in a cache in main memory, where they may be
//! be available for future access without reading from disk.
//
class VDF_API DataMgr : public Metadata, public VetsUtil::MyBase {

public:

 //! Constructor for the DataMgr class. 
 //!
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
	size_t mem_size
 );

 virtual ~DataMgr(); 

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
 //! \note The \p lock parameter increments a counter associated 
 //! with the requested region of memory. The counter is decremented
 //! when UnlockRegion() is invoked.
 //!
 //! \param[in] ts A valid time step from the Metadata object used 
 //! to initialize the class
 //! \param[in] varname A valid variable name 
 //! \param[in] reflevel Refinement level requested
 //! \param[in] lod Level of detail requested
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
    int lod,
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
 //! \param[in] reflevel Refinement level requested
 //! \param[in] lod Level of detail requested
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
    int lod,
    const size_t min[3],
    const size_t max[3],
	const float range[2],
    int lock = 0
);

 //! Read in, quantize and return a pair of subregions from the 
 //! multiresolution dataset
 //!
 //! This method is identical to the GetRegionUInt8() method except that the
 //! two variables are read and their values are stored in a single,
 //! interleaved array.
 //!
 //! \param[in] ts A valid time step from the Metadata object used 
 //! to initialize the class
 //! \param[in] varname1 First variable name 
 //! \param[in] varname2 Second variable name 
 //! \param[in] reflevel Refinement level requested
 //! \param[in] lod Level of detail requested
 //! \param[in] min Minimum region bounds in blocks
 //! \param[in] max Maximum region bounds in blocks
 //! \param[in] range1 First variable data range
 //! \param[in] range2 Second variable data range
 //! quantization mapping. 
 //! \param[in] lock If true, the memory region will be locked into the 
 //! \retval ptr A pointer to a region containing the desired data, 
 //! quantized to 8 bits, or NULL
 //! if the region can not be extracted.
 //! \sa WaveletBlock3DRegionReader, GetErrMsg(), GetRegion()
 //
 unsigned char   *GetRegionUInt8(
    size_t ts,
    const char *varname1,
    const char *varname2,
    int reflevel,
    int lod,
    const size_t min[3],
    const size_t max[3],
	const float range1[2],
	const float range2[2],
    int lock = 0
);

 //! Read in, quantize and return a pair of subregions from the 
 //! multiresolution dataset
 //!
 //! This method is identical to the GetRegionUInt16() method except that the
 //! two variables are read and their values are stored in a single,
 //! interleaved array.
 //!
 //! \param[in] ts A valid time step from the Metadata object used 
 //! to initialize the class
 //! \param[in] varname1 First variable name 
 //! \param[in] varname2 Second variable name 
 //! \param[in] reflevel Refinement level requested
 //! \param[in] lod Level of detail requested
 //! \param[in] min Minimum region bounds in blocks
 //! \param[in] max Maximum region bounds in blocks
 //! \param[in] range1 First variable data range
 //! \param[in] range2 Second variable data range
 //! quantization mapping. 
 //! \param[in] lock If true, the memory region will be locked into the 
 //! \retval ptr A pointer to a region containing the desired data, 
 //! quantized to 16 bits, or NULL
 //! if the region can not be extracted.
 //! \sa WaveletBlock3DRegionReader, GetErrMsg(), GetRegion()
 //
 unsigned char   *GetRegionUInt16(
    size_t ts,
    const char *varname1,
    const char *varname2,
    int reflevel,
    int lod,
    const size_t min[3],
    const size_t max[3],
	const float range1[2],
	const float range2[2],
    int lock = 0
);


 //! Read in, quantize and return a subregion from the multiresolution dataset
 //!
 //! This method is identical to the GetRegion() method except that the
 //! data are returned as quantized, 16-bit unsigned integers. 
 //! Regions with integer data types are created by quantizing
 //! native floating point representations such that floating values
 //! less than or equal to \p range[0] are mapped to min, and values 
 //! greater than or equal to \p range[1] are mapped to max, where "min" and
 //! "max" are the minimum and maximum values that may be represented 
 //! by the integer type. For example, for 16-bit, unsigned ints min is 0
 //! and max is 65535. Floating point values between \p range[0] and \p range[1]
 //! are linearly interpolated between min and max.
 //!
 //! \param[in] ts A valid time step from the Metadata object used 
 //! to initialize the class
 //! \param[in] varname A valid variable name 
 //! \param[in] reflevel Refinement level requested
 //! \param[in] lod Level of detail requested
 //! \param[in] min Minimum region bounds in blocks
 //! \param[in] max Maximum region bounds in blocks
 //! \param[in] range A two-element vector specifying the minimum and maximum
 //! quantization mapping. 
 //! \param[in] lock If true, the memory region will be locked into the 
 //! \retval ptr A pointer to a region containing the desired data, 
 //! quantized to 16 bits, or NULL
 //! if the region can not be extracted.
 //! \sa WaveletBlock3DRegionReader, GetErrMsg(), GetRegion()
 //
 unsigned char   *GetRegionUInt16(
    size_t ts,
    const char *varname,
    int reflevel,
    int lod,
    const size_t min[3],
    const size_t max[3],
	const float range[2],
    int lock = 0
);


 //! Unlock a floating-point region of memory 
 //!
 //! Decrement the lock counter associatd with a floating-point 
 //! region of memory, and if zero,
 //! unlock region of memory previously locked GetRegion(). 
 //! When the lock counter reaches zero the region is simply 
 //! marked available for
 //! internal garbage collection during subsequent GetRegion() calls
 //!
 //! \param[in] region A pointer to a region of memory previosly 
 //! returned by GetRegion()
 //! \retval status Returns a non-negative value on success
 //!
 //! \sa GetRegion(), GetRegion()
 //
 int	UnlockRegion (
    void *region
 );

 //! Return the current data range as a two-element array
 //!
 //! This method returns the minimum and maximum data values
 //! for the indicated time step and variable
 //!
 //! \note The range values returned are valid for the native
 //! data only. Data approximations produced by level-of-detail or 
 //! through multi-resolution may have values outside of this range.
 //!
 //! \param[in] ts A valid time step from the Metadata object used 
 //! to initialize the class
 //! \param[in] varname Name of variable 
 //! \param[out] range  A two-element vector containing the current 
 //! minimum and maximum.
 //! \retval status Returns a non-negative value on success 
 //! quantization mapping.
 //!
 //
 int GetDataRange(size_t ts, const char *varname, float range[2]);

 //! Return the valid region bounds for the specified region
 //!
 //! This method returns the minimum and maximum valid coordinate
 //! bounds (in voxels) of the subregion indicated by the timestep
 //! \p ts, variable name \p varname, and refinement level \p reflevel
 //! for the indicated time step and variable. Data are guaranteed to
 //! be available for this region.
 //!
 //!
 //! \param[in] ts A valid time step from the Metadata object used 
 //! to initialize the class
 //! \param[in] varname Name of variable 
 //! \param[in] reflevel Refinement level of the variable
 //! \param[out] min Minimum coordinate bounds (in voxels) of volume
 //! \param[out] max Maximum coordinate bounds (in voxels) of volume
 //! \retval status A non-negative int is returned on success
 //!
 //
 int GetValidRegion(
    size_t ts,
    const char *varname,
    int reflevel,
    size_t min[3],
    size_t max[3]
 );

 //! Clear the memory cache
 //!
 //! This method clears the internal memory cache of all entries
 //
 void	Clear();

 //! Returns true if indicated data volume is available
 //!
 //! Returns true if the variable identified by the timestep, variable
 //! name, refinement level, and level-of-detail is present in 
 //! the data set. Returns 0 if
 //! the variable is not present.
 //! \param[in] ts A valid time step from the Metadata object used
 //! to initialize the class
 //! \param[in] varname A valid variable name
 //! \param[in] reflevel Refinement level requested. The coarsest 
 //! refinement level is 0 (zero). A value of -1 indicates the finest
 //! refinement level contained in the VDC.
 //! \param[in] lod Compression level of detail requested. The coarsest 
 //! approximation level is 0 (zero). A value of -1 indicates the finest
 //! refinement level contained in the VDC.
 //
 virtual int VariableExists(
	size_t ts,
	const char *varname,
	int reflevel = 0,
	int lod = 0
 ) const = 0;

protected:

 // The protected methods below are pure virtual and must be implemented by any 
 // child class  of the DataMgr.


 //! Open the named variable for reading
 //!
 //! This method prepares the multi-resolution, multi-lod data volume, 
 //! indicated by a
 //! variable name and time step pair, for subsequent read operations by
 //! methods of this class.  Furthermore, the number of the refinement level
 //! parameter, \p reflevel indicates the resolution of the volume in
 //! the multiresolution hierarchy, and the \p lod parameter indicates
 //! the level of detail. 
 //!
 //! The valid range of values for
 //! \p reflevel is [0..max_refinement], where \p max_refinement is the
 //! maximum finement level of the data set: Metadata::GetNumTransforms().
 //! A value of zero indicates the
 //! coarsest resolution data, a value of \p max_refinement indicates the
 //! finest resolution data.
 //!
 //! The valid range of values for
 //! \p lod is [0..max_lod], where \p max_lod is the
 //! maximum lod of the data set: Metadata::GetCRatios().size() - 1.
 //! A value of zero indicates the
 //! highest compression ratio, a value of \p max_lod indicates the
 //! lowest compression ratio.
 //!
 //! An error occurs, indicated by a negative return value, if the
 //! volume identified by the {varname, timestep, reflevel, lod} tupple
 //! is not present on disk. Note the presence of a volume can be tested
 //! for with the VariableExists() method.
 //! \param[in] timestep Time step of the variable to read
 //! \param[in] varname Name of the variable to read
 //! \param[in] reflevel Refinement level of the variable. A value of -1
 //! indicates the maximum refinment level defined for the VDC
 //! \param[in] lod Level of detail requested. A value of -1
 //! indicates the lowest compression level available for the VDC
 //!
 //! \sa Metadata::GetVariableNames(), Metadata::GetNumTransforms()
 //!
 virtual int	OpenVariableRead(
	size_t timestep,
	const char *varname,
	int reflevel = 0,
	int lod = 0
 ) = 0;

 //! Close the currently opened variable.
 //!
 //! \sa OpenVariableWrite(), OpenVariableRead()
 //
 virtual int	CloseVariable() = 0;

 //! Read in and return a subregion from the currently opened multiresolution
 //! data volume.
 //!
 //! The dimensions of the region are provided in block coordinates. However,
 //! the returned region is not blocked. 
 //!
 //! \param[in] bmin Minimum region extents in block coordinates
 //! \param[in] bmax Maximum region extents in block coordinates
 //! \param[out] region The requested volume subregion
 //!
 //! \retval status Returns a non-negative value on success
 //! \sa OpenVariableRead(), Metadata::GetBlockSize(), MapVoxToBlk()
 //
 virtual int    BlockReadRegion(
    const size_t bmin[3], const size_t bmax[3],
    float *region
 ) = 0;



 //! Return the valid bounds of the currently opened region
 //!
 //! The data model permits the storage of volume subregions. This method may
 //! be used to query the valid domain of the currently opened volume. Results
 //! are returned in voxel coordinates, relative to the refinement level
 //! indicated by \p reflevel.
 //!
 //!
 //! \param[out] min A pointer to the minimum bounds of the subvolume
 //! \param[out] max A pointer to the maximum bounds of the subvolume
 //! \param[in] reflevel Refinement level of the variable. A value of -1
 //! indicates the maximum refinment level defined for the VDC
 //!
 //! \retval status Returns a negative value if the volume is not opened
 //! for reading.
 //!
 //! \sa OpenVariableWrite(), OpenVariableRead()
 //
 virtual void GetValidRegion(
    size_t min[3], size_t max[3], int reflevel
 ) const = 0;

 //! Return the data range for the currently open variable
 //!
 //! The method returns the minimum and maximum data values, respectively, 
 //! for the variable currently opened. If the variable is not opened,
 //! or if it is opened for writing, the results are undefined.
 //!
 //! \param[out] range Min and Max data values, respectively
 //!
 //! \retval status A negative integer is returned on failure, otherwise
 //! the method has succeeded.
 //!
 virtual const float *GetDataRange() const = 0;

private:

 size_t _mem_size;

 enum _dataTypes_t {UINT8,UINT16,UINT32,FLOAT32};

 typedef struct {
	size_t ts;
	string varname;
	int reflevel;
	int lod;
	size_t min[3];
	size_t max[3];
	_dataTypes_t	type;
	int lock_counter;
	void *blks;
 } region_t;

 // a list of all allocated regions
 list <region_t> _regionsList;

 // min and max bounds for quantization
 map <string, float *> _quantizationRangeMap;	

 map <size_t, map<string, float> > _dataRangeMinMap;
 map <size_t, map<string, float> > _dataRangeMaxMap;
 map <size_t, map<string, map<int, size_t *> > > _validRegMinMaxMap;

 int	_timestamp;	// access time of most recently accessed region

 BlkMemMgr	*_blk_mem_mgr;

 void	*get_region_from_cache(
	size_t ts,
	const char *varname,
	int reflevel,
	int lod,
	_dataTypes_t    type,
	const size_t min[3],
	const size_t max[3],
	int lock
 );

 void    *alloc_region(
	size_t ts,
	const char *varname,
	VarType_T vtype,
	int reflevel,
	int lod,
	_dataTypes_t type,
	const size_t min[3],
	const size_t max[3],
	int lock
 ); 

 int	free_region(
	size_t ts,
	const char *varname,
	int reflevel,
	int lod,
	_dataTypes_t type,
	const size_t min[3],
	const size_t max[3]
 );

 int	set_quantization_range(const char *varname, const float range[2]);

 void   setDefaultHighLowVals();
 void	free_var(const string &, int do_native);

 int	free_lru();

 int	_DataMgr(size_t mem_size);

 int get_cached_data_range(size_t ts, const char *varname, float range[2]);

 size_t *get_cached_reg_min_max(
	size_t ts, const char *varname, int reflevel
 );

 unsigned char   *get_quantized_region(
	size_t ts, const char *varname, int reflevel, int lod, const size_t min[3],
	const size_t max[3], const float range[2], int lock,
	_dataTypes_t type
 );

 void	quantize_region_uint8(
    const float *fptr, unsigned char *ucptr, size_t size, const float range[2]
 );

 void	quantize_region_uint16(
    const float *fptr, unsigned char *ucptr, size_t size, const float range[2]
 );



};

};

#endif	//	_DataMgr_h_
