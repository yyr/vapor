//
//      $Id$
//


#ifndef	_Metadata_h_
#define	_Metadata_h_

#include <stack>
#include <expat.h>
#include <vapor/MyBase.h>
#include <vaporinternal/common.h>
#include "vapor/XmlNode.h"
#include "vapor/ExpatParseMgr.h"
#ifdef WIN32
#pragma warning(disable : 4251)
#endif

namespace VAPoR {

//
// Macros to check for required Metadata elements
// Call SetErrMsg if element is not present
//
#define	CHK_TS_REQ(TS, RETVAL) \
	if (! _rootnode->GetChild(TS)) { \
		SetErrMsg("Invalid time step : %d", TS); \
		return(RETVAL); \
	}
#define	CHK_VAR_REQ(TS, VAR, RETVAL) \
	if (! _rootnode->GetChild(TS)) { \
		SetErrMsg("Invalid time step : %d", TS); \
		return(RETVAL); \
	}; \
	if (! _rootnode->GetChild(TS)->GetChild(VAR)) { \
		SetErrMsg("Invalid variable name : %s", VAR.c_str()); \
		return(RETVAL); \
	}

//
// Macros to check for optional Metadata elements
// Don't call SetErrMsg if element is not present
//
#define	CHK_TS_OPT(TS, RETVAL) \
	if (! _rootnode->HasChild(TS)) { \
		return(RETVAL); \
	}
#define	CHK_VAR_OPT(TS, VAR, RETVAL) \
	if (! _rootnode->HasChild(TS)) { \
		return(RETVAL); \
	}; \
	if (! _rootnode->GetChild(TS)->HasChild(VAR)) { \
		return(RETVAL); \
	}

const int VDF_VERSION = 3;

//
//! \class Metadata
//! \brief A class for managing data set metadata
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! The Metadata class is used to read and write VAPoR
//! metadata files.  A metadata file desribes a time-varying,
//! multi-variate, multiresolution data set. Elements of 
//! the data set are
//! 3D volumes of gridded field data. A data set's
//! metadata describes attributes of the field data, such 
//! as its dimension, grid spacing, real world coordinates,
//! etc., but does specify the values of the dependent 
//! field values themselves. In a VAPoR data set, 
//! metadata and dependent field data are store in
//! separate files.
//! 
//! The Metadata class is derived from the MyBase base
//!	class. Hence all of the methods make use of MyBase's
//!	error reporting capability - the success of any method
//!	(including constructors) can (and should) be tested
//!	with the GetErrCode() method. If non-zero, an error 
//!	message can be retrieved with GetErrMsg().
//!
//! Methods that retrieve required Metadata elements will
//! set an error code via MyBase::SetErrMsg() if the requested
//! element is not present in the Metadata object. Methods
//! that retrieve optional elements will NOT set an error 
//! code if an optional element is not present.
//!
class VDF_API Metadata : public VetsUtil::MyBase , public ParsedXml {
public:

 //! Create a metadata object from scratch. 
 //!
 //! \param[in] dim The X, Y, and Z coordinate dimensions of all
 //! data volumes, specified in grid coordinates (voxels)
 //! \param[in] numTransforms Number of wavelet transforms to perform
 //! \param[in] bs Internal blocking factor for transformed data. Must
 //! be a power of two.
 //! \param[in] nFilterCoef Number of filter coefficients used by wavelet
 //! transforms
 //! \param[in] nLiftingCoef Number of lifting coefficients used by wavelet
 //! transforms
 //! \param[in] msbFirst Boolean, if true storage order for volume data 
 //! will be
 //! most significant byte fist (i.e. big endian).
 //! \param[in] vdfVersion VDF file version number. In general this
 //! should not be changed from the default
 //
 Metadata(
	const size_t dim[3], size_t numTransforms, size_t bs[3],
	int nFilterCoef = 1, int nLiftingCoef = 1, int msbFirst =  1,
	int vdfVersion = VDF_VERSION
	);

 //! Create a metadata object from a metadata file stored on disk. 
 //!
 //! \param[in] path Path to metadata file
 //
 Metadata(const string &path);

 virtual ~Metadata();

 //! Merge the contents of two metadata objects
 //!
 //! This method merges the contents of the metadata object pointed to 
 //! by \p metadata with the current class object. In instances where there
 //! are collisions (nodes with the same name), the values \p metadata
 //! take precedence over the current object. The timestep parameter, 
 //! \p ts, may be used to specify a time step offset. I.e. The first
 //! timestep in \p metadata will be merged with the timestep of 
 //! this class objected indicated by \p ts. 
 //!
 //! The following Metadata attributes are required to match
 //! in both Metadata objects for the merge to take place: 
 //!
 //! \li BlockSize
 //! \li DimensionLength
 //! \li FilterCoefficents
 //! \li LiftingCoefficients
 //! \li NumTransforms
 //! \li VDFVersion
 //!
 //! The following metadata parameters are simply ignored (i.e. the
 //! parameter values for this class instance take precedence over those
 //! in the object pointed to by \p metadata):
 //!
 //! \li Extents
 //! \li CoordinateSystem
 //! \li GridType
 //!
 //! Similary, global and time step comments found in \p metadata are 
 //! not merged, nor is any user-defined data.
 //!
 //! As a side effect both the Metadata object pointed to by this class
 //! and the Metadata object pointed to by \p metadata are 
 //! made current with MakeCurrent().
 //!
 //! \param[in] metadata A pointer to a valid metadata object. 
 //! \param[in] ts Timestep offset
 //! \retval status A negative integer is returned on failure, otherwise
 //! the method has succeeded.
 //
 int Merge(Metadata *metadata, size_t ts = 0);

