//
//      $Id$
//

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <cstring>
#include <expat.h>
#include "proj_api.h"
#include <cassert>
#include <utility>
#include <algorithm>
#include <vector>
#include <vapor/MetadataROMS.h>
#include <vapor/XmlNode.h>
#include <vapor/CFuncs.h>
#include <vapor/ROMS.h>
#include <vapor/WRF.h>

using namespace VAPoR;
using namespace VetsUtil;

#define NC_ERR_READ(X) \
{ \
int rc = (X); \
if (rc != NC_NOERR) { \
MyBase::SetErrMsg("Error reading netCDF file at line %d : %s", \
__LINE__,  nc_strerror(rc)) ; \
return(-1); \
} \
}

//
//! The generic constructor.
//

MetadataROMS::MetadataROMS() {
  
  Vars3D.clear();
  Vars2Dxy.clear();
  
  UserTimes.clear();

  epochStartTimeSeconds = -1.e30;
  Extents.clear();
  for(int i = 0; i < 6; i++)
    Extents.push_back(0.0);
  PeriodicBoundary.clear();
  for(int i = 0; i < 3; i++)
    PeriodicBoundary.push_back(0);
  GridPermutation.clear();
  GridPermutation.push_back(1);
  GridPermutation.push_back(2);
  GridPermutation.push_back(3);
  Dimens[0] = Dimens[1] = Dimens[2] = 512;
  Reflevel = 0;

} // End of constructor.

//
//! The constructor with a vector of inpuut files and a topofile
//

