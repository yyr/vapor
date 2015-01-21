#include <vector>
#include <algorithm>
#include <map>
#include <iostream>
#include <cassert>

#ifdef _WINDOWS
#define _USE_MATH_DEFINES
#pragma warning(disable : 4251 4100)
#endif
#include <cmath>

#include <vapor/GeoUtil.h>
#include <vapor/UDUnitsClass.h>
#include <vapor/DCWRF.h>

using namespace VAPoR;
using namespace std;

DCWRF::DCWRF() {
	_ncdfc = NULL;

	_dx = -1.0;
	_dy = -1.0;
	_cen_lat = 0.0;
	_cen_lon = 0.0;
	_pole_lat = 90.0;
	_pole_lon = 0.0;
	_grav = 9.81;
	_radius = 0.0;
	_p2si = 1.0;
	_mapProj = 0;

	_ovr_fd = -1;
	_projString.clear();

	_sliceBuffer = NULL;

	_derivedX = NULL;
	_derivedY = NULL;
	_derivedXU = NULL;
	_derivedYU = NULL;
	_derivedXV = NULL;
	_derivedYV = NULL;

	_derivedElev = NULL;
	_derivedElevU = NULL;
	_derivedElevV = NULL;
	_derivedElevW = NULL;

	_derivedTime = NULL;

	_dimsMap.clear();
	_coordVarsMap.clear();
	_dataVarsMap.clear();


}

DCWRF::~DCWRF() {
	if (_derivedX) delete _derivedX;
	if (_derivedY) delete _derivedY;
	if (_derivedXU) delete _derivedXU;
	if (_derivedYU) delete _derivedYU;
	if (_derivedXV) delete _derivedXV;
	if (_derivedYV) delete _derivedYV;

	if (_derivedElev) delete _derivedElev;
	if (_derivedElevU) delete _derivedElevU;
	if (_derivedElevV) delete _derivedElevV;
	if (_derivedElevW) delete _derivedElevW;

	if (_derivedTime) delete _derivedTime;

	if (_sliceBuffer) delete [] _sliceBuffer;
	if (_ncdfc) delete _ncdfc;
}


int DCWRF::Initialize(const vector <string> &files) {


	vector <string> time_dimnames;
	vector <string> time_coordvars;
	time_dimnames.push_back("Time");

	NetCDFCollection *ncdfc = new NetCDFCollection();

	// Workaround for bug #953:  reverses time in WRF vdc conversion
	// NetCDFCollection expects a 1D time coordinate variable. WRF
	// data use a 2D "Times" time coordinate variable. By convention,
	// however, WRF output files are sorted by time. Just need to
	// make sure they're passed in the proper order. However, this will
	// fail if files aren't named according to convention. The better 
	// fix would be to modify NetCDFCollection to handle the WRF
	// time coordinate variable.
	//
	vector <string> sorted_files = files;
	std::sort(sorted_files.begin(), sorted_files.end());

	int rc = ncdfc->Initialize(sorted_files, time_dimnames, time_coordvars);
	if (rc<0) {
		SetErrMsg("Failed to initialize netCDF data collection for reading");
		return(-1);
	}



	rc = _udunits.Initialize();
	if (rc<0) {
		SetErrMsg(
			"Failed to initialize udunits2 library : %s",
			_udunits.GetErrMsg().c_str()
		);
		return(-1);
	}

	// Get required and optional global attributes 
	// Initializes members: _dx, _dy, _cen_lat, _cen_lon, _pole_lat,
	// _pole_lon, _grav, _radius, _p2si
	//
	rc = _InitAtts(ncdfc);
	if (rc < 0) return(-1);

	//
	//  Get the dimensions of the grid. 
	//	Initializes members: _dimsMap, _sliceBuffer
	//
	rc = _InitDimensions(ncdfc);
	if (rc< 0) {
		SetErrMsg("No valid dimensions");
		return(-1);
	}

	// Set up map projection transforms
	// Initializes members: _projString, _proj4API, _mapProj
	//
	rc = _InitProjection(ncdfc, _radius);
	if (rc< 0) {
		return(-1);
	}

	//  Set up the X, Y, XV, YV, XU, and YU coordinate variables
	//	Initializes members: _coordVarMap
	//
	rc = _InitHorizontalCoordinates(ncdfc, &_proj4API);
	if (rc<0) {
		return(-1);
	}

	//  Set up the ELEVATION coordinate variable
	//	Initializes members: _coordVarMap
	//
	rc = _InitVerticalCoordinates(ncdfc);
	if (rc<0) {
		return(-1);
	}

	// Set up user time and time stamps
	//	Initializes members: _coordVarsMap
	//
	rc = _InitTime(ncdfc);
	if (rc<0) {
		return(-1);
	}

	//
	// Identify data and coordinate variables. Sets up members:
	// Initializes members: _dataVarsMap, _coordVarsMap
	// _vars2dExcluded
	//
	rc = _InitVars(ncdfc) ;
	if (rc<0) return(-1);

	_ncdfc = ncdfc;

	return(0);
}


bool DCWRF::GetDimension(
	string dimname, DC::Dimension &dimension
) const {
	map <string, DC::Dimension>::const_iterator itr;

	itr = _dimsMap.find(dimname);
	if (itr == _dimsMap.end()) return(false);

	dimension = itr->second;
	return(true); 
}

std::vector <string> DCWRF::GetDimensionNames() const {
	map <string, DC::Dimension>::const_iterator itr;

	vector <string> names;

	for (itr=_dimsMap.begin(); itr != _dimsMap.end(); ++itr) {
		names.push_back(itr->first);
	}

	return(names);
}

bool DCWRF::GetCoordVarInfo(string varname, DC::CoordVar &cvar) const {

	map <string, DC::CoordVar>::const_iterator itr;

	itr = _coordVarsMap.find(varname);
	if (itr == _coordVarsMap.end()) {
		return(false);
	}

	cvar = itr->second;
	return(true);
}

bool DCWRF::GetDataVarInfo( string varname, DC::DataVar &datavar) const {

	map <string, DC::DataVar>::const_iterator itr;

	itr = _dataVarsMap.find(varname);
	if (itr == _dataVarsMap.end()) {
		return(false);
	}

	datavar = itr->second;
	return(true);
}

bool DCWRF::GetBaseVarInfo(string varname, DC::BaseVar &var) const {
	map <string, DC::CoordVar>::const_iterator itr;

	itr = _coordVarsMap.find(varname);
	if (itr != _coordVarsMap.end()) {
		var = itr->second;
		return(true);
	}

	map <string, DC::DataVar>::const_iterator itr1 = _dataVarsMap.find(varname);
	if (itr1 != _dataVarsMap.end()) {
		var = itr1->second;
		return(true);
	}

	return(false);
}


