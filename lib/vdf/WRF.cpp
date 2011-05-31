//
// $Id$
//
#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sstream>
#include <ctime>
#include <netcdf.h>
#include "assert.h"
#include "proj_api.h"
#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/WRF.h>
#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace VetsUtil;
using namespace VAPoR;

#define NC_ERR_READ(X) \
    { \
    int rc = (X); \
    if (rc != NC_NOERR) { \
        MyBase::SetErrMsg("Error reading netCDF file at line %d : %s", \
		__LINE__,  nc_strerror(rc)) ; \
        return(-1); \
    } \
    }

int WRF::_WRF(
	const string &wrfname, const map <string, string> &names
) {

	_ncid = 0;

	//
	// Deal with non-standard names for required variables
	//
	map <string, string>::const_iterator itr;
	_atypnames["U"] = ((itr=names.find("U"))!=names.end()) ? itr->second : "U";
	_atypnames["V"] = ((itr=names.find("V"))!=names.end()) ? itr->second : "V";
	_atypnames["W"] = ((itr=names.find("W"))!=names.end()) ? itr->second : "W";
	_atypnames["PH"] = ((itr=names.find("PH"))!=names.end()) ? itr->second : "PH";
	_atypnames["PHB"] = ((itr=names.find("PHB"))!=names.end()) ? itr->second : "PHB";
	_atypnames["P"] = ((itr=names.find("P"))!=names.end()) ? itr->second : "P";
	_atypnames["PB"] = ((itr=names.find("PB"))!=names.end()) ? itr->second : "PB";
	_atypnames["T"] = ((itr=names.find("T"))!=names.end()) ? itr->second : "T";

	_vertExts[0] = _vertExts[1] = 0.0;
	_dimLens[0] = _dimLens[1] = _dimLens[2] = _dimLens[3] = 1;

	// Open netCDF file and check for failure
	NC_ERR_READ( nc_open( wrfname.c_str(), NC_NOWRITE, &_ncid ));

	int rc = _GetWRFMeta(
		_ncid, _vertExts, _dimLens, _startDate, _mapProjection,
		_wrfVars3d, _wrfVars2d, _wrfVarInfo, _gl_attrib, _tsExtents
	);

	return(rc);
}

WRF::WRF(const string &wrfname) {
	map <string, string> atypnames;

	atypnames["U"] = "U";
	atypnames["V"] = "V";
	atypnames["W"] = "W";
	atypnames["PH"] = "PH";
	atypnames["PHB"] = "PHB";
	atypnames["P"] = "P";
	atypnames["PB"] = "PB";
	atypnames["T"] = "T";

	(void) _WRF(wrfname, atypnames);
}

WRF::WRF(const string &wrfname, const map <string, string> &atypnames) {

	(void) _WRF(wrfname, atypnames);
}

WRF::~WRF() {
	// Close the WRF file
	(void) nc_close( _ncid );
}

WRF::varFileHandle_t *WRF::Open(
	const string &varname
) {
	//
	// Make sure we have a record for the variable
	//
	bool found = false;
	int index = -1;
	for (int i=0; i<_wrfVarInfo.size(); i++) {
		if (varname.compare(_wrfVarInfo[i].alias) == 0) {
			found = true;
			index = i;
		}
	}
	if (! found) {
		SetErrMsg("Variable not found : %s", varname.c_str());
		return(NULL);
	}


	//
	// Allocate space for a single, staggered slice (even if variable 
	// is not staggered). N.B. dimLens contains unstaggered dimensions
	//
	size_t sz = (_dimLens[0]+1) * (_dimLens[1]+1);

	varFileHandle_t *fhptr = new varFileHandle_t();
	fhptr->buffer = new float[sz];
	if (! fhptr->buffer) return(NULL);

	fhptr->z = -1;	// Invalid slice #
	fhptr->wrft = -1;	// Invalid time step
	fhptr->thisVar = _wrfVarInfo[index];

	return(fhptr);
}

int WRF::Close(varFileHandle_t *fh) {

	if (fh->buffer) delete [] fh->buffer;
	delete fh;
	return(0);
}

