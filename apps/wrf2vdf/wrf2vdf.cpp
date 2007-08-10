//
//
//***********************************************************************
//                                                                       *
//                            Copyright (C)  2005                        *
//            University Corporation for Atmospheric Research            *
//                            All Rights Reserved                        *
//                                                                       *
//***********************************************************************/
//
//      File:		ncdf2vdf.cpp
//
//      Author:         Alan Norton
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           May 7, 2007
//
//      Description:	Read a WRF file containing a 3D array of floats or doubles
//			and insert the volume into an existing
//			Vapor Data Collection
//
//		Heavily modified: July 2007, Victor Snyder


#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cmath>
#include <ctime>
#include <netcdf.h>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/Metadata.h>
#include <vapor/WaveletBlock3DBufWriter.h>
#include <vapor/WaveletBlock3DRegionWriter.h>
#ifdef WIN32
#include "windows.h"
#endif

using namespace VetsUtil;
using namespace VAPoR;

const int MAX_WRF_TS = 1000; // Maximum number of time steps you think
							 // we'd find in a single WRF output file
const int TSWIDTH = 22; // Width of Time Stamp output field
const int CONVWIDTH = 13; // Width of Converted? output field

#define NC_ERR_READ(nc_status) \
    if (nc_status != NC_NOERR) { \
        fprintf(stderr, \
            "%s: Error reading netCDF file at line %d : %s \n",  ProgName, __LINE__, nc_strerror(nc_status) \
        ); \
    exit(1);\
    }

//
//	Command line argument stuff
//
struct opt_t {
	vector<string> varnames;
	vector<string> dervars;
	vector<string> atypvars;
	int tsincr;
	char * tsstart;
	char * tsend;
	int level;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"varnames",1, 	"novars",	"Colon delimited list of variable names in\n\t\t\t\tmetadata to convert"},
	{"atypvars",1,	"noatyps",	"Colon delimited list of atypical names for\n\t\t\t\tU:V:W:PH:PHB:P:PB:T that appear in WRF file"},
	{"dervars",	1,	"nodervars","Colon delimited list of derived variables\n\t\t\t\tto convert.  Choices are:\n\t\t\t\tphnorm: normalized geopotential (PH+PHB)/PHB\n\t\t\t\twind3d: 3D wind speed (U^2+V^2+W^2)^1/2\n\t\t\t\twind2d: 2D wind speed (U^2+V^2)^1/2\n\t\t\t\tpfull: full pressure P+PB\n\t\t\t\tpnorm: normalized pressure (P+PB)/PB\n\t\t\t\ttheta: potential temperature T+300\n\t\t\t\ttk: temp. in Kelvin 0.037*Theta*P_full^0.29"},
	{"tsincr",	1,	"1","Increment between Vapor times steps to convert\n\t\t\t\t(e.g., 3=every third), from Vapor time step 0"},
	{"tsstart", 1,  "???????", "Starting time stamp for conversion (default is\n\t\t\t\tfound in VDF"},
	{"tsend",	1,  "9999-12-31_23:59:59", "Last time stamp to convert (default is latest\n\t\t\t\ttime stamp)"},
	{"level",	1, 	"-1","Refinement levels saved. 0=>coarsest, 1=>next\n\t\t\t\trefinement, etc. -1=>finest"},
	{"help",	0,	"",	"Print this message and exit"},
	{"debug",	0,	"",	"Enable debugging"},
	{"quiet",	0,	"",	"Operate quietly (outputs only vertical extents\n\t\t\t\tthat are lower than those in the VDF)"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"varnames", VetsUtil::CvtToStrVec, &opt.varnames, sizeof(opt.varnames)},
	{"atypvars", VetsUtil::CvtToStrVec, &opt.atypvars, sizeof(opt.atypvars)},
	{"dervars", VetsUtil::CvtToStrVec, &opt.dervars, sizeof(opt.dervars)},
	{"tsincr", VetsUtil::CvtToInt, &opt.tsincr, sizeof(opt.tsincr)},
	{"tsstart", VetsUtil::CvtToString, &opt.tsstart, sizeof(opt.tsstart)},
	{"tsend", VetsUtil::CvtToString, &opt.tsend, sizeof(opt.tsend)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};


// A struct for storing info about a netCDF variable
struct varInfo
{
	const char * name; // Name of the variable in the VDF and WRF file
	int varid; // Variable ID in netCDF file
	int dimIndex[3]; // Specifies which netCDF dimension index goes with x,y, and z
	int dimids[NC_MAX_VAR_DIMS]; // Array of dimension IDs that this variable uses
	nc_type xtype; // The type of the variable (float, double, etc.)
	int ndimids; // Number of dimensions of which the variable is a function
	bool stag[3]; // Indicates which of the dimensions are staggered
	size_t sliceSize; // Size of a z slice before any interpolation of staggered grids
};

// A struct for storing names of certain WRF variables
struct WRFNames
{
	const char * U;
	const char * V;
	const char * W;
	const char * PH;
	const char * PHB;
	const char * P;
	const char * PB;
	const char * T;
} wrfNames;



const char	*ProgName;

//
// Backup a .vdf file
//
void save_file(const char *file) {
	FILE	*ifp, *ofp;
	int	c;

	string oldfile(file);
	oldfile.append(".old");

	ifp = fopen(file, "rb");
	if (! ifp) {
		cerr << ProgName << ": Could not open file \"" << 
			file << "\" : " <<strerror(errno) << endl;
		exit(1);
	}

	ofp = fopen(oldfile.c_str(), "wb");
	if (! ifp) {
		cerr << ProgName << ": Could not open file \"" << 
			oldfile << "\" : " <<strerror(errno) << endl;
		exit(1);
	}

	do {
		c = fgetc(ifp);
		if (c != EOF) c = fputc(c,ofp); 

	} while (c != EOF);

	if (ferror(ifp)) {
		cerr << ProgName << ": Error reading file \"" << 
			file << "\" : " <<strerror(errno) << endl;
		exit(1);
	}

	if (ferror(ofp)) {
		cerr << ProgName << ": Error writing file \"" << 
			oldfile << "\" : " <<strerror(errno) << endl;
		exit(1);
	}
}




// Sets up a writer for a variable and opens that variable for writing.
// You must close the variable yourself later, when you're done with it.
void MakeWritersOpen( WaveletBlock3DIO * & varWriter, // Not graceful?
					  WaveletBlock3DBufWriter * & varBufWriter,
					  size_t aVaporTs, // Vapor time step to open
					  const char * varName, // For the variable you want to write
					  const char * metafile // Path of VDF
					 )
{
	varWriter = new WaveletBlock3DBufWriter(metafile, 0);
	if (varWriter->GetErrCode() != 0) {
		cerr << ProgName << " : " << varWriter->GetErrMsg() << endl;
		exit(1);
	}
	// Must typecast phWriter so that we can use it for writing
	varBufWriter = (WaveletBlock3DBufWriter *) varWriter;
	// Open a variable for writing at the indicated time step
	if (varWriter->OpenVariableWrite(aVaporTs, varName, opt.level) < 0)
	{
		cerr << ProgName << " : " << varWriter->GetErrMsg() << endl;
		exit(1);
	}
}




// Closes a variable after you're done writing to it, and checks for errors
void CloseVar( WaveletBlock3DIO * & varWriter
			  )
{
	varWriter->CloseVariable();
	if (varWriter->GetErrCode() != 0)
	{
		cerr << ProgName << ": " << varWriter->GetErrMsg() << endl;
		exit(1);
	}
	delete varWriter; // Subtle, but can cause a HUGE memory leak
}



// Takes the start through the (start+lenth-1) elements of fullTime and puts
// them into pieceTime, with a null character on the end.  N.B., pieceTime must
// be of size at least length+1.
void FillTimeStr(
				  char * fullTime, // String with 19-character time stamp
				  char * pieceTime, // String where we'll store a certain piece
				  int start, // Index of fullTime to start the reading
				  int length // How many characters to read
				 )
{
	int i = 0; // Index on fullTime
	int j = 0; // Index on pieceTime
	
	for( i = start ; i < (start + length) ; i++, j++ )
	{
		// Make sure we're reading digits
		if ( isdigit( fullTime[i] ) )
			pieceTime[j] = fullTime[i];
		else
		{
			fprintf( stderr, "Non-numeric character in date or time\n" );
			exit( 1 );
		}
	
		// Make sure pieceTime ends with a null character
		if ( i == (start + length - 1) )
			pieceTime[j + 1] = '\0';
	}
}