std::vector <string> DCWRF::GetDataVarNames() const {
	map <string, DC::DataVar>::const_iterator itr;

	vector <string> names;
	for (itr = _dataVarsMap.begin(); itr != _dataVarsMap.end(); ++itr) {
		names.push_back(itr->first);
	}
	return(names);
}


std::vector <string> DCWRF::GetCoordVarNames() const {
	map <string, DC::CoordVar>::const_iterator itr;

	vector <string> names;
	for (itr = _coordVarsMap.begin(); itr != _coordVarsMap.end(); ++itr) {
		names.push_back(itr->first);
	}
	return(names);
}

int DCWRF::GetAtt(
	string varname, string attname, vector <double> &values
) const {
	values.clear();
	return(0);
}

int DCWRF::GetAtt(
	string varname, string attname, vector <long> &values
) const {
	values.clear();
	return(0);
}

int DCWRF::GetAtt(
	string varname, string attname, string &values
) const {
	values.clear();
	return(0);
}

std::vector <string> DCWRF::GetAttNames(string varname) const {
	vector <string> names;
	return(names);
}

DC::XType DCWRF::GetAttType(string varname, string attname) const {
	return(DC::FLOAT);
}

int DCWRF::GetDimLensAtLevel(
	string varname, int, std::vector <size_t> &dims_at_level,
	std::vector <size_t> &bs_at_level
) const {
	dims_at_level.clear();
	bs_at_level.clear();

	DC::BaseVar var;

	bool ok = GetBaseVarInfo(varname, var);
	if (! ok) {
		SetErrMsg("Invalid variable name : %s", varname.c_str());
		return(-1);
	}
	vector <DC::Dimension> dims = var.GetDimensions();
	for (int i=0; i<dims.size(); i++) {
		dims_at_level.push_back(dims[i].GetLength());
		bs_at_level.push_back(dims[i].GetLength());
	}
	return(0);
}

int DCWRF::GetMapProjection(string , string , string &projstring) const {
	projstring = _projString;
	return(0);
}


int DCWRF::OpenVariableRead(
	size_t ts, string varname
) {
	DCWRF::CloseVariable();

	_ovr_fd = _ncdfc->OpenRead(ts, varname);
	return(_ovr_fd);

}


int DCWRF::CloseVariable() {
	if (_ovr_fd < 0) return (0);
	int rc = _ncdfc->Close(_ovr_fd);
	_ovr_fd = -1;
	return(rc);
}

int DCWRF::Read(float *data) {
	return(_ncdfc->Read(data, _ovr_fd));
}

int DCWRF::ReadSlice(float *slice) {

	return(_ncdfc->ReadSlice(slice, _ovr_fd));
}

int DCWRF::ReadRegion(
	const vector <size_t> &min, const vector <size_t> &max, float *region
) {
	vector <size_t> ncdf_start = min;
	reverse(ncdf_start.begin(), ncdf_start.end());

	vector <size_t> ncdf_max = max;
	reverse(ncdf_max.begin(), ncdf_max.end());

	vector <size_t> ncdf_count;
	for (int i=0; i<ncdf_start.size(); i++) {
		ncdf_count.push_back(ncdf_max[i] - ncdf_start[i] + 1);
	}

	return(_ncdfc->Read(ncdf_start, ncdf_count, region, _ovr_fd));
}

int DCWRF::ReadRegionBlock(
	const vector <size_t> &min, const vector <size_t> &max, float *region
) {
	return(DCWRF::ReadRegion(min, max, region));
}

int DCWRF::GetVar(string varname, float *data) {

	DC::BaseVar var;

	bool ok = DCWRF::GetBaseVarInfo(varname, var);
	if (!ok) {
		SetErrMsg("Invalid variable name : %s", varname.c_str());
		return(-1);
	}
	vector <DC::Dimension> dims = var.GetDimensions();

	int nts = 1;
	if (var.IsTimeVarying()) {
		nts = dims[dims.size()-1].GetLength();
		dims.pop_back();
	}

	size_t sz = 1;
	for (int i=0; i<dims.size(); i++) {
		sz *= dims[i].GetLength();
	}
		
	float *ptr = data;
	for (int ts=0; ts<nts; ts++) {
		int rc = DCWRF::OpenVariableRead(ts, varname);
		if (rc<0) return(-1);

		rc = DCWRF::Read(ptr);
		if (rc<0) return(-1);

		rc = DCWRF::CloseVariable();
		if (rc<0) return(-1);
		
		ptr += sz;
	}
	return(0);
}

int DCWRF::GetVar(
	size_t ts, string varname, float *data
) {
	int rc = DCWRF::OpenVariableRead(ts, varname);
	if (rc<0) return(-1);

	rc = DCWRF::Read(data);
	if (rc<0) return(-1);

	rc = DCWRF::CloseVariable();
	if (rc<0) return(-1);

	return(0);
}

bool DCWRF::VariableExists(
	size_t ts, string varname, int, int 
) const {
	return(_ncdfc->VariableExists(ts, varname));
}


vector <size_t> DCWRF::_GetSpatialDims(
	NetCDFCollection *ncdfc, string varname
) const {
	vector <size_t> dims = ncdfc->GetSpatialDims(varname);
	reverse(dims.begin(), dims.end());
	return(dims);
}

vector <string> DCWRF::_GetSpatialDimNames(
	NetCDFCollection *ncdfc, string varname
) const {
	vector <string> v = ncdfc->GetSpatialDimNames(varname);
	reverse(v.begin(), v.end());
	return(v);
}

