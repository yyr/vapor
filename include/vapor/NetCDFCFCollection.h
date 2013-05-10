//
// $Id$
//


#ifndef	_NetCDFCFCollection_h_
#define	_NetCDFCFCollection_h_

#include <vector>
#include <map>
#include <algorithm>

#include <sstream>
#include <vapor/MyBase.h>
#include <vapor/NetCDFCollection.h>

union ut_unit;
struct ut_system;

namespace VAPoR {

//
//! \class NetCDFCFCollection
//! \brief Wrapper for a collection of netCDF files
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! This class provides access to CF-1 compliant collection of netCDF
//! files. This work is based on the "NetCDF Climate and Forecast
//! (CF) Metadata Conventions", version 1.6, 5, December 2011.
//

class VDF_API NetCDFCFCollection : public NetCDFCollection {
public:
 class UDUnits;
 
 NetCDFCFCollection();
 virtual ~NetCDFCFCollection();

 virtual int Initialize(
	const std::vector <string> &files
 );


 //! Return boolean indicating whether variable is a CF coordinate variable
 //!
 //! This method returns true if the variable named by \p var is a 
 //! "coordinate variable"
 //!
 //! CF1.X Definition of <em> coordinate variable </em>:
 //!
 //! "We use this term precisely as it is defined in section 2.3.1 of the NUG.
 //! It is a one- dimensional variable with the same name as its
 //! dimension [e.g., time(time)], and it is defined as a numeric data
 //! type with values that are ordered monotonically. Missing values are
 //!
 //! \retval true if \p var is a coordinate variable, false otherwise
 //
 virtual bool IsCoordVarCF(string var) const {
	return(std::find( _coordinateVars.begin(), _coordinateVars.end(), var) != _coordinateVars.end());
 }

 //! Return boolean indicating whether variable is a CF auxiliary
 //! coordinate variable
 //!
 //! This method returns true if the variable named by \p var is a 
 //! "auxiliary coordinate variable"
 //!
 //! CF1.X Definition of <em> auxiliary coordinate variable </em>:
 //!
 //! Any netCDF variable that contains coordinate data, but is not
 //! a coordinate variable (in the sense of that term defined by the
 //! NUG and used by this standard - see below). Unlike coordinate
 //! variables, there is no relationship between the name of an auxiliary
 //! coordinate variable and the name(s) of its dimension(s). 
 //!
 //! \retval true if \p var is an auxliary coordinate variable, false otherwise
 
 virtual bool IsAuxCoordVarCF(string var) const {
	return(std::find( _auxCoordinateVars.begin(),
	_auxCoordinateVars.end(), var) != _auxCoordinateVars.end());
 }
 virtual bool IsLatCoordVar(string var) const {
	return(std::find( _latCoordVars.begin(),
    _latCoordVars.end(), var) != _latCoordVars.end());
 }
 virtual bool IsLonCoordVar(string var) const {
	return(std::find( _lonCoordVars.begin(),
    _lonCoordVars.end(), var) != _lonCoordVars.end());
 }
 virtual bool IsTimeCoordVar(string var) const {
	return(std::find( _timeCoordVars.begin(),
    _timeCoordVars.end(), var) != _timeCoordVars.end());
 }
 virtual bool IsVertCoordVar(string var) const {
	return(std::find( _vertCoordVars.begin(),
    _vertCoordVars.end(), var) != _vertCoordVars.end());
 }

 //! Return true if the increasing direction of the named vertical coordinate 
 //! variable is up
 //!
 //! CF 1.x description of vertical coordinate direction:
 //!
 //! The direction of positive (i.e., the direction in which the coordinate
 //! values are increasing), whether up or down, cannot in all cases be
 //! inferred from the units. The direction of positive is useful for
 //! applications displaying the data. For this reason the attribute
 //! positive as defined in the COARDS standard is required if the
 //! vertical axis units are not a valid unit of pressure (a determination
 //! which can be made using the udunits routine, utScan) -- otherwise
 //! its inclusion is optional. The positive attribute may have the value
 //! up or down (case insensitive).
 //!
 virtual bool IsVertCoordVarUp(string var) const;
 
 

