#include <vector>
#include <algorithm>
#include <map>
#include <iostream>
#include <vapor/MyBase.h>
#include <vapor/NetCDFCollection.h>
#include <vapor/Proj4API.h>
#include <vapor/UDUnitsClass.h>
#include <vapor/DC.h>

#ifndef	_DCWRF_H_
#define	_DCWRF_H_

namespace VAPoR {


//!
//! \class DCWRF
//! \ingroup Public_VDCWRF
//!
//! \brief Defines API for reading a collection of data.
//!
//! \author John Clyne
//! \date    January, 2015
//!
class DCWRF : public VAPoR::DC {
public:


 //! Class constuctor
 //!
 //!
 DCWRF();
 virtual ~DCWRF();

 //! Initialize the DCWRF class
 //!
 //! Prepare a DCWRF for reading. This method prepares
 //! the master DCWRF file indicated by \p path for reading.
 //! The method should be called immediately after the constructor, 
 //! before any other class methods. This method
 //! exists only because C++ constructors can not return error codes.
 //!
 //! \param[in] path Path name of file that contains, or will
 //! contain, the DCWRF master file for this data collection
 //!
 //! \retval status A negative int is returned on failure
 //!
 //! \sa EndDefine();
 //
 virtual int Initialize(const vector <string> &paths);

 //! \copydoc DC::GetDimension()
 //!
 virtual bool GetDimension(
	string dimname, DC::Dimension &dimension
 ) const;

 //! \copydoc DC::GetDimensionNames()
 //!
 virtual std::vector <string> GetDimensionNames() const;

 //! \copydoc DC::GetCoordVarInfo()
 //!
 virtual bool GetCoordVarInfo(string varname, DC::CoordVar &cvar) const;

 //! \copydoc DC::GetDataVarInfo()
 //!
 virtual bool GetDataVarInfo( string varname, DC::DataVar &datavar) const;
 
 //! \copydoc DC::GetBaseVarInfo()
 //
 virtual bool GetBaseVarInfo(string varname, DC::BaseVar &var) const;


 //! \copydoc DC::GetDataVarNames()
 //!
 virtual std::vector <string> GetDataVarNames() const;


 //! \copydoc DC::GetCoordVarNames()
 //!
 virtual std::vector <string> GetCoordVarNames() const;


 //! \copydoc DC::GetCoordVarNames()
 //!
 virtual int GetNumRefLevels(string varname) const { return(1); }

 //! \copydoc DC::GetMapProjection()
 //!
 virtual int GetMapProjection(
    string lonname, string latname, string &projstring
 ) const;


 //! \copydoc DC::GetAtt()
 //!
 virtual int GetAtt(
	string varname, string attname, vector <double> &values
 ) const;
 virtual int GetAtt(
	string varname, string attname, vector <long> &values
 ) const;
 virtual int GetAtt(
	string varname, string attname, string &values
 ) const;

 //! \copydoc DC::GetAttNames()
 //!
 virtual std::vector <string> GetAttNames(string varname) const;

 //! \copydoc DC::GetAttType()
 //!
 virtual XType GetAttType(string varname, string attname) const;

 //! \copydoc DC::GetDimLensAtLevel()
 //!
 virtual int GetDimLensAtLevel(
	string varname, int level, std::vector <size_t> &dims_at_level,
	std::vector <size_t> &bs_at_level
 ) const;


 //! \copydoc DC::OpenVariableRead()
 //!
 virtual int OpenVariableRead(
	size_t ts, string varname, int , int 
 ) {
	return(DCWRF::OpenVariableRead(ts, varname));
 }

 virtual int OpenVariableRead(
	size_t ts, string varname
 );


 //! \copydoc DC::CloseVariable()
 //!
 virtual int CloseVariable();

 //! \copydoc DC::Read()
 //!
 int virtual Read(float *data);

 //! \copydoc DC::ReadSlice()
 //!
 virtual int ReadSlice(float *slice);

 //! \copydoc DC::ReadRegion()
 //
 virtual int ReadRegion(
    const vector <size_t> &min, const vector <size_t> &max, float *region
 );

 //! \copydoc DC::ReadRegionBlock()
 //!
 virtual int ReadRegionBlock(
    const vector <size_t> &min, const vector <size_t> &max, float *region
 );

