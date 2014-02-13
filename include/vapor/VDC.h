#include <vector>
#include <map>
#include <iostream>
#include <vapor/MyBase.h>
#include <vapor/UDUnitsClass.h>

#ifndef	_VDC_H_
#define	_VDC_H_

namespace VAPoR {

//! \class VDC
//!
//! Defines API for reading, writing, and appending data to a
//! VAPOR Data Collection (Version 3).  The VDC class is an abstract virtual
//! class, providing a public API, but performing no actual storage 
//! operations. Derived implementations of the VDC base class are 
//! required to support the API.
//!
//! In version 3 of the VDC the metadata (.vdf) file is replaced with a 
//! "master" file that describes the contents of the entire VDC. The master 
//! file imposes structure on the organization of the files containing data,
//! determining, for example, which data files contain which variables 
//! and time steps. 
//!
//! Unlike
//! the .vdf file, it is intended that the master file will be stored in the 
//! same scientific data
//! file format as the data themselves (though this depends on the
//! implementation of the derived class). 
//! Another important change in version 3
//! is that both the master file and the accompanying data files 
//! are intended to be accessible using the native file format API. I.e.
//! users may operate on files in the VDC using, for example, the 
//! NetCDF API, or they
//! may use the API provided by the VDC class object. The latter is only
//! required when reading or writing compressed variables (not all variables
//! in a VDC version 3 must be compressed). Thus if NetCDF is chosen
//! as the underlying format the NetCDF API may be used directly to read 
//! and write NetCDF "attributes" and variables (provided the variables
//! are not compressed). 
//!
//! Variables in a VDC may have 1, 2, or 3 spatial dimensions, and 0 or 1
//! temporal dimensions.
//!
//! The VDC is structured in the spirit of the "NetCDF Climate and Forecast
//! (CF) Metadata Conventions", version 1.6, 5, December 2011.
//! It supports only a subset of the CF functionality (e.g. there is no
//! support for "Discrete Sampling Geometries"). Moreover, it is 
//! more restrictive than the CF in a number of areas. Particular
//! items of note include:
//!
//! \li All dimensions defined in the VDC have a 1D coordinate variable
//! associated with them with the same name as the dimension.
//!
//! \li The API supports variables with 1 to 4 dimensions only. 
//!
//! \li Coordinate variables representing time must be 1D
//!
//! \li Each dimension is associated with exactly one axis: \b X (or longitude)
//! \b Y (or latitude), \b Z (height), and \b T (time)
//!
//! \li All data variables have a "coordinate" attribute identifying 
//! the coordinate (or auxilliary coordinate) variables associated with
//! each axis
//!
//! \li To be consistent with VAPOR, when specified in vector form the 
//! ordering of dimension lengths
//! and dimension names is from fastest varying dimension to slowest.
//! For example, if
//! 'dims' is a vector of dimensions, then dims[0] is the fastest varying
//! dimension, dim[1] is the next fastest, and so on. This ordering is the
//! opposite of the ordering used by NetCDF.
//!
class VDC : public VetsUtil::MyBase {
public:

 //! Read, Write, Append access mode
 //!
 enum AccessMode {R,W,A};

 //! External storage types for primitive data
 //
 enum XType {INVALID = -1, FLOAT, DOUBLE, INT32, INT64, TEXT};

 //! \class Dimension
 //!
 //! \brief Metadata describing a coordinate dimension
 //!
 class Dimension {
 public:

  //! Default dimension constructor
  //!
  Dimension() {
	_name.clear();
	_length = 0;
	_axis = 0;
  };

  //! Dimension class constructor
  //!
  //! \param[in] name The name of dimension
  //! \param[in] length The dimension length
  //! \param[in] axis The dimension axis, an integer in the range 0..3 
  //! indicating X (or longitude), Y (latitude), Z (height), and T (time), 
  //! respectively.
  //!
  Dimension(std::string name, size_t length, int axis) { 
	_name = name;
	_length = length;
	_axis = axis;
  };
  virtual ~Dimension() {};

  void Clear() {
	_name.clear();
	_length = 0;
	_axis = 0;
  }

  //! Get dimension name
  //
  string GetName() const {return (_name); };

  //! Set dimension name
  //
  void SetName(string name) {_name = name;};

  //! Access dimension length
  //
  size_t GetLength() const {return (_length); };

  //! Set dimension length
  //
  void SetLength(size_t length) {_length = length;};

  //! Access dimension axis
  //
  int GetAxis() const {return (_axis); };

  //! Set dimension axis
  //
  void SetAxis(int axis) {_axis = axis;};

  friend std::ostream &operator<<(
	std::ostream &o, const Dimension &dimension
  );

 private:
  string _name;
  size_t _length;
  int _axis;
 };

 //! \class Attribute
 //!
 //! \brief Variable or global metadata
 //!
 class Attribute {
 public:
  Attribute() {_name = ""; _type = FLOAT; _values.clear(); };

  //! Attribute constructor
  //!
  //! \param[in] name The name of the attribute
  //! \param[in] type External representation format
  //! \param[in] values A vector specifying the attribute's values
  //
  Attribute(string name, XType type, const vector <float> &values);
  Attribute(string name, XType type, const vector <double> &values);
  Attribute(string name, XType type, const vector <int> &values);
  Attribute(string name, XType type, const vector <long> &values);
  Attribute(string name, XType type, const string &values);
  virtual ~Attribute() {};

  //! Get variable name
  //
  string GetName() const {return (_name); };

  //! Get an attribute's external representation type
  //
  XType GetXType() const {return (_type);};

  //! Get an attribute's value(s)
  //! 
  //! Get the value(s) for an attribute, performing type conversion
  //! as necessary from the external storage type to the desired type
  //!
  void GetValues(vector <float> &values) const;
  void GetValues(vector <double> &values) const;
  void GetValues(vector <int> &values) const;
  void GetValues(vector <long> &values) const;
  void GetValues(string &values) const;

  //! Set an attribute's value(s)
  //! 
  //! Set the value(s) for an attribute, performing type conversion
  //! as necessary to meet the external storage type.
  //!
  void SetValues(const vector <float> &values);
  void SetValues(const vector <double> &values);
  void SetValues(const vector <int> &values);
  void SetValues(const vector <long> &values);
  void SetValues(const string &values);

  friend std::ostream &operator<<(std::ostream &o, const Attribute &attr);

 private:
  string _name;
  XType _type;
  union podunion {
   float f;
   double d;
   int i;
   long l;
   char c;
  };
  vector <podunion> _values;
  
 };

  //! \class VarBase
  //!
  //! \brief Base class for storing variable metadata
  //
 class VarBase {
 public:

  //! Default constructor
  //!
  VarBase() {};

