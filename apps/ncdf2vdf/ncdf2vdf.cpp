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
//      Description:	Read a NetCDF file containing a 3D array of floats or doubles
//			and insert the volume into an existing
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
	char *ncdfvarname;
	int level;
	vector <string> dimnames;
	vector <string> constDimNames;
	vector <int> constDimValues;
	OptionParser::Boolean_T	swapz;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"ts",		1, 	"0","Timestep of data file starting from 0"},
	{"varname",	1, 	"var1",	"Name of variable in metadata"},
	{"ncdfvar",	1, 	"???????",	"Name of variable in NetCDF, if different"},
	{"level",	1, 	"-1",	"Refinement levels saved. 0=>coarsest, 1=>next refinement, etc. -1=>finest"},
	{"swapz",	0,	"",	"Swap the order of processing for the Z coordinate (largest to smallest)"},
	{"help",	0,	"",	"Print this message and exit"},
	{"debug",	0,	"",	"Enable debugging"},
	{"quiet",	0,	"",	"Operate quietly"},
	{"dimnames", 1, "xdim:ydim:zdim", "Colon-separated list of x-, y-, and z-dimension names in NetCDF file"},
	{"cnstnames",1, "-", "Colon-separated list of constant dimension names"},
	{"cnstvals",1,"0", "Colon-separated list of constant dimension values, for corresponding constant dimension names"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"ts", VetsUtil::CvtToInt, &opt.ts, sizeof(opt.ts)},
	{"varname", VetsUtil::CvtToString, &opt.varname, sizeof(opt.varname)},
	{"ncdfvar", VetsUtil::CvtToString, &opt.ncdfvarname, sizeof(opt.ncdfvarname)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"swapz", VetsUtil::CvtToBoolean, &opt.swapz, sizeof(opt.swapz)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{"dimnames", VetsUtil::CvtToStrVec, &opt.dimnames, sizeof(opt.dimnames)},
	{"cnstnames", VetsUtil::CvtToStrVec, &opt.constDimNames, sizeof(opt.constDimNames)},
	{"cnstvals", VetsUtil::CvtToIntVec, &opt.constDimValues, sizeof(opt.constDimValues)},
	{NULL}
};

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
	


