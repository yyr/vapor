//
//      $Id$
//


#ifndef	_WavletBlock3DIO_h_
#define	_WavletBlock3DIO_h_

#include <cstdio>
#include <vapor/MyBase.h>
#include "WaveletBlock3D.h"

namespace VAPoR {


#if	defined(IRIX) || defined(LINUX)
#define	WAVELET_BLOCK_TIMER
#endif

#ifdef	WAVELET_BLOCK_TIMER
#include <time.h>
#include <assert.h>


double  inline gettime() {
        struct timespec ts;
        double  t;

        assert(clock_gettime(CLOCK_REALTIME, &ts) >= 0);


        t = (double) ts.tv_sec + (double) ts.tv_nsec*1.0e-9;
        return(t);
}

#define	TIMER_START(T0)		double (T0) = VAPoR::gettime();
#define	TIMER_STOP(T0, T1)	(T1) += VAPoR::gettime() - (T0);

#else

#define	TIMER_START(T0)
#define	TIMER_STOP(T0, T1)

#endif


//
//! \class WaveletBlock3DIO
//! \brief Performs data IO to VDF files.
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! This class provides an API for performing low-level IO 
//! to/from VDF files
//
class VDF_API	WaveletBlock3DIO : public VetsUtil::MyBase {

public:

 //! Constructor for the WaveletBlock3DIO class.
 //! \param[in] metadata Pointer to a metadata class object for which all
 //! future class operations will apply
 //! \param[in] nthreads Number of execution threads that may be used by
 //! the class for parallel execution.
 //! \note The success or failure of this constructor can be checked
 //! with the GetErrCode() method.
 //!
 //! \sa Metadata, WaveletBlock3DRegionReader, GetErrCode(),
 //
 WaveletBlock3DIO(
	Metadata *metadata,
	unsigned int	nthreads
 );

 //! Constructor for the WaveletBlock3DIO class.
 //! \param[in] metafile Path to a metadata file for which all
 //! future class operations will apply
 //! \param[in] nthreads Number of execution threads that may be used by
 //! the class for parallel execution.
 //! \note The success or failure of this constructor can be checked
 //! with the GetErrCode() method.
 //!
 //! \sa Metadata, WaveletBlock3DRegionReader, GetErrCode(),
 //
 WaveletBlock3DIO(
	const char *metafile,
	unsigned int	nthreads
 );

 virtual ~WaveletBlock3DIO();

 //! Returns true if indicated data volume exists on disk
 //!
 //! Returns true if the variable identified by the timestep, variable
 //! name, and number of transforms is present on disk. Returns 0 if
 //! the variable is not present.
 //! \param[in] ts A valid time step from the Metadata object used
 //! to initialize the class
 //! \param[in] varname A valid variable name
 //! \param[in] num_xforms Transformation number requested
 //
 int	VariableExists(
	size_t ts,
	const char *varname,
	size_t num_xforms
 );

 //! Open the named variable for writing
 //!
 //! Prepare a vapor data file for the creation of a multiresolution
 //! data volume via subsequent write operations by
 //! other methods of this class.
 //! The data volume is identified by the specfied time step and
 //! variable name. The number of forward transforms applied to
 //! the volume is determined by the Metadata object used to
 //! initialize the class.
 //!
 //! \param[in] timestep Time step of the variable to read
 //! \param[in] varname Name of the variable to read
 //! \retval status Returns a non-negative value on success
 //! \sa Metadata::GetVariableNames(), Metadata::GetNumTransforms()
 //!
 int	OpenVariableWrite(
	size_t timestep,
	const char *varname
 );

 //! Open the named variable for reading
 //!
 //! This method prepares the multiresolution data volume indicated by a
 //! variable name and time step pair, for subsequent read operations by
 //! methods of this class.  Furthermore, the number of forward transforms
 //! parameter, \p num_xforms indicates the resolution of the volume in
 //! the multiresolution hierarchy. The valid range of values for
 //! \p num_xforms is [0..max_xforms], where \p max_xforms is the
 //! maximum number of forward transforms applied to the multiresolution
 //! volume when the volume was created. A value of zero indicates the
 //! finest resolution data, a value of \p max_xforms indicates the
 //! coarsest resolution data.
 //!
 //! An error occurs, indicated by a negative return value, if the
 //! volume identified by the {varname, timestep, num_xforms} tripple
 //! is not present on disk. Note the presence of a volume can be tested
 //! for with the VariableExists() method.
 //! \param[in] timestep Time step of the variable to read
 //! \param[in] varname Name of the variable to read
 //! \param[in] num_xforms Transformation level of the variable
 //! \retval status Returns a non-negative value on success
 //! \sa Metadata::GetVariableNames(), Metadata::GetNumTransforms()
 //!
 int	OpenVariableRead(
	size_t timestep,
	const char *varname,
	size_t num_xforms
 );