  //! Constructor 
  //!
  //! \param[in] name The variable's name
  //! \param[in] dimensions A vector specifying the variable's spatial 
  //! and/or temporal dimensions
  //! \param[in] units A string recognized by Udunits-2 specifying the
  //! unit measure for the variable. An empty string indicates that the
  //! variable is unitless.
  //! \param[in] type The external storage type for variable data
  //! \param[in] bs An ordered array specifying the storage 
  //! blocking
  //! factor for the variable. Results are undefined if the rank of 
  //! of \p bs does not match that of \p dimensions.
  //! dimensions only the needed elements of \p bs are accessed
  //! \param[in] wname The wavelet family name for compressed variables
  //! \param[in] wmode The wavelet bounary handling mode for compressed
  //! variables.
  //! \param[in] cratios Specifies a vector of compression factors for
  //! compressed variable definitions. If empty, the variable is not 
  //! compressed
  //! \param[in] periodic An ordered array of booleans 
  //! specifying the
  //! spatial boundary periodicity.
  //! Results are undefined if the rank of 
  //! of \p periodic does not match that of \p dimensions.
  //!
  VarBase(
	string name, vector <VDC::Dimension> dimensions,
	string units, XType type, 
	vector <size_t> bs, string wname, string wmode, vector <size_t> cratios,
	vector <bool> periodic
  ) :
	_name(name),
	_dimensions(dimensions),
	_units(units),
	_type(type),
	_wname(wname),
	_wmode(wmode),
	_cratios(cratios),
	_bs(bs),
	_periodic(periodic)
  {
  };

  virtual ~VarBase() {};

  //! Get variable name
  //
  string GetName() const {return (_name); };

  //! Access variable's dimension names
  //
  vector <VDC::Dimension> GetDimensions() const {return (_dimensions); };
  void SetDimensions(vector <VDC::Dimension> dimensions) {
	_dimensions = dimensions;
  };

  //! Access variable units
  //
  string GetUnits() const {return (_units); };
  void SetUnits(string units) {_units = units; };

  //! Access variable external storage type
  //
  XType GetXType() const {return (_type); };
  void SetXType(XType type) {_type = type; };

  //! Access variable's compression flag
  //
  bool GetCompressed() const {return (_cratios.size() > 1); };

  //! Access variable's block size
  //
  vector <size_t> GetBS() const {return (_bs); };
  void SetBS(vector <size_t> bs) {_bs = bs; };

  //! Access variable's wavelet family name
  //
  string GetWName() const {return (_wname); };
  void SetWName(string wname) {_wname = wname; };

  //! Access variable's wavelet boundary handling mode
  //
  string GetWMode() const {return (_wmode); };
  void SetWMode(string wmode) {_wmode = wmode; };

  //! Access variable's compression ratios
  //
  vector <size_t> GetCRatios() const {return (_cratios); };
  void SetCRatios(vector <size_t> cratios) {_cratios = cratios; };

  //! Access variable bounary periodic 
  //
  vector <bool> GetPeriodic() const {return (_periodic); };
  void SetPeriodic(vector <bool> periodic) { _periodic = periodic; };

  //! Access variable attributes
  //
  std::map <string, Attribute> GetAttributes() const {return (_atts); };
  void SetAttributes(std::map <string, Attribute> &atts) {_atts = atts; };

  //! Get number of refinement levels
  //! 
  //! Returns an ordered list of the number of available refinement levels 
  //! for each spatial dimension. For data that are not compressed
  //! the number of refinement levels along all dimensions is 1.
  //!
  vector <size_t> GetRefinementLevels();

  friend std::ostream &operator<<(std::ostream &o, const VarBase &var);
  
 private:
  string _name;
  vector <VDC::Dimension> _dimensions;
  string _units;
  XType _type;
  string _wname;
  string _wmode;
  vector <size_t> _cratios;
  vector <size_t> _bs;
  vector <bool> _periodic;
  std::map <string, Attribute> _atts;
 };

 //! \class CoordVar
 //! \brief Coordinate variable metadata
 //
 class CoordVar : public VarBase {
 public:

  //! Default Coordinate Variable metadata constructor
  //
  CoordVar() : VarBase() {};


  //! Construct Data variable definition with missing values
  //!
  //! \copydetails VarBase(string name, vector <VDC::Dimension> dimensions,
  //!  string units, XType type, bool compressed,
  //!  vector <size_t> bs, string wname, string wmode, vector <size_t> cratios,
  //!  vector <bool> periodic)
  //!
  //! \param[in] axis an int in the range 0..3 indicating the coordinate
  //! axis, one of X, Y, Z, or T, respectively
  //! \param[in] uniform A bool indicating whether the coordinate variable
  //! is uniformly sampled.
  //
  CoordVar(
	string name, vector <VDC::Dimension> dimensions,
	string units, XType type, 
	vector <size_t> bs, string wname, string wmode, vector <size_t> cratios,
	vector <bool> periodic, int axis, bool uniform
  ) :
	VarBase(
		name, dimensions, units, type, bs, 
		wname, wmode, cratios,
		periodic
	),
	_axis(axis),
	_uniform(uniform)
 {}
  virtual ~CoordVar() {};

  //! Access coordinate variable axis
  //
  int GetAxis() const {return (_axis); };
  void SetAxis(int axis) {_axis = axis; };

  //! Access coordinate variable uniform sampling flag
  //
  bool GetUniform() const {return (_uniform); };
  void SetUniform(bool uniform) {_uniform = uniform; };

  friend std::ostream &operator<<(std::ostream &o, const CoordVar &var);

 private:
  int _axis;
  bool _uniform;
 };

 //! \class DataVar
 //! \brief Data variable metadata
 //!
 //! This class defines metadata associatd with a Data variable
 //!
 class DataVar : public VarBase {
 public:

  //! constructor for default Data variable definition
  //
  DataVar() : VarBase() {};

  //! Construct Data variable definition with missing values
  //!
  //! \copydetails VarBase(string name, vector <VDC::Dimension> dimensions,
  //!  string units, XType type, 
  //!  vector <size_t> bs, string wname, string wmode, vector <size_t> cratios,
  //!  vector <bool> periodic)
  //!
  //! \param[in] coordvars Names of coordinate variables associated 
  //! with this variables dimensions
  //! \param[in] missing_value  Value of the missing value indicator
  //!
  DataVar(
	string name, vector <VDC::Dimension> dimensions,
	string units, XType type, 
	vector <size_t> bs, string wname, string wmode, vector <size_t> cratios,
	vector <bool> periodic, vector <string> coordvars, 
	double missing_value
  ) :
	VarBase(
		name, dimensions, units, type, 
		bs, wname, wmode, cratios, periodic
	),
	_coordvars(coordvars),
	_has_missing(true),
	_missing_value(missing_value)
  {}