 virtual std::vector <string> GetLatCoordVars() const {return(_latCoordVars); };
 virtual std::vector <string> GetLonCoordVars() const {return(_lonCoordVars); };
 virtual std::vector <string> GetTimeCoordVars() const {return(_timeCoordVars); };
 virtual std::vector <string> GetVertCoordVars() const {return(_vertCoordVars); };

 
 //! Return a list of data variables with a given rank
 //!
 //!
 //! Returns a list of data variables having a spatial dimension rank
 //! of \p ndim. If the named variable is explicitly time varying, the
 //! time-varying dimension is not counted. For example, if a variable
 //! named 'v' is defined with 4 dimensions in the netCDF file, and the
 //! slowest varying is time , then the variable 'v' would be returned by a
 //! query with ndim==3.
 //!
 //! Names of variables that are coordinate or auxiliary coordinate 
 //! variables are not returned, nor are variables that are missing
 //! coordinate variables.
 //!
 //! \param[in] ndim Rank of spatial dimensions
 //!
 //! \sa NetCDFCollection::GetVariableNames()
 //
 virtual std::vector <string> GetDataVariableNames(int ndim) const;

 virtual bool VariableExists(string varname) const {
	//
	// Should be checking dependencies for derived variable!
	//
	if (NetCDFCFCollection::IsDerivedVar(varname)) return (true);
	return (NetCDFCollection::VariableExists(varname));
 }
 virtual bool VariableExists(size_t ts, string varname) const {
	//
	// Should be checking dependencies for derived variable!
	//
	if (NetCDFCFCollection::IsDerivedVar(varname)) return (true);
	return (NetCDFCollection::VariableExists(ts, varname));
 }

 virtual bool IsDerivedVar(string varname) const {
   return(_derivedVarsMap.find(varname) != _derivedVarsMap.end());
 }

 //!
 //! Return unordered list of coordinate or auxliary coordinate
 //! variables for the named variable.
 //!
 //! This method returns in \p cvars an unordered list of all of the
 //! spatio-temporal coordinate or auxliary coordinate variables 
 //! associated with the variable named by \p var. See Chapter 5 of
 //! the CF 1.X spec. for more detail, summarized here:
 //!
 //! CF1.X Chap. 5 excerpt :
 //!
 //! "The use of coordinate variables is required whenever they are
 //! applicable. That is, auxiliary coordinate variables may not be
 //! used as the only way to identify latitude and longitude coordinates
 //! that could be identified using coordinate variables. This is both
 //! to enhance conformance to COARDS and to facilitate the use of
 //! generic applications that recognize the NUG convention for coordinate
 //! variables. An application that is trying to find the latitude
 //! coordinate of a variable should always look first to see if any
 //! of the variable's dimensions correspond to a latitude coordinate
 //! variable. If the latitude coordinate is not found this way, then
 //! the auxiliary coordinate variables listed by the coordinates
 //! attribute should be checked. Note that it is permissible, but
 //! optional, to list coordinate variables as well as auxiliary
 //! coordinate variables in the coordinates attribute."
 //!
 //! \retval status a negative int is returned if the number of 
 //! elements in \p cvars does not match the number of spatio-temporal
 //! dimensions of the variable named by \p var
 //
 virtual int GetVarCoordVarNames(string var, std::vector <string> &cvars) const;

 virtual int GetVarUnits(string var, string &units) const;

 const UDUnits *GetUDUnits() const {return(_udunit); };


 virtual int Convert(
	const string from,
	const string to,
	const float *src,
	float *dst,
	size_t n
 ) const;


 virtual bool GetMissingValue(string varname, double &mv) const;
 virtual int OpenRead(size_t ts, string varname);
 virtual int ReadSlice(float *data);
 virtual int Close();

 virtual std::vector <string> GetVariableNames(int ndim) const;