 //! Close the currently opened variable.
 //!
 //! \sa OpenVariableWrite(), OpenVariableRead()
 //
 int	CloseVariable();

 int	GetBlockMins(unsigned int level, const float **mins);
 int	GetBlockMaxs(unsigned int level, const float **maxs);

 //! Get the dimension of a volume
 //! 
 //! Get the resulting dimension of the volume
 //! after undergoing a specified number of forward transforms. 
 //! If the number of transforms is zero, the
 //! value returned is the native volume dimension as specified in the
 //! Metadata structure used to construct the class.
 //! \param[in] num_xforms Transformation level of the variable
 //! \param[out] dim Transformed dimension.
 //!
 //! \sa Metadata::GetDimension()
 //
 void	GetDim(size_t num_xforms, size_t dim[3]) const;

 //! Get dimesion of a volume in blocks
 //!
 //! Performs same operation as GetDim() except returns
 //! dimensions in block coordinates instead of voxels.
 //! \param[in] num_xforms Transformation level of the variable
 //! \param[out] bdim Transformed dimension in blocks.
 //!
 //! \sa Metadata::GetDimension()
 //
 void	GetDimBlk(size_t num_xforms, size_t bdim[3]) const;

 //! Compute coordinates of a transformed voxel 
 //!
 //! Compute the resulting coordinates of a voxel after it undergoes
 //! a specified number of forward transforms, \p num_xforms.
 //! If num_xforms is zero, the operation performed is
 //! the identity  function.
 //! \note The coordinates returned may be 
 //! outside the 
 //! volume boundary and should always be checked to ensure that
 //! they are within the volume boundary with 
 //! the GetDim() method.
 //! \param[in] num_xforms Transformation level of the variable
 //! \param[in] vcoord0 Coordinate of input voxel in integer (voxel) 
 //! coordinates
 //! \param[out] vcoord1 Coordinate of transformed voxel in integer (voxel) 
 //! coordinates
 //
 void	TransformCoord(
	size_t num_xforms, const size_t vcoord0[3], size_t vcoord1[3]
 ) const;

 //! Map integer voxel coordinates into integer block coordinates. 
 //! 
 //! Compute the integer coordinate of the block containing
 //! a specified voxel. 
 //! \param[in] vcoord Coordinate of input voxel in integer (voxel)
 //! coordinates
 //! \param[out] bcoord Coordinate of block in integer coordinates containing
 //! the voxel. 
 //!
 void	MapVoxToBlk(const size_t vcoord[3], size_t bcoord[3]) const;
		

 //! Map integer voxel coordinates to user-defined floating point coords.
 //!
 //! Map the integer coordinates of the specified voxel to floating
 //! point coordinates in a user defined space. The voxel coordinates,
 //! \p vcoord0 are specified relative to the number of transforms 
 //! indicated by \p num_xforms for time step \p timestep.  
 //! I.e. vcoord0 are the integer offsets
 //! of a voxel from a coarsened volume that has undergone \p num_xform
 //! forward transforms.
 //! The mapping is performed by using linear interpolation 
 //! The user-defined coordinate system is obtained
 //! from the Metadata structure passed to the class constructor.
 //! The user coordinates are returned in \p vcoord1.
 //! Results are undefined if vcoord is outside of the volume 
 //! boundary.
 //!
 //! \param[in] num_xforms Transformation level of the variable
 //! \param[in] timestep Time step of the variable 
 //! \param[in] vcoord0 Coordinate of input voxel in integer (voxel)
 //! coordinates
 //! \param[out] vcoord1 Coordinate of transformed voxel in user-defined,
 //! floating point  coordinates
 //!
 //! \sa Metatdata::GetGridType(), Metadata::GetExtents(), 
 //! GetTSXCoords()
 //
 void	MapVoxToUser(
	size_t num_xforms, size_t timestep,
	const size_t vcoord0[3], double vcoord1[3]
 ) const;

