#include <vector>
#include <algorithm>
#include <map>
#include <iostream>
#include <vapor/MyBase.h>

#ifndef	_DC_H_
#define	_DC_H_

namespace VAPoR {

//!
//! \class DC
//! \ingroup Public_VDC
//!
//! \brief Defines API for reading a collection of data.
//!
//! \author John Clyne
//! \date    January, 2015
//!
//! The abstract Data Collection (DC) class defines an API for 
//! reading metadata and sampled
//! data from a data collection.  A data collection is a set of 
//! related data, most typically the discrete outputs from a single numerical
//! simulation. The DC class is an abstract virtual
//! class, providing a public API, but performing no actual storage 
//! operations. Derived implementations of the DC base class are 
//! required to support the API.
//!
//! Variables in a DC may have 1, 2, or 3 spatial dimensions, and 0 or 1
//! temporal dimensions.
//!
//! The DC is structured in the spirit of the "NetCDF Climate and Forecast
//! (CF) Metadata Conventions", version 1.6, 5, December 2011.
//! It supports only a subset of the CF functionality (e.g. there is no
//! support for "Discrete Sampling Geometries"). Moreover, it is 
//! more restrictive than the CF in a number of areas. Particular
//! items of note include:
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
//! Compressed variables in the DC, if they exist, have a multi-resolution 
//! representation: the sampling grid for multi-resolution variables
//! is hierarchical, and the dimension lengths of adjacent levels in the
//! hierarchy differ by a factor of two. The \p level parameter is 
//! used to select a particular depth of the hierarchy.
//!
//! To provide maximum flexibility as well as compatibility with previous
//! versions of the DC the interpretation of \p level is somewhat 
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
//! \p cratios vector (See DC::GetCRatios()).
//!
//! For negative values of \p lod a value of \b -1 indexes the
//! last element of \p cratios, a value of \b -2 indexes the 
//! second to last element, and so on.
//! Using negative values the first element of \p cratios - the greatest 
//! compression rate - is indexed by negating the size of the 
//! \p cratios vector.
//!
//! \param cratios A monotonically decreasing vector of
//! compression ratios. Compressed variables in the DC are stored
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
//! in the DC: variables are decomposed into blocks, and individual blocks
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
class VDF_API DC : public VetsUtil::MyBase {
public:

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

  bool operator==(const Dimension &rhs) const {
	return(_name == rhs._name && _length==rhs._length && _axis==rhs._axis);
  }
  bool operator!=(const Dimension &rhs) const {
	return(! (*this==rhs));
  }
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

  //! Get attribute name
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
  BaseVar() {
	  _name.clear();
	  _dimensions.clear();
	  _units = "";
	  _type = FLOAT;
	  _wname = "";
	  _cratios.clear();
	  _bs.clear();
	  _periodic.clear();
	  _atts.clear();
  }

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
	string name, std::vector <DC::Dimension> dimensions,
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
	for (int i=_bs.size(); i<_dimensions.size(); i++) _bs.push_back(1);
  };

  //! No compression constructor 
  //!
  //! \param[in] name The variable's name
  //! \param[in] dimensions An ordered vector specifying the variable's spatial 
  //! and/or temporal dimensions
  //! \param[in] units A string recognized by Udunits-2 specifying the
  //! unit measure for the variable. An empty string indicates that the
  //! variable is unitless.
  //! \param[in] type The external storage type for variable data
  //! factor for the variable. 
  //! \param[in] periodic An ordered array of booleans 
  //! specifying the
  //! spatial boundary periodicity.
  //! Results are undefined if the rank of 
  //! of \p periodic does not match that of \p dimensions.
  //!
  BaseVar(
	string name, std::vector <DC::Dimension> dimensions,
	string units, XType type, std::vector <bool> periodic
  );

  virtual ~BaseVar() {};

  //! Get variable name
  //
  string GetName() const {return (_name); };
  void SetName(string name) {_name = name; };