 //! Merge the contents of two metadata objects
 //!
 //! This method is identical to the overloaded version of the same
 //! name. However, a path name to a .vdf file is provided instead
 //! of an Metadata object
 //!
 //! \param[in] path Path to metadata file
 //! \param[in] ts Timestep offset
 //! \retval status A negative integer is returned on failure, otherwise
 //! the method has succeeded.
 //
 int Merge(const string &path, size_t ts = 0);

 //! Make the metadata object current
 //!
 //! This method updates the metadata object to the most current version.
 //! Metadata objects older than version 1 cannot be updated.
 //!
 //! \retval status A negative integer is returned on failure, otherwise
 //! the method has succeeded.
 //!
 //! \sa GetVDFVersion()
 //
 int MakeCurrent();

 //! Return the file path name to the metafile's parent directory.
 //! If the class was constructed with a path name, this method
 //! returns the parent directory of the path name. If the class
 //! was not constructed with a path name, an empty string is returned
 //! \retval dirname : parent directory or an empty string
 //
 const string &GetParentDir() const { return (_metafileDirName); }

 //! Return the base of file path name to the metafile.
 //! If the class was constructed with a path name, this method
 //! returns the basename of the path name. If the class
 //! was not constructed with a path name, an empty string is returned
 //! \retval dirname : parent directory or an empty string
 //
 const string &GetMetafileName() const { return (_metafileName); }

 //! Return the base of file path name to the data subdirectory.
 //! If the class was constructed with a path name, this method
 //! returns the basename of the path name, with any file extensions
 //! removed, and "_data" appended. If the class
 //! was not constructed with a path name, an empty string is returned
 //! \retval dirname : parent directory or an empty string
 //
 const string &GetDataDirName() const { return (_dataDirName); }

 int ConstructFullVBase(size_t ts, const string &var, string *path) const;
 int ConstructFullAuxBase(size_t ts, string *path) const;

 //! Write the metadata object to a file
 //!
 //! \param[in] path Name of the file to write to
 //! \param[in] relative_path Use relative path names for all data paths
 //! \retval status Returns a non-negative integer on success
 //
 int Write(const string &path, int relative_path = 1) ;


 //! Return the internal blocking factor use for WaveletBlock files
 //!
 //! \retval size Internal block factor
 //! \remarks Required element
 //
 const size_t *GetBlockSize() const { return(_bs); }

 //! Returns the X,Y,Z coordinate dimensions of the data in grid coordinates
 //! \retval dim A three element vector containing the voxel dimension of 
 //! the data at its native resolution
 //!
 //! \remarks Required element
 //!
 const size_t *GetDimension() const { return(_dim); }

 //! Returns the number of filter coefficients employed for wavelet transforms
 //! \retval _nFilterCoef Number of filter coefficients
 //!
 //! \remarks Required element
 //
 int GetFilterCoef() const { return(_nFilterCoef); }

 //! Returns the number of lifting coefficients employed for wavelet transforms
 //! \retval _nLiftingCoef Number of lifting coefficients
 //!
 //! \remarks Required element
 //
 int GetLiftingCoef() const { return(_nLiftingCoef); }

 //! Returns the number of wavelet transforms
 //! \param _numTransforms Number of transforms
 //!
 //! \remarks Required element
 //
 int GetNumTransforms() const { return(_numTransforms); }

 //! Returns true if the storage order for data is most signicant byte first
 //! \retval _msbFirst Booean
 //!
 //! \remarks Required element
 //
 int GetMSBFirst() const { return(_msbFirst); }

 //! Returns vdf file version number
 //! \retval _vdfVersion Version number
 //!
 //! \remarks Required element
 //
 int GetVDFVersion() const { return(_vdfVersion); }


 //------------------------------------------------------------------
 //			Metdata Attributes
 //
 // The methods below get, set, and possibly test various metadata
 // attributes.
 //
 // N.B. Get* methods return pointers to internal storage containing
 // the desired attribute values.
 //
 //------------------------------------------------------------------


 //! Set the grid type. 
 //!
 //! \param[in] value Grid type. One of \b regular or \b stretched or \b layered
 //! \retval status Returns a non-negative integer on success
 //
 int SetGridType(const string &value);

 //! Return the grid type.
 //!
 //! \retval type The grid type
 //!
 //! \remarks Required element
 //
 const string &GetGridType() const {
	return(_rootnode->GetElementString(_gridTypeTag));
	};

 //! Return true if \p value is a valid grid type.
 //! \param[in] value Grid type
 //! \retval boolean True if \p value is a valid grid type
 //!
 int IsValidGridType(const string &value) const {
	return(
		(VetsUtil::StrCmpNoCase(value,"regular") == 0) || 
		(VetsUtil::StrCmpNoCase(value,"stretched") == 0) || 
		(VetsUtil::StrCmpNoCase(value,"layered") == 0) || 
		(VetsUtil::StrCmpNoCase(value,"block_amr") == 0)
	);
	}

 //! Set the coordinate system type
 //! \param[in] value Coordinate system type. One of \b cartesian
 //! or \b spherical
 //! \retval status Returns a non-negative integer on success
 //
 int SetCoordSystemType(const string &value);

 //! Return the coordinate system type.
 //! \retval type The grid type
 //!
 //! \remarks Required element
 //
 const string &GetCoordSystemType() const {
	return(_rootnode->GetElementString(_coordSystemTypeTag));
	};

