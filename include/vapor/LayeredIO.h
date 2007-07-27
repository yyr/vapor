//

//


#ifndef	LAYEREDIO_H
#define	LAYEREDIO_H

#include <cstdio>
#include <netcdf.h>
#include <vapor/MyBase.h>

#include <vapor/VDFIOBase.h>

namespace VAPoR {

class WaveletBlock3DRegionReader;
//
//! \class LayeredIO
//! \brief Performs data IO of Layered data to vdf
//! \author Alan Norton
//! \version $Revision$
//! \date    $Date$
//!
//! This class provides an API for performing low-level IO 
//! to/from VDF files.  Data is retrieved in a two-step process.  First,
//! the data is reconstructed from wavelet coefficients to a 3D array, representing
//! the data values along horizontal layers of the data.  Then the data is 
//! interpolated from the layered representation to a uniform gridded representation.
//! The height of the uniform grid can vary dynamically.
//
class VDF_API	LayeredIO : public VAPoR::VDFIOBase {

public:

 //! Constructor for the LayeredIO class.
 //! \param[in] metadata Pointer to a metadata class object for which all
 //! future class operations will apply
 //! \param[in] nthreads Number of execution threads that may be used by
 //! the class for parallel execution.
 //! \note The success or failure of this constructor can be checked
 //! with the GetErrCode() method.
 //!
 //! \sa Metadata, LayeredRegionReader, GetErrCode(),
 //
 LayeredIO(
	const Metadata *metadata,
	unsigned int	nthreads = 1
 );

 //! Constructor for the LayeredIO class.
 //! \param[in] metafile Path to a metadata file for which all
 //! future class operations will apply
 //! \param[in] nthreads Number of execution threads that may be used by
 //! the class for parallel execution.
 //! \note The success or failure of this constructor can be checked
 //! with the GetErrCode() method.
 //!
 //! \sa Metadata, LayeredRegionReader, GetErrCode(),
 //
 LayeredIO(
	const char *metafile,
	unsigned int	nthreads = 1
 );

 virtual ~LayeredIO();

 //! Returns true if indicated data volume exists on disk
 //!
 //! Returns true if the variable identified by the timestep, variable
 //! name, and refinement level is present on disk. Returns 0 if
 //! the variable is not present.
 //! \param[in] ts A valid time step from the Metadata object used
 //! to initialize the class
 //! \param[in] varname A valid variable name
 //! \param[in] reflevel Refinement level requested. The coarsest
 //! refinement level is 0 (zero). A value of -1 indicates the finest
 //! refinement level contained in the VDC.
 //
 virtual int    VariableExists(
	size_t ts,
	const char *varname,
	int reflevel = 0
 ) const;


 //! Open the named variable for writing
 //!
 //! Prepare a vapor data file for the creation of a multiresolution
 //! data volume via subsequent write operations by
 //! other methods of this classes derived from this class.
 //! The data volume is identified by the specfied time step and
 //! variable name. The number of forward transforms applied to
 //! the volume is determined by the Metadata object used to
 //! initialize the class. The number of refinement levels actually 
 //! saved to the data collection are determined by \p reflevels. If
 //! \p reflevels is zero, the default, only the coarsest approximation is
 //! saved. If \p reflevels is one, all the coarsest and first refinement 
 //! level is saved, and so on. A value of -1 indicates the maximum
 //! refinment level permitted by the VDF
 //!
 //! \param[in] timestep Time step of the variable to read
 //! \param[in] varname Name of the variable to read
 //! \param[in] reflevel Refinement level of the variable. A value of -1
 //! indicates the maximum refinment level.
 //! \retval status Returns a non-negative value on success
 //! \sa Metadata::GetVariableNames(), Metadata::GetNumTransforms()
 //!
 virtual int	OpenVariableWrite(
	size_t timestep,
	const char *varname,
	int reflevel = -1
 );