int DCWRF::_InitAtts(
	NetCDFCollection *ncdfc
) {

	_dx = -1.0;
	_dy = -1.0;
	_cen_lat = 0.0;
	_cen_lon = 0.0;
	_pole_lat = 90.0;
	_pole_lon = 0.0;
	_grav = 9.81;
	_radius = 0.0;
	_p2si = 1.0;
	

	vector <double> dvalues;
	ncdfc->GetAtt("", "DX", dvalues);
	if (dvalues.size() != 1) {
		SetErrMsg("Error reading required attribute : DX");
		return(-1);
	}
	_dx = dvalues[0];

	ncdfc->GetAtt("", "DY", dvalues);
	if (dvalues.size() != 1) {
		SetErrMsg("Error reading required attribute : DY");
		return(-1);
	}
	_dy = dvalues[0];

	ncdfc->GetAtt("", "CEN_LAT", dvalues);
	if (dvalues.size() != 1) {
		SetErrMsg("Error reading required attribute : CEN_LAT");
		return(-1);
	}
	_cen_lat = dvalues[0];

	ncdfc->GetAtt("", "CEN_LON", dvalues);
	if (dvalues.size() != 1) {
		SetErrMsg("Error reading required attribute : CEN_LON");
		return(-1);
	}
	_cen_lon = dvalues[0];

	ncdfc->GetAtt("", "POLE_LAT", dvalues);
	if (dvalues.size() != 1) _pole_lat = 90.0;
	else _pole_lat = dvalues[0];

	ncdfc->GetAtt("", "POLE_LON", dvalues);
	if (dvalues.size() != 1) _pole_lon = 0.0;
	else _pole_lon = dvalues[0];

	//
	// "PlanetWRF" attributes
	//
	ncdfc->GetAtt("", "G", dvalues);
	if (dvalues.size() == 1) {

		_grav = dvalues[0];

		ncdfc->GetAtt("", "RADIUS", dvalues);
		if (dvalues.size() != 1) {
			SetErrMsg("Error reading required attribute : RADIUS");
			return(-1);
		}
		_radius = dvalues[0];

		ncdfc->GetAtt("", "P2SI", dvalues);
		if (dvalues.size() != 1) {
			SetErrMsg("Error reading required attribute : P2SI");
			return(-1);
		}
		_p2si = dvalues[0];
	}

	return(0);
}

int DCWRF::_GetProj4String(
	NetCDFCollection *ncdfc, float radius, int map_proj, string &projstring
) {
	projstring.clear();
	ostringstream oss;

	vector <double> dvalues;
	switch (map_proj) {
	case 0 : {//Lat Lon

		double lon_0 = _cen_lon;
		double lat_0 = _cen_lat;
		oss << " +lon_0=" << lon_0 << " +lat_0=" << lat_0;
		projstring = "+proj=eqc +ellps=WGS84" + oss.str();

	}
	break;
	case 1: { //Lambert
		ncdfc->GetAtt("", "STAND_LON", dvalues);
		if (dvalues.size() != 1) {
			SetErrMsg("Error reading required attribute : STAND_LON");
			return(-1);
		}
		float lon0 = dvalues[0];

		ncdfc->GetAtt("", "TRUELAT1", dvalues);
		if (dvalues.size() != 1) {
			SetErrMsg("Error reading required attribute : TRUELAT1");
			return(-1);
		}
		float lat1 = dvalues[0];

		ncdfc->GetAtt("", "TRUELAT2", dvalues);
		if (dvalues.size() != 1) {
			SetErrMsg("Error reading required attribute : TRUELAT2");
			return(-1);
		}
		float lat2 = dvalues[0];
		
		//Construct the projection string:
		projstring = "+proj=lcc";
		projstring += " +lon_0=";
		oss.str("");
		oss << (double)lon0;
		projstring += oss.str();

		projstring += " +lat_1=";
		oss.str("");
		oss << (double)lat1;
		projstring += oss.str();

		projstring += " +lat_2=";
		oss.str("");
		oss << (double)lat2;
		projstring += oss.str();

	break;
	}

	case 2: { //Polar stereographic (pure north or south)
		projstring = "+proj=stere";

		//Determine whether north or south pole (lat_ts is pos or neg)
		
		ncdfc->GetAtt("", "TRUELAT1", dvalues);
		if (dvalues.size() != 1) {
			SetErrMsg("Error reading required attribute : TRUELAT1");
			return(-1);
		}
		float latts = dvalues[0];
	
		float lat0;
		if (latts < 0.) lat0 = -90.0;
		else lat0 = 90.0;

		projstring += " +lat_0=";
		oss.str("");
		oss << (double)lat0;
		projstring += oss.str();

		projstring += " +lat_ts=";
		oss.str("");
		oss << (double)latts;
		projstring += oss.str();

		ncdfc->GetAtt("", "STAND_LON", dvalues);
		if (dvalues.size() != 1) {
			SetErrMsg("Error reading required attribute : STAND_LON");
			return(-1);
		}
		float lon0 = dvalues[0];
		
		projstring += " +lon_0=";
		oss.str("");
		oss << (double)lon0;
		projstring += oss.str();

	break;
	}

	case(3): { //Mercator
		
		ncdfc->GetAtt("", "TRUELAT1", dvalues);
		if (dvalues.size() != 1) {
			SetErrMsg("Error reading required attribute : TRUELAT1");
			return(-1);
		}
		float latts = dvalues[0];

		ncdfc->GetAtt("", "STAND_LON", dvalues);
		if (dvalues.size() != 1) {
			SetErrMsg("Error reading required attribute : STAND_LON");
			return(-1);
		}
		float lon0 = dvalues[0];

		//Construct the projection string:
		projstring = "+proj=merc";

		projstring += " +lon_0=";
		oss.str("");
		oss << (double)lon0;
		projstring += oss.str();
		
		projstring += " +lat_ts=";
		oss.str("");
		oss << (double)latts;
		projstring += oss.str();

	break;
	}

	case(6): { // lat-long, possibly rotated, possibly cassini
		
		// See if this is a regular cylindrical equidistant projection
		// with the pole in the default location
		//
		if (_pole_lat == 90.0 && _pole_lon == 0.0) {

			double lon_0 = _cen_lon;
			double lat_0 = _cen_lat;
			ostringstream oss;
			oss << " +lon_0=" << lon_0 << " +lat_0=" << lat_0;
			projstring = "+proj=eqc +ellps=WGS84" + oss.str();
		}
		else {

			//
			// Assume arbitrary pole displacement. Probably should 
			// check for cassini projection (transverse cylindrical)
			// but general rotated cyl. equidist. projection should work
			//
			ncdfc->GetAtt("", "STAND_LON", dvalues);
			if (dvalues.size() != 1) {
				SetErrMsg("Error reading required attribute : STAND_LON");
				return(-1);
			}
			float lon0 = dvalues[0];

			projstring = "+proj=ob_tran";
			projstring += " +o_proj=eqc";
			projstring += " +to_meter=0.0174532925199";

			projstring += " +o_lat_p=";
			oss.str("");
			oss << (double) _pole_lat;
			projstring += oss.str();
			projstring += "d"; //degrees, not radians

			projstring += " +o_lon_p=";
			oss.str("");
			oss << (double)(180.-_pole_lon);
			projstring += oss.str();
			projstring += "d"; //degrees, not radians

			projstring += " +lon_0=";
			oss.str("");
			oss << (double)(-lon0);
			projstring += oss.str();
			projstring += "d"; //degrees, not radians
		}
		
		break;
	}
	default: {

		SetErrMsg("Unsupported MAP_PROJ value : %d", _mapProj);
		return -1;
	}
	}

	if (projstring.empty()) return(0);

	//
	// PlanetWRF data if radius is not zero
	//
	if (radius > 0) {	// planetWRf (not on earth)
		projstring += " +ellps=sphere";
		stringstream ss;
		ss << radius;
		projstring += " +a=" + ss.str() + " +es=0";
	}
	else {
		projstring += " +ellps=WGS84";
	}

	return(0);
}