// Computes the difference in seconds between two of the 19-character time
// stamps that WRF output provides.  Accounts for leap-years.  
size_t DiffStrTimes( 
					char * later, // The later of the two dates
					char * earlier, // The earlier of the two
					bool & ok // true if earlier really is earlier than later,
							  // false otherwise
					)
{
	ok = true;
	// These hold integer values of the year, month, day, hour, minute, second
	int laterYear, earlierYear, laterMonth, earlierMonth, laterDay, earlierDay,
	    laterHour, earlierHour, laterMin, earlierMin, laterSec, earlierSec;

	// We'll chop up 'later' and 'earlier' and put them into these arrays
	char laterYearS[5], earlierYearS[5], laterMonthS[3], earlierMonthS[3],
		 laterDayS[3], earlierDayS[3], laterHourS[3], earlierHourS[3],
		 laterMinS[3], earlierMinS[3], laterSecS[3], earlierSecS[3];

	// These will hold time info for use with ctime library stuff
	tm laterTm;
    tm earlierTm;

	double diffDub; // Holds the result of a difftime
	size_t diff; // That result rounded off

	// Do that chopping
	FillTimeStr( later, laterYearS, 0, 4 );
	FillTimeStr( earlier, earlierYearS, 0, 4 );
	FillTimeStr( later, laterMonthS, 5, 2 );
	FillTimeStr( earlier, earlierMonthS, 5, 2 );
	FillTimeStr( later, laterDayS, 8, 2 );
	FillTimeStr( earlier, earlierDayS, 8, 2 );
	FillTimeStr( later, laterHourS, 11, 2 );
	FillTimeStr( earlier, earlierHourS, 11, 2 );
	FillTimeStr( later, laterMinS, 14, 2 );
	FillTimeStr( earlier, earlierMinS, 14, 2 );
	FillTimeStr( later, laterSecS, 17, 2 );
	FillTimeStr( earlier, earlierSecS, 17, 2 );
	
	// Turn strings into integers
	laterYear = atoi( laterYearS );
	earlierYear = atoi( earlierYearS );
	laterMonth = atoi( laterMonthS );
	earlierMonth = atoi( earlierMonthS );
	laterDay = atoi( laterDayS );
	earlierDay = atoi( earlierDayS );
	laterHour = atoi( laterHourS );
	earlierHour = atoi( earlierHourS );
	laterMin = atoi( laterMinS );
	earlierMin = atoi( earlierMinS );
	laterSec = atoi( laterSecS );
	earlierSec = atoi( earlierSecS );
	
	// For now, only handle years 1970 and after
	if ( laterYear < 1970 || earlierYear < 1970 )
	{
		fprintf( stderr, "Cannot handle times earlier than 1970\n" );
		exit( 1 );
	}

	// Fill the tm structs with the proper values
	laterTm.tm_year = laterYear - 1900;
	earlierTm.tm_year = earlierYear - 1900;
	laterTm.tm_mon = laterMonth - 1;
	earlierTm.tm_mon = earlierMonth - 1;
	laterTm.tm_mday = laterDay;
	earlierTm.tm_mday = earlierDay;
	laterTm.tm_hour = laterHour;
	earlierTm.tm_hour = earlierHour;
	laterTm.tm_min = laterMin;
	earlierTm.tm_min = earlierMin;
	laterTm.tm_sec = laterSec;
	earlierTm.tm_sec = earlierSec;
	laterTm.tm_isdst = -1;
	earlierTm.tm_isdst = -1;
	
	diffDub = difftime( mktime( &laterTm ), mktime( &earlierTm ) );
	diff = (size_t)(diffDub);
	// Round off the answer correctly
	if ( diffDub - (double)(diff) > 0.5 )
		diff++;
	
	// Make sure earlier really is earlier, but don't exit if it's not
	if ( laterYear > earlierYear )
		return diff;
	if ( laterYear == earlierYear ) {
		if ( laterMonth > earlierMonth )
			return diff;
		if ( laterMonth == earlierMonth ) {
			if ( laterDay > earlierDay )
				return diff;
			if ( laterDay == earlierDay ) {
				if ( laterHour > earlierHour )
					return diff;
				if ( laterHour == earlierHour ) {
					if ( laterMin > earlierMin )
						return diff;
					if ( laterMin == earlierMin ) {
						if ( laterSec >= earlierSec )
							return diff;
	}	}	}	}	}
				
	// If we make it down here, the given later time actually preceeds the given
	// earlier time.  Make a note of it, but don't exit with error.
	ok = false;
	return diff; // Should return something; it's garbage, though
}



// Opens a WRF netCDF file, gets the file's ID, the number of dimensions,
// and gets the values of the variable Times, and determines what Vapor
// time steps the WRF file contains
void OpenWrfFile(
				 const char * netCDFfile, // Path of WRF output file to be opened (input)
				 int & ncid, // ID of netCDF file (output)
				 int & ndims, // Number of dimensions in the file (output)
				 size_t & howManyTimes, // Number of times appearing in file (output)
				 size_t * vaporTs, // Vapor time steps in file (output)
				 vector<string> & tStamps, // Time stamps in WRF file (output)
				 bool * tsOk, // Element is true if corresponding Vapor time should be converted (output)
				 long minDeltaT, // Minimum number of seconds between any two time steps (input)
				 char * startTime, // Earliest time stamp to convert (input)
				 char * endTime, // Latest time stamp to convert (input)
				 long maxVts, // The last Vapor time step (input)
				 char * tsNaught // Time stamp corresponding to Vapor's step 0 (input)
				 )
{
	int nc_status;		// Holds error codes for debugging
	int nvars;          // number of variables (not used)
	int ngatts;         // number of global attributes
	int xdimid;         // id of unlimited dimension (not used)
	int timesId;		// ID of Times variable
	int timesDimsIds[NC_MAX_VAR_DIMS]; // Dimension ID's used by Times
	char tempName1[NC_MAX_NAME + 1]; // Just a placeholder
	char tempName2[NC_MAX_NAME + 1]; // Just a placeholder
	nc_type timesType;  // Times' type of variable (better be some kind of char)
	int nTimesDims;     // Number of dimensions Times is defined on
	int nTimesAtts;		// Number of attributes associate with Times
	size_t strLen;			// Length of the date-time string in the Times variable
	int timeId;			// ID of Time dimension
	int dateStrLenId;	// ID of DateStrLen dimension
	int timeSpot;		// 1 or 0, depending whether or not Time is the first or second
						// dimension of Times
	int dateStrLenSpot;	// 1 or 0, depending whether or not DateStrLen is the first
						// or second dimension of Times

	// Open netCDF file and check for failure
	NC_ERR_READ( nc_status = nc_open(netCDFfile, NC_NOWRITE, &ncid ) );

	// Find the number of dimensions, variables, and global attributes, and check
	// the existance of the unlimited dimension
	NC_ERR_READ( nc_status = nc_inq(ncid, &ndims, &nvars, &ngatts, &xdimid ) );

	// Look for the variable "Times" and get info about it so that we can read it
	NC_ERR_READ( nc_status = nc_inq_varid( ncid, "Times", &timesId ) );
	NC_ERR_READ( nc_status = nc_inq_var( ncid, timesId, tempName1, &timesType, &nTimesDims,
							 timesDimsIds, &nTimesAtts ) );
	
	// Times better have two dimensions
	if ( nTimesDims != 2)
	{
		fprintf( stderr, "Times has %d dimensions; expecting 2\n", nTimesDims );
		exit( 1 );
	}

	NC_ERR_READ( nc_status = nc_inq_dim( ncid, timesDimsIds[0], tempName1, &howManyTimes ) );
	NC_ERR_READ( nc_status = nc_inq_dim( ncid, timesDimsIds[1], tempName2, &strLen ) );
	// Expecting two particular dimensions
	if ( (strcmp( tempName1, "Time" ) != 0 && strcmp( tempName1, "DateStrLen" ) != 0)
		|| (strcmp( tempName2, "Time" ) != 0 && strcmp( tempName2, "DateStrLen" ) != 0) )
	{	
		fprintf( stderr, "WRF variable Times has unexpected dimensions\n" );
		exit( 1 );
	}
	else if ( strcmp( tempName1, "Time" ) == 0 && strcmp( tempName2, "DateStrLen" ) == 0 )
	{
		timeId = timesDimsIds[0];
		dateStrLenId = timesDimsIds[1];
		timeSpot = 0;
		dateStrLenSpot = 1;
	}
	else
	{
		int temp = strLen;
		strLen = howManyTimes;
		howManyTimes = temp;
		timeId = timesDimsIds[1];
		dateStrLenId = timesDimsIds[0];
		timeSpot = 1;
		dateStrLenSpot = 0;
	}

	// If there's too many time steps in this file, you'll have to readjust and
	// recompile with a different MAX_WRF_TS global constant
	if ( howManyTimes > MAX_WRF_TS )
	{
		fprintf( stderr, "Number of time steps in file exceeds assumed maximum possible (%d)\n",
				 MAX_WRF_TS );
		fprintf( stderr, "\n\tAdjust MAX_WRF_TS in source and rebuild\n" );
		exit( 1 );
	}
	// If the date-time strings aren't 19 characters long, we might not 
	// recognize them
	if ( strLen != 19 )
	{
		fprintf (stderr, "Expected 19 characters in a Times value, not %d\n", strLen );
		exit( 1 );
	}

	// Array to hold times in c-string form
	char ** charTimes = new char*[howManyTimes*(strLen+1)];
	for ( int i = 0 ; i < howManyTimes ; i++ )
		charTimes[i] = new char[strLen+1];

	// Set up start and count arrays for reading Times
	size_t start[2] = { 0, 0 };
	size_t count[2];
	count[timeSpot] = 1;
	count[dateStrLenSpot] = strLen;
	// Read all values of Times into our array for them
	for ( int i = 0 ; i < howManyTimes ; i++ )
	{
		NC_ERR_READ( nc_status = nc_get_vara_text( ncid, timesId, start, count, charTimes[i] ) );
		charTimes[19] = '\0';
		start[timeSpot]++;
		tStamps.push_back( charTimes[i] );
	}
	
	bool tsOkTemp; // Temporary variable
	// Now convert charTimes to Vapor time steps
	for ( int i = 0 ; i < howManyTimes ; i++ )
	{
		vaporTs[i] = DiffStrTimes( charTimes[i], tsNaught, tsOk[i] ); // Not a Vapor time step YET
		if ( vaporTs[i] % minDeltaT != 0 && tsOk[i] == true )
		{
			fprintf( stderr, "Minimum time step size incorrect, won't convert\n" );
			fprintf( stderr, "\tfractional Vapor time steps" );
			exit( 1 );
		}
		if ( tsOk[i] == true ) // Only convert if desired time step is or comes after
		{					   // Vapor's time step 0
			vaporTs[i] /= minDeltaT;
			// If a time step isn't one allowed by the time step increment chosen by
			// the user, set it to -1 so that we don't convert it
			if ( vaporTs[i] % opt.tsincr != 0 )
			{
				tsOk[i] = false;
				continue;
			}
			// Now see if this time step comes after the last desired or possible time
			DiffStrTimes( endTime, charTimes[i], tsOkTemp );
			if ( tsOkTemp == false || vaporTs[i] > maxVts )
			{
				tsOk[i] = false;
				continue;
			}
			// See if this time step comes before first desired time step
			DiffStrTimes( charTimes[i], startTime, tsOkTemp );
			if ( tsOkTemp == false )
				tsOk[i] = false;
		}
	}

	for ( int i = 0 ; i < howManyTimes ; i++ ) // Cleanup
		delete [] charTimes[i];
	delete [] charTimes;
}



