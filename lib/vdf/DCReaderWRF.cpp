//
//      $Id$
//

#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <cassert>
#include <stdio.h>
#include <vapor/DCReaderWRF.h>
#include <vapor/UDUnitsClass.h>

#ifdef _WINDOWS
#define _USE_MATH_DEFINES
#pragma warning(disable : 4251 4100)
#endif
#include <math.h>

using namespace VAPoR;
using namespace std;

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
	_srcProjDef = NULL;
	_dstProjDef = NULL;
	_mapProj = 0;
	_projString.clear();
	_elev = NULL;
	_timeBias = 0.0;


	vector <string> time_dimnames;
	vector <string> time_coordvars;
	time_dimnames.push_back("Time");

	NetCDFCollection *ncdfc = new NetCDFCollection();

	// Workaround for bug #953:  reverses time in WRF vdc conversion
	// NetCDFCollection expects a 1D time coordinate variable. WRF
	// data use a 2D "Times" time coordinate variable. By convention,
	// however, WRF output files are sorted by time. Just need to
	// make sure they're passed in the proper order
	//
	vector <string> sorted_files = files;
	std::sort(sorted_files.begin(), sorted_files.end());

	ncdfc->Initialize(sorted_files, time_dimnames, time_coordvars);
	if (GetErrCode() != 0) return;

	// Get required and optional global attributes 
	// Initializes members: _dx, _dy, _cen_lat, _cen_lon, _grav,
	// _days_per_year, _radius, _p2si
	//
	int rc = _InitAtts(ncdfc);
	if (rc< 0) {
		return;
	}

	// Set up map projection transforms
	// Initializes members: _projString, _srcProjDef, _dstProjDef, _mapProj
	//
	rc = _InitProjection(ncdfc, _radius);
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

	//  Set up the ELEVATION coordinate variable
	//	Initializes members: _elev
	//
	rc = _InitVerticalCoordinates(ncdfc);
	if (rc<0) {
		return;
	}

	// Set up user time and time stamps
	// Initialize members: _timeStamps, _times
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
}

int DCReaderWRF::_GetHorizExtents(
	size_t ts, double lon_exts[2], double lat_exts[2]
) const {
	for (int i=0; i<2; i++) {
		lon_exts[i] = 0.0;
		lat_exts[i] = 0.0;
	}
	
	int fd = _ncdfc->OpenRead(ts,"XLONG");
	if (fd<0) {
		SetErrMsg("Can't read XLONG variable");
		_ncdfc->Close(fd);
		return(-1);
	}

	int rc = _ncdfc->ReadSlice(_sliceBuffer, fd);
	if (rc<0) {
		SetErrMsg("Can't read XLONG variable");
		_ncdfc->Close(fd);
		return(-1);
	}
	_ncdfc->Close(fd);

	lon_exts[0] = _sliceBuffer[0];
	lon_exts[1] = _sliceBuffer[_dims[0]*_dims[1]-1];

	fd = _ncdfc->OpenRead(ts,"XLAT");
	if (fd<0) {
		SetErrMsg("Can't read XLAT variable");
		_ncdfc->Close(fd);
		return(-1);
	}

	rc = _ncdfc->ReadSlice(_sliceBuffer, fd);
	if (rc<0) {
		SetErrMsg("Can't read XLAT variable");
		_ncdfc->Close(fd);
		return(-1);
	}
	_ncdfc->Close(fd);

	lat_exts[0] = _sliceBuffer[0];
	lat_exts[1] = _sliceBuffer[_dims[0]*_dims[1]-1];

	// Convert to radians
	//
	for (int i=0; i<2; i++) {
		lon_exts[i] *= M_PI / 180.0;
		lat_exts[i] *= M_PI / 180.0;
	}

	//
	// Perform projection
	//
	if (_radius == 0.0) {
		rc = pj_transform(
			_srcProjDef, _dstProjDef, 2,1, lon_exts, lat_exts, NULL
		);
		if (rc>=1) {
			SetErrMsg(
				"Error transforming extents : %s",
				pj_strerrno(*(pj_get_errno_ref()))
			);
			return(-1);
		}
	}
	else {
		// Hack for Plan
		//
		for (int i=0; i<2; i++) {
			lon_exts[i] *= _radius;
			lat_exts[i] *= _radius;
		}
	}

	return(0);
}