 //! Return true if \p value is a valid coordinate system type.
 //!
 //! \retval boolean True if \p value is a valid type
 //
 int IsValidCoordSystemType(const string &value) const {
	return(
		(VetsUtil::StrCmpNoCase(value,"cartesian") == 0) || 
		(VetsUtil::StrCmpNoCase(value,"spherical") == 0)
	);
	}

 //! Set the domain extents of the data set 
 //!
 //! Set the domain extents of the data set in user-defined (world) 
 //! coordinates. 
 //! \param value A six-element array, the first three elements
 //! specify the minimum coordinate extents, the last three elements 
 //! specify the maximum coordinate extents.
 //! \retval status Returns a non-negative integer on success
 //
 int SetExtents(const vector<double> &value);

 //! Return the domain extents specified in user coordinates
 //!
 //! \retval extents A six-element array containing the min and max
 //! bounds of the data domain in user-defined coordinates
 //!
 //! \remarks Required element
 //
 const vector<double> &GetExtents() const {
	return(_rootnode->GetElementDouble(_extentsTag));
	};

 //! Return true if \p value is a valid coordinate extent definition
 //!
 //! \retval boolean True if \p value is a valid argument
 //
 int IsValidExtents(const vector<double> &value) const;

 //! Set the number of time steps in the data collection
 //!
 //! \param[in] value The number of time steps in the data set
 //! \retval status Returns a non-negative integer on success
 //
 int SetNumTimeSteps(long value);

 //! Return the number of time steps in the collection
 //!
 //! \retval value The number of time steps or a negative number on error
 //!
 //! \remarks Required element
 //
 long GetNumTimeSteps() const;

 //! Return true if \p value is a valid time step specification
 //!
 //! \retval boolean True if \p value is a valid argument
 //
 int IsValidTimeStep(long value) const {
	return(value >= 0);
	};

 //! Set the names of the field variables in the data collection
 //!
 //! All variables specified are of type 3D. 
 //! \param[in] value A white-space delimited list of names
 //! \retval status Returns a non-negative integer on success
 //
 int SetVariableNames(const vector <string> &value);



 //! Return the names of the variables in the collection 
 //!
 //! \retval value is a space-separated list of variable names
 //!
 //! \remarks Required element
 //
 const vector <string> &GetVariableNames() const {
	return(_varNames);
	};

 //! Indicate which variables in a VDC are of type 3D
 //!
 //! Specifies which variables in a VDC represent 
 //! three-dimensional fields.  The variable names must 
 //! match names previoulsy provided by SetVariableNames().
 //! \note As the default type of all variables is 3D, this method
 //! is largely superflous, but is included for completeness
 //!
 //! \param[in] value A white-space delimited list of names
 //! \retval status Returns a non-negative integer on success
 //
 int SetVariables3D(const vector <string> &value);

 //! Return the names of the 3D variables in the collection 
 //!
 //! \retval value is a space-separated list of 3D variable names
 //!
 //! \remarks Required element (VDF version 1.3 or greater)
 //
 const vector <string> &GetVariables3D() const {
	return(_varNames3D);
	};

 //! Indicate which variables in a VDC are of type 2DXY
 //!
 //! Specifies which variables in a VDC represent 
 //! two-dimensional, XY-plane fields.  The variable names must 
 //! match names previoulsy provided by SetVariableNames().
 //!
 //! \param[in] value A white-space delimited list of names
 //! \retval status Returns a non-negative integer on success
 //
 int SetVariables2DXY(const vector <string> &value);
 int SetVariables2DXZ(const vector <string> &value);
 int SetVariables2DYZ(const vector <string> &value);

 //! Return the names of the 2D, XY variables in the collection 
 //!
 //! \retval value is a space-separated list of 2D XY variable names
 //!
 //! \remarks Required element (VDF version 1.3 or greater)
 //
 const vector <string> &GetVariables2DXY() const {
	return(_varNames2DXY);
	};
 const vector <string> &GetVariables2DXZ() const {
	return(_varNames2DXZ);
	};
 const vector <string> &GetVariables2DYZ() const {
	return(_varNames2DYZ);
	};


 //! Set a global comment
 //!
 //! The comment is intended to refer to the entire data set
 //! \param[in] value A user defined comment
 //! \retval status Returns a non-negative integer on success
 //
 int SetComment(const string &value) {
	_rootnode->SetElementString(_commentTag, value);
	return(0);
	}

 //! Return the global comment, if it exists
 //!
 //! \retval value The global comment. An empty string is returned
 //! if the global comment is not defined.
 //!
 //! \remarks Optional element 
 //
 const string &GetComment() const {
	if (_rootnode->HasElementString(_commentTag))
		return(_rootnode->GetElementString(_commentTag));
	else
		return(_emptyString);
	};

 //! Set the grid boundary type
 //!
 //! \param value A three-element array, indicating whether the
 //! boundary conditions for X, Y, and Z coordinate axis, respectively,
 //! are periodic. Any non-zero value is interpreted as true.
 //! \retval status Returns a non-negative integer on success
 //
 int SetPeriodicBoundary(const vector<long> &value) {
	_rootnode->SetElementLong(_periodicBoundaryTag, value);
	return(0);
 }

 //! Return a three-element boolean array indicating if the X,Y,Z
 //! axes have periodic boundaries, respectively.
 //!
 //! \retval boolean-vector  
 //!
 //! \remarks Required element (VDF version 1.3 or greater)
 //
 const vector<long> &GetPeriodicBoundary() const {
	if (_rootnode->HasElementLong(_periodicBoundaryTag)) {
		return(_rootnode->GetElementLong(_periodicBoundaryTag));
	} else {
		return(_periodicBoundaryDefault);
	}
	};

