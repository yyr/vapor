//
//      $Id$
//

#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <cassert>
#include <stdio.h>
#include <vapor/GeoUtil.h>
#include <vapor/DCReaderWRF.h>
#include <vapor/UDUnitsClass.h>

#ifdef _WINDOWS
#define _USE_MATH_DEFINES
#pragma warning(disable : 4251 4100)
#endif
#include <math.h>

using namespace VAPoR;
using namespace std;

namespace {
bool mycompare(const pair<int, float>  &a, const pair<int, float>  &b) {
	return(a.second < b.second);
}
};

DCReaderWRF::DCReaderWRF(const vector <string> &files) {

	_dims.clear();
	_vars3d.clear();
	_vars2dXY.clear();
	_vars3dExcluded.clear();
	_vars2dExcluded.clear();
	_timeStamps.clear();
	_times.clear();
	_ncdfc = NULL;
	_sliceBuffer = NULL;
	_ovr_fd = -1;
	_mapProj = 0;
	_projString.clear();
	_elev = NULL;
	_timeBias = 0.0;


	vector <string> time_dimnames;
	vector <string> time_coordvars;
	time_dimnames.push_back("Time");

	NetCDFCollection *ncdfc = new NetCDFCollection();

	int rc = ncdfc->Initialize(files, time_dimnames, time_coordvars);
	if (rc<0) {
		SetErrMsg("Failed to initialize netCDF data collection for reading");
		return;
	}

	// Get required and optional global attributes 
	// Initializes members: _dx, _dy, _cen_lat, _cen_lon, _pole_lat,
	// _pole_lon, _grav, _days_per_year, _radius, _p2si
	//
	rc = _InitAtts(ncdfc);
	if (rc< 0) {
		return;
	}

	//
	//  Get the dimensions of the grid. 
	//	Initializes members: _dims, _sliceBuffer
	//
	rc = _InitDimensions(ncdfc);
	if (rc< 0) {
		SetErrMsg("No valid dimensions");
		return;
	}

	// Set up map projection transforms
	// Initializes members: _projString, _proj4API, _mapProj
	//
	rc = _InitProjection(ncdfc, _radius);
	if (rc< 0) {
		return;
	}

	//  Set up the ELEVATION coordinate variable
	//	Initializes members: _elev
	//
	rc = _InitVerticalCoordinates(ncdfc);
	if (rc<0) {
		return;
	}

	// Set up user time and time stamps
	// Initialize members: _timeStamps, _times, _timeLookup
	//
	rc = _InitTime(ncdfc);
	if (rc<0) {
		return;
	}

	//
	// Identify data and coordinate variables. Sets up members:
	// Initializes members: _vars2dXY, _vars3d, _vars3dExcluded, 
	// _vars2dExcluded
	//
	rc = _InitVars(ncdfc) ;
	if (rc<0) return;

	_ncdfc = ncdfc;

	sort(_vars3d.begin(), _vars3d.end());
	sort(_vars2dXY.begin(), _vars2dXY.end());
	sort(_vars3dExcluded.begin(), _vars3dExcluded.end());
	sort(_vars2dExcluded.begin(), _vars2dExcluded.end());
}

