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
#ifdef WIN32
#pragma warning(disable : 4251)
#endif

namespace VAPoR {

#define	CHK_TS(TS, RETVAL) \
	if (! _rootnode->GetChild(TS)) { \
		SetErrMsg("Invalid time step : %d", TS); \
		return(RETVAL); \
	}
#define	CHK_VAR(TS, VAR, RETVAL) \
	if (! _rootnode->GetChild(TS)) { \
		SetErrMsg("Invalid time step : %d", TS); \
		return(RETVAL); \
	}; \
	if (! _rootnode->GetChild(TS)->GetChild(VAR)) { \
		SetErrMsg("Invalid variable name : %s", VAR); \
		return(RETVAL); \
	}

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
//! The Meadata class is derived from the MyBase base
//!	class. Hence all of the methods make use of MyBase's
//!	error reporting capability - the success of any method
//!	(including constructors) can (and should) be tested
//!	with the GetErrCode() method. If non-zero, an error 
//!	message can be retrieved with GetErrMsg().
//!
class VDF_API Metadata : public VetsUtil::MyBase {
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
 //
 Metadata(
	const size_t dim[3], size_t numTransforms, size_t bs = 32, 
	int nFilterCoef = 1, int nLiftingCoef = 1, int msbFirst =  1
	);

 //! Create a metadata object from a metadata file stored on disk. 
 //!
 //! \param[in] path Path to metadata file
 //
 Metadata(const string &path);

 ~Metadata();

 //! Return the file path name to the metafile's parent directory.
 //! If the class was constructed with a path name, this method
 //! returns the parent directory of the path name. If the class
 //! was not constructed with a path name, NULL is returned.
 //! \retval dirname : parent directory or NULL
 //
 const char *GetParentDir() const { return (_metafileDirName); }

 //! Write the metadata object to a file
 //!
 //! \param[in] path Name of the file to write to
 //! \retval status Returns a non-negative integer on success
 //
 int Write(const string &path) const;


 //! Return the internal blocking factor use for WaveletBlock files
 //!
 //! \retval size Internal block factor
 //
 size_t GetBlockSize() const { return(_bs); }

 //! Returns the X,Y,Z coordinate dimensions of the data in grid coordinates
 //! \retval dim A three element vector containing the voxel dimension of 
 //! the data at its native resolution
 //!
 const size_t *GetDimension() const { return(_dim); }

 //! Returns the number of filter coefficients employed for wavelet transforms
 //! \retval _nFilterCoef Number of filter coefficients
 //
 int GetFilterCoef() const { return(_nFilterCoef); }

 //! Returns the number of lifting coefficients employed for wavelet transforms
 //! \retval _nLiftingCoef Number of lifting coefficients
 //
 int GetLiftingCoef() const { return(_nLiftingCoef); }

 //! Returns the number of wavelet transforms
 //! \param _numTransforms Number of transforms
 //
 int GetNumTransforms() const { return(_numTransforms); }

 //! Returns true if the storage order for data is most signicant byte first
 //! \retval _msbFirst Booean
 //
 int GetMSBFirst() const { return(_msbFirst); }


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
 //! \param[in] value Grid type. One of \b regular or \b stretched
 //! \retval status Returns a non-negative integer on success
 //
 int SetGridType(const string &value);

 //! Return the grid type.
 //!
 //! \retval type The grid type
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
		(VetsUtil::StrCmpNoCase(value,"stretched") == 0)
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
 //! \param[in] value A white-space delimited list of names
 //! \retval status Returns a non-negative integer on success
 //
 int SetVariableNames(const vector <string> &value);


 //! Return the names of the variables in the collection 
 //!
 //! \retval value is a space-separated list of variable names
 //
 const vector <string> &GetVariableNames() const {
	return(_varNames);
	};

 //! Set a global comment
 //!
 //! The comment is intended to refer to the entire data set
 //! \param[in] value A user defined comment
 //! \retval status Returns a non-negative integer on success
 //
 int SetComment(const string &value) {
	return(_rootnode->SetElementString(_commentTag, value));
	}