 //! Map floating point voxel coordinates to integer offsets.
 //!
 //! Map floating point coordinates, specified relative to a 
 //! user-defined coordinate system, to the closest integer voxel 
 //! coordinates for a voxel at a given transformation level. 
 //! The integer voxel coordinates, \p vcoord1
 //! are specified relative to the number of transforms 
 //! indicated by \p num_xforms for time step, \p timestep.
 //! I.e. \p vcoord1 are the integer offsets
 //! of a voxel from a coarsened volume that has undergone \p num_xform 
 //! forward transforms.
 //! The mapping is performed by using linear interpolation 
 //! The user defined coordinate system is obtained
 //! from the Metadata structure passed to the class constructor.
 //! The user coordinates are returned in \p vcoord1.
 //! Results are undefined if \p vcoord0 is outside of the volume 
 //! boundary.
 //!
 //! \param[in] num_xforms Transformation level of the variable
 //! \param[in] timestep Time step of the variable 
 //! \param[in] vcoord0 Coordinate of input voxel in floating point
 //! coordinates
 //! \param[out] vcoord1 Coordinate of closes, transformed voxel in i
 //! integer coordinates
 //!
 //! \sa Metatdata::GetGridType(), Metadata::GetExtents(), 
 //! GetTSXCoords()
 //
 void	MapUserToVox(
	size_t num_xforms, size_t timestep,
	const double vcoord0[3], size_t vcoord1[3]
 ) const;

 //! Return true if indicated region coordinates are valid
 //!
 //! Returns true if the region defined by \p num_xforms, 
 //! \p min, \p max is valid. I.e. returns true if the indicated 
 //! volume subregion is contained within the volume.
 //! 
 //! \param[in] num_xforms Transformation level of the variable
 //! \param[in] min Minimum region extents in voxel coordinates
 //! \param[in] max Maximum region extents in voxel coordinates
 //! \retval boolean True if region is valid
 //
 int IsValidRegion(
	size_t num_xforms, const size_t min[3], const size_t max[3]
 ) const;

 //! Return true if indicated region coordinates are valid
 //!
 //! Returns true if the region defined by \p num_xforms, and the block
 //! coordinates  
 //! \p min, \p max are valid. I.e. returns true if the indicated 
 //! volume subregion is contained within the volume.
 //! 
 //! \param[in] num_xforms Transformation level of the variable
 //! \param[in] min Minimum region extents in block coordinates
 //! \param[in] max Maximum region extents in block coordinates
 //! \retval boolean True if region is valid
 //
 int IsValidRegionBlk(
	size_t num_xforms, const size_t min[3], const size_t max[3]
 ) const;

 //! Unpack a block into a contiguous volume
 //!
 //! Unblock the block \p blk into a volume pointed to by \p voxels
 //! \param[in] blk A block of voxels
 //! \param[in] bcoord Offset of the start of the block within the 
 //! volume in integer coordinates
 //! \param[in] dim Dimension of the volume in voxels
 //! \param[out] voxels A pointer to a volume
 //
 void	Block2NonBlock(
		const float *blk, 
		const size_t bcoord[3],
		const size_t dim[3],
		float	*voxels
	) const;

 //! Return the metadata class object associated with this class
 //!
 const	Metadata *GetMetadata() const { return (metadata_c); };

 double	GetReadTimer() const { return(read_timer_c); };
 double	GetSeekTimer() const { return(seek_timer_c); };
 double	GetWriteTimer() const { return(write_timer_c); };
 double	GetXFormTimer() const { return(xform_timer_c); };


protected:
 static const int MAX_LEVELS = 16;	// Max # of forward transforms permitted
 VAPoR::Metadata *metadata_c;
 size_t	dim_c[3];		// volume dimensions in voxels (finest resolution)
 size_t	bdim_c[3];		// volume dimensions in blocks (finest resolution)
 size_t xdim_c[3];		// vol dim in voxels after 'num_xforms' forward
						// transforms

 size_t xbdim_c[3];		// vol dim in blocks after 'num_xforms' forward
						// transforms

 int	bs_c;			// block dimensions in voxels
 int	block_size_c;	// block size in voxels

 int	max_xforms_c;	// # of forward transforms applied/to-apply
						// 0 => none, 1 => one coarsening, etc.
						//

 int	num_xforms_c;	// Minimum transform number requested
						// num_xforms_c == 0 => finest.
						// num_xforms_c == max_xforms_c => coarsest.
						// For writes, num_xforms_c == 0
						// For reads, 0 <= num_xforms_c <= max_xforms_c.
						
 float	*super_block_c;		// temp storage for gamma blocks;

 double	xform_timer_c;	// records transform time by derived classes

 size_t _timeStep;		// Currently opened timestep
 string _varName;		// Currently opened variable

 VAPoR::WaveletBlock3D	*wb3d_c;