 //! Return true if a the Metadata object has a periodic boundary element
 //!
 //! \retval boolean True if a periodic boundary element exists 
 //
 int HasPeriodicBoundary() const {
	return(_rootnode->HasElementDouble(_periodicBoundaryTag));
	};

 //! Set the grid coordinate ordering
 //!
 //! \param value A three-element array indicating the association
 //! between the X,Y,Z (long, lat, radius) coordinates and the 
 //! ordering of the data. By default, the fastest varying coordinate
 //! is X (long.), followed by Y (lat), then Z (radius). This method
 //! permits a permutation of the data oredering to be specified via
 //! a three-element array containing a permutation of the ordered set 
 //! (0,1,2).  
 //!
 //! \retval status Returns a non-negative integer on success
 //
 int SetGridPermutation(const vector<long> &value) {
	_rootnode->SetElementLong(_gridPermutationTag, value);
	return(0);
 }

 //! Return a three-element integer array indicating the coordinate
 //! ordering permutation.
 //!
 //! \retval integer-vector  
 //!
 //! \remarks Required element (VDF version 1.3 or greater)
 //
 const vector<long> &GetGridPermutation() const {
	if (_rootnode->HasElementLong(_gridPermutationTag)) {
		return(_rootnode->GetElementLong(_gridPermutationTag));
	} else {
		return(_gridPermutationDefault);
	}
	};


 //! Set the time of a time step in user-defined coordinates.
 //!
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \param[in] value A single element vector specifying the time
 //! \retval status Returns a non-negative integer on success
 //
 int SetTSUserTime(size_t ts, const vector<double> &value);

 //! Return the time for a time step, if it exists, 
 //!
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \retval value A single element vector specifying the time
 //!
 //! \remarks Required element 
 //!
 //
 const vector<double> &GetTSUserTime(size_t ts) const {
	CHK_TS_REQ(ts, _emptyDoubleVec)
	return(_rootnode->GetChild(ts)->GetElementDouble(_userTimeTag));
	};

 //! Return the base path for auxiliary data, indicated by the time 
 //! step, \p ts, if it exists.  
 //!
 //! Paths to data files are constructed from the base path.
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \retval path Auxiliary data base path name
 //!
 //! \remarks Required element 
 //
 const string &GetTSAuxBasePath(size_t ts) const {
	CHK_TS_REQ(ts, _emptyString)
	return(_rootnode->GetChild(ts)->GetElementString(_auxBasePathTag));
	};

 //! Return true if \p value is a valid time specification.
 //!
 //! \retval boolean True if \p value is a valid argument
 //
 int IsValidUserTime(const vector<double> &value) const {
	return(value.size() == 1);
	};

 //! Specify the X dimension coordinates for a stretched grid
 //!
 //! Specify the X dimension coordinates of data samples for a 
 //! stretched grid. 
 //! Similar arrays must be specified for Y and Z sample coordinates. 
 //! These arrays are ignored if the grid type is regular, in which case
 //! the grid spacing is treated as uniform and derived from the metadata
 //! Extents attribute. Coordinate arrays are specified for the time
 //! step indicated by \p ts.
 //!
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \param[in] value An array of monotonically increasing values specifying 
 //! the X coordinates, in a user-defined coordinate system, of each 
 //! YZ sample plane. 
 //! \retval status Returns a non-negative integer on success
 //! \sa SetGridType(), GetGridType(), GetTSXCoords()
 //
 int SetTSXCoords(size_t ts, const vector<double> &value);

 //! Return the X dimension coordinate array, if it exists
 //!
 //! \retval value An array of monotonically increasing values specifying
 //! the X // coordinates, in a user-defined coordinate system, of each
 //! YZ sample plane. An empty vector is returned if the coordinate
 //! dimension array is not defined for the specified time step.
 //! \sa SetGridType(), GetGridType(), GetTSXCoords()
 //!
 //! \remarks Optional element 
 //
 const vector<double> &GetTSXCoords(size_t ts) const {
	CHK_TS_OPT(ts, _emptyDoubleVec)
	if (_rootnode->GetChild(ts)->HasElementDouble(_xCoordsTag))
		return(_rootnode->GetChild(ts)->GetElementDouble(_xCoordsTag));
	else 
		return(_emptyDoubleVec);
	};

 //! Return true if \p value is a valid X dimension coordinate array
 //!
 //! \param[i] value An array of monotonically increasing values specifying 
 //! the X // coordinates, in a user-defined coordinate system, of each 
 //! YZ sample plane. 
 //! \retval boolean True if \p value is a valid argument
 //
 int IsValidXCoords(const vector<double> &value) const {
	return(value.size() == _dim[0]);
	}

 int SetTSYCoords(size_t ts, const vector<double> &value);

 const vector<double> &GetTSYCoords(size_t ts) const {
	CHK_TS_OPT(ts, _emptyDoubleVec)
	if (_rootnode->GetChild(ts)->HasElementDouble(_yCoordsTag))
		return(_rootnode->GetChild(ts)->GetElementDouble(_xCoordsTag));
	else 
		return(_emptyDoubleVec);
	}
 int IsValidYCoords(const vector<double> &value) const {
	return(value.size() == _dim[1]);
	}

