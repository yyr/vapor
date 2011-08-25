//
//
//***********************************************************************
//                                                                       *
//                            Copyright (C)  2009                        *
//            University Corporation for Atmospheric Research            *
//                            All Rights Reserved                        *
//                                                                       *
//***********************************************************************/
//
//      File:		cart2layer.cpp
//
//      Author:         Alan Norton
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           October 2009
//
//      Description:	Read a raw file containing a 3D array of floats 
//			in a cartesian grid, 
//			insert the volume into an existing WRF-based (i.e. having ELEVATION variable)
//			Vapor Data Collection
//			Currently only supports raw data, but has hooks for reading NetCDF files
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

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/Metadata.h>
#include <vapor/MetadataVDC.h>
#include <vapor/WaveletBlock3DBufWriter.h>
#include <vapor/WaveletBlock3DRegionWriter.h>
#include <vapor/WaveletBlock3DBufReader.h>
#include <vapor/WaveletBlock3DReader.h>
#include <vapor/WaveletBlock3DRegionReader.h>
#ifdef WIN32
#include "windows.h"
#pragma warning( disable : 4996 )
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

	float topValue, botValue;
	float missValue, missOut;

	float minZExtent, maxZExtent;
	int zlevels;

	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {

	{"ts",		1, 	"0","Timestep of data file starting from 0"},
	{"varname",	1, 	"???????",	"Required: Name of variable in metadata"},

	{"minz",	1,  "0.0", "Minimum Z user extent of cartesian grid"},
	{"maxz",	1,  "1.0", "Maximum Z user extent of cartesian grid"},
	{"zlevels", 1,  "0", "Number of z-levels in cartesian grid, required"},
	{"topval",	1,	"-999.f", "Value assigned above cartesian grid"},
	{"botval",	1,	"-999.f", "Value assigned below cartesian grid"},
	{"missval",	1,	"-999.f", "Missing value in source data"},
	{"missout",	1,	"-999.f", "Missing value in VAPOR data"},

	{"help",	0,	"",	"Print this message and exit"},
	{"debug",	0,	"",	"Enable debugging"},
	{"quiet",	0,	"",	"Operate quietly"},

	{NULL}
};