int DCWRF::_InitProjection(
	NetCDFCollection *ncdfc, float radius
) {
	_projString.clear();
	_mapProj = 0;

	vector <long> ivalues;
	ncdfc->GetAtt("", "MAP_PROJ", ivalues);
	if (ivalues.size() != 1) {
		SetErrMsg("Error reading required attribute : MAP_PROJ");
		return(-1);
	}
	_mapProj = ivalues[0];

	int rc = _GetProj4String(ncdfc, radius, _mapProj, _projString);
	if (rc<0) return(rc);

	//
	// Set up projection transforms to/from geographic and cartographic
	// coordinates
	//
	rc = _proj4API.Initialize("", _projString);
	if (rc < 0) {
		SetErrMsg("Proj4API::Initalize(, %s)", _projString.c_str());
		return(-1);
	}

	return(0);
}

int DCWRF::_InitHorizontalCoordinates(
	NetCDFCollection *ncdfc, Proj4API *proj4API
) {
	_coordVarsMap.clear();
	_derivedX = NULL;
	_derivedY = NULL;
	_derivedXU = NULL;
	_derivedYU = NULL;
	_derivedXV = NULL;
	_derivedYV = NULL;

	vector <bool> periodic(2, false);

	// XLONG and XLAT must have same dimensionality
	//
	vector <size_t> latlondims = ncdfc->GetDims("XLONG");
	vector <size_t> dummy = ncdfc->GetDims("XLAT");
	if (latlondims.size() != 3 || dummy != latlondims) {
		SetErrMsg("Invalid coordinate variable : %s", "XLONG");
		return(-1);
	}

	// "X" coordinate, unstaggered
	//

	vector <DC::Dimension> dims;
	dims.push_back(_dimsMap["west_east"]);
	dims.push_back(_dimsMap["Time"]);
	string name = "X";

	_derivedX = new DerivedVarHorizontal(
		ncdfc, name, dims, latlondims, proj4API
	);

	ncdfc->InstallDerivedVar(name, _derivedX);

	_coordVarsMap[name] = CoordVar(
		name, dims, "meters", DC::FLOAT, periodic, 0, true
	);

	// "Y" coordinate, unstaggered
	//
	dims.clear();
	dims.push_back(_dimsMap["south_north"]);
	dims.push_back(_dimsMap["Time"]);
	name = "Y";

	_derivedY = new DerivedVarHorizontal(
		ncdfc, name, dims, latlondims, proj4API
	);
	ncdfc->InstallDerivedVar(name, _derivedY);

	_coordVarsMap[name] = CoordVar(
		name, dims, "meters", DC::FLOAT, periodic, 1, true
	);

	// "XU" coordinate, staggered
	//
	dims.clear();
	dims.push_back(_dimsMap["west_east_stag"]);
	dims.push_back(_dimsMap["Time"]);
	name = "XU";

	_derivedXU = new DerivedVarHorizontal(
		ncdfc, name, dims, latlondims, proj4API
	);
	ncdfc->InstallDerivedVar(name, _derivedXU);

	_coordVarsMap[name] = CoordVar(
		name, dims, "meters", DC::FLOAT, periodic, 0, true
	);

	// "YU" coordinate, unstaggered
	//

	dims.clear();
	dims.push_back(_dimsMap["south_north"]);
	dims.push_back(_dimsMap["Time"]);
	name = "YU";

	_derivedYU = new DerivedVarHorizontal(
		ncdfc, name, dims, latlondims, proj4API
	);
	ncdfc->InstallDerivedVar(name, _derivedYU);

	_coordVarsMap[name] = CoordVar(
		name, dims, "meters", DC::FLOAT, periodic, 1, true
	);

	// "XV" coordinate, unstaggered
	//
	dims.clear();
	dims.push_back(_dimsMap["west_east"]);
	dims.push_back(_dimsMap["Time"]);
	name = "XV";

	_derivedXV = new DerivedVarHorizontal(
		ncdfc, name, dims, latlondims, proj4API
	);
	ncdfc->InstallDerivedVar(name, _derivedXV);

	_coordVarsMap[name] = CoordVar(
		name, dims, "meters", DC::FLOAT, periodic, 0, true
	);

	// "YV" coordinate, staggered
	//
	dims.clear();
	dims.push_back(_dimsMap["south_north_stag"]);
	dims.push_back(_dimsMap["Time"]);
	name = "YV";

	_derivedYV = new DerivedVarHorizontal(
		ncdfc, name, dims, latlondims, proj4API
	);
	ncdfc->InstallDerivedVar(name, _derivedYV);

	_coordVarsMap[name] = CoordVar(
		name, dims, "meters", DC::FLOAT, periodic, 1, true
	);

	return(0);
}

int DCWRF::_InitVerticalCoordinates(
	NetCDFCollection *ncdfc
) {
	vector <bool> periodic(1, false);

	_derivedElev = NULL;
	_derivedElevU = NULL;
	_derivedElevV = NULL;
	_derivedElevW = NULL;

	vector <DC::Dimension> dims;
	dims.push_back(_dimsMap["west_east"]);
	dims.push_back(_dimsMap["south_north"]);
	dims.push_back(_dimsMap["bottom_top"]);
	dims.push_back(_dimsMap["Time"]);
	string name = "ELEVATION";

	_derivedElev = new DerivedVarElevation(ncdfc, name, dims, _grav);

	ncdfc->InstallDerivedVar(name, _derivedElev);

	_coordVarsMap[name] = CoordVar(
		name, dims, "meters", DC::FLOAT, periodic, 2, true
	);

	dims.clear();
	dims.push_back(_dimsMap["west_east_stag"]);
	dims.push_back(_dimsMap["south_north"]);
	dims.push_back(_dimsMap["bottom_top"]);
	dims.push_back(_dimsMap["Time"]);
	name = "ELEVATIONU";

	_derivedElevU = new DerivedVarElevation(ncdfc, name, dims, _grav);

	ncdfc->InstallDerivedVar(name, _derivedElevU);

	_coordVarsMap[name] = CoordVar(
		name, dims, "meters", DC::FLOAT, periodic, 2, true
	);

	dims.clear();
	dims.push_back(_dimsMap["west_east"]);
	dims.push_back(_dimsMap["south_north_stag"]);
	dims.push_back(_dimsMap["bottom_top"]);
	dims.push_back(_dimsMap["Time"]);
	name = "ELEVATIONV";

	_derivedElevV = new DerivedVarElevation(ncdfc, name, dims, _grav);

	ncdfc->InstallDerivedVar(name, _derivedElevV);

	_coordVarsMap[name] = CoordVar(
		name, dims, "meters", DC::FLOAT, periodic, 2, true
	);

	dims.clear();
	dims.push_back(_dimsMap["west_east"]);
	dims.push_back(_dimsMap["south_north"]);
	dims.push_back(_dimsMap["bottom_top_stag"]);
	dims.push_back(_dimsMap["Time"]);
	name = "ELEVATIONW";

	_derivedElevW = new DerivedVarElevation(ncdfc, name, dims, _grav);

	ncdfc->InstallDerivedVar(name, _derivedElevW);

	_coordVarsMap[name] = CoordVar(
		name, dims, "meters", DC::FLOAT, periodic, 2, true
	);

	return(0);
}