//Read 4 corner coordinates (in latitude and longitude) from the xlat,xlong variables.
//Put the result in coords[], in order ll, ur, ul, lr; so first 2 are extents
int WRF::_GetCornerCoords(int ncid, int ts, const varInfo_t &latInfo, const varInfo_t &lonInfo, float coords[8]){
	size_t coordIndex[3];
	coordIndex[2] = 0;
	coordIndex[1] = 0;
	coordIndex[0] = ts;
	int rc = nc_get_var1_float(ncid, lonInfo.varid, coordIndex, coords);
	if (rc<0) return rc;
	rc = nc_get_var1_float(ncid, latInfo.varid, coordIndex,coords+1);
	if (rc<0) return rc;
	//set to ysize, xsize, ts
	// dimlens contains time, south_north, west_east coords for both lat and lon
	// following could use either latInfo.dimlens or lonInfo.dimlens
	coordIndex[2] = lonInfo.dimlens[2]-1;//max west_east coord
	coordIndex[1] = lonInfo.dimlens[1]-1;//max south_north coord
	rc = nc_get_var1_float(ncid, lonInfo.varid, coordIndex,coords+2);
	if (rc<0) return rc;
	rc = nc_get_var1_float(ncid, latInfo.varid, coordIndex,coords+3);
	if (rc<0) return rc;
	//set to xsize, 0, ts
	coordIndex[2] = lonInfo.dimlens[2]-1;
	coordIndex[1] = 0;
	rc = nc_get_var1_float(ncid, lonInfo.varid, coordIndex,coords+4);
	if (rc<0) return rc;
	rc = nc_get_var1_float(ncid, latInfo.varid, coordIndex,coords+5);
	if (rc<0) return rc;
	
	//set to 0,ysize, ts
	coordIndex[2] = 0;
	coordIndex[1] = lonInfo.dimlens[1]-1;
	rc = nc_get_var1_float(ncid, lonInfo.varid, coordIndex,coords+6);
	if (rc<0) return rc;
	rc = nc_get_var1_float(ncid, latInfo.varid, coordIndex,coords+7);
	if (rc<0) return rc;
	return 0;
}

int WRF::_GetProjectionString(int ncid, string& projString){
	string empty;
	string ErrMsgStr;
	ostringstream oss;
	float lat0,lat1,lat2,lon0,latts,latp,lonp;
	int projNum;

	const double RAD2DEG = 180./3.141592653589793;
	if (nc_get_att_int( ncid, NC_GLOBAL, "MAP_PROJ", &projNum ) != NC_NOERR) return -1;

	switch (projNum){
		case(0): //Lat Lon
			projString = "+proj=latlong";
			projString += " +ellps=sphere";
			break;

		case(1): //Lambert
			
			NC_ERR_READ( nc_get_att_float( ncid, NC_GLOBAL, "STAND_LON", &lon0 ) );
			NC_ERR_READ( nc_get_att_float( ncid, NC_GLOBAL, "TRUELAT1", &lat1 ) );
			NC_ERR_READ( nc_get_att_float( ncid, NC_GLOBAL, "TRUELAT2", &lat2 ) );
			//Construct the projection string:
			projString = "+proj=lcc";
			projString += " +lon_0=";
			oss.str(empty);
			oss << (double)lon0;
			projString += oss.str();

			projString += " +lat_1=";
			oss.str(empty);
			oss << (double)lat1;
			projString += oss.str();

			projString += " +lat_2=";
			oss.str(empty);
			oss << (double)lat2;
			projString += oss.str();

			projString += " +ellps=sphere";
			break;
		case(2): //Polar stereographic (pure north or south)
			projString = "+proj=stere";
			//Determine whether north or south pole (lat_ts is pos or neg)
			
			NC_ERR_READ( nc_get_att_float( ncid, NC_GLOBAL, "TRUELAT1", &latts ) );
		
			if (latts < 0.) lat0 = -90.0;
			else lat0 = 90.0;

			projString += " +lat_0=";
			oss.str(empty);
			oss << (double)lat0;
			projString += oss.str();

			projString += " +lat_ts=";
			oss.str(empty);
			oss << (double)latts;
			projString += oss.str();

			NC_ERR_READ( nc_get_att_float( ncid, NC_GLOBAL, "STAND_LON", &lon0 ) );
			
			projString += " +lon_0=";
			oss.str(empty);
			oss << (double)lon0;
			projString += oss.str();

			projString += " +ellps=sphere";
			break;
		case(3): //Mercator
			
			NC_ERR_READ( nc_get_att_float( ncid, NC_GLOBAL, "TRUELAT1", &latts ) );
			NC_ERR_READ( nc_get_att_float( ncid, NC_GLOBAL, "STAND_LON", &lon0 ) );
			//Construct the projection string:
			projString = "+proj=merc";

			projString += " +lon_0=";
			oss.str(empty);
			oss << (double)lon0;
			projString += oss.str();
			
			projString += " +lat_ts=";
			oss.str(empty);
			oss << (double)latts;
			projString += oss.str();

			projString += " +ellps=sphere";
			break;
		case(6): //cassini or rotated lat/lon  
			
			NC_ERR_READ( nc_get_att_float( ncid, NC_GLOBAL, "POLE_LAT", &latp ) );
			NC_ERR_READ( nc_get_att_float( ncid, NC_GLOBAL, "POLE_LON", &lonp ) );
			NC_ERR_READ( nc_get_att_float( ncid, NC_GLOBAL, "STAND_LON", &lon0 ) );
			
			projString = "+proj=ob_tran";
			projString += " +o_proj=latlong";

			projString += " +o_lat_p=";
			oss.str(empty);
			oss << (double)latp;
			projString += oss.str();
			projString += "d"; //degrees, not radians

			projString += " +o_lon_p=";
			oss.str(empty);
			oss << (double)(180.-lonp);
			projString += oss.str();
			projString += "d"; //degrees, not radians

			projString += " +lon_0=";
			oss.str(empty);
			oss << (double)(-lon0);
			projString += oss.str();
			projString += "d"; //degrees, not radians
			projString += " +ellps=sphere";
			
			break;
		default:

			ErrMsgStr.assign("Unsupported MAP_PROJ value ");
			ErrMsgStr += projNum ;
			SetErrMsg(ErrMsgStr.c_str());
			return -1;

	}
	return 0;
	
}

