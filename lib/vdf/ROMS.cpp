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
#include <vapor/ROMS.h>
#include <vapor/WRF.h>
#include <vapor/WeightTable.h>
#ifdef _WINDOWS 
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

int ROMS::_ROMS(
	const string &toponame, const map <string, string> &names, const vector<string>& vars2d, const vector<string>& vars3d
) {
	depthsArray=0;
	_ncid = 0;

	//
	// Deal with non-standard names for required variables
	//
	
	map <string, string>::const_iterator itr;
	_atypnames["ocean_time"] = ((itr=names.find("ocean_time"))!=names.end()) ? itr->second : "ocean_time";
	_atypnames["h"] = ((itr=names.find("h"))!=names.end()) ? itr->second : "h";
	_atypnames["xi_rho"] = ((itr=names.find("xi_rho"))!=names.end()) ? itr->second : "xi_rho";
	_atypnames["xi_psi"] = ((itr=names.find("xi_psi"))!=names.end()) ? itr->second : "xi_psi";
	_atypnames["xi_u"] = ((itr=names.find("xi_u"))!=names.end()) ? itr->second : "xi_u";
	_atypnames["xi_v"] = ((itr=names.find("xi_v"))!=names.end()) ? itr->second : "xi_v";
	_atypnames["eta_rho"] = ((itr=names.find("eta_rho"))!=names.end()) ? itr->second : "eta_rho";
	_atypnames["eta_psi"] = ((itr=names.find("eta_psi"))!=names.end()) ? itr->second : "eta_psi";
	_atypnames["eta_u"] = ((itr=names.find("eta_u"))!=names.end()) ? itr->second : "eta_u";
	_atypnames["eta_v"] = ((itr=names.find("eta_v"))!=names.end()) ? itr->second : "eta_v";
	_atypnames["s_rho"] = ((itr=names.find("s_rho"))!=names.end()) ? itr->second : "s_rho";
	_atypnames["s_w"] = ((itr=names.find("s_w"))!=names.end()) ? itr->second : "s_w";

	_Exts[2] = _Exts[5] = 0.0;
	_dimLens[0] = _dimLens[1] = _dimLens[2] = _dimLens[3] = 1;

	add2dVars = add3dVars = true;
	
	_romsTimes.clear();
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

	int rc = _GetROMSTopo(_ncid );

	return(rc);
}

ROMS::ROMS(const string &toponame, const vector<string>& vars2d, const vector<string>& vars3d) {
	map <string, string> atypnames;
	(void) _ROMS(toponame, atypnames, vars2d, vars3d);
}

ROMS::ROMS(const string &romsname, const map <string, string> &atypnames, const vector<string>& vars2d, const vector<string>& vars3d) {
	
	(void) _ROMS(romsname, atypnames, vars2d, vars3d);
}

ROMS::~ROMS() {
	// Close the ROMS file
	(void) nc_close( _ncid );
}