  //! Access variable's dimension names
  //
  std::vector <DC::Dimension> GetDimensions() const {return (_dimensions); };
  void SetDimensions(std::vector <DC::Dimension> dimensions) {
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

  //! Return true if no wavelet is defined
  //
  bool IsCompressed() const { return (! _wname.empty()); };

  //! Return true if a time dimension is present
  //
  bool IsTimeVarying() const { 
	for (int i=0; i<_dimensions.size(); i++) {
		if (_dimensions[i].GetAxis() == 3) return(true);
	}
	return(false);
  };

  friend std::ostream &operator<<(std::ostream &o, const BaseVar &var);
  
 private:
  string _name;
  std::vector <DC::Dimension> _dimensions;
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
  CoordVar() : BaseVar() {
	_axis = 0;
	_uniform = false;
  }


  //! Construct coordinate variable 
  //!
  //! \copydetails BaseVar(string name, std::vector <DC::Dimension> dimensions,
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
	string name, std::vector <DC::Dimension> dimensions,
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

  //! Construct coordinate variable without compression
  //!
  //! \copydetails BaseVar(string name, std::vector <DC::Dimension> dimensions,
  //!  string units, XType type, std::vector <bool> periodic)
  //!
  //! \param[in] axis an int in the range 0..3 indicating the coordinate
  //! axis, one of X, Y, Z, or T, respectively
  //! \param[in] uniform A bool indicating whether the coordinate variable
  //! is uniformly sampled.
  //
  CoordVar(
	string name, std::vector <DC::Dimension> dimensions,
	string units, XType type, 
	std::vector <bool> periodic, 
	int axis, bool uniform
  ) :
	BaseVar(name, dimensions, units, type, periodic),
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
  DataVar() : BaseVar() {
	_coordvars.clear();
	_maskvar = "";
	_has_missing = false;
	_missing_value = 0.0;
  }

  //! Construct Data variable definition with missing values
  //!
  //! Elements of the variable whose value matches that specified by
  //! \p missing_value are considered invalid
  //!
  //! \copydetails BaseVar(string name, std::vector <DC::Dimension> dimensions,
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
	string name, std::vector <DC::Dimension> dimensions,
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
	_maskvar(""),
	_has_missing(true),
	_missing_value(missing_value)
  {}

  //! Construct Data variable definition with a mask variable
  //!
  //! This version of the constructor specifies the name of a variable
  //! \p varmask whose contents indicate the presense or absense of invalid
  //! entries in the data variable. The contents of the mask array are treated
  //! as booleans, true values indicating valid data. The rank of of the 
  //! variable may be less than or equal to that of \p name. The dimensions
  //! of \p maskvar must match the fastest varying dimensions of \p name.
  //!
  //! \copydetails BaseVar(string name, std::vector <DC::Dimension> dimensions,
  //!  string units, XType type, 
  //!  std::vector <size_t> bs, string wname,
  //!  std::vector <size_t> cratios,
  //!  std::vector <bool> periodic)
  //!
  //! \param[in] coordvars Names of coordinate variables associated 
  //! with this variables dimensions
  //! \param[in] maskvar  Name of variable containing mask array. 
  //!
  DataVar(
	string name, std::vector <DC::Dimension> dimensions,
	string units, XType type, 
	std::vector <size_t> bs, string wname,
	std::vector <size_t> cratios,
	std::vector <bool> periodic, std::vector <string> coordvars, 
	string maskvar
  ) :
	BaseVar(
		name, dimensions, units, type, 
		bs, wname, cratios, periodic
	),
	_coordvars(coordvars),
	_maskvar(maskvar),
	_has_missing(false),
	_missing_value(0.0)
  {}

  //! Construct Data variable definition without missing values
  //!
  //! \copydetails BaseVar(string name, std::vector <DC::Dimension> dimensions,
  //!  string units, XType type, 
  //!  std::vector <size_t> bs, string wname,
  //!  std::vector <size_t> cratios,
  //!  vector <bool> periodic)
  //!
  //! \param[in] coordvars Names of coordinate variables associated 
  //! with this variables dimensions
  //!
  DataVar(
	string name, std::vector <DC::Dimension> dimensions,
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
	_maskvar(""),
	_has_missing(false),
	_missing_value(0.0)
  {}

  //! Construct Data variable definition with missing values but no compression
  //!
  //! \copydetails BaseVar(string name, std::vector <DC::Dimension> dimensions,
  //!  string units, XType type, 
  //!  std::vector <bool> periodic)
  //!
  //! \param[in] coordvars Names of coordinate variables associated 
  //! with this variables dimensions
  //! \param[in] missing_value  Value of the missing value indicator
  //!
  DataVar(
	string name, std::vector <DC::Dimension> dimensions,
	string units, XType type, 
	std::vector <bool> periodic, std::vector <string> coordvars, 
	double missing_value
  ) : 
	BaseVar(
		name, dimensions, units, type, periodic
	),
	_coordvars(coordvars),
	_maskvar(""),
	_has_missing(true),
	_missing_value(missing_value)
	{}

  //! Construct Data variable definition with a mask but no compression
  //!
  //! This version of the constructor specifies the name of a variable
  //! \p varmask whose contents indicate the presense or absense of invalid
  //! entries in the data variable. The contents of the mask array are treated
  //! as booleans, true values indicating valid data. The rank of of the 
  //! variable may be less than or equal to that of \p name. The dimensions
  //! of \p maskvar must match the fastest varying dimensions of \p name.
  //!
  //! \copydetails BaseVar(string name, std::vector <DC::Dimension> dimensions,
  //!  string units, XType type, 
  //!  std::vector <bool> periodic)
  //!
  //! \param[in] coordvars Names of coordinate variables associated 
  //! with this variables dimensions
  //! \param[in] missing_value  Value of the missing value indicator
  //!
  DataVar(
	string name, std::vector <DC::Dimension> dimensions,
	string units, XType type, 
	std::vector <bool> periodic, std::vector <string> coordvars, 
	string maskvar
  ) : 
	BaseVar(
		name, dimensions, units, type, periodic
	),
	_coordvars(coordvars),
	_maskvar(maskvar),
	_has_missing(false),
	_missing_value(0.0)
	{}

  //! Construct Data variable definition with no missing values or compression
  //!
  //! \copydetails BaseVar(string name, std::vector <DC::Dimension> dimensions,
  //!  string units, XType type, 
  //!  std::vector <bool> periodic)
  //!
  //! \param[in] coordvars Names of coordinate variables associated 
  //! with this variables dimensions
  //!
  DataVar(
	string name, std::vector <DC::Dimension> dimensions,
	string units, XType type, 
	std::vector <bool> periodic, std::vector <string> coordvars
  ) : 
	BaseVar(
		name, dimensions, units, type, periodic
	),
	_coordvars(coordvars),
	_maskvar(""),
	_has_missing(false),
	_missing_value(0.0)
	{}


  virtual ~DataVar() {};

  //! Access data variable's coordinate variable names
  //
  std::vector <string> GetCoordvars() const {return (_coordvars); };
  void SetCoordvars(std::vector <string> coordvars) {_coordvars = coordvars; };

  //! Access data variable's mask variable names
  //
  string GetMaskvar() const {return (_maskvar); };
  void SetMaskvar(string maskvar) {_maskvar = maskvar; };

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
  string _maskvar;
  bool _has_missing;
  double _missing_value;
 };



 //! Class constuctor
 //!
 //!
 DC() {};
 virtual ~DC() {}

 //! Initialize the DC class
 //!
 //! Prepare a DC for reading. This method prepares
 //! the master DC file indicated by \p path for reading.
 //! The method should be called immediately after the constructor, 
 //! before any other class methods. This method
 //! exists only because C++ constructors can not return error codes.
 //!
 //! \param[in] path Path name of file that contains, or will
 //! contain, the DC master file for this data collection
 //!
 //! \retval status A negative int is returned on failure
 //!
 //
 virtual int Initialize(const vector <string> &paths) = 0;


 //! Return a dimensions's definition
 //!
 //! This method returns the definition of the dimension named
 //! by \p dimname as a reference to a DC::Dimension object. If
 //! \p dimname is not defined as a dimension then the name of \p dimension
 //! will be the empty string()
 //!
 //! \param[in] dimname A string specifying the name of the dimension. 
 //! \param[out] dimension The returned Dimension object reference
 //! \retval bool If the named dimension can not be found false is returned.
 //!
 virtual bool GetDimension(
	string dimname, DC::Dimension &dimension
 ) const = 0;

 //! Return names of all defined dimensions
 //!
 //! This method returns the list of names of all of the dimensions
 //! defined in the DC.
 //!
 virtual std::vector <string> GetDimensionNames() const = 0;

 //! Return a coordinate variable's definition
 //!
 //! Return a reference to a DC::CoordVar object describing 
 //! the coordinate variable named by \p varname
 //!
 //! \param[in] varname A string specifying the name of the coordinate 
 //! variable. 
 //! \param[out] coordvar A CoordVar object containing the definition
 //! of the named variable.
 //! \retval bool False is returned if the named coordinate variable does 
 //! not exist, and the contents of \p cvar will be undefined.
 //!
 virtual bool GetCoordVarInfo(string varname, DC::CoordVar &cvar) const = 0;

 //! Return a data variable's definition
 //!
 //! Return a reference to a DC::DataVar object describing 
 //! the data variable named by \p varname
 //!
 //! \param[in] varname A string specifying the name of the variable. 
 //! \param[out] datavar A DataVar object containing the definition
 //! of the named Data variable.
 //!
 //! \retval bool If the named data variable cannot be found false 
 //! is returned and the values of \p datavar are undefined.
 //!
 virtual bool GetDataVarInfo( string varname, DC::DataVar &datavar) const = 0;
 
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
 virtual bool GetBaseVarInfo(string varname, DC::BaseVar &var) const = 0;


 //! Return a list of names for all of the defined data variables.
 //!
 //! Returns a list of names for all data variables defined 
 //!
 //
 virtual std::vector <string> GetDataVarNames() const = 0;


 //! Return a list of names for all of the defined coordinate variables.
 //!
 //! Returns a list of names for all coordinate variables defined 
 //!
 //
 virtual std::vector <string> GetCoordVarNames() const = 0;


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
 virtual int GetNumRefLevels(string varname) const = 0;


 //! Read an attribute
 //!
 //! This method reads an attribute from the DC. The attribute can either
 //! be "global", if \p varname is the empty string, or bound to a variable
 //! if \p varname indentifies a variable in the DC.
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
 //
 virtual int GetAtt(
	string varname, string attname, vector <double> &values
 ) const = 0;
 virtual int GetAtt(
	string varname, string attname, vector <long> &values
 ) const = 0;
 virtual int GetAtt(
	string varname, string attname, string &values
 ) const = 0;

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
 //! \sa GetAtt()
 //
 virtual std::vector <string> GetAttNames(string varname) const = 0;

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
 virtual XType GetAttType(string varname, string attname) const = 0;

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
 //! \sa VAPoR::DC, DC::BaseVar::GetBS(), DC::BaseVar::GetDimensions()
 //
 virtual int GetDimLensAtLevel(
	string varname, int level, std::vector <size_t> &dims_at_level,
	std::vector <size_t> &bs_at_level
 ) const = 0;

 //! Return a Proj4 map projection string.
 //!
 //! For georeference data sets that have map projections this
 //! method returns a properly formatted Proj4 projection string 
 //! for mapping from geographic to cartographic coordinates. If no
 //! such projection exists an empty string is returned.
 //!
 //! \param[in] lonname Name of longitude coordinate variable
 //! \param[in] latname Name of latitude coordinate variable
 //! \param[out] projstring An empty string if a Proj4 map projection is
 //! not available for the named coordinate pair, otherwise a properly 
 //! formatted Proj4 projection
 //! string is returned.
 //!
 //
 virtual int GetMapProjection(
	string lonname, string latname, string &projstring
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
 //! DC::BaseVar::GetCRatios().size(), or the value -1 may be used
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
 //! indicates the maximum approximation level defined for the DC.
 //! Ignored if the variable is not compressed.
 //! \retval status Returns a non-negative value on success
 //!
 //! \sa GetNumRefLevels(), DC::BaseVar::GetCRatios(), OpenVariableRead()
 //
 virtual int OpenVariableRead(
	size_t ts, string varname, int level=0, int lod=0
 ) = 0;


 //! Close the currently opened variable
 //!
 //! Close the handle for variable opened with OpenVariableRead()
 //!
 //! \sa  OpenVariableRead()
 //
 virtual int CloseVariable() = 0;

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
 //! DC::SetCompressionBlock()
 //!
 //! \li The hyperslab copied to \p region will preserve its underlying
 //! storage blocking (the data will not be contiguous)
 //!
 virtual int ReadRegionBlock(
    const vector <size_t> &min, const vector <size_t> &max, float *region
 ) = 0;

 //! Read an entire variable in one call
 //!
 //! This method reads and entire variable (all time steps, all grid points)
 //! from a DC.  This is the simplest interface for reading data from
 //! a DC. If the variable is split across multiple files GetVar()
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
 //! indicates the maximum approximation level defined for the DC.
 //! Ignored if the variable is not compressed.
 //! \param[out] data Pointer to where data will be copied. It is the 
 //! caller's responsbility to ensure \p data points to sufficient memory.
 //! 
 //! \retval status A negative int is returned on failure
 //!
 //
 virtual int GetVar(string varname, int level, int lod, float *data) = 0;

 //! Read an entire variable at a given time step in one call
 //!
 //! This method reads and entire variable (all grid points) at 
 //! time step \p ts
 //! from a DC.  This is the simplest interface for reading data from
 //! a DC. 
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
 //! indicates the maximum approximation level defined for the DC.
 //! Ignored if the variable is not compressed.
 //! \param[out] data Pointer to where data will be copied. It is the 
 //! caller's responsbility to ensure \p data points to sufficient memory.
 //! 
 //! \retval status A negative int is returned on failure
 //!
 //
 virtual int GetVar(
	size_t ts, string varname, int level, int lod, float *data
 ) = 0;


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
 //! refinement level contained in the DC.
 //
 virtual bool VariableExists(
    size_t ts,
    string varname,
    int reflevel = 0,
    int lod = 0
 ) const = 0;


 /////////////////////////////////////////////////////////////////////
 //
 // The following are convenience methods provided by the DC base 
 // class. In general they should NOT need to be reimplimented by
 // derived classes.
 //
 /////////////////////////////////////////////////////////////////////

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
 //! \sa DC::Dimension
 //!
 virtual bool GetDimension(
	string dimname, size_t &length, int &axis
 ) const;

 //! Return a list of data variables with a given dimension rank
 //!
 //! Returns a list of all data variables defined having a 
 //! dimension rank of \p ndim. If \p spatial is true, only the spatial
 //! dimension rank of the variable is compared against \p ndim
 //!
 //! \param[in] ndim Rank of dimensions for comparision
 //! \param[in] spatial Only compare spatial dimensions against \p ndim
 //!
 //
 virtual std::vector <string> GetDataVarNames(int ndim, bool spatial) const;


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
 virtual bool IsTimeVarying(string varname) const;

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
 //
 virtual bool IsCompressed(string varname) const;

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
 virtual int GetNumTimeSteps(string varname) const;

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
 virtual int GetCRatios(string varname, vector <size_t> &cratios) const;


 //! Return a boolean indicating whether a variable is a data variable 
 //!
 //! This method returns \b true if a data variable is defined
 //! with the name \p varname.  Otherwise the method returns false.
 //!
 //! \retval bool Returns true if \p varname names a defined data variable
 //!
 virtual bool IsDataVar(string varname) const {
	vector <string> names = GetDataVarNames();
	return(find(names.begin(), names.end(), varname) != names.end());
 }

 //! Return a boolean indicating whether a variable is a coordinate variable 
 //!
 //! This method returns \b true if a coordinate variable is defined
 //! with the name \p varname.  Otherwise the method returns false.
 //!
 //! \retval bool Returns true if \p varname names a defined coordinate 
 //! variable
 //!
 virtual bool IsCoordVar(string varname) const {
	vector <string> names = GetDataVarNames();
	return(find(names.begin(), names.end(), varname) != names.end());
 }

 //! Return a Proj4 map projection string for the named variable.
 //!
 //! For georeference data sets that have map projections this
 //! method returns a properly formatted Proj4 projection string 
 //! for mapping from geographic to cartographic coordinates. If no
 //! such projection exists for the named data variable an empty 
 //! string is returned.
 //!
 //! \param[in] varname Name of a data variable
 //!
 //! \param[out] projstring An empty string if a Proj4 map projection is
 //! not available for the named coordinate pair, otherwise a properly 
 //! formatted Proj4 projection
 //! string is returned.
 //!
 //
 virtual int GetMapProjection(string varname, string &projstring) const;

 //! Parse a vector of DC::Dimensions into space and time dimensions
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
	const vector <DC::Dimension> &dimensions,
	vector <size_t> &sdims, size_t &numts
 );


};
};

#endif
