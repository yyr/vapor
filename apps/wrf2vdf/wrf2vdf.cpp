//
//
//***********************************************************************
//                                                                       *
//                            Copyright (C)  2007                        *
//            University Corporation for Atmospheric Research            *
//                            All Rights Reserved                        *
//                                                                       *
//***********************************************************************/
//
//      File:		wrf2vdf.cpp
//
//      Author:         Alan Norton
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           May 22, 2007
//
//      Description:	Read a NetCDF file containing WRF data
//			and insert a variable from that data into an existing
//			Vapor Data Collection
//
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cmath>
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
	int	ts;
	char *varname;
	int level;
	float low_out;
	float high_out;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"ts",		1, 	"0","Timestep of data file starting from 0"},
	{"varname",	1, 	"var1",	"Name of variable in wrf (=same as in metadata)"},
	{"level",	1, 	"-1",	"Refinement levels saved. 0=>coarsest, 1=>next refinement, etc. -1=>finest"},
	{"help",	0,	"",	"Print this message and exit"},
	{"debug",	0,	"",	"Enable debugging"},
	{"quiet",	0,	"",	"Operate quietly"},
	{"lowval",	1, "-1.e30", "value assigned points below grid"},
	{"highval",	1, "1.e30", "value assigned points above grid"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"ts", VetsUtil::CvtToInt, &opt.ts, sizeof(opt.ts)},
	{"varname", VetsUtil::CvtToString, &opt.varname, sizeof(opt.varname)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{"lowval", VetsUtil::CvtToFloat, &opt.low_out, sizeof(opt.low_out)},
	{"highval", VetsUtil::CvtToFloat, &opt.high_out, sizeof(opt.high_out)},
	{NULL}
};

const char	*ProgName;

//Global variables:
float minElevation, maxElevation;
float minVar, maxVar;
int gridHeight;

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
	