// Read a data file.  Add any new variables found to current list of discovered variables.
// Add any timesteps found to timestep list, if any variables are found.  Return nonzero if no variables found.
int ROMS::addFile(const string& datafile, float extents[6], vector<string>&vars2d, vector<string>&vars3d){
	// Open netCDF file and check for failure
	int ncid;
	NC_ERR_READ( nc_open( datafile.c_str(), NC_NOWRITE, &ncid ));

	//If the number of layers has not been initialized, get that from the vertical dimension
	if( _dimLens[2] == 0 ) { //uninitialized
		string altname = _atypnames["s_rho"];
		int vertVarid;
	
		int rc = nc_inq_varid(ncid, altname.c_str(), &vertVarid);
		if (rc == NC_NOERR) {
			//Now we can get the vertical dimension 
			int vdimid; 
			size_t vdimsize;
			NC_ERR_READ(nc_inq_vardimid(ncid, vertVarid, &vdimid));
			NC_ERR_READ(nc_inq_dimlen(ncid, vdimid, &vdimsize));
			_dimLens[2] = vdimsize;
		}
	}

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
	if (strcmp("ocean_time",nctimename) == 0) timename = "ocean_time";
	
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
	nc_close(ncid);
	return 0;
}
int ROMS::extractStartTime(int ncid, int timevarid){
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
		size_t strPos = strAtt.find("seconds since");
		int y,m,d,h,mn,s;
		if (strPos != string::npos){
			//See if we can extract the subsequent two tokens, and that they are of the format
			// yyyy-mm-dd and hh:mm:ss
			
			string substr = strAtt.substr(13+strPos, 20);
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


float* ROMS::GetDepths(){
	if (depthsArray) {
		delete depthsArray;
		depthsArray = 0;
	}
	//See if we can open the h variable in the topo file
	const string& depthVar = _atypnames["h"];
	int varid;
	int rc = nc_inq_varid(topoNcId, depthVar.c_str(), &varid);
	if (rc != NC_NOERR) {//Not there.  
		return 0;
	}
	// Create array to hold double data.  Note that this is on the rho-grid
	int xdim = _dimLens[0]+1;
	int ydim = _dimLens[1]+1;
	double* depthsDbl = new double[xdim*ydim];
	
	rc = nc_get_var_double(topoNcId, varid, depthsDbl);
	if (rc != NC_NOERR) {//Not there. 
		delete depthsDbl;
		return 0;
	}
	double mv = vaporMissingValue();
	rc = nc_get_att_double(topoNcId, varid, "_FillValue", &mv);	
	depthsArray = new float[xdim*ydim];
	float maxdepth = -100000.;
	float mindepth = 1000000.;
	//Negate, convert to float (this is height above sea level:
	for (size_t i = 0; i<xdim*ydim; i++){
		if (depthsDbl[i] != mv ){
			depthsArray[i] = (float) (-depthsDbl[i]);
			if (depthsArray[i]<mindepth) mindepth = depthsArray[i];
			if (depthsArray[i]>maxdepth) maxdepth = depthsArray[i];
		}
		else depthsArray[i] = (float)vaporMissingValue();
	}
	delete depthsDbl;
	return depthsArray;
}
float* ROMS::GetAngles(){
	//First see if there is an "angle" variable in the grid file.  If so, return that.
	//If not, calculate the angles that the ROMS data grid (x-axis) makes with latitude. Produce an 
	//array of angles (in radians, one at each ROMS grid vertex.  Use the Weight table to 
	//get the angles (since it already has the geolat and geolon variables)
	
 	bool haveAngles = true;
	// Create array to hold double data.  Note that this is on the rho-grid
	int xdim = _dimLens[0]+1;
	int ydim = _dimLens[1]+1;
	anglesArray = new float[xdim*ydim];
	int varid;
	int rc = nc_inq_varid(topoNcId, "angle", &varid);
	if (rc != NC_NOERR) {//Not there.  
		haveAngles = false;
	}
	//Is it double?
	nc_type vartype;
	rc = nc_inq_vartype(topoNcId, varid,  &vartype);
	bool isDouble = (vartype == NC_DOUBLE); //either float or double
	if (!isDouble){
		rc = nc_get_var_float(topoNcId, varid, anglesArray);
		if (rc != NC_NOERR) {//Not there.  
			haveAngles = false;
		}
	} else {
		double* dAnglesArray = new double[xdim*ydim];
		rc = nc_get_var_double(topoNcId, varid, dAnglesArray);
		if (rc != NC_NOERR) {//Not there.  
			haveAngles = false;
		} else {
			for (int i = 0; i< xdim*ydim; i++){
				anglesArray[i] = (float)dAnglesArray[i];
			}
		}
		delete dAnglesArray;
	}
	if (haveAngles) return anglesArray;

	//use rho-grid for calculating angles
	WeightTable *wt =GetWeightTable(3);
	for (int i = 0; i<ydim; i++){
		for (int j = 0; j<xdim; j++){
			anglesArray[j+xdim*i] = wt->getAngle(i,j);
		}
	}
	return anglesArray;
}
float* ROMS::GetLats(){
	//This can be obtained from the Weight Table.  
	
	//use rho-grid for getting latitude
	WeightTable *wt =GetWeightTable(3);
	return wt->getGeoLats();
}

int ROMS::_GetROMSTopo(
	int ncid // Holds netCDF file ID (in)
	
) {
	string ErrMsgStr;
	
	
	int nvars; // Number of variables
	
	// Find the number of variables
	NC_ERR_READ( nc_inq_nvars(ncid, &nvars ) );

	// Loop through all the variables in the file, looking for 2D float or double geolat and geolon variables.
	// These should be named lat_rho, lat_u, lat_v, lat_psi, lon_rho, lon_u, lon_v, lon_psi
	// The variable names and ncId's are inserted in positions 0,1,2,3 for the
	// psi, u, v, and rho grids respectively.
	
	// When such a variable is found, add it to geolatvars and geolonvars
	// Exactly four variables should be found.  
	// Find the min and max value of each lat and each lon variable
	geolatvars.clear();
	geolonvars.clear();
	geolatvars.push_back("lat_psi");
	geolatvars.push_back("lat_u");
	geolatvars.push_back("lat_v");
	geolatvars.push_back("lat_rho");
	geolonvars.push_back("lon_psi");
	geolonvars.push_back("lon_u");
	geolonvars.push_back("lon_v");
	geolonvars.push_back("lon_rho");
	vector<int> latvarids;
	vector<int> lonvarids;
	//Initialize varid's to -1
	for (int i = 0; i<4; i++){
		latvarids.push_back(-1);
		lonvarids.push_back(-1);
	}

	char varname[NC_MAX_NAME+1];
	for (int i = 0; i<nvars; i++){
		nc_type vartype;
		NC_ERR_READ( nc_inq_vartype(ncid, i,  &vartype) );
		if (vartype != NC_FLOAT && vartype != NC_DOUBLE) continue;
		int ndims;
		NC_ERR_READ(nc_inq_varndims(ncid, i, &ndims));
		if (ndims != 2) continue;
		
		NC_ERR_READ(nc_inq_varname(ncid, i, varname));
		for (int j =0; j<4; j++){
			if (geolatvars[j].compare(varname)==0){
				latvarids[j] = i;
				break;
			} else if (geolonvars[j].compare(varname)==0){
				lonvarids[j] = i;
				break;
			}
		}
	}
	//Check what was found
	for (int j = 0; j<4; j++){
		if (latvarids[j] < 0 || lonvarids[3] < 0){
			MyBase::SetErrMsg(" geo-lat or geo-lon variable not found in grid file");
			return -1;
		}
	}
	
	// Read the fourth geolat and geolon variables, determine the extents of the rho grid
	// First determine the dimensions of the 4th geolat and geolon vars
	int londimids[2], latdimids[2];
	size_t londimlen[2], latdimlen[2];
	//Get the two dimids for the rho latitude:
	NC_ERR_READ(nc_inq_vardimid(ncid, latvarids[3], latdimids));
	//Get the sizes of these dimensions:
	NC_ERR_READ(nc_inq_dimlen(ncid, latdimids[0], latdimlen));
	NC_ERR_READ(nc_inq_dimlen(ncid, latdimids[1], latdimlen+1));
	//Get the two dimids for the rho longitude
	NC_ERR_READ(nc_inq_vardimid(ncid, lonvarids[3], londimids));
	//Get the sizes of these dimensions:
	NC_ERR_READ(nc_inq_dimlen(ncid, londimids[0], londimlen));
	NC_ERR_READ(nc_inq_dimlen(ncid, londimids[1], londimlen+1));
	

	//Now read the rho grid geolat and geolon vars, to find extents  
	
	float minlat, maxlat, minlon, maxlon;
	
	double * buf = new double[latdimlen[0]*latdimlen[1]];
	NC_ERR_READ(nc_get_var_double(ncid, latvarids[3], buf));
	
	maxlat = -1.e30;
	minlat = 1.e30;
	for (int j = 0; j<latdimlen[0]*latdimlen[1]; j++){
		if (buf[j]<minlat) minlat = buf[j];
		if (buf[j]>maxlat) maxlat = buf[j];
	}
	
	delete buf;
	
	//Longitude is more tricky because it may "wrap".  When that happens the difference between the largest and smallest latitudes
	//will be nearly 360.  In that case we can find a longitude L that is missed by the lon variable (by at least one degree) and then subtract 360 from all 
	//longitudes greater than L
	
	
	int ulondim = londimlen[0];
	int ulatdim = londimlen[1];
	float* fbuf = getMonotonicLonData(ncid, geolonvars[3].c_str(), ulondim, ulatdim);
	if (!fbuf) return -1;
	maxlon = -1.e30;
	minlon = 1.e30;
	for (int j = 0; j<ulondim*ulatdim; j++){
		if (fbuf[j]<minlon) minlon = fbuf[j];
		if (fbuf[j]>maxlon) maxlon = fbuf[j];
	}
	assert(maxlon > minlon);
	delete fbuf;

	
	// Save the extents and the rho-grid  dimensions, converted to meters at equator
	_LonLatExts[0] = minlon;
	_LonLatExts[1] = minlat;
	_LonLatExts[2] = maxlon;
	_LonLatExts[3] = maxlat;
	_Exts[0] = _LonLatExts[0]*111177.;
	_Exts[1] = _LonLatExts[1]*111177.;
	_Exts[3] = _LonLatExts[2]*111177.;
	_Exts[4] = _LonLatExts[3]*111177.;
	//By default make the grid the same size as the data (psi) grid
	_dimLens[0] = londimlen[1]-1;
	_dimLens[1] = londimlen[0]-1;
	//Note: dimLens[2] needs number of vertical layers, which will be found in data files
	_dimLens[2] = 0;
	if ((londimlen[0] != latdimlen[0]) || (londimlen[1] != latdimlen[1])) {
		MyBase::SetErrMsg(" geolon and geolat dimensions differ");
		return -1;
	}

	//Now get the vertical dimensions and extents.
	//Find the min and max values of h (depth)
	
	string altname = _atypnames["h"];
	int vertVarid;
	
	int rc = nc_inq_varid(ncid, altname.c_str(), &vertVarid);
	if (rc != NC_NOERR) //Not there.  quit.
		return(0);
	
	//Now we can get the vertical dimension and extents:
	// Read the depth variable, find its max and min values
	int vdimid[2]; 
	size_t vdimsize[2];
	NC_ERR_READ(nc_inq_vardimid(ncid, vertVarid, vdimid));
	NC_ERR_READ(nc_inq_dimlen(ncid, vdimid[0], vdimsize));
	NC_ERR_READ(nc_inq_dimlen(ncid, vdimid[1], vdimsize+1));
	//read the variable, and find the top and bottom of the data
	double* depths = new double[vdimsize[0]*vdimsize[1]];
	
	NC_ERR_READ(nc_get_var_double(ncid, vertVarid, depths));
	//Find min and max (these are positive!)
	double mindepth = 1.e30;
	double maxdepth = -1.e30;
	for (int i = 0; i< vdimsize[0]*vdimsize[1]; i++){
		if (depths[i]>maxdepth) maxdepth = depths[i];
		if (depths[i]<mindepth) mindepth = depths[i];
	}

	delete depths;
	
	//negate, turn upside down:
	
	_Exts[5] = 100.0;  //Positive, to include room for ocean surface, even though ROMS data does not go higher than 0.0.
	_Exts[2] = -maxdepth;  
	
	return(0);
} // End of _GetROMSTopo.



void ROMS::addTimes(int numtimes, double times[]){
	for (int i = 0; i< numtimes; i++){
		int k;
		for (k = 0; k < _romsTimes.size(); k++){
			if (_romsTimes[k] == times[i]) continue;
		}
		if (k >= _romsTimes.size()) _romsTimes.push_back(times[i]);
	}

}
void ROMS::addVarName(int dim, string& vname, vector<string>&vars2d, vector<string>&vars3d){
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
int ROMS::varIsValid(int ncid, int ndims, int varid){
	char varname[NC_MAX_NAME+1];
	NC_ERR_READ(nc_inq_varname(ncid, varid, varname));
	int dimids[4];
	NC_ERR_READ(nc_inq_vardimid(ncid, varid, dimids));
	if (ndims == 4){
		//Find the 2nd dimension name:
		char dimname[NC_MAX_NAME+1];
		NC_ERR_READ(nc_inq_dimname(ncid, dimids[1], dimname));
		if ((_atypnames["s_w"] != dimname) &&
			(_atypnames["s_rho"] != dimname) ){
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
int ROMS::GetGeoLonLatVar(int ncid, int varid, int* geolon, int* geolat){
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
		for (int i = 0; i<4; i++){
			size_t strPos = strAtt.find(geolatvars[i]);
			if (strPos != string::npos){
				*geolat = i;
				break;
			}
		}
		if (*geolat < 0) return -1;
		for (int i = 0; i<4; i++){
			size_t strPos = strAtt.find(geolonvars[i]);
			if (strPos != string::npos){
				*geolon = i;
				break;
			}
		}
		if (*geolon < 0) return -1;
		return 0;
	}
	return -1;
	
}
//Make a weight table for matched pairs of geolon/geolat variables

int ROMS::MakeWeightTables(){
	WeightTables = new WeightTable*[geolatvars.size()];
	for (int lattab = 0; lattab < geolatvars.size(); lattab++){
		WeightTable* wt = new WeightTable(this, lattab);
		int rc = wt->calcWeights(topoNcId);
		if (!rc) WeightTables[lattab] = wt;
		else return rc;
			
	}
	return 0;
}

			


//Determine the index in time array that corresponding to a ROMS date.
//Requires a vector of times in seconds since simulation start time, such as those in Metadata
//ROMS dates are in months relative to the simulation start time.
//romsTime is the number of days since simulation start time.
size_t ROMS::GetVDCTimeStep(double romsTime, const vector<double>& times,  double tol){
	//convert everything to seconds since 01/01/70:
	if (startTimeDouble <= -1.e30) return (size_t)(-1);
	//Convert to seconds since simulation start time
	
	size_t mints = 0, maxts = times.size()-1;
	//make sure we are in right interval:
	if (romsTime <= times[0]-tol) return -1;
	if (romsTime >= times[maxts]+tol) return -1;
	//Do binary search
	
	size_t mid = (maxts+mints)/2;
	int ntries = 0;
	while (mints < maxts-1){
		if (times[mid]< romsTime)
			mints = mid;
		else if (times[mid] != romsTime)
			maxts = mid;
		else return(mid);
		mid = (maxts+mints)/2;
		if(ntries++ > 100) assert(0);
	}
	if(abs(times[mid]-romsTime) < tol) return mid;
	else if (abs(times[mints]-romsTime) < tol) return mints;
	else if (abs(times[maxts]-romsTime) < tol) return maxts;
	else return (size_t)-1;
	
}
//Method that obtains a geolon variable and modifies it to be monotonic
//Find the widest longitude interval that is avoided by the variable, then 
//if that interval does not contain 360, subtract 360 from
//the longitudes above that interval.

float* ROMS::getMonotonicLonData(int ncid, const char* varname, int londimsize, int latdimsize){
	int geolonvarid;
	int longitudes[361];
	for (int i = 0; i<361; i++) longitudes[i]=-1;
	int rc = nc_inq_varid (ncid, varname, &geolonvarid);
	if (rc != NC_NOERR) return 0;

	double* dbuf = new double[londimsize*latdimsize];
	float* fbuf = new float[londimsize*latdimsize];
	
	if (nc_get_var_double(ncid, geolonvarid, dbuf) != NC_NOERR){
		delete dbuf;
		return 0;
	}
	// scan the longitudes, while converting to float 
	for (int j = 0; j<latdimsize*londimsize; j++){
		int intlon = (int)(dbuf[j]+0.5);
		assert(intlon >= 0 && intlon <= 360);
		longitudes[intlon]=0;
		fbuf[j] = (float)dbuf[j];
	}
	delete dbuf;
	//Find empty interval lengths
	int maxLonInterval = -1;
	int maxLonStart = -1;
	for (int i = 0; i<= 360; i++){
		if (longitudes[i] == 0 ) continue;   //occupied 
		
		longitudes[i] = 1;
		for (int j = i+1; j< 360+i; j++){//add one for every empty degree to the right, circle around 360
			int ja = j;
			if (ja > 360) ja -= 360;
			if (longitudes[ja] == 0) break;
			longitudes[i]++;
		}
		if (longitudes[i]>maxLonInterval) {
			maxLonInterval = longitudes[i];
			maxLonStart = i;
		}
	}
	//Make sure there's a gap:
	if (maxLonInterval<1) return 0;
	
	//See if the maxLonInterval includes 360, if so we are done
	if (maxLonStart + maxLonInterval >= 360) return fbuf;
	float mxlon = maxLonStart+0.5f;
	//fix all the larger longitude values (above the gap) to be negative
	for (int j = 0; j<londimsize*latdimsize; j++){
		if (fbuf[j] > mxlon) fbuf[j] -= 360.f;
	}
	return fbuf;
	 
}


