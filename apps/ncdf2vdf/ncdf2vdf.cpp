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
//      Description:	Read a NetCDF file containing a 2-3D array of floats or doubles
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
#include <vapor/MetadataVDC.h>
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
	{"varname",	1, 	"???????",	"Required: Name of variable in metadata"},
	{"ncdfvar",	1, 	"???????",	"Name of variable in NetCDF, if different"},
	{"level",	1, 	"-1",	"Refinement levels saved. 0=>coarsest, 1=>next refinement, etc. -1=>finest"},
	{"swapz",	0,	"",	"Swap the order of processing for the Z coordinate (largest to smallest)"},
	{"help",	0,	"",	"Print this message and exit"},
	{"debug",	0,	"",	"Enable debugging"},
	{"quiet",	0,	"",	"Operate quietly"},
	{"dimnames", 1, "???:???:???", "Required: Colon-separated list of x-, y-, and z-dimension names in NetCDF file\n (z ignored if variable is 2D)"},
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
//Perform averaging as needed for staggered dimensions.  If x or y is staggered, the output array
//is different from the input; otherwise it's the same.
//
void averageSlice(nc_type dataType,void* inSlice, void* outSlice, 
					  size_t inX, size_t inY, size_t outX, size_t outY) {
	float *inFloat = (float*)inSlice; 
	float *outFloat = (float*)outSlice;
	double *inDouble = (double*)inSlice;
	double* outDouble = (double*)outSlice;

	if (dataType == NC_FLOAT) {
		if (inX > outX){ //average in x, over all y; array is inSizeX by inSizeY
			//In this case the outSlice is one shorter (in X) than the inSlice
			for (int iy = 0; iy < inY; iy++){
				for (int ix = 0; ix < outX; ix++) {
					outFloat[ix+iy*outX] = 0.5*(inFloat[ix+1+iy*inX]+inFloat[ix+iy*inX]);
				}
			}
		}
		if (inY > outY){ //average in y, over all x.  Input data is always in outSlice:
			for (int ix = 0; ix < outX; ix++){
				for (int iy = 0; iy < outY; iy++) {
					outFloat[ix+iy*outX] = 0.5*(outFloat[ix+iy*outX]+outFloat[ix+(iy+1)*outX]);
				}
			}
		}
	} else { //Same, but with doubles:
		if (inX > outX){ //average in x, over all y; array is inSizeX by inSizeY
			//In this case the outSlice is one shorter (in X) than the inSlice
			for (int iy = 0; iy < inY; iy++){
				for (int ix = 0; ix < outX; ix++) {
					outDouble[ix+iy*outX] = 0.5*(inDouble[ix+1+iy*inX]+inDouble[ix+iy*inX]);
				}
			}
		}
		if (inY > outY){ //average in y, over all x.  Input data is always in outSlice:
			for (int ix = 0; ix < outX; ix++){
				for (int iy = 0; iy < outY; iy++) {
					outDouble[ix+iy*outX] = 0.5*(outDouble[ix+iy*outX]+outDouble[ix+(iy+1)*outX]);
				}
			}
		}
	}
	// if no averaging, but output is not input, then copy to output:
	if (inX == outX && inY == outY){
		if (dataType == NC_FLOAT && inFloat != outFloat) {
			for (int iy = 0; iy < inY; iy++){
				for (int ix = 0; ix < inX; ix++) {
					outFloat[ix+iy*inX] = inFloat[ix+iy*inX];
				}
			}
		} else if ((dataType != NC_FLOAT) && (inDouble != outDouble)) {
			for (int ix = 0; ix < inX; ix++){
				for (int iy = 0; iy < inY; iy++) {
					outDouble[ix+iy*inX] = inDouble[ix+iy*inX];
				}
			}
		}
	}
	return;
}