// Gets info about a variable and stores that info in thisVar.
void GetVarInfo(
				 int ncid, // ID of the file we're reading
				 int ndims, // Number of dimensions
				 varInfo & thisVar, // Variable info
				 const size_t *dim //dimensions from vdf
				 )
{
	int nc_status; // Holds error codes

	size_t* ncdfdims; // The variable's dimension from the netCDF file
	
	//Check on the variable we are going to read; if found, store its ID in varid
	nc_status = nc_inq_varid(ncid, thisVar.name, &thisVar.varid);
	if (nc_status != NC_NOERR){
		fprintf(stderr, "variable %s not found in netcdf file\n", thisVar.name);
		exit(1);
	}

	//allocate array for dimensions in the netcdf file:
	ncdfdims = new size_t[ndims];

	//Get all the dimensions in the file:
	for (int d=0; d<ndims; d++) {
		nc_status = nc_inq_dimlen(ncid, d, &ncdfdims[d]);
		NC_ERR_READ(nc_status);
	}

	char name[NC_MAX_NAME+1]; // Will hold a variable's name
	int natts; // Number of attributes associated with this variable

	// For that variable, find all that stuff above
	nc_status = nc_inq_var( ncid, thisVar.varid, name, &thisVar.xtype, &thisVar.ndimids,
							thisVar.dimids, &natts );
	// Check for failure
	NC_ERR_READ(nc_status);

	// Check for valid variable type
	if ( thisVar.xtype != NC_FLOAT && thisVar.xtype != NC_DOUBLE )
	{
		cerr << ProgName << ": Invalid variable data type" << endl;
		exit(1);
	}

	// Need at least 3 dimensions
	if (thisVar.ndimids < 3) {
		cerr << ProgName << ": Insufficient netcdf variable dimensions" << endl;
		exit(1);
	}

	bool foundXDim = false, foundYDim = false, foundZDim = false;
	int dimIDs[3];// dimension ID's (in netcdf file) for each of the 3 dimensions we are using
	
	// Check on constant dimension
	int constDimID;
	if (nc_inq_dimid(ncid, "Time", &constDimID) != NC_NOERR)
	{
		fprintf(stderr, "Time dimension not found in WRF netCDF file\n" );
		exit(1);
	}
					
	//Go through the dimensions looking for the 3 dimensions that we are using,
	for (int i = 0; i<thisVar.ndimids; i++)
	{
		//For each dimension id, get the name associated with it
		nc_status = nc_inq_dimname(ncid, thisVar.dimids[i], name);
		NC_ERR_READ(nc_status);
		//See if that dimension name is a coordinate of the desired array.
		if (!foundXDim){
			if (strcmp(name, "west_east") == 0 || strcmp(name, "west_east_stag") == 0){
				foundXDim = true;
				dimIDs[0] = thisVar.dimids[i];
				thisVar.dimIndex[0] = i;
				//Allow dimension to be off by 1, in case of 
				//staggered dimensions.
				if (ncdfdims[dimIDs[0]] != dim[0] &&
					ncdfdims[dimIDs[0]] != (dim[0]+1)){
					fprintf(stderr, "NetCDF and VDF array do not match in dimension 0\n");
					exit(1);
				}
				// Set a flag if this dimension is staggered
				if ( ncdfdims[dimIDs[0]] == (dim[0] + 1) )
					thisVar.stag[0] = true;
				else
					thisVar.stag[0] = false;
				continue;
			} 
		} 
		if (!foundYDim) {
			if (strcmp(name, "south_north") == 0 || strcmp(name, "south_north_stag") == 0){
				foundYDim = true;
				dimIDs[1] = thisVar.dimids[i];
				thisVar.dimIndex[1] = i;
				if (ncdfdims[dimIDs[1]] != dim[1] &&
					ncdfdims[dimIDs[1]] != (dim[1]+1)){
					fprintf(stderr, "NetCDF and VDF array do not match in dimension 1\n");
					exit(1);
				}
				if ( ncdfdims[dimIDs[1]] == (dim[1] + 1) )
					thisVar.stag[1] = true;
				else
					thisVar.stag[1] = false;
				continue;
			}
		} 
		if (!foundZDim) {
			if (strcmp(name, "bottom_top") == 0 || strcmp(name, "bottom_top_stag") == 0){
				foundZDim = true;
				thisVar.dimIndex[2] = i;
				dimIDs[2] = thisVar.dimids[i];
				if (ncdfdims[dimIDs[2]] != dim[2] &&
					ncdfdims[dimIDs[2]] != (dim[2]+1)){
					fprintf(stderr, "NetCDF and VDF array do not match in dimension 2\n");
					exit(1);
				}
				if ( ncdfdims[dimIDs[2]] == (dim[2] + 1) )
					thisVar.stag[2] = true;
				else
					thisVar.stag[2] = false;
				continue;
			}
		}
		
	}
	if (!foundZDim || !foundYDim || !foundXDim){
		fprintf(stderr, "Specified Netcdf dimension name not found");
		exit(1);
	}


	// The size of a slice before any interpolation
    thisVar.sliceSize = 1;
	if ( thisVar.stag[0] )
		thisVar.sliceSize *= (dim[0] + 1);
	else
		thisVar.sliceSize *= dim[0];
	if ( thisVar.stag[1] )
		thisVar.sliceSize *= (dim[1] + 1);
	else
		thisVar.sliceSize *= dim[1];

	delete [] ncdfdims;
}



