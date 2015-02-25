//
//      $Id$
//

#include <iostream>
#include <sstream>
#include <iterator>
#include <cassert>
#include <vector>

#include <vapor/GeoUtil.h>
#include <vapor/DCReaderMOM.h>
#include <vapor/Proj4API.h>
#ifdef WIN32
#pragma warning(disable : 4251)
#endif

using namespace VAPoR;
using namespace std;

DCReaderMOM::DCReaderMOM(const vector <string> &files) {

	_dims.clear();
	_latExts[0] = _latExts[1] = 0.0;
	_lonExts[0] = _lonExts[1] = 0.0;
	_vertCoordinates.clear();
	_cartesianExtents.clear();
	_vars3d.clear();
	_vars2dXY.clear();
	_vars3dExcluded.clear();
	_vars2dExcluded.clear();
	_varsDerived.clear();
	_weightTableMap.clear();
	_varsLatLonMap.clear();
	_ncdfc = NULL;
	_sliceBuffer = NULL;
	_vertCV.clear();
	_timeCV.clear();
	_latCVs.clear();
	_lonCVs.clear();
	_ovr_weight_tbl = NULL;
	_ovr_varname.clear();
	_ovr_slice = 0;
	_ovr_nz = 0;
	_ovr_fd = -1;
	_defaultMV = 1e37;
	_reverseRead = false;
	_angleRADBuf = NULL;
	_latDEGBuf = NULL;

	NetCDFCFCollection *ncdfc = new NetCDFCFCollection();
	int rc = ncdfc->Initialize(files);
    if (rc<0) {
        SetErrMsg("Failed to initialize netCDF data collection for reading");
        return;
    }

	//
	// Identify data and coordinate variables. Sets up members:
	// _vars2dXY, _vars3d, _varsLatLonMap, _weightTableMap,
	// _latCVs, _lon_CVs, _timeCV, _vertCV, _vars3dExcluded,
	// _vars2dExcluded
	//
	rc = _InitCoordVars(ncdfc) ;
	if (rc<0) return;

	if (! _vertCV.empty() && ncdfc->IsVertDimensionless(_vertCV)) {
		SetErrMsg("Dimensionless vertical coordinates not currently supported");
		return;
	}

	rc = _InitVerticalCoordinates(
		ncdfc, _vertCV, _vertCoordinates 
	);
	if (rc<0) {
		SetErrMsg("Unrecognized units for vertical coordinate");
		return;
	}

	//
	//  Get the dimensions of the grid. Destructively process
	// _vars3d and _var2dxy. Any variables with dimensions that don't
	// match the "one" grid are tossed
	//
	_InitDimensions(ncdfc, _dims, _vars3d, _vars2dXY);
	if (!_dims.size()) {
		SetErrMsg("No valid variables");
		return;
	}

	_sliceBuffer = new float[_dims[0]*_dims[1]];

	//
	// Set up weight tables to intepolate data variables onto a lat-lon
	// grid. Each data variable can conceivably be sampled on a 
	// different horizontal grid. We need a weight table for each 
	// grid used. The map '_weightTableMap' contains an entry (key)
	// for each grid.
	//
	// This is a two-pass algorithm. The first time through we read
	// in all the coordinate variables and temporarily store them.
	// Then we calculate the min and max lat-lon extents for all
	// grids. In the second
	// pass we generate the weight tables.
	//
	map <string, WeightTable *>::iterator itr;
	map <string, latLonBuf> lat_lon_buf_map;
	bool first = true;
	for (itr = _weightTableMap.begin(); itr !=_weightTableMap.end(); ++itr) {
		string key = itr->first;
		string latcv, loncv;	// lat and lon coord variable names;
		_decodeLatLon(key, latcv, loncv);
		latLonBuf llb;

		rc = _initLatLonBuf(ncdfc, latcv, loncv, llb);
		if (rc<0) return;
		
		lat_lon_buf_map[key] = llb;

		if (first) {
			_latExts[0] = llb._latexts[0];
			_latExts[1] = llb._latexts[1];
			_lonExts[0] = llb._lonexts[0];
			_lonExts[1] = llb._lonexts[1];
		}
		else {
			if (llb._latexts[0] < _latExts[0]) _latExts[0] = llb._latexts[0];
			if (llb._latexts[1] > _latExts[1]) _latExts[1] = llb._latexts[1];
			if (llb._lonexts[0] < _lonExts[0]) _lonExts[0] = llb._lonexts[0];
			if (llb._lonexts[1] > _lonExts[1]) _lonExts[1] = llb._lonexts[1];
		}
	}

	for (itr = _weightTableMap.begin(); itr !=_weightTableMap.end(); ++itr) {
		string key = itr->first;
		string latcv, loncv;
		_decodeLatLon(key, latcv, loncv);
		latLonBuf llb = lat_lon_buf_map[key];

		itr->second = new WeightTable(
			llb._latbuf, llb._lonbuf, llb._ny, llb._nx, 
			llb._latexts, llb._lonexts
		);
		if (llb._latbuf) delete [] llb._latbuf;
		if (llb._lonbuf) delete [] llb._lonbuf;
	}

	rc = _InitCartographicExtents(
		GetMapProjection(), _lonExts, _latExts, _vertCoordinates,
		_cartesianExtents
	);
	if (rc<0) return;

	//
	// Compute the two derived variables: angleRAD and latDEG
	//
	_vars2dXY.push_back("angleRAD");
	_vars2dXY.push_back("latDEG");
	_varsDerived.push_back("angleRAD");
	_varsDerived.push_back("latDEG");

	_angleRADBuf = new float[_dims[0]*_dims[1]];
	_latDEGBuf = new float[_dims[0]*_dims[1]];
	_getRotationVariables(_weightTableMap, _angleRADBuf, _latDEGBuf);

	_ncdfc = ncdfc;

	sort(_vars3d.begin(), _vars3d.end());
	sort(_vars2dXY.begin(), _vars2dXY.end());
	sort(_vars3dExcluded.begin(), _vars3dExcluded.end());
	sort(_vars2dExcluded.begin(), _vars2dExcluded.end());
}