 // This method moves the file pointer associated with the currently
 // open variable to the disk block indicated by 'offset' and
 // 'num_xforms', where 'offset' indicates the desired position, in 
 // blocks, and 'num_xforms' indicates the number of 
 // forward transformations. If 'num_xforms' is zero, for example, 
 // the file pointer is moved 'offset' blocks past the beginning
 // of the coefficients associated with the finest data resolution.
 //
 int	seekBlocks(unsigned int offset, size_t num_xforms);

 // This method moves the file pointer associated with the currently
 // open variable to the disk block containing lambda coefficients
 // indicated by the block coordinates 'bcoord'.
 //
 // A non-negative return value indicates success
 //
 int	seekLambdaBlocks(const size_t bcoord[3]);

 // This method moves the file pointer associated with the currently
 // open variable to the disk block containing gamma coefficients
 // indicated by the block coordinates 'bcoord', and the number of
 // forward transformations, 'num_xforms'. The parameter 'num_xforms'
 // must be in the range [0..max_xforms_c-1]. Note, if max_xforms_c is 
 // zero, an error is generated as there are no gamma coefficients.
 //
 // A non-negative return value indicates success
 //
 int	seekGammaBlocks(size_t num_xforms, const size_t bcoord[3]);

 // Read 'n' contiguous coefficient blocks, associated with the indicated
 // number of transforms, 'num_xforms', from the currently open variable
 // file. The 'num_xforms' parameter must be in the range [0..max_xforms_c],
 // where a value of zero indicates the coefficients for the
 // finest resolution. The results are stored in 'blks', which must 
 // point to an area of adequately sized memory.
 //
 int	readBlocks(size_t n, size_t num_xforms, float *blks);

 //
 // Read 'n' contiguous lambda coefficient blocks
 // from the currently open variable file. 
 // The results are stored in 'blks', which must 
 // point to an area of adequately sized memory.
 //
 int	readLambdaBlocks(size_t n, float *blks);

 //
 // Read 'n' contiguous gamma coefficient blocks, associated with
 // the indicated number of transforms, 'num_xforms',
 // from the currently open variable file. 
 // The results are stored in 'blks', which must 
 // point to an area of adequately sized memory.
 // An error is generated if max_xforms_c is less than one or
 // 'num_xforms' is greater than max_xforms_c.
 //
 int	readGammaBlocks(size_t n, size_t num_xforms, float *blks);

 // Write 'n' contiguous coefficient blocks, associated with the indicated
 // number of transforms, 'num_xforms', from the currently open variable
 // file. The 'num_xforms' parameter must be in the range [0..max_xforms_c],
 // where a value of zero indicates the coefficients for the
 // finest resolution. The coefficients are copied from the memory area
 // pointed to by 'blks'
 //
 int	writeBlocks(const float *blks, size_t n, size_t num_xforms);

 // Write 'n' contiguous lambda coefficient blocks
 // from the currently open variable file. 
 // The blocks are copied to disk from the memory area pointed to 
 // by  'blks'.
 //
 int	writeLambdaBlocks(const float *blks, size_t n);

 // Write 'n' contiguous gamma coefficient blocks, associated with
 // the indicated number of transforms, 'num_xforms',
 // from the currently open variable file. 
 // The data are copied from the area of memory pointed to by 'blks'.
 // An error is generated if max_xforms_c is less than one or
 // 'num_xforms' is greater than max_xforms_c.
 //
 int	writeGammaBlocks(const float *blks, size_t n, size_t num_xforms);



private:
 static const int VERSION  = 2;

 typedef int int32_t;

 FILE	*file_ptrs_c[MAX_LEVELS+1];
 float	*mins_c[MAX_LEVELS];	// min value contained in a block
 float	*maxs_c[MAX_LEVELS];	// max value contained in a block

 double	read_timer_c;
 double	write_timer_c;
 double	seek_timer_c;

 int	version_c;
 int	n_c;		// # filter coefficients
 int	ntilde_c;	// # lifting coefficients
 int	nthreads_c;	// # execution threads
 int	type_c;		// data type;
 int	is_open_c;	// true if a file is open
 int	write_mode_c;	// true if file opened for writing
 float	*block_c;		// temp storage for byteswapping blocks;
 int	_doFreeMeta;	// does the class own the metadata object?
 char	*_metafileDirName; // path to metafile parent directory

 void	swapbytes(void *vptr, int n) const;
 void	swapbytescopy(const void *src, void *dst, int size, int n) const;
 int	verify(int a, int b);
 int	my_alloc();
 void	my_free();

 void	_WaveletBlock3DIO(
	const VAPoR::Metadata *metadata,
	unsigned int	nthreads
 );

};

}

#endif	//	_WavletBlock3d_h_