 int SetTSZCoords(size_t ts, const vector<double> &value);
 const vector<double> &GetTSZCoords(size_t ts) const {
	CHK_TS_OPT(ts, _emptyDoubleVec)
	if (_rootnode->GetChild(ts)->HasElementDouble(_zCoordsTag))
		return(_rootnode->GetChild(ts)->GetElementDouble(_xCoordsTag));
	else 
		return(_emptyDoubleVec);
	}
 int IsValidZCoords(const vector<double> &value) const {
	return(value.size() == _dim[2]);
	}

 //! Set a comment for the time step indicated by \p ts
 //
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \param value A comment string
 //! \retval status Returns a non-negative integer on success
 //
 int SetTSComment(size_t ts, const string &value);

 //! Return the comment for the indicated time step, \p ts, if it exists
 //
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \retval comment A comment string. An empty string is returned if
 //! the comment for the specified time step is not defined.
 //!
 //! \remarks Optional element 
 //
 const string &GetTSComment(size_t ts) const {
	CHK_TS_OPT(ts, _emptyString)
	if (_rootnode->GetChild(ts)->HasElementString(_commentTag))
		return(_rootnode->GetChild(ts)->GetElementString(_commentTag));
	else 
		return(_emptyString);
	}

 //! Set the spatial domain extents of the indicated time step
 //!
 //! Set the spatial domain extents of the data set in user-defined (world) 
 //! coordinates for the indicated time step. 
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \param value A six-element array, the first three elements
 //! specify the minimum coordinate extents, the last three elements 
 //! specify the maximum coordinate extents.
 //! \retval status Returns a non-negative integer on success
 //
 int SetExtents(size_t ts, const vector<double> &value);

 //! Return the domain extents specified in user coordinates
 //! for the indicated time step
 //!
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \retval extents A six-element array containing the min and max
 //! bounds of the data domain in user-defined coordinates.
 //! An empty vector is returned if the extents for the specified time
 //! step is not defined.
 //!
 //! \remarks Optional element
 //
 const vector<double> &GetExtents(size_t ts) const {
	CHK_TS_OPT(ts, _emptyDoubleVec)
	if (_rootnode->GetChild(ts)->HasElementDouble(_extentsTag))
		return(_rootnode->GetChild(ts)->GetElementDouble(_extentsTag));
	else 
		return(_emptyDoubleVec);
	};

 //! Set a comment for the variable, \p v at the time step indicated by \p ts
 //!
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \param[in] var A valid data set variable name
 //! \param[in] value A comment string
 //! \retval status Returns a non-negative integer on success
 //
 int SetVComment(size_t ts, const string &var, const string &value);

 //! Return the comment for the variable, \p v, indicated by the time 
 //! step, \p ts, if it exists
 //!
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \param[in] var A valid data set variable name
 //! \retval comment A comment string. An emptry string is returned if no
 //! comment is defined for the specified variable
 //!
 //! \remarks Optional element 
 //
 const string &GetVComment(size_t ts, const string &var) const {
	CHK_VAR_OPT(ts, var, _emptyString)
	if (_rootnode->GetChild(ts)->GetChild(var)->HasElementString(_commentTag))
		return(_rootnode->GetChild(ts)->GetChild(var)->GetElementString(_commentTag));
	else 
		return(_emptyString);
	}

 //! Return the base path for the variable, \p v, indicated by the time 
 //! step, \p ts, if it exists.  
 //!
 //! Paths to data files are constructed from the base path.
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \param[in] var A valid data set variable name
 //! \retval path Variable base path name
 //!
 //! \remarks Required element 
 //
 const string &GetVBasePath(size_t ts, const string &var) const;


 int SetVBasePath(
    size_t ts, const string &var, const string &value
 );

 //! Set the data range for the variable, \p v at the time step 
 //! indicated by \p ts
 //!
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \param[in] var A valid data set variable name
 //! \param[in] value A two-element vector containing the min and max
 //! values for the indicated volume
 //! \retval status Returns a non-negative integer on success
 //
 int SetVDataRange(size_t ts, const string &var, const vector<double> &value);

 //! Return the base path for the variable, \p v, indicated by the time 
 //! step, \p ts, if it exists.  
 //!
 //! Paths to data files are constructed from the base path.
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \param[in] var A valid data set variable name
 //! \retval path Variable base path name
 //!
 //! \nb This method is deprecated and should no longer be used.
 //
 const vector<double> &GetVDataRange(size_t ts, const string &var) const {
	CHK_VAR_REQ(ts, var, _emptyDoubleVec)
	return(_rootnode->GetChild(ts)->GetChild(var)->GetElementDouble(_dataRangeTag));
	}

 int IsValidVDataRange(const vector<double> &value) const {
	return(value.size() == 2);
	}

 //------------------------------------------------------------------
 //			User-Defined Metdata Attributes
 //
 // The methods below provide a means for the user get and set user-defined
 // attributes. The interpretation of these attributes is solely up
 // to the user. Attributes may be set/get at three "levels"; at the
 // top level, where they may represent global metadata; at the
 // time step level, where the attribute is associated with a 
 // particular time step; and at the variable level, where the attribute
 // is associated with a particular variable, *and* a particular
 // time step.
 // 
 // Overloaded get/set methods exist for attributes of type
 // vector<long>, vector<double>, and string. Addionally, for each 
 // attribute type (long, string, or double) a method is provided for 
 // retrieving the names of all attributes defined of that type.
 //
 // A user-defined attribute is defined whenever it is set via a Set* 
 // method
 //
 // The valid range for the 'ts' parameters in the methods below is
 // [0..NumTS-1], where NumTS is the value returned by method 
 // GetNumTimeSteps.
 //
 // Valid values for the 'var' parameter are those returned by
 // by the GetVariableNames() method.
 //
 //------------------------------------------------------------------