string DCReaderMOM::GetMapProjection() const {
	double lon_0 = (_lonExts[0] + _lonExts[1]) / 2.0;
	double lat_0 = (_latExts[0] + _latExts[1]) / 2.0;
	ostringstream oss;
	oss << " +lon_0=" << lon_0 << " +lat_0=" << lat_0;
	string projstring = "+proj=eqc +ellps=WGS84" + oss.str();

    return(projstring);
}

vector <size_t> DCReaderMOM::_GetSpatialDims(
	NetCDFCFCollection *ncdfc, string varname
) const {
	vector <size_t> dims = ncdfc->GetSpatialDims(varname);
	reverse(dims.begin(), dims.end());
	return(dims);
}

int DCReaderMOM::_InitVerticalCoordinates(
	NetCDFCFCollection *ncdfc, 
	string cvar,
	vector <double> &vertCoords
) {
	vertCoords.clear();
	double scaleFactor = 1.0;

	//
	// Handle case if there is no vertical coordinate variable
	//
	if (cvar.empty()) {
		vertCoords.push_back(0.0);
		vertCoords.push_back(0.0);
		return(0);
	}

	//
	// Read the vertical coordinates from cvar
	//
	vector <size_t> dims = _GetSpatialDims(ncdfc, cvar);

	float *buf = _get_1d_var(ncdfc, 0, cvar);
	if (! buf) return(-1);

	if (! ncdfc->IsVertCoordVarUp(cvar)) {
		for (int i=dims[0]-1; i>= 0; i--) {
			vertCoords.push_back(-1.0*buf[i]);
		}
		_reverseRead = true;
	}
	else {
		for (int i=0; i<dims[0]; i++) {
			vertCoords.push_back(buf[i]);
		}
		_reverseRead = false;
	}
	delete [] buf;

	// Now get the conversion scale factor from whatever units 
	// the vertical coordinate is expressed in to meters. N.B. the 
	// vertical unit may be expressed as pressure.
	//
	// Get the units for the vertical coordinate variable
	//
	string from;
	int rc = ncdfc->GetVarUnits(cvar, from);
	if (rc<0) return(-1);

	double unkown_unit = 1.0;

	const UDUnits *udunit = ncdfc->GetUDUnits();
	if (udunit->IsPressureUnit(from)) {
		string to = "dbars";
		double p;
		if (! udunit->Convert(from, to, &unkown_unit, &p, 1)) {
			return(-1);
		}
		const float g = 9.80665;	// gravity
		scaleFactor = ((((-1.82e-15  * p + 2.279e-10 ) * p - 2.2512e-5 ) * p + 9.72659) * p) / g;

	}
	else {
		//
		// Convert 1 unit of the unknown vertical unit to a meter
		//
		string to = "meter";
		if (! udunit->Convert(from, to, &unkown_unit, &scaleFactor, 1)) {
	
			// Truly unknown unit. Skip conversion
			//
			scaleFactor = 1.0;	
//			return(-1);
		}
	}

	// 
	// Finally, convert to meters
	// Doh! Probably should just let UDUnits do this
	//
	for (int i=0; i<vertCoords.size(); i++) {
		vertCoords[i] *= scaleFactor;
	}

	return(0);
}