  //! Construct Data variable definition without missing values
  //!
  //! \copydetails VarBase(string name, vector <VDC::Dimension> dimensions,
  //!  string units, XType type, 
  //!  vector <size_t> bs, string wname, string wmode, vector <size_t> cratios,
  //!  vector <bool> periodic)
  //!
  //! \param[in] coordvars Names of coordinate variables associated 
  //! with this variables dimensions
  //!
  DataVar(
	string name, vector <VDC::Dimension> dimensions,
	string units, XType type, 
	vector <size_t> bs, string wname, string wmode, vector <size_t> cratios,
	vector <bool> periodic, vector <string> coordvars
  ) :
	VarBase(
		name, dimensions, units, type, 
		bs, wname, wmode, cratios, periodic
	),
	_coordvars(coordvars),
	_has_missing(false),
	_missing_value(0.0)
  {}
  virtual ~DataVar() {};

  //! Access data variable's coordinate variable names
  //
  vector <string> GetCoordvars() const {return (_coordvars); };
  void SetCoordvars(vector <string> coordvars) {_coordvars = coordvars; };

  //! Access data variable's missing data flag
  //
  bool GetHasMissing() const {return (_has_missing); };
  void SetHasMissing(bool has_missing) {_has_missing = has_missing; };

  //! Access data variable's missing data value
  //
  double GetMissingValue() const {return (_missing_value); };
  void SetMissingValue(double missing_value) {_missing_value = missing_value; };

  friend std::ostream &operator<<(std::ostream &o, const DataVar &var);

 private:
  vector <string> _coordvars;
  bool _has_missing;
  double _missing_value;
 };



 //! Class constuctor
 //!
 //!
 VDC();
 virtual ~VDC() {}

 //! Initialize the VDC class
 //!
 //! Prepare a VDC for reading or writing/appending. This method prepares
 //! the master VDC file indicated by \p path for reading or writing.
 //! The method should be called before any other class methods. This method
 //! exists only because C++ constructors can not return error codes.
 //!
 //! \param[in] path Path name of file that contains, or will
 //! contain, the VDC master file for this data collection
 //! \param[in] mode One of \b R, \b W, or \b A, indicating whether \p path
 //! will be opened for reading, writing, or appending, respectively. 
 //! When \p mode is \b A underlying NetCDF files will be opened
 //! opened with \em nc_open(path, NC_WRITE)). When \p mode is \b W
 //! NetCDF files will be created (opened with \em nc_create(path)).
 //! When \p mode is \b A additional time steps may be added to
 //! an existing file.
 //!
 //! \note The parameter \p mode controls the access to the master
 //! file indicated by \p path and the variable data files in a somewhat
 //! unintuitive manner.  If \p mode is \b R or \b A the master file \p path
 //! must already exist. If \p mode is
 //! \b A or \b W the contents of the VDC master may be changed (written) 
 //! and the 
 //! VDC is put into \b define mode until EndDefine() 
 //! is called. While in \b define mode metadata that will be contained
 //! in the VDC master file may be changed, but coordinate and data variables
 //! may not be accessed (read or written). Similarly, when not in define 
 //! mode coordinate
 //! and data variables may be accessed (read or written), but metadata
 //! in the VDC master may not be changed. See OpenVariableRead() and
 //! OpenVariableWrite() for discussion on how \p mode effects reading
 //! and writing of coordinate and data variables.
 //!
 //! \retval status A negative int is returned on failure
 //!
 //! \sa EndDefine();
 //
 virtual int Initialize(string path, AccessMode mode);

 //! Sets various parameters for storage blocks for subsequent variable 
 //! definitions
 //!
 //! This method sets the storage parameters for subsequent variable
 //! definitions. \p bs is a three-element array, with the first element
 //! specifying the length of the fastest varying dimension (e.g. X) of
 //! the storage block, the
 //! second element specifies the length of the next fastest varying
 //! dimension, etc. If a variable definition defines a variable with \b n
 //! dimensions, where \b n is less than three, only the first \b n elements
 //! of \p bs will be used. For example, a 2D variable will be stored in
 //! blocks having dimensions \b bs[0] x \b bs[1].
 //!
 //! Variables whose spatial dimensions are less than the coresponding 
 //! dimension of \p bs will be padded to block boundaries.
 //!
 //! \p wname and \p wmode set the wavelet family name and 
 //! boundary handling mode
 //! for subsequent compressed variable definitions.
 //! Wider wavelets (those requiring
 //! more filter coefficients) will typically yield higher compression rates,
 //! but are more computationally expensive and will limit the depth of 
 //! of the grid resolution refinement hierarchy. 
 //! 
 //! Recommended values for \p wname are \e bior1.1, \e bior1.3, \e bior1.5
 //! \e bior3.3, \e bior3.5, \e bior3.7, \e bior3.9, \e bior2.2, \e bior2.6,
 //! \e bior2.6, and \e bior2.8. For odd length filters (e.g. bior1.3) 
 //! \p wmode should be set to \e symh. For even (e.g. bior2.4), use \e symw
 //!
 //! Finally, \p cratios specifies a vector of compression factors for
 //! subsequent compressed variable definitions. 
 //!
 //! \note For compressed variables compression is applied to individual blocks.
 //! Larger blocks permit deeper grid refinement hierarchies, but may
 //! result in poor cache performance and slowed disk storage access
 //!
 //! \note The wavelet and compression ratio parameters are ignored by variable 
 //! definitions for variables that are not compressed.
 //!
 //!
 //! \param[in] bs A three-element array specifying the storage block size. All
 //! elements of \p must be great than or equal to one. The default value
 //! of \p bs is (64, 64, 64).
 //! \param[in] wname A wavelet family name. The default value is "bior3.3".
 //! \param[in] wmode The wavelet boundary handling mode. The default
 //! value is "symh".
 //! \param[in] cratios A vector of compression of integer compression
 //! factors.
 //! The default compression ratio vector is: (1, 10, 100, 500)
 //!
 //! \retval status A negative int is returned if an invalid parameter
 //! or parameter combination is specified.
 //!
 //! \sa DefineDataVar(), DefineCoordVar(), VDC()
 //
 int SetCompressionBlock(
	vector <size_t> bs, string wname, string wmode,
	vector <size_t> cratios
 );
 
 //! Retrieve current compression block settings.
 //!
 //! \sa SetCompressionBlock()
 //
 void GetCompressionBlock(
	vector <size_t> &bs, string &wname, string &wmode,
	vector <size_t> &cratios
 ) const;