int DCWRF::_InitTime(
	NetCDFCollection *ncdfc
) {
	_derivedElev = NULL;

	vector <size_t> sdims = _GetSpatialDims(ncdfc, "Times");
	if (sdims.size() != 1) {
		SetErrMsg("Invalid Times variable\n");
		return(-1);
	}

	char *buf = new char[sdims[0]+1];
	buf[sdims[0]] = '\0';	// Null terminate

	const char *format = "%4d-%2d-%2d_%2d:%2d:%2d";

	vector <float> times;
	for (size_t ts = 0; ts < ncdfc->GetTimeDim("Times"); ts++) {

		int fd = ncdfc->OpenRead(ts, "Times");
		if (fd<0) {
			SetErrMsg("Can't Read \"Times\" variable");
			return(-1);
		}

		int rc = ncdfc->Read(buf, fd);
		if (rc<0) {
			SetErrMsg("Can't Read \"Times\" variable");
			ncdfc->Close(fd);
			return(-1);
		}
		ncdfc->Close(fd);

		int year, mon, mday, hour, min, sec;	
		rc = sscanf(buf, format, &year, &mon, &mday, &hour, &min, &sec);
		if (rc != 6) {
			rc = sscanf(
				buf, "%4d-%5d_%2d:%2d:%2d", &year,  &mday, &hour, &min, &sec
			);
			mon = 1;
			if (rc != 5) {
				SetErrMsg("Unrecognized time stamp: %s", buf);
				ncdfc->Close(fd);
				return(-1);
			}
		}

		times.push_back(
			_udunits.EncodeTime(year, mon, mday, hour, min, sec) * _p2si
		);

	}
	delete [] buf;

	vector <DC::Dimension> dims;
	dims.push_back(_dimsMap["Time"]);

	string name = "Time";
	_derivedTime = new DerivedVarTime(ncdfc, dims[0], times);
	ncdfc->InstallDerivedVar(name, _derivedTime);

	vector <bool> periodic(1,false);
	_coordVarsMap[name] = CoordVar(
		name, dims, "seconds", DC::FLOAT, periodic, 3, false
	);

	return(0);
}

int DCWRF::_InitDimensions(
	NetCDFCollection *ncdfc
) {
	_dimsMap.clear();
	_sliceBuffer = NULL;

	vector <string> dimnames = ncdfc->GetDimNames();
	vector <size_t> dimlens = ncdfc->GetDims();
	assert(dimnames.size() == dimlens.size());

	size_t maxnx = 0;
	size_t maxny = 0;

	int axis;
	string timedimname = "Time";
	for (int i=0; i<dimnames.size(); i++) {
		if ((dimnames[i].compare("west_east") == 0) || 
			(dimnames[i].compare("west_east_stag") == 0)) { 

			axis = 0;
			maxnx = max(maxnx,dimlens[i]);
		}
		else if ((dimnames[i].compare("south_north") == 0) ||
			(dimnames[i].compare("south_north_stag") == 0)) {

			axis = 1;
			maxny = max(maxny,dimlens[i]);
		} 
		else if ((dimnames[i].compare("bottom_top") == 0) ||
			(dimnames[i].compare("bottom_top_stag") == 0)) {

			axis = 2;
		}
		else if ((dimnames[i].compare(timedimname) == 0)) { 

			axis = 3;
		}
		else {
			continue;
		} 
		Dimension dim(dimnames[i], dimlens[i], axis);
		_dimsMap[dimnames[i]] = dim;
	}

	if (
		(_dimsMap.find("west_east") == _dimsMap.end()) ||
		(_dimsMap.find("west_east_stag") == _dimsMap.end()) ||
		(_dimsMap.find("south_north") == _dimsMap.end()) ||
		(_dimsMap.find("south_north_stag") == _dimsMap.end()) ||
		(_dimsMap.find("bottom_top") == _dimsMap.end()) ||
		(_dimsMap.find("bottom_top_stag") == _dimsMap.end()) ||
		(_dimsMap.find("Time") == _dimsMap.end())) {

		SetErrMsg("Missing dimension");
		return(-1);
	}

	_sliceBuffer = new float[maxnx * maxny];
	if (! _sliceBuffer) return(-1);
	
	return(0);
}