void DCReaderMOM::_InitDimensions(
	NetCDFCFCollection *ncdfc,
	vector <size_t> &dims,
	vector <string> &vars3d,
	vector <string> &vars2dxy
) {
	
	dims.clear();

	//
	// Get the dimension of 3D variables. All dimensions must be
	// be equal or we discard the variable. sigh
	//
	vector <string> newvars;
	for (int i=0; i<vars3d.size(); i++) {
        vector <size_t> dims1 = _GetSpatialDims(ncdfc, vars3d[i]);
		if (! dims.size()) {
			dims = dims1;
		}
		else {
			if (dims != dims1) continue;
		}
		newvars.push_back(vars3d[i]);
	}
	vars3d = newvars;

	//
	// Now check the 2D variables. Discard any 2D variables with 
	// differing dimensions from the first variable processed
	//
	newvars.clear();
	for (int i=0; i<vars2dxy.size(); i++) {
        vector <size_t> dims1 = _GetSpatialDims(ncdfc, vars2dxy[i]);
		if (! dims.size()) {
			dims = dims1;
			dims.push_back(1);
		}
		else {
			if ((dims[0] != dims1[0]) || (dims[1] != dims1[1])) continue;
		}
		newvars.push_back(vars2dxy[i]);
	}
	vars2dxy = newvars;
}


DCReaderMOM::~DCReaderMOM() {
	map <string, WeightTable *>::iterator itr;
	for (itr = _weightTableMap.begin(); itr !=_weightTableMap.end(); ++itr) {
		if (itr->second) delete itr->second;
	}
	if (_sliceBuffer) delete [] _sliceBuffer;
	if (_angleRADBuf) delete [] _angleRADBuf;
	if (_latDEGBuf) delete [] _latDEGBuf;

	if (_ncdfc) delete _ncdfc;
}

double DCReaderMOM::GetTSUserTime(size_t ts) const {
	if (ts >= _ncdfc->GetNumTimeSteps()) return(0.0); 

	double time_d;
	_ncdfc->GetTime(ts, time_d);


	// Convert time from whatever is used in the file to seconds
	//
	string from;
	double from_time = time_d;
	int rc = _ncdfc->GetVarUnits(_timeCV, from);
	if (rc<0) return(from_time);

	string to = "seconds";
	double to_time;

	rc = _ncdfc->Convert(from, to, &from_time, &to_time, 1);
	if (rc<0) return(from_time);

	return(to_time);
};

void DCReaderMOM::GetTSUserTimeStamp(size_t ts, string &s) const {
	s.clear();
	double time_d = DCReaderMOM::GetTSUserTime(ts);

	_ncdfc->FormatTimeStr(time_d, s);
}

