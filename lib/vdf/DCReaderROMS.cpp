//
//      $Id$
//

#include <iostream>
#include <sstream>
#include <iterator>
#include <cassert>

#include <vapor/DCReaderROMS.h>
#ifdef WIN32
#pragma warning(disable : 4251)
#endif

using namespace VAPoR;
using namespace std;

DCReaderROMS::DCReaderROMS(const vector <string> &files) {

	_dims.clear();
	_latExts[0] = _latExts[1] = 0.0;
	_lonExts[0] = _lonExts[1] = 0.0;
	_vertCoordinates.clear();
	_vars3d.clear();
	_vars2dXY.clear();
	_vars3dExcluded.clear();
	_vars2dExcluded.clear();
	_varsDerived.clear();
	_weightTable = NULL;
	_varsLatLonMap.clear();
	_ncdfc = NULL;
	_sliceBuffer = NULL;
	_vertCVs.clear();
	_timeCV.clear();
	_latCVs.clear();
	_lonCVs.clear();
	_ovr_varname.clear();
	_ovr_slice = 0;
	_ovr_fd = -1;
	_defaultMV = 1e37;
	_angleRADBuf = NULL;
	_latDEGBuf = NULL;

	NetCDFCFCollection *ncdfc = new NetCDFCFCollection();
	ncdfc->Initialize(files);
	if (GetErrCode() != 0) return;

	//
	// Identify data and coordinate variables. Sets up members:
	// _vars2dXY, _vars3d, _varsLatLonMap, 
	// _latCVs, _lonCVs, _timeCV, _vertCVs, _vars3dExcluded,
	// _vars2dExcluded
	//
	int rc = _InitCoordVars(ncdfc) ;
	if (rc<0) return;

	rc = _InitVerticalCoordinates(
		ncdfc, _vertCVs, _vertCoordinates 
	);
	if (rc<0) {
		SetErrMsg("Unrecognized units for vertical coordinate");
		return;
	}
	if (_vertCVs.size()) _vars3d.push_back("ELEVATION");

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
	// Set up weight table to intepolate data variables onto a lat-lon
	// grid. 
	//
	//
	map <string, latLonBuf> lat_lon_buf_map;
	string latcv = _latCVs[0];
	string loncv = _lonCVs[0]; // lat and lon coord variable names;
	latLonBuf llb;

	rc = _initLatLonBuf(ncdfc, latcv, loncv, llb);
	if (rc<0) return;
		

	_latExts[0] = llb._latexts[0];
	_latExts[1] = llb._latexts[1];
	_lonExts[0] = llb._lonexts[0];
	_lonExts[1] = llb._lonexts[1];

	_weightTable = new WeightTable(
			llb._latbuf, llb._lonbuf, llb._ny, llb._nx, 
			llb._latexts, llb._lonexts
		);
	if (llb._latbuf) delete [] llb._latbuf;
	if (llb._lonbuf) delete [] llb._lonbuf;

	//
	// Compute the two derived variables: angleRAD and latDEG
	//
	_vars2dXY.push_back("angleRAD");
	_vars2dXY.push_back("latDEG");
	_varsDerived.push_back("angleRAD");
	_varsDerived.push_back("latDEG");

	_angleRADBuf = new float[_dims[0]*_dims[1]];
	_latDEGBuf = new float[_dims[0]*_dims[1]];
	_getRotationVariables(_weightTable, _angleRADBuf, _latDEGBuf);

	_ncdfc = ncdfc;
}

vector <double> DCReaderROMS::GetExtents(size_t ) const {
	vector <double> cartesianExtents(6);

	
	// 
	// Convert horizontal extents expressed in lat-lon to Cartesian
	// coordinates in whatever units the vertical coordinate is 
	// expressed in. Multiply lat-lon by 111177.0 gives  meters
	// at the equator. 
	//
	cartesianExtents[0] = _lonExts[0] * 111177.0;
	cartesianExtents[1] = _latExts[0] * 111177.0;
	cartesianExtents[2] = _vertCoordinates[0];

	cartesianExtents[3] = _lonExts[1] * 111177.0;
	cartesianExtents[4] = _latExts[1] * 111177.0;
	cartesianExtents[5] = _vertCoordinates[_vertCoordinates.size()-1];
	return(cartesianExtents);
}