bool DCWRF::_GetVarCoordinates(
	NetCDFCollection *ncdfc, string varname,
	vector <DC::Dimension> &dimensions,
	vector <string> &coordvars
) {
	dimensions.clear();
	coordvars.clear();

	vector <size_t> dims = ncdfc->GetDims(varname);
	reverse(dims.begin(), dims.end());	// DC dimension order

	vector <string> dimnames = ncdfc->GetDimNames(varname);
	reverse(dimnames.begin(), dimnames.end());
	assert(dims.size() == dimnames.size());

	if (dimnames.size() == 1) {
		if (dimnames[0].compare("Times")!=0) {
			return(false);
		}
		dimensions.push_back(DC::Dimension("Time", dims.back(), 3));
		coordvars.push_back("Time");
		return(true);
	} 

	// only handle 2d, 3d, and 4d variables
	//
	if (dims.size() < 2) return(false);


	if (
		dimnames[0].compare("west_east")==0 && 
		dimnames[1].compare("south_north")==0
	) {
		dimensions.push_back(DC::Dimension("west_east", dims[0], 0));
		dimensions.push_back(DC::Dimension("south_north", dims[1], 1));
		coordvars.push_back("X");
		coordvars.push_back("Y");
		if ((dimnames.size() > 2) && dimnames[2].compare("bottom_top")==0) {
			dimensions.push_back(DC::Dimension("bottom_top", dims[2], 2));
			coordvars.push_back("ELEVATION");
		}
		else if ((dimnames.size() > 2) && dimnames[2].compare("bottom_top_stag")==0) {
			dimensions.push_back(DC::Dimension("bottom_top_stag", dims[2], 2));
			coordvars.push_back("ELEVATIONW");
		}
	}
	else if (
		dimnames[0].compare("west_east_stag")==0 && 
		dimnames[1].compare("south_north")==0
	) {
		dimensions.push_back(DC::Dimension("west_east_stag", dims[0], 0));
		dimensions.push_back(DC::Dimension("south_north", dims[1], 1));
		coordvars.push_back("XU");
		coordvars.push_back("YU");
		if ((dimnames.size() > 2) && dimnames[2].compare("bottom_top")==0) {
			dimensions.push_back(DC::Dimension("bottom_top", dims[2], 2));
			coordvars.push_back("ELEVATIONU");
		}
	}
	else if (
		dimnames[0].compare("west_east")==0 && 
		dimnames[1].compare("south_north_stag")==0
	) {
		dimensions.push_back(DC::Dimension("west_east", dims[0], 0));
		dimensions.push_back(DC::Dimension("south_north_stag", dims[1], 1));
		coordvars.push_back("XV");
		coordvars.push_back("YV");
		if ((dimnames.size() > 2) && dimnames[2].compare("bottom_top")==0) {
			dimensions.push_back(DC::Dimension("bottom_top", dims[2], 2));
			coordvars.push_back("ELEVATIONV");
		}
	}
	else {
		return(false);
	}

	if (dims.size()==2) return(true);

	if (dimnames.back().compare("Time")==0) {
		dimensions.push_back(DC::Dimension("Time", dims.back(), 3));
		coordvars.push_back("Time");
	}
	return(true);

}

int DCWRF::_InitVars(NetCDFCollection *ncdfc) 
{
	_dataVarsMap.clear();

	vector <bool> periodic(3, false);
	//
	// Get data variables 
	//
	vector <string> vars;
	for (int i=1; i<4; i++) {
		vector <string> v = ncdfc->GetVariableNames(i,true);
		vars.insert(vars.end(), v.begin(), v.end());
	}

	for (int i=0; i<vars.size(); i++) {

		// variable type must be float or int
		//
		int type = ncdfc->GetXType(vars[i]);
		if ( ! (
			NetCDFSimple::IsNCTypeFloat(type) || 
			NetCDFSimple::IsNCTypeFloat(type))) continue; 

		// If variables are in _coordVarsMap then they are coordinate, not
		// data, variables
		//
		if (_coordVarsMap.find(vars[i]) != _coordVarsMap.end()) continue; 
		
		vector <DC::Dimension> dimensions;
		vector <string> coordvars;

		bool ok = _GetVarCoordinates(ncdfc, vars[i], dimensions, coordvars);
		if (! ok) continue;
		//if (! ok) {
		//	SetErrMsg("Invalid variable : %s", vars[i].c_str());
		//	return(-1);
		//}

		string units;
		ncdfc->GetAtt(vars[i], "units", units);
		if (! _udunits.ValidUnit(units)) {
			units = "";
		} 

		_dataVarsMap[vars[i]] = DataVar(
			vars[i], dimensions, units, DC::FLOAT, periodic, coordvars
		);
	}

	return(0);
}


DCWRF::DerivedVarElevation::DerivedVarElevation(
	NetCDFCollection *ncdfc, string name, 
	const vector <DC::Dimension> &dims, float grav
) : DerivedVar(ncdfc) {

	assert((name.compare("ELEVATION")==0) || (name.compare("ELEVATIONU")==0) || (name.compare("ELEVATIONV")==0) || (name.compare("ELEVATIONW")==0));

	assert(dims.size());

	_name.clear();
	_time_dim = 1;
	_time_dim_name.clear();
	_sdims.clear();
	_sdimnames.clear();
	
	_grav = grav;
	_PHvar = "PH";
	_PHBvar = "PHB";
	_PH = NULL;
	_PHB = NULL;
	_zsliceBuf = NULL;
	_xysliceBuf = NULL;
	_PHfd = -1;
	_PHBfd = -1;
	_is_open = false;
	_xextrapolate = false;
	_yextrapolate = false;
	_zinterpolate = false;
	_ph_dims.clear();
	_firstSlice = true;

	// NetCDF dimension order
	//
	vector <DC::Dimension> dimsncdf = dims;
	reverse(dimsncdf.begin(), dimsncdf.end());

	_name = name;
	_time_dim = dimsncdf[0].GetLength();
	_time_dim_name = dimsncdf[0].GetName();

	_sdims.clear();
	for (int i=0; i<3; i++) _sdims.push_back(dimsncdf[i+1].GetLength());

	_sdimnames.clear();
	for (int i=0; i<3; i++) _sdimnames.push_back(dimsncdf[i+1].GetName());


	_ph_dims = _sdims;
	if (_name.compare("ELEVATION")==0) {
		_zinterpolate = true;
		_ph_dims[0]++;	// Z dimension
	}
	if (_name.compare("ELEVATIONU")==0) {
		_xextrapolate = true;
		_zinterpolate = true;
		_ph_dims[2]--;	// X dimension
		_ph_dims[0]++;	// Z dimension
	}
	else if (_name.compare("ELEVATIONV")==0) {
		_yextrapolate = true;
		_zinterpolate = true;
		_ph_dims[1]--;	// Y dimension
		_ph_dims[0]++;	// Z dimension
	}

	_PH = new float[_ph_dims[1] * _ph_dims[2]];
	_PHB = new float[_ph_dims[1] * _ph_dims[2]];
	_zsliceBuf = new float[_ph_dims[1] * _ph_dims[2]];
	_xysliceBuf = new float[_ph_dims[1] * _ph_dims[2]];
}

DCWRF::DerivedVarElevation::~DerivedVarElevation() {

	if (_PH) delete [] _PH;
	if (_PHB) delete [] _PHB;
	if (_zsliceBuf) delete [] _zsliceBuf;
	if (_xysliceBuf) delete [] _xysliceBuf;
}


int DCWRF::DerivedVarElevation::Open(size_t ts) {

	if (_is_open) return(-1);	// Only one variable open at a time

	int PHfd = _ncdfc->OpenRead(ts, _PHvar);
	if (PHfd < 0) return(-1);

	int PHBfd = _ncdfc->OpenRead(ts, _PHBvar);
	if (PHBfd < 0) {
		_ncdfc->Close(PHfd);
		return(-1);
	}

	_PHfd = PHfd;
	_PHBfd = PHBfd;

	_firstSlice = true;
	_is_open = true;

	return(0);
}