int DCReaderWRF::_GetLatLonExtentsCorners(
	NetCDFCollection *ncdfc,
	size_t ts, double lon_exts[2], double lat_exts[2],
	double lon_corners[], double lat_corners[]
) const {
	for (int i=0; i<2; i++) {
		lon_exts[i] = 0.0;
		lat_exts[i] = 0.0;
	}
	
	int fd = ncdfc->OpenRead(ts,"XLONG");
	if (fd<0) {
		SetErrMsg("Can't read XLONG variable");
		ncdfc->Close(fd);
		return(-1);
	}

	int rc = ncdfc->ReadSlice(_sliceBuffer, fd);
	if (rc<0) {
		SetErrMsg("Can't read XLONG variable");
		ncdfc->Close(fd);
		return(-1);
	}
	ncdfc->Close(fd);

	// corners, starting from lower left and going clockwise
	lon_corners[0] = _sliceBuffer[0];
	lon_corners[1] = _sliceBuffer[_dims[0] * (_dims[1]-1)];
	lon_corners[2] = _sliceBuffer[(_dims[0] * _dims[1]) -1];
	lon_corners[3] = _sliceBuffer[(_dims[0]-1)];

	float l0, l1;
	GeoUtil::LonExtents( _sliceBuffer, _dims[0], _dims[1], l0, l1);
	lon_exts[0] = l0;
	lon_exts[1] = l1;

	fd = ncdfc->OpenRead(ts,"XLAT");
	if (fd<0) {
		SetErrMsg("Can't read XLAT variable");
		ncdfc->Close(fd);
		return(-1);
	}

	rc = ncdfc->ReadSlice(_sliceBuffer, fd);
	if (rc<0) {
		SetErrMsg("Can't read XLAT variable");
		ncdfc->Close(fd);
		return(-1);
	}
	ncdfc->Close(fd);

	// corners, starting from lower left and going clockwise
	lat_corners[0] = _sliceBuffer[0];
	lat_corners[1] = _sliceBuffer[_dims[0] * (_dims[1]-1)];
	lat_corners[2] = _sliceBuffer[(_dims[0] * _dims[1]) -1];
	lat_corners[3] = _sliceBuffer[(_dims[0]-1)];

	GeoUtil::LatExtents(_sliceBuffer, _dims[0], _dims[1], l0, l1);
	lat_exts[0] = l0;
	lat_exts[1] = l1;

	return(0);
}



vector <double> DCReaderWRF::GetExtents(size_t ts) const {
	vector <double> extents;
	extents.push_back(0.0);
	extents.push_back(0.0);
	extents.push_back(0.0);
	extents.push_back(1.0);
	extents.push_back(1.0);
	extents.push_back(1.0);

	ts = _timeLookup[ts];
	
	double height[2];
	int rc = _GetVerticalExtents(_ncdfc, ts, height);
	if (rc<0) return(extents);	// No error status. Sigh :-(

	extents[2] = height[0];
	extents[5] = height[1];

	if (! _projString.empty()) {
		//
		// Now get lat-lon coords of grid corners. 
		// We assume these get mapped to corners in the projection
		// into cartographic space. This assumption is only valid
		// with currently supported projections (MAP_PROJ=0,1,2,3,6);
		//
		double lon_exts[2]; // order: ll, ur
		double lat_exts[2]; // order: ll, ur
		double lon_corners[4];	// order clockwise starting lower left
		double lat_corners[4];
		rc = _GetLatLonExtentsCorners(
			_ncdfc, ts, lon_exts, lat_exts, lon_corners, lat_corners
		);
		if (rc<0) return(extents);	// No error status. Sigh :-(

		//
		// Transform lat-lon to cartographic coordinates (performed in place)
		//
		rc = _proj4API.Transform(lon_corners, lat_corners, 4);
		if (rc<0) {
			SetErrMsg("Proj4API::Transform()");
			return(extents);
		}
		double lonmin = lon_corners[0];
		double lonmax = lon_corners[0];
		double latmin = lat_corners[0];
		double latmax = lat_corners[0];
		for (int i=0; i<4; i++) {
			if (lon_corners[i] < lonmin) lonmin = lon_corners[i];
			if (lat_corners[i] < latmin) latmin = lat_corners[i];
			if (lon_corners[i] > lonmax) lonmax = lon_corners[i];
			if (lat_corners[i] > latmax) latmax = lat_corners[i];
		}

		extents[0] = lonmin;
		extents[1] = latmin;
		extents[3] = lonmax;
		extents[4] = latmax;
	}
	else {
		// Idealized case. No geographic coordinates, no projection
		//
		extents[0] = 0.0;
		extents[1] = 0.0;
		extents[3] = (_dims[0] - 1) * _dx;
		extents[4] = (_dims[1] - 1) * _dy;
	}

	return(extents);
}