 //! Set the boundary periodic for subsequent variable definitions
 //!
 //! This method specifies an ordered, three-element boolean vector 
 //! indicating the boundary periodicty for a variable's spatial 
 //! dimensions. The ordering is from fastest to slowest varying dimension.
 //!
 //! \param[in] periodic A three-element array of booleans. The
 //! default value of \p periodic is (\b false, \b false, \b false).
 //!
 //! \retval status A negative int is returned on error
 //!
 void SetPeriodicBoundary(vector <bool> periodic) {
	_periodic = periodic;
	for (int i=_periodic.size(); i<3; i++) _periodic.push_back(false);
 }

 //! Retrieve current boundary periodic settings
 //!
 //! \sa SetPeriodicBoundary()
 //
 vector <bool> GetPeriodicBoundary() const { return(_periodic); };

 //! Define a dimension in the VDC
 //!
 //! This method specifies the name, length, and axis of a dimension.
 //! A variable in the VDC may have one to four dimensions (one to 
 //! three spatial, and zero or one temporal).  Dimensions may be of 
 //! any length
 //! greater than or equal to one. 
 //!
 //! This method also defines a 1D VDC coordinate variable with the same
 //! name as the dimension, \p dimname. The coordinate variable will 
 //! be defined to be unitless, have uniform sampling, an external
 //! data type of \b FLOAT, and not compressed.
 //! The units, external data type, and uniformity of this 
 //! coordinate variable can subsequently
 //! be redefined with DefineCoordVar() or DefineCoordVarUniform().
 //! Thus for each dimension a 1D coordinate variable of the same name
 //! must exist.
 //! 
 //! This method must be called prior to defining any variables requring
 //! the defined dimensions.
 //!
 //! There are no default dimensions defined.
 //!
 //! It is an error to call this method if the VDC master is not currently
 //! in \b define mode.
 //!
 //! \param[in] dimname A string specifying the name of the dimension. 
 //! \param[in] length The dimension length, which must be greater than zero. 
 //! \param[in] axis The axis associated with the dimension. Acceptable 
 //! values are \b 0 (for X or longitude),
 //! \b 1 (for Y or latitude), \b 2 (for Z or vertical), and \b 3 (for time).
 //!
 //! \note When the VDC master file is initialized in 
 //! append (\b mode = \b A)
 //! mode it is an error to redefine an
 //! existing dimension. New dimensions may, however, be defined.
 //!
 //! \retval status A negative int is returned on error
 //!
 //! \sa DefineCoordVar(), DefineDataVar(), GetDimension()
 //
 int DefineDimension(string dimname, size_t length, int axis);

 //! Return a dimensions's definition
 //!
 //! This method returns the length and axis for the dimension
 //! named by \p dimname. If \p dimname is not defined as a dimension
 //! \p length and \p axis will both be set to zero, and false returned.
 //!
 //! \param[in] dimname A string specifying the name of the dimension. 
 //! \param[in] reflevel The refinement level
 //! \param[out] length The dimension length, which must be greater than zero. 
 //! \param[out] axis The axis associated with the dimension. 
 //! \retval bool If the named dimension can not be found false is returned.
 //! 
 //! \note Need to define meaning of \p reflevel. If it's kept compatible 
 //! with VDC2 then we are precluded from having varying numbers of 
 //! refinement levels along different dimensions
 //!
 bool GetDimension(
	string dimname, int reflevel, size_t &length, int &axis
 ) const;

 //! Return a dimensions's definition
 //!
 //! This method returns the definition of the dimension named
 //! by \p dimname as a reference to a VDC::Dimension object. If
 //! \p dimname is not defined as a dimension then the name of \p dimension
 //! will be the empty string()
 //!
 //! \param[in] dimname A string specifying the name of the dimension. 
 //! \param[in] reflevel The refinement level
 //! \param[out] dimension The returned Dimension object reference
 //! \retval bool If the named dimension can not be found false is returned.
 //!
 bool GetDimension(
	string dimname, int reflevel, VDC::Dimension &dimension
 ) const;

 //! Return names of all defined dimensions
 //!
 //! This method returns the list of names of all of the dimensions
 //! defined in the VDC.
 //!
 //! \sa DefineDimension()
 //!
 vector <string> GetDimensionNames() const;


 //! Define a coordinate variable
 //!
 //! This method provides the definition for a coordinate variable: a 
 //! variable providing spatial or temporal coordinates for a subsequently
 //! defined data variable.
 //!
 //! If the variable's name, \p varname, matches a dimension defined
 //! with DefineDimension(), only the units and external data type
 //! may differ from the 1D coordinate variable impliclity defined by
 //! DefineDimension() 
 //!
 //! \param[in] varname The name of the coordinate variable. 
 //! \param[in] dimnames An ordered vector specifying the variables
 //! dimension names. The dimension names must have previously be defined
 //! with the DefineDimensions() method. 
 //! \param[in] units This parameter specifies a string describing the 
 //! units of measure for the
 //! variable. The string is compatible with the udunits2 conversion
 //! package. If the quantity is unitless an empty string may be specified.
 //! \param[in] axis An integer indicating the spatial or temporal 
 //! coordinate axis. Acceptable values are \b 0 (for X or longitude), 
 //! \b 1 (for Y or latitude), \b 2 (for Z or vertical), and \b 3 (for time).
 //! \param[in] type The primitive data type storage format. 
 //! Currently supported values
 //! are \b FLOAT. This is the type that will be used to store the 
 //! variable on disk
 //! \param[in] compressed A boolean indicating whether the 
 //! coordinate variable is
 //! to be wavelet transformed.
 //!
 //! It is an error to call this method if the VDC master is not currently
 //! in \b define mode.
 //! 
 //! \note Temporal coordinate variables (axis=3) must have exactly one
 //! dimension.
 //!
 //! \note When in append (\b A) mode it is an error to redefine an
 //! existing variable.
 //!
 //! \retval status A negative int is returned on error
 //!
 //! \sa DefineDimensions(), DefineCoordVar(), SetCompressionBlock()
 //
 int DefineCoordVar(
	string varname, vector <string> dimnames, 
	string units, int axis, XType type, bool compressed
 );