 //------------------------------------------------------------------
 //			Top Level (Global) User-Defined Metdata Attributes
 //
 // The user-defined attribute access methods defined below set 
 // and get top-level 
 // metadata attributes - attributes that may apply to the entire
 // data set. 
 //
 //------------------------------------------------------------------


 //! Return a vector of global, user-defined metadata tags for \b long data
 //!
 //! The vector returned contains the tags (names) for any user-defined,
 //! global metadata. The value(s) of the metadata associated with
 //! the returned tags may be queried with GetUserDataLong().
 //! \retval vector A vector of tag names
 //! \sa SetUserDataLong(), GetUserDataLong()
 //
 const vector<string> &GetUserDataLongTags() const {return(_userDLTags);}

 //! Set global, user-defined metadata
 //!
 //! Set the value(s) of a top-level (global) user-defined metadata tag of 
 //! type \b long.
 //! This method both defines the tag name and sets it's value
 //! \param[in] tag Name of metadata tag
 //! \param[in] value A vector of one or more metadata values
 //! \retval status Returns a non-negative integer on success
 //! \sa GetUserDataLongTag(), GetUserDataLong()
 //
 int SetUserDataLong(const string &tag, const vector<long> &value) {
	_RecordUserDataTags(_userDLTags, tag);
	_rootnode->SetElementLong(tag, value);
	return(0);
 }

 //! Get global, user-defined metadata
 //!
 //! Get the value(s) of a top-level (global), user-defined metadata tag of
 //! type \b long.
 //! 
 //! \param[in] tag Name of metadata tag
 //! \retval vector A vector of metadata values associated with \p tag. If
 //! \p tag is not defined by the metadata class, the vector returned 
 //! is empty.
 //! \sa GetUserDataLongTag(), SetUserDataLong()
 //!
 //! \remarks Optional element
 //
 const vector<long> &GetUserDataLong(const string &tag) const {
	if (_rootnode->HasElementLong(tag))
		return(_rootnode->GetElementLong(tag));
	else 
		return(_emptyLongVec);
 }

 const vector<string> &GetUserDataDoubleTags() const {return(_userDDTags);}

 int SetUserDataDouble(const string &tag, const vector<double> &value) {
	_RecordUserDataTags(_userDDTags, tag);
	_rootnode->SetElementDouble(tag, value);
	return(0);
 }
 const vector<double> &GetUserDataDouble(const string &tag) const {
	if (_rootnode->HasElementDouble(tag))
		return(_rootnode->GetElementDouble(tag));
	else 
		return(_emptyDoubleVec);
 }

 const vector<string> &GetUserDataStringTags() const {return(_userDSTags);}

 int SetUserDataString(const string &tag, const string &value) {
	_RecordUserDataTags(_userDSTags, tag);
	_rootnode->SetElementString(tag, value);
	return(0);
 }
 const string &GetUserDataString(const string &tag) const {
	if (_rootnode->HasElementString(tag))
		return(_rootnode->GetElementString(tag));
	else 
		return(_emptyString);
 }


 //------------------------------------------------------------------
 //			Time Step User-Defined Metdata Attributes
 //
 // The user-defined attribute access methods defined below set 
 // and get time step
 // metadata attributes - attributes that may apply to a particular
 // time step
 //
 //
 //------------------------------------------------------------------

 //! Return a vector of time step metadata tags for \b long data
 //!
 //! The vector returned contains the tags (names) for any user-defined,
 //! time step metadata. The value(s) of the metadata associated with
 //! the returned tags may be queried with GetTSUserDataLong().
 //! \retval vector A vector of tag names
 //! \sa SetTSUserDataLong(), GetTSUserDataLong()
 //!
 //
 const vector<string> &GetTSUserDataLongTags() const {
	return(_timeStepUserDLTags); 
 }

 //! Set time step, user-defined metadata
 //!
 //! Set the value(s) of a time step, user-defined metadata tag of 
 //! type \b long, associated with time step \p ts.
 //! This method both defines the tag name and sets it's value
 //! \param[in] ts The time step this metadata field applies to  
 //! \param[in] tag Name of metadata tag
 //! \param[in] value A vector of one or more metadata values
 //! \sa GetTSUserDataLongTag(), GetTSUserDataLong()
 //
 int SetTSUserDataLong(size_t ts, const string &tag, const vector<long> &value) {
	CHK_TS_REQ(ts, -1)
	_RecordUserDataTags(_timeStepUserDLTags, tag);
	_rootnode->GetChild(ts)->SetElementLong(tag, value);
	return(0);
 }

 //! Get time step, user-defined metadata
 //!
 //! Get the value(s) of a time step, user-defined metadata tag of
 //! type \b long, associated with time step \p ts.
 //! 
 //! \param[in] ts The time step this metadata field applies to  
 //! \param[in] tag Name of metadata tag
 //! \retval vector A vector of metadata values associated with \p tag. If
 //! \p tag is not defined by the metadata class, the vector returned 
 //! is empty.
 //! \sa GetTSUserDataLongTag(), SetTSUserDataLong()
 //!
 //! \remarks Optional element
 //
 const vector<long> &GetTSUserDataLong( size_t ts, const string &tag) const {
	CHK_TS_OPT(ts, _emptyLongVec)
	if (_rootnode->GetChild(ts)->HasElementLong(tag))
		return(_rootnode->GetChild(ts)->GetElementLong(tag));
	else 
		return(_emptyLongVec);
 }