void	process_volume(
	WaveletBlock3DBufWriter *bufwriter,
	const size_t *dim, //dimensions from vdf
	const int ncid,
	double *read_timer
) {
	*read_timer = 0.0;

	// Check out the netcdf file.
	// It needs to have the specified variable name, and the specified dimension names
	// The dimensions must agree with the dimensions in the metadata, and they 
	// must be in the same order.
	size_t* ncdfdims;
	size_t* start;
	size_t* count;
	size_t* outCount, *inCount;
	

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
	int dimIDs[3] = {0,0,0};// dimension ID's (in netcdf file) for each of the 3 dimensions we are using
	int dimIndex[3] = {0,0,0}; //specify which dimension (for this variable) is associated with x,y,z
	// Initialize the count and start arrays for extracting slices from the data:
	count = new size_t[ndimids];
	start = new size_t[ndimids];
	outCount = new size_t[ndimids];
	inCount = new size_t[ndimids];
	for (int i = 0; i<ndimids; i++){
		outCount[i] = 1;
		inCount[i] = 1;
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
			constDimLen <= opt.constDimValues[i] ||
			opt.constDimValues[i] < 0)
		{
			fprintf(stderr, "Invalid value %d of constant dimension %s of length %d\n",
				opt.constDimValues[i],
				opt.constDimNames[i].c_str(),
				(int) constDimLen);
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
				outCount[i] = dim[0];
				inCount[i] = ncdfdims[dimIDs[0]];
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
				outCount[i] = dim[1];
				inCount[i] = ncdfdims[dimIDs[1]];
				
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
				
				outCount[i] = dim[2];
				inCount[i] = ncdfdims[dimIDs[2]];
				
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
		fprintf(stderr, "A specified NetCDF dimension name was not used with specified variable.\n");
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

	size_t outSize = outCount[dimIndex[0]]*inCount[dimIndex[1]];
	size_t inSize = inCount[dimIndex[0]]*inCount[dimIndex[1]];
	//allocate a buffer big enough for input 2D slice (constant z):
	
	float* inFBuffer = 0, *outFBuffer = 0, *outFBuffer2 = 0;
	double *inDBuffer = 0, *outDBuffer = 0, *outDBuffer2 = 0;

	//When z is staggered we need a separate input buffer from output:
	float* stagInFBuffer = 0;
	double* stagInDBuffer = 0;
	
	if (xtype == NC_DOUBLE) inDBuffer = new double[inSize];
	else inFBuffer = new float[inSize];

	// If data is staggered in x, need additional buffer for output

	if (inCount[dimIndex[0]]>outCount[dimIndex[0]]){
		if (!opt.quiet) fprintf(stderr, "variable is staggered in x\n");
		if (xtype == NC_DOUBLE) outDBuffer = new double[outSize];
		else outFBuffer = new float[outSize];
	} else {
		outFBuffer = inFBuffer;
		outDBuffer = inDBuffer;
	}
	//Always need an outFBuffer, (for conversion if data is double)
	if (!outFBuffer) outFBuffer = new float[outSize];

	if (inCount[dimIndex[1]]>outCount[dimIndex[1]] && !opt.quiet)
		fprintf(stderr, "variable is staggered in y\n");
	if (inCount[dimIndex[2]]>outCount[dimIndex[2]] && !opt.quiet)
		fprintf(stderr, "variable is staggered in z\n");

	if(!opt.quiet) fprintf(stderr, "dimensions of output array are: %d %d %d\n",
		(int) outCount[dimIndex[0]],(int) outCount[dimIndex[1]],(int) outCount[dimIndex[2]]);
	
	//
	// Translate the volume one slice at a time
	//
	// Set up counts to grab a z-slice of input
	for (int i = 0; i< ndimids; i++) count[i] = inCount[i];
	count[dimIndex[2]] = 1;
	//set up z traversal interval based on input data size:
	int zbegin = 0;
	int zend = ncdfdims[dimIDs[2]];
	int zinc = 1;
	//Prepare for reversing in z:
	if (opt.swapz) {
		zbegin = ncdfdims[dimIDs[2]]-1;
		zend = -1;
		zinc = -1;
	}
	int inSizeX = inCount[dimIndex[0]], inSizeY = inCount[dimIndex[1]];
	int outSizeX = outCount[dimIndex[0]], outSizeY = outCount[dimIndex[1]];

	//Handle z-staggered separate from un-staggered
	if(ncdfdims[dimIDs[2]] == dim[2]) {
		for(int z=zbegin; z!=zend; z+=zinc) {

			if (z%50 == 0 && ! opt.quiet) {
				cout << "Reading slice # " << z << endl;
			}

			double t1 = bufwriter->GetTime();
			start[dimIndex[2]] = z;
			
			if (xtype == NC_FLOAT) {
			
				nc_status = nc_get_vara_float(
					ncid, varid, start, count, inFBuffer
				);
				
				NC_ERR_READ(nc_status);
				*read_timer += bufwriter->GetTime() - t1;
				averageSlice(xtype,(void*)inFBuffer, (void*)outFBuffer, 
						inSizeX, inSizeY, outSizeX, outSizeY);

				
			} else if (xtype == NC_DOUBLE){
				
				nc_status = nc_get_vara_double(
					ncid, varid, start, count, inDBuffer
				);
				NC_ERR_READ(nc_status);
				*read_timer += bufwriter->GetTime() - t1;
				averageSlice(xtype,(void*)inDBuffer, (void*)outDBuffer, 
						inSizeX, inSizeY, outSizeX, outSizeY);
				//Convert to float:
				for(int i=0; i<dim[0]*dim[1]; i++) outFBuffer[i] = (float)outDBuffer[i];
			}
			//
			// Write a single slice of data
			//
			
			bufwriter->WriteSlice(outFBuffer);
			if (bufwriter->GetErrCode() != 0) {
				cerr << ProgName << ": " << bufwriter->GetErrMsg() << endl;
				exit(1);
			}
		} 
		
		

	} else { //handle staggered z-dimension
		if (xtype == NC_DOUBLE) outDBuffer2 = new double[outSize];
		else outFBuffer2 = new float[outSize];
		//Identify pointers to buffers.  These are swapped each time we
		//average two slices
		double* newDBuffer = outDBuffer2;
		double* oldDBuffer = outDBuffer;
		float* newFBuffer = outFBuffer2;
		float* oldFBuffer = outFBuffer;
		
		if (xtype == NC_DOUBLE) stagInDBuffer = new double[inSize];
		else stagInFBuffer = new float[inSize];

		
		//First, read newbuffer, average inside slice as needed
		double t1 = bufwriter->GetTime();
		start[dimIndex[2]] = zbegin;
		
		if (xtype == NC_FLOAT) {
		
			nc_status = nc_get_vara_float(
				ncid, varid, start, count, stagInFBuffer
			);
			
			NC_ERR_READ(nc_status);
			*read_timer += bufwriter->GetTime() - t1;
			averageSlice(xtype,(void*)stagInFBuffer, (void*)newFBuffer, 
					inSizeX, inSizeY, outSizeX, outSizeY);
			
		} else if (xtype == NC_DOUBLE){
			
			nc_status = nc_get_vara_double(
				ncid, varid, start, count, stagInDBuffer
			);
			NC_ERR_READ(nc_status);
			*read_timer += bufwriter->GetTime() - t1;
			averageSlice(xtype,(void*)stagInDBuffer, (void*)newDBuffer, 
					inSizeX, inSizeY, outSizeX, outSizeY);
		}
	
		
		//Repeatedly, loop over output lines, averaging each pair::
		for(int z=zbegin+zinc; z!=zend; z+=zinc) {

			if (z%50 == 1 && ! opt.quiet) {
				cout << "Reading slice # " << z << endl;
			}

			
			start[dimIndex[2]] = z;
			//  swap pointers
			float *tempFBuffer = newFBuffer;
			double* tempDBuffer = newDBuffer;
			newFBuffer = oldFBuffer;
			newDBuffer = oldDBuffer;
			oldFBuffer = tempFBuffer;
			oldDBuffer = tempDBuffer;
			//  read newBuffer
			
			if (xtype == NC_FLOAT) {
				double t1 = bufwriter->GetTime();
				nc_status = nc_get_vara_float(
					ncid, varid, start, count, stagInFBuffer
				);
				
				NC_ERR_READ(nc_status);
				*read_timer += bufwriter->GetTime() - t1;
				averageSlice(xtype,(void*)stagInFBuffer, (void*)newFBuffer, 
						inSizeX, inSizeY, outSizeX, outSizeY);
				//Average two slices putting result into oldBuffer
				for(int i=0; i<dim[0]*dim[1]; i++) oldFBuffer[i] = 0.5*(oldFBuffer[i]+newFBuffer[i]);
				//Write out oldFBuffer:
				bufwriter->WriteSlice(oldFBuffer);
				if (bufwriter->GetErrCode() != 0) {
					cerr << ProgName << ": " << bufwriter->GetErrMsg() << endl;
					exit(1);
				}
			} else if (xtype == NC_DOUBLE){
				double t1 = bufwriter->GetTime();
				nc_status = nc_get_vara_double(
					ncid, varid, start, count, stagInDBuffer
				);
				NC_ERR_READ(nc_status);
				*read_timer += bufwriter->GetTime() - t1;
				averageSlice(xtype,(void*)stagInDBuffer, (void*)newDBuffer, 
						inSizeX, inSizeY, outSizeX, outSizeY);
				//Average two slices putting result into outFBuffer
				for(int i=0; i<dim[0]*dim[1]; i++) outFBuffer[i] =(float)( 0.5*(oldDBuffer[i]+newDBuffer[i]));
				//Write out outFBuffer:
				bufwriter->WriteSlice(outFBuffer);
				if (bufwriter->GetErrCode() != 0) {
					cerr << ProgName << ": " << bufwriter->GetErrMsg() << endl;
					exit(1);
				}
			}
			
			
		}
		//Delete extra buffers:
		if (outFBuffer2) delete outFBuffer2;
		if (outDBuffer2) delete outDBuffer2;
	}
	
	if (inFBuffer && inFBuffer != outFBuffer) delete inFBuffer;
	if (inDBuffer && inDBuffer != outDBuffer)
		delete inDBuffer;

	delete outFBuffer;
	if (outDBuffer) delete outDBuffer;

	if(stagInDBuffer) delete stagInDBuffer;
	if(stagInFBuffer) delete stagInFBuffer;
	
}

void	process_slice(
	WaveletBlock3DRegionWriter *bufwriter,
	const size_t *dim, //dimensions from vdf
	const int ncid,
	double *read_timer
) {
	*read_timer = 0.0;

	// Check out the netcdf file.
	// It needs to have the specified variable name, and the specified dimension names
	// The dimensions must agree with the dimensions in the metadata, and they 
	// must be in the same order.
	size_t* ncdfdims;
	size_t* start;
	size_t* count;
	size_t* outCount, *inCount;
	

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

	//Need at least 2 dimensions
	if (ndimids < 2) {
		cerr << ProgName << ": Insufficient netcdf variable dimensions" << endl;
		exit(1);
	}
	
	bool foundXDim = false, foundYDim = false;
	int dimIDs[2];// dimension ID's (in netcdf file) for each of the 3 dimensions we are using
	int dimIndex[2] = {0,0}; //specify which dimension (for this variable) is associated with x,y,z
	// Initialize the count and start arrays for extracting slices from the data:
	count = new size_t[ndimids];
	start = new size_t[ndimids];
	outCount = new size_t[ndimids];
	inCount = new size_t[ndimids];
	for (int i = 0; i<ndimids; i++){
		outCount[i] = 1;
		inCount[i] = 1;
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
			constDimLen <= opt.constDimValues[i] ||
			opt.constDimValues[i] < 0)
		{
			fprintf(stderr, "Invalid value %d of constant dimension %s of length %d\n",
				opt.constDimValues[i],
				opt.constDimNames[i].c_str(),
				(int) constDimLen);
			exit(1);
		}
	}

	
	//Go through the dimensions looking for the 2 dimensions that we are using,
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
				outCount[i] = dim[0];
				inCount[i] = ncdfdims[dimIDs[0]];
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
				outCount[i] = dim[1];
				inCount[i] = ncdfdims[dimIDs[1]];
				
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
	if (!foundYDim || !foundXDim){
		fprintf(stderr, "A specified NetCDF dimension name was not used with specified variable.\n");
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

	size_t outSize = outCount[dimIndex[0]]*inCount[dimIndex[1]];
	size_t inSize = inCount[dimIndex[0]]*inCount[dimIndex[1]];
	//allocate a buffer big enough for input 2D slice :
	
	float* inFBuffer = 0, *outFBuffer = 0;
	double *inDBuffer = 0, *outDBuffer = 0;

	
	if (xtype == NC_DOUBLE) inDBuffer = new double[inSize];
	else inFBuffer = new float[inSize];

	// If data is staggered in x, need additional buffer for output

	if (inCount[dimIndex[0]]>outCount[dimIndex[0]]){
		if (!opt.quiet) fprintf(stderr, "variable is staggered in x\n");
		if (xtype == NC_DOUBLE) outDBuffer = new double[outSize];
		else outFBuffer = new float[outSize];
	} else {
		outFBuffer = inFBuffer;
		outDBuffer = inDBuffer;
	}
	//Always need an outFBuffer, (for conversion if data is double)
	if (!outFBuffer) outFBuffer = new float[outSize];

	if (inCount[dimIndex[1]]>outCount[dimIndex[1]] && !opt.quiet)
		fprintf(stderr, "variable is staggered in y\n");
	

	if(!opt.quiet) fprintf(stderr, "dimensions of output array are: %d %d\n",
		(int) outCount[dimIndex[0]],(int) outCount[dimIndex[1]]);
	
	//
	// Translate the volume one slice at a time
	//
	// Set up counts to grab a slice of input
	for (int i = 0; i< ndimids; i++) count[i] = inCount[i];
	
	int inSizeX = inCount[dimIndex[0]], inSizeY = inCount[dimIndex[1]];
	int outSizeX = outCount[dimIndex[0]], outSizeY = outCount[dimIndex[1]];


	double t1 = bufwriter->GetTime();
	
	if (xtype == NC_FLOAT) {
	
		nc_status = nc_get_vara_float(
			ncid, varid, start, count, inFBuffer
		);
		
		NC_ERR_READ(nc_status);
		*read_timer += bufwriter->GetTime() - t1;
		averageSlice(xtype,(void*)inFBuffer, (void*)outFBuffer, 
				inSizeX, inSizeY, outSizeX, outSizeY);

		
	} else if (xtype == NC_DOUBLE){
		
		nc_status = nc_get_vara_double(
			ncid, varid, start, count, inDBuffer
		);
		NC_ERR_READ(nc_status);
		*read_timer += bufwriter->GetTime() - t1;
		averageSlice(xtype,(void*)inDBuffer, (void*)outDBuffer, 
				inSizeX, inSizeY, outSizeX, outSizeY);
		//Convert to float:
		for(int i=0; i<dim[0]*dim[1]; i++) outFBuffer[i] = (float)outDBuffer[i];
	}
	//
	// Write the slice of data
	//
	
	bufwriter->WriteRegion(outFBuffer);
	if (bufwriter->GetErrCode() != 0) {
		cerr << ProgName << ": " << bufwriter->GetErrMsg() << endl;
		exit(1);
	}
	
	if (inFBuffer && inFBuffer != outFBuffer) delete inFBuffer;
	if (inDBuffer && inDBuffer != outDBuffer)
		delete inDBuffer;

	delete outFBuffer;
	if (outDBuffer) delete outDBuffer;
	
}


int	main(int argc, char **argv) {

	OptionParser op;
	
	const char	*metafile;
	const char	*netCDFfile;

	double	timer = 0.0;
	double	read_timer = 0.0;
	string	s;

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
		cerr << "Usage: " << ProgName << " [options] -varname variable_name -dimnames xdim:ydim:zdim metafile.vdf NetCDFfile" << endl;
		cerr << "  Converts (2D or 3D) variable in NetCDF file to variable in VAPOR VDC." << endl;
		cerr << "  X dimension of variable must be after Y dimension in NetCDF file." << endl;
		cerr << "  2D variables use X and Y dimension names." << endl;
		cerr << "  If a NetCDF dimension is one greater than the corresponding VDC dimension," << endl;
		cerr << "  data is averaged to the VDC dimension, to support staggered grids" << endl;
		cerr << "  Options are:" << endl;
		op.PrintOptionHelp(stderr);
		exit(argc != 3);
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
	if (strcmp(opt.dimnames[0].c_str(),"???") == 0){
		cerr << "Dimension names of the variable must be specified" << endl;
		exit(1);
	}
	if (strcmp(opt.varname,"???????") == 0){
		cerr << "The name of the variable in the NetCDF file must be specified." << endl;
		exit(1);
	}

	metafile = argv[1];	// Path to a vdf file
	netCDFfile = argv[2];	// Path to raw data file 

    if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	// If the netcdf variable name was not specified, use the same name as
	//  in the vdf
	if (strcmp(opt.ncdfvarname,"???????") == 0){
		opt.ncdfvarname = opt.varname;
	}

	
	// Determine if variable is 3D
	//
	MetadataVDC metadata (metafile);
	if (MetadataVDC::GetErrCode() != 0) {
		MyBase::SetErrMsg("Error processing metafile \"%s\"", metafile);
		exit(1);
	}
	Metadata::VarType_T vtype = metadata.GetVarType(opt.varname);
	if (vtype == Metadata::VARUNKNOWN) {
		MyBase::SetErrMsg("Unknown variable \"%s\"", opt.varname);
		exit(1);
	}

	//
	// Create an appropriate WaveletBlock writer. 
	//
	WaveletBlockIOBase *wbwriter3D;
	if (vtype == Metadata::VAR3D) {
		wbwriter3D = new WaveletBlock3DBufWriter(metadata);
	} else {
		wbwriter3D = new WaveletBlock3DRegionWriter(metafile);
	}
	
	if (wbwriter3D->OpenVariableWrite(opt.ts, opt.varname, opt.level) < 0) {
		exit(1);
	} 


	//
	// If pre version 2, create a backup of the .vdf file. The 
	// translation process will generate a new .vdf file
	//
	if (metadata.GetVDFVersion() < 2) save_file(metafile);

	
    int nc_status;
    int ncid;

    nc_status = nc_open(netCDFfile, NC_NOWRITE, &ncid);
	NC_ERR_READ(nc_status);

   
	// Get the dimensions of the volume
	//
	const size_t *dim = metadata.GetDimension();


	double t0 = wbwriter3D->GetTime();
	if (vtype == Metadata::VAR3D){
		process_volume(
			(WaveletBlock3DBufWriter *) wbwriter3D, dim, ncid, &read_timer
		);
	}
	else {
		process_slice(
			(WaveletBlock3DRegionWriter *) wbwriter3D, dim, ncid, &read_timer
		);
	}

	wbwriter3D->CloseVariable();
	if (wbwriter3D->GetErrCode() != 0) {
		exit(1);
	}
	timer = wbwriter3D->GetTime() - t0;
	
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
	if (metadata.GetVDFVersion() < 2) {
		metadata.Write(metafile);
	}
	nc_close(ncid);
	exit(0);
}