int DCReaderWRF::_GetVerticalExtents(
	NetCDFCollection *ncdfc, size_t ts, double height[2]
) const {
	height[0] = height[1] = 0.0;

	//
	// Get vertical extents
	//
	int fd = ncdfc->OpenRead(ts,"ELEVATION");
	if (fd<0) {
		SetErrMsg("Can't compute ELEVATION variable");
		return(-1);	
	}

	// Read bottom slice and look for minimum
	//
	int rc = ncdfc->ReadSlice(_sliceBuffer, fd);
	if (rc<0) {
		SetErrMsg("Can't compute ELEVATION variable");
		return(-1);
	}
	float min = _sliceBuffer[0];
	for (size_t i = 0; i < _dims[0]*_dims[1]; i++) {
		if (_sliceBuffer[i]<min) min = _sliceBuffer[i];
	}

	// Read top slice (skipping over inbetween slices) and look for maximum
	//
	ncdfc->SeekSlice(0, 2, fd);
	if (rc<0) {
		SetErrMsg("Can't compute ELEVATION variable");
		return(-1);
	}

	rc = ncdfc->ReadSlice(_sliceBuffer, fd);
	if (rc<0) {
		SetErrMsg("Can't compute ELEVATION variable");
		return(-1);
	}
	ncdfc->Close(fd);

	float max = _sliceBuffer[0];
	for (size_t i = 0; i < _dims[0]*_dims[1]; i++) {
		if (_sliceBuffer[i]>max) max = _sliceBuffer[i];
	}

	height[0] = min;
	height[1] = max;
	return(0);
}

	




vector <size_t> DCReaderWRF::_GetSpatialDims(
	NetCDFCollection *ncdfc, string varname
) const {
	vector <size_t> dims = ncdfc->GetSpatialDims(varname);
	reverse(dims.begin(), dims.end());
	return(dims);
}

vector <string> DCReaderWRF::_GetSpatialDimNames(
	NetCDFCollection *ncdfc, string varname
) const {
	vector <string> v = ncdfc->GetSpatialDimNames(varname);
	reverse(v.begin(), v.end());
	return(v);
}

int DCReaderWRF::_InitAtts(
	NetCDFCollection *ncdfc
) {

	_dx = -1.0;
	_dy = -1.0;
	_cen_lat = 0.0;
	_cen_lon = 0.0;
	_pole_lat = 90.0;
	_pole_lon = 0.0;
	_grav = 9.81;
	_days_per_year = 0;
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

//	string s;
//	ncdfc->GetAtt("", "SIMULATION_START_DATE", s);
//	if (s.empty()) {
//		ncdfc->GetAtt("", "START_DATE", s);
//	}
//	_start_date = s;

	return(0);
}

