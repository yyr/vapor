//
//      $Id$
//


#ifndef	_DCReaderROMS_h_
#define	_DCReaderROMS_h_

#include <vector>
#include <cassert>
#include <vapor/NetCDFCFCollection.h>
#include <vapor/DCReaderNCDF.h>
#include <vapor/WeightTable.h>
#ifdef WIN32
#pragma warning(disable : 4251)
#endif

namespace VAPoR {

//
//! \class DCReaderROMS
//! \brief ???
//!
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//!
class VDF_API DCReaderROMS : public DCReader {
public:

 DCReaderROMS(const std::vector <string> &files);

 virtual ~DCReaderROMS();

 virtual void   GetGridDim(size_t dim[3]) const {
	for (int i=0; i<3; i++) dim[i] = _dims[i];
 }

 void   GetBlockSize(size_t bs[3], int) const {
	DCReaderROMS::GetGridDim(bs);
 }

 virtual string GetGridType() const {
	if (_vertCVs.size()) return("layered"); 
	else return("regular");
 }

 virtual std::vector<double> GetTSZCoords(size_t ) const {
    return(_vertCoordinates);
 };

 virtual std::vector <double> GetExtents(size_t ts = 0) const;

 long GetNumTimeSteps() const {
	return(_ncdfc->GetNumTimeSteps());
 }

 virtual string GetMapProjection() const {
	return("+proj=latlon +ellps=sphere");
 };

 virtual std::vector <string> GetVariables3D() const {
    return(_vars3d);
 }; 

 virtual std::vector <string> GetVariables2DXY() const {
    return(_vars2dXY);
 }; 


 virtual std::vector <string> GetVariables2DXZ() const {
	std::vector <string> empty; return(empty);
 };

 virtual std::vector <string> GetVariables2DYZ() const {
	std::vector <string> empty; return(empty);
 };

 virtual std::vector <string> GetVariables3DExcluded() const {
    return(_vars3dExcluded);
 };

 virtual std::vector <string> GetVariables2DExcluded() const {
    return(_vars2dExcluded);
 };


 //
 // There a no spatial coordinate variables because we project
 // the data to a horizontally uniformly sampled Cartesian
 // grid.
 //
 virtual std::vector <string> GetCoordinateVariables() const {
	vector <string> v;
	v.push_back("NONE"); v.push_back("NONE"); v.push_back("NONE");
	return(v);
}

 //
 // Boundary may be periodic in X (longitude), but we have no way
 // of knowing this by examining the netCDF files
 //
 virtual std::vector<long> GetPeriodicBoundary() const {
    vector <long> p;
    p.push_back(0); p.push_back(0); p.push_back(0);
    return(p);
 }

 virtual std::vector<long> GetGridPermutation() const {
    vector <long> p;
    p.push_back(0); p.push_back(1); p.push_back(2);
    return(p);
}

 double GetTSUserTime(size_t ts) const ;

 bool GetMissingValue(string varname, float &value) const;

 void GetTSUserTimeStamp(size_t ts, string &s) const;
 
 virtual bool IsCoordinateVariable(string varname) const;


 virtual int OpenVariableRead(
    size_t timestep, string varname, int reflevel=0, int lod=0
 );

 virtual int CloseVariable();

 virtual int ReadSlice(float *slice);

 virtual int Read(float *data);

 virtual bool VariableExists(size_t ts, string varname) const {
	if (IsVariableDerived(varname)) return (true);
    return(_ncdfc->VariableExists(ts, varname));
 }

 virtual bool IsVariableDerived(string varname) const {
	return(find(_varsDerived.begin(), _varsDerived.end(), varname) != _varsDerived.end());
 }

private:
 std::vector <size_t> _dims;
 double _latExts[2];
 double _lonExts[2];
 std::vector <double> _vertCoordinates;
 std::vector <string> _vars3d;
 std::vector <string> _vars2dXY;
 std::vector <string> _vars3dExcluded;
 std::vector <string> _vars2dExcluded;
 std::vector <string> _varsDerived;
 WeightTable * _weightTable;
 std::map <string, string> _varsLatLonMap;
 NetCDFCFCollection *_ncdfc;
 float *_sliceBuffer;	// buffer for reading data
 float *_angleRADBuf;	// buffer for derived "angleRAD" variable
 float *_latDEGBuf;	// buffer for derived "latDEG" variable
 string _timeCV;	// time coordinate variable name
 std::vector <string> _latCVs;	// all valid latitude coordinate variables
 std::vector <string> _lonCVs;	// all valid longitude coordinate variables
 std::vector <string> _vertCVs;	// all valid vertical coordinate variables
 string _ovr_varname;
 size_t _ovr_slice;
 float _defaultMV;

 class latLonBuf {
 public:
  size_t _nx;
  size_t _ny;
  float *_latbuf;
  float *_lonbuf;
  float _latexts[2];
  float _lonexts[2];
 };


 int _initLatLonBuf(
	NetCDFCFCollection *ncdfc, string latvar, string lonvar,
	DCReaderROMS::latLonBuf &llb
 ) const;

 void _getRotationVariables(
	WeightTable * wt, float *_angleRADBuf, float *_latDEGBuf
 ) const;

 std::vector <size_t> _GetDims(
    NetCDFCFCollection *ncdfc, string varname
 ) const;
 std::vector <string> _GetDimNames(
    NetCDFCFCollection *ncdfc, string varname
 ) const;

 int _InitCoordVars(NetCDFCFCollection *ncdfc);

 void _ParseCoordVarNames(
    NetCDFCFCollection *ncdfc, const vector <string> &cvars,
    string &timecv, string &vertcv, string &latcv, string &loncv
 ) const;


 int _InitVerticalCoordinates(
	NetCDFCFCollection *ncdfc, 
	std::vector <string> &cvars, std::vector <double> &vertCoords
 );

 void _InitDimensions(
    NetCDFCFCollection *ncdfc,
    vector <size_t> &dims,
    vector <string> &vars3d,
    vector <string> &vars2dxy
 );

 float *_get_2d_var(NetCDFCFCollection *ncdfc, size_t ts, string name) const;
 float *_get_1d_var(NetCDFCFCollection *ncdfc, size_t ts, string name) const;

 

};
};

#endif	//	_DCReaderROMS_h_
