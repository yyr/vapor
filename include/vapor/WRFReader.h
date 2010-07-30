//
// $Id$
//
#ifndef	_WRFReader_h_
#define	_WRFReader_h_

#include <vapor/MetadataWRF.h>

namespace VAPoR {

//
//! \class WRFReader
//! \brief A reader for a WRF data set stored in a collection netCDF files.
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! This class provides a set of methods for reading 2D (2D + time) 
//! and 3D (3D + time) variables from
//! a WRF data set stored in a series of netCDF files. In general a "data
//! set" implies that the series of netCDF files was generated from a
//! single WRF simulation. I.e. the files contain the same variables, 
//! computed on the same grid, with the same projection, etc., and each
//! file contains a range of time steps that do no overlap with any
//! other file in the data set.
//!
//!
//
class VDF_API WRFReader : public MetadataWRF {
public:

 //!
 //! Create a WRFReader object from a MetatdaWRF object
 //!
 //! \param[in] metadata A MetadataWRF object that has already
 //!  been initialized with a WRF data set.
 //!
 WRFReader(const MetadataWRF &metadata);

 //!
 //! Create a WRFReader object from a list of netCDF files comprising
 //! a single WRF data set. 
 //!
 //! \param[in] metadata A MetadataWRF object that has already
 //!  been initialized with a WRF data set.
 //!
 WRFReader(const vector<string> &infiles);

 virtual ~WRFReader();

 //! Open the WRF data set for reading the named variable 
 //!
 //! Prepare the WRF data set for reading the 2D or 3D variable \p varname. 
 //!
 //! \param[in] timestep Time step of the variable to read
 //! \param[in] varname Name of the variable to read
 //! 
 //! \retval id Returns a file handle on success, and a negative value on 
 //! failure. This method will fail if the netCDF file containing 
 //! \p varname at the given time step, \p timestep, can not be opened 
 //! for reading.
 //!
 //! \sa MetadataWRF::GetNumTimeSteps(), MetadataWRF::MapVDCTimestep()
 //!
 virtual int OpenVariableRead(
	size_t timestep, const char *varname 
 );

 //! Close a currently opened variable.
 //!
 //! Close the variable currently opened for reading associated with 
 //! the file handle, \p handle.
 //!
 //! \param[in] handle An open file handle returned by OpenVariableRead()
 //!
 //! \retval status A negative integer is returned on failure
 //!
 //! \sa OpenVariableRead()
 //
 virtual int CloseVariable(int handle);

 //! Read in and return the currently opened variable.
 //!
 //! This method reads the variable associated with the open
 //! file handle, \p handle, into the array pointed to by \p region. 
 //! The entire 2D or 3D variable is copied to \p region. It is the caller's
 //! responsiblity to ensure \p region points to enough space for the 
 //! variable. The dimensions of the variable are given by 
 //! MetadataWRF::GetDimension().
 //!
 //! \param[in] handle An open file handle returned by OpenVariableRead()
 //! \param[in] region A pointer to sufficient space to contain the 2D or
 //! 3D variable.
 //!
 //! \retval status Returns a non-negative value on success
 //!
 //! \sa OpenVariableRead(), GetDimension(), GetVarType()
 //
 virtual int ReadVariable(
	int handle, float *region
 );


 //! Read in and return a slice from the currently opened variable.
 //!
 //! This method reads a single slice of the variable associated with the open
 //! file handle, \p handle, into the array pointed to by \p region. 
 //! The slice is identified by \p z. For 3D variables \p z must be in 
 //! the range from 0
 //! to nz-1, where nz is the 3rd element of the vector returned by
 //! GetDimension(). For 2D variables \p z must be zero.
 //! It is the caller's
 //! responsiblity to ensure \p region points to enough space for one slice
 //! of the variable. The dimensions of the slize are given by 
 //! the first two elements of the vector returned by 
 //! MetadataWRF::GetDimension().
 //!
 //! \param[in] handle An open file handle returned by OpenVariableRead()
 //! \param[in] z Index of the slice to read.
 //! \param[in] region A pointer to sufficient space to contain a slice
 //!
 //! \retval status Returns a non-negative value on success. This method
 //! will fail if \p z is out of range.
 //!
 //! \sa OpenVariableRead(), GetDimension(), GetVarType()
 //
 virtual int ReadSlice(
	int handle, int z, float *region
 );

 virtual void InterpolateRegion(
    float *region,          // Destination interpolated ROI
    const float *elevBlks,  // Source elevation ROI
    const float *varBlks,   // Source variable ROI
    const size_t blkMin[3],
    const size_t blkMax[3], // coords of native (layered grid)
                            // ROI specified in blocks
    size_t zmini,
    size_t zmaxi,           // Z coords extents of interpolated ROI
    float *lowVal,
    float *highVal
 ) const;


 //! Returns true if the indicated data volume exists on disk
 //!
 //! Returns true if the variable identified by the timestep, variable
 //! name, refinement level, and level-of-detail is present on disk. 
 //! Returns 0 if
 //! the variable is not present.
 //! \param[in] ts A valid time step from the Metadata object used
 //! to initialize the class
 //! \param[in] varname A valid variable name
 //! \param[in] reflevel Ignored
 //! \param[in] lod Compression level of detail requested. The coarsest 
 //! approximation level is 0 (zero). A value of -1 indicates the finest
 //! refinement level contained in the VDC.
 //
 virtual int    VariableExists(
    size_t ts,
    const char *varname
 ) const ;

private:

	class wrf_file_t {
	public:
		int _ncid;
		string _varname;
		size_t _wrf_ts;
		size_t _nx;
		size_t _ny;
		size_t _nz;
		bool _in_use;
	};

	vector <wrf_file_t> _openFiles;

};
};

#endif	// _WRFReader_h_