vector <size_t> DCReaderROMS::_GetSpatialDims(
	NetCDFCFCollection *ncdfc, string varname
) const {
	vector <size_t> dims = ncdfc->GetSpatialDims(varname);
	reverse(dims.begin(), dims.end());
	return(dims);
}

vector <string> DCReaderROMS::_GetSpatialDimNames(
	NetCDFCFCollection *ncdfc, string varname
) const {
	vector <string> v = ncdfc->GetSpatialDimNames(varname);
	reverse(v.begin(), v.end());
	return(v);
}

int DCReaderROMS::_InitVerticalCoordinates(
	NetCDFCFCollection *ncdfc, 
	vector <string> &cvars,
	vector <double> &vertCoords
) {

	vertCoords.clear();

	if (cvars.size() == 0) { 	// No vertical dimension
		vertCoords.push_back(0.0);
		vertCoords.push_back(0.0);
		return(0);
	}

	if (cvars.size() > 1) {
		SetErrMsg("Only one vertical coordinate variable supported");
		return(-1);
	}

	int rc = ncdfc->InstallStandardVerticalConverter(
		cvars[0], "ELEVATION", "meters"
	);
	if (rc<0) {
		SetErrMsg("Can't compute ELEVATION variable");
		return(-1);
	}

	vector <size_t> dims = _GetSpatialDims(ncdfc, "ELEVATION");
	float *slice = new float[dims[0]*dims[1]];

	int fd = ncdfc->OpenRead(0,"ELEVATION");
	if (fd<0) {
		SetErrMsg("Can't compute ELEVATION variable");
		return(-1);
	}

	// Read bottom slice and look for minimum
	//
	rc = ncdfc->ReadSlice(slice);
	if (rc<0) {
		SetErrMsg("Can't compute ELEVATION variable");
		return(-1);
	}
	float min = slice[0];
	for (size_t i = 0; i < dims[0]*dims[1]; i++) {
		if (slice[i]<min) min = slice[i];
	}

	// Read top slice (skipping over inbetween slices) and look for maximum
	//
	for (int i=1; i<dims[2]; i++) {
		rc = ncdfc->ReadSlice(slice);
		if (rc<0) {
			SetErrMsg("Can't compute ELEVATION variable");
			return(-1);
		}
	}
	ncdfc->Close(fd);

	float max = slice[0];
	for (size_t i = 0; i < dims[0]*dims[1]; i++) {
		if (slice[i]>max) max = slice[i];
	}
	delete [] slice;
	

	vertCoords.push_back(min);
	vertCoords.push_back(max);

	return(0);
}

void DCReaderROMS::_InitDimensions(
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


DCReaderROMS::~DCReaderROMS() {
	if (_weightTable) delete _weightTable;
	if (_sliceBuffer) delete [] _sliceBuffer;
	if (_angleRADBuf) delete [] _angleRADBuf;
	if (_latDEGBuf) delete [] _latDEGBuf;

	if (_ncdfc) delete _ncdfc;
}

double DCReaderROMS::GetTSUserTime(size_t ts) const {
	if (ts >= _ncdfc->GetNumTimeSteps()) return(0.0); 

	double time_d;
	_ncdfc->GetTime(ts, time_d);


	// Convert time from whatever is used in the file to seconds
	//
	string from;
	float from_time = time_d;
	int rc = _ncdfc->GetVarUnits(_timeCV, from);
	if (rc<0) return(from_time);

	string to = "seconds";
	float to_time;

	rc = _ncdfc->Convert(from, to, &from_time, &to_time, 1);
	if (rc<0) return(from_time);

	return(to_time);
};

void DCReaderROMS::GetTSUserTimeStamp(size_t ts, string &s) const {
	s.clear();
	double time_d = DCReaderROMS::GetTSUserTime(ts);

	_ncdfc->FormatTimeStr(time_d, s);
}

bool DCReaderROMS::GetMissingValue(string varname, float &value) const {
	value = 0.0;

	if (IsVariableDerived(varname)) {
		value = _defaultMV;
		return(true);
	}

	// 
	// coordinate variables can't have missing values
	//
	if (varname.compare("ELEVATION") == 0) return(false);

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
			value = (float) _defaultMV;
			has_missing = true;
		}
	}
	return(has_missing);
}