// Reads a single horizontal slice of netCDF data
void ReadZSlice(
				int ncid, // ID of the netCDF file
				varInfo & thisVar, // Struct for the variable we want
				size_t wrfT, // The WRF time step we want
				int z, // Which z slice we want
				float * fbuffer, // Buffer we're going to store slice in
				const size_t * dim // Dimensions from VDF
				)
{
	// Make sure this is an OK z slice before we waste any time
	if ( z > dim[2] )
		if ( !thisVar.stag[2] || (thisVar.stag[2] && z > dim[2] + 1) )
		{
			fprintf( stderr, "z slice %d exceeds dimension length\n", z );
			exit( 1 );
		}
		

	size_t* start; // The point from which we start reading netCDF data
	size_t* count; // What interval to read the data
	int nc_status; // Holds netCDF error codes

	// Initialize the count and start arrays for extracting slices from the data:
	count = new size_t[thisVar.ndimids];
	start = new size_t[thisVar.ndimids];
	for (int i = 0; i<thisVar.ndimids; i++){
		start[i] = 0; // Start reading in a corner
		count[i] = 1; // Read every point (changes later)
	}

	char name[NC_MAX_NAME+1]; // Will hold the dimension's name

	// Set start so that we start at the correct value of the constant dimensions
	for ( int i = 0 ; i < thisVar.ndimids ; i++ )
	{
		//For each dimension id, get the name associated with it
		NC_ERR_READ( nc_status = nc_inq_dimname(ncid, thisVar.dimids[i], name) );
		if (strcmp( name, "Time" ) == 0)
		{
			start[i] = wrfT;
			break;
		}
	}

	// Need this array in case we're reading in doubles instead of floats
	double *dbuffer = 0;
	if (thisVar.xtype == NC_DOUBLE) 
		dbuffer = new double[thisVar.sliceSize];

	//
	// Translate the volume one slice at a time
	//
	// Set up counts to only grab a z-slice
	for (int i = 0; i < thisVar.ndimids; i++)
	{
		count[i] = 1;
		if ( i == thisVar.dimIndex[0] )
		{
			if ( thisVar.stag[0] ) // Grab all x values
				count[i] = dim[0] + 1;
			else
				count[i] = dim[0]; 
		}
		if ( i == thisVar.dimIndex[1] )
		{
			if ( thisVar.stag[1] ) // Grab all y values
				count[i] = dim[1] + 1;
			else
				count[i] = dim[1];
		}
	}
	count[thisVar.dimIndex[2]] = 1; // Grab only 1 z value
	start[thisVar.dimIndex[2]] = z;
		
	if (thisVar.xtype == NC_FLOAT)
	{
		NC_ERR_READ( nc_status = nc_get_vara_float( ncid, thisVar.varid, start, count, fbuffer ) );
	}
	else if (thisVar.xtype == NC_DOUBLE)
	{
		NC_ERR_READ( nc_status = nc_get_vara_double( ncid, thisVar.varid, start, count, dbuffer ) );
		//Convert to float:
		for(int i=0; i<dim[0]*dim[1]; i++)
			fbuffer[i] = (float)dbuffer[i];
	}

	delete [] count;
	delete [] start;
	if ( dbuffer )
		delete [] dbuffer;
}



void InterpHorizSlice(
					  float * fbuffer, // The slice of data to interpolate
					  varInfo & thisVar, // Data about the variable
					  const size_t *dim // Dimensions from VDF
					  )
{
	// NOTE: As of July 20, 2007 (when this function was written), interpolating
	// data that is staggered in both horizontal dimensions has not been tested.

	size_t xUnstagDim = dim[0]; // Define these just for convenience
	size_t xStagDim = dim[0] + 1;
	size_t yUnstagDim = dim[1];
	size_t yStagDim = dim[1] + 1;

	size_t xDimWillBe = xUnstagDim; // More convenience
	size_t xDimNow, yDimNow;
	if ( thisVar.stag[0] )
		xDimNow = xStagDim;
	else
		xDimNow = xUnstagDim;

	if ( thisVar.stag[1] )
		yDimNow = yStagDim;
	else
		yDimNow = yUnstagDim;
	
	// Interpolate a staggered (N+1 points) x variable to an unstaggered (N points) x grid
	if ( thisVar.stag[0] )
	{
		size_t x = 0; // x grid point in staggered grid
		size_t y = 0; // y grid point in y's (unstaggered) grid
		float avg = 0.0; // Holds average
		for ( size_t l = 0 ; l < xDimWillBe*yDimNow ; l++ ) // l indexes storage location
		{
			if ( x < xDimWillBe ) // For all but the last x point for a given y
			{
				avg = (fbuffer[x + xDimNow*y] + fbuffer[x + 1 + xDimNow*y])/2.0;
				fbuffer[l] = avg;
				x++;
			}
			else // This is for the last x point for a given y
			{
				y++;
				avg = (fbuffer[y*xDimNow] + fbuffer[1 + y*xDimNow])/2.0;
				fbuffer[l] = avg;
				x=1;
			}
		}
	}

	// Interpolate a staggered (N+1 points) y variable to an unstaggered (N points) y grid
	if ( thisVar.stag[1] )
	{
		size_t x = 0; // x grid point in staggered grid
		size_t y = 0; // y grid point in y's (unstaggered) grid
		float avg = 0.0; // Holds average
		for ( size_t l = 0 ; l < xUnstagDim*(yUnstagDim - 1) ; l++ ) // l indexes storage location
		{
			avg = (fbuffer[x + xUnstagDim*y] + fbuffer[x + (y + 1)*xUnstagDim])/2.0;
			fbuffer[l] = avg;
			if ( x < xUnstagDim - 1 ) // For all but the last x point for a given y point
				x++;
			else // If this happens, you're at the last x value for a given y
			{
				x = 0;
				y++;
			}
		}
	}
}





// Reads a horizontal slice of the variable indicated by thisVar, and interpolates any
// points on a staggered grid to an unstaggered grid
void GetZSlice(
			   int ncid, // ID of netCDF file
			   varInfo & thisVar, // varInfo struct for the variable we're interested in
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
			   )
{
	// Read a slice from the netCDF
	if ( needAnother )
	{
		ReadZSlice( ncid, thisVar, wrfT, z, fbuffer, dim );
		// Do horizontal interpolation of staggered grids, if necessary
		if ( thisVar.stag[0] || thisVar.stag[1] )
			InterpHorizSlice( fbuffer, thisVar, dim );
	}

	// If the vertical grid is staggered, we'll need the slice above
	// for interpolation
	if ( thisVar.stag[2] )
	{
		// If the vertical grid is staggered, save the slice above the one we
		// just wrote as the one we are going to work on next time
		if ( !needAnother )
		{
			float * tempPtr = fbuffer;
			fbuffer = fbufferAbove;
			fbufferAbove = tempPtr;
		}

		ReadZSlice( ncid, thisVar, wrfT, z+1, fbufferAbove, dim );
		// Iterpolate horizontally, if needed
		if ( thisVar.stag[0] || thisVar.stag[1] )
			InterpHorizSlice( fbufferAbove, thisVar, dim );
		// Now do the vertical interpolation
		for ( size_t l = 0 ; l < dim[0]*dim[1] ; l++ )
		{
			fbuffer[l] += fbufferAbove[l];
			fbuffer[l] /= 2.0;
		}
		// Now we no longer need to read two slices
		if ( needAnother )
			needAnother = false;
	}
}




