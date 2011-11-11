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
	//Save the topo ncid:
	topoNcId = _ncid;

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
				string vname(varname);
				addVarName(ndims-1, vname, vars2d, vars3d);
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
float* MOM::GetDepths(){
	depthsArray = 0;
	//See if we can open the ht variable in the topo file
	const string& depthVar = _atypnames["ht"];
	int varid;
	int rc = nc_inq_varid(topoNcId, depthVar.c_str(), &varid);
	if (rc != NC_NOERR) {//Not there.  
		return 0;
	}
	// Create array to hold data:
	depthsArray = new float[_dimLens[0]*_dimLens[1]];
	
	rc = nc_get_var_float(topoNcId, varid, depthsArray);
	if (rc != NC_NOERR) {//Not there.  
		return 0;
	}
	float mv = -1.e30f;
	rc = nc_get_att_float(topoNcId, varid, "missing_value", &mv);	
	
	//If this is POP, convert to meters:
	if (depthVar == "HT"){
		for (size_t i = 0; i<_dimLens[0]*_dimLens[1]; i++){
			if (depthsArray[i] != mv)
				depthsArray[i] *= 0.01;
		}
	}
	//Negate (this is height above sea level:
	for (size_t i = 0; i<_dimLens[0]*_dimLens[1]; i++){
		if (depthsArray[i] != mv )
			depthsArray[i] = -depthsArray[i];
		else depthsArray[i] = 50.f;
	}
	return depthsArray;
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
	for (int i = 0; i<geolatvars.size() && i<2; i++){
		NC_ERR_READ(nc_inq_vardimid(ncid, latvarids[i], latdimids+2*i));
		for (int j = 0; j<2; j++)
			NC_ERR_READ(nc_inq_dimlen(ncid, latdimids[j+2*i], latdimlen+(j+2*i)));
	}

	for (int i = 0; i<geolonvars.size() && i<2; i++){
		NC_ERR_READ(nc_inq_vardimid(ncid, lonvarids[i], londimids+2*i));
		for (int j = 0; j<2; j++)//get the dimension length
			NC_ERR_READ(nc_inq_dimlen(ncid, londimids[j+2*i], londimlen+(j+2*i)));
	}
	//Now read the geolat and geolon vars, to find extents and to identify T vs U grid.
	float minlat[2], maxlat[2], minlon[2], maxlon[2];
	for (int i = 0; i<geolatvars.size() && i<2; i++){
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
	for (int i = 0; i<geolonvars.size() && i<2; i++){
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
	// Save the extents and the T-grid  dimensions, converted to meters at equator
	_LonLatExts[0] = minlon[lonTgrid];
	_LonLatExts[1] = minlat[latTgrid];
	_LonLatExts[2] = maxlon[lonTgrid];
	_LonLatExts[3] = maxlat[latTgrid];
	_Exts[0] = _LonLatExts[0]*111177.;
	_Exts[1] = _LonLatExts[1]*111177.;
	_Exts[3] = _LonLatExts[2]*111177.;
	_Exts[4] = _LonLatExts[3]*111177.;
	//Note that the first dimension length is the lat
	_dimLens[0] = londimlen[2*lonTgrid+1];
	_dimLens[1] = londimlen[2*lonTgrid];
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
	float* tempVertLayers = new float[vdimsize];
	vertLayers = new float[vdimsize];
	NC_ERR_READ(nc_get_var_float(ncid, vertVarid, tempVertLayers));
	_dimLens[2] = vdimsize;
	if (_atypnames["st_ocean"] == "z_h"){//convert cm to meters
		for (int i = 0; i<vdimsize; i++) tempVertLayers[i] *= 0.01;
	}
	//negate, turn upside down:
	for (int i = 0; i<vdimsize; i++) vertLayers[i] = -tempVertLayers[vdimsize-i-1];
	delete tempVertLayers;
	_Exts[5] = 100.0;  //Positive, to include room for ocean surface, even though MOM data does not go higher than -5.0.
	_Exts[2] = vertLayers[0];  
	

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
	char varname[NC_MAX_NAME+1];
	NC_ERR_READ(nc_inq_varname(ncid, varid, varname));
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
		//Check that the other two dimensions are nlon and nlat sized (or one more)
		size_t dimlen;
		NC_ERR_READ(nc_inq_dimlen(ncid, dimids[2], &dimlen));
		if (dimlen != _dimLens[1] && dimlen != _dimLens[1]+1) return -1;
		NC_ERR_READ(nc_inq_dimlen(ncid, dimids[3], &dimlen));
		if (dimlen != _dimLens[0] && dimlen != _dimLens[0]+1) return -1;
	} else { //2D variable
		size_t dimlen;
		NC_ERR_READ(nc_inq_dimlen(ncid, dimids[1], &dimlen));
		if (dimlen != _dimLens[1] && dimlen != _dimLens[1]+1) return -1;
		NC_ERR_READ(nc_inq_dimlen(ncid, dimids[2], &dimlen));
		if (dimlen != _dimLens[0] && dimlen != _dimLens[0]+1) return -1;
	}
	int geolatvar, geolonvar;
	return GetGeoLonLatVar(ncid, varid, &geolonvar, &geolatvar);

}
//Determine what are the geolat and geolon variables associated with a variable in a netcdf file.
//Return nonzero if invalid geolon/geolat coordinates
int MOM::GetGeoLonLatVar(int ncid, int varid, int* geolon, int* geolat){
	//Check that the geolat and geolon variables are in the "coordinates" attribute
	nc_type atttype;
	size_t attlen;
	NC_ERR_READ(nc_inq_att(ncid, varid, "coordinates", &atttype, &attlen));
	char* attrVal=0;
	*geolat = -1;
	*geolon = -1;
	if (attlen > 0) {
		attrVal = new char[attlen+1];
		NC_ERR_READ(nc_get_att_text(ncid, varid, "coordinates", attrVal));
		attrVal[attlen] = '\0';
		string strAtt(attrVal);
		delete attrVal;
		size_t strPos = strAtt.find(geolatvars[0]);
		if (strPos != string::npos){
			*geolat = 0;
		} else if (geolatvars.size()>1){
			strPos = strAtt.find(geolatvars[1]);
			if (strPos != string::npos)
				*geolat = 1;
		}
		if (*geolat < 0) return -1;
		strPos = strAtt.find(geolonvars[0]);
		if (strPos != string::npos){
			*geolon = 0;
		} else if (geolonvars.size()>1){
			strPos = strAtt.find(geolonvars[1]);
			if (strPos != string::npos)
				*geolon = 1;
		}
		if (*geolon < 0) return -1;
		return 0;
	}
	return -1;
	
}
//Make a weight table for each combination of geolon/geolat variables
int MOM::MakeWeightTables(){
	WeightTables = new WeightTable*[geolatvars.size()*geolonvars.size()];
	for (int lattab = 0; lattab < geolatvars.size(); lattab++){
		for (int lontab = 0; lontab < geolonvars.size(); lontab++){
			WeightTable* wt = new WeightTable(this, lontab, lattab);
			int rc = wt->calcWeights(topoNcId);
			if (!rc) WeightTables[lontab + lattab*geolonvars.size()] = wt;
			else return rc;
			
		}
	}
	return 0;
}

			
WeightTable::WeightTable(MOM* mom, int latvar, int lonvar){
	geoLatVarName = mom->getGeoLatVars()[latvar];
	geoLonVarName = mom->getGeoLonVars()[lonvar];
	size_t dims[3];
	mom->GetDims(dims);
	nlon = dims[0];
	nlat = dims[1];
	nv = dims[2];
	const float *llexts = mom->GetLonLatExtents();
	for (int i = 0; i<4; i++) lonLatExtents[i] = llexts[i];
		
	alphas = new float[nlon*nlat];
	betas = new float[nlon*nlat];
	testValues = new float[nlon*nlat];
	cornerLons = new int[nlon*nlat];
	cornerLats = new int[nlon*nlat];
	for (int i = 0; i<nlon*nlat; i++){
		testValues[i] = 1.e30f;
		alphas[i] = - 1.f;
		betas[i] = -1.f;
	}
	geo_lat = 0;
	geo_lon = 0;
	float deltaLat = (lonLatExtents[3]-lonLatExtents[1])/nlat;
	float deltaLon = (lonLatExtents[2]-lonLatExtents[0])/nlon;
	// Calc epsilon for converging alpha,beta 
	epsilon = Max(deltaLat, deltaLon)*1.e-7;
	// Calc epsilon for checking inside rectangle
	epsRect = Max(deltaLat,deltaLon)*1.e-5;
}
//Interpolation functions, can be called after the alphas and betas arrays have been calculated.
//Following can also be used on slices of 3D data
void WeightTable::interp2D(const float* sourceData, float* resultData, float missingValue, float missMap){
	int corlon, corlat;
	for (int j = 0; j<nlat; j++){
		for (int i = 0; i<nlon; i++){
			corlon = cornerLons[i+nlon*j];
			corlat = cornerLats[i+nlon*j];
			float alpha = alphas[i+nlon*j];
			float beta = betas[i+nlon*j];
			// Special treatment if one of the points is missing value:
			if (sourceData[corlon+nlon*corlat] == missingValue && abs((1.-alpha)*(1.-beta)) > 1.e-3) {
				resultData[i+j*nlon] = missMap;
				continue;
			}
			if (sourceData[corlon+1+nlon*corlat] == missingValue && abs(alpha*(1.-beta)) > 1.e-3) {
				resultData[i+j*nlon] = missMap;
				continue;
			}
			if (sourceData[corlon+1+nlon*(corlat+1)] == missingValue && abs(alpha*beta) > 1.e-3) {
				resultData[i+j*nlon] = missMap;
				continue;
			}
			if (sourceData[corlon+nlon*(corlat+1)] == missingValue && abs((1.-alpha)*beta) > 1.e-3) {
				resultData[i+j*nlon] = missMap;
				continue;
			}
			resultData[i+j*nlon] = (1.-alpha)*(1.-beta)*sourceData[corlon+nlon*corlat] +
				alpha*(1.-beta)*sourceData[corlon+1+nlon*corlat] +
				alpha*beta*sourceData[corlon+1+nlon*(corlat+1)] +
				(1.-alpha)*beta*sourceData[corlon+nlon*(corlat+1)];
		}
	}
}
int WeightTable::calcWeights(int ncid){
	
	//	Allocate arrays for geolat and geolon.
	geo_lat = new float[nlat*nlon];
	geo_lon = new float[nlat*nlon];
	//Find the geo_lat and geo_lon variable id's in the file
	int geolatvarid, geolonvarid;
	NC_ERR_READ(nc_inq_varid (ncid, geoLatVarName.c_str(), &geolatvarid));
	NC_ERR_READ(nc_inq_varid (ncid, geoLonVarName.c_str(), &geolonvarid));
	//	Read the geolat and geolon variables into arrays.
	// Note that lon increases fastest
	NC_ERR_READ(nc_get_var_float(ncid, geolatvarid, geo_lat));
	NC_ERR_READ(nc_get_var_float(ncid, geolonvarid, geo_lon));
	float deltaLat = (lonLatExtents[3]-lonLatExtents[1])/(nlat-1);
	float deltaLon = (lonLatExtents[2]-lonLatExtents[0])/(nlon-1);
	float eps = Max(deltaLat,deltaLon)*1.e-3;
	bool wrapLon=false;
	//Check for global wrap in longitude:
	if (abs(lonLatExtents[2] - 360.0 - lonLatExtents[0])< eps) {
		deltaLon = (lonLatExtents[2]-lonLatExtents[0])/nlon;
		wrapLon=true;
	}
	//Check for approaching north pole
	bool upNorth = false;
	if (lonLatExtents[3]>80.) upNorth = false;
		
	
		
	//	Loop over (nlon/nlat) user grid vertices.  These are similar but not the same as lat and lon.
	//  Call them ulat and ulon to indicate "lat and lon" in user coordinates
	if (!upNorth) for (int ulat = 0; ulat<nlat-1; ulat++){
		for (int ulon = 0; ulon<nlon-1; ulon++){
			//	For each cell, find min and max longitude and latitude; expand by eps in all directions to define maximal ulon-ulat cell in
			// actual lon/lat coordinates
			float lon0 = geo_lon[ulon+nlon*ulat];
			float lon1 = geo_lon[ulon+1+nlon*ulat];
			float lon2 = geo_lon[ulon+nlon*(ulat+1)];
			float lon3 = geo_lon[ulon+1+nlon*(ulat+1)];
			float lat0 = geo_lat[ulon+nlon*ulat];
			float lat1 = geo_lat[ulon+1+nlon*ulat];
			float lat2 = geo_lat[ulon+nlon*(ulat+1)];
			float lat3 = geo_lat[ulon+1+nlon*(ulat+1)];
				
			float minlon = Min(Min(lon0,lon1),Min(lon2,lon3))-eps;
			float maxlon = Max(Max(lon0,lon1),Max(lon2,lon3))+eps;

			
			float minlat = Min(Min(geo_lat[ulon+nlon*ulat], geo_lat[ulon+1+nlon*ulat]),Min(geo_lat[ulon+nlon*(ulat+1)], geo_lat[ulon+1+nlon*(ulat+1)])) - eps;
			float maxlat = Max(Max(geo_lat[ulon+nlon*ulat], geo_lat[ulon+1+nlon*ulat]),Max(geo_lat[ulon+nlon*(ulat+1)], geo_lat[ulon+1+nlon*(ulat+1)])) + eps;
			int latInd = (int)((lat2-lonLatExtents[1])/deltaLat);
			int lonInd = (int)((lon2-lonLatExtents[0])/deltaLon);
			
						
			// find all the latlon grid vertices that fit near this rectangle: 
			// Add 1 to the LL corner because it is definitely below and left of the rectangle
			int latInd0 = (int)(1.+((minlat-lonLatExtents[1])/deltaLat));
			int lonInd0 = (int)(1.+((minlon-lonLatExtents[0])/deltaLon));
			//cover the edges too!
			// edgeflag is used to enable extrapolation when points are slightly outside of the grid extents
			bool edgeFlag = false;
			if (ulat == 0 && latInd0 ==1) {latInd0 = 0; edgeFlag = true;}
			if (ulon == 0 && lonInd0 == 1) {
				lonInd0 = 0; edgeFlag = true;
			}
			
			// Whereas the UR corner vertex may be inside:
			int latInd1 = (int)((maxlat-lonLatExtents[1])/deltaLat);
			int lonInd1 = (int)((maxlon-lonLatExtents[0])/deltaLon);
			
			
			if (latInd0 <0 || lonInd0 < 0) {
				continue;
			}
			if (latInd1 >= nlat || (lonInd1 > nlon)||(lonInd1 == nlon && !wrapLon)){
				continue;
			}
						
			//Loop over all the lon/lat grid vertices in the maximized cell:
			for (int qlat = latInd0; qlat<= latInd1; qlat++){
				for (int plon = lonInd0; plon<= lonInd1; plon++){
					float testLon = lonLatExtents[0]+plon*deltaLon;
					int plon1 = plon;
					if(plon == nlon) {
						assert(wrapLon);
						plon1 = 0;
					}
					float testLat = lonLatExtents[1]+qlat*deltaLat;
					float eps = testInQuad(testLon,testLat,ulon, ulat);
					if (eps < epsRect || edgeFlag){  //OK, point is inside, or close enough.
						//Check to make sure eps is smaller than previous entries:
						if (eps > testValues[plon+nlon*qlat]) continue;
						//It's OK, stuff it in the weights arrays:
						float alph, beta;
						findWeight(testLon, testLat, ulon, ulat,&alph, &beta);
						alphas[plon1+nlon*qlat] = alph;
						betas[plon1+nlon*qlat] = beta;
						testValues[plon1+nlon*qlat] = eps;
						cornerLats[plon1+nlon*qlat] = ulat;
						cornerLons[plon1+nlon*qlat] = ulon;
						
						//	printf(" lon, lat: %d, %d; alpha = %f beta = %f; test= %f, corners: %d %d\n",
						//	   plon1,qlat,alphas[plon1+nlon*qlat],betas[plon1+nlon*qlat],testValues[plon1+nlon*qlat],cornerLons[plon1+nlon*qlat],cornerLats[plon1+nlon*qlat]);
					}
				}
			}
		}
	} /*else  
		//upNorth is true 
		
		for (int ulat = 0; ulat<nlat-1; ulat++){
			float lat0 = geo_lat[ulon+nlon*ulat];
			float lat1 = geo_lat[ulon+1+nlon*ulat];
			float lat2 = geo_lat[ulon+nlon*(ulat+1)];
			float lat3 = geo_lat[ulon+1+nlon*(ulat+1)];
			if (lat1 >= 0.){  //polar project the northern hemisphere to a circle with circumference 360/2pi.  Polar coordinates are rad and thet
				float rad0 = 2.*(90.-lat0)/M_PI;
				float rad1 = 2.*(90.-lat1)/M_PI;
				float rad2 = 2.*(90.-lat2)/M_PI;
				float rad3 = 2.*(90.-lat3)/M_PI;
			}
			float eps2 = eps*(90.-lat0)/90.;
			for (int ulon = 0; ulon<nlon-1; ulon++){
				//	For each cell, find min and max longitude and latitude; expand by eps in all directions to define maximal ulon-ulat cell in
				// actual lon/lat coordinates
				float lon0 = geo_lon[ulon+nlon*ulat];
				float lon1 = geo_lon[ulon+1+nlon*ulat];
				float lon2 = geo_lon[ulon+nlon*(ulat+1)];
				float lon3 = geo_lon[ulon+1+nlon*(ulat+1)];
				
				float thet0 = lon0*M_PI/180.;
				float thet1 = lon1*M_PI/180.;
				float thet2 = lon2*M_PI/180.;
				float thet3 = lon3*M_PI/180.;
				
				//Now convert to (x,y) coordinates:
				float x0 = rad0*cos(thet0);
				float x1 = rad1*cos(thet1);
				float x0 = rad2*cos(thet2);
				float x1 = rad3*cos(thet3);
				float y0 = rad0*sin(thet0);
				float y0 = rad1*sin(thet1);
				float y0 = rad2*sin(thet2);
				float y0 = rad3*sin(thet3);
				
				//shrink eps by (90-lat0)/90
								
				float minx = Min(Min(x0,x1),Min(x2,x3))-eps2;
				float maxx = Max(Max(x0,x1),Max(x2,x3))+eps2;
				float miny = Min(Min(y0,y1),Min(y2,y3))-eps2;
				float maxy = Max(Max(y0,y1),Max(y2,y3))+eps2;
				

				int latInd = (int)((lat2-lonLatExtents[1])/deltaLat);
				int lonInd = (int)((lon2-lonLatExtents[0])/deltaLon);
				
				
				// find all the latlon grid vertices that fit near this rectangle: 
				// Add 1 to the LL corner because it is definitely below and left of the rectangle
				int latInd0 = (int)(1.+((minlat-lonLatExtents[1])/deltaLat));
				int lonInd0 = (int)(1.+((minlon-lonLatExtents[0])/deltaLon));
				//cover the edges too!
				// edgeflag is used to enable extrapolation when points are slightly outside of the grid extents
				bool edgeFlag = false;
				if (ulat == 0 && latInd0 ==1) {latInd0 = 0; edgeFlag = true;}
				if (ulon == 0 && lonInd0 == 1) {
					lonInd0 = 0; edgeFlag = true;
				}
				
				// Whereas the UR corner vertex may be inside:
				int latInd1 = (int)((maxlat-lonLatExtents[1])/deltaLat);
				int lonInd1 = (int)((maxlon-lonLatExtents[0])/deltaLon);
				
				
				if (latInd0 <0 || lonInd0 < 0) {
					continue;
				}
				if (latInd1 >= nlat || (lonInd1 > nlon)||(lonInd1 == nlon && !wrapLon)){
					continue;
				}
				
				//Loop over all the lon/lat grid vertices in the maximized cell:
				for (int qlat = latInd0; qlat<= latInd1; qlat++){
					for (int plon = lonInd0; plon<= lonInd1; plon++){
						float testLon = lonLatExtents[0]+plon*deltaLon;
						int plon1 = plon;
						if(plon == nlon) {
							assert(wrapLon);
							plon1 = 0;
						}
						float testLat = lonLatExtents[1]+qlat*deltaLat;
						float eps = testInQuad(testLon,testLat,ulon, ulat);
						if (eps < epsRect || edgeFlag){  //OK, point is inside, or close enough.
							//Check to make sure eps is smaller than previous entries:
							if (eps > testValues[plon+nlon*qlat]) continue;
							//It's OK, stuff it in the weights arrays:
							float alph, beta;
							findWeight(testLon, testLat, ulon, ulat,&alph, &beta);
							alphas[plon1+nlon*qlat] = alph;
							betas[plon1+nlon*qlat] = beta;
							testValues[plon1+nlon*qlat] = eps;
							cornerLats[plon1+nlon*qlat] = ulat;
							cornerLons[plon1+nlon*qlat] = ulon;
							
							//	printf(" lon, lat: %d, %d; alpha = %f beta = %f; test= %f, corners: %d %d\n",
							//	   plon1,qlat,alphas[plon1+nlon*qlat],betas[plon1+nlon*qlat],testValues[plon1+nlon*qlat],cornerLons[plon1+nlon*qlat],cornerLats[plon1+nlon*qlat]);
						}
					}
				}
			}
	} */
	//Check it out:
	/*
	for (int i = 0; i<nlat; i++){
		for (int j = 0; j<nlon; j++){
			if (testValues[j+nlon*i] <1.){
				continue;
			}
			else {
				if (testValues[j+nlon*i] < 1000000.) printf( "poor estimate: %f at lon, lat %d %d\n", testValues[j+nlon*i],j,i);
				else printf(" missing lon, lat: %d, %d\n",j,i);
			}
		}
	}
	 */
	return 0;				
}
float WeightTable::testInQuad(float plon, float plat, int ilon, int ilat){
	//Sides of quad are determined by lines thru the 4 points at grid corners:
	//(ilon,ilat), (ilon+1,ilat), (ilon+1,ilat+1), (ilon,ilat+1)
	//These points are mapped to 4 (lon,lat) coordinates (xi,yi) using geolon,geolat variables
	//For each pair P=(xi,yi), Q=(xi+1,yi+1) of these points, the formula
	//Fi(x,y) = (yi+1-yi)*(x-xi)-(y-yi)*(xi+1-xi) defines a function that is <0 on the in-side of the line
	//and >0 on the out-side of the line.
	//Return the maximum of the four formulas Fi evaluated at (plon,plat), or the first one that
	//exceeds epsilon
	//Bottom side:
	float F0 = (geo_lat[ilon+1+nlon*ilat]-geo_lat[ilon+nlon*ilat])*(plon - geo_lon[ilon+nlon*ilat]) -
		(plat-geo_lat[ilon+nlon*ilat])*(geo_lon[ilon+1+nlon*ilat]-geo_lon[ilon+nlon*ilat]);
	if (F0 > epsRect) return F0;
	//Right side:
	float F1 = (geo_lat[ilon+1+nlon*(ilat+1)]-geo_lat[ilon+1+nlon*ilat])*(plon - geo_lon[ilon+1+nlon*ilat]) -
		(plat-geo_lat[ilon+1+nlon*ilat])*(geo_lon[ilon+1+nlon*(ilat+1)]-geo_lon[ilon+1+nlon*ilat]);
	if (F1 > epsRect) return F1;
	//Top side:
	float F2 = (geo_lat[ilon+nlon*(ilat+1)]-geo_lat[ilon+1+nlon*(ilat+1)])*(plon - geo_lon[ilon+1+nlon*(ilat+1)]) -
		(plat-geo_lat[ilon+1+nlon*(ilat+1)])*(geo_lon[ilon+nlon*(ilat+1)]-geo_lon[ilon+1+nlon*(ilat+1)]);
	if (F2 > epsRect) return F2;
	//Left side
	float F3 = (geo_lat[ilon+nlon*ilat]-geo_lat[ilon+nlon*(ilat+1)])*(plon - geo_lon[ilon+nlon*(ilat+1)]) -
		(plat-geo_lat[ilon+nlon*(ilat+1)])*(geo_lon[ilon+nlon*ilat]-geo_lon[ilon+nlon*(ilat+1)]);
	if (F3 > epsRect) return F3;
	return Max(Max(F0,F1),Max(F2,F3));
}

void WeightTable::findWeight(float plon, float plat, int ilon, int ilat, float* alpha, float* beta){
	//use iteration to determine alpha and beta that interpolates plon, plat from the user grid
	//corner associated with ilon and ilat.
	//start with prevAlpha=0, prevBeta=0.  
	float th[4],ph[4];
	th[0] = geo_lon[ilon+nlon*ilat];
	ph[0] = geo_lat[ilon+nlon*ilat];
	th[1] = geo_lon[ilon+1+nlon*ilat];
	ph[1] = geo_lat[ilon+1+nlon*ilat];
	th[2] = geo_lon[ilon+1+nlon*(ilat+1)];
	ph[2] = geo_lat[ilon+1+nlon*(ilat+1)];
	th[3] = geo_lon[ilon+nlon*(ilat+1)];
	ph[3] = geo_lat[ilon+nlon*(ilat+1)];
	float alph = 0.f, bet = 0.f;  //first guess
	float newalpha, newbeta;
	int iter;
	for (iter = 0; iter < 10; iter++){
		newalpha = NEWA(alph, bet, plon, plat, th, ph);
		newbeta = NEWB(alph, bet, plon, plat, th, ph);
		//float err = errP(plon, plat, newalpha, newbeta,th,ph);
		if (errP(plon, plat, newalpha, newbeta,th,ph) < 1.e-9) break;
		alph = newalpha;
		bet = newbeta;
	}
	if(iter > 10){
		printf(" Weight nonconvergence, errp: %f , alpha, beta: %f %f\n",errP(plon, plat, newalpha, newbeta,th,ph), newalpha, newbeta);
	}
	//float thetaInterp = finterp(newalpha,newbeta,th);
	//float phiInterp = finterp(newalpha,newbeta,ph);
	*alpha = newalpha;
	*beta = newbeta;
	
}

//Determine the index in time array that corresponding to a MOM date.
//Requires a vector of times in seconds after 1970, such as those in Metadata
//MOM dates are in months relative to the simulation start time.
size_t MOM::GetVDCTimeStep(double momTime, const vector<double>& times,  double tol){
	//convert everything to seconds since 01/01/70:
	if (startTimeDouble <= -1.e30) return (size_t)(-1);
	momTime *= (24.*60.*60.);
	momTime += startTimeDouble;
	tol *= (24.*60.*60.);
	
	size_t mints = 0, maxts = times.size()-1;
	//make sure we are in right interval:
	if (momTime <= times[0]-tol) return -1;
	if (momTime >= times[maxts]+tol) return -1;
	//Do binary search
	
	size_t mid = (maxts+mints)/2;
	int ntries = 0;
	while (mints < maxts-1){
		if (times[mid]< momTime)
			mints = mid;
		else if (times[mid] != momTime)
			maxts = mid;
		else return(mid);
		mid = (maxts+mints)/2;
		if(ntries++ > 100) assert(0);
	}
	if(abs(times[mid]-momTime) < tol) return mid;
	else if (abs(times[mints]-momTime) < tol) return mints;
	else if (abs(times[maxts]-momTime) < tol) return maxts;
	else return (size_t)-1;
	
}