bool DCReaderROMS::IsCoordinateVariable(string varname) const {
	return(varname.compare("ELEVATION") == 0);
}

int DCReaderROMS::OpenVariableRead(
    size_t timestep, string varname, int, int 
) {

	DCReaderROMS::CloseVariable();

	_ovr_varname.clear();
	_ovr_slice = 0;

	if (IsVariableDerived(varname)) { 
		_ovr_varname = varname;
		return(0);
	}

	_ovr_varname = varname;

	_ovr_fd = _ncdfc->OpenRead(timestep, varname);
	return(_ovr_fd);
}

int DCReaderROMS::ReadSlice(float *slice) {

	//
	// Deal with derived variables
	//
	if (IsVariableDerived(_ovr_varname)) {
		const float *ptr;
		if (_ovr_varname.compare("angleRAD") == 0) {
			if (_ovr_slice > 0) return(0);  // EOF
			ptr = _angleRADBuf;
		}
		else {
			if (_ovr_slice > 0) return(0);  // EOF
			ptr = _latDEGBuf;
		}
		for (int i=0; i<_dims[0]*_dims[1]; i++) {
			slice[i] = ptr[i];
		}
		_ovr_slice++;
		return(1);
	}

	if (_GetSpatialDims(_ncdfc, _ovr_varname).size() < 2) {
		SetErrMsg("Invalid operation");
		return(-1);
	}


	int rc = _ncdfc->ReadSlice(_sliceBuffer, _ovr_fd);
	if (rc<1) return(rc);

	float srcMV, dstMV;
	bool has_missing = DCReaderROMS::GetMissingValue(_ovr_varname, srcMV);

	//
	// If there are no missing values  resampling may introduce them.
	//
	if (! has_missing) srcMV = _defaultMV;

	if (_ovr_varname.compare("ELEVATION") == 0) {
		vector <double> extents = DCReaderROMS::GetExtents(0);
		dstMV = (float) extents[5];
	}
	else {
		dstMV = srcMV;
	}

	size_t dims[] = {_dims[0], _dims[1]};
	_weightTable->interp2D(_sliceBuffer, slice, srcMV, dstMV, dims);

	_ovr_slice++;
	return(1);
	
}

int DCReaderROMS::Read(float *data) {
	float *ptr = data;

	int rc;
	while ((rc = DCReaderROMS::ReadSlice(ptr)) > 0) {
		ptr += _dims[0] * _dims[1];
	}
	return(rc);
}

int DCReaderROMS::CloseVariable() {
	if (_ovr_fd < 0) return(0);

	bool derived = IsVariableDerived(_ovr_varname);
	_ovr_varname.clear();
	_ovr_slice = 0;

	if (derived) return(0);

	int rc = _ncdfc->Close(_ovr_fd);
	_ovr_fd = -1;
	return(rc);
}