OptionParser::Option_T	get_options[] = {

	{"ts", VetsUtil::CvtToInt, &opt.ts, sizeof(opt.ts)},
	{"varname", VetsUtil::CvtToString, &opt.varname, sizeof(opt.varname)},

	{"minz", VetsUtil::CvtToFloat, &opt.minZExtent, sizeof(opt.minZExtent)},
	{"maxz", VetsUtil::CvtToFloat, &opt.maxZExtent, sizeof(opt.maxZExtent)},
	{"zlevels", VetsUtil::CvtToInt, &opt.zlevels, sizeof(opt.zlevels)},
	{"topval", VetsUtil::CvtToFloat, &opt.topValue, sizeof(opt.topValue)},
	{"botval", VetsUtil::CvtToFloat, &opt.botValue, sizeof(opt.botValue)},
	{"missval", VetsUtil::CvtToFloat, &opt.missValue, sizeof(opt.missValue)},
	{"missout", VetsUtil::CvtToFloat, &opt.missOut, sizeof(opt.missOut)},

	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},

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
	const char* cartesianFile,
	WaveletBlock3DBufWriter *bufwriter,	  //For writing variable to VDC
	WaveletBlock3DBufReader *elevReader,  //For reading ELEVATION from the VDC
	const size_t *dim, //dimensions from vdf
	double *read_timer
) {
	//specify buffers:  Full 3d size for input variable in cartesian file
	//				Just 2D slice for ELEVATION and output variable;
	//				We shall process one slice of output at a time.
	size_t outSize = dim[0]*dim[1]; 
	size_t in3DSize;
	size_t in2DSize = outSize;
	
	float* in3DBuffer = 0, *in2DBuffer = 0, *outBuffer = 0;
	
	float minz = opt.minZExtent;
	float maxz = opt.maxZExtent;
	
	FILE *fp = 0;
	
	//data is in binary file
	in3DSize = dim[0]*dim[1]*opt.zlevels;
	//Open the rawdata file
	fp = FOPEN64(cartesianFile, "rb");
	if (! fp) {
		cerr << ": Could not open binary input file \"" << 
			cartesianFile << "\" : " <<strerror(errno) << endl;
		exit(1);
	}
	
	
	// allocate buffers
	in3DBuffer = new float[in3DSize];
	//check for valid allocation of biggest buffer:
	if (!in3DBuffer) {
		cerr << ": Unable to allocate required memory buffer size: " << in3DSize*4 <<endl;
			exit(1);
	}
	in2DBuffer = new float[in2DSize];
	outBuffer = new float[outSize];
	int zend = dim[2];
	int xsize = dim[0];
	int ysize = dim[1];


	if(!opt.quiet) fprintf(stderr, "dimensions of output array will be: %d %d %d\n",
		(int) dim[0],(int) dim[1],(int) dim[2]);

	//if (opt.bin){
		//Read the entire variable into buffer:
		
		int rc = fread(in3DBuffer, sizeof(float), in3DSize, fp);
		if (rc != in3DSize) {
			if (rc<0) {
				cerr << ProgName << ": Error reading input file : " << 
					strerror(errno) << endl;
			}
			else {
				cerr << ProgName << ": short read" << endl;
			}
			exit(1);
		}
		
		fclose(fp);

	
	//Interpolate the output, one z-slice at a time.
	//This is all done using the VDC api
	
	int numValid = 0;
	for(int z=0; z<zend; z++) {

		if (z%10 == 0 && ! opt.quiet) {
			cout << "Processing slice # " << z << endl;
		}

		
		//Read in a slice of ELEVATION:
		elevReader->ReadSlice(in2DBuffer);
		
		if (elevReader->GetErrCode() != 0) {
			cerr << ProgName << ": " << elevReader->GetErrMsg() << endl;
			exit(1);
		}
	
		
	
		//Interpolate over this slice:
		float minelev = 1.e30f;
		float maxelev = -1.e30f;
		for (int j = 0; j<ysize; j++){
			for (int i = 0; i<xsize; i++){
				//Elevation at current (WRF) grid point
				float elev = in2DBuffer[i+xsize*j];
				if (elev < minelev) minelev = elev;
				if (elev > maxelev) maxelev = elev;
				//Elevation as a position in the cartesian grid coordinate:
				float gridValue = (elev - minz)*(zend-1.f)/(maxz-minz);
				
				//Grid indices below and above the cartesian grid point:
				int lowIndex = (int)gridValue;
				int highIndex = lowIndex+1;
				float variableValue;
				//Check whether this is above, in, or below the WRF grid:
				if (lowIndex < 0){
					if (gridValue > -0.5f) //use value at bottom of grid:
						variableValue = in3DBuffer[i+xsize*j];
					else
						variableValue = opt.botValue;
				} else if (highIndex > zend-1){
					if (gridValue < (float)(zend-0.5f)) //use value at top of grid:
						variableValue = in3DBuffer[i+xsize*(j+ysize*(zend-1))];
					else
						variableValue = opt.topValue;
				} else { //inside grid, so interpolate
					//Relative position (between 0 and 1) of current point inbetween cartesian grid points.
					float beta = (gridValue-(float)lowIndex);
					//Values of variable at lower and upper grid points
					float varHigh = in3DBuffer[i+xsize*(j+highIndex*ysize)];
					float varLow = in3DBuffer[i+xsize*(j+lowIndex*ysize)];
					//Don't interpolate missValue
					if (varHigh == opt.missValue && varLow == opt.missValue)
						variableValue = opt.missOut;
					else {
						numValid++;
						if (varHigh == opt.missValue) {
							//Use the low value if it's less than half way from current point
							if (beta >= 0.5f)
								variableValue = varLow;
							else 
								variableValue = opt.missOut;
						}
						else if (varLow == opt.missValue) {
							//Use the high value if it's less than half way from current point
							if (beta <= 0.5f)
								variableValue = varHigh;
							else
								variableValue = opt.missOut;
						}
						else
							//Interpolated value at current elevation:
							variableValue = varLow*beta + varHigh*(1.f-beta);
						
					}
				}
			
				outBuffer[i+xsize*j] = variableValue;

			}
		}
		//
		// Write a single slice of data
		//
			
		bufwriter->WriteSlice(outBuffer);
		if (bufwriter->GetErrCode() != 0) {
			cerr << ProgName << ": " << bufwriter->GetErrMsg() << endl;
			exit(1);
		}
	} 
	
	delete in2DBuffer;
	delete in3DBuffer;
	delete outBuffer;

	
}


