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
#include <netcdf.h>
#include <vapor/MetadataWRF.h>
#include <vapor/XmlNode.h>
#include <vapor/CFuncs.h>
#include <vapor/WRF.h>

using namespace VAPoR;
using namespace VetsUtil;

//Define ordering of pairs <timestep, extents> so they can be sorted together
bool timeOrdering(
	pair<TIME64_T, vector<float> > p, pair<TIME64_T, vector<float> > q
) {
  if (p.first < q.first) 
    return true;
  return false;
}


//
//! The generic constructor.
//

MetadataWRF::MetadataWRF() {
  Global_attrib.clear();
  Vars3D.clear();
  Vars2Dxy.clear();
  UserTimeStamps.clear();
  UserTimes.clear();
  StartDate.clear();
  MapProjection.clear();
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
  Time_latlon_extents.clear();
  Timestep_filename.clear();
  Timestep_offset.clear();
  Reflevel = 0;

} // End of constructor.

//
//! The constructor with a vector of inpuut files.
//

void MetadataWRF::_MetadataWRF(
	const vector<string> &infiles, const map <string, string> &atypnames
) {
  vector<string> twrfvars3d, twrfvars2d;
  vector <pair< TIME64_T, vector <float> > > tt_latlon_exts;
  vector<pair<string, double> > tgl_attr;
  vector<pair<string, double> >::iterator sf_pair_iter;
  vector<string>::iterator striter;
  pair<string, double> attr;
  string startdate, tstartdate;
  string mapprocetion, tmapprojection;
  string ErrMsgStr;
  size_t tdimlens[4];
  float dx = 0.0, tdx = 0.0, dy = 0.0, tdy = 0.0;
  float cen_lat= 1.e30f, cen_lon=1.e30f;
  int i = 0;
  bool first_file_flag = true;
  bool mismatch_flag = false;

//
// Some initialization.
//

  Global_attrib.clear();
  Vars3D.clear();
  Vars2Dxy.clear();
  UserTimeStamps.clear();
  UserTimes.clear();
  StartDate.clear();
  MapProjection.clear();
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
  Time_latlon_extents.clear();
  Timestep_filename.clear();
  Timestep_offset.clear();
  Reflevel = 0;

// Walk through the files and see if each is valid by all
// have the same variables and attribute values. Keep the 
// good files.  The first file is the template for the rest.
  bool usingRotLatLon = false;
  for(i=0; i < infiles.size(); i++) {
    twrfvars3d.clear();
    twrfvars2d.clear();
    tgl_attr.clear();
    tt_latlon_exts.clear();

    WRF wrf(infiles[i], atypnames);
	if (WRF::GetErrCode() != 0) {
		WRF::SetErrMsg(
			"Error processing file %s, skipping.",infiles[i].c_str()
		);
        WRF::SetErrCode(0);
        continue;
    }
   
    wrf.GetWRFMeta(
      tdimlens,tstartdate,tmapprojection,twrfvars3d,
      twrfvars2d,tgl_attr,tt_latlon_exts
    );

	  usingRotLatLon = (tmapprojection.find("ob_tran") != string::npos);

// Grab the dx and dy and cen_lat, cen_lon values from attrib list;

      for(sf_pair_iter= tgl_attr.begin();sf_pair_iter< tgl_attr.end();sf_pair_iter++) {
        attr = *sf_pair_iter;
        if (attr.first == "DX") 
          tdx = attr.second;
        if (attr.first == "DY") 
          tdy = attr.second;
		if (attr.first == "CEN_LAT") 
          cen_lat = attr.second;
        if (attr.first == "CEN_LON") 
          cen_lon = attr.second;
      }
      if(tdx < 0.0 || tdy < 0.0) {
        ErrMsgStr.assign("Error: DX and DY attributes not found in ");
        ErrMsgStr += infiles[i];
        ErrMsgStr.append(", skipping.");
        SetErrMsg(ErrMsgStr.c_str());
        SetErrCode(0);
        continue;
      }

	  if(usingRotLatLon && (cen_lat == 1.e30f || cen_lon == 1.e30f)) {
        ErrMsgStr.assign("Error: Using Rotated Lat Lon but CEN_LAT and CEN_LON attributes not found in ");
        ErrMsgStr += infiles[i];
        ErrMsgStr.append(", skipping.");
        SetErrMsg(ErrMsgStr.c_str());
        SetErrCode(0);
        continue;
      }

      if (first_file_flag) {
        dx = tdx;
        dy = tdy;
        Vars3D = twrfvars3d;
        Vars2Dxy = twrfvars2d;
        Dimens[0] = tdimlens[0];
        Dimens[1] = tdimlens[1];
        Dimens[2] = tdimlens[2];
        StartDate = tstartdate;
        MapProjection = tmapprojection;
        Global_attrib = tgl_attr;

        first_file_flag = false;
      } // end first_file_flag.
      else {
        mismatch_flag = false;

// Check to see if things match between this file and 
// the template.

        if (Dimens[0] != tdimlens[0] || Dimens[1] != tdimlens[1] || Dimens[2] != tdimlens[2] )
          mismatch_flag = true;

		/*  Ignore mismatched dates
        if(StartDate != tstartdate)
          mismatch_flag = true;
		  */ 

        if(MapProjection != tmapprojection)
          mismatch_flag = true;

        if(Global_attrib.size() != tgl_attr.size())
          mismatch_flag = true;
        else {
          for(sf_pair_iter = Global_attrib.begin(); sf_pair_iter < Global_attrib.end(); sf_pair_iter++) {
            attr = *sf_pair_iter;
			//Don't require the CEN_LAT,CEN_LON to match
			if (attr.first == "CEN_LAT" || attr.first == "CEN_LON") continue;
            if (find(tgl_attr.begin(), tgl_attr.end(), attr) == tgl_attr.end()) {
              mismatch_flag = true;
            }
          }
        } // end of else Global_attr.

        if(Vars3D.size() != twrfvars3d.size())
          mismatch_flag = true;
        else {
          for(striter = Vars3D.begin(); striter < Vars3D.end(); striter++) {
            if (find(twrfvars3d.begin(), twrfvars3d.end(), *striter) == twrfvars3d.end()) {
              mismatch_flag = true;
            }
          }
        } // end of else Vars3D.

        if(Vars2Dxy.size() != twrfvars2d.size())
          mismatch_flag = true;
        else {
          for(striter = Vars2Dxy.begin(); striter < Vars2Dxy.end(); striter++) {
            if (find(twrfvars2d.begin(), twrfvars2d.end(), *striter) == twrfvars2d.end()) {
              mismatch_flag = true;
            }
          }
        } // end of else Vars2Dxy.

        if(mismatch_flag) {
          ErrMsgStr.assign("File mismatch, skipping file ");
          ErrMsgStr += infiles[i];
          SetErrMsg(ErrMsgStr.c_str());
          SetErrCode(0);
          continue;
        }
      } // end else first_file_flag.


// Copy over all the timesteps and lat-lon extents from this file.
// Save timestamp and filename for future lookup make sure that the
// timestamp is unique here.

      bool found_flag = false;
      for(int j = 0; j < tt_latlon_exts.size(); j++) {
        found_flag = false;
        Time_latlon_extents.push_back(tt_latlon_exts[j]);
        for(int k = 0; k < Timestep_filename.size(); k++) {
          if(Timestep_filename[k].first == tt_latlon_exts[j].first) {
            found_flag = true;
          }
        }
        if(!found_flag) {
          Timestep_filename.push_back(make_pair(tt_latlon_exts[j].first, infiles[i]));
          Timestep_offset.push_back(make_pair(tt_latlon_exts[j].first, j));
        }
      } // End of for j. 

  } // End for files to open (i).

  if (Timestep_filename.size() == 0) {
    SetErrMsg("No valid WRF input files");
    return;
  } 

// Finished all the input files, now can process over 
// all of the colllected data.


// sort the time steps and remove duplicates

  sort(Time_latlon_extents.begin(), Time_latlon_extents.end(), timeOrdering);
  tt_latlon_exts.clear();
  if(Time_latlon_extents.size())
    tt_latlon_exts.push_back(Time_latlon_extents[0]);
  for(size_t i = 1; i < Time_latlon_extents.size(); i++) {
    if(Time_latlon_extents[i].first != tt_latlon_exts.back().first)
      tt_latlon_exts.push_back(Time_latlon_extents[i]);
  }
  Time_latlon_extents = tt_latlon_exts; 

// Calculate the rest of the Extents.
  if (!usingRotLatLon){
	Extents[0] = 0.0; 
	Extents[1] = 0.0; 
	Extents[3] = dx * (Dimens[0] - 1); 
	Extents[4] = dy * (Dimens[1] - 1); 
  } else {
		//Make the rotated latlon center at the middle of the projection.
		Extents[0] = -0.5* dx * (Dimens[0] - 1); 
		Extents[1] = -0.5* dy * (Dimens[1] - 1); 
		Extents[3] = 0.5* dx * (Dimens[0] - 1); 
		Extents[4] = 0.5* dy * (Dimens[1] - 1); 
  }
	  

  

// Calculate the user time stamp.
// If there is the value for PLANET_YEAR in the
// global attributes the dealing with a planetwrf
// file which has a different string format and the
// timesteps need to adjusted to SI units with P2SI.
// While looping over everything, grab the mix and 
// max lat-lon values.  Also need to reproject these
// extents.

  string ttime_str;
  float p2si = 1.0;
  TIME64_T ts_now;
  int daysperyear = 0;
  for(int i = 0; i < Global_attrib.size(); i++) {
    if(Global_attrib[i].first == "PLANET_YEAR") 
      daysperyear = (int) Global_attrib[i].second;
  }
  if(daysperyear) {
    for(int i = 0; i < Global_attrib.size(); i++) {
      if(Global_attrib[i].first == "P2SI")
        p2si = Global_attrib[i].second;
    }
    for(int i = 0; i < Time_latlon_extents.size(); i++) {
      Time_latlon_extents[i].first = (TIME64_T) (Time_latlon_extents[i].first * p2si);
    }
    for(int i = 0; i < Timestep_filename.size(); i++) {
      Timestep_filename[i].first = (TIME64_T) (Timestep_filename[i].first * p2si);
      Timestep_offset[i].first = (TIME64_T) (Timestep_offset[i].first * p2si);
    }
  } // End if daysperyear.
  minLat = minLon = 500.0;
  maxLat = maxLon = -500.0;
  vector <float> llexts;
  for(int i = 0; i < Time_latlon_extents.size(); i++) {
    ttime_str.clear();
    ts_now = Time_latlon_extents[i].first;
    if(daysperyear) {
      ts_now = tt_latlon_exts[i].first;
      WRF::EpochToWRFTimeStr(ts_now, ttime_str, daysperyear);
    }
    else {
      WRF::EpochToWRFTimeStr(ts_now, ttime_str);
    }
    UserTimeStamps.push_back(ttime_str);
    llexts = Time_latlon_extents[i].second;
    if (llexts.size() == 10 ){ //Check all 4 corners for max and min longitude/latitude
      for (int cor = 0; cor < 4; cor++){
        if (llexts[cor*2] < minLon) minLon = llexts[cor*2];
        if (llexts[cor*2] > maxLon) maxLon = llexts[cor*2];
        if (llexts[cor*2+1] < minLat) minLat = llexts[cor*2+1];
        if (llexts[cor*2+1] > maxLat) maxLat = llexts[cor*2+1];
      }
    } // If llexts

  } // end for time_latlon_extents.
  if(ReprojectTsLatLon(MapProjection)) {
    ErrMsgStr.assign("Error in map reprojection.");
    SetErrMsg(ErrMsgStr.c_str());
    SetErrCode(0);	// Warning
  }

// Need to make sure that both HGT and ELEVATION variables are
// there for Vapor, add if needed.

  if(find(Vars3D.begin(),Vars3D.end(),"ELEVATION") == Vars3D.end()) {
    Vars3D.push_back("ELEVATION");
  }
  if(find(Vars2Dxy.begin(),Vars2Dxy.end(),"HGT") == Vars2Dxy.end()) {
    Vars2Dxy.push_back("HGT");
  }

} // End of constructor.