// Given a slice of data and an array of elevations,
// interpolate the data (in the vertical coordinate) to a regular grid.
// the minElevation and maxElevation values are mapped to the bottom and top of the grid
// values below minElevation or above maxElevation are mapped to opt.out
// elevs is 2D array of size (elevLayers)*hsize, specifying the elevation at
// the bottom and top of each level
// stag is either 0 or 1 depending on whether the variable is staggered.
// varData is 2D array of size elevlayers*hsize, however it doesn't use the
// last place if stag is 0.
// expressing the variable value
// either: at the middle of a layer if stag is 0, or at the edge if stag is 1.
// If stag is 1, then the variable is interpolated between its values at the
// layer edges.  
// If stag is 1, then values of the variable are interpolated between values at the middles 
// of the layers.  In this case, the variable values in the bottom 1/2 layer and
// the top 1/2 layer of the volume are constant, equal to the values in the middle of those layers.
// 
// gridData is a 2D array of size gridHeight*hsize, where the resampled data is placed
// In both cases the hsize coordinate is the slowly changing one.
//
void interpolateSlice(int hsize, int elevLayers, float* elevs, 
					  float* varData, float* gridData, int stag, float* minLayerWidth, float* maxHt ){
	
	int varLayers = elevLayers - (1-stag);  //number of vertical variable data values
	//Do this one horizontal coord at a time, since we need to resamp the vert coord:
	for (int j = 0; j<hsize; j++){
		//Keep track of the index of the lower level of the elev layer for the
		//current grid point.
		int currentLevel = 0;
		int varLevel;
		//Check layer width and height:
		if (*maxHt < elevs[hsize*(elevLayers -1) + j])
			*maxHt = elevs[hsize*(elevLayers -1) + j];
		if (*minLayerWidth > (elevs[hsize +j] - elevs[j]))
			*minLayerWidth = (elevs[hsize +j] - elevs[j]);
		for (int i = 0; i < gridHeight; i++){
			// convert the grid index to an elevation.
			// ht is the height of a cell in the grid:
			float ht = (maxElevation - minElevation)/(float)(gridHeight-1);
			float vertHeight = minElevation + ht*(float)i;
			
			//compare with top and bottom layers:
			if (vertHeight < elevs[j]){
				gridData[i*hsize + j] =  opt.low_out;
				continue;
			}
			else if(vertHeight > elevs[hsize*(elevLayers -1) + j]){
				gridData[i*hsize + j] =  opt.high_out;
				continue;
			}
			//If stag==1 then the interpolation is based on the position between elev values.
			//If stag==0 then the interpolation is between midpoints between elev values.
	
			//adjust setting of CurrentLevel to indicate the elev level below vertHeight.
			while (vertHeight > elevs[j+hsize*(currentLevel+1)]){
				currentLevel++;
				assert(currentLevel < elevLayers);
			}
			varLevel = currentLevel;
			
			float lowerHeight = elevs[j+hsize*currentLevel];
			float upperHeight = elevs[j+hsize*(currentLevel+1)];
			//adjust if not staggered:
			if (!stag){
				//check whether we are above or below the midpoint between layers
				float mid = 0.5f*(lowerHeight+upperHeight);
				if (vertHeight < mid) {
					//If below the midpoint, the varLevel is one less than currentLevel
					varLevel = currentLevel -1;
					if (currentLevel > 0){
						lowerHeight = 0.5f*(lowerHeight + elevs[j+hsize*(currentLevel-1)]);
						upperHeight = mid;
					} else { //Use value at bottom of grid
					
						gridData[j+hsize*i] = varData[j];
						if (varData[j] < minVar) minVar = varData[j];
						if (varData[j] > maxVar) maxVar = varData[j];
						continue;
					}
				} else { 
					//In the top half-layer, the variable level is the same as the
					//current elevation level:
					varLevel = currentLevel;
					if (currentLevel < elevLayers-2){ //Make sure not at very top
						
						upperHeight = 0.5f*(upperHeight + elevs[j+hsize*(currentLevel+2)]);
						lowerHeight = mid;
						
					} else {  //Use value at top layer
						float val = varData[j+ hsize*(varLayers-1)];
						gridData[j+hsize*i] =  val;
						if (val < minVar) minVar = val;
						if (val > maxVar) maxVar = val;
						
						continue;
					}
				}
			} 
			

			//Now we should be inside the interval:
			assert (lowerHeight <= vertHeight && upperHeight >= vertHeight);
			assert (varLevel < varLayers && varLevel >= 0);
			//Perform interpolation:
			float frac = (vertHeight - lowerHeight)/(upperHeight - lowerHeight);
			float val = (1.f - frac)*varData[j + hsize*varLevel] +
				frac*varData[j+hsize*(varLevel+1)];
			if (val < minVar) minVar = val;
			if (val > maxVar) maxVar = val;
			gridData[j+hsize*i] = val;
		
		}
	}
}