int DCReaderWRF::_InitProjection(
	NetCDFCollection *ncdfc, float radius
) {
	_projString.clear();
	_mapProj = 0;


	string empty;
	ostringstream oss;

	vector <long> ivalues;
	ncdfc->GetAtt("", "MAP_PROJ", ivalues);
	if (ivalues.size() != 1) {
		SetErrMsg("Error reading required attribute : MAP_PROJ");
		return(-1);
	}
	_mapProj = ivalues[0];

	vector <double> dvalues;
	switch (_mapProj) {
		case 0 : {//Lat Lon
			double lon[2], lat[2];
			double dummy[4];
			(void) _GetLatLonExtentsCorners(ncdfc, 0, lon, lat, dummy, dummy);

			if ((lon[0]-lon[1]) == 0.0 || (lat[0]-lat[1]) == 0.0) {

				// Idealized case. No projection
				//
				_projString.clear();
			}
			else {

				// Compute center lat and lon. May be able to just
				// get this from WRF CEN_LON and CEN_LAT attributes, but
				// not sure if it's reliable
				//
				double lon_0 = (lon[0] + lon[1]) / 2.0;
				double lat_0 = (lat[0] + lat[1]) / 2.0;
				ostringstream oss;
				oss << " +lon_0=" << lon_0 << " +lat_0=" << lat_0;
				_projString = "+proj=eqc +ellps=WGS84" + oss.str();
			}

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
			_projString = "+proj=lcc";
			_projString += " +lon_0=";
			oss.str(empty);
			oss << (double)lon0;
			_projString += oss.str();

			_projString += " +lat_1=";
			oss.str(empty);
			oss << (double)lat1;
			_projString += oss.str();

			_projString += " +lat_2=";
			oss.str(empty);
			oss << (double)lat2;
			_projString += oss.str();

		break;
		}

		case 2: { //Polar stereographic (pure north or south)
			_projString = "+proj=stere";

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

			_projString += " +lat_0=";
			oss.str(empty);
			oss << (double)lat0;
			_projString += oss.str();

			_projString += " +lat_ts=";
			oss.str(empty);
			oss << (double)latts;
			_projString += oss.str();

			ncdfc->GetAtt("", "STAND_LON", dvalues);
			if (dvalues.size() != 1) {
				SetErrMsg("Error reading required attribute : STAND_LON");
				return(-1);
			}
			float lon0 = dvalues[0];
			
			_projString += " +lon_0=";
			oss.str(empty);
			oss << (double)lon0;
			_projString += oss.str();

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
			_projString = "+proj=merc";

			_projString += " +lon_0=";
			oss.str(empty);
			oss << (double)lon0;
			_projString += oss.str();
			
			_projString += " +lat_ts=";
			oss.str(empty);
			oss << (double)latts;
			_projString += oss.str();

		break;
		}

		case(6): { // lat-long, possibly rotated, possibly cassini
			
			// See if this is a regular cylindrical equidistant projection
			// with the pole in the default location
			//
			if (_pole_lat == 90.0 && _pole_lon == 0.0) {

				// Compute center lat and lon. May be able to just
				// get this from WRF CEN_LON and CEN_LAT attributes, but
				// not sure if it's reliable
				//
				double lon[2], lat[2];
				double dummy[4];
				(void) _GetLatLonExtentsCorners(
					ncdfc, 0, lon, lat, dummy, dummy
				);
				double lon_0 = (lon[0] + lon[1]) / 2.0;
				double lat_0 = (lat[0] + lat[1]) / 2.0;
				ostringstream oss;
				oss << " +lon_0=" << lon_0 << " +lat_0=" << lat_0;
				_projString = "+proj=eqc +ellps=WGS84" + oss.str();
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

				_projString = "+proj=ob_tran";
				_projString += " +o_proj=eqc";
				_projString += " +to_meter=0.0174532925199";

				_projString += " +o_lat_p=";
				oss.str(empty);
				oss << (double) _pole_lat;
				_projString += oss.str();
				_projString += "d"; //degrees, not radians

				_projString += " +o_lon_p=";
				oss.str(empty);
				oss << (double)(180.-_pole_lon);
				_projString += oss.str();
				_projString += "d"; //degrees, not radians

				_projString += " +lon_0=";
				oss.str(empty);
				oss << (double)(-lon0);
				_projString += oss.str();
				_projString += "d"; //degrees, not radians
			}
			
			break;
		}
		default:

			SetErrMsg("Unsupported MAP_PROJ value : %d", _mapProj);
			return -1;

	}

	if (_projString.empty()) return(0);

	//
	// PlanetWRF data if radius is not zero
	//
	if (radius > 0) {	// planetWRf (not on earth)
		_projString += " +ellps=sphere";
		stringstream ss;
		ss << radius;
		_projString += " +a=" + ss.str() + " +es=0";
	}
	else {
		_projString += " +ellps=WGS84";
	}

	//
	// Set up projection transforms to/from geographic and cartographic
	// coordinates
	//
	int rc = _proj4API.Initialize("", _projString);
	if (rc < 0) {
		SetErrMsg("Proj4API::Initalize(, %s)", _projString.c_str());
		return(-1);
	}

	return 0;
}