MetadataWRF::MetadataWRF(const vector<string> &infiles) {

	map <string, string> atypnames;	// no atypical names
	_MetadataWRF(infiles, atypnames);
}

MetadataWRF::MetadataWRF(
	const vector<string> &infiles, const map <string, string> &atypnames
) {
	_atypnames = atypnames;
	_MetadataWRF(infiles, _atypnames);
}

//
//! The destructor.
//
 
MetadataWRF::~MetadataWRF() {
  Global_attrib.clear();
  Vars3D.clear();
  Vars2Dxy.clear();
  UserTimeStamps.clear();
  UserTimes.clear();
  PeriodicBoundary.clear();
  GridPermutation.clear();
  StartDate.clear();
  MapProjection.clear();
  Time_latlon_extents.clear();
  Timestep_filename.clear();
  Timestep_offset.clear();
} // End of destructor.

int MetadataWRF::MapVDCTimestep(
  size_t timestep,
  string &wrf_fname,
  size_t &wrf_ts) {
  size_t i;
  TIME64_T tmp_usertime;
  int retval = 0;
  bool found_flag = false;

  if(Timestep_filename.size() == 0 ||
      Timestep_offset.size() == 0 ||
      Time_latlon_extents.size() == 0 )
    retval = -1;

  if( timestep < 0 || timestep >= Time_latlon_extents.size() )
    retval = -1;

  tmp_usertime = Time_latlon_extents[timestep].first;

  if (retval == 0) {
    for(i = 0; i < Timestep_offset.size() && !found_flag; i++) {
      if(Timestep_offset[i].first == tmp_usertime) {
        wrf_ts = Timestep_offset[i].second;
        found_flag = true;
      }
    }
    if (!found_flag)
      retval = -1;
  } // End of if.
  found_flag = false;
  if (retval == 0) {
    for(i = 0; i < Timestep_filename.size() && !found_flag; i++) {
      if(Timestep_filename[i].first == tmp_usertime) {
        wrf_fname = Timestep_filename[i].second;
        found_flag = true;
      }
    }
    if (!found_flag)
      retval = -1;
  } // End of if.
  if(retval == -1) {
    wrf_fname.clear();
    wrf_ts = 0;
  }

  return(retval);
} // End of MapVDCTimestep.

