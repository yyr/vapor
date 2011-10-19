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
#include <vapor/MOM.h>
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

int MOM::_MOM(
	const string &toponame, const map <string, string> &names, const vector<string>& vars2d, const vector<string>& vars3d
) {

	_ncid = 0;

	//
	// Deal with non-standard names for required variables
	//
	map <string, string>::const_iterator itr;
	_atypnames["time"] = ((itr=names.find("time"))!=names.end()) ? itr->second : "time";
	_atypnames["ht"] = ((itr=names.find("ht"))!=names.end()) ? itr->second : "ht";
	_atypnames["st_ocean"] = ((itr=names.find("st_ocean"))!=names.end()) ? itr->second : "st_ocean";
	_atypnames["st_edges_ocean"] = ((itr=names.find("st_edges_ocean"))!=names.end()) ? itr->second : "st_edges_ocean";
	_atypnames["sw_ocean"] = ((itr=names.find("sw_ocean"))!=names.end()) ? itr->second : "sw_ocean";
	_atypnames["sw_edges_ocean"] = ((itr=names.find("sw_edges_ocean"))!=names.end()) ? itr->second : "sw_edges_ocean";

	_Exts[2] = _Exts[5] = 0.0;
	_dimLens[0] = _dimLens[1] = _dimLens[2] = _dimLens[3] = 1;

	add2dVars = add3dVars = true;
	
	_momTimes.clear();
	if (vars2d.size()>0){//Don't add new vars if vars are already specified
		add2dVars = false;
	}
	if (vars3d.size()>0){
		add3dVars = false;
	}

	// Open netCDF topo file and check for failure
	NC_ERR_READ( nc_open( toponame.c_str(), NC_NOWRITE, &_ncid ));

	//Look in the topo file for geolat and geolon variables, get the extents of t-grid variables.
	//Check for the existence of a vertical dimension and its corresponding variable.  Save extents in _vertExts.
	//returns 0 on success.

	int rc = _GetMOMTopo(_ncid );

	return(rc);
}

MOM::MOM(const string &toponame, const vector<string>& vars2d, const vector<string>& vars3d) {
	map <string, string> atypnames;

	atypnames["time"] = "time";
	atypnames["ht"] = "ht";
	atypnames["st_ocean"] = "st_ocean";
	atypnames["st_edges_ocean"] = "st_edges_ocean";
	atypnames["sw_ocean"] = "sw_ocean";
	atypnames["sw_edges_ocean"] = "sw_edges_ocean";

	(void) _MOM(toponame, atypnames, vars2d, vars3d);
}

MOM::MOM(const string &momname, const map <string, string> &atypnames, const vector<string>& vars2d, const vector<string>& vars3d) {

	(void) _MOM(momname, atypnames, vars2d, vars3d);
}

MOM::~MOM() {
	// Close the MOM file
	(void) nc_close( _ncid );
}