void	process_volume(
	WaveletBlock3DBufWriter *bufwriter,
	const size_t *dim, //dimensions from vdf
	const int ncid,
	double *read_timer
) {

	// Check out the netcdf file.
	// It needs to have the specified variable name, and the grid dimensions must
	// agree with the metadata (or off by one, if it's a _stag dimension.
	// start and _count arrays are indexed by netcdf dimension indices,
	// determine how to extract arrays from file.
	size_t* start;
	size_t* elev_count, *var_count; 
	// full counts are same but indicate full array sizes.
	size_t* fullElev_count, *fullVar_count; 
	//ncdfdims is an array of all the dimension id's in the netcdf file:
	size_t* ncdfdims;
	

    int nc_status;

    int ndims;          // # dims in source file
    int nvars;          // number of variables (not used)
    int ngatts;         // number of global attributes (not used)
    int xdimid;         // id of unlimited dimension (not used)
	int varid;			// id of variable we are reading
	int phVarid;
	int phbVarid;		//ID's of PH and PHB

	nc_status = nc_inq(ncid, &ndims, &nvars, &ngatts, &xdimid);
	
	NC_ERR_READ(nc_status);

	//Check on the variable we are going to read:
	nc_status = nc_inq_varid(ncid, opt.varname, &varid);
	if (nc_status != NC_NOERR){
		fprintf(stderr, "variable %s not found in netcdf file\n", opt.varname);
		exit(1);
	}
	//Check on PH and PHB
	nc_status = nc_inq_varid(ncid, "PH", &phVarid);
	if (nc_status != NC_NOERR){
		fprintf(stderr, "variable PH not found in netcdf file\n");
		exit(1);
	}
	nc_status = nc_inq_varid(ncid, "PHB", &phbVarid);
	if (nc_status != NC_NOERR){
		fprintf(stderr, "variable PHB not found in netcdf file\n");
		exit(1);
	}
	
	//allocate array for all dimensions in the netcdf file:
	ncdfdims = new size_t[ndims];
	
	//Get all the dimensions in the file:
	for (int d=0; d<ndims; d++) {
		nc_status = nc_inq_dimlen(ncid, d, &ncdfdims[d]);
		NC_ERR_READ(nc_status);
	}

	
	char name[NC_MAX_NAME+1];
	nc_type xtype;
	int ndimids;
	//Array of dimension id's used in netcdf file
	int dimids[NC_MAX_VAR_DIMS];
	int natts;

	//Find the type and the dimensions associated with the variable:
	nc_status = nc_inq_var(
		ncid, varid, name, &xtype, &ndimids,
		dimids, &natts
	);
	NC_ERR_READ(nc_status);

	//Need at least 3 dimensions
	if (ndimids < 3 || ndimids > 4) {
		cerr << ProgName << ": Wrong number of  variable dimensions" << endl;
		exit(1);
	}
	//The dimensions need to include bot_top, south_north, and west_east, or the
	//corresponding _stag dimensions.
	//They may also include time
	bool foundXDim = false, foundYDim = false, foundZDim = false;
	//int dimIDs[3];// dimension ID's (in netcdf file) for each of the 3 dimensions we are using
	//south_north is X, bottom_top is Y, west_east is Z.
	int dimIndex[4]; //specify which dimension ID (for this variable) is associated with time, x,y,z
	// Initialize the count and start arrays for extracting slices from the data:
	var_count = new size_t[ndimids];
	elev_count = new size_t[ndimids];
	start = new size_t[ndimids];
	fullElev_count = new size_t[ndimids];
	fullVar_count = new size_t[ndimids];
	for (int i = 0; i<ndimids; i++){
		fullElev_count[i] = 1;
		fullVar_count[i] = 1;
		start[i] = 0;
		var_count[i] = 1;
		elev_count[i] = 1;
	}

	// stag is 1 if the variable is staggered in vertical dimension
	int stag = 0;
	// elevLayers is the number of elevation layers,
	// varLayers is elevLayers -1 + stag
	int elevLayers, varLayers; 
	//Go through the dimensions looking for the dimensions that we are using
	// These are North-south, east-west, level, and time.
	// Any of the first three can be _stag; (we will initially ignore the largest value)
	for (int i = 0; i<ndimids; i++){
		//For each dimension id, get the name associated with it
		nc_status = nc_inq_dimname(ncid, dimids[i], name);
		NC_ERR_READ(nc_status);
		//See if that dimension name is a coordinate of the desired array.
		if (!foundXDim){
			if ((strcmp(name, "south_north" ) == 0)|| strcmp(name, "south_north_stag") == 0) {
				foundXDim = true;
				int thisDimid = dimids[i];
				dimIndex[1] = i;
				if (ncdfdims[thisDimid] != dim[0] &&
					ncdfdims[thisDimid] != dim[0]+1){
					fprintf(stderr, "WRF and VDF array do not match in dimension 0\n");
					exit(1);
				}
				fullVar_count[i] = dim[0];
				fullElev_count[i] = dim[0];
				continue;
			} 
		} 
		if (!foundYDim) {

			//Any positive value is OK for this coord.
			if ((strcmp(name, "bottom_top") == 0) || strcmp(name, "bottom_top_stag") == 0){
				foundYDim = true;
				int thisDimid = dimids[i];
				dimIndex[2] = i;
				//FullVar_count is the number of layers of the variable  
				fullVar_count[i] = ncdfdims[dimids[i]];
				varLayers = fullVar_count[i];
				//If stag is false, the elev is 1 bigger than the variable
				if (strcmp(name,"bottom_top_stag") == 0){
					stag = 1;
					elevLayers = varLayers;
				} else {
					stag = 0;
					elevLayers = varLayers+1;
				}
				fullElev_count[i] = elevLayers;
				continue;
			} 
		} 
		if (!foundZDim) {
			if ((strcmp(name, "west_east") == 0) || strcmp(name, "west_east_stag") == 0){
				foundZDim = true;
				int thisDimid = dimids[i];
				dimIndex[3] = i;
				if (ncdfdims[thisDimid] != dim[2] &&
					ncdfdims[thisDimid] != dim[2]+1){
					fprintf(stderr, "WRF and VDF array do not match in dimension 2\n");
					exit(1);
				}
				fullVar_count[i] = dim[2];
				fullElev_count[i] = dim[2];
				continue;
			} 
		}
		//See if the dimension name is Time
		
		if (strcmp(name, "Time") == 0){
			
			dimIndex[0] = i;
			start[i] = (size_t) opt.ts;
			continue;
		}
		 
	}
	if (!foundZDim || !foundYDim || !foundXDim){
		fprintf(stderr, "Not all 3 WRF dimension names were found");
		exit(1);
	}
	
	//Find the type and the dimensions associated with the variable:
	nc_status = nc_inq_var(
		ncid, varid, name, &xtype, &ndimids,
		dimids, &natts
	);
	NC_ERR_READ(nc_status);


	size_t elev_size = fullElev_count[dimIndex[1]]*elevLayers;
	size_t var_size = fullVar_count[dimIndex[1]]*varLayers;
	//allocate a buffer big enough for reading 2d slice (constant West_East and time):
	if(!opt.quiet) fprintf(stderr, "dimensions of elevation array are: %d %d %d\n",
		fullElev_count[dimIndex[1]],fullElev_count[dimIndex[2]],fullElev_count[dimIndex[3]]);
	
	
	float *buffer = new float[var_size];
	float *phBuffer = new float[elev_size];
	float *phbBuffer = new float[elev_size];

	//Also allocate an output buffer based on vdf dimensions.
	//This is where the interpolated data is placed.
	float *outBuffer = new float[dim[0]*dim[1]];

	fprintf(stderr," input buffer size allocated: %d\n", 4*elev_size);
	
	
	//
	// Translate the volume one slice at a time
	//
	// Set up counts to only grab a West_East-slice, const time 
	for (int i = 0; i< ndimids; i++) {
		elev_count[i] = fullElev_count[i];
		var_count[i] = fullVar_count[i];
	}
	//limit to west_east slice, constant time:
	elev_count[dimIndex[3]] = 1;
	var_count[dimIndex[3]] = 1;
	elev_count[dimIndex[0]] = 1;
	elev_count[dimIndex[0]] = 1;

	//Initialize min/max grid val:
	float maxHeight = -1.f;
	float minWidth = 1.e30f;
	minVar = 1.e30f;
	maxVar = -1.e30f;
	//Loop over z (West_east):
	for(int z=0; z<dim[2]; z++) {

		if (z%50 == 0 && ! opt.quiet) {
			cout << "Reading slice # " << z << endl;
		}

		TIMER_START(t1);
		start[dimIndex[3]] = z;
		
		
		
		nc_status = nc_get_vara_float(
			ncid, varid, start, var_count, buffer
		);
		
		NC_ERR_READ(nc_status);
		
		//Also grab PB and PHB for this slice:
		nc_status = nc_get_vara_float(
			ncid, phVarid, start, elev_count, phBuffer
		);
		nc_status = nc_get_vara_float(
			ncid, phbVarid, start, elev_count, phbBuffer
		);
		TIMER_STOP(t1, *read_timer);

		//
		// Interpolate a single slice of data.  
		//
		//Calculate elevation, which is (PB+PBH)/g
		int hsize = dim[0];
		
		for (int layer = 0; layer< elevLayers; layer++){
			for (int j = 0; j<hsize; j++) {
				phBuffer[layer*hsize+j] += phbBuffer[layer*hsize+j];
				phBuffer[layer*hsize+j] /= 9.8f;
			}
		}
		interpolateSlice(hsize, elevLayers, phBuffer, buffer, outBuffer, stag,
			&minWidth, &maxHeight);
	
		
		bufwriter->WriteSlice(outBuffer);

		if (bufwriter->GetErrCode() != 0) {
			cerr << ProgName << ": " << bufwriter->GetErrMsg() << endl;
			exit(1);
		}

	}
	if (!opt.quiet){
		fprintf(stderr, "min layer width: %g   Max layer height: %g\n",
			minWidth, maxHeight);
		fprintf(stderr, "minimal, maximal variable values in grid: %g, %g\n",
			minVar, maxVar);
	}
}