 //! Open the named variable for reading
 //!
 //! This method prepares the multiresolution data volume, indicated by a
 //! variable name and time step pair, for subsequent read operations by
 //! methods of this class.  Furthermore, the number of the refinement level
 //! parameter, \p reflevel indicates the resolution of the volume in
 //! the multiresolution hierarchy. The valid range of values for
 //! \p reflevel is [0..max_refinement], where \p max_refinement is the
 //! maximum finement level of the data set: Metadata::GetNumTransforms() - 1.
 //! volume when the volume was created. A value of zero indicates the
 //! coarsest resolution data, a value of \p max_refinement indicates the
 //! finest resolution data.
 //!
 //! An error occurs, indicated by a negative return value, if the
 //! volume identified by the {varname, timestep, reflevel} tripple
 //! is not present on disk. Note the presence of a volume can be tested
 //! for with the VariableExists() method.
 //! \param[in] timestep Time step of the variable to read
 //! \param[in] varname Name of the variable to read
 //! \param[in] reflevel Refinement level of the variable. A value of -1
 //! indicates the maximum refinment level defined for the VDC
 //! \param[in] full_height indicates the grid height for interpolation.
 //! Use full_height = 0 when opening just to obtain data range.
 //! \retval status Returns a non-negative value on success
 //! \sa Metadata::GetVariableNames(), Metadata::GetNumTransforms()
 //!
 virtual int	OpenVariableRead(
	size_t timestep,
	const char *varname,
	int reflevel = 0,
	size_t full_height = 0
 );

 //! Close the currently opened variable.
 //!
 //! \sa OpenVariableWrite(), OpenVariableRead()
 //
 virtual int	CloseVariable();


 //! Return the transform timer
 //!
 //! This method returns the accumulated clock time, in seconds,
 //! spent peforming wavelet transforms. There is presently no
 //! way to reset the counter (without creating a new class object)
 //!
 double	GetXFormTimer() const { return(_xform_timer); };
//! Return the WaveletBlock3DRegionReader that is used by this LayeredIO
//! to read the associated wavelet block data
//
 WaveletBlock3DRegionReader* GetWBReader() { return wbRegionReader;}
//! Performs an interpolation, converting layered data to uniformly gridded data.
//! \param[out] blks returns a blocked region of interpolated data 
//! \param[in] elevblocks is the blocked region of elevation data, representing
//! the elevation associated with each point on a layer
//! \param[in] wbblocks is the blocked region of layered data, 
//! of the desired variable in its WB representation.
//! Note that both elevblocks and weblocks are currently required to be available
//! at the full vertical (Z) extent of the volume.
//! \param[in] minblks specifies the output region min block coordinates
//! \param[in] maxblks specifies the output region max block coordinates
//!
void InterpolateRegion(float* blks, const float* elevblocks, const float* wbblocks, 
	 const size_t minblks[3], const size_t maxblks[3], 
	 size_t full_height,
	 float lowValue, float highValue);
//!
//!  Specify the interpolation height to be used for grid interpolation.
//!  This is necessary if the grid height has been modified and new region extents
//!  are to be calculated before a variable is opened.
//!  \param[in] full_height is the full number of voxels to be used in
//!  interpolating the data at the highest refinement level.
//!
void SetGridHeight(size_t full_height);
protected:
 static const int MAX_LEVELS = 16;	// Max # of forward transforms permitted


 int	_reflevel;		// refinement level of currently opened file.
 size_t _timeStep;		// Currently opened timestep
 string _varName;		// Currently opened variable

 double	_xform_timer;	// records interpolation time 


 VAPoR::WaveletBlock3DRegionReader	*wbRegionReader;


private:
 int	_objInitialized;	// has the obj successfully been initialized?

 typedef int int32_t;

 int	type_c;		// data type;
 int	is_open_c;	// true if a file is open


 int	_LayeredIO();

};

}

#endif	//	_LayeredIO_h_