MOM::varFileHandle_t *MOM::Open(
	const string &varname
) {
	//
	// Make sure we have a record for the variable
	//
	bool found = false;
	int index = -1;
	for (int i=0; i<_momVarInfo.size(); i++) {
		if (varname.compare(_momVarInfo[i].alias) == 0) {
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
	fhptr->momt = -1;	// Invalid time step
	fhptr->thisVar = _momVarInfo[index];

	return(fhptr);
}

int MOM::Close(varFileHandle_t *fh) {

	if (fh->buffer) delete [] fh->buffer;
	delete fh;
	return(0);
}
// Read a data file.  Add any new variables found to current list of discovered variables.
// Add any timesteps found to timestep list, if any variables are found.  Return nonzero if no variables found.
int MOM::addFile(const string& datafile, float extents[6], vector<string>&vars2d, vector<string>&vars3d){
	// Open netCDF file and check for failure
	int ncid;
	NC_ERR_READ( nc_open( datafile.c_str(), NC_NOWRITE, &ncid ));

	//Process time:  
	startTimeDouble = -1.e30;  //Invalid time
	//	find the time dimension and the time variable, read all its values
	string timename;
	int timedimid;
	size_t timelen;
	char nctimename[NC_MAX_NAME+1];
	NC_ERR_READ(nc_inq_unlimdim(ncid, &timedimid));
	NC_ERR_READ(nc_inq_dim(ncid, timedimid, nctimename, &timelen));
	//Check that the time has a valid name:
	if (strcmp("time",nctimename) == 0) timename = "time";
	else if (strcmp("Time",nctimename) == 0) timename = "Time";
	else if (strcmp("TIME",nctimename) == 0) timename = "TIME";
	else {
		MyBase::SetErrMsg("No time dimension in file %s , skipping",datafile.c_str());
		MyBase::SetErrCode(0);
		return -2;
	}
	
	//Find and read the time variable.  It must be the (only) unlimited dimension
	int timevarid;
	NC_ERR_READ(nc_inq_varid(ncid, timename.c_str(), &timevarid));
	
	size_t timestart[1] = {0};
	size_t timecount[1];
	timecount[0] = timelen;
	double* fileTimes = new double[timelen];
	NC_ERR_READ(nc_get_vara_double(ncid, timevarid, timestart, timecount, fileTimes));
	bool timesInserted = false;

	//Check the units attribute of the time variable.  If it starts with "days since" then construct a WRF-style time stamp
	//from the next two tokens in that attribute.
	nc_type atttype;
	size_t attlen;
	NC_ERR_READ(nc_inq_att(ncid, timevarid, "units", &atttype, &attlen));
	char* attrVal=0;
	char dateTimeStamp[20];
	if (attlen > 0) {
		attrVal = new char[attlen+1];
		NC_ERR_READ(nc_get_att_text(ncid, timevarid, "units", attrVal));
		attrVal[attlen] = '\0';
		string strAtt(attrVal);
		size_t strPos = strAtt.find("days since");
		int y,m,d,h,mn,s;
		if (strPos != string::npos){
			//See if we can extract the subsequent two tokens, and that they are of the format
			// yyyy-mm-dd and hh:mm:ss
			
			string substr = strAtt.substr(10+strPos, 20);
			sscanf(substr.c_str(),"%4d-%2d-%2d %2d:%2d:%2d", &y, &m, &d, &h, &mn, &s);
			//Create a WRF-style date-time stamp
			sprintf(dateTimeStamp,"%04d-%02d-%02d_%02d:%02d:%02d",y,m,d,h,mn,s);
			dateTimeStamp[19] = '\0';
			startTimeStamp = std::string(dateTimeStamp);
			TIME64_T startTimeSeconds;
			int rc = WRF::WRFTimeStrToEpoch(startTimeStamp, &startTimeSeconds);
			if (!rc) startTimeDouble = (double)startTimeSeconds;

		}
		
	}

	//Get the min and max values of the vertical dimension, if it is defined
	// in this file.  It should agree with current values of vertexts

	//Loop over all variables in the file.
	//For each 3D or 4D double or float variable, 
	//  check if it uses one of the valid time and (for 4D) vertical dimension names.
	//  See if the coordinates attribute contains the geolat and geolon coordinate names (either T or U grid)
	//  for validity check that the variable dimension lengths are either the same, or one more, than the
	//  saved dimensions.
	//  If it passes this test then:
	//	  If the appropriate add2DVars or add3DVars flag is true then add the varname to the list
	//    in any case insert all the times into the times list
	//
	// Find the number of variables
	int nvars;
	char varname[NC_MAX_NAME+1];
	NC_ERR_READ( nc_inq_nvars(ncid, &nvars ) );
	for (int i = 0; i<nvars; i++){
		nc_type vartype;
		NC_ERR_READ( nc_inq_vartype(ncid, i,  &vartype) );
		if (vartype != NC_FLOAT && vartype != NC_DOUBLE) continue;
		int ndims;
		NC_ERR_READ(nc_inq_varndims(ncid, i, &ndims));
		if (ndims <3 || ndims >4) continue;
		//Check that the first dimension is time:
		int dimids[4];
		NC_ERR_READ(nc_inq_vardimid(ncid, i, dimids));
		if (dimids[0] != timedimid) continue;
		if( 0==varIsValid(ncid, ndims, i)){
			NC_ERR_READ(nc_inq_varname(ncid, i, varname));
			if ((ndims ==3 && add2dVars) || (ndims == 4 && add3dVars)){
				addVarName(ndims-1, string(varname), vars2d, vars3d);
			}
			if (!timesInserted){
				addTimes(timelen, fileTimes);
				timesInserted = true;
			}
		}
	}
	delete fileTimes;
	for (int i = 0; i<6; i++) extents[i] = _Exts[i];
	return 0;
}




// Gets info about a  variable and stores that info in thisVar.
int	MOM::_GetVarInfo(
	int ncid, // ID of the file we're reading
	string momvname,	// name of variable as it appears in file
	const vector <ncdim_t> &ncdims,
	varInfo_t & thisVar // Variable info
) {

	thisVar.momvname = momvname;
	thisVar.alias = momvname;

    map <string, string>::const_iterator itr;
	for (itr = _atypnames.begin(); itr != _atypnames.end(); itr++) {
		if (momvname.compare(itr->second) == 0) {
			thisVar.alias = itr->first;
		}
	} 

	int nc_status = nc_inq_varid(ncid, thisVar.momvname.c_str(), &thisVar.varid);
	if (nc_status != NC_NOERR) {
		MyBase::SetErrMsg(
			"Variable %s not found in netCDF file", thisVar.momvname.c_str()
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
int MOM::_ReadZSlice4D(
	int ncid, // ID of the netCDF file
	const varInfo_t & thisVar, // Struct for the variable we want
	size_t momT, // The MOM time step we want
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

	start[thisVar.dimids.size()-4] = momT;
	start[thisVar.dimids.size()-3] = z;
	count[thisVar.dimids.size()-2] = thisVar.dimlens[thisVar.dimids.size()-2];
	count[thisVar.dimids.size()-1] = thisVar.dimlens[thisVar.dimids.size()-1];

	NC_ERR_READ( nc_get_vara_float(ncid, thisVar.varid, start, count, fbuffer));

	return(0);
}

// Reads a single slice from a 3D (time + space) variable 
int MOM::_ReadZSlice3D(
	int ncid, // ID of the netCDF file
	const varInfo_t & thisVar, // Struct for the variable we want
	size_t momT, // The MOM time step we want
	float * fbuffer // Buffer we're going to store slice in
) {

	size_t start[NC_MAX_DIMS]; // The point from which we start reading netCDF data
	size_t count[NC_MAX_DIMS]; // What interval to read the data

	// Initialize the count and start arrays for extracting slices from the data:
	for (int i = 0; i<thisVar.dimids.size(); i++){
		start[i] = 0; // Start reading in a corner
		count[i] = 1; // Read every point (changes later)
	}

	start[thisVar.dimids.size()-3] = momT;
	count[thisVar.dimids.size()-2] = thisVar.dimlens[thisVar.dimids.size()-2];
	count[thisVar.dimids.size()-1] = thisVar.dimlens[thisVar.dimids.size()-1];

	NC_ERR_READ( nc_get_vara_float(ncid, thisVar.varid, start, count, fbuffer));

	return(0);
}



void MOM::_InterpHorizSlice(
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


int MOM::GetZSlice(
	varFileHandle_t *fh,
	size_t momt,	// MOM time step
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

			rc = _ReadZSlice3D( _ncid, fh->thisVar, momt, fh->buffer);
		}
		else {
			rc = _ReadZSlice4D( _ncid, fh->thisVar, momt, z, fh->buffer);
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
	if (!( fh->z == z && fh->momt == momt)) {
		// Read a slice from the netCDF
		//
		rc = _ReadZSlice4D( _ncid, fh->thisVar, momt, z, fh->buffer);
		if (rc<0) return(-1);

		if ( fh->thisVar.stag[0] || fh->thisVar.stag[1] ) {
			_InterpHorizSlice(fh->buffer, fh->thisVar);
		}
		fh->z = z;
		fh->momt = momt;
	}

	// Now read second slice
	//
	rc = _ReadZSlice4D( _ncid, fh->thisVar, momt, z+1, buffer);
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
	fh->momt = momt;

	return(0);
}



int MOM::_GetMOMTopo(
	int ncid // Holds netCDF file ID (in)
	
) {
	string ErrMsgStr;
	
	
	int nvars; // Number of variables
	
	// Find the number of variables
	NC_ERR_READ( nc_inq_nvars(ncid, &nvars ) );

	// Loop through all the variables in the file, looking for 2D float variables.
	// For each such variable check its attributes for the following:
		// longitude or latitude must appear in long_name
		// degree_north, degrees_north, degree_N, degrees_N, degreeN, or degreesN must be units attribute of latitude
		// degree_east, degrees_east, degree_E, degrees_E, degreeE, or degreesE must be units attribute of longitude
	// When such a variable is found, add it to geolatvars and geolonvars
	// No more than two such variables should be found.  
	// Find the min and max value of each lat and each lon variable, use that to identify the T-grid.
	geolatvars.clear();
	geolonvars.clear();
	vector<int> latvarids;
	vector<int> lonvarids;
	char varname[NC_MAX_NAME+1];
	for (int i = 0; i<nvars; i++){
		nc_type vartype;
		NC_ERR_READ( nc_inq_vartype(ncid, i,  &vartype) );
		if (vartype != NC_FLOAT) continue;
		int ndims;
		NC_ERR_READ(nc_inq_varndims(ncid, i, &ndims));
		if (ndims != 2) continue;
		int geocode = testVarAttribs(ncid, i);
		if (!geocode) continue;
		
		NC_ERR_READ(nc_inq_varname(ncid, i, varname));
		if (geocode == 1){ //geo-lat
			//push geolat var name 
			geolatvars.push_back(varname);
			latvarids.push_back(i);
		} else { //geo-lon
			//push geolon var name 
			geolonvars.push_back(varname);
			lonvarids.push_back(i);
		}
	}
	//Check what was found
	if (geolatvars.size() == 0 || geolonvars.size() == 0){
		MyBase::SetErrMsg(" geo-lat or geo-lon variable not found in topo file");
		return -1;
	}
	if (geolatvars.size() > 2 || geolonvars.size() > 2){
		//Warning
		MyBase::SetErrMsg(" More than two geo-lat or geo-lon variables found in topo file");
		MyBase::SetErrCode(0);
	}
	// Read the (first two) geolat and geolon variables, determine the extents.
	// First determine the two dimensions of the geolat and geolon vars
	int londimids[4], latdimids[4];
	size_t londimlen[4], latdimlen[4];
	for (int i = 0; i<geolatvars.size(), i<2; i++){
		NC_ERR_READ(nc_inq_vardimid(ncid, latvarids[i], latdimids+2*i));
		for (int j = 0; j<2; j++)
			NC_ERR_READ(nc_inq_dimlen(ncid, latdimids[j+2*i], latdimlen+(j+2*i)));
	}

	for (int i = 0; i<geolonvars.size(), i<2; i++){
		NC_ERR_READ(nc_inq_vardimid(ncid, lonvarids[i], londimids+2*i));
		for (int j = 0; j<2; j++)//get the dimension length
			NC_ERR_READ(nc_inq_dimlen(ncid, londimids[j+2*i], londimlen+(j+2*i)));
	}
	//Now read the geolat and geolon vars, to find extents and to identify T vs U grid.
	float minlat[2], maxlat[2], minlon[2], maxlon[2];
	for (int i = 0; i<geolatvars.size(), i<2; i++){
		float * buf = new float[latdimlen[2*i]*latdimlen[2*i+1]];
		NC_ERR_READ(nc_get_var_float(ncid, latvarids[i], buf));
		//Identify the fill_value
		float fillval;
		NC_ERR_READ(nc_inq_var_fill(ncid, latvarids[i], 0, &fillval));
		float maxval = -1.e30f;
		float minval = 1.e30f;
		for (int j = 0; j<latdimlen[2*i]*latdimlen[2*i+1]; j++){
			if (buf[j] == fillval) continue;
			if (buf[j]<minval) minval = buf[j];
			if (buf[j]>maxval) maxval = buf[j];
		}
		minlat[i] = minval;
		maxlat[i] = maxval;
		delete buf;
	}
	for (int i = 0; i<geolonvars.size(), i<2; i++){
		float * buf = new float[londimlen[2*i]*londimlen[2*i+1]];
		NC_ERR_READ(nc_get_var_float(ncid, lonvarids[i], buf));
		//Identify the fill_value
		float fillval;
		NC_ERR_READ(nc_inq_var_fill(ncid, lonvarids[i], 0, &fillval));
		float maxval = -1.e30f;
		float minval = 1.e30f;
		for (int j = 0; j<londimlen[2*i]*londimlen[2*i+1]; j++){
			if (buf[j] == fillval) continue;
			if (buf[j]<minval) minval = buf[j];
			if (buf[j]>maxval) maxval = buf[j];
		}
		minlon[i] = minval;
		maxlon[i] = maxval;
		delete buf;
	}
	// Determine which is the T-grid.  The U-grid has larger lat/lon coordinates
	int latTgrid = 0;
	int lonTgrid = 0;
	if ((geolatvars.size()>1) && (minlat[1]<minlat[0])) latTgrid = 1;
	if ((geolonvars.size()>1) && (minlon[1]<minlon[0])) lonTgrid = 1;
	//Warn if grid mins are not different:
	if ((geolatvars.size()>1) && (minlat[1]==minlat[0])) {
		MyBase::SetErrMsg(" geolat T and U grid have same minimum: %f", minlat[0]);
		MyBase::SetErrCode(0);
	}
	if ((geolonvars.size()>1) && (minlon[1]==minlon[0])) {
		MyBase::SetErrMsg(" geolon T and U grid have same minimum: %f", minlon[0]);
		MyBase::SetErrCode(0);
	}
	// Save the extents and the T-grid  dimensions
	_Exts[0] = minlon[lonTgrid];
	_Exts[1] = minlat[latTgrid];
	_Exts[3] = maxlon[lonTgrid];
	_Exts[4] = maxlat[latTgrid];
	_dimLens[0] = londimlen[2*lonTgrid];
	_dimLens[1] = londimlen[2*lonTgrid+1];
	if ((londimlen[2*lonTgrid] != latdimlen[2*latTgrid]) || (londimlen[2*lonTgrid+1] != latdimlen[2*latTgrid+1])) {
		MyBase::SetErrMsg(" geolon and geolat dimensions differ");
		return -1;
	}
	//If the T-grid is the second grid, reorder the grid varnames
	if (lonTgrid != 0){
		string lonvar = geolonvars[1];
		geolonvars[1] = geolonvars[0];
		geolonvars[0] = lonvar;
	}
	if (latTgrid != 0){
		string latvar = geolatvars[1];
		geolatvars[1] = geolatvars[0];
		geolatvars[0] = latvar;
	}


	//Now get the vertical dimensions and extents.
	//See if there is a variable z_t or st_ocean, or if an alternate name has been specified.
	//If such a variable is in the topo file, find its size and its max and min value.
	string altname = _atypnames["st_ocean"];
	int vertVarid;
	if (altname != "st_ocean"){
		int rc = nc_inq_varid(ncid, altname.c_str(), &vertVarid);
		if (rc != NC_NOERR) //Not there.  quit.
			return(0);
	} else { //check standard names
		int rc = nc_inq_varid(ncid, "st_ocean", &vertVarid);
		if (rc != NC_NOERR) {//Not there.  try other name:
			rc = nc_inq_varid(ncid, "z_h", &vertVarid);
			if (rc != NC_NOERR) // not there, quit
				return (0);
			//OK, z_h is the name
			_atypnames["st_ocean"] = "z_h";
		}
	}
	//Now we can get the vertical dimension and extents:
	//Find the dimension length of the variable.  Assume it's a 1D variable!
	int vdimid; 
	size_t vdimsize;
	NC_ERR_READ(nc_inq_vardimid(ncid, vertVarid, &vdimid));
	NC_ERR_READ(nc_inq_dimlen(ncid, vdimid, &vdimsize));
	//read the variable, and find the top and bottom of the data
	float* vertLayers = new float[vdimsize];
	NC_ERR_READ(nc_get_var_float(ncid, vertVarid, vertLayers));
	_dimLens[2] = vdimsize;
	_Exts[2] = -vertLayers[vdimsize-1];
	_Exts[5] = -vertLayers[0];
	if (_atypnames["st_ocean"] == "z_h"){//convert cm to meters
		_Exts[2]*= 0.01f;
		_Exts[5]*= 0.01f;
	}
	delete [] vertLayers;

	//Check for ht, sw_edges_ocean, sw_ocean, st_edges_ocean.  If these variables are not in the file, then modify
	//the atypnames to map to an empty string.
	altname = _atypnames["ht"];
	int checkid;
	if (altname != "ht"){
		int rc = nc_inq_varid(ncid, altname.c_str(), &checkid);
		if (rc != NC_NOERR) //Not there.  
			_atypnames["ht"] = "";
	} else { //check standard names
		int rc = nc_inq_varid(ncid, "ht", &checkid);
		if (rc != NC_NOERR) {//Not there.  try POP name:
			rc = nc_inq_varid(ncid, "HT", &checkid);
			if (rc != NC_NOERR) // not there, remove from list 
				_atypnames["ht"] = "";
			//OK, HT is the name
			else _atypnames["ht"] = "HT";
		}
	}
	altname = _atypnames["sw_edges_ocean"];
	if (altname != "sw_edges_ocean"){
		int rc = nc_inq_varid(ncid, altname.c_str(), &checkid);
		if (rc != NC_NOERR) //Not there.  
			_atypnames["sw_edges_ocean"] = "";
	} else { //check standard names
		int rc = nc_inq_varid(ncid, "sw_edges_ocean", &checkid);
		if (rc != NC_NOERR) {//not there, remove from list 
				_atypnames["sw_edges_ocean"] = "";
		}
	}
	altname = _atypnames["sw_ocean"];
	if (altname != "sw_ocean"){
		int rc = nc_inq_varid(ncid, altname.c_str(), &checkid);
		if (rc != NC_NOERR) //Not there.  
			_atypnames["sw_ocean"] = "";
	} else { //check standard names
		int rc = nc_inq_varid(ncid, "sw_ocean", &checkid);
		if (rc != NC_NOERR) {//Not there.  try POP name:
			rc = nc_inq_varid(ncid, "z_w", &checkid);
			if (rc != NC_NOERR) // not there, remove from list 
				_atypnames["sw_ocean"] = "";
			//OK, z_w is the name
			else _atypnames["sw_ocean"] = "z_w";
		}
	}
	altname = _atypnames["st_edges_ocean"];
	if (altname != "st_edges_ocean"){
		int rc = nc_inq_varid(ncid, altname.c_str(), &checkid);
		if (rc != NC_NOERR) //Not there.  
			_atypnames["st_edges_ocean"] = "";
	} else { //check standard names
		int rc = nc_inq_varid(ncid, "st_edges_ocean", &checkid);
		if (rc != NC_NOERR) {//not there, remove from list 
				_atypnames["st_edges_ocean"] = "";
		}
	}
	return(0);
} // End of _GetMOMTopo.


//Test if variable has right attributes to be a geolat or geolon variable.
// returns 1 for geolat, 2 for geolon
int MOM::testVarAttribs(int ncid, int varid){
	char* latUnits[] = {"degree_north","degrees_north","degree_N","degrees_N","degreeN","degreesN"};
	char* lonUnits[] = {"degree_east","degrees_east","degree_E","degrees_E","degreeE","degreesE"};
	int latlon = 0;
	
	//First look for the "long_name" attribute:
	
	nc_type atttype;
	size_t attlen;
	NC_ERR_READ(nc_inq_att(ncid, varid, "long_name", &atttype, &attlen));
	char* attrVal=0;
	if (attlen > 0) {
		attrVal = new char[attlen+1];
		NC_ERR_READ(nc_get_att_text(ncid, varid, "long_name", attrVal));
		attrVal[attlen] = '\0';
		string strAtt(attrVal);
		size_t strPos = strAtt.find("latitude");
		if (strPos != string::npos){
			latlon = 1;
		}
		strPos = strAtt.find("longitude");
		if (strPos != string::npos){
			latlon = 2;
		}
	}
	if (attrVal) delete attrVal;
	
	if (!latlon) return 0;
	//Now verify that the units attribute is OK
	attrVal=0;
	NC_ERR_READ(nc_inq_att(ncid, varid, "units", &atttype, &attlen));
	bool unitsOK = false;
	if (attlen > 0) {
		attrVal = new char[attlen+1];
		NC_ERR_READ(nc_get_att_text(ncid, varid, "units", attrVal));
		attrVal[attlen] = '\0';
		string strAtt(attrVal);
		if (latlon == 1) {
			for (int j = 0; j<6; j++){
				size_t strPos = strAtt.find(latUnits[j]);
				if (strPos != string::npos) {
					unitsOK = true;
					break;
				}
			}
		}
		else if (latlon == 2) {
			for (int j = 0; j<6; j++){
				size_t strPos = strAtt.find(lonUnits[j]);
				if (strPos != string::npos) {
					unitsOK = true;
					break;
				}
			}
		}
		if (attrVal) delete attrVal;
		if (unitsOK) return (latlon);
		else return 0;
	}
	return 0;
}
void MOM::addTimes(int numtimes, double times[]){
	for (int i = 0; i< numtimes; i++){
		int k;
		for (k = 0; k < _momTimes.size(); k++){
			if (_momTimes[k] == times[i]) continue;
		}
		if (k >= _momTimes.size()) _momTimes.push_back(times[i]);
	}

}
void MOM::addVarName(int dim, string& vname, vector<string>&vars2d, vector<string>&vars3d){
	if (dim == 2){
		for (int i = 0; i< vars2d.size(); i++){
			if (vars2d[i] == vname) return;
		}
		vars2d.push_back(vname);
		return;
	}
	else{
		for (int i = 0; i< vars3d.size(); i++){
			if (vars3d[i] == vname) return;
		}
		vars3d.push_back(vname);
		return;
	}
}
//  if variable is 4D make sure it uses a valid vertical dimension name.
//  See if the coordinates attribute contains the geolat and geolon coordinate names (either T or U grid)
//  for validity check that the variable dimension lengths are either the same, or one more, than the
//  saved dimensions.
int MOM::varIsValid(int ncid, int ndims, int varid){
	
	int dimids[4];
	NC_ERR_READ(nc_inq_vardimid(ncid, varid, dimids));
	if (ndims == 4){
		//Find the 2nd dimension name:
		char dimname[NC_MAX_NAME+1];
		NC_ERR_READ(nc_inq_dimname(ncid, dimids[1], dimname));
		if ((_atypnames["st_ocean"] != dimname) &&
			(_atypnames["st_edges_ocean"] != dimname) &&
			(_atypnames["sw_edges_ocean"] != dimname) &&
			(_atypnames["sw_ocean"] != dimname) ){
				return -1;
		}
	}
	//Check that the other two dimensions are nlon and nlat sized (or one more)
	size_t dimlen;
	NC_ERR_READ(nc_inq_dimlen(ncid, dimids[2], &dimlen));
	if (dimlen != _dimLens[0] && dimlen != _dimLens[0]+1) return -1;
	NC_ERR_READ(nc_inq_dimlen(ncid, dimids[3], &dimlen));
	if (dimlen != _dimLens[1] && dimlen != _dimLens[1]+1) return -1;

	//Check that the geolat and geolon variables are in the "coordinates" attribute
	nc_type atttype;
	size_t attlen;
	NC_ERR_READ(nc_inq_att(ncid, varid, "coordinates", &atttype, &attlen));
	char* attrVal=0;
	if (attlen > 0) {
		attrVal = new char[attlen+1];
		NC_ERR_READ(nc_get_att_text(ncid, varid, "coordinates", attrVal));
		attrVal[attlen] = '\0';
		string strAtt(attrVal);
		delete attrVal;
		bool latok = false, lonok = false;
		size_t strPos = strAtt.find(geolatvars[0]);
		if (strPos != string::npos){
			latok = true;
		} else if (geolatvars.size()>1){
			strPos = strAtt.find(geolatvars[1]);
			if (strPos != string::npos)
				latok = true;
		}
		if (!latok) return -1;
		strPos = strAtt.find(geolonvars[0]);
		if (strPos != string::npos){
			lonok = true;
		} else if (geolonvars.size()>1){
			strPos = strAtt.find(geolonvars[1]);
			if (strPos != string::npos)
				lonok = true;
		}
		if (!lonok) return -1;
		return 0;
		if (attrVal) delete attrVal;

	}
	return -1;
}