int	main(int argc, char **argv) {

	OptionParser op;
	
	const char	*metafile;
	const char	*cartesianFile;

	double	timer = 0.0;
	double	read_timer = 0.0;
	string	s;
	const MetadataVDC	*metadata;

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

	if (opt.help || argc != 3) {
		cerr << "Usage: " << ProgName << " [options] -varname variable_name  metafile.vdf inputfile" << endl;
		cerr << "  Converts 3D variable on Cartesian grid in input file to variable in VAPOR WRF VDC." << endl;
		cerr << "  Input file is raw binary." << endl;

		cerr << "  The number of z-levels must be specified." << endl;
		cerr << "  Options are:" << endl;
		op.PrintOptionHelp(stderr);
		exit(argc != 3);
	}

	
	if (strcmp(opt.varname,"???????") == 0){
		cerr << "The name of the Cartesian variable must be specified." << endl;
		exit(1);
	}
	//If it's raw data, the z levels must be specified:
	if (opt.zlevels == 0 ){
		cerr << "The number of z-levels in cartesian input data must be specified." << endl;
		exit(1);
	}

	metafile = argv[1];	// Path to a vdf file
	cartesianFile = argv[2];	// Path to netcdf or raw data file 

    if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	
	//Determine that the variable is 3D, create a temporary metadata:
	MetadataVDC mdTemp (metafile);
	const vector<string> vars3d = mdTemp.GetVariables3D();

	bool is3D = false;
	for (int i = 0; i<vars3d.size(); i++){
		if (vars3d[i] == opt.varname) {
			is3D = true;
			break;
		}
	}
	if (!is3D){
		//Not allowed: variable must be 3D and in the vdf
		cerr << "Variable named " << opt.varname << " is not a 3D variable in the vdf file." << endl;
		exit(1);
	}
	

	WaveletBlock3DBufWriter	*wbwriter3D = 0;
	WaveletBlock3DBufReader	*wbElevReader = 0;

	//
	// Create appropriate WaveletBlock reader and writer. Initialize with
	// path to .vdf file
	//
	
	wbwriter3D = new WaveletBlock3DBufWriter(metafile);
	if (wbwriter3D->GetErrCode() != 0) {
		cerr << ProgName << " : " << wbwriter3D->GetErrMsg() << endl;
		exit(1);
	}
	wbElevReader = new WaveletBlock3DBufReader(metafile);
	if (wbElevReader->GetErrCode() != 0) {
		cerr << ProgName << " : " << wbElevReader->GetErrMsg() << endl;
		exit(1);
	}
	metadata = (MetadataVDC*)wbwriter3D;
	if (wbwriter3D->OpenVariableWrite(opt.ts, opt.varname) < 0) {
		cerr << ProgName << " : " << wbwriter3D->GetErrMsg() << endl;
		exit(1);
	} 
	if (wbElevReader->OpenVariableRead(opt.ts, "ELEVATION",-1) < 0) {
		cerr << ProgName << " : " << wbElevReader->GetErrMsg() << endl;
		exit(1);
	} 

	//
	// If pre version 2, create a backup of the .vdf file. The 
	// translation process will generate a new .vdf file
	//
	if (metadata->GetVDFVersion() < 2) save_file(metafile);

	// Get the dimensions of the volume
	//
	const size_t *dim = metadata->GetDimension();
	
	process_volume(cartesianFile, wbwriter3D,  wbElevReader,dim, &read_timer);

	wbwriter3D->CloseVariable();
	if (wbwriter3D->GetErrCode() != 0) {
		cerr << ProgName << ": " << wbwriter3D->GetErrMsg() << endl;
		exit(1);
	}

	if (! opt.quiet) {
		float write_timer, xform_timer, *range;
		
		write_timer = wbwriter3D->GetWriteTimer();
		xform_timer = wbwriter3D->GetXFormTimer();
		range = (float*) wbwriter3D->GetDataRange();
		
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
		MetadataVDC *m = (MetadataVDC *) metadata;
		m->Write(metafile);
	}
	
	exit(0);
}

