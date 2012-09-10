//
//      $Id$
//


#ifndef _ROMS_h_
#define _ROMS_h_


#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <netcdf.h>
#include <vapor/PVTime.h>

namespace VAPoR {

class WeightTable;
class VDF_API ROMS : public VetsUtil::MyBase {
public:

	ROMS(const string &toponame, const vector<string>& vars2d, const vector<string>& vars3d);

	ROMS(const string &toponame,  const map <string, string> &atypnames, const vector<string>& vars2d, const vector<string>& vars3d);
	virtual ~ROMS();

	int addFile(const string& romsname, float extents[6], vector<string>& vars2d, vector<string>& vars3d);

	//Determine geolon and geolat variables associated with a variable
	int GetGeoLonLatVar(int ncid, int varid, int* geolon, int* geolat);
	double getStartSeconds(){return startTimeDouble;} //seconds since 1/1/1970, + or -

	float* GetDepths();


	const vector<double>& GetTimes(){return _romsTimes;}
	const vector<string>& getGeoLatVars() {return geolatvars;}
	const vector<string>& getGeoLonVars() {return geolonvars;}
	int getTopoNcid(){return topoNcId;}
	const float* GetExtents(){return _Exts;}
	const float* GetLonLatExtents(){return _LonLatExts;}

	void GetDims(size_t dims[3]) {for (int i = 0; i<3; i++) dims[i]=_dimLens[i];}
	size_t getNumTimesteps(){return _dimLens[3];}
	int MakeWeightTables();
	WeightTable* GetWeightTable(int lonlatnum){
		return WeightTables[lonlatnum];
	}
	// Find the closest time step associated with a specified day.
	// Uses an array of user times that should be obtained from the Metadata.
	// Return (size_t)-1 if nothing within specified tolerance, or if the time is not inside the VDC min/max time interval,
	// or if the simulation start time (epochStartTimeSeconds) has not been established.
	// \param[in] double romstime  Time (in months) in ROMS data, relative to start time
	// \param[in] double tol  Optional error tolerance in months
	// \retval size_t closest index in userTimes
	//
	size_t GetVDCTimeStep(double romstime, const vector<double>& userTimes, double tol = 1.0);
	
	int extractStartTime(int ncid, int timevarid);  //Determine the start time from the current data file.  Set startTimeStamp and startTimeDouble.
	//Retrieve geolon data, make it monotonic:
	static float* getMonotonicLonData(int ncid, const char* varname, int londimsize, int latdimsize);
	static double vaporMissingValue(){ return 1.e38;}
private:

	// A mapping between required ROMS variable names and how  these
	// names may appear in the file. The first string is the alias,
	// the second is the name of the var as it appears in the file
	//
	map <string, string> _atypnames;

	int _ncid;
	float _Exts[6]; // extents
	float _LonLatExts[4];
	size_t _dimLens[4]; // Lengths of x, y, z, and time dimensions (unstaggered)

	bool add2dVars, add3dVars;  //Should we add to the existing variable names?
	vector<double> _romsTimes;  //Times for which valid data has been found

	vector <string> geolatvars; //up to two geolat var names.  First is T-grid
	vector <string> geolonvars; //up to two geolon var names.  First is T-grid

	string startTimeStamp; //Start time found in metadata of time variable
	double startTimeDouble; // Conversion of start time into seconds since 1/1/1970, or -1.e30 if not present
	int topoNcId;  //ncid for topo file
	
	//hold the weight tables that are constructed:
	WeightTable** WeightTables;
	
	float* depthsArray;

	int _ROMS(const string &romsname, const map <string, string> &atypnames, const vector<string>& vars2d, const vector<string>& vars3d);
	int _GetROMSTopo(int ncid); // Get netCDF file ID (input)
	
	void addTimes(int numtimes, double times[]);
	void addVarName(int dim, string& vname, vector<string>&vars2d, vector<string>&vars3d);
	int varIsValid(int ncid, int ndims, int varid);

	//Check for geolat or geolon attributes of a variable.
	//Return 1 for geolat, 2 for geolon, 0 for neither.
	int testVarAttribs(int ncid, int varid);


};
	
}; //End namespace VAPoR

#endif