// Gets info about a  variable and stores that info in thisVar.
int	WRF::_GetVarInfo(
	int ncid, // ID of the file we're reading
	string wrfvname,	// name of variable as it appears in file
	const vector <ncdim_t> &ncdims,
	varInfo_t & thisVar // Variable info
) {

	thisVar.wrfvname = wrfvname;
	thisVar.alias = wrfvname;

    map <string, string>::const_iterator itr;
	for (itr = _atypnames.begin(); itr != _atypnames.end(); itr++) {
		if (wrfvname.compare(itr->second) == 0) {
			thisVar.alias = itr->first;
		}
	} 

	int nc_status = nc_inq_varid(ncid, thisVar.wrfvname.c_str(), &thisVar.varid);
	if (nc_status != NC_NOERR) {
		MyBase::SetErrMsg(
			"Variable %s not found in netCDF file", thisVar.wrfvname.c_str()
		);
		return(-1);
	}

	char dummy[NC_MAX_NAME+1]; // Will hold a variable's name
	int natts; // Number of attributes associated with this variable
	int dimids[NC_MAX_VAR_DIMS], ndimids;
	NC_ERR_READ( nc_inq_var(
		ncid, thisVar.varid, dummy, &thisVar.xtype, &ndimids,
		dimids, &natts
	));

	thisVar.dimids.clear();
	thisVar.dimlens.clear();
	for (int i = 0; i<ndimids; i++) {
		thisVar.dimids.push_back(dimids[i]);
		thisVar.dimlens.push_back(ncdims[dimids[i]].size);
	}

	// Determine if variable has staggered dimensions. Only three
	// fastest varying dimensions are checked
	//
	thisVar.stag.push_back(false);
	thisVar.stag.push_back(false);
	thisVar.stag.push_back(false);

	if (thisVar.dimids.size() == 4) { 
		if (strcmp(ncdims[thisVar.dimids[1]].name, "bottom_top_stag") == 0) {
			thisVar.stag[2] = true;
		}
		if (strcmp(ncdims[thisVar.dimids[2]].name, "south_north_stag") == 0) { 
			thisVar.stag[1] = true;
		}
		if (strcmp(ncdims[thisVar.dimids[3]].name, "west_east_stag") == 0) {
			thisVar.stag[0] = true;
		}
	}
	else if (thisVar.dimids.size() == 3) { 
		if (strcmp(ncdims[thisVar.dimids[1]].name, "south_north_stag") == 0) {
			thisVar.stag[1] = true;
		}
		if (strcmp(ncdims[thisVar.dimids[2]].name, "west_east_stag") == 0) { 
			thisVar.stag[0] = true;
		}
	}

	return(0);
	
}



// Reads a single horizontal slice of netCDF data
int WRF::_ReadZSlice4D(
	int ncid, // ID of the netCDF file
	const varInfo_t & thisVar, // Struct for the variable we want
	size_t wrfT, // The WRF time step we want
	size_t z, // Which z slice we want
	float * fbuffer // Buffer we're going to store slice in
) {

	size_t start[NC_MAX_DIMS]; // The point from which we start reading netCDF data
	size_t count[NC_MAX_DIMS]; // What interval to read the data

	// Initialize the count and start arrays for extracting slices from the data:
	for (int i = 0; i<thisVar.dimids.size(); i++){
		start[i] = 0; // Start reading in a corner
		count[i] = 1; // Read every point (changes later)
	}

	start[thisVar.dimids.size()-4] = wrfT;
	start[thisVar.dimids.size()-3] = z;
	count[thisVar.dimids.size()-2] = thisVar.dimlens[thisVar.dimids.size()-2];
	count[thisVar.dimids.size()-1] = thisVar.dimlens[thisVar.dimids.size()-1];

	NC_ERR_READ( nc_get_vara_float(ncid, thisVar.varid, start, count, fbuffer));

	return(0);
}