void MetadataROMS::_MetadataROMS(
	const vector<string> &infiles, const string &topofile, const map <string, string> &atypnames, vector<string>& vars2d, vector<string>& vars3d
) {
  
  pair<string, double> attr;
  string startdate, tstartdate;

  string ErrMsgStr;
  
  int i = 0;

  vector<string> tempVars2D, tempVars3D;

//
// Some initialization.
//

  //The vars vectors will contain either the supplied variable names, or
  //all the valid vars that appear in all the files.
  Vars3D.clear();
  Vars2Dxy.clear();

  UserTimes.clear();
  epochStartTimeSeconds = -1.e30;
  Extents.clear();
  for(int i = 0; i < 6; i++)
    Extents.push_back(0.0);
  PeriodicBoundary.clear();
  for(int i = 0; i < 3; i++)
    PeriodicBoundary.push_back(0);
  GridPermutation.clear();
  GridPermutation.push_back(1);
  GridPermutation.push_back(2);
  GridPermutation.push_back(3);
  Dimens[0] = Dimens[1] = Dimens[2] = 512;
  
  Reflevel = 0;

// The ROMS constructor will check the topofile,
// initialize the variable names
  ROMS roms(topofile, atypnames, vars2d, vars3d);
  if (ROMS::GetErrCode() != 0) {
		ROMS::SetErrMsg(
			"Error processing topofile file %s, exiting.",topofile.c_str()
		);
        return;
    }
	
//Then process all the data files.   Find all the valid variables that appear in these files.
//Keep track of all the time steps that occur.
  for(i=0; i < infiles.size(); i++) {
	float exts[6];
    int rc = roms.addFile(infiles[i], exts, tempVars2D, tempVars3D);
	if (ROMS::GetErrCode() != 0) {
		ROMS::SetErrMsg(
			"Error processing file %s, skipping.",infiles[i].c_str()
		);
        ROMS::SetErrCode(0);
        continue;
    }
	if (rc != 0){
		ROMS::SetErrMsg(
			"No valid variables found in %s, skipping.",infiles[i].c_str()
		);
        ROMS::SetErrCode(0);
        continue;
	}

	roms.GetDims(Dimens);
//Should check consistency; for now just copy over extents:
	Extents[0] = exts[0];
	Extents[1] = exts[1];
	Extents[3] = exts[3];
	Extents[4] = exts[4];
	//Copy vertical extents if they are different:
	if (exts[5] > exts[2]) {Extents[5] = exts[5]; Extents[2] = exts[2];}

	const float* latlonexts = roms.GetLonLatExtents();
	  
	minLon = latlonexts[0];
	maxLon = latlonexts[2];
	minLat = latlonexts[1];
	maxLat = latlonexts[3];
// Add the timesteps in this file into the full timestep list, convert to seconds
	
	const vector<double> newtimes = roms.GetTimes();
	for (int j = 0; j<newtimes.size(); j++){
		UserTimes.push_back(newtimes[j]);
	
	}
	//Save the startTime, if it is in the file.
	//Check and warn if inconsistent
	if (roms.getStartSeconds()> -1.e30) {
		if (epochStartTimeSeconds != -1.e30 && epochStartTimeSeconds != roms.getStartSeconds()){
			MyBase::SetErrMsg("Inconsistent start times in files");
		}
		epochStartTimeSeconds = roms.getStartSeconds();
	}
  } // End for files to open (i).



// Finished all the input files, now can process over 
// all of the collected data.
// All the data is retrieved from the ROMS instance.


// sort the time steps and remove duplicates

  sort(UserTimes.begin(), UserTimes.end());
  //Remove duplicates 
  vector<double>::iterator iter = UserTimes.end();
  for (int j = UserTimes.size()-1; j >0; j--){
	  iter--;
	  if (UserTimes[j] == UserTimes[j-1])
		  UserTimes.erase(iter);
  }
  
  //Either add the variable names that were passed in, or those that were found.
  if (vars2d.size() > 0){
	  for (int j = 0; j<vars2d.size(); j++){
			if (find(Vars2Dxy.begin(), Vars2Dxy.end(), vars2d[j]) == Vars2Dxy.end()) 
				Vars2Dxy.push_back(vars2d[j]);
	  }
  } else {
	  for (int j = 0; j<tempVars2D.size(); j++){
		  if (find(Vars2Dxy.begin(), Vars2Dxy.end(), tempVars2D[j])== Vars2Dxy.end()) 
			Vars2Dxy.push_back(tempVars2D[j]);
	  }
  }
  if (vars3d.size() > 0){
	  for (int j = 0; j<vars3d.size(); j++){
		  if (find(Vars3D.begin(), Vars3D.end(), vars3d[j]) == Vars3D.end()) 
			Vars3D.push_back(vars3d[j]);
	  }
  } else {
	  for (int j = 0; j<tempVars3D.size(); j++){
		  if (find(Vars3D.begin(), Vars3D.end(), tempVars3D[j])== Vars3D.end()) 
			Vars3D.push_back(tempVars3D[j]);
	  }
  }
	
	
} // End of constructor.

MetadataROMS::MetadataROMS(const vector<string> &infiles, const string &topofile, vector<string>& vars2d, vector<string>& vars3d) {

	map <string, string> atypnames;	// no atypical names
	_MetadataROMS(infiles, topofile, atypnames, vars2d, vars3d);
}

MetadataROMS::MetadataROMS(
	const vector<string> &infiles, const string& topofile, const map <string, string> &atypnames, vector<string>& vars2d, vector<string>& vars3d
) {
	_atypnames = atypnames;
	_MetadataROMS(infiles, topofile, _atypnames, vars2d, vars3d);
}

//
//! The destructor.
//
 
MetadataROMS::~MetadataROMS() {
  Vars3D.clear();
  Vars2Dxy.clear();
  
  UserTimes.clear();
  PeriodicBoundary.clear();
  GridPermutation.clear();

  
} // End of destructor.


void MetadataROMS::GetTSUserTimeStamp(size_t ts, string &s) const { 
	s.clear();
	if (ts >= UserTimes.size()) return;
	double dblSeconds = epochStartTimeSeconds;
	if (dblSeconds <= -1.e30) return;
	dblSeconds += UserTimes[ts];
	TIME64_T currSeconds = (dblSeconds > 0.) ? (TIME64_T) (dblSeconds + 0.5) : (TIME64_T) (dblSeconds - 0.5) ;
	int rc = WRF::EpochToWRFTimeStr( currSeconds, s);
	if (rc) s.clear();
}
