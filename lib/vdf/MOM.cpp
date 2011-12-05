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
#define _USE_MATH_DEFINES
#include <math.h>
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

	//Get the time stamp from the file:
	extractStartTime(ncid, timevarid);
	

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
int MOM::extractStartTime(int ncid, int timevarid){
	//Check the units attribute of the time variable.  If it exists and it starts with "days since" then construct a WRF-style time stamp
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
			else return -1;
		}
		return 0;
	} else return -1;
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



int MOM::_GetMOMTopo(
	int ncid // Holds netCDF file ID (in)
	
) {
	string ErrMsgStr;
	
	
	int nvars; // Number of variables
	
	// Find the number of variables
	NC_ERR_READ( nc_inq_nvars(ncid, &nvars ) );

	// Loop through all the variables in the file, looking for 2D float or double variables.
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
		if (vartype != NC_FLOAT && vartype != NC_DOUBLE) continue;
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
	//The T grid has smaller lower-left coordinates.
	//The T-grid will be used as the basis for the VAPOR extents.
	float minlat[2], maxlat[2], minlon[2], maxlon[2], lllat[2], lllon[2];
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
		lllat[i] = buf[0];
		delete buf;
	}
	//Longitude is more tricky because it may "wrap".  To make the longitude mapping monotonic,
	//Find the largest longitude at the maximum user-longitude coordinate (i.e. along the right edge of the mapping.
	//Every longitude that is larger than that max lon gets 360 subtracted
	
	for (int i = 0; i<geolonvars.size() && i<2; i++){
		int ulondim = londimlen[2*i+1];
		int ulatdim = londimlen[2*i];
		float* buf = getMonotonicLonData(ncid, geolonvars[i].c_str(), ulondim, ulatdim);
		if (!buf) return -1;
		float maxval = -1.e30f;
		float minval = 1.e30f;
		for (int j = 0; j<ulondim*ulatdim; j++){
			if (buf[j]<minval) minval = buf[j];
			if (buf[j]>maxval) maxval = buf[j];
		}
		minlon[i] = minval;
		maxlon[i] = maxval;
		lllon[i] = buf[0];
		delete buf;
	}
	// Determine which is the T-grid.  The U-grid has larger lower-left lat/lon coordinates
	int latTgrid = 0;
	int lonTgrid = 0;
	if ((geolatvars.size()>1) && (lllat[1]<lllat[0])) latTgrid = 1;
	if ((geolonvars.size()>1) && (lllon[1]<lllon[0])) lonTgrid = 1;
	//Warn if grid mins are not different:
	if ((geolatvars.size()>1) && (lllat[1]==lllat[0])) {
		MyBase::SetErrMsg(" geolat T and U grid have same lower-left corner: %f", lllat[0]);
		MyBase::SetErrCode(0);
	}
	if ((geolonvars.size()>1) && (lllon[1]==lllon[0])) {
		MyBase::SetErrMsg(" geolon T and U grid have same lower-left corner: %f", lllon[0]);
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
			rc = nc_inq_varid(ncid, "z_t", &vertVarid);
			if (rc != NC_NOERR) // not there, quit
				return (0);
			//OK, z_t is the name
			_atypnames["st_ocean"] = "z_t";
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
	if (_atypnames["st_ocean"] == "z_t"){//convert cm to meters
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
	char* latUnits[] = {(char*)"degree_north",(char*)"degrees_north",(char*)"degree_N",(char*)"degrees_N",(char*)"degreeN",(char*)"degreesN"};
	char* lonUnits[] = {(char*)"degree_east",(char*)"degrees_east",(char*)"degree_E",(char*)"degrees_E",(char*)"degreeE",(char*)"degreesE"};
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
	deltaLat = (lonLatExtents[3]-lonLatExtents[1])/(nlat-1);
	deltaLon = (lonLatExtents[2]-lonLatExtents[0])/(nlon-1);
	wrapLon=false;
	//Check for global wrap in longitude:
	
	
	if (abs(lonLatExtents[2] - 360.0 - lonLatExtents[0])< 2.*deltaLon) {
		deltaLon = (lonLatExtents[2]-lonLatExtents[0])/nlon;
		wrapLon=true;
	}
	//Does the grid go to the north pole?
	if (lonLatExtents[3] >= 90.-2.*deltaLat) wrapLat = true;
	else wrapLat = false;
	
	// Calc epsilon for converging alpha,beta 
	epsilon = Max(deltaLat, deltaLon)*1.e-7;
	// Calc epsilon for checking outside rectangle
	epsRect = Max(deltaLat,deltaLon)*0.1;
}

//Interpolation functions, can be called after the alphas and betas arrays have been calculated.
//Following can also be used on slices of 3D data
//If the corner latitude is at the top then
void WeightTable::interp2D(const float* sourceData, float* resultData, float missingValue, float missMap){
	int corlon, corlat, corlatp, corlona, corlonb, corlonp;
	for (int j = 0; j<nlat; j++){
		for (int i = 0; i<nlon; i++){
			corlon = cornerLons[i+nlon*j];
			corlat = cornerLats[i+nlon*j];
			corlatp = corlat+1;
			corlona = corlon;
			corlonb = corlon+1;
			corlonp = corlon+1;
			if (corlon == nlon-1){ //Wrap-around longitude.  Note that longitude will not wrap at zipper.
				corlonp = 0;
				corlonb = 0;
			} else if (corlat == nlat-1){ //Get corners on opposite side of zipper:
				corlatp = corlat;
				corlona = nlon - corlon -1;
				corlonb = nlon - corlon -2;
			}
			float alpha = alphas[i+nlon*j];
			float beta = betas[i+nlon*j];
			float data0 = sourceData[corlon+nlon*corlat];
			float data1 = sourceData[corlonp+nlon*corlat];
			float data2 = sourceData[corlonb+nlon*corlatp];
			float data3 = sourceData[corlona+nlon*corlatp];
			float cf0 = (1.-alpha)*(1.-beta);
			float cf1 = alpha*(1.-beta);
			float cf2 = alpha*beta;
			float cf3 = (1.-alpha)*beta;
			float goodSum = 0.f;
			float mvCoef = 0.f;
			//Accumulate values and coefficients for missing and non-missing values
			//If less than half the weight comes from missing value, then apply weights to non-missing values,
			//otherwise just set result to missing value.
			if (data0 == missingValue) mvCoef += cf0;
				else goodSum += cf0*data0;
			if (data1 == missingValue) mvCoef += cf1;
				else goodSum += cf1*data1;
			if (data2 == missingValue) mvCoef += cf2;
				else goodSum += cf2*data2;
			if (data3 == missingValue) mvCoef += cf3;
				else goodSum += cf3*data3;
			if (mvCoef >= 0.5f) resultData[i+j*nlon] = missMap;
			else resultData[i+j*nlon] = goodSum/(1.-mvCoef); 
		}
	}
}
//For given latitude longitude grid vertex, used for testing.
//check all geolon/geolat quads.  See what ulat/ulon cell contains it.
float WeightTable::bestLatLon(int ilon, int ilat){
	
	
	//Loop over all the lon/lat grid vertices
	
	float testLon = lonLatExtents[0]+ilon*deltaLon;
	float testLat = lonLatExtents[1]+ilat*deltaLat;
	//Loop over all cells in grid
	float minval = 1.e30;
	int bestulon,bestulat;
	//Loop over the ulat/ulon grid.  For each ulon-ulat grid coord
	//See if ilon,ilat is in cell mapped from ulon,ulat
	for (int qlat = 0; qlat< nlat-1; qlat++){
		for (int plon = 0; plon< nlon; plon++){
			//Test if ilon,ilat coordinates are in quad of ulon,ulat
			float eps = testInQuad(testLon,testLat,plon, qlat);
			if (eps < minval) {
				minval = eps;
				bestulon = plon;
				bestulat = qlat;
			}
		}
	}
	return minval;
}

//check all ulon,ulat quads.  See if lat/lon vertex lies in one of them
float WeightTable::bestLatLonPolar(int ilon, int ilat){
	//Loop over all the lon/lat grid vertices
	
	float testLon = lonLatExtents[0]+ilon*deltaLon;
	float testLat = lonLatExtents[1]+ilat*deltaLat;
	
	//Loop over all cells in grid
	float minval = 1.e30;
	int bestulon, bestulat;
	float testRad, testThet;
	float bestx, besty;
	
	float bestLat, bestLon, bestThet, bestRad;
	//loop over ulon,ulat cells
	for (int qlat = 0; qlat< nlat; qlat++){
		if (qlat == nlat-1 && !wrapLat) continue;
		
		for (int plon = 0; plon< nlon; plon++){
			if (qlat == nlat-1 && plon > nlon/2) continue;  // only do first half of zipper
			if (plon == nlon-1 && !wrapLon) continue;
			
			//Convert lat/lon coordinates to x,y
			testRad = (90.-testLat)*2./M_PI;
			testThet = testLon*M_PI/180.;
			float testx = testRad*cos(testThet);
			float testy = testRad*sin(testThet);
			float eps = testInQuad2(testx,testy,plon, qlat);
			if (eps < minval){
				minval = eps;
				bestulon = plon;
				bestulat = qlat;
				bestLat = testLat;
				bestLon = testLon;
				bestThet = testThet;
				bestRad = testRad;
				bestx = testx;
				besty = testy;
			}
		}
	}
	return minval;
}

	
int WeightTable::calcWeights(int ncid){
	
	//	Allocate arrays for geolat and geolon.
	geo_lat = new float[nlat*nlon];
	
	//Find the geo_lat and geo_lon variable id's in the file
	int geolatvarid;
	NC_ERR_READ(nc_inq_varid (ncid, geoLatVarName.c_str(), &geolatvarid));
	//	Read the geolat and geolon variables into arrays.
	// Note that lon increases fastest
	NC_ERR_READ(nc_get_var_float(ncid, geolatvarid, geo_lat));
	geo_lon = MOM::getMonotonicLonData(ncid, geoLonVarName.c_str(), nlon, nlat);
	if (!geo_lon) return -1;
	float eps = Max(deltaLat,deltaLon)*1.e-3;
	
	float tst = bestLatLon(0,0);
	
	//	Loop over (nlon/nlat) user grid vertices.  These are similar but not the same as lat and lon.
	//  Call them ulat and ulon to indicate "lat and lon" in user coordinates
	float lat[4],lon[4];
	for (int ulat = 0; ulat<nlat; ulat++){
		if (ulat == nlat-1 && !wrapLat) continue;
		float beglat = geo_lat[nlon*ulat];
		if(beglat < 0. ){ //below equator
						
			
		
			for (int ulon = 0; ulon<nlon; ulon++){
				
				if (ulon == nlon-1 && !wrapLon) continue;
				if (ulon == nlon-1){
					lat[0] = geo_lat[ulon+nlon*ulat];
					lat[1] = geo_lat[nlon*ulat];
					lat[2] = geo_lat[ulon+nlon*(ulat+1)];
					lat[3] = geo_lat[nlon*(ulat+1)];
				} else {
					lat[0] = geo_lat[ulon+nlon*ulat];
					lat[1] = geo_lat[ulon+1+nlon*ulat];
					lat[2] = geo_lat[ulon+nlon*(ulat+1)];
					lat[3] = geo_lat[ulon+1+nlon*(ulat+1)];
				}
				float minlat = Min(Min(lat[0],lat[1]),Min(lat[0],lat[3])) - eps;
				float maxlat = Max(Max(lat[0],lat[1]),Min(lat[2],lat[3])) + eps;

				
				//	For each cell, find min and max longitude and latitude; expand by eps in all directions to define maximal ulon-ulat cell in
				// actual lon/lat coordinates
				float lon0,lon1,lon2,lon3;
				if (ulon == nlon-1){
					lon0 = geo_lon[ulon+nlon*ulat];
					lon1 = geo_lon[nlon*ulat];
					lon2 = geo_lon[ulon+nlon*(ulat+1)];
					lon3 = geo_lon[nlon*(ulat+1)];
					
				} else {
					lon0 = geo_lon[ulon+nlon*ulat];
					lon1 = geo_lon[ulon+1+nlon*ulat];
					lon2 = geo_lon[ulon+nlon*(ulat+1)];
					lon3 = geo_lon[ulon+1+nlon*(ulat+1)];
				}
							
				float minlon = Min(Min(lon0,lon1),Min(lon2,lon3))-eps;
				float maxlon = Max(Max(lon0,lon1),Max(lon2,lon3))+eps;

								
				// find all the latlon grid vertices that fit near this rectangle: 
				// Add 1 to the LL corner latitude because it is definitely below and left of the rectangle
				int latInd0 = (int)(1.+((minlat-lonLatExtents[1])/deltaLat));
				// Don't add 1 to the longitude corner because in some cases (with wrap) it would eliminate a valid longitude index.
				int lonInd0 = (int)(((minlon-lonLatExtents[0])/deltaLon));
				//cover the edges too!
				// edgeflag is used to enable extrapolation when points are slightly outside of the grid extents
				bool edgeFlag = false;
				if (ulat == 0 && latInd0 ==1) {latInd0 = 0; edgeFlag = true;}
				if (ulon == 0) {
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
						
						int plon1 = plon;
						if(plon == nlon) {
							assert(wrapLon);
							plon1 = 0;
						}
						float testLon = lonLatExtents[0]+plon1*deltaLon;
						float testLat = lonLatExtents[1]+qlat*deltaLat;
						float eps = testInQuad(testLon,testLat,ulon, ulat);
						if (eps < epsRect || edgeFlag){  //OK, point is inside, or close enough.
							//Check to make sure eps is smaller than previous entries:
							if (eps > testValues[plon1+nlon*qlat]) continue;
							//It's OK, stuff it in the weights arrays:
							float alph, beta;
							findWeight(testLon, testLat, ulon, ulat,&alph, &beta);
							assert(plon1 >= 0 && plon1 < nlon);
							assert(qlat >= 0 && qlat < nlat);
							alphas[plon1+nlon*qlat] = alph;
							betas[plon1+nlon*qlat] = beta;
							testValues[plon1+nlon*qlat] = eps;
							cornerLats[plon1+nlon*qlat] = ulat;
							cornerLons[plon1+nlon*qlat] = ulon;
							
						}
					}
				}
			}
		} else { //provide mapping to polar coordinates of northern hemisphere
			//map to circle of circumference 360 (matching lon/lat projection above at equator)
			//There are two cases to consider:
			//All 4 points lie in a sector of angle less than 180 degrees, or not.
			//To determine if they lie in such a sector, we first check if the
			//difference between the largest and smallest longitude is less than 180 degrees.
			//If that is not the case, it is still possible that they lie in such a sector, when
			//the angles are considered modulo 360 degrees.  So, for each of the 4 points, test if it
			//is the smallest longitude L in such a sector by seeing if all the other 3 points have longitude
			//no more than 180 + L when an appropriate multiple of 360 is added.
			//If that does not work for any of the 4 points, then there is no such sector and the points
			//may circumscribe the north pole.  In that case the boolean "fullCircle" is set to true.
			//
			// In the case that the points do lie in a sector of less than 180 degrees, we also must
			//determine an inner and outer radius (latitude) to search for latlon points.
			//Begin by using the largest and smallest radius of the four points.
			//However, in order to be sure the sector contains all the points inside the rectangle of the
			//four points, the sector's inner radius must be multiplied by the factor cos(theta/2) where
			//theta is the sector's angle.  This factor ensures that the secant crossing the inner
			//radius will be included in the modified sector.
			for (int ulon = 0; ulon<nlon; ulon++){
				
				if (ulat == nlat-1 && ulon > nlon/2) continue;  // only do first half of zipper
				if (ulon == nlon-1 && !wrapLon) continue;
				
				if (ulon == nlon-1){
					lat[0] = geo_lat[ulon+nlon*ulat];
					lat[1] = geo_lat[nlon*ulat];
					lat[2] = geo_lat[ulon+nlon*(ulat+1)];
					lat[3] = geo_lat[nlon*(ulat+1)];
					
					lon[0] = geo_lon[ulon+nlon*ulat];
					lon[1] = geo_lon[nlon*ulat];
					lon[2] = geo_lon[ulon+nlon*(ulat+1)];
					lon[3] = geo_lon[nlon*(ulat+1)];
				} else if (ulat == nlat-1) { //Handle zipper:
					// The point (ulat, ulon) is across from (ulat, nlon-ulon-1)
					// Whenever ulat+1 is used, replace it with ulat, and switch ulon to nlon-ulon-1
					lat[0] = geo_lat[ulon+nlon*ulat];
					lat[1] = geo_lat[ulon+1+nlon*ulat];
					lat[2] = geo_lat[nlon-ulon-2+nlon*ulat];
					lat[3] = geo_lat[nlon -ulon -1+nlon*ulat];
					lon[0] = geo_lon[ulon+nlon*ulat];
					lon[1] = geo_lon[ulon+1+nlon*ulat];
					lon[2] = geo_lon[nlon-ulon-2+nlon*ulat];
					lon[3] = geo_lon[nlon-ulon-1+nlon*ulat];
				} else {
					lat[0] = geo_lat[ulon+nlon*ulat];
					lat[1] = geo_lat[ulon+1+nlon*ulat];
					lat[2] = geo_lat[ulon+nlon*(ulat+1)];
					lat[3] = geo_lat[ulon+1+nlon*(ulat+1)];
					
					lon[0] = geo_lon[ulon+nlon*ulat];
					lon[1] = geo_lon[ulon+1+nlon*ulat];
					lon[2] = geo_lon[ulon+nlon*(ulat+1)];
					lon[3] = geo_lon[ulon+1+nlon*(ulat+1)];
				}
				
							
				float minlat = 1000., maxlat = -1000.;
				
				//Get min/max latitude
				for (int k=0;k<4;k++) {
					
					if (lat[k]<minlat) minlat = lat[k];
					if (lat[k]>maxlat) maxlat = lat[k];
				}
				
				//Determine a minimal sector (longitude interval)  that contains the 4 corners:
				float maxlon, minlon;
				
				bool fullCircle = false;
				minlon = Min(Min(lon[0],lon[1]),Min(lon[2],lon[3]));
				maxlon = Max(Max(lon[0],lon[1]),Max(lon[2],lon[3]));
				//If the sector width is too great, try other orderings mod 360, or set fullCircle = true.
				//the lon[] array is only used to find max and min.
				if (maxlon - minlon >= 180.) {
					//see if we can rearrange the points modulo 360 degrees to get them to lie in a 180 degree sector
					int firstpoint = -1;
					for (int k = 0; k< 4; k++){
						bool ok = true;
						for( int m = 0; m<4; m++){
							if (m == k) continue;
							if (lon[m]>= lon[k] && lon[m] < lon[k]+180.) continue;
							if (lon[m] <= lon[k] && (lon[m]+360.) < lon[k]+180.) continue;
							//Hmmm.  This point is more than 180 degrees above, when considered modulo 360
							ok = false;
							break;
						}
						if (ok){ firstpoint = k; break;}
					}
					if (firstpoint >= 0) { // we need to consider thet[firstpoint] as the starting angle in the sector
						minlon = lon[firstpoint];
						maxlon = minlon -1000.;
						for (int k = 0; k<4; k++){
							if (k==firstpoint) continue;
							if (lon[k] < lon[firstpoint]) lon[k]+=360.;
							assert(lon[k] >= lon[firstpoint] && lon[k]<(lon[firstpoint]+180.));
							if(lon[k]>maxlon) maxlon = lon[k];
						}
						
					} else fullCircle = true;
							
						
				}
				
				
				//Expand slightly to include points on boundary
				maxlon += eps;
				minlon -= eps;
				
				//If fullCircle is false the sector goes from minlat, minlon to maxlat, maxlon
				//If fullCircle is true, the sector is a full circle, from minrad= 0 to maxrad
				
				//Shrink the inner radius (largest latitude) to be sure to include secant lines between two vertices on max latitude:
				if (!fullCircle){
					float sectorAngle = (maxlon - minlon)*M_PI/180.;
					float shrinkFactor = cos(sectorAngle*0.5);
					maxlat = 90.- (90.-maxlat)*shrinkFactor;
				}
				minlat -= eps;
				maxlat += eps;

				
				// Test all lat/lon pairs that are in the sector, to see if they are inside the rectangle of (x,y) corners.
				// first, discretize the sector extents to match lat/lon grid
				int lonMin, lonMax;
				int latMin = (int)(1.+(minlat-lonLatExtents[1])/deltaLat);
				int latMax = (int)((maxlat-lonLatExtents[1])/deltaLat);
				if (fullCircle) {
					lonMin = 0; lonMax = nlon;
					latMax = nlat -1;
				}
				else {
					lonMin = (int)(1.+(minlon  -lonLatExtents[0])/deltaLon);
					lonMax = (int)((maxlon -lonLatExtents[0])/deltaLon);
				}
				
				//cover the edges too!
				// edgeflag is used to enable extrapolation when points are slightly outside of the grid extents
				bool edgeFlag = false;
				if (ulat == 0 && latMin ==1) {latMin = 0; edgeFlag = true;}
				if (ulon == 0 && lonMin == 1) {
					lonMin = 0; edgeFlag = true;
				}
				
				//slightly enlarge the bounds (??)
				if (latMin > 0) latMin--;
				if (lonMin > 0) lonMin--;
				if (latMax < nlat-1) latMax++;
				if (lonMax < nlon -1) lonMax++;
				//Test each point in lon/lat interval:
				for (int qlat = latMin; qlat<= latMax; qlat++){
					for (int plon = lonMin; plon<= lonMax; plon++){
						float testLon = lonLatExtents[0]+plon*deltaLon;
						int plon1 = plon;
						if(plon >= nlon) {
							assert(wrapLon);
							plon1 = plon -nlon;
						}
						float testLat = lonLatExtents[1]+qlat*deltaLat;
						//Convert lat/lon coordinates to x,y
						float testRad = (90.-testLat)*2./M_PI;
						float testThet = testLon*M_PI/180.;
						float testx = testRad*cos(testThet);
						float testy = testRad*sin(testThet);
						float eps = testInQuad2(testx,testy,ulon, ulat);
						if (eps < epsRect || edgeFlag){  //OK, point is inside, or close enough.
							assert(plon1 >= 0 && plon1 < nlon);
							assert(qlat >= 0 && qlat < nlat);

							//Check to make sure eps is smaller than previous entries:
							if (eps >= testValues[plon1+nlon*qlat]) continue;
							//It's OK, stuff it in the weights arrays:
							float alph, beta;
							
							findWeight2(testx, testy, ulon, ulat,&alph, &beta);
							
							alphas[plon1+nlon*qlat] = alph;
							betas[plon1+nlon*qlat] = beta;
							testValues[plon1+nlon*qlat] = eps;
							cornerLats[plon1+nlon*qlat] = ulat;
							cornerLons[plon1+nlon*qlat] = ulon;
							
						}
					}
				}
				
			} //end ulon loop
		} //end else
	} //end ulat loop
	//Check it out:
	
	for (int i = 0; i<nlat; i++){
		for (int j = 0; j<nlon; j++){
			if (testValues[j+nlon*i] <1.){
				if (alphas[j+nlon*i] > 2.0 || alphas[j+nlon*i] < -1.0 || betas[j+nlon*i]>2.0 || betas[j+nlon*i]< -1.0)
					fprintf(stderr," bad alpha %g beta %g at lon,lat %d , %d, ulon %d ulat %d, testval: %g\n",alphas[j+nlon*i],betas[j+nlon*i], j, i, cornerLons[j+nlon*i], cornerLats[j+nlon*i],testValues[j+i*nlon]);
				continue;
			}
			else {
				if (testValues[j+nlon*i] < 10000.) printf( "poor estimate: %f at lon, lat %d %d, ulon, ulat %d %d\n", testValues[j+nlon*i],j,i, cornerLons[j+nlon*i], cornerLats[j+nlon*i]);
				else {
					MyBase::SetErrMsg(" Error remapping coordinates.  Unmapped lon, lat: %d, %d\n",j,i);
					//exit (-1);
				}
			}
		}
	}
	 
	return 0;				
}
float WeightTable::testInQuad(float plon, float plat, int ilon, int ilat){
	//Sides of quad are determined by lines thru the 4 points at user grid corners:
	//(ilon,ilat), (ilon+1,ilat), (ilon+1,ilat+1), (ilon,ilat+1)
	//These points are mapped to 4 (lon,lat) coordinates (xi,yi) using geolon,geolat variables
	//For each pair P=(xi,yi), Q=(xi+1,yi+1) of these points, the formula
	//Fi(x,y) = (yi+1-yi)*(x-xi)-(y-yi)*(xi+1-xi) defines a function that is <0 on the in-side of the line
	//and >0 on the out-side of the line.
	//Return the maximum of the four formulas Fi evaluated at (plon,plat), or the first one that
	//exceeds epsilon
	float lg[4],lt[4];
	if (ilon == nlon-1){//wrap in ilon coord
		lt[0] = geo_lat[ilon+nlon*ilat];
		lt[1] = geo_lat[nlon*ilat];
		lt[2] = geo_lat[nlon*(ilat+1)];
		lt[3] = geo_lat[ilon+nlon*(ilat+1)];
		lg[0] = geo_lon[ilon+nlon*ilat];
		lg[1] = geo_lon[nlon*ilat];
		lg[2] = geo_lon[nlon*(ilat+1)];
		lg[3] = geo_lon[ilon+nlon*(ilat+1)];
	} else {
		lt[0] = geo_lat[ilon+nlon*ilat];
		lt[1] = geo_lat[ilon+1+nlon*ilat];
		lt[2] = geo_lat[ilon+1+nlon*(ilat+1)];
		lt[3] = geo_lat[ilon+nlon*(ilat+1)];
		lg[0] = geo_lon[ilon+nlon*ilat];
		lg[1] = geo_lon[ilon+1+nlon*ilat];
		lg[2] = geo_lon[ilon+1+nlon*(ilat+1)];
		lg[3] = geo_lon[ilon+nlon*(ilat+1)];
	}
	//Make sure we're not off by 360 degrees..
	for (int k = 0; k<4; k++){
		if (lg[k] > plon + 180.) lg[k] -= 360.;
	}
	float dist = -1.e30;
	for (int j = 0; j<4; j++){
		float dst = (lt[(j+1)%4]-lt[j])*(plon-lg[j]) - (plat-lt[j])*(lg[(j+1)%4] - lg[j]);
		if (dst > dist) dist = dst;

	}
	return dist;
}
float WeightTable::testInQuad2(float x, float y, int ulon, int ulat){
	//Sides of quad are determined by lines thru the 4 points at user grid corners
	//(ulon,ulat), (+1,ilat), (ulon+1,ulat+1), (ulon,ulat+1)
	//These points are mapped to 4 (lon,lat) coordinates  using geolon,geolat variables.
	//Then the 4 points are mapped into a circle of circumference 360. (radius 360/2pi) using polar coordinates
	// lon*180/M_PI is theta, (90-lat)*2/M_PI is radius
	// this results in 4 x,y corner coordinates
	float xc[4],yc[4];
	float rad[4],thet[4];
	if (ulon == nlon-1 ){  //wrap; ie. ulon+1 is replaced by 0
		rad[0] = (90.-geo_lat[ulon+nlon*ulat])*2./M_PI;
		rad[1] = (90.-geo_lat[nlon*ulat])*2./M_PI;
		rad[2] = (90.-geo_lat[nlon*(ulat+1)])*2./M_PI;
		rad[3] = (90.-geo_lat[ulon+nlon*(ulat+1)])*2./M_PI;
		thet[0] = geo_lon[ulon+nlon*ulat]*M_PI/180.;
		thet[1] = geo_lon[nlon*ulat]*M_PI/180.;
		thet[2] = geo_lon[nlon*(ulat+1)]*M_PI/180.;
		thet[3] = geo_lon[ulon+nlon*(ulat+1)]*M_PI/180.;
	} else if (ulat == nlat-1) { //Handle zipper:
		// The point (ulat, ulon) is across from (ulat, nlon-ulon-1)
		// Whenever ulat+1 is used, replace it with ulat, and switch ulon to nlon-ulon-1
		rad[0] = (90.-geo_lat[ulon+nlon*ulat])*2./M_PI;
		rad[1] = (90.-geo_lat[ulon+1+nlon*ulat])*2./M_PI;
		rad[2] = (90.-geo_lat[nlon-ulon-2+nlon*ulat])*2./M_PI;
		rad[3] = (90.-geo_lat[nlon -ulon -1+nlon*ulat])*2./M_PI;
		thet[0] = geo_lon[ulon+nlon*ulat]*M_PI/180.;
		thet[1] = geo_lon[ulon+1+nlon*ulat]*M_PI/180.;
		thet[2] = geo_lon[nlon-ulon-2+nlon*ulat]*M_PI/180.;
		thet[3] = geo_lon[nlon-ulon-1+nlon*ulat]*M_PI/180.;
	} else {
		rad[0] = (90.-geo_lat[ulon+nlon*ulat])*2./M_PI;
		rad[1] = (90.-geo_lat[ulon+1+nlon*ulat])*2./M_PI;
		rad[2] = (90.-geo_lat[ulon+1+nlon*(ulat+1)])*2./M_PI;
		rad[3] = (90.-geo_lat[ulon+nlon*(ulat+1)])*2./M_PI;
		thet[0] = geo_lon[ulon+nlon*ulat]*M_PI/180.;
		thet[1] = geo_lon[ulon+1+nlon*ulat]*M_PI/180.;
		thet[2] = geo_lon[ulon+1+nlon*(ulat+1)]*M_PI/180.;
		thet[3] = geo_lon[ulon+nlon*(ulat+1)]*M_PI/180.;
	}
	for (int i = 0; i<4; i++){
		xc[i] = rad[i]*cos(thet[i]);
		yc[i] = rad[i]*sin(thet[i]);
	}
	//
	//For each pair P=(xi,yi), Q=(xi+1,yi+1) of these points, the formula
	//Fi(x,y) = (yi+1-yi)*(x-xi)-(y-yi)*(xi+1-xi) defines a function that is <0 on the in-side of the line
	//and >0 on the out-side of the line.
	//Return the maximum of the four formulas Fi evaluated at (x,y), or the first one that
	//exceeds epsilon
	float fmax = -1.e30;
	for (int j = 0; j<4; j++){
		float dist = (yc[(j+1)%4]-yc[j])*(x-xc[j]) - (y-yc[j])*(xc[(j+1)%4] - xc[j]);
		//if (dist > epsRect) return dist;
		if (dist > fmax) fmax = dist;
	}
	return fmax;
}
		