// Reads a single slice from a 3D (time + space) variable 
int WRF::_ReadZSlice3D(
	int ncid, // ID of the netCDF file
	const varInfo_t & thisVar, // Struct for the variable we want
	size_t wrfT, // The WRF time step we want
	float * fbuffer // Buffer we're going to store slice in
) {

	size_t start[NC_MAX_DIMS]; // The point from which we start reading netCDF data
	size_t count[NC_MAX_DIMS]; // What interval to read the data

	// Initialize the count and start arrays for extracting slices from the data:
	for (int i = 0; i<thisVar.dimids.size(); i++){
		start[i] = 0; // Start reading in a corner
		count[i] = 1; // Read every point (changes later)
	}

	start[thisVar.dimids.size()-3] = wrfT;
	count[thisVar.dimids.size()-2] = thisVar.dimlens[thisVar.dimids.size()-2];
	count[thisVar.dimids.size()-1] = thisVar.dimlens[thisVar.dimids.size()-1];

	NC_ERR_READ( nc_get_vara_float(ncid, thisVar.varid, start, count, fbuffer));

	return(0);
}



void WRF::_InterpHorizSlice(
	float * fbuffer, // The slice of data to interpolate
	const varInfo_t & thisVar // Data about the variable
) {
	// NOTE: As of July 20, 2007 (when this function was written), interpolating
	// data that is staggered in both horizontal dimensions has not been tested.

	// Define these just for convenience
	size_t xUnstagDim = _dimLens[0];
	size_t xStagDim = xUnstagDim + 1;
	size_t yUnstagDim = _dimLens[1];
	size_t yStagDim = yUnstagDim + 1;

	size_t xDimNow, yDimNow;
	if ( thisVar.stag[0] )
		xDimNow = xStagDim;
	else
		xDimNow = xUnstagDim;

	if ( thisVar.stag[1] )
		yDimNow = yStagDim;
	else
		yDimNow = yUnstagDim;
	
	// Interpolate a staggered (N+1 points) x 
	// variable to an unstaggered (N points) x grid
	//
	float *bufptr = fbuffer;
	if ( thisVar.stag[0] ) {
		for (size_t y = 0; y<yDimNow; y++) {
		for (size_t x = 0; x<xUnstagDim; x++) {
			*bufptr = (fbuffer[x + xDimNow*y] + fbuffer[x + 1 + xDimNow*y])/2.0;
			bufptr++;
		}
		}
	}

	// Interpolate a staggered (N+1 points) y 
	// variable to an unstaggered (N points) y grid
	//
	bufptr = fbuffer;
	if ( thisVar.stag[1] ) {
		for (size_t y = 0; y<yUnstagDim; y++) {
		for (size_t x = 0; x<xUnstagDim; x++) {
			*bufptr = (fbuffer[x + xDimNow*y] + fbuffer[x + xDimNow*(y+1)])/2.0;
			bufptr++;
		}
		}
	}
}


int WRF::GetZSlice(
	varFileHandle_t *fh,
	size_t wrft,	// WRF time step
	size_t z, // The (unstaggered) vertical coordinate of the plane we want
	float *buffer
) {
	
	int rc;

	// size of one unstaggered slice
	//
	size_t slice_sz = _dimLens[0] * _dimLens[1];

	// If z dimension is not staggered simply read and return slice (possibly
	// doing horizontal interpolation)
	//
	if (! fh->thisVar.stag[2]) {

		if ( fh->thisVar.dimids.size() == 3 ) {

			rc = _ReadZSlice3D( _ncid, fh->thisVar, wrft, fh->buffer);
		}
		else {
			rc = _ReadZSlice4D( _ncid, fh->thisVar, wrft, z, fh->buffer);
		}
		if (rc<0) return(-1);

		// Do in-place horizontal interpolation of staggered grids, 
		// if necessary
		//
		if ( fh->thisVar.stag[0] || fh->thisVar.stag[1] ) {
			_InterpHorizSlice(fh->buffer, fh->thisVar);
		}

		for ( size_t l = 0 ; l < slice_sz ; l++ ) {
			buffer[l] = fh->buffer[l];
		}
		fh->z = z;

		return(0);
	}

	assert (fh->thisVar.dimids.size() == 4);


	// Need two slices. See if first is already buffered from previous 
	// invocation. If not, read it.
	//
	if (!( fh->z == z && fh->wrft == wrft)) {
		// Read a slice from the netCDF
		//
		rc = _ReadZSlice4D( _ncid, fh->thisVar, wrft, z, fh->buffer);
		if (rc<0) return(-1);

		if ( fh->thisVar.stag[0] || fh->thisVar.stag[1] ) {
			_InterpHorizSlice(fh->buffer, fh->thisVar);
		}
		fh->z = z;
		fh->wrft = wrft;
	}

	// Now read second slice
	//
	rc = _ReadZSlice4D( _ncid, fh->thisVar, wrft, z+1, buffer);
	if (rc<0) return(-1);

	if ( fh->thisVar.stag[0] || fh->thisVar.stag[1] ) {
		_InterpHorizSlice(buffer, fh->thisVar);
	}

	// Now do the vertical interpolation
	// and buffer bottom (top?) slice for next invocation
	//
	for ( size_t l = 0 ; l < slice_sz ; l++ ) {

		float tmp = buffer[l];
		buffer[l] = (buffer[l] + fh->buffer[l]) / 2.0;
		fh->buffer[l] = tmp;
	}
	fh->z = z+1;
	fh->wrft = wrft;

	return(0);
}