bool DCReaderMOM::GetMissingValue(string varname, float &value) const {
	value = 0.0;

	if (IsVariableDerived(varname)) {
		value = _defaultMV;
		return(true);
	}

	double value_d;
	bool has_missing = _ncdfc->GetMissingValue(varname, value_d);
	if (has_missing) {
		value = (float) value_d;
	}
	else {

		//
		// Even though netCDF variable doesn't define a missing value
		// for this variable resampling to a Regular grid may
		// introduce missing values unless the native grid is already
		// lat-lon
		//
		bool lat_lon_grid = true;
		for (int i=0; i<_latCVs.size(); i++) {
			if (_GetSpatialDims(_ncdfc, _latCVs[i]).size() != 1) lat_lon_grid = false;
		}
		for (int i=0; i<_lonCVs.size(); i++) {
			if (_GetSpatialDims(_ncdfc, _lonCVs[i]).size() != 1) lat_lon_grid = false;
		}
		if (lat_lon_grid) {
			has_missing = false;
		}
		else {
			//
			// We always have a missing value because the native grid is 
			// resampled to 
			value = (float) 1e37;
			has_missing = true;
		}
	}
	return(has_missing);
}

bool DCReaderMOM::IsCoordinateVariable(string varname) const {
	return(false);
}

int DCReaderMOM::OpenVariableRead(
    size_t timestep, string varname, int, int 
) {
	DCReaderMOM::CloseVariable();

	_ovr_weight_tbl = NULL;
	_ovr_varname.clear();
	_ovr_slice = 0;
	_ovr_fd = -1;

	if (IsVariableDerived(varname)) { 
		_ovr_varname = varname;
		_ovr_nz = 1;	// derived variables are 2D
		return(0);
	}

	//
	// Find the appropriate weight table to re-grid this variable
	//
	map <string, string>::const_iterator itr;
	if ((itr = _varsLatLonMap.find(varname)) == _varsLatLonMap.end()) {
		SetErrMsg("Invalid variable : %s", varname.c_str());
		return(-1);
	}
	string key = itr->second;

	map <string, WeightTable *>::const_iterator itr1;
	itr1 = _weightTableMap.find(key);
	assert(itr1 != _weightTableMap.end());

	_ovr_weight_tbl = itr1->second;
	_ovr_varname = varname;
	_ovr_nz = (_GetSpatialDims(_ncdfc, varname).size() == 3) ? _dims[2] : 1;

	_ovr_fd = _ncdfc->OpenRead(timestep, varname);
	if (_ovr_fd < 0) return (_ovr_fd);

	if (_reverseRead) {
		_ncdfc->SeekSlice(0,2,_ovr_fd);
	}
	return(_ovr_fd);
}

int DCReaderMOM::ReadSlice(float *slice) {

	if (_ovr_slice >= _ovr_nz) {
		return(0); // EOF
	}

	//
	// Deal with derived variables
	//
	if (IsVariableDerived(_ovr_varname)) {
		const float *ptr;
		if (_ovr_varname.compare("angleRAD") == 0) {
			ptr = _angleRADBuf;
		}
		else {
			ptr = _latDEGBuf;
		}
		for (int i=0; i<_dims[0]*_dims[1]; i++) {
			slice[i] = ptr[i];
		}
		_ovr_slice++;
		return(1);
	}
	if (_ovr_fd < 0) return (-1);

	if (_GetSpatialDims(_ncdfc, _ovr_varname).size() < 2) {
		SetErrMsg("Invalid operation");
		return(-1);
	}

	int rc = _ncdfc->ReadSlice(_sliceBuffer, _ovr_fd);
	if (rc<1) return(rc);

    if (_reverseRead) {
        _ncdfc->SeekSlice(-2,1,_ovr_fd);
    }

	float mv;
	bool has_missing = DCReaderMOM::GetMissingValue(_ovr_varname, mv);

	//
	// If there are no missing values resampling may still 
	// introduce them.
	//
	if (! has_missing) mv = _defaultMV;

	size_t dims[] = {_dims[0], _dims[1]};
	_ovr_weight_tbl->interp2D(_sliceBuffer, slice, mv, mv, dims);

	_ovr_slice++;
	return(1);
	
}

int DCReaderMOM::Read(float *data) {
	float *ptr = data;

	int rc;
	while ((rc = DCReaderMOM::ReadSlice(ptr)) > 0) {
		ptr += _dims[0] * _dims[1];
	}
	return(rc);
}