int MetadataWRF::MapWRFTimestep(
  const string &wrf_fname,
  size_t wrf_ts,
  size_t &timestep) {
  size_t tmp_usertime = 0;
  int retval = 0;
  bool found_flag = false;
  timestep = 0;

  for(size_t i = 0; i < Timestep_filename.size() && !found_flag; i++) {
    if ( Timestep_filename[i].second == wrf_fname) {
      tmp_usertime = Timestep_filename[i].first;
      for(size_t j = 0; j < Timestep_offset.size() && !found_flag; j++) {
        if(Timestep_offset[i].first == tmp_usertime && Timestep_offset[i].second == wrf_ts) {
          found_flag = true;
        }
      } // End for j ( offset) .
    } // End if.
  } // End for i (filename).

  if(found_flag) {
    bool done_flag = false;
    for(int i = 0; i < Time_latlon_extents.size() || !done_flag; i++) {
      if(Time_latlon_extents[i].first == tmp_usertime) {
        timestep = i;
        done_flag = true;
      }
    }
  }
  else
    retval = -1;

  return(retval);
} // End of MapWRFTimestep.

int MetadataWRF::ReprojectTsLatLon(string mapprojstr) {
  string ErrMsgStr;
  const double DEG2RAD = 3.141592653589793/180.;
  int retval = 0;

  Time_extents.clear();
  if (mapprojstr.size() > 0 ) {
    projPJ p = pj_init_plus(mapprojstr.c_str());
    if (!p) {
      //Invalid string. Get the error code:
      int *pjerrnum = pj_get_errno_ref();
      SetErrMsg("Invalid geo-referencing with proj string %s\n in WRF output : %s",
	    mapprojstr.c_str(),
        pj_strerrno(*pjerrnum));
    }
    if(p) {
	  // Check for rotated lat/lon.  It gets special attention
		
	  bool rotLatLonMapping = (mapprojstr.find("ob_tran") != string::npos);

      //reproject latlon to current coord projection.
      //Must convert to radians:
      string projwrkstr = "+proj=latlong";
      double radius = 0.0;

      for(int i = 0; i < Global_attrib.size(); i++ ) {
        if (Global_attrib[i].first == "RADIUS")
          radius = Global_attrib[i].second;
      }

      // If radius is > 0 then not dealing with Earth so need a different
      // projection string.

      if (radius > 0) {
        stringstream ss;
        ss << radius;
        projwrkstr += " +a=" + ss.str() + " +es=0";
      }
      else {
        projwrkstr += " +ellps=sphere";
      }

      const char* latLongProjString = projwrkstr.c_str();
      projPJ latlon_p = pj_init_plus(latLongProjString);
      if(!latlon_p) {
        ErrMsgStr.assign("Error in creating map reprojection string!");
        SetErrMsg(ErrMsgStr.c_str());
        pj_free(p);
        return(-1);
      }

      double dbextents[4];
      vector<double> currExtents;

      bool has_latlon = true;
      for ( size_t t = 0 ; t < Time_latlon_extents.size() ; t++ ){
		vector <float> exts = Time_latlon_extents[t].second;
          if (exts[0] == exts[2] && exts[1] == exts[3]) {
			has_latlon = false;
		} else if (rotLatLonMapping){
			  if (0!= GetRotatedLatLonExtents(dbextents))
				  SetErrMsg("Error calculating rotated lat lon domain extents");
		  } else {
			if (!exts.size()) continue;
			//exts 0,1,2,3 are the lonlat extents, other 4 corners can be ignored here
			for (int j = 0; j<4; j++)
			  dbextents[j] = exts[j]*DEG2RAD;
			if (radius == 0 ) {
			  retval = pj_transform(latlon_p,p,2,2, dbextents,dbextents+1, 0);
			  if (retval){
				int *pjerrnum = pj_get_errno_ref();
				SetErrMsg("Error geo-referencing WRF domain extents : %s",
				  pj_strerrno(*pjerrnum));
			  }
			} else { // PlanetWRF reporjection.
				for (int j = 0; j<4; j++)
				dbextents[j] *= radius;
			}
		  }
        
 
        //Put the reprojected extents into the vdf
        //Use the global specified extents for vertical coords

        if (has_latlon && ((dbextents[0] >= dbextents[2])||(dbextents[1] >= dbextents[3]))) {
          retval = -1;
          SetErrMsg(
            "Invalid geo-referencing WRF domain extents : min(%f,%f), max(%f,%f)",
            dbextents[0], dbextents[1], dbextents[2], dbextents[3]
          );
          SetErrCode(0);
        }

        if (!retval && has_latlon){
          currExtents.clear();
          currExtents.push_back(dbextents[0]);
          currExtents.push_back(dbextents[1]);
          currExtents.push_back(exts[8]);
          currExtents.push_back(dbextents[2]);
          currExtents.push_back(dbextents[3]);
          currExtents.push_back(exts[9]);
        }
		else {
          currExtents.clear();
          currExtents.push_back(Extents[0]);
          currExtents.push_back(Extents[1]);
          currExtents.push_back(exts[8]);
          currExtents.push_back(Extents[3]);
          currExtents.push_back(Extents[4]);
          currExtents.push_back(exts[9]);
		}
        Time_extents.push_back(make_pair(Time_latlon_extents[t].first, currExtents));
      } // End of for t.
      pj_free(p);
      pj_free(latlon_p);
    } // End if p .
  } // End if mapprojstr.size . 

  return (retval);
} // End of ReprojectTsLatLon.