// Calculates needed and desired quantities involving PH and PHB--elevation (needed for
// on-the-fly interpolation in Vapor) and normalized geopotential,
// and the original variables.
void DoGeopotStuff(
				   int ncid, // ID of netCDF file
				   vector<varInfo> desiredVars, // Vector of varInfo structs corresponding to
				    							// the desired WRF variables
				   size_t aVaporTs, // Vapor time step we're currently working on
				   int wrfT, // WRF time step we're currently working on
				   bool * stillNeeded, // Array says whether or not a variable (same index as
									   // in desiredVars) still needs to be written
				   const char * metafile, // Path and file name of VDF
				   int ndims, // Number of dimensions in netCDF file
				   const size_t * dim, // Dimensions of VDF
				   float * theseExts, // Array for lowest vertical extents for this time step only
				   float * newExts, // Array for lowest extents
				   bool wantPhnorm // True if you want to output PH_norm, false otherwise
        	       )
{
	size_t phIndex = 0; // Indices of desiredVars corresponding to PH and PHB
	size_t phbIndex = 0;
	bool wantPh = false; // Whether or not user wants to output PH, PHB
	bool wantPhb = false;

	varInfo * phInfoPtr = 0; // Still need to decide where to point these at
	varInfo * phbInfoPtr = 0;
	
	// See if user wants to output PH and/or PHB
	for ( size_t i = 0 ; i < desiredVars.size() ; i++ )
	{
		if ( strcmp( desiredVars[i].name, wrfNames.PH ) == 0 ) // User wants PH output
		{
			phIndex = i;
			phInfoPtr = &desiredVars[i];
			wantPh = true;
			continue;
		}
		if ( strcmp( desiredVars[i].name, wrfNames.PHB ) == 0 ) // User wants PHB output
		{
			phbIndex = i;
			phbInfoPtr = &desiredVars[i];
			wantPhb = true;
			continue;
		}
	}

	varInfo phTemp, phbTemp; // Needed if we don't already have such info

	if ( !wantPh ) // User doesn't want PH output, but we still need info about PH
	{
		phTemp.name = wrfNames.PH;
		GetVarInfo( ncid, ndims, phTemp, dim );
		phInfoPtr = &phTemp;
	}
	if ( !wantPhb ) // User doesn't want PHB output, but we still need info about PHB
	{
		phbTemp.name = wrfNames.PHB;
		GetVarInfo( ncid, ndims, phbTemp, dim );
		phbInfoPtr = &phbTemp;
	}

	// Allocate storage array
	float * workBuffer = new float[dim[0]*dim[1]]; // Will hold ELEVATION and PH_norm
	float * phBuffer = new float[(*phInfoPtr).sliceSize]; // Holds a slice of PH
	float * phbBuffer = new float[(*phbInfoPtr).sliceSize]; // Holds a slice of PHB
	// Allocate dummy arrays, in case some variables are vertically staggered
	float * phBufferAbove = 0;
	float * phbBufferAbove = 0;
	if ( (*phbInfoPtr).stag[2] )
		phbBufferAbove = new float[(*phbInfoPtr).sliceSize];
	if ( (*phInfoPtr).stag[2] )
		phBufferAbove = new float[(*phInfoPtr).sliceSize];
	
	// Switches to prevent redundant reads, in case of vertical staggering
	bool needAnotherPh = true;
	bool needAnotherPhb = true;

	// Prepare wavelet block writers
	// This one's for the elevation
	WaveletBlock3DIO	*elevWriter;
	WaveletBlock3DBufWriter * elevBufWriter;
	MakeWritersOpen( elevWriter, elevBufWriter, aVaporTs, "ELEVATION", metafile );
	// For normalized geopotential (if desired)
	WaveletBlock3DIO	*geopnormWriter;
	WaveletBlock3DBufWriter *geopnormBufWriter;
	if ( wantPhnorm )
		MakeWritersOpen( geopnormWriter, geopnormBufWriter, aVaporTs, "PH_norm", metafile );
	// For PH (if desired)
	WaveletBlock3DIO	*phWriter;
	WaveletBlock3DBufWriter *phBufWriter;
	if ( wantPh )
		MakeWritersOpen( phWriter, phBufWriter, aVaporTs, (*phInfoPtr).name, metafile );
	// For PHB (if desired)
	WaveletBlock3DIO	*phbWriter;
	WaveletBlock3DBufWriter *phbBufWriter;
	if ( wantPhb )
		MakeWritersOpen( phbWriter, phbBufWriter, aVaporTs, (*phbInfoPtr).name, metafile );

	// Loop over z slices
	for ( size_t z = 0 ; z < dim[2] ; z++ )
	{
		// Read needed slices
		GetZSlice( ncid, (*phInfoPtr), wrfT, z, phBuffer, phBufferAbove, needAnotherPh, dim );
		GetZSlice( ncid, (*phbInfoPtr), wrfT, z, phbBuffer, phbBufferAbove, needAnotherPhb, dim );
	
		// Write PH and/or PHB, if desired
		if ( wantPh )
			phBufWriter->WriteSlice( phBuffer );
		if ( wantPhb )
			phbBufWriter->WriteSlice( phbBuffer );

		// Write geopotential height, with name ELEVATION.  This is required by Vapor
		// to handle leveled data.
		for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
		{
			workBuffer[i] = (phBuffer[i] + phbBuffer[i])/9.81;
			// Want to find the bottom of the bottom layer and the bottom of the
			// top layer so that we can output them
			if ( z == 0 && workBuffer[i] < theseExts[0] )
				theseExts[0] = workBuffer[i]; // Bottom of bottom for this time step
			if ( z == dim[2] - 1 && workBuffer[i] < theseExts[1] )
				theseExts[1] = workBuffer[i]; // Bottom of top for this time step
			if ( z == 0 && workBuffer[i] < newExts[0] )
				newExts[0] = workBuffer[i]; // Bottom of bottom for all time steps
			if ( z == dim[2] - 1 && workBuffer[i] < newExts[1] )
				newExts[1] = workBuffer[i]; // Bottom of top for all time steps
		}
		elevBufWriter->WriteSlice( workBuffer );
				
		// Find and write normalized geopotential, if desired
		if ( wantPhnorm )
		{
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				workBuffer[i] *= 9.81/phbBuffer[i];
			geopnormBufWriter->WriteSlice( workBuffer );
		}

	}

	// Close the variables. We're done writing.
	CloseVar( elevWriter );
	if ( wantPhnorm )
		CloseVar( geopnormWriter );
	if ( wantPh )
		CloseVar( phWriter );
	if ( wantPhb )
		CloseVar( phbWriter );

	// Any desired WRF variables have now been written
	if ( wantPh )
		stillNeeded[phIndex] = false;
	if ( wantPhb )
		stillNeeded[phbIndex] = false;

	delete [] workBuffer; // Cleanup
	delete [] phBuffer; 
	delete [] phbBuffer;
	if ( (*phInfoPtr).stag[2] )
		delete [] phBufferAbove;
	if ( (*phbInfoPtr).stag[2] )
		delete [] phbBufferAbove;
}