int DCReaderMOM::CloseVariable() {

	bool derived = IsVariableDerived(_ovr_varname);
	_ovr_weight_tbl = NULL;
	_ovr_varname.clear();
	_ovr_slice = 0;

	if (derived) return(0);

	if (_ovr_fd < 0) return(0);
	int rc = _ncdfc->Close(_ovr_fd);
	_ovr_fd = -1;
	return(rc);
}


float *DCReaderMOM::_get_2d_var(
	NetCDFCFCollection *ncdfc, size_t ts, string name
) const {

	vector <size_t> dims = _GetSpatialDims(ncdfc, name);
	if (dims.size() != 2) return (NULL);

	float *buf = new float[dims[0]*dims[1]];

	int fd;
	if ((fd = ncdfc->OpenRead(0, name)) < 0) { 
		SetErrMsg(
			"Missing required grid variable \"%s\n", name.c_str()
		);
		delete [] buf;
		return(NULL);
	}
	if (ncdfc->Read(buf, fd) < 0) {
		SetErrMsg(
			"Missing required grid variable \"%s\n", name.c_str()
		);
		delete [] buf;
	}
	ncdfc->Close(fd);
	return(buf);
}

float *DCReaderMOM::_get_1d_var(
	NetCDFCFCollection *ncdfc, size_t ts, string name
) const {

	vector <size_t> dims = _GetSpatialDims(ncdfc, name);
	if (dims.size() != 1) return (NULL);

	float *buf = new float[dims[0]];

	int fd;
	if ((fd = ncdfc->OpenRead(0, name)) < 0) { 
		SetErrMsg(
			"Missing required grid variable \"%s\n", name.c_str()
		);
		delete [] buf;
		return(NULL);
	}
	if (ncdfc->Read(buf, fd) < 0) {
		SetErrMsg(
			"Missing required grid variable \"%s\n", name.c_str()
		);
		delete [] buf;
	}
	ncdfc->Close(fd);
	return(buf);
}

void DCReaderMOM::_ParseCoordVarNames(
	NetCDFCFCollection *ncdfc, const vector <string> &cvars, 
	string &timecv, string &vertcv, string &latcv, string &loncv
) const {
	timecv.clear();
	vertcv.clear();
	latcv.clear();
	loncv.clear();

	//
	// Look for time, vertical, lat, and lon coordinate variables. We
	// ignore a variable if we don't know  it's type, and we don't check
	// for the possibility that a coordinate variable of a given type
	// appears twice.
	//
	for (int i=0; i<cvars.size(); i++) {
		if (ncdfc->IsTimeCoordVar(cvars[i])) {
			timecv = cvars[i];
		}
		else if (ncdfc->IsVertCoordVar(cvars[i])) {
			vertcv = cvars[i];
		}
		else if (ncdfc->IsLatCoordVar(cvars[i])) {
			latcv = cvars[i];
		}
		else if (ncdfc->IsLonCoordVar(cvars[i])) {
			loncv = cvars[i];
		}
	}
}