int	main(int argc, char **argv) {

	OptionParser op;
	
	const char	*metafile;
	const char	*netCDFfile;

	double	timer = 0.0;
	double	read_timer = 0.0;
	string	s;
	const Metadata	*metadata;

	//
	// Parse command line arguments
	//
	ProgName = Basename(argv[0]);

	if (op.AppendOptions(set_opts) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	if (opt.help) {
		cerr << "Usage: " << ProgName << " [options] metafile datafile" << endl;
		op.PrintOptionHelp(stderr);
		exit(0);
	}

	if (argc != 3) {
		cerr << "Usage: " << ProgName << " [options] metafile NetCDFfile" << endl;
		op.PrintOptionHelp(stderr);
		exit(1);
	}

	metafile = argv[1];	// Path to a vdf file
	netCDFfile = argv[2];	// Path to raw data file 

    if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	WaveletBlock3DIO	*wbwriter;

	//
	// Create an appropriate WaveletBlock writer. Initialize with
	// path to .vdf file
	//
	
	wbwriter = new WaveletBlock3DBufWriter(metafile, 0);
	
	if (wbwriter->GetErrCode() != 0) {
		cerr << ProgName << " : " << wbwriter->GetErrMsg() << endl;
		exit(1);
	}

	// Get a pointer to the Metadata object associated with
	// the WaveletBlock3DBufWriter object
	//
	metadata = wbwriter->GetMetadata();


	//
	// Open a variable for writing at the indicated time step
	//
	if (wbwriter->OpenVariableWrite(opt.ts, opt.varname, opt.level) < 0) {
		cerr << ProgName << " : " << wbwriter->GetErrMsg() << endl;
		exit(1);
	} 

	//
	// If pre version 2, create a backup of the .vdf file. The 
	// translation process will generate a new .vdf file
	//
	if (metadata->GetVDFVersion() < 2) save_file(metafile);

    int nc_status;
    int ncid;

    nc_status = nc_open(netCDFfile, NC_NOWRITE, &ncid);
	NC_ERR_READ(nc_status);

   
	// Get the dimensions of the volume
	//
	const size_t *dim = metadata->GetDimension();
	const std::vector<double> &exts = metadata->GetExtents();;
	minElevation = exts[1];
	maxElevation = exts[4];
	gridHeight = dim[1];


	TIMER_START(t0);

	process_volume((WaveletBlock3DBufWriter *) wbwriter, dim, ncid, &read_timer);
	
	// Close the variable. We're done writing.
	//
	wbwriter->CloseVariable();
	if (wbwriter->GetErrCode() != 0) {
		cerr << ProgName << ": " << wbwriter->GetErrMsg() << endl;
		exit(1);
	}
	TIMER_STOP(t0,timer);

	if (! opt.quiet) {
		float	write_timer = wbwriter->GetWriteTimer();
		float	xform_timer = wbwriter->GetXFormTimer();
		const float *range = wbwriter->GetDataRange();

		fprintf(stdout, "read time : %f\n", read_timer);
		fprintf(stdout, "write time : %f\n", write_timer);
		fprintf(stdout, "transform time : %f\n", xform_timer);
		fprintf(stdout, "total transform time : %f\n", timer);
		fprintf(stdout, "min and max values of data output: %g, %g\n",range[0], range[1]);
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
	nc_close(ncid);
	exit(0);
}