// Reads/calculates and outputs any of these quantities: U, V, W,
// 2D wind speed (U-V plane) and 3D wind speed
void DoWindSpeed(
				int ncid, // ID of netCDF file
				vector<varInfo> desiredVars, // Vector of varInfo structs corresponding to
				 							 // the desired WRF variables
				size_t aVaporTs, // Vapor time step we're currently working on
				int wrfT, // WRF time step we're currently working on
				bool * stillNeeded, // Array says whether or not a variable (same index as
				               	    // in desiredVars) still needs to be written
				const char * metafile, // Path and file name of VDF
				int ndims, // Number of dimensions in netCDF file
				const size_t * dim, // Dimensions of VDF
				bool wantWind2d, // True if you want to output 2D wind speed, false otherwise
				bool wantWind3d // True if you want to output 3D wind speed, false otherwise
				)
{
	bool wantU = false; // Whether or not user wants to convert these quantities
	bool wantV = false;
	bool wantW = false;

	varInfo * uInfoPtr = 0; // Points to varInfo's that have info about U, V, W
	varInfo * vInfoPtr = 0;
	varInfo * wInfoPtr = 0;

	int uIndex = 0; // Indices of these variables in desiredVars)
	int vIndex = 0;
	int wIndex = 0;

	// See if user wants U, V, W output
	for ( int i = 0 ; i < opt.varnames.size() ; i++ )
	{
		if ( strcmp( desiredVars[i].name, wrfNames.U ) == 0 )
		{
			wantU = true;
			uInfoPtr = &desiredVars[i];
			uIndex = i;
			continue;
		}
		if ( strcmp( desiredVars[i].name, wrfNames.V ) == 0 )
		{
			wantV = true;
			vInfoPtr = &desiredVars[i];
			vIndex = i;
			continue;
		}
		if ( strcmp( desiredVars[i].name, wrfNames.W ) == 0 )
		{
			wantW = true;
			wInfoPtr = &desiredVars[i];
			wIndex = i;
			continue;
		}
	}

	varInfo uInfoTemp, vInfoTemp, wInfoTemp; // Needed if we don't already
		// such info

	// Even if user doesn't want to convert U, V, and W, we still need to know
	// them in order to find wind speed
	if ( !wantU && (wantWind2d || wantWind3d) )
	{
		uInfoTemp.name = wrfNames.U;
		GetVarInfo( ncid, ndims, uInfoTemp, dim );
		uInfoPtr = &uInfoTemp;
	}
	if ( !wantV && (wantWind2d || wantWind3d) )
	{
		vInfoTemp.name = wrfNames.V;
		GetVarInfo( ncid, ndims, vInfoTemp, dim );
		vInfoPtr = &vInfoTemp;
	}
	if ( !wantW && wantWind3d ) // Only need W for 3D wind speed
	{
		wInfoTemp.name = wrfNames.W;
		GetVarInfo( ncid, ndims, wInfoTemp, dim );
		wInfoPtr = &wInfoTemp;
	}

	// May need any number and any one of these arrays
	float * uBuffer = 0;
	float * uBufferAbove = 0;
	float * vBuffer = 0;
	float * vBufferAbove = 0;
	float * wBuffer = 0;
	float * wBufferAbove = 0;
	float * workBuffer = 0;

	// Allocate arrays
	if ( wantU || wantWind2d || wantWind3d )
	{
		uBuffer = new float[(*uInfoPtr).sliceSize];
		if ( (*uInfoPtr).stag[2] )
			uBufferAbove = new float[(*uInfoPtr).sliceSize];
	}
	if ( wantV || wantWind2d || wantWind3d )
	{
		vBuffer = new float[(*vInfoPtr).sliceSize];
		if ( (*vInfoPtr).stag[2] )
			vBufferAbove = new float[(*vInfoPtr).sliceSize];
	}
	if ( wantW || wantWind3d )
	{
		wBuffer = new float[(*wInfoPtr).sliceSize];
		if ( (*wInfoPtr).stag[2] )
			wBufferAbove = new float[(*wInfoPtr).sliceSize];
	}
	if ( wantWind2d || wantWind3d )
		workBuffer = new float[(dim[0]*dim[1])];

	// Create stuff we need for writing data
	// For U (if desired)
	WaveletBlock3DIO	*uWriter;
	WaveletBlock3DBufWriter *uBufWriter;
	if ( wantU )
		MakeWritersOpen( uWriter, uBufWriter, aVaporTs, (*uInfoPtr).name, metafile );
	// For V (if desired)
	WaveletBlock3DIO	*vWriter;
	WaveletBlock3DBufWriter *vBufWriter;
	if ( wantV )
		MakeWritersOpen( vWriter, vBufWriter, aVaporTs, (*vInfoPtr).name, metafile );
	// For W (if desired)
	WaveletBlock3DIO	*wWriter;
	WaveletBlock3DBufWriter *wBufWriter;
	if ( wantW )
		MakeWritersOpen( wWriter, wBufWriter, aVaporTs, (*wInfoPtr).name, metafile );
	// For 2D wind (if desired)
	WaveletBlock3DIO	*wind2dWriter;
	WaveletBlock3DBufWriter *wind2dBufWriter;
	if ( wantWind2d )
		MakeWritersOpen( wind2dWriter, wind2dBufWriter, aVaporTs, "Wind_UV", metafile );
	// For 3D wind (if desired)
	WaveletBlock3DIO	*wind3dWriter;
	WaveletBlock3DBufWriter *wind3dBufWriter;
	if ( wantWind3d )
		MakeWritersOpen( wind3dWriter, wind3dBufWriter, aVaporTs, "Wind_UVW", metafile );

	// Switches to prevent redundant reads in case of vertical staggering
	bool needAnotherU = true;
	bool needAnotherV = true;
	bool needAnotherW = true;

	// Read, calculate, and write the data
	for ( size_t z = 0 ; z < dim[2] ; z++ )
	{
		// Read (if desired or needed)
		if ( wantU || wantWind2d || wantWind3d )
			GetZSlice( ncid, (*uInfoPtr), wrfT, z, uBuffer, uBufferAbove, needAnotherU, dim );
		if ( wantV || wantWind2d || wantWind3d )
			GetZSlice( ncid, (*vInfoPtr), wrfT, z, vBuffer, vBufferAbove, needAnotherV, dim );
		if ( wantW || wantWind3d )
			GetZSlice( ncid, (*wInfoPtr), wrfT, z, wBuffer, wBufferAbove, needAnotherW, dim );
		
		// Write U, V, W (if desired)
		if ( wantU )
			uBufWriter->WriteSlice( uBuffer );
		if ( wantV )
			vBufWriter->WriteSlice( vBuffer );
		if ( wantW )
			wBufWriter->WriteSlice( wBuffer );

		// Calculate and write 2D wind (if desired or needed)
		if ( wantWind2d )
		{
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				workBuffer[i] = sqrt( uBuffer[i]*uBuffer[i] + vBuffer[i]*vBuffer[i] );
			wind2dBufWriter->WriteSlice( workBuffer );
		}
		// Calculate and write 3D wind (if desired)
		if ( wantWind2d && wantWind3d ) // Maybe a hair faster
		{
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				workBuffer[i] = sqrt( workBuffer[i]*workBuffer[i] + wBuffer[i]*wBuffer[i] );
			wind3dBufWriter->WriteSlice( workBuffer );
		}
		if ( wantWind3d && !wantWind2d )
		{
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				workBuffer[i] = sqrt( uBuffer[i]*uBuffer[i] + vBuffer[i]*vBuffer[i] + wBuffer[i]*wBuffer[i] );
			wind3dBufWriter->WriteSlice( workBuffer );
		}
	}

	// Cleanup
	if ( wantU )
	{
		CloseVar( uWriter );
		stillNeeded[uIndex] = false;
	}
	if ( wantV )
	{
		CloseVar( vWriter );
		stillNeeded[vIndex] = false;
	}
	if ( wantW )
	{
		CloseVar( wWriter );
		stillNeeded[wIndex] = false;
	}
	if ( wantWind2d )
		CloseVar( wind2dWriter );
	if ( wantWind3d )
		CloseVar( wind3dWriter );
	if ( wantU || wantWind2d || wantWind3d )
	{
		delete [] uBuffer;
		if ( (*uInfoPtr).stag[2] )
			delete uBufferAbove;
	}
	if ( wantV || wantWind2d || wantWind3d )
	{
		delete [] vBuffer;
		if ( (*vInfoPtr).stag[2] )
			delete [] vBufferAbove;
	}
	if ( wantW || wantWind3d )
	{
		delete [] wBuffer;
		if ( (*wInfoPtr).stag[2] )
			delete [] wBufferAbove;
	}
	if ( wantWind2d || wantWind3d )
		delete [] workBuffer;
}