 //! Define a coordinate variable with uniform sampling
 //!
 //! This method provides the definition for a uniform coordinate variable. 
 //! A uniformly sampled coordinate variable is a 1D variable
 //! for which the coordinates may be given by <em> i * dx </em>, where
 //! \em i is an index starting from zero, and \em dx is a real number
 //! representing the spacing between points.
 //!
 //! One-dimensional coordinate variables that have uniform sampling
 //! should be declared as such using this method rather than the 
 //! more general DefineCoordVar().
 //!
 //! \param[in] varname The name of the coordinate variable. 
 //! \param[in] dimname An string specifying the variable's
 //! dimension name. The dimension names must have previously be defined
 //! with the DefineDimensions() method. 
 //! \param[in] units This parameter specifies a string describing the 
 //! units of measure for the
 //! variable. The string is compatible with the udunits2 conversion
 //! package. If the quantity is unitless an empty string may be specified.
 //! \param[in] axis An integer indicating the spatial or temporal 
 //! coordinate axis. Acceptable values are \b 0 (for X or longitude), 
 //! \b 1 (for Y or latitude), \b 2 (for Z or vertical), and \b 3 (for time).
 //! \param[in] type The primitive data type storage format.
 //! Currently supported values
 //! are \b FLOAT
 //! \param[in] compressed A boolean indicating whether the 
 //! coordinate variable is
 //! to be wavelet transformed.
 //!
 //! It is an error to call this method if the VDC master is not currently
 //! in \b define mode.
 //! 
 //! \note When in append (\b A) mode it is an error to redefine an
 //! existing variable.
 //!
 //! \retval status A negative int is returned on error
 //!
 //! \sa DefineDimensions(), DefineCoordVar(), SetCompressionBlock()
 //
 int DefineCoordVarUniform(
	string varname, string dimname, 
	string units, int axis, XType type, bool compressed
 );

 //! Return a coordinate variable's definition
 //!
 //! This method returns the definition for the coordinate 
 //! variable named by \p varname.
 //! If \p varname is not defined as a coordinate variable
 //! \p dimnames will be set to a zero-length vector, and values of all other
 //! output parameters will be undefined.
 //!
 //! \param[in] varname A string specifying the name of the dimension. 
 //! \param[out] dimnames The ordered list of dimension names for this variable
 //! \param[out] units The variable's units string
 //! \param[out] axis The axis associated with the dimension. 
 //! \param[out] type The external data storage type
 //! \param[out] compressed A boolean indicating if the variable is compressed
 //! \param[out] uniform A boolean indicating if the variable has uniform
 //! sampling
 //! \retval bool If the named coordinate variable cannot be found false 
 //! is returned.
 //!
 //! \sa DefineCoordVar(), DefineCoordVarUniform()
 //!
 bool GetCoordVar(
	string varname, vector <string> &dimnames, 
	string &units, int &axis, XType &type, bool &compressed, bool &uniform
 ) const;

 //! Return a coordinate variable's definition
 //!
 //! Return a reference to a VDC::CoordVar object describing 
 //! the coordinate variable named by \p varname
 //!
 //! If \p varname is not defined the NULL pointer is returned.
 //!
 //! \param[in] varname A string specifying the name of the coordinate 
 //! variable. 
 //! \param[out] coordvar A CoordVar object containing the definition
 //! of the named variable.
 //! \retval bool False is returned if the named coordinate variable does 
 //! not exist
 //!
 //! \sa DefineCoordVar(), DefineCoordVarUniform(), SetCompressionBlock(),
 //! SetPeriodicBoundary()
 //!
 bool GetCoordVar(string varname, VDC::CoordVar &cvar) const;

 //! Define a data variable
 //!
 //! This method defines a data variable in the VDC master file
 //!
 //! \param[in] varname The name of the coordinate variable. 
 //! \param[in] dimnames An ordered vector specifying the variables
 //! dimension names. The dimension names must have previously be defined
 //! with the DefineDimensions() method. 
 //! \param[in] coordvars An ordered vector specifying the coordinate
 //! variable names providing the coordinates for this variable. The 
 //! dimension names must have previously been defined
 //! with the DefineCoordVar() method. Moreover, the dimension names
 //! of each coordinate variable must be a subset of those in \p dimnames.
 //! \param[in] units This parameter specifies a string describing the 
 //! units of measure for the
 //! variable. The string is compatible with the udunits2 conversion
 //! package. If the quantity is unitless an empty string may be specified.
 //! \param[in] type The primitive data type storage format. 
 //! Currently supported values
 //! are \b FLOAT
 //! \param[in] compressed A boolean indicating whether the coordinate 
 //! variable is to be wavelet transformed.
 //!
 //! It is an error to call this method if the VDC master is not currently
 //! in \b define mode.
 //!
 //! \note When in append (\b A) mode it is an error to redefine an
 //! existing variable.
 //!
 //! \sa DefineDimensions(), DefineCoordVar(), SetCompressionBlock()
 //!
 int DefineDataVar(
	string varname, vector <string> dimnames, vector <string> coordvars, 
	string units, XType type, bool compressed
 );

 //! Define a data variable with missing values 
 //!
 //! \param[in] missing_value The missing value
 //!
 //! \sa DefineDataVar()
 //!
 int DefineDataVar(
	string varname, vector <string> dimnames, vector <string> coordvars, 
	string units, XType type, bool compressed,
	double missing_value
 );

 //! Return a data variable's definition
 //!
 //! This method returns the definition for the data 
 //! variable named by \p varname.
 //! If \p varname is not defined as a coordinate variable
 //! \p dimnames will be set to a zero-length vector, and values of all other
 //! output parameters will be undefined.
 //!
 //! \param[in] varname A string specifying the name of the dimension. 
 //! \param[out] dimnames An ordered list of dimension names for this variable
 //! \param[out] coordvars An ordered list of coordinate names for this variable
 //! \param[out] units The variable's units string
 //! \param[out] type The external data storage type
 //! \param[out] compressed A boolean indicating if the variable is compressed
 //! \param[out] has_missing A boolean indicating if the variable has missing
 //! values
 //! \param[out] missing_value If \p has_missing is true this parameter
 //! contains the value of variable's missing value maker. If \p has_missing
 //! is false the value of \p missing_value is undefined.
 //
 //! \retval bool False is returned if the named data variable does 
 //! not exist
 //!
 //! \sa DefineCoordVar(), DefineCoordVarUniform()
 //!
 bool GetDataVar(
	string varname, vector <string> &dimnames, vector <string> &coordvars, 
	string &units, XType &type, bool &compressed,
	bool &has_missing, double &missing_value
 ) const;

 //! Return a data variable's definition
 //!
 //! Return a reference to a VDC::DataVar object describing 
 //! the data variable named by \p varname
 //!
 //! If \p varname is not defined the NULL pointer is returned.
 //!
 //! \param[in] varname A string specifying the name of the variable. 
 //! \param[out] datavar A DataVar object containing the definition
 //! of the named Data variable.
 //! \retval bool False is returned if the named Data variable does 
 //! not exist
 //!
 //! \sa DefineCoordVar(), DefineCoordVarUniform(), SetCompressionBlock(),
 //! SetPeriodicBoundary()
 //!
 bool GetDataVar( string varname, VDC::DataVar &datavar) const;


 //! Return a list of names for all of the defined data variables.
 //!
 //! Returns a list of names for all data variables defined 
 //!
 //! \sa DefineDataVar()
 //
 virtual std::vector <string> GetDataVarNames() const;