 int SetTSUserDataDouble(
	size_t ts, const string &tag, const vector<double> &value
 ) {
	CHK_TS_REQ(ts, -1)
	_RecordUserDataTags(_timeStepUserDDTags, tag);
	_rootnode->GetChild(ts)->SetElementDouble(tag, value);
	return(0);
 }

 const vector<double> &GetTSUserDataDouble(size_t ts, const string &tag) const {
	CHK_TS_OPT(ts, _emptyDoubleVec)
	if (_rootnode->GetChild(ts)->HasElementDouble(tag))
		return(_rootnode->GetChild(ts)->GetElementDouble(tag));
	else 
		return(_emptyDoubleVec);
 }
 const vector<string> &GetTSUserDataDoubleTags() const {
	return(_timeStepUserDDTags);
 }

 int SetTSUserDataString(
	size_t ts, const string &tag, const string &value
 ) {
	CHK_TS_REQ(ts, -1)
	_RecordUserDataTags(_timeStepUserDSTags, tag);
	_rootnode->GetChild(ts)->SetElementString(tag, value);
	return(0);
 }

 const string &GetTSUserDataString(size_t ts, const string &tag) const {
	CHK_TS_OPT(ts, _emptyString)
	if (_rootnode->GetChild(ts)->HasElementString(tag))
		return(_rootnode->GetChild(ts)->GetElementString(tag));
	else 
		return(_emptyString);
 }
 const vector<string> &GetTSUserDataStringTags() const {
	return(_timeStepUserDSTags);
 }

 //------------------------------------------------------------------
 //			Variable User-Defined Metdata Attributes
 //
 // The user-defined attribute access methods defined below set 
 // and get variable
 // metadata attributes - attributes that may apply to a particular
 // variable within a given time step.
 // Attribute get/set methods exist for attributes of type
 // vector<long>, vector<double>, and string. Addionally, for each 
 // attribute type (long, string, or double) a method is provided for 
 // retrieving the names of all attributes defined of that type.
 //
 //------------------------------------------------------------------

 // Return a list of the names of user defined variable attributes
 // of type long.
 //
 //! Return a vector of variable metadata tags for \b long data
 //!
 //! The vector returned contains the tags (names) for any user-defined,
 //! variable metadata. The value(s) of the metadata associated with
 //! the returned tags may be queried with GetVUserDataLong().
 //! \retval vector A vector of tag names
 //! \sa SetVUserDataLong(), GetVUserDataLong(), GetVariableNames()
 //
 const vector<string> &GetVUserDataLongTags() const {
	return(_variableUserDLTags); 
 }

 //! Set variable, user-defined metadata
 //!
 //! Set the value(s) of a variable, user-defined metadata tag of 
 //! type \b long, associated with time step \p ts and variable \p var.
 //! This method both defines the tag name and sets it's value
 //! \param[in] ts The time step this metadata field applies to  
 //! \param[in] var The field variable this metadata field applies to
 //! \param[in] tag Name of metadata tag
 //! \param[in] value A vector of one or more metadata values
 //! \retval status Returns a non-negative integer on success
 //! \sa GetVUserDataLongTag(), GetVUserDataLong()
 //
 int SetVUserDataLong(
	size_t ts, const string &var, const string &tag, const vector<long> &value
 ) {
	CHK_VAR_REQ(ts, var, -1)
	_RecordUserDataTags(_variableUserDLTags, tag);
	_rootnode->GetChild(ts)->GetChild(var)->SetElementLong(tag, value);
	return(0);
 }

 //! Get variable, user-defined metadata
 //!
 //! Get the value(s) of a variable, user-defined metadata tag of
 //! type \b long, associated with time step \p ts and variable \p var.
 //! 
 //! \param[in] ts The time step this metadata field applies to  
 //! \param[in] var The field variable this metadata field applies to
 //! \param[in] tag Name of metadata tag
 //! \retval vector A vector of metadata values associated with \p tag. If
 //! \p tag is not defined by the metadata class, the vector returned 
 //! is empty.
 //! \sa GetTSUserDataLongTag(), SetTSUserDataLong()
 //!
 //! \remarks Optional element
 //
 const vector<long> &GetVUserDataLong(
	size_t ts, const string &var, const string &tag
 ) const {
	CHK_VAR_OPT(ts, var, _emptyLongVec)
	if (_rootnode->GetChild(ts)->GetChild(var)->HasElementLong(tag))
		return(_rootnode->GetChild(ts)->GetChild(var)->GetElementLong(tag));
	else 
		return(_emptyLongVec);
 }

 const vector<string> &GetVUserDataDoubleTags() const {
	return(_variableUserDDTags);
 }
 int SetVUserDataDouble(
	size_t ts, const string &var, const string &tag, const vector<double> &value
 ) {
	CHK_VAR_REQ(ts, var, -1)
	_RecordUserDataTags(_variableUserDDTags, tag);
	_rootnode->GetChild(ts)->GetChild(var)->SetElementDouble(tag, value);
	return(0);
 }

 const vector<double> &GetVUserDataDouble(
	size_t ts, const string &var, const string &tag
 ) const {
	CHK_VAR_OPT(ts, var, _emptyDoubleVec)
	if (_rootnode->GetChild(ts)->GetChild(var)->HasElementDouble(tag))
		return(_rootnode->GetChild(ts)->GetChild(var)->GetElementDouble(tag));
	else 
		return(_emptyDoubleVec);
 }