// Read/calculates and outputs some or all of P, PB, P_full, P_norm,
// T, Theta, and TK.  Sets flags so that variables are not read twice.
void DoPTStuff(
			    int ncid, // ID of netCDF file
				vector<varInfo> desiredVars, // Vector of varInfo structs corresponding to
				 							 // the desired WRF variables
				size_t aVaporTs, // Vapor time step we're currently working on
				int wrfT, // WRF time step we're currently working on
				bool * stillNeeded, // Array says whether or not a variable (same index as
				               	    // in desiredVars) still needs to be written
				const char * metafile, // Path and file name of VDF
				int ndims, // Number of dimensions in netCDF file
				const size_t * dim, // Dimensions of VDF
				bool wantPfull, // True if user wants P_full output, false otherwise
				bool wantPnorm, // ...likewise
				bool wantTheta,
				bool wantTk
				)
{
	bool wantP = false; // Will hold whether or not user wants these converted
	bool wantPb = false;
	bool wantT = false;
	size_t pIndex = 0; // Indices of these variables in desiredVars
	size_t pbIndex = 0;
	size_t tIndex = 0;
	varInfo * pInfoPtr = 0; // Pointers to information about variables
	varInfo * pbInfoPtr = 0;
	varInfo * tInfoPtr = 0;
	
	// Find out if user wants WRF variables converted
	for ( size_t i = 0 ; i < desiredVars.size() ; i++ )
	{
		if ( strcmp( desiredVars[i].name, wrfNames.P ) == 0 )
		{
			wantP = true;
			pIndex = i;
			pInfoPtr = &desiredVars[i];
			continue;
		}
		if ( strcmp( desiredVars[i].name, wrfNames.PB ) == 0 )
		{
			wantPb = true;
			pbIndex = i;
			pbInfoPtr = &desiredVars[i];
			continue;
		}
		if ( strcmp( desiredVars[i].name, wrfNames.T ) == 0 )
		{
			wantT = true;
			tIndex = i;
			tInfoPtr = &desiredVars[i];
			continue;
		}
	}

	varInfo pTempInfo, pbTempInfo, tTempInfo; // Needed if we don't
		// already have such info

	// Even if user doesn't want P, PB, T, we may still need them
	if ( !wantP && (wantPfull || wantPnorm || wantTk) )
	{
		pTempInfo.name = wrfNames.P;
		GetVarInfo( ncid, ndims, pTempInfo, dim );
		pInfoPtr = &pTempInfo;
	}
	if ( !wantPb && (wantPfull || wantPnorm || wantTk) )
	{
		pbTempInfo.name = wrfNames.PB;
		GetVarInfo( ncid, ndims, pbTempInfo, dim );
		pbInfoPtr = &pbTempInfo;
	}
	if ( !wantT && (wantTheta || wantTk) )
	{
		tTempInfo.name = wrfNames.T;
		GetVarInfo( ncid, ndims, tTempInfo, dim );
		tInfoPtr = &tTempInfo;
	}

	// Pointers to whatever dynamic arrays we may need
	float * pBuffer = 0;
	float * pbBuffer = 0;
	float * tBuffer = 0;
	float * pBufferAbove = 0;
	float * pbBufferAbove = 0;
	float * tBufferAbove = 0;
	float * workBuffer = 0;

	// Allocate arrays
	if ( wantP || wantPfull || wantPnorm || wantTk )
	{
		pBuffer = new float[(*pInfoPtr).sliceSize];
		if ( (*pInfoPtr).stag[2] )
			pBufferAbove = new float[(*pInfoPtr).sliceSize];
	}
	if ( wantPb || wantPfull || wantPnorm || wantTk )
	{
		pbBuffer = new float[(*pbInfoPtr).sliceSize];
		if ( (*pbInfoPtr).stag[2] )
			pbBufferAbove = new float[(*pbInfoPtr).sliceSize];
	}
	if ( wantT || wantTheta || wantTk )
	{
		tBuffer = new float[(*tInfoPtr).sliceSize];
		if ( (*tInfoPtr).stag[2] )
			tBufferAbove = new float[(*tInfoPtr).sliceSize];
	}
	if ( wantPfull || wantPnorm || wantTheta || wantTk )
		workBuffer = new float[dim[0]*dim[1]];

	// Create whatever writers we may need
	// For P (if desired)
	WaveletBlock3DIO	*pWriter;
	WaveletBlock3DBufWriter *pBufWriter;
	if ( wantP )
		MakeWritersOpen( pWriter, pBufWriter, aVaporTs, (*pInfoPtr).name, metafile );
	// For PB (if desired)
	WaveletBlock3DIO	*pbWriter;
	WaveletBlock3DBufWriter *pbBufWriter;
	if ( wantPb )
		MakeWritersOpen( pbWriter, pbBufWriter, aVaporTs, (*pbInfoPtr).name, metafile );
	// For T (if desired)
	WaveletBlock3DIO	*tWriter;
	WaveletBlock3DBufWriter *tBufWriter;
	if ( wantT )
		MakeWritersOpen( tWriter, tBufWriter, aVaporTs, (*tInfoPtr).name, metafile );
	// For P_full
	WaveletBlock3DIO	*pfullWriter;
	WaveletBlock3DBufWriter *pfullBufWriter;
	if ( wantPfull )
		MakeWritersOpen( pfullWriter, pfullBufWriter, aVaporTs, "P_full", metafile );
	// For P_norm
	WaveletBlock3DIO	*pnormWriter;
	WaveletBlock3DBufWriter *pnormBufWriter;
	if ( wantPnorm )
		MakeWritersOpen( pnormWriter, pnormBufWriter, aVaporTs, "P_norm", metafile );
	// For Theta
	WaveletBlock3DIO	*thetaWriter;
	WaveletBlock3DBufWriter *thetaBufWriter;
	if ( wantTheta )
		MakeWritersOpen( thetaWriter, thetaBufWriter, aVaporTs, "Theta", metafile );
	// For TK
	WaveletBlock3DIO	*tkWriter;
	WaveletBlock3DBufWriter *tkBufWriter;
	if ( wantTk )
		MakeWritersOpen( tkWriter, tkBufWriter, aVaporTs, "TK", metafile );

	// Switches to prevent redundant reads in the case of vertical staggering
	bool needAnotherP = true;
	bool needAnotherPb = true;
	bool needAnotherT = true;

	// Read, calculate, and write
	for ( size_t z = 0 ; z < dim[2] ; z++ )
	{
		// Read
		if ( wantP || wantPfull || wantPnorm || wantTk )
			GetZSlice( ncid, (*pInfoPtr), wrfT, z, pBuffer, pBufferAbove, needAnotherP, dim );
		if ( wantPb || wantPfull || wantPnorm || wantTk )
			GetZSlice( ncid, (*pbInfoPtr), wrfT, z, pbBuffer, pbBufferAbove, needAnotherPb, dim );
		if ( wantT || wantTheta || wantTk )
			GetZSlice( ncid, (*tInfoPtr), wrfT, z, tBuffer, tBufferAbove, needAnotherT, dim );
		// Write out any desired WRF variables
		if ( wantP )
			pBufWriter->WriteSlice( pBuffer );
		if ( wantPb )
			pbBufWriter->WriteSlice( pbBuffer );
		if ( wantT )
			tBufWriter->WriteSlice( tBuffer );
		// Find and write Theta (if desired)
		if ( wantTheta )
		{
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				workBuffer[i] = tBuffer[i] + 300.0;
			thetaBufWriter->WriteSlice( workBuffer );
		}
		// Find and write P_norm (if desired)
		if ( wantPnorm )
		{
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				workBuffer[i] = pBuffer[i]/pbBuffer[i] + 1.0;
			pnormBufWriter->WriteSlice( workBuffer );
		}
		// Find and write P_full (if desired)
		if ( wantPfull && wantPnorm)
		{
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				workBuffer[i] *= pbBuffer[i];
			pfullBufWriter->WriteSlice( workBuffer );
		}
		if ( wantPfull && !wantPnorm )
		{
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				workBuffer[i] = pBuffer[i] + pbBuffer[i];
			pfullBufWriter->WriteSlice( workBuffer );
		}
		// Find and write TK (if desired).  Forget optimizations.
		if ( wantTk )
		{	
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				workBuffer[i] = 0.03719785781*(tBuffer[i] + 300.0)
								* pow( pBuffer[i] + pbBuffer[i], 0.285896414f );
			tkBufWriter->WriteSlice( workBuffer );
		}
	}

	// Cleanup
	if ( wantP || wantPfull || wantPnorm || wantTk )
	{
		delete [] pBuffer;
		if ( (*pInfoPtr).stag[2] )
			delete [] pBufferAbove;
	}
	if ( wantPb || wantPfull || wantPnorm || wantTk )
	{
		delete [] pbBuffer;
		if ( (*pbInfoPtr).stag[2] )
			delete [] pbBufferAbove;
	}
	if ( wantT || wantTheta || wantTk )
	{
		delete [] tBuffer;
		if ( (*tInfoPtr).stag[2] )
			delete tBufferAbove;
	}
	if ( wantPfull || wantPnorm || wantTheta || wantTk )
		delete [] workBuffer;
	if ( wantP )
	{
		CloseVar( pWriter );
		stillNeeded[pIndex] = false; // Already been converted, no longer needs it
	}
	if ( wantPb )
	{
		CloseVar( pbWriter );
		stillNeeded[pbIndex] = false;

	}
	if ( wantT )
	{
		CloseVar( tWriter );
		stillNeeded[tIndex] = false;
	}
	if ( wantPfull )
		CloseVar( pfullWriter );
	if ( wantPnorm )
		CloseVar( pnormWriter );
	if ( wantTheta )
		CloseVar( thetaWriter );
	if ( wantTk )
		CloseVar( tkWriter );
}