 //! \copydoc DC::GetVar()
 //!
 virtual int GetVar(string varname, int, int, float *data) {
	return(DCWRF::GetVar(varname, data));
 }
 virtual int GetVar(string varname, float *data);

 //! \copydoc DC::GetVar()
 //!
 virtual int GetVar(
	size_t ts, string varname, int, int, float *data
 ) {
	return(DCWRF::GetVar(ts, varname, data));
 }

 virtual int GetVar(
	size_t ts, string varname, float *data
 );


 //! \copydoc DC::VariableExists()
 //!
 virtual bool VariableExists(
    size_t ts,
    string varname,
    int reflevel = 0,
    int lod = 0
 ) const;

private:
 NetCDFCollection *_ncdfc;
 VAPoR::UDUnits _udunits;

 //
 // Various attributes from a WRF data file needed for computing map 
 // projections
 //
 float _dx;
 float _dy;
 float _cen_lat;
 float _cen_lon;
 float _pole_lat;
 float _pole_lon;
 float _grav;
 float _radius;
 float _p2si;
 float _mapProj;

 std::vector<string> _timeStamps;
 std::vector<float> _times;
 int _ovr_fd;
 string _projString;
 Proj4API _proj4API;


 float *_sliceBuffer;

 class DerivedVarHorizontal;
 DerivedVarHorizontal *_derivedX;
 DerivedVarHorizontal *_derivedY;
 DerivedVarHorizontal *_derivedXU;
 DerivedVarHorizontal *_derivedYU;
 DerivedVarHorizontal *_derivedXV;
 DerivedVarHorizontal *_derivedYV;

 class DerivedVarElevation;
 DerivedVarElevation *_derivedElev;
 DerivedVarElevation *_derivedElevU;
 DerivedVarElevation *_derivedElevV;
 DerivedVarElevation *_derivedElevW;

 class DerivedVarTime;
 DerivedVarTime *_derivedTime;

 std::map <string, DC::Dimension> _dimsMap;
 std::map <string, DC::CoordVar> _coordVarsMap;
 std::map <string, DC::DataVar> _dataVarsMap;


 vector <size_t> _GetSpatialDims(
	NetCDFCollection *ncdfc, string varname
 ) const;

 vector <string> _GetSpatialDimNames(
	NetCDFCollection *ncdfc, string varname
 ) const;

 int _InitAtts(NetCDFCollection *ncdfc);

 int _GetProj4String(
	NetCDFCollection *ncdfc, float radius, int map_proj, string &projstring
 ); 

 int _InitProjection(NetCDFCollection *ncdfc, float radius); 

 int _InitHorizontalCoordinates(NetCDFCollection *ncdfc, Proj4API *proj4API); 

 int _InitVerticalCoordinates(NetCDFCollection *ncdfc); 

 int _InitTime(NetCDFCollection *ncdfc);

 int _InitDimensions(NetCDFCollection *ncdfc);

 int _GetCoordVars(
	NetCDFCollection *ncdfc,
	string varname, vector <string> &cvarnames
 ); 

 bool _GetVarCoordinates(
	NetCDFCollection *ncdfc, string varname,
	vector <DC::Dimension> &dimensions,
	vector <string> &coordvars
 ); 

 int _InitVars(NetCDFCollection *ncdfc);

 class DerivedVarElevation : public VAPoR::NetCDFCollection::DerivedVar {
 public:
  DerivedVarElevation(
	NetCDFCollection *ncdfc, string name, 
	const vector <DC::Dimension> &dims, float grav
  );
  virtual ~DerivedVarElevation();

  virtual int Open(size_t ts);
  virtual int ReadSlice(float *slice, int );
  virtual int Read(float *buf, int );
  virtual int SeekSlice(int offset, int whence, int );
  virtual int Close(int fd);
  virtual bool TimeVarying() const {return(true); };
  virtual std::vector <size_t>  GetSpatialDims() const { return(_sdims); }
  virtual std::vector <string>  GetSpatialDimNames() const {return(_sdimnames);}
  virtual size_t  GetTimeDim() const {return(_time_dim); }
  virtual string  GetTimeDimName() const {return(_time_dim_name); }
  virtual bool GetMissingValue(double &mv) const { return(false); }