void WeightTable::findWeight(float plon, float plat, int ilon, int ilat, float* alpha, float* beta){
	//use iteration to determine alpha and beta that interpolates plon, plat from the user grid
	//corner associated with ilon and ilat.
	//start with prevAlpha=0, prevBeta=0.  
	float th[4],ph[4];
	if (ilon == nlon-1){
		th[0] = geo_lon[ilon+nlon*ilat];
		ph[0] = geo_lat[ilon+nlon*ilat];
		th[1] = geo_lon[nlon*ilat];
		ph[1] = geo_lat[nlon*ilat];
		th[2] = geo_lon[nlon*(ilat+1)];
		ph[2] = geo_lat[nlon*(ilat+1)];
		th[3] = geo_lon[ilon+nlon*(ilat+1)];
		ph[3] = geo_lat[ilon+nlon*(ilat+1)];
		
	} else {
		th[0] = geo_lon[ilon+nlon*ilat];
		ph[0] = geo_lat[ilon+nlon*ilat];
		th[1] = geo_lon[ilon+1+nlon*ilat];
		ph[1] = geo_lat[ilon+1+nlon*ilat];
		th[2] = geo_lon[ilon+1+nlon*(ilat+1)];
		ph[2] = geo_lat[ilon+1+nlon*(ilat+1)];
		th[3] = geo_lon[ilon+nlon*(ilat+1)];
		ph[3] = geo_lat[ilon+nlon*(ilat+1)];
	}
	for (int k = 0; k<4; k++){
		if (th[k]> plon+180.) th[k] -= 360.;
	}
	float alph = 0.f, bet = 0.f;  //first guess
	float newalpha, newbeta;
	int iter;
	for (iter = 0; iter < 10; iter++){
		newalpha = NEWA(alph, bet, plon, plat, th, ph);
		newbeta = NEWB(alph, bet, plon, plat, th, ph);
		
		if (errP(plon, plat, newalpha, newbeta,th,ph) < 1.e-9) break;
		alph = newalpha;
		bet = newbeta;
	}
	if(iter > 10){
		printf(" Weight nonconvergence, errp: %f , alpha, beta: %f %f\n",errP(plon, plat, newalpha, newbeta,th,ph), newalpha, newbeta);
	}
	
	*alpha = newalpha;
	*beta = newbeta;
	
}
void WeightTable::findWeight2(float x, float y, int ilon, int ilat, float* alpha, float* beta){
	//use iteration to determine alpha and beta that interpolates x, y from the user grid
	//corners associated with ilon and ilat, mapped into polar coordinates.
	//start with prevAlpha=0, prevBeta=0.  
	float xc[4],yc[4];
	float rad[4],thet[4];
	if (ilon == nlon-1){//wrap 
		rad[0] = (90.-geo_lat[ilon+nlon*ilat])*2./M_PI;
		rad[1] = (90.-geo_lat[nlon*ilat])*2./M_PI;
		rad[2] = (90.-geo_lat[nlon*(ilat+1)])*2./M_PI;
		rad[3] = (90.-geo_lat[ilon+nlon*(ilat+1)])*2./M_PI;
		thet[0] = geo_lon[ilon+nlon*ilat]*M_PI/180.;
		thet[1] = geo_lon[nlon*ilat]*M_PI/180.;
		thet[2] = geo_lon[nlon*(ilat+1)]*M_PI/180.;
		thet[3] = geo_lon[ilon+nlon*(ilat+1)]*M_PI/180.;
	} else if (ilat == nlat-1){  //zipper
		rad[0] = (90.-geo_lat[ilon+nlon*ilat])*2./M_PI;
		rad[1] = (90.-geo_lat[ilon+1+nlon*ilat])*2./M_PI;
		rad[2] = (90.-geo_lat[nlon - ilon -2 +nlon*ilat])*2./M_PI;
		rad[3] = (90.-geo_lat[nlon - ilon - 1 +nlon*ilat])*2./M_PI;
		thet[0] = geo_lon[ilon+nlon*ilat]*M_PI/180.;
		thet[1] = geo_lon[ilon+1+nlon*ilat]*M_PI/180.;
		thet[2] = geo_lon[nlon - ilon -2 +nlon*ilat]*M_PI/180.;
		thet[3] = geo_lon[nlon - ilon -1 +nlon*ilat]*M_PI/180.;
	} else {
		rad[0] = (90.-geo_lat[ilon+nlon*ilat])*2./M_PI;
		rad[1] = (90.-geo_lat[ilon+1+nlon*ilat])*2./M_PI;
		rad[2] = (90.-geo_lat[ilon+1+nlon*(ilat+1)])*2./M_PI;
		rad[3] = (90.-geo_lat[ilon+nlon*(ilat+1)])*2./M_PI;
		thet[0] = geo_lon[ilon+nlon*ilat]*M_PI/180.;
		thet[1] = geo_lon[ilon+1+nlon*ilat]*M_PI/180.;
		thet[2] = geo_lon[ilon+1+nlon*(ilat+1)]*M_PI/180.;
		thet[3] = geo_lon[ilon+nlon*(ilat+1)]*M_PI/180.;
	}
	
	for (int i = 0; i<4; i++){
		xc[i] = rad[i]*cos(thet[i]);
		yc[i] = rad[i]*sin(thet[i]);
	}
	
	float alph = 0.f, bet = 0.f;  //first guess
	float newalpha, newbeta;
	int iter;
	for (iter = 0; iter < 10; iter++){
		newalpha = NEWA(alph, bet, x, y, xc, yc);
		newbeta = NEWB(alph, bet, x, y, xc, yc);
		
		if (errP(x, y, newalpha, newbeta,xc,yc) < 1.e-9) break;
		alph = newalpha;
		bet = newbeta;
	}
	if(iter > 10){
		fprintf(stderr," Weight nonconvergence, errp: %f , alpha, beta: %f %f\n",errP(x, y, newalpha, newbeta, xc,yc), newalpha, newbeta);
	}
	*alpha = newalpha;
	*beta = newbeta;
	
}
		