int WRF::WRFTimeStrToEpoch(
	const string &wrftime,
	TIME64_T *seconds,
	int daysperyear
) {

	int rc;
	int year_offset = 1900;
	const char *format = "%4d-%2d-%2d_%2d:%2d:%2d";
	struct tm ts;

	rc = sscanf(
		wrftime.c_str(), format, &ts.tm_year, &ts.tm_mon, &ts.tm_mday, 
		&ts.tm_hour, &ts.tm_min, &ts.tm_sec
	);
	if (rc != 6) {
		rc = sscanf(
			wrftime.c_str(), "%4d-%5d_%2d:%2d:%2d", &ts.tm_year,  &ts.tm_mday, 
			&ts.tm_hour, &ts.tm_min, &ts.tm_sec
		);
		ts.tm_mon=1;
		year_offset = 0;
		if (rc != 5) {
			MyBase::SetErrMsg("Unrecognized time stamp: %s", wrftime.c_str());
			return(-1);
		}
	}

	if (daysperyear == 0) {	
		ts.tm_mon -= 1;
		ts.tm_year -= year_offset;
		ts.tm_isdst = -1;	// let mktime() figure out timezone

		*seconds = MkTime64(&ts);
	}
	else { // PlanetWRF.
		*seconds = (86400*daysperyear)*ts.tm_year + (86400)*ts.tm_mday + (3600)*ts.tm_hour + (60)*ts.tm_min + ts.tm_sec;
	}	
	return(0);
}

// In this scheme of date, a time of zero seconds
// will be a date of 1970-01-01:00:00:00.  But
// in a PlantWRF file that is still the same, but
// different number of days per year.  Still
// 60 seconds per minute, 60 minutes per hours,
// and 24 hours per day. But not SI seconds.

int WRF::EpochToWRFTimeStr(
	TIME64_T seconds,
	string &str,
	int daysyear
) {
	struct tm ts;
	const char *format = "%Y-%m-%d_%H:%M:%S";
	char buf[128];

	if(daysyear == 0) {
		GmTime64_r(&seconds, &ts);

		if (strftime(buf,sizeof(buf), format, &ts) == 0) {
			MyBase::SetErrMsg("strftime(%s) : ???, format");
			return(-1);
		}
	}
	else {
		TIME64_T w_sec;
		int spd = 60 * 60 * 24; // seconds per day
		int year, day, hour, min, sec;
		year = day = hour = min = sec = 0;
		w_sec = seconds;
		year = w_sec / (spd * daysyear);
		w_sec -= year * (spd * daysyear);
		day = w_sec/spd;
		w_sec -= day * spd;
		hour = w_sec / 3600;
		w_sec -= hour * 3600;
		min = w_sec / 60;
		w_sec -= min * 60;
		sec = w_sec;
		sprintf(buf,"%04d-%05d_%02d:%02d:%02d",year,day,hour,min,sec);
	}
	str = buf;

	return(0);
}