int DCReaderWRF::_InitVerticalCoordinates(
	NetCDFCollection *ncdfc
) {
	_elev = new DerivedVarElevation(ncdfc, _grav);
	
	ncdfc->InstallDerivedVar("ELEVATION", _elev);

	return(0);
}

int DCReaderWRF::_InitTime(
	NetCDFCollection *ncdfc
) {
	_timeStamps.clear();
	_times.clear();

	vector <size_t> dims = _GetSpatialDims(ncdfc, "Times");
	if (dims.size() != 1) {
		SetErrMsg("Invalid Times variable\n");
		return(-1);
	}

	UDUnits udunits;
	int rc = udunits.Initialize();
	if (rc<0) return(-1);

	char *buf = new char[dims[0]+1];
	buf[dims[0]] = '\0';	// Null terminate

	const char *format = "%4d-%2d-%2d_%2d:%2d:%2d";

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

		_timeStamps.push_back(buf);
		_times.push_back(
			udunits.EncodeTime(year, mon, mday, hour, min, sec) * _p2si
		);

	}
	delete [] buf;

	// The NetCDFCollection class doesn't handle the WRF time 
	// variable. Hence, the time steps aren't sorted. Sort them now and
	// create a lookup table to map a time index to the correct time step	
	// in the WRF data collection. N.B. this is only necessary if multiple
	// WRF files are present and they're not passed to Initialize() in 
	// the correct order.
	//
	vector <pair<int, float> > timepairs;
	for (int i=0; i<_times.size(); i++) {
		timepairs.push_back(make_pair(i,_times[i]));
	}
	sort(timepairs.begin(), timepairs.end(), mycompare);
	
	for (int i=0; i<timepairs.size(); i++) {
		_timeLookup.push_back(timepairs[i].first);
	}

	return(0);
}

int DCReaderWRF::_InitDimensions(
	NetCDFCollection *ncdfc
) {
	_dims.clear();
	_sliceBuffer = NULL;

	vector <string> dimnames = ncdfc->GetDimNames();
	vector <size_t> dimlens = ncdfc->GetDims();
	assert(dimnames.size() == dimlens.size());

	size_t nx = 0;
	size_t nxs = 0;
	size_t ny = 0;
	size_t nys = 0;
	size_t nz = 0;
	size_t nzs = 0;
	for (int i=0; i<dimnames.size(); i++) {
		if (dimnames[i].compare("west_east") == 0) nx = dimlens[i];
		if (dimnames[i].compare("west_east_stag") == 0) nxs = dimlens[i];
		if (dimnames[i].compare("south_north") == 0) ny = dimlens[i];
		if (dimnames[i].compare("south_north_stag") == 0) nys = dimlens[i];
		if (dimnames[i].compare("bottom_top") == 0) nz = dimlens[i];
		if (dimnames[i].compare("bottom_top_stag") == 0) nzs = dimlens[i];
	}

	if (nx == 0 || ny == 0 || nz == 0) {
		SetErrMsg("Missing dimension");
		return(-1);
	}

	if ((nxs != 0 && nxs != nx+1) || 
		(nys != 0 && nys != ny+1) || 
		(nzs != 0 && nzs != nz+1)) {

		SetErrMsg("Invalid staggered dimension");
		return(-1);
	}

	_dims.push_back(nx);
	_dims.push_back(ny);
	_dims.push_back(nz);


	//
	// Inform NetCFCFCollection of staggered dim names
	//
	vector <string> sdimnames;
	sdimnames.push_back("west_east_stag");
	sdimnames.push_back("south_north_stag");
	sdimnames.push_back("bottom_top_stag");
	ncdfc->SetStaggeredDims(sdimnames);

	_sliceBuffer = new float[_dims[0]*_dims[1]];
	if (! _sliceBuffer) return(-1);
	
	return(0);
}