//Determine the index in time array that corresponding to a MOM date.
//Requires a vector of times in seconds since simulation start time, such as those in Metadata
//MOM dates are in months relative to the simulation start time.
//momTime is the number of days since simulation start time.
size_t MOM::GetVDCTimeStep(double momTime, const vector<double>& times,  double tol){
	//convert everything to seconds since 01/01/70:
	if (startTimeDouble <= -1.e30) return (size_t)(-1);
	//Convert to seconds (since simulation start time)
	momTime *= (24.*60.*60.);
	
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
//Method that obtains a geolon variable and modifies it to be monotonic
float* MOM::getMonotonicLonData(int ncid, const char* varname, int londimsize, int latdimsize){
	int geolonvarid;
	int rc = nc_inq_varid (ncid, varname, &geolonvarid);
	if (rc != NC_NOERR) return 0;

	float* buf = new float[londimsize*latdimsize];
	float mxlon = -1.e30f;
	if (nc_get_var_float(ncid, geolonvarid, buf) != NC_NOERR){
		delete buf;
		return 0;
	}
	// scan the longitudes at maximum ulats
	for (int j = 0; j<latdimsize; j++){
		if (buf[londimsize-1+ londimsize*j] > mxlon) mxlon = buf[londimsize-1+ londimsize*j];
	}
	//Now fix all the larger longitude values to be negative
	for (int j = 0; j<londimsize*latdimsize; j++){
		if (buf[j] > mxlon) buf[j] -= 360.f;
	}
	return buf;
	 
}


