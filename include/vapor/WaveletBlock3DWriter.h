//
//      $Id$
//

#ifndef	_WavletBlock3DWriter_h_
#define	_WavletBlock3DWriter_h_

#include <vapor/MyBase.h>
#include "vapor/WaveletBlock3DIO.h"

namespace VAPoR {

//
//! \class WaveletBlock3DWriter
//! \brief A slab writer for VDF files
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! This class provides a low-level API for writing data volumes to
//! a VDF file. The Write methods contained within are the  most efficient
//! (both in terms of memory and performance) for writing an entire data
//! volume.
//
class VDF_API	WaveletBlock3DWriter : public WaveletBlock3DIO {

public:

 //! Constructor for the WaveletBlock3DWriter class.
 //! \param[in,out] metadata A pointer to a Metadata structure identifying the
 //! data set upon which all future operations will apply.
 //! \param[in] nthreads The number of parallel execution threads to
 //! create.
 //! \note The success or failure of this constructor can be checked
 //! with the GetErrCode() method.
 //!
 //! \sa Metadata, GetErrCode()
 //
 WaveletBlock3DWriter(
	Metadata *metadata,
	unsigned int    nthreads = 1
 );

 //! Constructor for the WaveletBlock3DWriter class.
 //! \param[in] metafile Path to a metadata file for which all
 //! future class operations will apply
 //! \param[in] nthreads The number of parallel execution threads to
 //! create.
 //! \note The success or failure of this constructor can be checked
 //! with the GetErrCode() method.
 //!
 //! \sa Metadata, GetErrCode()
 //
 WaveletBlock3DWriter(
	const char	*metafile,
	unsigned int    nthreads = 1
 );

 ~WaveletBlock3DWriter();

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
 virtual int	OpenVariableWrite(size_t timestep, const char *varname);

 virtual int	CloseVariable();

 //! Transform and write out two "slabs" of voxels to the currently opened
 //! multiresolution
 //! data volume.  Subsequent calls will write successive pairs of slabs
 //! until the entire volume has been written. The number of transforms
 //! applied is determined by the contents of the Metadata structure
 //! used to initialize this class. If zero, the slabs are not transformed
 //! and are written at their full resolution.
 //! The dimensions of a pair of slabs is NBX by NBY by 2,
 //! where NBX is the dimesion of the volume along the X axis, specified
 //! in blocks, and NBY is the Y axis dimension. The dimension of each block
 //! are given by the Metadata structure used to initialize this class.
 //! It is the caller's responsbility to pad the slabs to block boundaries.
 //!
 //! This method should be called exactly NBZ/2 times, where NBZ is the
 //! dimesion of the volume in blocks along the Z axis. Each invocation
 //! should pass a succesive pair of volume slabs.
 //! 
 //! \sa two_slabs A pair of slabs of raw data
 //! \retval status Returns a non-negative value on success
 //
 int	WriteSlabs(const float *two_slabs);

private:
 int	_objInitialized;	// has the obj successfully been initialized?

 double	writer_timer_c;
 string _metafile;

 vector <double>	_dataRange;	// min and max data values 

 int	slab_cntr_c;
 int	is_open_c;
 float	*lambda_blks_c[MAX_LEVELS];	// temp storage for lambda blocks
 float	*zero_block_c;	// a block of zero data for padding

 int	write_slabs(
	size_t	num_xforms,
	const float *two_slabs
	);

 int	write_gamma_slabs(
	int	level,
	const float *two_slabs,
	int	src_nbx,
	int	src_nby,
	float	*dst_lambda_buf,
	int	dst_nbx,
	int	dst_nby
	);

 int	my_alloc();
 void	my_free();

 void	_WaveletBlock3DWriter();

};

};

#endif	//	_WavletBlock3d_h_