 virtual std::vector <size_t>  GetDims(string varname) const;

 virtual bool IsVertDimensionless(string cvar) const;

 virtual int InstallStandardVerticalConverter(
	string cvar, string newvar, string units = "meters"
 );

 virtual void UninstallStandardVerticalConverter(string cvar) ;

 void FormatTimeStr(double time, string &str) const;

 friend std::ostream &operator<<(
    std::ostream &o, const NetCDFCFCollection &ncdfc
 );

 class UDUnits {
 public:
  UDUnits();
  ~UDUnits();
  int Initialize();
  
  bool IsPressureUnit(string unitstr) const;
  bool IsTimeUnit(string unitstr) const;
  bool IsLatUnit(string unitstr) const;
  bool IsLonUnit(string unitstr) const;
  bool IsLengthUnit(string unitstr) const;
  bool AreUnitsConvertible(const ut_unit *unit, string unitstr) const;
  bool Convert(
    const string from,
    const string to,
    const float *src,
    float *dst,
    size_t n
  ) const;
  void DecodeTime(
	double seconds, int* year, int* month, int* day,
	int* hour, int* minute, int* second
  ) const;

  string GetErrMsg() const;

 private:
  std::map <int, std::string> _statmsg;
  int _status;
  ut_unit *_pressureUnit;
  ut_unit *_timeUnit;
  ut_unit *_latUnit;
  ut_unit *_lonUnit;
  ut_unit *_lengthUnit;
  ut_system *_unitSystem;
 };

private:
 class DerivedVar {
 public:
  DerivedVar(
	NetCDFCFCollection *ncdfcf, 
	const std::map <string, string> &formula_map, string units
  ) {
	_ncdfcf = ncdfcf;
	_formula_map = formula_map;
	_units = units;
  };
  virtual ~DerivedVar() {};
  virtual int Open(size_t ts) = 0;
  virtual int ReadSlice(float *slice) = 0;
  virtual int Read(float *buf) = 0;
  virtual void Close() {};
  virtual bool TimeVarying() const = 0;
  virtual std::vector <size_t>  GetDims() const = 0;
 protected:
  NetCDFCFCollection *_ncdfcf;
  std::map <string, string> _formula_map;
  string _units;
 };

 class DerivedVar_ocean_s_coordinate_g1 : public DerivedVar {
 private:
  std::vector <size_t> _dims;
  size_t _slice_num;
  float *_s;
  float *_C;
  float *_eta;
  float *_depth;
  float _depth_c;
  string _svar;
  string _Cvar;
  string _etavar;
  string _depthvar;
  string _depth_cvar;
  bool _is_open;
  bool _ok;

 public:
  DerivedVar_ocean_s_coordinate_g1(
	NetCDFCFCollection *ncdfcf, 
	const std::map <string, string> &formula_map, string units
  );
  virtual ~DerivedVar_ocean_s_coordinate_g1();

  virtual int Open(size_t ts);
  virtual int ReadSlice(float *slice);
  virtual int Read(float *buf);
  virtual void Close() {_is_open = false; };
  virtual bool TimeVarying() const {return(false); };
  virtual std::vector <size_t>  GetDims() const { return(_dims); }
 };

 class DerivedVar_ocean_s_coordinate_g2 : public DerivedVar {
 private:
  std::vector <size_t> _dims;
  size_t _slice_num;
  float *_s;
  float *_C;
  float *_eta;
  float *_depth;
  float _depth_c;
  string _svar;
  string _Cvar;
  string _etavar;
  string _depthvar;
  string _depth_cvar;
  bool _is_open;
  bool _ok;

 public:
  DerivedVar_ocean_s_coordinate_g2(
	NetCDFCFCollection *ncdfcf, 
	const std::map <string, string> &formula_map, string units
  );
  virtual ~DerivedVar_ocean_s_coordinate_g2();