 int SetVUserDataString(
	size_t ts, const string &var, const string &tag, const string &value
 ) {
	CHK_VAR_REQ(ts,var,-1)
	_RecordUserDataTags(_variableUserDSTags, tag);
	_rootnode->GetChild(ts)->GetChild(var)->SetElementString(tag, value);
	return(0);
 }

 const string &GetVUserDataString(
	size_t ts, const string &var, const string &tag
 ) const {
	CHK_VAR_OPT(ts, var, _emptyString)
	if (_rootnode->GetChild(ts)->GetChild(var)->HasElementString(tag))
		return(_rootnode->GetChild(ts)->GetChild(var)->GetElementString(tag));
	else 
		return(_emptyString);
 }

 const vector<string> &GetVUserDataStringTags() const {
	return(_variableUserDSTags);
 }

protected:
 int	_objInitialized;	// has the obj successfully been initialized?
 XmlNode	*_rootnode;		// root node of the xml tree
 vector <string> _varNames;	// Names of all the field variables
 vector <string> _varNames3D;	// Names of all 3D field variables
 vector <string> _varNames2DXY;	// Names of all 2D XY field variables
 vector <string> _varNames2DXZ;	// Names of all 2D XZ field variables
 vector <string> _varNames2DYZ;	// Names of all 2D YZ field variables
 size_t _bs[3];				// blocking factor to be used by data
 size_t _dim[3];			// data dimensions
 int	_nFilterCoef;		// Lifting filter coefficients
 int	_nLiftingCoef;
 int	_numTransforms;		// Number of wavelet transforms
 int	_msbFirst;			// Most Significant Byte First storage order
 int	_vdfVersion;		// VDF file version number
 string	_metafileDirName;	// path to metafile parent directory
 string	_metafileName;		// basename of path to metafile 
 string _dataDirName;		// basename of path to data directory

 vector <double>	_emptyDoubleVec;
 vector <long>		_emptyLongVec;
 string 			_emptyString;

 
 string _currentVar;	// name of variable currently being processed
 long 	_currentTS;	// Number of time step currently being processed
 vector <long> _periodicBoundaryDefault;	// default periodic boundary mask
 vector <long> _gridPermutationDefault;	// default grid permutation


 // Known xml tags
 //
 static const string _childrenTag;
 static const string _commentTag;
 static const string _coordSystemTypeTag;
 static const string _dataRangeTag;
 static const string _extentsTag;
 static const string _gridTypeTag;
 static const string _numTimeStepsTag;
 static const string _basePathTag;
 static const string _auxBasePathTag;
 static const string _rootTag;
 static const string _userTimeTag;
 static const string _timeStepTag;
 static const string _varNamesTag;
 static const string _vars3DTag;
 static const string _vars2DXYTag;
 static const string _vars2DXZTag;
 static const string _vars2DYZTag;
 static const string _xCoordsTag;
 static const string _yCoordsTag;
 static const string _zCoordsTag;
 static const string _periodicBoundaryTag;
 static const string _gridPermutationTag;

 // known xml attribute names
 //
 static const string _blockSizeAttr;
 static const string _dimensionLengthAttr;
 static const string _numTransformsAttr;
 static const string _filterCoefficientsAttr;
 static const string _liftingCoefficientsAttr;
 static const string _msbFirstAttr;
 static const string _vdfVersionAttr;
 static const string _numChildrenAttr;
 

 // Names of tags for user-defined data of type long, double, or string
 //
 vector <string> _userDLTags;	// top-level long tags
 vector <string> _userDDTags;
 vector <string> _userDSTags;

 vector <string> _timeStepUserDLTags;	// time step long tags
 vector <string> _timeStepUserDDTags;
 vector <string> _timeStepUserDSTags;

 vector <string> _variableUserDLTags;	// variable long tags
 vector <string> _variableUserDDTags;
 vector <string> _variableUserDSTags;

 int _init(
	const size_t dim[3], size_t numTransforms, size_t bs[3],
	int nFilterCoef = 1, int nLiftingCoef = 1, int msbFirst = 1,
	int vdfVersion = VDF_VERSION
	);

 int _SetNumTimeSteps(long value);
 int _setVariableTypes(
	const string &tag,
	const vector <string> &value,
	const vector <string> &delete_tags
 );
 int _SetVariableNames(XmlNode *node, long ts);
 int _RecordUserDataTags(vector <string> &keys, const string &tag);

bool elementStartHandler(ExpatParseMgr*, int depth , std::string& tag, const char **attr);
bool elementEndHandler(ExpatParseMgr*, int depth , std::string& );

 // XML Expat element handler helps. A different handler is defined
 // for each possible state (depth of XML tree) from 0 to 3
 //
 void _startElementHandler0(ExpatParseMgr*,const string &tag, const char **attrs);
 void _startElementHandler1(ExpatParseMgr*,const string &tag, const char **attrs);
 void _startElementHandler2(ExpatParseMgr*,const string &tag, const char **attrs);
 void _startElementHandler3(ExpatParseMgr*,const string &tag, const char **attrs);
 void _endElementHandler0(ExpatParseMgr*,const string &tag);
 void _endElementHandler1(ExpatParseMgr*,const string &tag);
 void _endElementHandler2(ExpatParseMgr*,const string &tag);
 void _endElementHandler3(ExpatParseMgr*,const string &tag);
 virtual int SetDefaults();	// Set some defaults 


};


};

#endif	//	_Metadata_h_