 //! Return a list of data variables with a given dimension rank
 //!
 //! Returns a list of all data variables defined having a 
 //! dimension rank of \p ndim. If \p spatial is true, only the spatial
 //! dimension rank of the variable is compared against \p ndim
 //!
 //! \param[in] ndim Rank of spatial dimensions
 //! \param[in] spatial Only compare spatial dimensions against \p ndim
 //!
 //! \sa DefineDataVar()
 //
 virtual std::vector <string> GetDataVarNames(int ndim, bool spatial) const;

 //! Return a list of names for all of the defined coordinate variables.
 //!
 //! Returns a list of names for all coordinate variables defined 
 //!
 //! \sa DefineDataVar()
 //
 virtual std::vector <string> GetCoordVarNames() const;

 //
 //! Return a list of coordinate variables with a given dimension rank
 //!
 //! Returns a list of all coordinate variables defined having a 
 //! dimension rank of \p ndim. If \p spatial is true, only the spatial
 //! dimension rank of the variable is compared against \p ndim
 //!
 //! \param[in] ndim Rank of spatial dimensions
 //! \param[in] spatial Only compare spatial dimensions against \p ndim
 //!
 //! \sa DefineCoordVar()
 //
 virtual std::vector <string> GetCoordVarNames(int ndim, bool spatial) const;

 //! Return a boolean indicating whether a variable is time varying
 //!
 //! This method returns \b true if the variable named by \p varname is defined
 //! and it has a time axis dimension. If either of these conditions
 //! is not true the method returns false.
 //!
 //! \param[in] varname A string specifying the name of the variable. 
 //! \retval bool Returns true if variable \p varname exists and is 
 //! time varying.
 //!
 bool IsTimeVarying(string varname) const;

 //! Return a boolean indicating whether a variable is compressed
 //!
 //! This method returns \b true if the variable named by \p varname is defined
 //! and it has a compressed representation. If either of these conditions
 //! is not true the method returns false.
 //!
 //! \param[in] varname A string specifying the name of the variable. 
 //! \retval bool Returns true if variable \p varname exists and is 
 //! compressed
 //!
 //! \sa DefineCoordVar() DefineDataVar()
 //
 bool IsCompressed(string varname) const;

 //! Return the time dimension length for a variable
 //!
 //! Returns the number of time steps (length of the time dimension)
 //! for which a variable is defined. If \p varname does not have a 
 //! time coordinate 0 is returned. If \p varname is not defined 
 //! as a variable a negative int is returned.
 //!
 //! \param[in] varname A string specifying the name of the variable. 
 //! \retval[out] count The length of the time dimension, or a negative
 //! int if \p varname is undefined.
 //!
 int GetNumTimeSteps(string varname) const;

 //! Return a boolean indicating whether a variable is a data variable 
 //!
 //! This method returns \b true if a data variable is defined
 //! with the name \p varname.  Otherwise the method returns false.
 //!
 //! \retval bool Returns true if \p varname names a defined data variable
 //!
 bool IsDataVar(string varname) const {
	return(_dataVars.find(varname) != _dataVars.end());
 }

 //! Return a boolean indicating whether a variable is a coordinate variable 
 //!
 //! This method returns \b true if a coordinate variable is defined
 //! with the name \p varname.  Otherwise the method returns false.
 //!
 //! \retval bool Returns true if \p varname names a defined coordinate 
 //! variable
 //!
 bool IsCoordVar(string varname) const {
	return(_coordVars.find(varname) != _coordVars.end());
 }

 //! Write an attribute
 //!
 //! This method write an attribute to the VDC. The attribute can either
 //! be "global", if \p varname is the empty string, or bound to a variable
 //! if \p varname indentifies a variable in the VDC. 
 //!
 //! \param[in] varname The name of a variable already defined in the VDC,
 //! or the empty string if the attribute is to be global
 //! \param[in] attname The attributes name
 //! \param[in] type The primitive data type storage format. 
 //! This is the type that will be used to store the 
 //! attribute on disk
 //! \param[in] values A vector of floating point attribute values
 //!
 //! \retval status A negative int is returned on failure
 //!
 //! \sa GetAtt()
 //
 int PutAtt(
	string varname, string attname, XType type, const vector <double> &values
 );
 int PutAtt(
	string varname, string attname, XType type, const vector <long> &values
 );
 int PutAtt(
	string varname, string attname, XType type, const string &values
 );

 //! Read an attribute
 //!
 //! This method reads an attribute from the VDC. The attribute can either
 //! be "global", if \p varname is the empty string, or bound to a variable
 //! if \p varname indentifies a variable in the VDC.
 //! 
 //! \param[in] varname The name of the variable the attribute is bound to,
 //! or the empty string if the attribute is global
 //! \param[in] attname The attributes name
 //! \param[out] type The primitive data type storage format. 
 //! This is the type that will be used to store the 
 //! attribute on disk
 //! \param[out] values A vector to contain the returned floating point 
 //! attribute values
 //!
 //! \retval status A negative int is returned on failure
 //!
 //! \sa PutAtt()
 //
 int GetAtt(
	string varname, string attname, vector <double> &values
 ) const;
 int GetAtt(
	string varname, string attname, vector <long> &values
 ) const;
 int GetAtt(
	string varname, string attname, string &values
 ) const;

 //! Return a list of available attribute's names
 //!
 //! Returns a vector of all attribute names for the
 //! variable, \p varname. If \p varname is the empty string the names
 //! of all of the global attributes are returned. If \p varname is 
 //! not defined an empty vector is returned.
 //!
 //! \param[in] varname The name of the variable to query,
 //! or the empty string if the names of global attributes are desired.
 //! \retval attnames A vector of returned attribute names
 //!
 //! \sa PutAtt(), GetAtt()
 //
 std::vector <string> GetAttNames(string varname) const;

 //! Return the external data type for an attribute
 //!
 //! Returns the external storage type of the named variable attribute.
 //!
 //! \param[in] varname The name of the variable to query,
 //! or the empty string if the names of global attributes are desired.
 //! \param[in] name Name of the attribute.
 //!
 //! \retval If an attribute named by \p name does not exist, a
 //! negative value is returned.
 //!
 XType GetAttType(string varname, string attname) const;

 //!
 //! When the open mode \b mode is \b A or \b W this method signals the 
 //! class object that metadata defintions have been completed and it 
 //! commits them to the master VDC file. This method also prepares 
 //! the VDC for the reading or writing of variable or coordinate data. 
 //!
 //! Ignored if \b mode is \b R.
 //!
 //! \retval status A negative it is returned if the master file is not
 //! successfully written for any reason
 //!
 //! \note The master file should not be accessed with the native file
 //! format API (e.g. NetCDF) if the VDC is in define mode (e.g. until
 //! after EndDefine() is called).
 //!
 //! \sa VDC() 
 //!
 int EndDefine();