int WRF::_GetWRFMeta(
	int ncid, // Holds netCDF file ID (in)
	float *vertExts, // Vertical extents (out)
	size_t dimLens[4], // Lengths of x, y, z, and time dimensions (out)
	string &startDate, // Place to put START_DATE attribute (out)
	string &mapProjection, //PROJ4 projection string
	vector<string> &wrfVars3d, 
	vector<string> &wrfVars2d, 
	vector<varInfo_t> &wrfVarInfo,
	vector<pair<string, double> > &gl_attrib,
	vector <pair< TIME64_T, vector <float> > > &tsExtents //Times in seconds, lat/lon corners (out)
) {
	string ErrMsgStr;
	float dx = -1.0; // Place to put DX attribute  (-1 if it's not there)
	float dy = -1.0; // Place to put DY attribute (-1 if it's not there)
	float cen_lat = 1.e30f; //place to put CEN_LAT
	float cen_lon = 1.e30f; //place to put CEN_LON
	int ndims; // Number of dimensions in netCDF
	int ngatts; // Number of global attributes
	int nvars; // Number of variables
	int daysperyear; // Used in timestr conversion.
	int xdimid; // ID of unlimited dimension (not used)
	
	char dimName[NC_MAX_NAME + 1]; // Temporary holder for dimension names

	char *buf = NULL;
	size_t bufSize = 0;
	
	wrfVars3d.clear();
	wrfVars2d.clear();
	wrfVarInfo.clear();
	gl_attrib.clear();
	tsExtents.clear();

	// Find the number of dimensions, variables, and global attributes, and check
	// the existance of the unlimited dimension (not that we need to)
	NC_ERR_READ( nc_inq(ncid, &ndims, &nvars, &ngatts, &xdimid ) );

	// Find out dimension lengths.  x <==> west_east, y <==> south_north,
	// z <==> bottom_top.  Need names, too, for finding vertical extents.
	int timeId = -1;
	int weId = -1;
	int snId = -1;
	int btId = -1;
	int wesId = -1;
	int snsId = -1;
	int btsId = -1;
	for ( int i = 0 ; i < ndims ; i++ )
	{
		NC_ERR_READ( nc_inq_dimname( ncid, i, dimName ) );
		if ( strcmp( dimName, "west_east" ) == 0 )
		{
			weId = i;
			NC_ERR_READ( nc_inq_dimlen( ncid, i, &dimLens[0] ) );
		} else if ( strcmp( dimName, "south_north" ) == 0 )
		{
			snId = i;
			NC_ERR_READ( nc_inq_dimlen( ncid, i, &dimLens[1] ) );
		} else if ( strcmp( dimName, "bottom_top" ) == 0 )
		{
			btId = i;
			NC_ERR_READ( nc_inq_dimlen( ncid, i, &dimLens[2] ) );
		} else if ( strcmp( dimName, "west_east_stag" ) == 0 )
		{
			wesId = i;
		} else if ( strcmp( dimName, "south_north_stag" ) == 0 )
		{
			snsId = i;
		} else if ( strcmp( dimName, "bottom_top_stag") == 0 )
		{
			btsId = i;
		} else if ( strcmp( dimName, "Time" ) == 0 )
		{
			NC_ERR_READ( nc_inq_dimlen( ncid, i, &dimLens[3] ) );
			timeId = i;
		}
	}
	// Make sure we found all the dimensions we need
	if ( weId < 0 || snId < 0 || btId < 0 )
	{
		ErrMsgStr.assign("Could not find expected dimensions WRF file ");
		MyBase::SetErrMsg(ErrMsgStr.c_str());
		return(-1);
	}
	for (int i=0; i<4; i++) {
		if (dimLens[i] < 1) {
			ErrMsgStr.assign("Zero-length WRF variable dimension ");
			MyBase::SetErrMsg(ErrMsgStr.c_str());
			return(-1);
		}
	}
	if ( wesId < 0 || snsId < 0 || btsId < 0 ) {
		ErrMsgStr.assign("Caution: could not find staggered dimensions in WRF file ");
		MyBase::SetErrMsg(ErrMsgStr.c_str());
		MyBase::SetErrCode(0);
	}

	// Get DX and DY, set to -1 if they aren't there.
	// Also get CEN_LAT,CEN_LON, they are needed with rotated lat/lon to 
	// determine the domain extents.
	int ncrc = nc_get_att_float( ncid, NC_GLOBAL, "DX", &dx );
	if (ncrc != NC_NOERR) dx = -1.f;
	gl_attrib.push_back(make_pair("DX",dx));
	ncrc = nc_get_att_float( ncid, NC_GLOBAL, "DY", &dy );
	if (ncrc != NC_NOERR) dy = -1.f;
	gl_attrib.push_back(make_pair("DY",dy));

	ncrc = nc_get_att_float( ncid, NC_GLOBAL, "CEN_LAT", &cen_lat );
	if (ncrc != NC_NOERR) cen_lat = 1.e30f;
	gl_attrib.push_back(make_pair("CEN_LAT",cen_lat));
	ncrc = nc_get_att_float( ncid, NC_GLOBAL, "CEN_LON", &cen_lon );
	if (ncrc != NC_NOERR) cen_lon = 1.e30f;
	gl_attrib.push_back(make_pair("CEN_LON",cen_lon));

	// If planetWRF there will be a gravity value.
 	double grav = 9.81;
	daysperyear = 0;
	float tgrav;
	ncrc = nc_get_att_float(ncid, NC_GLOBAL, "G", &tgrav);
	if (ncrc == NC_NOERR) { 
	// this is a PlanetWRF file, so we will grab the
	// other attributes values.
		grav = tgrav;
		gl_attrib.push_back(make_pair("G",grav));
		float tempval;
		ncrc = nc_get_att_float( ncid, NC_GLOBAL, "PLANET_YEAR", &tempval);
		gl_attrib.push_back(make_pair("PLANET_YEAR",tempval));
		daysperyear = (int) tempval;
		ncrc = nc_get_att_float( ncid, NC_GLOBAL, "RADIUS", &tempval);
		gl_attrib.push_back(make_pair("RADIUS",tempval));
		ncrc = nc_get_att_float( ncid, NC_GLOBAL, "P2SI", &tempval);
		gl_attrib.push_back(make_pair("P2SI",tempval));
	}

	//Build the projection string, 
	//If we can't, the projection string is length 0
	int rc = _GetProjectionString(ncid, mapProjection);
	if (rc<0) return(-1);
	
	// Get starting time stamp
	// We'd prefer SIMULATION_START_DATE, but it's okay if it doesn't exist

	size_t attlen;
	const char *start_attr = "SIMULATION_START_DATE";
	int nc_status = nc_inq_attlen(ncid, NC_GLOBAL, start_attr, &attlen);
	if (nc_status == NC_ENOTATT) {
		start_attr = "START_DATE";
		nc_status = nc_inq_attlen(ncid, NC_GLOBAL, start_attr, &attlen);
		if (nc_status == NC_ENOTATT) {
			start_attr = NULL;
			startDate.erase();
		}
	}
	if (start_attr) {
		if (bufSize < attlen+1) {
			if (buf) delete [] buf;
			buf = new char[attlen+1];
			bufSize = attlen+1;
		}

		NC_ERR_READ(nc_get_att_text( ncid, NC_GLOBAL, start_attr, buf ));

		startDate.assign(buf, attlen);
	}
		
    // build list of all dimensions found in the file
    //
	vector <ncdim_t> ncdims;
    for (int dimid = 0; dimid < ndims; dimid++) {
        ncdim_t dim;

        NC_ERR_READ(nc_inq_dim( ncid, dimid, dim.name, &dim.size));
        ncdims.push_back(dim);
    }

	// Find names of variables with appropriate dimmensions
	for ( int i = 0 ; i < nvars ; i++ ) {
		varInfo_t varinfo;
		char name[NC_MAX_NAME+1];
		NC_ERR_READ( nc_inq_varname(ncid, i, name ) );
		if (_GetVarInfo(ncid, name, ncdims, varinfo) < 0) continue;
#ifdef WIN32
//On windows, CON is not a valid vapor variable name because windows does
//not permit CON to be directory name
// CON is a 2D variable, "orographic convexity"
		if (strcmp(name, "CON") == 0) continue;
#endif

		wrfVarInfo.push_back(varinfo);

		if ((varinfo.dimids.size() == 4) && 
			(varinfo.dimids[0] ==  timeId) &&
			((varinfo.dimids[1] ==  btId) || (varinfo.dimids[1] == btsId)) &&
			((varinfo.dimids[2] ==  snId) || (varinfo.dimids[2] == snsId)) &&
			((varinfo.dimids[3] ==  weId) || (varinfo.dimids[3] == wesId))) {

			wrfVars3d.push_back(varinfo.alias);
		}
		else if ((varinfo.dimids.size() == 3) && 
			(varinfo.dimids[0] ==  timeId) &&
			((varinfo.dimids[1] ==  snId) || (varinfo.dimids[1] == snsId)) &&
			((varinfo.dimids[2] ==  weId) || (varinfo.dimids[2] == wesId))) {

			wrfVars2d.push_back(varinfo.alias);
		}
	}


	// Get vertical extents if requested
	//
	if (vertExts) {

		// Get ready to read PH and PHB
		varInfo_t phInfo; // structs for variable information
		varInfo_t phbInfo;

		// GetVarInfo takes actual file names
		//
		map <string, string>::const_iterator itr = _atypnames.find("PH");
		string varname = (itr != _atypnames.end()) ? itr->second : "PH";

		if (_GetVarInfo( ncid, varname, ncdims, phInfo) < 0) return(-1);
		if (phInfo.dimids.size() != 4) {
			MyBase::SetErrMsg("Variable %s has wrong # dims", "PH");
			return(-1);
		}
		size_t ph_slice_sz = phInfo.dimlens[phInfo.dimids.size()-1] * 
			phInfo.dimlens[phInfo.dimids.size()-2];

		itr = _atypnames.find("PHB");
		varname = (itr != _atypnames.end()) ? itr->second : "PHB";

		if (_GetVarInfo( ncid, varname, ncdims, phbInfo) < 0) return(-1);
		if (phbInfo.dimids.size() != 4) {
			MyBase::SetErrMsg("Variable %s has wrong # dims", "PHB");
			return(-1);
		}

		size_t phb_slice_sz = phbInfo.dimlens[phbInfo.dimids.size()-1] * 
			phbInfo.dimlens[phbInfo.dimids.size()-2];
			

		// Allocate memory
		float * phBuf = new float[ph_slice_sz];
		float * phbBuf = new float[phb_slice_sz];
		

		bool first = true;

		varFileHandle_t *fh_ph, *fh_phb;

		fh_ph = Open("PH");
		if (! fh_ph) return(-1);

		fh_phb = Open("PHB");
		if (! fh_phb) return(-1);

		for (size_t t = 0; t<dimLens[3]; t++) {
			float height;
			int rc;

			rc = GetZSlice(fh_ph, t, 0, phBuf);
			if (rc<0) return (rc);

			rc = GetZSlice(fh_phb, t, 0, phbBuf);
			if (rc<0) return (rc);

			if (first) {
				vertExts[0] = (phBuf[0] + phbBuf[0])/grav;
			}
			
			for ( size_t i = 0 ; i < dimLens[0]*dimLens[1] ; i++ ) {

				height = (phBuf[i] + phbBuf[i])/grav;
				// Want to find the bottom of the bottom layer and the bottom 
				// of the
				// top layer so that we can output them
				if (height < vertExts[0] ) vertExts[0] = height;
			}

			//  Read the top slices
			rc = GetZSlice(fh_ph, t, dimLens[2]-1, phBuf);
			if (rc<0) return (rc);

			rc = GetZSlice(fh_phb, t, dimLens[2]-1, phbBuf);
			if (rc<0) return (rc);

			if (first) {
				vertExts[1] = (phBuf[0] + phbBuf[0])/grav;
			}
			
			for ( size_t i = 0 ; i < dimLens[0]*dimLens[1] ; i++ ) {

				height = (phBuf[i] + phbBuf[i])/grav;
				// Want to find the bottom of the bottom layer and the bottom 
				// of the
				// top layer so that we can output them
				if (height < vertExts[1] ) vertExts[1] = height;
			}
			first = false;

		}

		Close(fh_ph);
		Close(fh_phb);

		delete [] phBuf;
		delete [] phbBuf;

	}

	// Get time stamps and lat/lon extents
	//
	varInfo_t timeInfo; // structs for variable information

	varInfo_t latInfo, lonInfo;
	bool haveLatLon = false;
	//Check to make sure we have the XLAT and XLONG variables
	int vid;
	haveLatLon = ( NC_NOERR == nc_inq_varid(ncid, "XLAT", &vid));

	if (haveLatLon) haveLatLon = ( NC_NOERR ==  nc_inq_varid(ncid, "XLONG", &vid));

	map <string,string>::const_iterator itr;
	if (haveLatLon){

		itr = _atypnames.find("XLAT");
		string varname = (itr != _atypnames.end()) ? itr->second : "XLAT";

		if(_GetVarInfo(ncid, varname, ncdims, latInfo) < 0) haveLatLon = false;

		itr = _atypnames.find("XLONG");
		varname = (itr != _atypnames.end()) ? itr->second : "XLONG";
		if(_GetVarInfo(ncid, varname, ncdims, lonInfo) < 0) haveLatLon = false;
	}


	itr = _atypnames.find("Times");
	string varname = (itr != _atypnames.end()) ? itr->second : "Times";
	if (_GetVarInfo( ncid, varname, ncdims, timeInfo) < 0) return(-1);
	if (timeInfo.dimids.size() != 2) {
		MyBase::SetErrMsg("Variable %s has wrong # dims", "Times");
		return(-1);
	}
	size_t sz = timeInfo.dimlens[0] * timeInfo.dimlens[1];
	if (bufSize < sz) {
		if (buf) delete [] buf;
		buf = new char[sz];
		bufSize = sz;
	}
	nc_status = nc_get_var_text(ncid, timeInfo.varid, buf);
	for (int i =0; i<timeInfo.dimlens[0]; i++) {
		//Try to get corner coords if we can.
		//Not needed for wrf2vdf
		vector <float> llevec;
		if (haveLatLon){ 
			float latlonexts[8];
			(void) _GetCornerCoords(ncid, i, latInfo, lonInfo, latlonexts);
			for (int j=0; j<8; j++) llevec.push_back(latlonexts[j]);
		}
			 
		string time_fmt(buf+(i*timeInfo.dimlens[1]), timeInfo.dimlens[1]);
		TIME64_T seconds;

		if (WRFTimeStrToEpoch(time_fmt, &seconds, daysperyear) < 0) return(-1);
		pair <TIME64_T, vector <float> > pr;
		pr = make_pair(seconds, llevec);
		tsExtents.push_back(pr);
	}

	if (buf) delete [] buf;

	return(0);
} // End of GetWRFMeta.

void WRF::GetWRFMeta(
	float *vertExts, // Vertical extents (out)
	size_t dimLens[4], // Lengths of x, y, z, and time dimensions (out)
	string &startDate, // Place to put START_DATE attribute (out)
	string &mapProjection, //PROJ4 projection string
	vector<string> &wrfVars3d, 
	vector<string> &wrfVars2d, 
	vector<pair<string, double> > &gl_attrib,
	vector <pair< TIME64_T, vector <float> > > &tsExtents //Times in seconds, lat/lon corners (out)
) {
	vertExts[0] = _vertExts[0];
	vertExts[1] = _vertExts[1];

	dimLens[0] = _dimLens[0];
	dimLens[1] = _dimLens[1];
	dimLens[2] = _dimLens[2];
	dimLens[3] = _dimLens[3];
	startDate = _startDate;
	mapProjection = _mapProjection;
	wrfVars3d = _wrfVars3d;
	wrfVars2d = _wrfVars2d;
	gl_attrib = _gl_attrib;
	tsExtents = _tsExtents;
}