int	main(int argc, char **argv) 
{
	OptionParser op;
	
	const char	*metafile;
	const char	*netCDFfile; // Path to netCDF file 
	
	string	s;
	const Metadata	*metadata;

	// Parse command line arguments and check for errors
	ProgName = Basename(argv[0]);
	if (op.AppendOptions(set_opts) < 0)
	{
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}
	if (op.ParseOptions(&argc, argv, get_options) < 0) 
	{
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}
	if (opt.help) 
	{
		cerr << "Usage: " << ProgName << " [options] metafile datafile" << endl;
		op.PrintOptionHelp(stderr);
		exit(0);
	}
	if (argc != 3)
	{
		cerr << "Usage: " << ProgName << " [options] metafile NetCDFfile" << endl;
		op.PrintOptionHelp(stderr);
		exit(1);
	}
	// Handle atypical variable naming
	if ( opt.atypvars[0] == "noatyps" )
	{
		// Default values
		wrfNames.U = "U";
		wrfNames.V = "V";
		wrfNames.W = "W";
		wrfNames.PH = "PH";
		wrfNames.PHB = "PHB";
		wrfNames.P = "P";
		wrfNames.PB = "PB";
		wrfNames.T = "T";
	}
	else if ( opt.atypvars.size() != 8 )
	{
		cerr << "If -atypvars option is given, colon delimited list must have exactly eight elements specifying names of variables which are typically named U:V:W:PH:PHB:P:PB:T" << endl;
		exit( 1 );
	}
	else
	{
		wrfNames.U = opt.atypvars[0].c_str();
		wrfNames.V = opt.atypvars[1].c_str();
		wrfNames.W = opt.atypvars[2].c_str();
		wrfNames.PH = opt.atypvars[3].c_str();
		wrfNames.PHB = opt.atypvars[4].c_str();
		wrfNames.P = opt.atypvars[5].c_str();
		wrfNames.PB = opt.atypvars[6].c_str();
		wrfNames.T = opt.atypvars[7].c_str();
	}
	
	metafile = argv[1];	// Path to a vdf file
	netCDFfile = argv[2];	// Path to netCDF data file

	if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	WaveletBlock3DIO	*wbwriter;
	wbwriter = new WaveletBlock3DBufWriter(metafile, 0);
	if (wbwriter->GetErrCode() != 0) {
		cerr << ProgName << " : " << wbwriter->GetErrMsg() << endl;
		exit(1);
	}
	
	int ncid; // Holds file ID of netCDF file
	int ndims; // Holds number of dimensions in netCDF file
	size_t howManyTimes; // Number of time steps in the WRF file
	size_t vaporTs[MAX_WRF_TS]; // Holds corresponding Vapor time steps
	bool tsOk[MAX_WRF_TS]; // Says whether or not to convert a time step
	long minDeltaT = 0; // Minimum WRF physical timestep for output
	char * tsNaught = new char[20]; // WRF time stamp corresponding to Vapor's step 0
	char * startTime = new char[20];
	char * endTime = new char[20];
	long maxVts = 1; // The last Vapor time step
	startTime = opt.tsstart; // Starting time stamp from option list
	endTime = opt.tsend; // Last time step to convert
	
	if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	// Must typecast wbwriter so that we can use it for writing
	WaveletBlock3DBufWriter *bufwriter;
	bufwriter = (WaveletBlock3DBufWriter *) wbwriter;

	// Get a pointer to the Metadata object associated with
	// the WaveletBlock3DBufWriter object
	metadata = wbwriter->GetMetadata();

	// Get info from the VDF

	// If pre version 2, create a backup of the .vdf file. The 
	// translation process will generate a new .vdf file
	if (metadata->GetVDFVersion() < 2) save_file(metafile);

	// Get the dimensions of the volume
	const size_t * dim = metadata->GetDimension();
			
	// Get time step-related attributes
	const string tsNaughtTemp = metadata->GetUserDataString( "WRF_START_TIME" );
	tsNaughtTemp.copy( tsNaught, 19 );
	tsNaught[19] = '\0';
	vector<long> mindtTemp(1);
	mindtTemp = metadata->GetUserDataLong( "WRF_MIN_DT" );
	minDeltaT = mindtTemp[0];
	if ( strcmp( startTime, "???????" ) == 0 ) // If no startTime option is given,
		startTime = tsNaught;				   // default to starting stamp from VDF
	maxVts = metadata->GetNumTimeSteps() - 1;
	vector<string> tStamps(0); // Holds time stamps for output

	// Get info from the WRF file
	OpenWrfFile( netCDFfile, ncid, ndims, howManyTimes, vaporTs, tStamps,
				 tsOk, minDeltaT, startTime, endTime, maxVts, tsNaught );

	// Create varInfo structs for all the variables to be converted
	vector<varInfo> theVars( opt.varnames.size() );
	// An array indicating which variables still need to be written
	bool * stillNeeded = new bool[opt.varnames.size()];
	for ( size_t i = 0 ; i < opt.varnames.size() ; i++ )
	{
		// Assign names based on user input
		theVars[i].name = opt.varnames[i].c_str();
		// Get information about the variable (as long as there is a variable)
		if ( strcmp( opt.varnames[i].c_str(), "novars" ) != 0 )
			GetVarInfo( ncid, ndims, theVars[i], dim );
	}

	// Decide what derived variables we want
	bool wantPfull = false;
	bool wantPnorm = false;
	bool wantPhnorm = false;
	bool wantTheta = false;
	bool wantTk = false;
	bool wantWind3d = false;
	bool wantWind2d = false;
	for ( int i = 0 ; i < opt.dervars.size() ; i++ )
	{
		if ( opt.dervars[i] == "pfull" )
			wantPfull = true;
		else if ( opt.dervars[i] == "pnorm" )
			wantPnorm = true;
		else if ( opt.dervars[i] == "phnorm" )
			wantPhnorm = true;
		else if ( opt.dervars[i] == "theta" )
			wantTheta = true;
		else if ( opt.dervars[i] == "tk" )
			wantTk = true;
		else if ( opt.dervars[i] == "wind2d" )
			wantWind2d = true;
		else if ( opt.dervars[i] == "wind3d" )
			wantWind3d = true;
		else if ( opt.dervars[i] == "nodervars" ) // Default
			continue;
		else
		{
			fprintf( stderr, "Invalid derived variable\n" );
			exit( 1 );
		}
	}

	// Allocate space for variables
	float * fBuffer = new float[(dim[0] + 1)*(dim[1] + 1)];
	float * fBufferAbove = 0; // May be needed if variable is vertically staggered
	for ( size_t i = 0 ; i < opt.varnames.size() ; i++ )
	{
		if ( theVars[i].stag[2] )
		{
			fBufferAbove = new float[(dim[0] + 1)*(dim[1] + 1)];
			break;
		}
	}

	// Get extents from VDF so that we know if we need to advise the
	// user to change them
	const vector<double> exts = metadata->GetExtents();
	float newExts[] = { exts[2], exts[5] }; // Bottom of bottom and bottom of top for all time steps
	float theseExts[2]; // Bottom of bottom and bottom of top for each time step

	// Counts how many time steps were actually converted
	int totalConverted = 0;

	// Heading for all the output info
	cout.setf( ios::fixed );
	cout.setf( ios::showpoint );
	cout.precision( 2 );
	cout.setf(ios::left);
	if ( !opt.quiet )
		cout << setw(TSWIDTH) << "Time Stamp" << setw(CONVWIDTH) << "Converted?" << "Low Vert. Extents" << endl;

	// Convert data for all time steps that we're supposed to
	for ( size_t wrfT = 0 ; wrfT < howManyTimes ; wrfT++ )
	{
		// If we want output, print what time step we're working on
		if ( !opt.quiet )
			cout << setw(TSWIDTH) << tStamps[wrfT];

		if ( tsOk[wrfT] == false )
		{
			// Tell user we didn't convert this
			if ( !opt.quiet )
				cout << setw(CONVWIDTH) << "No" << endl;
			continue; // If we don't want this time step, skip everything below
		}
				
		// Needed to avoid redundant readings when variable is vertically staggered
		// (handled entirely by GetZSlice)
		bool needAnother = true;
		for ( int i = 0 ; i < opt.varnames.size() ; i++ )
			stillNeeded[i] = true;

		// theseExts must be reset to default values at every time step
		theseExts[0] = 400000.0;
		theseExts[1] = 400000.0;

		// Find elevation and do all the geopotential stuff
		DoGeopotStuff( ncid, theVars, vaporTs[wrfT], wrfT, stillNeeded,
					   metafile, ndims, dim, theseExts, newExts, wantPhnorm );
		// Find wind speeds, if necessary
		if ( wantWind2d || wantWind3d )
			DoWindSpeed( ncid, theVars, vaporTs[wrfT], wrfT, stillNeeded,
						 metafile, ndims, dim, wantWind2d, wantWind3d );
		// Find P- or T-related derived variables, if necessary
		if ( wantTheta || wantTk || wantPnorm || wantPfull )
			DoPTStuff( ncid, theVars, vaporTs[wrfT], wrfT, stillNeeded, metafile, 
					   ndims, dim, wantPfull, wantPnorm, wantTheta, wantTk );

		// If we don't want any WRF variables, we're done
		if ( strcmp( opt.varnames[0].c_str(), "novars" ) == 0 )
			continue;
		
		// Now read, write, and convert all variables that still need it
		for ( size_t i = 0 ; i < opt.varnames.size() ; i++ )
		{
			if ( stillNeeded[i] )
			{
				if (wbwriter->OpenVariableWrite(vaporTs[wrfT], theVars[i].name, opt.level) < 0)
				{
					cerr << ProgName << " : " << wbwriter->GetErrMsg() << endl;
					exit(1);
				}
				needAnother = true; // Every variable starts this way
				// Read and write slices of a variable
				for ( size_t z = 0 ; z < dim[2] ; z++ )
				{
					GetZSlice( ncid, theVars[i], wrfT, z, fBuffer, fBufferAbove, needAnother, dim );
					// Write the variable
					bufwriter->WriteSlice( fBuffer );
				}
				// Close the variable. We're done writing.
				wbwriter->CloseVariable();
				if (wbwriter->GetErrCode() != 0)
				{
					cerr << ProgName << ": " << wbwriter->GetErrMsg() << endl;
					exit(1);
				}
				stillNeeded[i] = false; // Now we're done with the ith variable
			}
		}
		// Tell user we converted this time step
		if ( !opt.quiet )
			cout << setw(CONVWIDTH) << "Yes" << theseExts[0] << " to " << theseExts[1] << endl;
		// Successfully converted a time step--make a note of it
		totalConverted++;
	}

	// For pre-version 2 vdf files we need to write out the updated metafile. 
	// If we don't call this then
	// the .vdf file will not be updated with stats gathered from
	// the volume we just translated.
	//
	if (metadata->GetVDFVersion() < 2) {
		Metadata *m = (Metadata *) metadata;
		m->Write(metafile);
	}

	// Report summary information
	if ( !opt.quiet )
	{
		cout << "Most restrictive vertical extents: " << newExts[0] << " to " << newExts[1] << endl;
		cout << "Time steps converted: " << totalConverted << " of " << howManyTimes << endl;
	}
	else // Quieter version of the most restrictive extents--needed by script
		cout << newExts[0] << " " << newExts[1] << endl;
	
	delete [] tsNaught;
//	delete [] startTime; // Don't know why, but Visual C++ says bad assertion
	delete [] endTime;
	delete [] stillNeeded;
	delete [] fBuffer;
	for ( size_t i = 0 ; i < opt.varnames.size() ; i++ )
	{
		if ( theVars[i].stag[2] )
		{
			delete [] fBufferAbove;
			break;
		}
	}

	nc_close(ncid); // Close the netCDF file

	exit(0);
}