 //! Return the path name and temporal offset for a variable
 //!
 //! Data and coordinate variables in a VDC are in general distributed 
 //! into multiple files. For example, for large variables only a single 
 //! time step
 //! may be stored per file. 
 //! This method returns the file path name, \p path, of the file 
 //! containing \p varname at time step \p ts. Also returned is the 
 //! integer time offset of the variable within \p path.
 //!
 //! \param[in] varname Data or coordinate variable name.
 //! \param[in] ts Integer offset relative to a variable's temporal dimension
 //! \param[in] lod Approximation level of the variable. A value of -1
 //! indicates the maximum approximation level defined for the VDC
 //! \param[out] path Path to file containing variable \p varname at 
 //! time step \p ts.
 //! \param[out] file_ts Temporal offset of variable \p varname in file
 //! \p path.
 //!
 //! \retval status A negative int is returned if \p varname or 
 //! \p ts are invalid, or if the class object is in define mode.
 //!
 virtual int GetPath(
	string varname, size_t ts, int lod, string &path, size_t &file_ts
 ) const = 0;

 //! Open the named variable for reading
 //!
 //! This method prepares a data or coordinate variable, indicated by a
 //! variable name and time step pair, for subsequent read operations by
 //! methods of this class.  The number of the refinement levels
 //! parameter, \p reflevel, indicates the resolution of the volume in
 //! the multiresolution hierarchy. The valid range of values for
 //! \p reflevel is [0..n-1], where \p n is the
 //! maximum refinement level of the variable. 
 //!
 //! A value of zero indicates the
 //! coarsest resolution data, a value of \p n (or -1) indicates
 //! the
 //! finest resolution data.
 //! \note Need to change refinement level definition (0 =>native resolution)
 //! if we want to support varying refinement levels along each dimension for
 //! data such as global atm/ocean where only few vertical levels are 
 //! present.
 //!
 //! The level-of-detail parameter, \p lod, selects
 //! the approximation level. Valid values for \p lod are integers in
 //! the range 0..n-1, where \e n is returned by 
 //! VDC::VarBase::GetCRatios().size(), or the value -1 may be used
 //! to select the best approximation available. 
 //!
 //! An error occurs, indicated by a negative return value, if the
 //! volume identified by the {varname, timestep, reflevel, lod} tupple
 //! is not available. Note the availability of a volume can be tested
 //! with the VariableExists() method.
 //!
 //! \param[in] ts Time step of the variable to read. This is the integer
 //! offset into the variable's temporal dimension. If the variable
 //! does not have a temporal dimension \p ts is ignored.
 //! \param[in] varname Name of the variable to read
 //! \param[in] reflevel Refinement level of the variable. A value of -1
 //! indicates the maximum refinment level defined for the VDC
 //! \param[in] lod Approximation level of the variable. A value of -1
 //! indicates the maximum approximation level defined for the VDC
 //! \retval status Returns a non-negative value on success
 //!
 //!
 int OpenVariableRead(size_t ts, string varname, int reflevel=0, int lod=-1);

 //! Close the currently opened read variable
 //!
 //! Close the handle for variable opened with OpenVariableRead()
 //!
 int CloseVariableRead();

 //! Open the named variable for writing
 //!
 //! This method prepares a data or coordinate variable, indicated by a
 //! variable name and time step pair, for subsequent write operations by
 //! methods of this class.  The number of the refinement levels
 //! parameter, \p reflevel, indicates the resolution of the volume in
 //! the multiresolution hierarchy. The valid range of values for
 //! \p reflevel is [0..n-1], where \p n is the
 //! maximum refinement level of the variable. 
 //!
 //! The behavior of this method is impacted somewhat by the setting
 //! of the Initialize() \b mode parameter. Coordinate or data variable 
 //! files may be 
 //! written regardless of the \p mode setting. However, if 
 //! \p mode is \b W the first time a coordinate or data file is written
 //! it will be created (opened with \em nc_create(path), for example) 
 //! regareless of whether the file previously existed. If \p
 //! mode is \b A or \b R existing coordinate or data files will be opened
 //! for appending (e.g. opened with \em nc_open(path, NC_WRITE)). New
 //! files will be created (opened with \em nc_create(path)).
 //!
 //! An error occurs, indicated by a negative return value, if the
 //! varible identified by the {varname, timestep, reflevel, lod} tupple
 //! is not defined. 
 //!
 //! \param[in] ts Time step of the variable to read. This is the integer
 //! offset into the variable's temporal dimension. If the variable
 //! does not have a temporal dimension \p ts is ignored.
 //! \param[in] varname Name of the variable to read
 //! \param[in] lod Approximation level of the variable. A value of -1
 //! indicates the maximum approximation level defined for the VDC
 //! \retval status Returns a non-negative value on success
 //!
 //
 int OpenVariableWrite(size_t ts, string varname, int lod=-1);

 //! Close the currently opened write variable
 //!
 //! Close the handle for variable opened with OpenVariableWrite()
 //!
 int CloseVariableWrite();


 //! Write all spatial values to the currently opened variable
 //!
 //! This method writes, and compresses as necessary, 
 //!  the contents of the array contained in 
 //! \p region to the currently opened variable. The number of values
 //! written from \p region is given by the product of the spatial 
 //! dimensions of the open variable at the refinement level specified.
 //!
 //! \param[in] region An array of data to be written
 //! \retval status Returns a non-negative value on success
 //!
 //! \sa OpenVariableWrite()
 //
 int Write(const float *region);

 //! Write a single slice of data to the currently opened variable
 //!
 //! Transform and write a single slice (2D array) of data to the variable
 //! indicated by the most recent call to OpenVariableWrite().
 //! The dimensions of a slices are NX by NY,
 //! where NX is the dimension of the array along the fastest varying
 //! spatial dimension, specified
 //! in grid points, and NY is the length of the second fastest varying
 //! dimension.
 //!
 //! This method should be called exactly NZ times for each opened variable,
 //! where NZ is the dimension of third, and slowest varying dimension.
 //! In the case of a 2D variable, NZ is 1.
 //!
 //! \param[in] slice A 2D slice of data
 //! \retval status Returns a non-negative value on success
 //!
 //! \sa OpenVariableWrite()
 //!
 int WriteSlice(const float *slice);

 //! Read all spatial values of the currently opened variable
 //!
 //! This method reads, and decompresses as necessary, 
 //!  the contents of the currently opened variable into the array 
 //! \p region. The number of values
 //! read into \p region is given by the product of the spatial 
 //! dimensions of the open variable at the refinement level specified.
 //!
 //! It is the caller's responsibility to ensure \p region points
 //! to adequate space.
 //!
 //! \param[out] region An array of data to be written
 //! \retval status Returns a non-negative value on success
 //!
 //! \sa OpenVariableWrite()
 //
 int Read(float *region);