DCReaderWRF::~DCReaderWRF() {

	if (_elev) delete _elev;
	if (_sliceBuffer) delete [] _sliceBuffer;

	if (_ncdfc) delete _ncdfc;
}

bool DCReaderWRF::IsCoordinateVariable(string varname) const {
	return(varname.compare("ELEVATION") == 0);
}
	
double DCReaderWRF::GetTSUserTime(size_t ts) const  {
	if (ts >= DCReaderWRF::GetNumTimeSteps()) return(0.0);

	ts = _timeLookup[ts];
	return(_times[ts] + _timeBias);
}

void DCReaderWRF::GetTSUserTimeStamp(size_t ts, string &s) const {
	if (ts >= DCReaderWRF::GetNumTimeSteps()) {
		s = "";
		return;
	}

	ts = _timeLookup[ts];
	s = _timeStamps[ts];
}


int DCReaderWRF::OpenVariableRead(
    size_t ts, string varname, int, int 
) {
	DCReaderWRF::CloseVariable();

	ts = _timeLookup[ts];

	_ovr_fd = _ncdfc->OpenRead(ts, varname);
	return(_ovr_fd);
}

int DCReaderWRF::ReadSlice(float *slice) {

	return(_ncdfc->ReadSlice(slice, _ovr_fd));
}

int DCReaderWRF::Read(float *data) {
	float *ptr = data;

	int rc;
	while ((rc = DCReaderWRF::ReadSlice(ptr)) > 0) {
		ptr += _dims[0] * _dims[1];
	}
	return(rc);
}

int DCReaderWRF::CloseVariable() {
	if (_ovr_fd < 0) return (0);
	int rc = _ncdfc->Close(_ovr_fd);
	_ovr_fd = -1;
	return(rc);
}

int DCReaderWRF::_InitVars(NetCDFCollection *ncdfc) 
{
	_vars2dXY.clear();
	_vars3d.clear();
	_vars3dExcluded.clear();
	_vars2dExcluded.clear();

	//
	// Get 3D variables that are sampled on the base (or staggered) grid
	//
	vector <string> vars = ncdfc->GetVariableNames(3, true);
	for (int i=0; i<vars.size(); i++) {
		bool exclude = false;

		vector <size_t> dims = _GetSpatialDims(ncdfc, vars[i]);
		assert(dims.size() == 3);

		if (dims != _dims) {
			exclude = true;
		}
		if (exclude) {
			_vars3dExcluded.push_back(vars[i]);
		}
		else {
			_vars3d.push_back(vars[i]);
		}
	}

	//
	// Get 2D variables that are sampled on the base (or staggered) grid
	//
	vars = ncdfc->GetVariableNames(2, true);
	for (int i=0; i<vars.size(); i++) {
		bool exclude = false;

		vector <size_t> dims = _GetSpatialDims(ncdfc, vars[i]);
		assert(dims.size() == 2);

		for (int j=0; j<2; j++) {
			if (dims[j] != _dims[j]) {
				exclude = true;
			}
		}
		if (exclude) {
			_vars2dExcluded.push_back(vars[i]);
		}
		else {
			_vars2dXY.push_back(vars[i]);
		}
	}

	return(0);
}