 private:
  string _name;	// name of derived variable
  size_t _time_dim; // number of time steps
  string _time_dim_name; // Name of time dimension
  std::vector <size_t> _sdims;	// spatial dimensions
  std::vector <string> _sdimnames;	// spatial dimension names
  float _grav;	// gravitational constant
  string _PHvar; // name of PH variable
  string _PHBvar; // name of PHB variable
  float *_PH; // buffer for PH variable slice
  float *_PHB; // buffer for PHB variable slice
  float *_zsliceBuf;	// buffer for z starggering interpolation
  float *_xysliceBuf;	// buffer for horizontal extrapolation
  int _PHfd;
  int _PHBfd;	// file descriptors
  bool _is_open;	// variable open for reading?
  bool _xextrapolate;	// need extrapolation along X?
  bool _yextrapolate;	// need extrapolation along Y?
  bool _zinterpolate;	// need interpolation along Z?
  std::vector <size_t>_ph_dims;	// spatial dims of PH and PHB variables
  bool _firstSlice; // true if first slice for current open variable

  int _ReadSlice(float *slice);
 };


 class DerivedVarHorizontal : public NetCDFCollection::DerivedVar {
 public:
  DerivedVarHorizontal(
	NetCDFCollection *ncdfc, string name, const vector <DC::Dimension> &dims,
	const vector <size_t> latlondims, Proj4API *proj4API
  );
  virtual ~DerivedVarHorizontal();

  virtual int Open(size_t ts);
  virtual int ReadSlice(float *slice, int );
  virtual int Read(float *buf, int );
  virtual int SeekSlice(int offset, int whence, int );
  virtual int Close(int fd);
  virtual bool TimeVarying() const {return(true); };
  virtual std::vector <size_t>  GetSpatialDims() const { return(_sdims); }
  virtual std::vector <string>  GetSpatialDimNames() const {return(_sdimnames);}
  virtual size_t  GetTimeDim() const {return(_time_dim); }
  virtual string  GetTimeDimName() const {return(_time_dim_name); }
  virtual bool GetMissingValue(double &mv) const { return(false); }
 private:
  string _name;	// name of derived variable
  size_t _time_dim; // number of time steps
  string _time_dim_name; // Name of time dimension
  std::vector <size_t> _sdims;	// spatial dimensions
  std::vector <string> _sdimnames;	// spatial dimension names
  size_t _nx;
  size_t _ny;	// spatial dimensions  of XLONG and XLAT variables
  bool _is_open;	// Open for reading?
  float *_coords;	// cached coordinates
  float *_sliceBuf;	// space for reading lat an lon variables
  float *_lonBdryBuf;	// boundary points of lat and lon
  float *_latBdryBuf;	// boundary points of lat and lon
  size_t _ncoords;	// length of coords
  size_t _cached_ts;	// 	 time step of cached coordinate
  string _lonname;	
  string _latname;	// name of lat and lon coordinate variables
  Proj4API *_proj4API;

  int _GetCartCoords(size_t ts);
 };

 class DerivedVarTime : public NetCDFCollection::DerivedVar {
 public:
  DerivedVarTime(
	NetCDFCollection *ncdfc, DC::Dimension dims, 
	const std::vector <float> &timecoords
  );
  virtual ~DerivedVarTime() {}

  virtual int Open(size_t ts);
  virtual int ReadSlice(float *slice, int );
  virtual int Read(float *buf, int );
  virtual int SeekSlice(int offset, int whence, int );
  virtual int Close(int fd);
  virtual bool TimeVarying() const {return(true); };
  virtual std::vector <size_t>  GetSpatialDims() const { return(_sdims); }
  virtual std::vector <string>  GetSpatialDimNames() const {return(_sdimnames);}
  virtual size_t  GetTimeDim() const {return(_timecoords.size()); }
  virtual string  GetTimeDimName() const {return(_time_dim_name); }
  virtual bool GetMissingValue(double &mv) const { return(false); }
 private:
  string _time_dim_name; // Name of time dimension
  std::vector <size_t> _sdims;	// spatial dimensions
  std::vector <string> _sdimnames;	// spatial dimension names
  vector <float> _timecoords;	// cached coordinates
  size_t _ts;	// 	 current time step
 };

};
};

#endif