//With rotated lat/lon, extents (in meters) requires a special reprojection
int MetadataWRF::GetRotatedLatLonExtents(double exts[4]){
	//Get DX,DY, CEN_LAT,CEN_LON out of Global_attrib
	const double DEG2RAD = 3.141592653589793/180.;
	const double RAD2DEG = 180./3.141592653589793;
	double dx=-1., dy=-1.;
	double grid_center[2] = {1.e30f,1.e30f};
	 for(int i = 0; i < Global_attrib.size(); i++ ) {
        if (Global_attrib[i].first == "DX")
          dx = (double)Global_attrib[i].second;
		if (Global_attrib[i].first == "DY")
          dy = (double)Global_attrib[i].second;
		if (Global_attrib[i].first == "CEN_LAT")
          grid_center[1] = (double)Global_attrib[i].second *DEG2RAD;
		if (Global_attrib[i].first == "CEN_LON")
          grid_center[0] = (double)Global_attrib[i].second *DEG2RAD;
      }
	 if (dx < 0 || dy < 0 || grid_center[0] == 1.e30f || grid_center[1] == 1.e30f){
		 SetErrMsg("Error in creating map reprojection string!");
		 return -1;
	 }
	//inverse project CEN_LAT/CEN_LON, that gives grid center in computational grid.
	 projPJ latlon_p = pj_init_plus("+proj=latlong +ellps=sphere");
	 const char* rotlatlonprojstr = MapProjection.c_str();
     projPJ rotlatlon_p = pj_init_plus(rotlatlonprojstr);
     if(!rotlatlon_p || !latlon_p) {
        SetErrMsg("Error in creating map reprojection string!");
        return(-1);
     }
	 int p = pj_transform(latlon_p,rotlatlon_p,1,1,grid_center, grid_center+1, 0);
	 if (p) return p;
	 grid_center[0]*=RAD2DEG;
	 grid_center[1]*=RAD2DEG;
	 //Convert grid center (currently in degrees from (-180, -90) to (180,90)) to 
	 //meters from origin
	 grid_center[0] = grid_center[0]*111177.;
	 grid_center[1] = grid_center[1]*111177.;
	
	//Use grid size to calculate extents
	exts[0] = grid_center[0]-0.5*(Dimens[0]-1)*dx;
	exts[2] = grid_center[0]+0.5*(Dimens[0]-1)*dx;
	exts[1] = grid_center[1]-0.5*(Dimens[1]-1)*dy;
	exts[3] = grid_center[1]+0.5*(Dimens[1]-1)*dy;
	return 0;
}