float *DCReaderROMS::_get_2d_var(
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

float *DCReaderROMS::_get_1d_var(
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

void DCReaderROMS::_ParseCoordVarNames(
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

int DCReaderROMS::_InitCoordVars(NetCDFCFCollection *ncdfc) 
{
	_vars2dXY.clear();
	_vars3d.clear();
	_vars3dExcluded.clear();
	_vars2dExcluded.clear();
	_varsLatLonMap.clear();
	_timeCV.clear();
	_vertCVs.clear();
	_latCVs.clear();
	_lonCVs.clear();


	//
	// Find the dimensions of the "unstaggered" base grid
	//
	vector <size_t> udims;	// unstaggered dims 
	vector <string> vars = ncdfc->GetDataVariableNames(3, true);
	for (int i=0; i<vars.size(); i++) {

		vector <size_t> dims = _GetSpatialDims(ncdfc, vars[i]);
		assert(dims.size() == 3);
		if (udims.size() == 0) udims = dims;

		//
		// The grid with minimum dimensions is the based grid that 
		// staggered variables are resampled to
		//
		for (int j=0; j<3; j++) {
			if (dims[j] < udims[j]) {
				udims[j] = dims[j];
			}
		}
	}

	vector <string> sdimnames;	// staggered dim names
	vars = ncdfc->GetDataVariableNames(3, true);
	for (int i=0; i<vars.size(); i++) {
		vector <size_t> dims = _GetSpatialDims(ncdfc, vars[i]);
		vector <string> dimnames = _GetSpatialDimNames(ncdfc, vars[i]);
		if (dims[0] == udims[0]+1) sdimnames.push_back(dimnames[0]);
		if (dims[1] == udims[1]+1) sdimnames.push_back(dimnames[1]);
		if (dims[2] == udims[2]+1) sdimnames.push_back(dimnames[2]);
	}

	vars = ncdfc->GetDataVariableNames(2, true);
	for (int i=0; i<vars.size(); i++) {
		vector <size_t> dims = _GetSpatialDims(ncdfc, vars[i]);
		vector <string> dimnames = _GetSpatialDimNames(ncdfc, vars[i]);
		if (dims[0] == udims[0]+1) sdimnames.push_back(dimnames[0]);
		if (dims[1] == udims[1]+1) sdimnames.push_back(dimnames[1]);
	}


	//
    // sort and remove duplicates
    //
    sort(sdimnames.begin(), sdimnames.end());
    vector <string>::iterator lasts;
    lasts = unique(sdimnames.begin(), sdimnames.end());
    sdimnames.erase(lasts, sdimnames.end());

	
	//
	// Inform NetCFCFCollection of staggered dim names
	//
	ncdfc->SetStaggeredDims(sdimnames);


	//
	// Find the coordinate variables for each 2D data variable. 
	// A latitude and longitude coordinate variable must exist
	// for each data variable or we consider the data variable invalid
	// and ignore it
	//
	vars = ncdfc->GetDataVariableNames(2, true);
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
		// Only one time coordinate variable permitted
		// Vertical coordinate variable must be 1D (and so must time);
		//
		if (_timeCV.empty() && !timecv.empty()) _timeCV = timecv;
		if (! _timeCV.empty() && ! timecv.empty() && _timeCV.compare(timecv) != 0) excluded = true;
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
		if (! ncdfc->IsStaggeredVar(vertcv)) {
			if (find(_vertCVs.begin(), _vertCVs.end(), vertcv) == _vertCVs.end()) {
				_vertCVs.push_back(vertcv);
			}
		}

		_vars3d.push_back(vars[i]);

	}

	if ((! _latCVs.size()) || (! _lonCVs.size())) {
		SetErrMsg("Missing lat or lon coordinate (or auxiliary) variable");
		return(-1);
	}
		

	return(0);
}

int DCReaderROMS::_initLatLonBuf(
	NetCDFCFCollection *ncdfc, string latvar, string lonvar, 
	DCReaderROMS::latLonBuf &llb
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
		float tmp = llb._lonbuf[j*llb._nx+i];
		llb._lonexts[0] = tmp < llb._lonexts[0] ? tmp : llb._lonexts[0];
		llb._lonexts[1] = tmp > llb._lonexts[1] ? tmp : llb._lonexts[1];
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
	return(0);
}

void DCReaderROMS::_getRotationVariables(
    WeightTable *wt,
	float *_angleRADBuf, float *_latDEGBuf
) const {

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
