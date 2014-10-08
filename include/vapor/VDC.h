#include <vector>
#include <map>
#include <iostream>
#include <vapor/MyBase.h>
#include <vapor/UDUnitsClass.h>

#ifndef	_VDC_H_
#define	_VDC_H_

namespace VAPoR {

//!
//! \class VDC
//! \ingroup Public_VDC
//!
//! \brief Defines API for reading, writing, and appending data to a
//! VAPOR Data Collection (Version 3)
//!
//! \author John Clyne
//! \date    July, 2014
//!
//! This abstract class efines API for reading, writing, and 
//! appending data to a
//! VAPOR Data Collection (Version 3).  The VDC class is an abstract virtual
//! class, providing a public API, but performing no actual storage 
//! operations. Derived implementations of the VDC base class are 
//! required to support the API.
//!
//! In version 3 of the VDC the metadata (.vdf) file found in VDC version 1
//! and 2 is replaced with a 
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
//! This class inherits from VetsUtil::MyBase. Unless otherwise documented
//! any method that returns an integer value is returning status. A negative
//! value indicates failure. Error messages are logged via
//! VetsUtil::MyBase::SetErrMsg(). Methods that return a boolean do
//! not, unless otherwise documented, log an error message upon 
//! failure (return of false).
//!
//! \param level 
//! \parblock
//! Grid refinement level for multiresolution variables. 
//! Compressed variables in the VDC have a multi-resolution 
//! representation: the sampling grid for multi-resolution variables
//! is hierarchical, and the dimension lengths of adjacent levels in the
//! hierarchy differ by a factor of two. The \p level parameter is 
//! used to select a particular depth of the hierarchy.
//!
//! To provide maximum flexibility as well as compatibility with previous
//! versions of the VDC the interpretation of \p level is somewhat 
//! complex. Both positive and negative values may be used to specify
//! the refinement level and have different interpretations. 
//!
//! For positive
//! values of \p level, a value of \b 0 indicates the coarsest 
//! member of the 
//! grid hierarchy. A value of \b 1 indicates the next grid refinement 
//! after the coarsest, and so on. Using postive values the finest level 
//! in the hierarchy is given by GetNumRefLevels() - 1. Values of \p level
//! that are greater than GetNumRefLevels() - 1 are treated as if they
//! were equal to GetNumRefLevels() - 1.
//!
//! For negative values of \p level a value of -1 indicates the
//! variable's native grid resolution (the finest resolution available). 
//! A value of -2 indicates the next coarsest member in the hierarchy after 
//! the finest, and so
//! on. Using negative values the coarsest available level in the hierarchy is 
//! given by negating the value returned by GetNumRefLevels(). Values of
//! \p level that are less than the negation of GetNumRefLevels() are
//! treated as if they were equal to the negation of the GetNumRefLevels()
//! return value.
//!
//! \param lod
//! The level-of-detail parameter, \p lod, selects
//! the approximation level for a compressed variable. 
//! The \p lod parameter is similar to the \p level parameter in that it
//! provides control over accuracy of a compressed variable. However, instead
//! of selecting the grid resolution the \p lod parameter controls 
//! the compression factor by indexing into the \p cratios vector (see below).
//! As with the \p level parameter, both positive and negative values may be 
//! used to index into \p cratios and 
//! different interpretations. 
//! 
//! For positive
//! values of \p lod, a value of \b 0 indicates the 
//! the first element of \p cratios, a value of \b 1 indicates
//! the second element, and so on up to the size of the 
//! \p cratios vector (See VDC::GetCRatios()).
//!
//! For negative values of \p lod a value of \b -1 indexes the
//! last element of \p cratios, a value of \b -2 indexes the 
//! second to last element, and so on.
//! Using negative values the first element of \p cratios - the greatest 
//! compression rate - is indexed by negating the size of the 
//! \p cratios vector.
//!
//! \param cratios A monotonically decreasing vector of
//! compression ratios. Compressed variables in the VDC are stored
//! with a fixed, finite number of compression factors. The \p cratios
//! vector is used to specify the available compression factors (ratios). 
//! A compression factor of 1 indicates no compression (1:1). A value
//! of 2 indciates two to one compression (2:1), and so on. The minimum
//! valid value of \p cratios is \b 1. The maximum value is determined
//! by a number of factors and can be obtained using the CompressionInfo()
//! method.
//!
//! \param bs An ordered list of block dimensions that specifies the
//! block decomposition of the variable. The rank of \p bs may be less
//! than that of a variable's array dimensions, in which case only
//! the \b n fastest varying variable dimensions will be blocked, where
//! \b n is the rank of \p bs. The ordering of the dimensions in \p bs
//! is from fastest to slowest. A block is the basic unit of compression
//! in the VDC: variables are decomposed into blocks, and individual blocks
//! are compressed independently.
//!
//! \param wname Name of wavelet used for transforming compressed 
//! variables between wavelet and physical space. Valid values
//! are "bior1.1", "bior1.3", "bior1.5", "bior2.2", "bior2.4",
//! "bior2.6", "bior2.8", "bior3.1", "bior3.3", "bior3.5", "bior3.7",
//! "bior3.9", "bior4.4"
//!
//! \endparblock
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
  Attribute(string name, XType type, const std::vector <float> &values);
  Attribute(string name, XType type, const std::vector <double> &values);
  Attribute(string name, XType type, const std::vector <int> &values);
  Attribute(string name, XType type, const std::vector <long> &values);
  Attribute(string name, XType type, const string &values);
  Attribute(string name, XType type) {
	_name = name; _type = type; _values.clear(); 
  };
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
  void GetValues(std::vector <float> &values) const;
  void GetValues(std::vector <double> &values) const;
  void GetValues(std::vector <int> &values) const;
  void GetValues(std::vector <long> &values) const;
  void GetValues(string &values) const;