void	process_volume(
	WaveletBlock3DBufWriter *bufwriter,
	const size_t *dim, //dimensions from vdf
	const int ncid,
	double *read_timer
) {

	// Check out the netcdf file.
	// It needs to have the specified variable name, and the specified dimension names
	// The dimensions must agree with the dimensions in the metadata, and they 
	// must be in the same order.
	size_t* ncdfdims;
	size_t* start;
	size_t* count;
	size_t* fullCount;
	

    int nc_status;

    int ndims;          // # dims in source file
    int nvars;          // number of variables (not used)
    int ngatts;         // number of global attributes (not used)
    int xdimid;         // id of unlimited dimension (not used)
	int varid;			// id of variable we are reading

	nc_status = nc_inq(ncid, &ndims, &nvars, &ngatts, &xdimid);
	
	NC_ERR_READ(nc_status);

	//Check on the variable we are going to read:
	nc_status = nc_inq_varid(ncid, opt.ncdfvarname, &varid);
	if (nc_status != NC_NOERR){
		fprintf(stderr, "variable %s not found in netcdf file\n", opt.ncdfvarname);
		exit(1);
	}
	
	//allocate array for dimensions in the netcdf file:
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
	if (ndimids < 3) {
		cerr << ProgName << ": Insufficient netcdf variable dimensions" << endl;
		exit(1);
	}
	
	bool foundXDim = false, foundYDim = false, foundZDim = false;
	int dimIDs[3];// dimension ID's (in netcdf file) for each of the 3 dimensions we are using
	int dimIndex[3]; //specify which dimension (for this variable) is associated with x,y,z
	// Initialize the count and start arrays for extracting slices from the data:
	count = new size_t[ndimids];
	start = new size_t[ndimids];
	fullCount = new size_t[ndimids];
	for (int i = 0; i<ndimids; i++){
		fullCount[i] = 1;
		start[i] = 0;
		count[i] = 1;
	}

	//Make sure any constant dimension names are valid:
	for (int i = 0; i<opt.constDimNames.size(); i++){
		int constDimID;
		if (nc_inq_dimid(ncid, opt.constDimNames[i].c_str(), &constDimID) != NC_NOERR){
			fprintf(stderr, "Constant dimension name %s not in NetCDF file\n",
				opt.constDimNames[i].c_str());
			exit(1);
		}
		//OK, found it.  Verify that the constant value is OK:
		size_t constDimLen=0;
		if((nc_inq_dimlen(ncid, constDimID, &constDimLen) != NC_NOERR) ||
			constDimLen != opt.constDimValues[i] ||
			opt.constDimValues[i] < 0)
		{
			fprintf(stderr, "Invalid value of constant dimension %s\n",
				opt.constDimNames[i].c_str());
			exit(1);
		}
	}


			
	//Go through the dimensions looking for the 3 dimensions that we are using,
	//as well as the constant dimension names/values
	for (int i = 0; i<ndimids; i++){
		//For each dimension id, get the name associated with it
		nc_status = nc_inq_dimname(ncid, dimids[i], name);
		NC_ERR_READ(nc_status);
		//See if that dimension name is a coordinate of the desired array.
		if (!foundXDim){
			if (strcmp(name, opt.dimnames[0].c_str()) == 0){
				foundXDim = true;
				dimIDs[0] = dimids[i];
				dimIndex[0] = i;
				//Allow dimension to be off by 1, in case of 
				//staggered dimensions.
				if (ncdfdims[dimIDs[0]] != dim[0] &&
					ncdfdims[dimIDs[0]] != (dim[0]+1)){
					fprintf(stderr, "NetCDF and VDF array do not match in dimension 0\n");
					exit(1);
				}
				fullCount[i] = dim[0];
				continue;
			} 
		} 
		if (!foundYDim) {
			if (strcmp(name, opt.dimnames[1].c_str()) == 0){
				foundYDim = true;
				dimIDs[1] = dimids[i];
				dimIndex[1] = i;
				if (ncdfdims[dimIDs[1]] != dim[1] &&
					ncdfdims[dimIDs[1]] != (dim[1]+1)){
					fprintf(stderr, "NetCDF and VDF array do not match in dimension 1\n");
					exit(1);
				}
				fullCount[i] = dim[1];
				continue;
			}
		} 
		if (!foundZDim) {
			if (strcmp(name, opt.dimnames[2].c_str()) == 0){
				foundZDim = true;
				dimIndex[2] = i;
				dimIDs[2] = dimids[i];
				if (ncdfdims[dimIDs[2]] != dim[2] &&
					ncdfdims[dimIDs[2]] != (dim[2]+1)){
					fprintf(stderr, "NetCDF and VDF array do not match in dimension 2\n");
					exit(1);
				}
				fullCount[i] = dim[2];
				continue;
			}
		}
		//See if the dimension name is an identified constant dimension:
		for (int k = 0; k<opt.constDimNames.size(); k++){
			if (strcmp(name, opt.constDimNames[k].c_str()) == 0){
				start[i] = (size_t) opt.constDimValues[k];
				break;
			}
		}
	}
	if (!foundZDim || !foundYDim || !foundXDim){
		fprintf(stderr, "Specified Netcdf dimension name not found");
		exit(1);
	}

	int elem_size = 0;
	switch (xtype) {
		case NC_FLOAT:
			elem_size = sizeof(float);
			break;
		case NC_DOUBLE:
			elem_size = sizeof(double);
			break;
		default:
			cerr << ProgName << ": Invalid variable data type" << endl;
			exit(1);
			break;
	}

	size_t size = fullCount[dimIndex[0]]*fullCount[dimIndex[1]];
	//allocate a buffer big enough for 2d slice (constant z):
	
	float* fbuffer = new float[size];
	double *dbuffer = 0;
	if (xtype == NC_DOUBLE) dbuffer = new double[size];
	if(!opt.quiet) fprintf(stderr, "dimensions of array are: %d %d %d\n",
		fullCount[dimIndex[0]],fullCount[dimIndex[1]],fullCount[dimIndex[2]]);
	
	//
	// Translate the volume one slice at a time
	//
	// Set up counts to only grab a z-slice
	for (int i = 0; i< ndimids; i++) count[i] = fullCount[i];
	count[dimIndex[2]] = 1;
	int zbegin = 0;
	int zend = dim[2];
	int zinc = 1;
	if (opt.swapz) {
		zbegin = dim[2]-1;
		zend = -1;
		zinc = -1;
	}
	for(int z=zbegin; z!=zend; z+=zinc) {

		if (z%50 == 0 && ! opt.quiet) {
			cout << "Reading slice # " << z << endl;
		}

		TIMER_START(t1);
		start[dimIndex[2]] = z;
		
		if (xtype == NC_FLOAT) {
		
			nc_status = nc_get_vara_float(
				ncid, varid, start, count, fbuffer
			);
			
			NC_ERR_READ(nc_status);
		} else if (xtype == NC_DOUBLE){
			
			nc_status = nc_get_vara_double(
				ncid, varid, start, count, dbuffer
			);
			NC_ERR_READ(nc_status);
			//Convert to float:
			
			
			for(int i=0; i<dim[0]*dim[1]; i++) fbuffer[i] = (float)dbuffer[i];
		}
		TIMER_STOP(t1, *read_timer);
		//
		// Write a single slice of data
		//
		
		bufwriter->WriteSlice(fbuffer);
		if (bufwriter->GetErrCode() != 0) {
			cerr << ProgName << ": " << bufwriter->GetErrMsg() << endl;
			exit(1);
		}

	}
	delete fbuffer;
	if (dbuffer) delete dbuffer;
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

	//Make sure that the names and values of constant dimensions agree:
	if (opt.constDimNames[0] == "-"){
		//make sure we aren't dealing with nothing:
		opt.constDimNames.clear();
		opt.constDimValues.clear();
	}
	if (opt.constDimNames.size() != opt.constDimValues.size()){
		cerr << "The number of constant dimension names "<< opt.constDimNames.size() << " and constant dimension values "<< opt.constDimValues.size() <<" must be the same" << endl;
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

	// If the netcdf variable name was not specified, use the same name as
	//  in the vdf
	if (strcmp(opt.ncdfvarname,"???????") == 0){
		opt.ncdfvarname = opt.varname;
	}
	
	
    int nc_status;
    int ncid;

    nc_status = nc_open(netCDFfile, NC_NOWRITE, &ncid);
	NC_ERR_READ(nc_status);

   
	// Get the dimensions of the volume
	//
	const size_t *dim = metadata->GetDimension();


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