int DCReaderWRF::_GetHorizExtentsRot(
	size_t ts, double lon_exts[2], double lat_exts[2]
) const {
	for (int i=0; i<2; i++) {
		lon_exts[i] = 0.0;
		lat_exts[i] = 0.0;
	}
	
	double grid_center[2] = {
		_cen_lon * M_PI / 180.0,
		_cen_lat * M_PI / 180.0
	};
	

	// inverse project CEN_LAT/CEN_LON, that gives grid center 
	// in computational grid.
	//
	int rc = pj_transform(
		_srcProjDef, _dstProjDef, 1,1, grid_center, grid_center+1, NULL
	);
	if (rc>=1) {
		SetErrMsg(
			"Error transforming extents : %s",
			pj_strerrno(*(pj_get_errno_ref()))
		);
		return(-1);
	}
	grid_center[0] *= 180.0 / M_PI;
	grid_center[1] *= 180.0 / M_PI;

	// Convert grid center (currently in degrees from (-180, -90) 
	// to (180,90)) to meters from origin at equator
	//
     grid_center[0] = grid_center[0]*111177.;
     grid_center[1] = grid_center[1]*111177.;


    // Use grid size to calculate extents
	//
	lon_exts[0] = grid_center[0]-0.5*(_dims[0]-1)*_dx;
    lon_exts[1] = grid_center[1]-0.5*(_dims[1]-1)*_dy;
    lat_exts[0] = grid_center[0]+0.5*(_dims[0]-1)*_dx;
    lat_exts[1] = grid_center[1]+0.5*(_dims[1]-1)*_dy;

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
	

	//
	// Get vertical extents
	//
	int fd = _ncdfc->OpenRead(ts,"ELEVATION");
	if (fd<0) {
		SetErrMsg("Can't compute ELEVATION variable");
		return(extents);	// No error status. Sigh :-(
	}

	// Read bottom slice and look for minimum
	//
	int rc = _ncdfc->ReadSlice(_sliceBuffer, fd);
	if (rc<0) {
		SetErrMsg("Can't compute ELEVATION variable");
		return(extents);	// No error status. Sigh :-(
	}
	float min = _sliceBuffer[0];
	for (size_t i = 0; i < _dims[0]*_dims[1]; i++) {
		if (_sliceBuffer[i]<min) min = _sliceBuffer[i];
	}

	// Read top slice (skipping over inbetween slices) and look for maximum
	//
	_ncdfc->SeekSlice(0, 2, fd);
	if (rc<0) {
		SetErrMsg("Can't compute ELEVATION variable");
		return(extents);	// No error status. Sigh :-(
	}

	rc = _ncdfc->ReadSlice(_sliceBuffer, fd);
	if (rc<0) {
		SetErrMsg("Can't compute ELEVATION variable");
		return(extents);	// No error status. Sigh :-(
	}
	_ncdfc->Close(fd);

	float max = _sliceBuffer[0];
	for (size_t i = 0; i < _dims[0]*_dims[1]; i++) {
		if (_sliceBuffer[i]>max) max = _sliceBuffer[i];
	}
	extents[2] = min;
	extents[5] = max;

	//
	// Now get horizontal extents
	//
	double lon_exts[2]; // order: ll, ur
	double lat_exts[2]; // order: ll, ur

	if (_mapProj == 6) {	//cassini or rotated lat/lon
		rc = _GetHorizExtentsRot(ts, lon_exts, lat_exts);
	}
	else {
		rc = _GetHorizExtents(ts, lon_exts, lat_exts);
	}
	if (rc<0) {
		return(extents);	// No error status. Sigh :-(
	}

	extents[0] = lon_exts[0];
	extents[1] = lat_exts[0];
	extents[3] = lon_exts[1];
	extents[4] = lat_exts[1];

	return(extents);
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
	_srcProjDef = NULL;
	_dstProjDef = NULL;
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
	switch (_mapProj){
		case 0 : //Lat Lon
			_projString = "+proj=latlong";
			_projString += " +ellps=sphere";

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

			_projString += " +ellps=sphere";
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

			_projString += " +ellps=sphere";
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

			_projString += " +ellps=sphere";
		break;
		}

		case(6): { //cassini or rotated lat/lon  
			
			ncdfc->GetAtt("", "POLE_LAT", dvalues);
			if (dvalues.size() != 1) {
				SetErrMsg("Error reading required attribute : POLE_LAT");
				return(-1);
			}
			float latp = dvalues[0];

			ncdfc->GetAtt("", "POLE_LON", dvalues);
			if (dvalues.size() != 1) {
				SetErrMsg("Error reading required attribute : POLE_LON");
				return(-1);
			}
			float lonp = dvalues[0];

			ncdfc->GetAtt("", "STAND_LON", dvalues);
			if (dvalues.size() != 1) {
				SetErrMsg("Error reading required attribute : STAND_LON");
				return(-1);
			}
			float lon0 = dvalues[0];

			_projString = "+proj=ob_tran";
			_projString += " +o_proj=latlong";

			_projString += " +o_lat_p=";
			oss.str(empty);
			oss << (double)latp;
			_projString += oss.str();
			_projString += "d"; //degrees, not radians

			_projString += " +o_lon_p=";
			oss.str(empty);
			oss << (double)(180.-lonp);
			_projString += oss.str();
			_projString += "d"; //degrees, not radians

			_projString += " +lon_0=";
			oss.str(empty);
			oss << (double)(-lon0);
			_projString += oss.str();
			_projString += "d"; //degrees, not radians
			_projString += " +ellps=sphere";
			
			break;
		}
		default:

			SetErrMsg("Unsupported MAP_PROJ value : %d", _mapProj);
			return -1;

	}

	string latLongProjString = "+proj=latlong";

	//
	// We set up a different proj4 conversion string for 
	// PlanetWRF data, but later on we don't actually us it :-(. 
	//
	if (radius > 0) {	// planetWRf (not on earth)
		stringstream ss;
		ss << radius;
		latLongProjString += " +a=" + ss.str() + " +es=0";
	}
	else {
		latLongProjString += " +ellps=sphere";
	}



	
	_srcProjDef = pj_init_plus(latLongProjString.c_str());
	if (! _srcProjDef) {
		SetErrMsg(
			"Error in creating map reprojection string : %s",
			pj_strerrno(*(pj_get_errno_ref()))
		);
		return(-1);
	}

	_dstProjDef = pj_init_plus(_projString.c_str());
	if (! _srcProjDef) {
		SetErrMsg(
			"Invalid geo-referencing with proj string %s: %s",
			_projString.c_str(), pj_strerrno(*(pj_get_errno_ref()))
		);
		pj_free(_srcProjDef);
		return(-1);
	}

	return 0;
}


int DCReaderWRF::_InitVerticalCoordinates(
	NetCDFCollection *ncdfc
) {
	_elev = NULL;

	DerivedVarElevation *_elev = new DerivedVarElevation(ncdfc, _grav);
	
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

	if (_srcProjDef) pj_free(_srcProjDef);
	if (_dstProjDef) pj_free(_dstProjDef);

	if (_ncdfc) delete _ncdfc;
}

bool DCReaderWRF::IsCoordinateVariable(string varname) const {
	return(varname.compare("ELEVATION") == 0);
}
	
double DCReaderWRF::GetTSUserTime(size_t ts) const  {
	if (ts >= DCReaderWRF::GetNumTimeSteps()) return(0.0);

	return(_times[ts] + _timeBias);
}

void DCReaderWRF::GetTSUserTimeStamp(size_t ts, string &s) const {
	if (ts >= DCReaderWRF::GetNumTimeSteps()) {
		s = "";
		return;
	}

	s = _timeStamps[ts];
}


int DCReaderWRF::OpenVariableRead(
    size_t timestep, string varname, int, int 
) {
	DCReaderWRF::CloseVariable();

	_ovr_fd = _ncdfc->OpenRead(timestep, varname);
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