int DCReaderMOM::_InitCoordVars(NetCDFCFCollection *ncdfc) 
{
	_vars2dXY.clear();
	_vars3d.clear();
	_vars3dExcluded.clear();
	_vars2dExcluded.clear();
	_varsLatLonMap.clear();
	_weightTableMap.clear();
	_timeCV.clear();
	_vertCV.clear();
	_latCVs.clear();
	_lonCVs.clear();

	//
	// Find the coordinate variables for each 2D data variable. 
	// A latitude and longitude coordinate variable must exist
	// for each data variable or we consider the data variable invalid
	// and ignore it
	//
	vector <string> vars = ncdfc->GetDataVariableNames(2, true);
	for (int i=0; i<vars.size(); i++) {

		bool excluded = false;
		vector <string> cvars;
		int rc = ncdfc->GetVarCoordVarNames(vars[i], cvars);
		if (rc<0) return(rc);

		string timecv, vertcv, latcv, loncv;
		_ParseCoordVarNames(ncdfc, cvars, timecv, vertcv, latcv, loncv);
		if (latcv.empty() || loncv.empty()) excluded = true;

		//
		// Only one time coordinate variable permitted
		//
		if (_timeCV.empty() && !timecv.empty()) _timeCV = timecv;
		if (! _timeCV.empty() && ! timecv.empty() && _timeCV.compare(timecv) != 0) excluded = true;

		//
		// Lat and lon coordinate variables must be 1D or 2D and
		// not time varying
		//
		if (! excluded && ncdfc->IsTimeVarying(latcv)) excluded = true;
		if (! excluded && ncdfc->IsTimeVarying(loncv)) excluded = true;
		if (! excluded && _GetSpatialDims(ncdfc, latcv).size() > 2) excluded = true;
		if (! excluded && _GetSpatialDims(ncdfc, loncv).size() > 2) excluded = true;

		if (excluded) {
			_vars2dExcluded.push_back(vars[i]);
			continue;
		}

		if (find(_latCVs.begin(), _latCVs.end(), latcv) == _latCVs.end()) {
			_latCVs.push_back(latcv);
		}
		if (find(_lonCVs.begin(), _lonCVs.end(), loncv) == _lonCVs.end()) {
			_lonCVs.push_back(loncv);
		}

		_vars2dXY.push_back(vars[i]);

		//
		// Encode lat and lon coordinate variable name as a unique
		// string (key) for a particular combination of these two
		// coordinate variables.
		//
		string key;
		_encodeLatLon(latcv,loncv, key);
		_varsLatLonMap[vars[i]] = key;
		_weightTableMap[key] = NULL;
	}

	vars = ncdfc->GetDataVariableNames(3, true);
	for (int i=0; i<vars.size(); i++) {

		bool excluded = false;
		vector <string> cvars;
		int rc = ncdfc->GetVarCoordVarNames(vars[i], cvars);
		if (rc<0) return(rc);

		string timecv, vertcv, latcv, loncv;
		_ParseCoordVarNames(ncdfc, cvars, timecv, vertcv, latcv, loncv);
		if (vertcv.empty() || latcv.empty() || loncv.empty()) excluded = true;

		//
		// Only one time and one vertical coordinate variable permitted
		// Vertical coordinate variable must be 1D (and so must time);
		//
		if (_timeCV.empty() && !timecv.empty()) _timeCV = timecv;
		if (! _timeCV.empty() && ! timecv.empty() && _timeCV.compare(timecv) != 0) excluded = true;
		if (_vertCV.empty()) _vertCV = vertcv;

		// Thu Aug  1 14:56:37 MDT 2013 : relax requirement that all
		// 3d vars use same vertical coordinate so POP WVEL variables are
		// not excluded
		//
		// if (! _vertCV.empty() && _vertCV.compare(vertcv) != 0) excluded = true;
		if (! excluded && _GetSpatialDims(ncdfc, vertcv).size() > 1) excluded = true;

		//
		// Lat and lon coordinate variables must be 1D or 2D and
		// not time varying
		//
		if (! excluded && ncdfc->IsTimeVarying(latcv)) excluded = true;
		if (! excluded && ncdfc->IsTimeVarying(loncv)) excluded = true;
		if (! excluded && _GetSpatialDims(ncdfc, latcv).size() > 2) excluded = true;
		if (! excluded && _GetSpatialDims(ncdfc, loncv).size() > 2) excluded = true;

		if (excluded) {
			_vars3dExcluded.push_back(vars[i]);
			continue;
		}

		if (find(_latCVs.begin(), _latCVs.end(), latcv) == _latCVs.end()) {
			_latCVs.push_back(latcv);
		}
		if (find(_lonCVs.begin(), _lonCVs.end(), loncv) == _lonCVs.end()) {
			_lonCVs.push_back(loncv);
		}
		_vars3d.push_back(vars[i]);

		string key;
		_encodeLatLon(latcv,loncv, key);
		_varsLatLonMap[vars[i]] = key;
		_weightTableMap[key] = NULL;
	}

	return(0);
}

