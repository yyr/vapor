//
//      $Id$
//


#ifndef _WRF_h_
#define _WRF_h_


#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <netcdf.h>

using namespace VetsUtil;
using namespace VAPoR;

namespace VAPoR {

class VDF_API WRF : public VetsUtil::MyBase {
public:

 // A struct for storing info about a netCDF variable
 typedef struct varInfo {
	string name; // Name of the variable in the VDF and WRF file
	int varid; // Variable ID in netCDF file
	int dimids[NC_MAX_VAR_DIMS]; // Array of dimension IDs that this variable uses
	size_t dimlens[NC_MAX_VAR_DIMS]; // Array of dimensions 
	int ndimids; // Number of dimensions of which the variable is a function
	nc_type xtype; // The type of the variable (float, double, etc.)
	bool stag[3]; // Indicates which of the fastest varying three dimensions 
				// are staggered. N.B. order of array is reversed from
				// netCDF convection. I.e. stag[0] == X, stag[2] == Z.
 } varInfo_t;


 // structure for storing dimension info
 typedef struct {
    char name[NC_MAX_NAME+1];	// dim name
    size_t size;				// dim len
 } ncdim_t;

 // A struct for storing names of certain WRF variables
 typedef struct atypVarNames {
    string U;
    string V;
    string W;
    string PH;
    string PHB;
    string P;
    string PB;
    string T;
 } atypVarNames_t;



 // Gets info about a 3D variable and stores that info in thisVar.
 static int	GetVarInfo(
	int ncid, // ID of the file we're reading
	const char *name,
	const vector <ncdim_t> &ncdims,
	varInfo_t & thisVar // Variable info
 );

 // Reads a single horizontal slice of netCDF data
 static int ReadZSlice4D(
	int ncid, // ID of the netCDF file
	varInfo_t & thisVar, // Struct for the variable we want
	size_t wrfT, // The WRF time step we want
	size_t z, // Which z slice we want
	float * fbuffer, // Buffer we're going to store slice in
	const size_t * dim // Dimensions from VDF
 );





 // Reads a horizontal slice of the variable indicated by thisVar, 
 // and interpolates any points on a staggered grid to an unstaggered grid
 //
 static int GetZSlice(
	int ncid, // ID of netCDF file
	varInfo_t & thisVar, // varInfo_t struct for the variable we're interested in
	size_t wrfT, // The WRF time step we want
	size_t z, // The (unstaggered) vertical coordinate of the plane we want
	float * fbuffer, // Array into which we read the slice
	float * fbufferAbove, // Pointer to temporary array, needed if variable
						 // is staggered vertically.  If variable is not
						 // staggered vertically, pass a null pointer.
	bool & needAnother, // If variable is staggered vertically, this will be
					   // set to false after the first z slice is read, to
					   // reduce redundant reads
	const size_t * dim // Dimensions from VDF
);


 static int WRFTimeStrToEpoch(
	const string &wrftime,
	time_t *seconds
 );

 static int EpochToWRFTimeStr(
	time_t seconds,
	string &wrftime
 );


 static int OpenWrfGetMeta(
	const char * wrfName, // Name of the WRF file (in)
	const atypVarNames_t &atypnames,
	float & dx, // Place to put DX attribute (out)
	float & dy, // Place to put DY attribute (out)
	float * vertExts, // Vertical extents (out)
	size_t dimLens[4], // Lengths of x, y, and z dimensions (out)
	string &startDate, // Place to put START_DATE attribute (out)
	vector<string> & wrfVars, // Variable names in WRF file (out)
	vector <long> &timestamps // Time stamps, in seconds (out)
);

private:
 static void InterpHorizSlice(
	float * fbuffer, // The slice of data to interpolate
	varInfo_t & thisVar, // Data about the variable
	const size_t *dim // Dimensions from VDF
 );

};
};

#endif