  virtual int Open(size_t ts);
  virtual int ReadSlice(float *slice);
  virtual int Read(float *buf);
  virtual void Close() {_is_open = false; };
  virtual bool TimeVarying() const {return(false); };
  virtual std::vector <size_t>  GetDims() const { return(_dims); }
 };




 std::vector <std::string> _coordinateVars;
 std::vector <std::string> _auxCoordinateVars;
 std::vector <std::string> _lonCoordVars;
 std::vector <std::string> _latCoordVars;
 std::vector <std::string> _vertCoordVars;
 std::vector <std::string> _timeCoordVars;

 std::map <string, DerivedVar *> _derivedVarsMap;
 DerivedVar * _derivedVar; // if current opened variable is derived this is it

 UDUnits	*_udunit;

 //
 // Map a variable name to it's missing value (if any)
 //
 std::map <string, double> _missingValueMap;

 std::vector <std::string> _GetCoordAttrs(
	const NetCDFSimple::Variable &varinfo
 ) const;

 //! CF1.X Definition of <em> coordinate variable </em>:
 //!
 //! "We use this term precisely as it is defined in section 2.3.1 of the NUG. 
 //! It is a one- dimensional variable with the same name as its
 //! dimension [e.g., time(time)], and it is defined as a numeric data
 //! type with values that are ordered monotonically. Missing values are
 //! not allowed in coordinate variables."
 //
 bool _IsCoordinateVar(const NetCDFSimple::Variable &varinfo) const; 

 //!
 //! CF1.X Determination of a longitude coordinate variable:
 //!
 //! "We recommend the determination that a coordinate is a longitude
 //! type should be done via a string match between the given unit and
 //! one of the acceptable forms of degrees_east.
 //! 
 //! Optionally, the longitude type may be indicated additionally by
 //! providing the standard_name attribute with the value longitude,
 //! and/or the axis attribute with the value X.
 //! 
 //! Coordinates of longitude with respect to a rotated pole should be
 //! given units of degrees, not degrees_east or equivalents, because
 //! applications which use the units to identify axes would have no
 //! means of distinguishing such an axis from real longitude, and might
 //! draw incorrect coastlines, for instance."
 
 bool _IsLonCoordVar(
    const NetCDFSimple::Variable &varinfo
 ) const;

 //! CF1.X Determination of a latitude coordinate variable:
 //!
 //! Hence, determination that a coordinate is a latitude type should
 //! be done via a string match between the given unit and one of the
 //! acceptable forms of degrees_north.
 //! 
 //! Optionally, the latitude type may be indicated additionally by
 //! providing the standard_name attribute with the value latitude,
 //! and/or the axis attribute with the value Y
 //! 
 bool _IsLatCoordVar(
     const NetCDFSimple::Variable &varinfo
 ) const;
 
 //! CF1.X Determination of vertical coordinate variable
 //!
 //! A vertical coordinate will be identifiable by:
 //! 
 //! units of pressure; or
 //! 
 //! the presence of the positive attribute with a value of up or down
 //! (case insensitive).
 //! 
 //! Optionally, the vertical type may be indicated additionally by
 //! providing the standard_name attribute with an appropriate value,
 //! and/or the axis attribute with the value Z.
 //! 
 bool _IsVertCoordVar(
    const NetCDFSimple::Variable &varinfo 
 ) const;

 //! CF1.X Determination of time coordinate variable
 //!
 //! A time coordinate is identifiable from its units string alone. The
 //! Udunits routines utScan() and utIsTime() can be used to make this
 //! determination.
 //! 
 //! Optionally, the time coordinate may be indicated additionally by
 //! providing the standard_name attribute with an appropriate value,
 //! and/or the axis attribute with the value T.
 //!
 bool _IsTimeCoordVar(
     const NetCDFSimple::Variable &varinfo 
 ) const;

 bool _GetMissingValue(string varname, string &attname, double &mv)  const;
 void _GetMissingValueMap(map <string, double> &missingValueMap) const;

 int _parse_formula(
    string formula_terms, map <string, string> &parsed_terms
 ) const;



};
};
 
 
#endif