int DCWRF::DerivedVarElevation::Read(float *buf, int) {

	//
	// NetCDF dimension ordering!!!
	//
	size_t nx = _sdims[2];
	size_t ny = _sdims[1];

	int rc;
	float *ptr = buf;
	while ((rc = DCWRF::DerivedVarElevation::ReadSlice(buf, 0))>0) {
		ptr += nx*ny;
	}
	return(rc);
		
	return(0);
}

int DCWRF::DerivedVarElevation::_ReadSlice(
	float *slice
) {
	// NetCDF dimension ordering
	//
	size_t nx = _ph_dims[2];
	size_t ny = _ph_dims[1];

	if (_firstSlice && _zinterpolate) {
		int rc;
		rc = _ncdfc->ReadSlice(_PH, _PHfd);
		if (rc<=0) return(rc);

		rc = _ncdfc->ReadSlice(_PHB, _PHBfd);
		if (rc<=0) return(rc);

		for (size_t y=0; y<ny; y++) {
		for (size_t x=0; x<nx; x++) {
			_zsliceBuf[y*nx + x] = (_PH[y*nx + x] + _PHB[y*nx + x]) / _grav;
		}
		}
		_firstSlice = false;
	}
	int rc;
	rc = _ncdfc->ReadSlice(_PH, _PHfd);
	if (rc<=0) return(rc);

	rc = _ncdfc->ReadSlice(_PHB, _PHBfd);
	if (rc<=0) return(rc);

	if (_zinterpolate) {

		// if interpolating _sliceBuf contains previous slice
		//
		for (size_t y=0; y<ny; y++) {
		for (size_t x=0; x<nx; x++) {
			slice[y*nx + x] = 0.5 * _zsliceBuf[y*nx + x];
			_zsliceBuf[y*nx + x] = (_PH[y*nx + x] + _PHB[y*nx + x]) / _grav;
			slice[y*nx + x] += 0.5 * _zsliceBuf[y*nx + x];
		}
		}
	}
	else {
		for (size_t y=0; y<ny; y++) {
		for (size_t x=0; x<nx; x++) {
			slice[y*nx + x] = (_PH[y*nx + x] + _PHB[y*nx + x]) / _grav;
		}
		}
	}
	return(1);
}

int DCWRF::DerivedVarElevation::ReadSlice(
	float *slice, int
) {
	if (! _is_open) {
		SetErrMsg("Invalid operation");
		return(-1);
	}
	
	if (! (_xextrapolate || _yextrapolate)) {
		return (DerivedVarElevation::_ReadSlice(slice));
	}

	int rc = DerivedVarElevation::_ReadSlice(_xysliceBuf);
	if (rc<=0) return(rc);

	// NetCDF dimension ordering
	//
	size_t nx = _ph_dims[2];
	size_t ny = _ph_dims[1];

	if (_xextrapolate) {

		// First interpolate interior points
		//
		size_t nxs = nx+1;	// staggered dimension
		for (size_t y=0; y<ny; y++) {
		for (size_t x=1; x<nxs-1; x++) {
			slice[y*nxs+x] = 0.5*(_xysliceBuf[y*nx+x-1] + _xysliceBuf[y*nx+x]);
		}
		}

		// Next extrapolate boundary points that are outside of 
		// PH and PHB grid: Y[*] = Y[k-1] + deltaX * (Y[k] - Y[k-1])
		//
		for (size_t y=0; y<ny; y++) {	// left boundary
			slice[y*nxs] = _xysliceBuf[y*nx+0] + 
				(-0.5*(_xysliceBuf[y*nx+1] - _zsliceBuf[y*nx+0]));
		}
		for (size_t y=0; y<ny; y++) {	// right boundary
			slice[y*nxs+nxs-1] = _xysliceBuf[y*nx+nx-2] + 
				(0.5*(_xysliceBuf[y*nx+nx-1] - _zsliceBuf[y*nx+nx-2]));
		}
	}
	else {
		// First interpolate interior points
		//
		size_t nys = ny+1;	// staggered dimension
		for (size_t y=1; y<nys-1; y++) {
		for (size_t x=0; x<nx; x++) {
			slice[y*nx+x] = 0.5*(_xysliceBuf[(y-1)*nx+x] + _xysliceBuf[y*nx+x]);
		}
		}

		// Next extrapolate boundary points that are outside of 
		// PH and PHB grid: Y[*] = Y[k-1] + deltaX * (Y[k] - Y[k-1])
		//
		for (size_t x=0; x<nx; x++) {	// bottom boundary
			slice[x] = _xysliceBuf[x] + 
				(-0.5*(_xysliceBuf[nx+x] - _zsliceBuf[x]));
		}
		for (size_t x=0; x<nx; x++) {	// top boundary
			slice[(nys-1)*nx + x] = _xysliceBuf[(ny-2)*nx + x] + 
				(0.5*(_xysliceBuf[(ny-1)*nx + x] - _zsliceBuf[(ny-2)*nx + x]));
		}
	}

	return(1);
}


int DCWRF::DerivedVarElevation::SeekSlice(
    int offset, int whence, int
) {

	int rc = 0;
	if (_ncdfc->SeekSlice(offset, whence, _PHfd)<0) rc = -1;
	if (_ncdfc->SeekSlice(offset, whence, _PHBfd)<0) rc = -1;

	return(rc);
}

int DCWRF::DerivedVarElevation::Close(int) {
	if (! _is_open) return(0);

	int rc = 0;
	if (_ncdfc->Close(_PHfd)<0) rc = -1;
	if (_ncdfc->Close(_PHBfd)<0) rc = -1;

	_is_open = false;
	return(rc);
}