int DCReaderMOM::_InitCartographicExtents(
    string mapProj,
    const double lonExts[2],
    const double latExts[2],
    const std::vector <double> vertCoordinates,
    std::vector <double> &extents
) const {
	extents.clear();

	Proj4API proj4API;

	int rc = proj4API.Initialize("", mapProj);
	if (rc<0) {
		SetErrMsg("Invalid map projection : %s", mapProj.c_str());
		return(-1);
	}

	double x[] = {lonExts[0], lonExts[1]};
	double y[] = {latExts[0], latExts[1]};

	rc = proj4API.Transform(x,y,2,1);
	if (rc < 0) {
		SetErrMsg("Invalid map projection : %s", mapProj.c_str());
		return(-1);
	}
	extents.push_back(x[0]);
	extents.push_back(y[0]);
	extents.push_back(vertCoordinates[0]);
	extents.push_back(x[1]);
	extents.push_back(y[1]);
	extents.push_back(vertCoordinates[vertCoordinates.size()-1]);

	return(0);
}

void DCReaderMOM::_encodeLatLon(string latcv,string loncv, string &key) const {
	key = latcv + " " + loncv;	// white space delmited string
}

void DCReaderMOM::_decodeLatLon(
	string key, string &latcv, string &loncv
) const {

	vector <string> v;
    stringstream ss(key);
    istream_iterator<std::string> begin(ss);
    istream_iterator<std::string> end;
    v.insert(v.begin(), begin, end);
	assert(v.size() == 2);
	latcv = v[0];
	loncv = v[1];
}