 //! Read a single slice of data from the currently opened variable
 //!
 //! Inverse transform, as necessary, and read a single slice (2D array) of 
 //! data from the variable
 //! indicated by the most recent call to OpenVariableRead().
 //! The dimensions of a slices are NX by NY,
 //! where NX is the dimension of the array along the fastest varying
 //! spatial dimension, specified
 //! in grid points, and NY is the length of the second fastest varying
 //! dimension.
 //!
 //! This method should be called exactly NZ times for each opened variable,
 //! where NZ is the dimension of third, and slowest varying dimension.
 //! In the case of a 2D variable, NZ is 1.
 //!
 //! It is the caller's responsibility to ensure \p region points
 //! to adequate space.
 //!
 //! \param[out] slice A 2D slice of data
 //! \retval status Returns a non-negative value on success
 //!
 //! \sa OpenVariableWrite()
 //!
 int ReadSlice(float *slice);

 //! Read in and return a subregion from the currently opened
 //! variable
 //!
 //! This method reads and returns a subset of variable data.
 //! The \p min and \p max vectors whose dimensions match the
 //! spatial rank of the currently opened variable, identifying the minimum and
 //! maximum extents, in grid coordinates, of the subregion of interest. The
 //! minimum and maximum valid values of an element of \b min or \b max 
 //! are \b 0 and
 //! \b n-1, respectively, where \b n is the length of the associated
 //! dimension at the opened refinement level. 
 //!
 //! The region
 //! returned is stored in the memory region pointed to by \p region. It
 //! is the caller's responsbility to ensure adequate space is available.
 //!
 //! \param[in] min Minimum region extents in grid coordinates
 //! \param[in] max Maximum region extents in grid coordinates
 //! \param[out] region The requested volume subregion
 //!
 //! \retval status Returns a non-negative value on success
 //! \sa OpenVariableRead(), GetDimension(), GetDimensionNames()
 //
 virtual int ReadRegion(
    const size_t min[3], const size_t max[3], float *region
 );
 

 //! Write an entire variable in one call
 //!
 //! This method writes and entire variable (all time steps, all grid points)
 //! into a VDC.  This is the simplest interface for writing data into
 //! a VDC. If the variable is split across multiple files PutVar()
 //! ensures that the data are correctly distributed.
 //! Any variables currently opened with OpenVarWrite() are first closed.
 //! Thus variables need not be opened with OpenVarWrite() prior to
 //! calling PutVar();
 //!
 //! It is an error to call this method in \b define mode
 //!
 //! \param[in] varname Name of the variable to write
 //! \param[in] data Pointer from where the data will be copied
 //! 
 //! \retval status A negative int is returned on failure
 //!
 //! \sa GetVar()
 //
 int PutVar(string varname, const float *data);
 
 //! Write a variable at single time step
 //!
 //! This method writes a variable hyperslab consisting of the
 //! variable's entire spatial dimensions at the time step
 //! indicated by \p ts.
 //! Any variables currently opened with OpenVarWrite() are first closed.
 //! Thus variables need not be opened with OpenVarWrite() prior to
 //! calling PutVar();
 //!
 //! It is an error to call this method in \b define mode
 //!
 //! \param[in] ts Time step of the variable to write. This is the integer
 //! offset into the variable's temporal dimension. If the variable
 //! does not have a temporal dimension \p ts is ignored.
 //! \param[in] varname Name of the variable to write
 //! \param[in] data Pointer from where the data will be copied
 //! 
 //! \retval status A negative int is returned on failure
 //!
 //! \sa GetVar()
 //
 int PutVar(size_t ts, string varname, const float *data);

 //! Read an entire variable in one call
 //!
 //! This method reads and entire variable (all time steps, all grid points)
 //! from a VDC.  This is the simplest interface for reading data from
 //! a VDC. If the variable is split across multiple files GetVar()
 //! ensures that the data are correctly gathered and assembled into memory
 //! Any variables currently opened with OpenVarRead() are first closed.
 //! Thus variables need not be opened with OpenVarRead() prior to
 //! calling GetVar();
 //!
 //! It is an error to call this method in \b define mode
 //!
 //! \param[in] varname Name of the variable to write
 //! \param[out] data Pointer to where data will be copied. It is the 
 //! caller's responsbility to ensure \p data points to sufficient memory.
 //! 
 //! \retval status A negative int is returned on failure
 //!
 //! \sa PutVar()
 //
 int GetVar(string varname, float *data);

 //! Read an entire variable at a given time step in one call
 //!
 //! This method reads and entire variable (all grid points) at 
 //! time step \p ts
 //! from a VDC.  This is the simplest interface for reading data from
 //! a VDC. 
 //! Any variables currently opened with OpenVarRead() are first closed.
 //! Thus variables need not be opened with OpenVarRead() prior to
 //! calling GetVar();
 //!
 //! It is an error to call this method in \b define mode
 //!
 //! \param[in] ts Time step of the variable to write. This is the integer
 //! offset into the variable's temporal dimension. If the variable
 //! does not have a temporal dimension \p ts is ignored.
 //! \param[in] varname Name of the variable to write
 //! \param[out] data Pointer to where data will be copied. It is the 
 //! caller's responsbility to ensure \p data points to sufficient memory.
 //! 
 //! \retval status A negative int is returned on failure
 //!
 //! \sa PutVar()
 //
 int GetVar(size_t ts, string varname, float *data);

 friend std::ostream &operator<<(std::ostream &o, const VDC &vdc);

protected:
 string _master_path;
 AccessMode _mode;
 bool _defineMode;
 std::vector <size_t> _bs;
 string _wname;
 string _wmode;
 std::vector <size_t> _cratios;
 vector <bool> _periodic;
 VAPoR::UDUnits _udunits;

 std::map <string, Dimension> _dimsMap;
 std::map <string, Attribute> _atts;
 std::map <string, CoordVar> _coordVars;
 std::map <string, DataVar> _dataVars;

 bool _ValidCompressBlock(
    vector <size_t> bs, string wname, string wmode,
    vector <size_t> cratios
 ) const;

 bool _ValidDefineDimension(string name, size_t length, int axis) const;

 bool _ValidDefineCoordVar(
    string varname, vector <string> dimnames,
    string units, int axis, XType type, bool compressed
 ) const;

 bool _ValidDefineDataVar(
    string varname, vector <string> dimnames, vector <string> coordnames,
    string units, XType type, bool compressed
 ) const;

 virtual int _WriteMasterMeta() = 0;
 virtual int _ReadMasterMeta() = 0;

};
};

#endif

//TBD
//	coordinate systems (spherical and cartesian)