DCWRF::DerivedVarHorizontal::DerivedVarHorizontal(
	NetCDFCollection *ncdfc, string name, const vector <DC::Dimension> &dims,
	const vector <size_t> latlondims, Proj4API *proj4API
) : DerivedVar(ncdfc) {

	assert((name.compare("X")==0) || (name.compare("Y")==0) || (name.compare("XU")==0) || (name.compare("YU")==0) || (name.compare("XV")==0) || (name.compare("YV")==0));

	assert(dims.size() == 2);
	assert(latlondims.size() == 3);

	_proj4API = proj4API;

	// NetCDF dimension order
	//
	vector <DC::Dimension> dimsncdf = dims;
	reverse(dimsncdf.begin(), dimsncdf.end());

	_name = name;
	_time_dim = dimsncdf[0].GetLength();
	_time_dim_name = dimsncdf[0].GetName();

	_sdims.clear();
	_sdims.push_back(dimsncdf[1].GetLength());

	_sdimnames.clear();
	_sdimnames.push_back(dimsncdf[1].GetName());

	// NetCDF order on dimensions
	//
	_nx = latlondims[2];
	_ny = latlondims[1];

	_is_open = false;
	_coords = NULL;
	_sliceBuf = NULL;
	_lonBdryBuf = NULL;
	_latBdryBuf = NULL;
	_ncoords = 0;
	_cached_ts = (size_t) -1;
	_lonname.clear();
	_latname.clear();

	if ((_name.compare("X")==0) || (_name.compare("Y")==0)) {
		_lonname = "XLONG";
		_latname = "XLAT";
		if (_name.compare("X")==0) _ncoords = _nx;
		else _ncoords = _ny;
	}
	else if ((_name.compare("XU")==0) || (_name.compare("YU")==0)) {
		_lonname = "XLONG_U";
		_latname = "XLAT_U";
		if (_name.compare("XU")==0) _ncoords = _nx;
		else _ncoords = _ny;
	}
	else if ((_name.compare("XV")==0) || (_name.compare("YV")==0)) {
		_lonname = "XLONG_V";
		_latname = "XLAT_V";
		if (_name.compare("XV")==0) _ncoords = _nx;
		else _ncoords = _ny;
	}

	_coords = new float[_ncoords];
	_sliceBuf = new float[_nx*_ny];
	_lonBdryBuf = new float[2*_nx + 2*+_ny - 4];
	_latBdryBuf = new float[2*_nx + 2*+_ny - 4];
}

DCWRF::DerivedVarHorizontal::~DerivedVarHorizontal() {

	if (_coords) delete [] _coords;
	if (_sliceBuf) delete [] _sliceBuf;
	if (_lonBdryBuf) delete [] _lonBdryBuf;
	if (_latBdryBuf) delete [] _latBdryBuf;
}

int DCWRF::DerivedVarHorizontal::_GetCartCoords(
	size_t ts
) {

	int fd = _ncdfc->OpenRead(ts,_lonname);
	if (fd<0) {
		SetErrMsg("Can't read %s variable", _lonname.c_str());
		return(-1);
	}

	int rc = _ncdfc->ReadSlice(_sliceBuf, fd);
	if (rc<0) {
		SetErrMsg("Can't read %s variable", _lonname.c_str());
		_ncdfc->Close(fd);
		return(-1);
	}
	_ncdfc->Close(fd);

	GeoUtil::ExtractBoundary(_sliceBuf, _nx, _ny, _lonBdryBuf);

	fd = _ncdfc->OpenRead(ts,_latname);
	if (fd<0) {
		SetErrMsg("Can't read %s variable", _latname.c_str());
		return(-1);
	}

	rc = _ncdfc->ReadSlice(_sliceBuf, fd);
	if (rc<0) {
		SetErrMsg("Can't read %s variable", _latname.c_str());
		_ncdfc->Close(fd);
		return(-1);
	}
	_ncdfc->Close(fd);

	GeoUtil::ExtractBoundary(_sliceBuf, _nx, _ny, _latBdryBuf);

	size_t n = 2*_nx + 2*_ny - 4;
	rc = _proj4API->Transform(_lonBdryBuf, _latBdryBuf, n);

	float minx = _lonBdryBuf[0];
	float maxx = _lonBdryBuf[0];
	float miny = _latBdryBuf[0];
	float maxy = _latBdryBuf[0];
	for (int i=0; i<n; i++) {

		minx = min(minx,_lonBdryBuf[i]);
		maxx = max(maxx,_lonBdryBuf[i]);
		miny = min(miny,_latBdryBuf[i]);
		maxy = max(maxy,_latBdryBuf[i]);
	}

	assert(_nx>1);
	assert(_ny>1);

	float delta;
	if (_name.compare(0,1,"X")==0) { 
		assert(_ncoords == _nx);
		delta = (maxx - minx) / (float) (_nx-1);
	}
	else{
		assert(_ncoords == _ny);
		delta = (maxy - miny) / (float) (_ny-1);
	}
	for (int i=0; i<_ncoords; i++) {
		_coords[i] = (float) i * delta;
	}

	return(0);
}


int DCWRF::DerivedVarHorizontal::Open(size_t ts) {

	if (_is_open) return(-1);	// Only one variable open at a time
	if (ts == _cached_ts) return(0);	// nothing to do - results cached


	// Compute _coords  for timestep ts
	//
	int rc = DerivedVarHorizontal::_GetCartCoords(ts);
	if (rc<0) {
		return(-1);
	}

	_cached_ts = ts;
	_is_open = true;
	return(0);
}

int DCWRF::DerivedVarHorizontal::Read(float *buf, int) {

	if (! _is_open) {
		SetErrMsg("Invalid operation");
		return(-1);
	}

	for (int i=0; i<_ncoords; i++) {
		buf[i] = _coords[i];
	}
	return(0);
}

int DCWRF::DerivedVarHorizontal::ReadSlice(
	float *slice, int
) {
	SetErrMsg("Invalid operation");
	return(-1);
}


int DCWRF::DerivedVarHorizontal::SeekSlice(
    int offset, int whence, int
) {
	SetErrMsg("Invalid operation");
	return(-1);
}

int DCWRF::DerivedVarHorizontal::Close(int) {
	_is_open = false;
	return(0);
}

DCWRF::DerivedVarTime::DerivedVarTime(
	NetCDFCollection *ncdfc, DC::Dimension dim, 
	const std::vector <float> &timecoords
) : DerivedVar(ncdfc) {

	_ts = 0;
	_time_dim_name = dim.GetName();

	_sdims.clear();
	_sdimnames.clear();

	_timecoords = timecoords;
}

int DCWRF::DerivedVarTime::Open(size_t ts) {
	if (ts >= _timecoords.size()) {
		SetErrMsg("Invalid time step : %d", ts);
		return(-1);
	}
	_ts = ts;
	return(0);
}

int DCWRF::DerivedVarTime::Read(float *buf, int) {
	assert(_ts <= _timecoords.size());
	buf[0] = _timecoords[_ts];
	return(0);
}

int DCWRF::DerivedVarTime::ReadSlice(
	float *slice, int
) {
	SetErrMsg("Invalid operation");
	return(-1);
}


int DCWRF::DerivedVarTime::SeekSlice(
    int offset, int whence, int
) {
	SetErrMsg("Invalid operation");
	return(-1);
}

int DCWRF::DerivedVarTime::Close(int) {
	return(0);
}