  //! Set an attribute's value(s)
  //! 
  //! Set the value(s) for an attribute, performing type conversion
  //! as necessary to meet the external storage type.
  //!
  void SetValues(const std::vector <float> &values);
  void SetValues(const std::vector <double> &values);
  void SetValues(const std::vector <int> &values);
  void SetValues(const std::vector <long> &values);
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
  std::vector <podunion> _values;
  
 };

  //! \class BaseVar
  //!
  //! \brief Base class for storing variable metadata
  //
 class BaseVar {
 public:

  //! Default constructor
  //!
  BaseVar() {};

  //! Constructor 
  //!
  //! \param[in] name The variable's name
  //! \param[in] dimensions An ordered vector specifying the variable's spatial 
  //! and/or temporal dimensions
  //! \param[in] units A string recognized by Udunits-2 specifying the
  //! unit measure for the variable. An empty string indicates that the
  //! variable is unitless.
  //! \param[in] type The external storage type for variable data
  //! \param[in] bs An ordered array specifying the storage 
  //! blocking
  //! factor for the variable. Results are undefined if the rank of 
  //! of \p bs does not match that of \p dimensions spatial dimensions.
  //! dimensions only the needed elements of \p bs are accessed
  //! \param[in] wname The wavelet family name for compressed variables
  //! \param[in] cratios Specifies a vector of compression factors for
  //! compressed variable definitions. If empty, or if cratios.size()==1 
  //! and cratios[0]==1, the variable is not 
  //! compressed
  //! \param[in] periodic An ordered array of booleans 
  //! specifying the
  //! spatial boundary periodicity.
  //! Results are undefined if the rank of 
  //! of \p periodic does not match that of \p dimensions.
  //!
  BaseVar(
	string name, std::vector <VDC::Dimension> dimensions,
	string units, XType type, 
	std::vector <size_t> bs, string wname, 
	std::vector <size_t> cratios, std::vector <bool> periodic
  ) :
	_name(name),
	_dimensions(dimensions),
	_units(units),
	_type(type),
	_wname(wname),
	_cratios(cratios),
	_bs(bs),
	_periodic(periodic)
  {
	if (_cratios.size()==0) _cratios.push_back(1);
  };

  virtual ~BaseVar() {};

  //! Get variable name
  //
  string GetName() const {return (_name); };
  void SetName(string name) {_name = name; };

  //! Access variable's dimension names
  //
  std::vector <VDC::Dimension> GetDimensions() const {return (_dimensions); };
  void SetDimensions(std::vector <VDC::Dimension> dimensions) {
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

  //! Return true if the number of avaiable compression ratios are 
  //! not equal to one or the number is one the compression ratio
  //! itself is not equal to one
  //
  bool GetCompressed() const {
	return (!
		(_cratios.size()==0 || (_cratios.size()==1 && (_cratios[0]==1)))
	); 
  };

  //! Access variable's block size
  //
  std::vector <size_t> GetBS() const {return (_bs); };
  void SetBS(std::vector <size_t> bs) {_bs = bs; };

  //! Access variable's wavelet family name
  //
  string GetWName() const {return (_wname); };
  void SetWName(string wname) {_wname = wname; };

  //! Access variable's compression ratios
  //
  std::vector <size_t> GetCRatios() const {return (_cratios); };
  void SetCRatios(std::vector <size_t> cratios) {
	_cratios = cratios;
	if (_cratios.size()==0) _cratios.push_back(1);
  };

  //! Access variable bounary periodic 
  //
  std::vector <bool> GetPeriodic() const {return (_periodic); };
  void SetPeriodic(std::vector <bool> periodic) { _periodic = periodic; };

  //! Access variable attributes
  //
  std::map <string, Attribute> GetAttributes() const {return (_atts); };
  void SetAttributes(std::map <string, Attribute> &atts) {_atts = atts; };

  friend std::ostream &operator<<(std::ostream &o, const BaseVar &var);
  
 private:
  string _name;
  std::vector <VDC::Dimension> _dimensions;
  string _units;
  XType _type;
  string _wname;
  std::vector <size_t> _cratios;
  vector <size_t> _bs;
  std::vector <bool> _periodic;
  std::map <string, Attribute> _atts;
 };

 //! \class CoordVar
 //! \brief Coordinate variable metadata
 //
 class CoordVar : public BaseVar {
 public:

  //! Default Coordinate Variable metadata constructor
  //
  CoordVar() : BaseVar() {};


  //! Construct Data variable definition with missing values
  //!
  //! \copydetails BaseVar(string name, std::vector <VDC::Dimension> dimensions,
  //!  string units, XType type, bool compressed,
  //!  vector <size_t> bs, string wname, 
  //!  std::vector <size_t> cratios,
  //!  std::vector <bool> periodic)
  //!
  //! \param[in] axis an int in the range 0..3 indicating the coordinate
  //! axis, one of X, Y, Z, or T, respectively
  //! \param[in] uniform A bool indicating whether the coordinate variable
  //! is uniformly sampled.
  //
  CoordVar(
	string name, std::vector <VDC::Dimension> dimensions,
	string units, XType type, 
	std::vector <size_t> bs, string wname, 
	std::vector <size_t> cratios, std::vector <bool> periodic, 
	int axis, bool uniform
  ) :
	BaseVar(
		name, dimensions, units, type, bs, 
		wname, cratios,
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
 class DataVar : public BaseVar {
 public:

  //! constructor for default Data variable definition
  //
  DataVar() : BaseVar() {};

  //! Construct Data variable definition with missing values
  //!
  //! \copydetails BaseVar(string name, std::vector <VDC::Dimension> dimensions,
  //!  string units, XType type, 
  //!  std::vector <size_t> bs, string wname,
  //!  std::vector <size_t> cratios,
  //!  std::vector <bool> periodic)
  //!
  //! \param[in] coordvars Names of coordinate variables associated 
  //! with this variables dimensions
  //! \param[in] missing_value  Value of the missing value indicator
  //!
  DataVar(
	string name, std::vector <VDC::Dimension> dimensions,
	string units, XType type, 
	std::vector <size_t> bs, string wname,
	std::vector <size_t> cratios,
	std::vector <bool> periodic, std::vector <string> coordvars, 
	double missing_value
  ) :
	BaseVar(
		name, dimensions, units, type, 
		bs, wname, cratios, periodic
	),
	_coordvars(coordvars),
	_has_missing(true),
	_missing_value(missing_value)
  {}

  //! Construct Data variable definition without missing values
  //!
  //! \copydetails BaseVar(string name, std::vector <VDC::Dimension> dimensions,
  //!  string units, XType type, 
  //!  std::vector <size_t> bs, string wname,
  //!  std::vector <size_t> cratios,
  //!  vector <bool> periodic)
  //!
  //! \param[in] coordvars Names of coordinate variables associated 
  //! with this variables dimensions
  //!
  DataVar(
	string name, std::vector <VDC::Dimension> dimensions,
	string units, XType type, 
	std::vector <size_t> bs, string wname,
	std::vector <size_t> cratios,
	std::vector <bool> periodic, std::vector <string> coordvars
  ) :
	BaseVar(
		name, dimensions, units, type, 
		bs, wname, cratios, periodic
	),
	_coordvars(coordvars),
	_has_missing(false),
	_missing_value(0.0)
  {}
  virtual ~DataVar() {};

  //! Access data variable's coordinate variable names
  //
  std::vector <string> GetCoordvars() const {return (_coordvars); };
  void SetCoordvars(std::vector <string> coordvars) {_coordvars = coordvars; };

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
  std::vector <string> _coordvars;
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
 //! The method should be called immediately after the constructor, 
 //! before any other class methods. This method
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
 //! definitions for compressed variables. 
 //! \p bs is a three-element array, with the first element
 //! specifying the length of the fastest varying dimension (e.g. X) of
 //! the storage block, the
 //! second element specifies the length of the next fastest varying
 //! dimension, etc. If a variable definition defines a variable with \b n
 //! spatial dimensions, where \b n is less than three, only the 
 //! first \b n elements
 //! of \p bs will be used. For example, if the rank of \b bs is greater than
 //! two a 2D variable will be stored in
 //! blocks having dimensions \b bs[0] x \b bs[1].
 //!
 //! Variables whose spatial dimension lengths are less than the coresponding 
 //! dimension of \p bs will be padded to block boundaries.
 //!
 //! \p wname set the wavelet family name and 
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
 //! \param[in] bs A one to three-element array specifying the storage 
 //! block size. All
 //! elements of \p must be great than or equal to one. The default value
 //! of \p bs is (64, 64, 64).
 //! \param[in] wname A wavelet family name. The default value is "bior3.3".
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
	std::vector <size_t> bs, string wname, std::vector <size_t> cratios
 );
 
 //! Retrieve current compression block settings.
 //!
 //! \param[out] bs An ordered vector containing the current compression 
 //! block dimensions.
 //! \param[out] wname The wavelet family name. 
 //! \param[out] cratios A vector of compression of integer compression
 //! factors.
 //!
 //! \sa SetCompressionBlock()
 //
 void GetCompressionBlock(
	std::vector <size_t> &bs, string &wname, 
	std::vector <size_t> &cratios
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
 void SetPeriodicBoundary(std::vector <bool> periodic) {
	_periodic = periodic;
	for (int i=_periodic.size(); i<3; i++) _periodic.push_back(false);
 }

 //! Retrieve current boundary periodic settings
 //!
 //! \sa SetPeriodicBoundary()
 //
 std::vector <bool> GetPeriodicBoundary() const { return(_periodic); };

 //! Define a dimension in the VDC
 //!
 //! This method specifies the name, length, and axis of a dimension.
 //! A variable in the VDC may have one to four dimensions (one to 
 //! three spatial, and zero or one temporal).  Dimensions may be of 
 //! any length
 //! greater than or equal to one. 
 //!
 //! This method also defines a 1D VDC coordinate variable with the same
 //! name as the dimension, \p dimname. The implicitly defined
 //! coordinate variable will 
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
 //! \param[out] length The dimension length, which must be greater than zero. 
 //! \param[out] axis The axis associated with the dimension. 
 //! \retval bool If the named dimension can not be found false is returned.
 //!
 bool GetDimension(
	string dimname, size_t &length, int &axis
 ) const;

 //! Return a dimensions's definition
 //!
 //! This method returns the definition of the dimension named
 //! by \p dimname as a reference to a VDC::Dimension object. If
 //! \p dimname is not defined as a dimension then the name of \p dimension
 //! will be the empty string()
 //!
 //! \param[in] dimname A string specifying the name of the dimension. 
 //! \param[out] dimension The returned Dimension object reference
 //! \retval bool If the named dimension can not be found false is returned.
 //!
 bool GetDimension(
	string dimname, VDC::Dimension &dimension
 ) const;

 //! Return names of all defined dimensions
 //!
 //! This method returns the list of names of all of the dimensions
 //! defined in the VDC.
 //!
 //! \sa DefineDimension()
 //!
 std::vector <string> GetDimensionNames() const;


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
 //! dimension names. The dimension names must have previously been defined
 //! with the DefineDimension() method. 
 //! \param[in] units This parameter specifies a string describing the 
 //! units of measure for the
 //! variable. The string is compatible with the Unidata udunits2 conversion
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
 //! \sa DefineDimension(), DefineCoordVar(), SetCompressionBlock()
 //
 int DefineCoordVar(
	string varname, std::vector <string> dimnames, 
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
 //! with the DefineDimension() method. 
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
 //! \sa DefineDimension(), DefineCoordVar(), SetCompressionBlock()
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
 //! \param[in] varname A string specifying the name of the variable. 
 //! \param[out] dimnames The ordered list of dimension names for this variable
 //! \param[out] units The variable's units string
 //! \param[out] axis The axis associated with the dimension. 
 //! \param[out] type The external data storage type
 //! \param[out] compressed A boolean indicating if the variable is compressed
 //! \param[out] uniform A boolean indicating if the variable has uniform
 //! sampling
 //! \retval bool If the named coordinate variable cannot be found false 
 //! is returned and the values of the output parameters will be 
 //! undefined.
 //!
 //! \sa DefineCoordVar(), DefineCoordVarUniform()
 //!
 bool GetCoordVarInfo(
	string varname, std::vector <string> &dimnames, 
	string &units, int &axis, XType &type, bool &compressed, bool &uniform
 ) const;

 //! Return a coordinate variable's definition
 //!
 //! Return a reference to a VDC::CoordVar object describing 
 //! the coordinate variable named by \p varname
 //!
 //! \param[in] varname A string specifying the name of the coordinate 
 //! variable. 
 //! \param[out] coordvar A CoordVar object containing the definition
 //! of the named variable.
 //! \retval bool False is returned if the named coordinate variable does 
 //! not exist, and the contents of cvar will be undefined.
 //!
 //! \sa DefineCoordVar(), DefineCoordVarUniform(), SetCompressionBlock(),
 //! SetPeriodicBoundary()
 //!
 bool GetCoordVarInfo(string varname, VDC::CoordVar &cvar) const;

 //! Define a data variable
 //!
 //! This method defines a data variable in the VDC master file
 //!
 //! \param[in] varname The name of the data variable. 
 //! \param[in] dimnames An ordered vector specifying the variables
 //! dimension names. The dimension names must have previously be defined
 //! with the DefineDimension() method. 
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
 //! \sa DefineDimension(), DefineCoordVar(), SetCompressionBlock()
 //!
 int DefineDataVar(
	string varname, std::vector <string> dimnames, 
	std::vector <string> coordvars, 
	string units, XType type, bool compressed
 );

 //! Define a data variable with missing values 
 //!
 //! \param[in] missing_value The missing value
 //!
 //! \sa DefineDataVar()
 //!
 int DefineDataVar(
	string varname, std::vector <string> dimnames, 
	std::vector <string> coordvars, 
	string units, XType type, bool compressed,
	double missing_value
 );

 //! Return a data variable's definition
 //!
 //! This method returns the definition for the data 
 //! variable named by \p varname.
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
 //!
 //! \retval bool If the named data variable cannot be found false 
 //! is returned and the values of the output parameters will be 
 //! undefined.
 //!
 //! \sa DefineCoordVar(), DefineCoordVarUniform()
 //!
 bool GetDataVarInfo(
	string varname, std::vector <string> &dimnames, 
	std::vector <string> &coordvars, 
	string &units, XType &type, bool &compressed,
	bool &has_missing, double &missing_value
 ) const;

 //! Return a data variable's definition
 //!
 //! Return a reference to a VDC::DataVar object describing 
 //! the data variable named by \p varname
 //!
 //! \param[in] varname A string specifying the name of the variable. 
 //! \param[out] datavar A DataVar object containing the definition
 //! of the named Data variable.
 //!
 //! \retval bool If the named data variable cannot be found false 
 //! is returned and the values of \p datavar are undefined.
 //!
 //! \sa DefineCoordVar(), DefineCoordVarUniform(), SetCompressionBlock(),
 //! SetPeriodicBoundary()
 //!
 bool GetDataVarInfo( string varname, VDC::DataVar &datavar) const;
 
 //! Return metadata about a data or coordinate variable
 //!
 //! If the variable \p varname is defined as either a 
 //! data or coordinate variable its metadata will
 //! be returned in \p var.
 //!
 //! \retval bool If the named variable cannot be found false 
 //! is returned and the values of \p var are undefined.
 //!
 //! \sa GetDataVarInfo(), GetCoordVarInfo()
 //
 bool GetBaseVarInfo(string varname, VDC::BaseVar &var) const;


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
 //! \param[in] ndim Rank of dimensions for comparision
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
 //! \param[in] ndim Rank of dimensions for comparision
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
 //! \sa DefineCoordVar(), DefineDataVar(), VDC::BaseVar::GetCompressed()
 //
 bool IsCompressed(string varname) const;

 //! Return the time dimension length for a variable
 //!
 //! Returns the number of time steps (length of the time dimension)
 //! for which a variable is defined. If \p varname does not have a 
 //! time coordinate 1 is returned. If \p varname is not defined 
 //! as a variable a negative int is returned.
 //!
 //! \param[in] varname A string specifying the name of the variable. 
 //! \retval count The length of the time dimension, or a negative
 //! int if \p varname is undefined.
 //!
 //! \sa IsTimeVarying()
 //
 int GetNumTimeSteps(string varname) const;

 //! Return the number of refinement levels for the indicated variable
 //!
 //! Compressed variables have a multi-resolution grid representation.
 //! This method returns the number of levels in the hiearchy. A value
 //! of one indicates that only the native resolution is available. 
 //! A value of two indicates that two levels, the native plus the
 //! next coarsest are available, and so on.
 //!
 //! \param[in] varname Data or coordinate variable name.
 //!
 //! \retval num If \p varname is unknown zero is returned. if \p varname
 //! is not compressed (has no multi-resolution representation) one is
 //! returned. Otherwise the total number of levels in the multi-resolution
 //! hierarchy are returned.
 //
 int GetNumRefLevels(string varname) const;

 //! Return the compression ratio vector for the indicated variable
 //!
 //! Return the compression ratio vector for the indicated variable. 
 //! The vector returned contains an ordered list of available 
 //! compression ratios for the variable named by \p variable. 
 //! If the variable is not compressed, the \p cratios parameter will
 //! contain a single element, one.
 //!
 //! \param[in] varname Data or coordinate variable name.
 //! \param[out] cratios Ordered vector of compression ratios
 //!
 //! \retval status A negative int is returned on failure
 //
int GetCRatios(string varname, vector <size_t> &cratios) const;


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

 //! Parse a vector of VDC::Dimensions into space and time dimensions
 //!
 //! This is a convenience utility that parses an ordered 
 //! vector of Dimensions into a vector of spatial lengths, and
 //! the number of time steps (if time varying). The dimension
 //! vector axis must be ordered: X, Y, Z, T
 //!
 //! \param[in] dimensions An ordered vector of dimensions
 //! \param[out] sdims Ordered vector of dimension lengths
 //! extracted from \p dimensions
 //! \param[out] numts The number of time steps if \p dimensions
 //! contains a time dimension, otherwise \p numts will be one
 //! 
 //! \retval status A value of true is returned if the \p dimensions
 //! contains a correctly sized and ordered vector of dimensions
 //
 static bool ParseDimensions(
	const vector <VDC::Dimension> &dimensions,
	vector <size_t> &sdims, size_t &numts
 );

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
 //! \param[out] path Path to file containing variable \p varname at 
 //! time step \p ts.
 //! \param[out] file_ts Temporal offset of variable \p varname in file
 //! \p path.
 //! \param[out] max_ts Maximum number of time steps stored in 
 //! \p path 
 //!
 //! \retval status A negative int is returned if \p varname or 
 //! \p ts are invalid, or if the class object is in define mode.
 //!
 virtual int GetPath(
	string varname, size_t ts, string &path, size_t &file_ts,
	size_t &max_ts

 ) const = 0;

 //! Return a variable's dimension lengths at a specified refinement level
 //!
 //! Compressed variables have a multi-resolution grid representation.
 //! This method returns the variable's ordered spatial and 
 //! temporal dimension lengths, 
 //! and block dimensions
 //! at the multiresolution refinement level specified by \p level.
 //! 
 //! If the variable named by \p varname is not compressed the variable's
 //! native dimensions are returned.
 //!
 //! \param[in] varname Data or coordinate variable name.
 //! \param[in] level Specifies a member of a multi-resolution variable's
 //! grid hierarchy as described above.
 //! \param[out] dims_at_level An ordered vector containing the variable's 
 //! dimensions at the specified refinement level
 //! \param[out] bs_at_level An ordered vector containing the variable's 
 //! block dimensions at the specified refinement level
 //!
 //! \retval status Zero is returned upon success, otherwise -1.
 //!
 //! \sa VAPoR::VDC, VDC::BaseVar::GetBS(), VDC::BaseVar::GetDimensions()
 //
 virtual int GetDimLensAtLevel(
	string varname, int level, std::vector <size_t> &dims_at_level,
	std::vector <size_t> &bs_at_level
 ) const = 0;


 //! Open the named variable for reading
 //!
 //! This method prepares a data or coordinate variable, indicated by a
 //! variable name and time step pair, for subsequent read operations by
 //! methods of this class.  The value of the refinement levels
 //! parameter, \p level, indicates the resolution of the volume in
 //! the multiresolution hierarchy as described by GetDimLensAtLevel().
 //!
 //! The level-of-detail parameter, \p lod, selects
 //! the approximation level. Valid values for \p lod are integers in
 //! the range 0..n-1, where \e n is returned by 
 //! VDC::BaseVar::GetCRatios().size(), or the value -1 may be used
 //! to select the best approximation available. 
 //!
 //! An error occurs, indicated by a negative return value, if the
 //! volume identified by the {varname, timestep, level, lod} tupple
 //! is not available. Note the availability of a volume can be tested
 //! with the VariableExists() method.
 //!
 //! \param[in] ts Time step of the variable to read. This is the integer
 //! offset into the variable's temporal dimension. If the variable
 //! does not have a temporal dimension \p ts is ignored.
 //! \param[in] varname Name of the variable to read
 //! \param[in] level Refinement level of the variable. Ignored if the
 //! variable is not compressed.
 //! \param[in] lod Approximation level of the variable. A value of -1
 //! indicates the maximum approximation level defined for the VDC.
 //! Ignored if the variable is not compressed.
 //! \retval status Returns a non-negative value on success
 //!
 //! \sa GetNumRefLevels(), VDC::BaseVar::GetCRatios(), OpenVariableRead()
 //
 virtual int OpenVariableRead(
	size_t ts, string varname, int level=0, int lod=0
 ) = 0;

 //! Open the named variable for writing
 //!
 //! This method prepares a data or coordinate variable, indicated by a
 //! variable name and time step pair, for subsequent write operations by
 //! methods of this class.  
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
 //! varible identified by the {varname, timestep, lod} tupple
 //! is not defined. 
 //!
 //! \param[in] ts Time step of the variable to read. This is the integer
 //! offset into the variable's temporal dimension. If the variable
 //! does not have a temporal dimension \p ts is ignored.
 //! \param[in] varname Name of the variable to read
 //! \param[in] lod Approximation level of the variable. A value of -1
 //! indicates the maximum approximation level defined for the VDC.
 //! Ignored if the variable is not compressed.
 //! \retval status Returns a non-negative value on success
 //!
 //! \sa GetNumRefLevels(), VDC::BaseVar::GetCRatios(), OpenVariableRead()
 //
 virtual int OpenVariableWrite(size_t ts, string varname, int lod=-1) = 0;

 //! Close the currently opened variable
 //!
 //! Close the handle for variable opened with OpenVariableWrite(),
 //! or OpenVariableRead()
 //!
 //! \sa OpenVariableWrite(), OpenVariableRead()
 //
 virtual int CloseVariable() = 0;


 //! Write all spatial values to the currently opened variable
 //!
 //! This method writes, and compresses as necessary, 
 //!  the contents of the array contained in 
 //! \p data to the currently opened variable. The number of values
 //! written from \p data is given by the product of the spatial 
 //! dimensions of the open variable.
 //!
 //! \param[in] data An array of data to be written
 //! \retval status Returns a non-negative value on success
 //!
 //! \sa OpenVariableWrite()
 //
 virtual int Write(const float *data) = 0;

 //! Write a single slice of data to the currently opened variable
 //!
 //! Compress, and necessary, and write a single slice (2D array) of 
 //! data to the variable
 //! indicated by the most recent call to OpenVariableWrite().
 //! The dimensions of a slices are NX by NY,
 //! where NX is the dimension of the array along the fastest varying
 //! spatial dimension, specified
 //! in grid points, and NY is the length of the second fastest varying
 //! dimension at the currently opened refinement level. See 
 //! OpenVariableWrite().
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
 virtual int WriteSlice(const float *slice) = 0;

 //! Read all spatial values of the currently opened variable
 //!
 //! This method reads, and decompresses as necessary, 
 //!  the contents of the currently opened variable into the array 
 //! \p data. The number of values
 //! read into \p data is given by the product of the spatial 
 //! dimensions of the open variable at the refinement level specified.
 //!
 //! It is the caller's responsibility to ensure \p data points
 //! to adequate space.
 //!
 //! \param[out] data An array of data to be written
 //! \retval status Returns a non-negative value on success
 //!
 //! \sa OpenVariableRead()
 //
 int virtual Read(float *data) = 0;

 //! Read a single slice of data from the currently opened variable
 //!
 //! Decompress, as necessary, and read a single slice (2D array) of 
 //! data from the variable
 //! indicated by the most recent call to OpenVariableRead().
 //! The dimensions of a slices are NX by NY,
 //! where NX is the dimension of the array along the fastest varying
 //! spatial dimension, specified
 //! in grid points, and NY is the length of the second fastest varying
 //! dimension at the currently opened grid refinement level. See
 //! OpenVariableRead().
 //!
 //! This method should be called exactly NZ times for each opened variable,
 //! where NZ is the dimension of third, and slowest varying dimension.
 //! In the case of a 2D variable, NZ is 1.
 //!
 //! It is the caller's responsibility to ensure \p slice points
 //! to adequate space.
 //!
 //! \param[out] slice A 2D slice of data
 //! \retval status Returns a non-negative value on success
 //!
 //! \sa OpenVariableRead()
 //!
 virtual int ReadSlice(float *slice) = 0;

 //! Read in and return a subregion from the currently opened
 //! variable
 //!
 //! This method reads and returns a subset of variable data.
 //! The \p min and \p max vectors, whose dimensions must match the
 //! spatial rank of the currently opened variable, identify the minimum and
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
    const vector <size_t> &min, const vector <size_t> &max, float *region
 ) = 0;

 //! Read in and return a blocked subregion from the currently opened
 //! variable.
 //!
 //! This method is identical to ReadRegion() with the exceptions
 //! that for compressed variables:
 //!
 //! \li The vectors \p start and \p count must be aligned
 //! with the underlying storage block of the variable. See
 //! VDC::SetCompressionBlock()
 //!
 //! \li The hyperslab copied to \p region will preserve its underlying
 //! storage blocking (the data will not be contiguous)
 //!
 virtual int ReadRegionBlock(
    const vector <size_t> &min, const vector <size_t> &max, float *region
 ) = 0;

 //! Write an entire variable in one call
 //!
 //! This method writes and entire variable (all time steps, all grid points)
 //! into a VDC.  This is the simplest interface for writing data into
 //! a VDC. If the variable is split across multiple files PutVar()
 //! ensures that the data are correctly distributed.
 //! Any variables currently opened with OpenVariableWrite() are first closed.
 //! Thus variables need not be opened with OpenVariableWrite() prior to
 //! calling PutVar();
 //!
 //! It is an error to call this method in \b define mode
 //!
 //! \param[in] varname Name of the variable to write
 //! \param[in] lod Approximation level of the variable. A value of -1
 //! indicates the maximum approximation level defined for the VDC.
 //! Ignored if the variable is not compressed.
 //! \param[in] data Pointer from where the data will be copied
 //! 
 //! \retval status A negative int is returned on failure
 //!
 //! \sa GetVar()
 //
 virtual int PutVar(string varname, int lod, const float *data) = 0;
 
 //! Write a variable at single time step
 //!
 //! This method writes a variable hyperslab consisting of the
 //! variable's entire spatial dimensions at the time step
 //! indicated by \p ts.
 //! Any variables currently opened with OpenVariableWrite() are first closed.
 //! Thus variables need not be opened with OpenVariableWrite() prior to
 //! calling PutVar();
 //!
 //! It is an error to call this method in \b define mode
 //!
 //! \param[in] ts Time step of the variable to write. This is the integer
 //! offset into the variable's temporal dimension. If the variable
 //! does not have a temporal dimension \p ts is ignored.
 //! \param[in] varname Name of the variable to write
 //! \param[in] lod Approximation level of the variable. A value of -1
 //! indicates the maximum approximation level defined for the VDC.
 //! Ignored if the variable is not compressed.
 //! \param[in] data Pointer from where the data will be copied
 //! 
 //! \retval status A negative int is returned on failure
 //!
 //! \sa GetVar()
 //
 virtual int PutVar(size_t ts, string varname, int lod, const float *data) = 0;

 //! Read an entire variable in one call
 //!
 //! This method reads and entire variable (all time steps, all grid points)
 //! from a VDC.  This is the simplest interface for reading data from
 //! a VDC. If the variable is split across multiple files GetVar()
 //! ensures that the data are correctly gathered and assembled into memory
 //! Any variables currently opened with OpenVariableRead() are first closed.
 //! Thus variables need not be opened with OpenVariableRead() prior to
 //! calling GetVar();
 //!
 //! It is an error to call this method in \b define mode
 //!
 //! \param[in] varname Name of the variable to write
 //! \param[in] level Refinement level of the variable. 
 //! Ignored if the variable is not compressed.
 //! \param[in] lod Approximation level of the variable. A value of -1
 //! indicates the maximum approximation level defined for the VDC.
 //! Ignored if the variable is not compressed.
 //! \param[out] data Pointer to where data will be copied. It is the 
 //! caller's responsbility to ensure \p data points to sufficient memory.
 //! 
 //! \retval status A negative int is returned on failure
 //!
 //! \sa PutVar()
 //
 virtual int GetVar(string varname, int level, int lod, float *data) = 0;

 //! Read an entire variable at a given time step in one call
 //!
 //! This method reads and entire variable (all grid points) at 
 //! time step \p ts
 //! from a VDC.  This is the simplest interface for reading data from
 //! a VDC. 
 //! Any variables currently opened with OpenVariableRead() are first closed.
 //! Thus variables need not be opened with OpenVariableRead() prior to
 //! calling GetVar();
 //!
 //! It is an error to call this method in \b define mode
 //!
 //! \param[in] ts Time step of the variable to write. This is the integer
 //! offset into the variable's temporal dimension. If the variable
 //! does not have a temporal dimension \p ts is ignored.
 //! \param[in] varname Name of the variable to write
 //! \param[in] level Refinement level of the variable. 
 //! Ignored if the variable is not compressed.
 //! \param[in] lod Approximation level of the variable. A value of -1
 //! indicates the maximum approximation level defined for the VDC.
 //! Ignored if the variable is not compressed.
 //! \param[out] data Pointer to where data will be copied. It is the 
 //! caller's responsbility to ensure \p data points to sufficient memory.
 //! 
 //! \retval status A negative int is returned on failure
 //!
 //! \sa PutVar()
 //
 virtual int GetVar(
	size_t ts, string varname, int level, int lod, float *data
 ) = 0;

 //! This method computes and returns the depth (number of levels) in a
 //! a multi-resolution hierarch for a given wavelet, \p wname,
 //! and decomposition block, \p bs.
 //! It also computes the maximum compression ratio, \p cratio, possible
 //! for the
 //! the specified combination of block size, \p bs, and wavelet, \p wname.
 //! The maximum compression ratio is \p cratio:1.
 //!
 //! \param[in] bs Dimensions of native decomposition block. The rank of
 //! \p bs may be less than or equal to the rank of \p dims.
 //! \param[in] wname wavelet name
 //! \param[out] nlevels Number of levels in hierarchy
 //! \param[out] maxcratio Maximum compression ratio
 //!
 //! \retval bool If \p bs, \p wname, or the combination there of is invalid
 //! false is returned and the values of \p nlevels and \p maxcratio are
 //! undefined. Upon success true is returned.
 //!
 virtual bool CompressionInfo(
    vector <size_t> bs, string wname, size_t &nlevels, size_t &maxcratio
 ) const = 0; 


 //! Returns true if indicated data volume is available
 //!
 //! Returns true if the variable identified by the timestep, variable
 //! name, refinement level, and level-of-detail is present in
 //! the data set. Returns false if
 //! the variable is not available.
 //!
 //! \param[in] ts A valid time step between 0 and GetNumTimesteps()-1
 //! \param[in] varname A valid variable name
 //! \param[in] reflevel Refinement level requested.
 //! \param[in] lod Compression level of detail requested.
 //! refinement level contained in the VDC.
 //
 virtual bool VariableExists(
    size_t ts,
    string varname,
    int reflevel = 0,
    int lod = 0
 ) const = 0;

 friend std::ostream &operator<<(std::ostream &o, const VDC &vdc);

private:
 std::vector <string> _newUniformVars;

protected:
 string _master_path;
 AccessMode _mode;
 bool _defineMode;
 std::vector <size_t> _bs;
 string _wname;
 std::vector <size_t> _cratios;
 vector <bool> _periodic;
 VAPoR::UDUnits _udunits;

 std::map <string, Dimension> _dimsMap;
 std::map <string, Attribute> _atts;
 std::map <string, CoordVar> _coordVars;
 std::map <string, DataVar> _dataVars;

#ifndef DOXYGEN_SKIP_THIS

 bool _ValidDefineDimension(string name, size_t length, int axis) const;

 bool _ValidDefineCoordVar(
    string varname, vector <string> dimnames,
    string units, int axis, XType type, bool compressed
 ) const;

 bool _ValidDefineDataVar(
    string varname, vector <string> dimnames, vector <string> coordnames,
    string units, XType type, bool compressed
 ) const;


 bool _ValidCompressionBlock(
	vector <size_t> bs, string wname, vector <size_t> cratios
 ) const;

 virtual int _WriteMasterMeta() = 0;
 virtual int _ReadMasterMeta() = 0;

#endif

};
};

#endif