int DCReaderMOM::_initLatLonBuf(
	NetCDFCFCollection *ncdfc, string latvar, string lonvar, 
	DCReaderMOM::latLonBuf &llb
) const {

	llb._nx = 0;
	llb._ny = 0;
	llb._latbuf = NULL;
	llb._lonbuf = NULL;
	llb._latexts[0] = llb._latexts[1] = 0.0;
	llb._lonexts[0] = llb._lonexts[1] = 0.0;

	vector <size_t> latdims = _GetSpatialDims(ncdfc, latvar);
	vector <size_t> londims = _GetSpatialDims(ncdfc, lonvar);
	if (latdims.size() == 1) {
		llb._ny = latdims[0];
	}
	else {	// Must be rank 2 if not rank 1 
		llb._nx = latdims[0];
		llb._ny = latdims[1];
	}

	//
	// Lon dims must match lat
	//
	if (londims.size() == 1) {
		if (llb._nx && llb._nx != londims[0]) {
			SetErrMsg("Lat and Lon dimension mismatch");
			return(-1);
		}
		else {
			llb._nx = londims[0];
		}
	}
	else {
		if ((llb._nx && llb._nx != londims[0]) || (llb._ny && llb._ny != londims[1])) {
			SetErrMsg("Lat and Lon dimension mismatch");
			return(-1);
		}
		else {
			llb._nx = londims[0];
			llb._ny = londims[1];
		}
	}
	
	
	//
	// Read lat and lon coordinate variables
	//
	if (latdims.size() == 1) {
		float *buf = _get_1d_var(ncdfc, 0, latvar);
		if (! buf) return(-1);
		
		//
		// Weight tables only understand 2D lat and lon coordinate variables
		//
		llb._latbuf = new float[llb._nx*llb._ny];
		for (int j=0; j<llb._ny; j++) {
		for (int i=0; i<llb._nx; i++) {
			llb._latbuf[j*llb._nx + i] = buf[j];
			
		}
		}
		delete [] buf;
	}
	else {
		llb._latbuf = _get_2d_var(ncdfc, 0, latvar);
		if (! llb._latbuf) return(-1);
	}

	if (londims.size() == 1) {
		float *buf = _get_1d_var(ncdfc, 0, lonvar);
		if (! buf) {
			delete [] llb._latbuf;
			return(-1);
		}

		llb._lonbuf = new float[llb._nx*llb._ny];
		for (int j=0; j<llb._ny; j++) {
		for (int i=0; i<llb._nx; i++) {
			llb._lonbuf[j*llb._nx + i] = buf[i];
		}
		}
		delete [] buf;
	}
	else {
		llb._lonbuf = _get_2d_var(ncdfc, 0, lonvar);
		if (! llb._lonbuf) {
			delete [] llb._latbuf;
			return(-1);
		}
	}

	GeoUtil::LonExtents(
		llb._lonbuf, llb._nx, llb._ny, llb._lonexts[0], llb._lonexts[1]
	);
	GeoUtil::LatExtents(
		llb._latbuf, llb._nx, llb._ny, llb._latexts[0], llb._latexts[1]
	);

	return(0);

#ifdef	DEAD

	//
	// Get lat extents.  Really only need to check data on boundary, 
	// but we're lazy. N.B. doesn't handle case where data cross either pole.
	// 
	//
	llb._latexts[0] = llb._latexts[1] = llb._latbuf[0];
	for (int j=0; j<llb._ny; j++) {
	for (int i=0; i<llb._nx; i++) {
		float tmp = llb._latbuf[j*llb._nx+i];
		llb._latexts[0] = tmp < llb._latexts[0] ? tmp : llb._latexts[0];
		llb._latexts[1] = tmp > llb._latexts[1] ? tmp : llb._latexts[1];
	}
	}


	//
	// Now deal with longitude, which may wrap (i.e. the values may
	// not be monotonicly increasing along a scan line. First we 
	// handle wraparound. We simply look for a big jump between adjacent
	// points. N.B. testing for changes from increasing to decreasing (or
	// vise versa don't work for data sets that are extremely distored).
	//
	for (int j=0; j<llb._ny; j++) {
	for (int i=0; i<llb._nx-1; i++) {
		float delta = 180.0;	
		if (fabs(llb._lonbuf[j*llb._nx+i] - llb._lonbuf[j*llb._nx+i+1])>delta) {
			llb._lonbuf[j*llb._nx+i+1] += 360.0;
		}
	}
	}

	//
	// Now get lon extents. 
	//
	llb._lonexts[0] = llb._lonexts[1] = llb._lonbuf[0];
	for (int j=0; j<llb._ny; j++) {
	for (int i=0; i<llb._nx; i++) {
		//
		// Ugh. Don't look at longitudes near poles 'cause things 
		// get squirely there
		//
		if (llb._latbuf[j*llb._nx+i] > -60.0 && llb._latbuf[j*llb._nx+i] < 60.0){
			float tmp = llb._lonbuf[j*llb._nx+i];
			llb._lonexts[0] = tmp < llb._lonexts[0] ? tmp : llb._lonexts[0];
			llb._lonexts[1] = tmp > llb._lonexts[1] ? tmp : llb._lonexts[1];
		}
	}
	}

	//
	// Finally, try to bring everything back to -360 to 360
	//
	if (llb._lonexts[0] > 180 || llb._lonexts[1] > 360.0) {
		for (int j=0; j<llb._ny; j++) {
		for (int i=0; i<llb._nx; i++) {
				llb._lonbuf[j*llb._nx+i] -= 360.0;
		}
		}
		llb._lonexts[0] -= 360.0;
		llb._lonexts[1] -= 360.0;
	}
	if ((llb._lonexts[1] - llb._lonexts[0]) > 360.0) {
		llb._lonexts[1] = llb._lonexts[0] + 360.0;
	}
	return(0);
#endif

}

void DCReaderMOM::_getRotationVariables(
    const std::map <string, WeightTable *> _weightTableMap,
	float *_angleRADBuf, float *_latDEGBuf
) const {

	//
	// Just grab the first weight able. Technically, the should be a 
	// different pair of rotation variables for each weight table :-(
	//
	map <string, WeightTable *>::const_iterator itr = _weightTableMap.begin();
	WeightTable *wt = itr->second;

	float mv = _defaultMV;
	for (int lat = 0; lat<_dims[1]; lat++){
	for (int lon = 0; lon<_dims[0]; lon++){
		_sliceBuffer[_dims[0]*lat + lon] = wt->getAngle(lon,lat);
	}
	}
	size_t dims[] = {_dims[0], _dims[1]};
	wt->interp2D(_sliceBuffer, _angleRADBuf, mv, mv, dims);

	for (int lat = 0; lat<_dims[1]; lat++){
	for (int lon = 0; lon<_dims[0]; lon++){
		_sliceBuffer[_dims[0]*lat + lon] = wt->getGeoLats()[_dims[0]*lat + lon];
	}
	}
	wt->interp2D(_sliceBuffer, _latDEGBuf, mv, mv, dims);

}