 //! Return the global comment, if it exists
 //!
 //! \retval value The global comment
 //
 const string &GetComment() const {
	return(_rootnode->GetElementString(_commentTag));
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
 //
 const vector<double> &GetTSUserTime(size_t ts) const {
	CHK_TS(ts, _emptyDoubleVec)
	return(_rootnode->GetChild(ts)->GetElementDouble(_userTimeTag));
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
 //! YZ sample plane.
 //! \sa SetGridType(), GetGridType(), GetTSXCoords()
 //
 const vector<double> &GetTSXCoords(size_t ts) const {
	CHK_TS(ts, _emptyDoubleVec)
	return(_rootnode->GetChild(ts)->GetElementDouble(_xCoordsTag));
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
	CHK_TS(ts, _emptyDoubleVec)
	return(_rootnode->GetChild(ts)->GetElementDouble(_yCoordsTag));
	}
 int IsValidYCoords(const vector<double> &value) const {
	return(value.size() == _dim[1]);
	}

 int SetTSZCoords(size_t ts, const vector<double> &value);
 const vector<double> &GetTSZCoords(size_t ts) const {
	CHK_TS(ts, _emptyDoubleVec)
	return(_rootnode->GetChild(ts)->GetElementDouble(_zCoordsTag));
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
 //! \retval comment A comment string
 //
 const string &GetTSComment(size_t ts) const;

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
 //! \retval comment A comment string
 //
 const string &GetVComment(size_t ts, const string &var) const;

 //! Return the base path for the variable, \p v, indicated by the time 
 //! step, \p ts, if it exists.  
 //!
 //! Paths to data files are constructed from the base path.
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \param[in] var A valid data set variable name
 //! \retval path Variable base path name
 //
 const string &GetVBasePath(size_t ts, const string &var) const;

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
 //
 const vector<double> &GetVDataRange(size_t ts, const string &var) const {
	CHK_VAR(ts, var, _emptyDoubleVec)
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
	return(_rootnode->SetElementLong(tag, value));
 }

 //! Get global, user-defined metadata
 //!
 //! Get the value(s) of a top-level (global), user-defined metadata tag of
 //! type \b long.
 //! 
 //! \param[in] tag Name of metadata tag
 //! \retval vector A vector of metadata values associated with \p tag. If
 //! \p tag is not defined by the metadata class, the vector returned 
 //! \sa GetUserDataLongTag(), SetUserDataLong()
 //
 const vector<long> &GetUserDataLong(const string &tag) const {
	return(_rootnode->GetElementLong(tag));
 }

 const vector<string> &GetUserDataDoubleTags() const {return(_userDDTags);}

 int SetUserDataDouble(const string &tag, const vector<double> &value) {
	_RecordUserDataTags(_userDDTags, tag);
	return(_rootnode->SetElementDouble(tag, value));
 }
 const vector<double> &GetUserDataDouble(const string &tag) const {
	return(_rootnode->GetElementDouble(tag));
 }

 const vector<string> &GetUserDataStringTags() const {return(_userDSTags);}

 int SetUserDataString(const string &tag, const string &value) {
	_RecordUserDataTags(_userDSTags, tag);
	return(_rootnode->SetElementString(tag, value));
 }
 const string &GetUserDataString(const string &tag) const {
	return(_rootnode->GetElementString(tag));
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
 //! \retval status Returns a non-negative integer on success
 //! \sa GetTSUserDataLongTag(), GetTSUserDataLong()
 //
 int SetTSUserDataLong(size_t ts, const string &tag, const vector<long> &value) {
	CHK_TS(ts, -1)
	_RecordUserDataTags(_timeStepUserDLTags, tag);
	return(_rootnode->GetChild(ts)->SetElementLong(tag, value));
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
 //! \sa GetTSUserDataLongTag(), SetTSUserDataLong()
 //
 const vector<long> &GetTSUserDataLong( size_t ts, const string &tag) const {
	CHK_TS(ts, _emptyLongVec)
	return(_rootnode->GetChild(ts)->GetElementLong(tag));
 }

 int SetTSUserDataDouble(
	size_t ts, const string &tag, const vector<double> &value
 ) {
	CHK_TS(ts, -1)
	_RecordUserDataTags(_timeStepUserDDTags, tag);
	return(_rootnode->GetChild(ts)->SetElementDouble(tag, value));
 }

 const vector<double> &GetTSUserDataDouble(size_t ts, const string &tag) const {
	CHK_TS(ts, _emptyDoubleVec)
	return(_rootnode->GetChild(ts)->GetElementDouble(tag));
 }
 const vector<string> &GetTSUserDataDoubleTags() const {
	return(_timeStepUserDDTags);
 }

 int SetTSUserDataString(
	size_t ts, const string &tag, const string &value
 ) {
	CHK_TS(ts, -1)
	_RecordUserDataTags(_timeStepUserDSTags, tag);
	return(_rootnode->GetChild(ts)->SetElementString(tag, value));
 }

 const string &GetTSUserDataString(size_t ts, const string &tag) const {
	CHK_TS(ts, _emptyString)
	return(_rootnode->GetChild(ts)->GetElementString(tag));
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
	CHK_VAR(ts, var, -1)
	_RecordUserDataTags(_variableUserDLTags, tag);
	return(_rootnode->GetChild(ts)->GetChild(var)->SetElementLong(tag, value));
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
 //! \sa GetTSUserDataLongTag(), SetTSUserDataLong()
 //
 const vector<long> &GetVUserDataLong(
	size_t ts, const string &var, const string &tag
 ) const {
	CHK_VAR(ts, var, _emptyLongVec)
	return(_rootnode->GetChild(ts)->GetChild(var)->GetElementLong(tag));
 }

 const vector<string> &GetVUserDataDoubleTags() const {
	return(_variableUserDDTags);
 }
 int SetVUserDataDouble(
	size_t ts, const string &var, const string &tag, const vector<double> &value
 ) {
	CHK_VAR(ts, var, -1)
	_RecordUserDataTags(_variableUserDDTags, tag);
	return(_rootnode->GetChild(ts)->GetChild(var)->SetElementDouble(tag, value));
 }

 const vector<double> &GetVUserDataDouble(
	size_t ts, const string &var, const string &tag
 ) const {
	CHK_VAR(ts, var, _emptyDoubleVec)
	if (! _rootnode->GetChild(ts)->GetChild(var)) return(_emptyDoubleVec);
	return(_rootnode->GetChild(ts)->GetChild(var)->GetElementDouble(tag));
 }

 int SetVUserDataString(
	size_t ts, const string &var, const string &tag, const string &value
 ) {
	CHK_VAR(ts,var,-1)
	_RecordUserDataTags(_variableUserDSTags, tag);
	return(_rootnode->GetChild(ts)->GetChild(var)->SetElementString(tag, value));
 }

 const string &GetVUserDataString(
	size_t ts, const string &var, const string &tag
 ) const {
	CHK_VAR(ts, var, _emptyString)
	return(_rootnode->GetChild(ts)->GetChild(var)->GetElementString(tag));
 }
 const vector<string> &GetVUserDataStringTags() const {
	return(_variableUserDSTags);
 }

private:
 XmlNode	*_rootnode;		// root node of the xml tree
 vector <string> _varNames;	// Names of all the field variables
 size_t _bs;				// blocking factor to be used by data
 size_t _dim[3];			// data dimensions
 int	_nFilterCoef;		// Lifting filter coefficients
 int	_nLiftingCoef;
 int	_numTransforms;		// Number of wavelet transforms
 int	_msbFirst;			// Most Significant Byte First storage order
 char	*_metafileDirName;	// path to metafile parent directory

 vector <double>	_emptyDoubleVec;
 vector <long>		_emptyLongVec;
 string 			_emptyString;

 // Structure used for parsing metadata files
 //
 class VDF_API _expatStackElement {
	public:
	string tag;			// xml element tag
	string data_type;	// Type of element data (string, double, or long)
	int has_data;		// does the element have data?
	int user_defined;	// is the element user defined?
 };
 stack	<VDF_API _expatStackElement *> _expatStateStack;

 XML_Parser _expatParser;	// XML Expat parser handle
 string _expatStringData;	// temp storage for XML element character data
 vector <long> _expatLongData;	// temp storage for XML long data
 vector <double> _expatDoubleData;	// temp storage for XML double  data
 string _expatCurrentVar;	// name of variable currently being processed
 long 	_expatCurrentTS;	// Number of time step currently being processed


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
 static const string _rootTag;
 static const string _userTimeTag;
 static const string _timeStepTag;
 static const string _varNamesTag;
 static const string _xCoordsTag;
 static const string _yCoordsTag;
 static const string _zCoordsTag;

 // known xml attribute names
 //
 static const string _blockSizeAttr;
 static const string _dimensionLengthAttr;
 static const string _numTransformsAttr;
 static const string _filterCoefficientsAttr;
 static const string _liftingCoefficientsAttr;
 static const string _msbFirstAttr;
 static const string _numChildrenAttr;
 static const string _typeAttr;

 // known xml attribute values
 //
 static const string _stringType;
 static const string _longType;
 static const string _doubleType;

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

 void _init(
	const size_t dim[3], size_t numTransforms, size_t bs = 32, 
	int nFilterCoef = 1, int nLiftingCoef = 1, int msbFirst = 1
	);

 int _SetNumTimeSteps(long value);
 int _SetVariableNames(XmlNode *node, long ts);
 int _RecordUserDataTags(vector <string> &keys, const string &tag);

 // XML Expat element handlers
 friend void	metadataStartElementHandler(
	void *userData, const XML_Char *tag, const char **attrs
 ) {
	Metadata *meta = (Metadata *) userData;
	meta->_startElementHandler(tag, attrs);
 }


 friend void metadataEndElementHandler(void *userData, const XML_Char *tag) {
	Metadata *meta = (Metadata *) userData;
	meta->_endElementHandler(tag);
 }

 friend void	metadataCharDataHandler(
	void *userData, const XML_Char *s, int len
 ) {
	Metadata *meta = (Metadata *) userData;
	meta->_charDataHandler(s, len);
 }


 void _startElementHandler(const XML_Char *tag, const char **attrs);
 void _endElementHandler(const XML_Char *tag);
 void _charDataHandler(const XML_Char *s, int len);

 // XML Expat element handler helps. A different handler is defined
 // for each possible state (depth of XML tree) from 0 to 3
 //
 void _startElementHandler0(const string &tag, const char **attrs);
 void _startElementHandler1(const string &tag, const char **attrs);
 void _startElementHandler2(const string &tag, const char **attrs);
 void _startElementHandler3(const string &tag, const char **attrs);
 void _endElementHandler0(const string &tag);
 void _endElementHandler1(const string &tag);
 void _endElementHandler2(const string &tag);
 void _endElementHandler3(const string &tag);

 // Report an XML parsing error
 //
 void _parseError(const char *format, ...);


};


};

#endif	//	_Metadata_h_