DCReaderWRF::DerivedVarElevation::DerivedVarElevation(
	NetCDFCollection *ncdfc, float grav
) : DerivedVar(ncdfc) {

	_dims.resize(3);
	_dimnames.resize(3);
	_grav = grav;
	_PHvar = "PH";
	_PHBvar = "PHB";
	_PH = NULL;
	_PHB = NULL;
	_PHfd = -1;
	_PHBfd = -1;
	_num_ts = 0;
	_is_open = false;
	_ok = false;

	vector <size_t> dims_tmp;
	vector <string> dimnames_tmp;
	size_t num_ts_tmp;
	if (_ncdfc->VariableExists(_PHvar)) {
		dims_tmp = _ncdfc->GetSpatialDims(_PHvar);
		dimnames_tmp = _ncdfc->GetSpatialDimNames(_PHvar);
		num_ts_tmp = _ncdfc->GetTimeDim(_PHvar);
	}
	else {
		return;
	}
	_dims = dims_tmp;
	_dimnames = dimnames_tmp;
	_num_ts = num_ts_tmp;

	if (_ncdfc->VariableExists(_PHBvar)) {
		dims_tmp = _ncdfc->GetSpatialDims(_PHBvar);
		dimnames_tmp = _ncdfc->GetSpatialDimNames(_PHBvar);
		num_ts_tmp = _ncdfc->GetTimeDim(_PHBvar);
	}
	else {
		return;
	}

	if ((dims_tmp != _dims) || (dimnames_tmp != _dimnames) || (num_ts_tmp != _num_ts)) {
		return;
	}

	_PH = new float[_dims[1] * _dims[2]];
	_PHB = new float[_dims[1] * _dims[2]];
	_ok = true;
}

DCReaderWRF::DerivedVarElevation::~DerivedVarElevation() {
	if (_PH) delete [] _PH;
	if (_PHB) delete [] _PHB;
}


int DCReaderWRF::DerivedVarElevation::Open(size_t ts) {

	if (_is_open) return(-1);	// Only one variable open at a time
	if (! _ok) {
		SetErrMsg("Missing forumla terms");
		return(-1);
	}

	int PHfd = _ncdfc->OpenRead(ts, _PHvar);
	if (PHfd < 0) return(-1);

	int PHBfd = _ncdfc->OpenRead(ts, _PHBvar);
	if (PHBfd < 0) {
		_ncdfc->Close(PHfd);
		return(-1);
	}

	_PHfd = PHfd;
	_PHBfd = PHBfd;

	_is_open = true;
	return(0);
}

int DCReaderWRF::DerivedVarElevation::Read(float *buf, int) {

	//
	// NetCDF dimension ordering!!!
	//
	size_t nx = _dims[2];
	size_t ny = _dims[1];

	int rc;
	float *ptr = buf;
	while ((rc = DCReaderWRF::DerivedVarElevation::ReadSlice(buf, 0))>0) {
		ptr += nx*ny;
	}
	return(rc);
		
	return(0);
}

int DCReaderWRF::DerivedVarElevation::ReadSlice(
	float *slice, int
) {
	size_t nx = _dims[2];
	size_t ny = _dims[1];

	int rc;
	rc = _ncdfc->ReadSlice(_PH, _PHfd);
	if (rc<=0) return(rc);

	rc = _ncdfc->ReadSlice(_PHB, _PHBfd);
	if (rc<=0) return(rc);
	
	for (size_t y=0; y<ny; y++) {
	for (size_t x=0; x<nx; x++) {
		slice[y*nx + x] = (_PH[y*nx + x] + _PHB[y*nx + x]) / _grav;
	}
	}

	return(1);
}


int DCReaderWRF::DerivedVarElevation::SeekSlice(
    int offset, int whence, int
) {

	int rc = 0;
	if (_ncdfc->SeekSlice(offset, whence, _PHfd)<0) rc = -1;
	if (_ncdfc->SeekSlice(offset, whence, _PHBfd)<0) rc = -1;

	return(rc);
}

int DCReaderWRF::DerivedVarElevation::Close(int) {
	if (! _is_open) return(0);

	int rc = 0;
	if (_ncdfc->Close(_PHfd)<0) rc = -1;
	if (_ncdfc->Close(_PHBfd)<0) rc = -1;

	_is_open = false;
	return(rc);
}
