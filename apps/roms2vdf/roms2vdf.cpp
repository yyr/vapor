
//
// $Id$
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
//      File:		roms2vdf.cpp
//
//      Author:         John Clyne 
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:          	August 2012 
//
//      Description:	Read ROMS files containing a 3D array of floats or doubles
//			and insert the volume into an existing
//			Vapor Data Collection
//


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
#include <algorithm>
#include <climits>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/MetadataROMS.h>
#include <vapor/ROMS.h>
#include <vapor/WeightTable.h>
#include <vapor/WaveletBlock3DBufWriter.h>
#include <vapor/WaveletBlock3DRegionWriter.h>
#include <vapor/WaveCodecIO.h>

#ifdef _WINDOWS 
#include "windows.h"
#endif

using namespace VetsUtil;
using namespace VAPoR;

#define NC_ERR_READ(nc_status) \
    if (nc_status != NC_NOERR) { \
        cerr <<  ProgName << \
            " : Error reading netCDF file at line " <<  __LINE__  << \
			" : " << nc_strerror(nc_status) << endl; \
		return(-1); \
    }

//
//	Command line argument stuff
//
struct opt_t {
	vector<string> varnames;
	vector<string> missval;
	int numts;
	int level;
	int lod;
	int nthreads;
	float tolerance;
	OptionParser::Boolean_T	noelev;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"varnames",1, 	"",	"Colon delimited list of variable names in ROMS files to convert. The default is to convert variables in the set intersection of those variables found in the VDF file and those in the ROMS files."},
	{"numts",	1,	"-1","Maximum number of time steps that may be converted. A -1 implies the conversion of all time steps found"},
	{"level",	1, 	"-1","Refinement levels saved. 0=>coarsest, 1=>next refinement, etc. -1=>finest"},
	{"lod",	1, 	"-1",	"Compression levels saved. 0 => coarsest, 1 => "
		"next refinement, etc. -1 => all levels defined by the .vdf file"},
	{"nthreads",1, 	"0",	"Number of execution threads (0 => # processors)"},
	{"tolerance",	1, 	"0.0000001","Tolerance for comparing relative user times"},
	{"help",	0,	"",	"Print this message and exit."},
	{"debug",	0,	"",	"Enable debugging."},
	{"quiet",	0,	"",	"Operate quietly (outputs only vertical extents that are lower than those in the VDF)."},
	{NULL}
};

OptionParser::Option_T	get_options[] = {
	{"varnames", VetsUtil::CvtToStrVec, &opt.varnames, sizeof(opt.varnames)},
	{"numts", VetsUtil::CvtToInt, &opt.numts, sizeof(opt.numts)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"lod", VetsUtil::CvtToInt, &opt.lod, sizeof(opt.lod)},
	{"nthreads", VetsUtil::CvtToInt, &opt.nthreads, sizeof(opt.nthreads)},
	{"tolerance", VetsUtil::CvtToFloat, &opt.tolerance, sizeof(opt.tolerance)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};

const char	*ProgName;

//Method to be used when a float array is used for all time steps
int CopyConstantVariable3D(
				 const float *dataValues,
				 VDFIOBase *vdfio3d,
				 int level,
				 int lod, 
				 const char* varname,
				 const size_t dim[3],
				 size_t tsVDC) {
	
	static size_t sliceBufferSize = 0;
	static float *sliceBuffer = NULL;
	
	int rc;
	
	rc = vdfio3d->OpenVariableWrite(tsVDC, varname, level, lod);

	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to open for write ROMS variable %s at time step %d",
			varname, tsVDC
		);
		return (-1);
	}
	
	size_t slice_sz = dim[0] * dim[1];
	
	if (sliceBufferSize < slice_sz) {
		if (sliceBuffer) delete [] sliceBuffer;
		sliceBuffer = new float[slice_sz];
		sliceBufferSize = slice_sz;
	}
	
	for (size_t z = 0; z<dim[2]; z++) {
		//fill the buffer with the appropriate value:
		for (size_t i = 0; i< slice_sz; i++){
			sliceBuffer[i] = dataValues[i+slice_sz*z];
		}
		if (vdfio3d->WriteSlice(sliceBuffer) < 0) {
			MyBase::SetErrMsg(
							  "Failed to write ROMS variable %s slice at time step %d ",
							  varname, tsVDC
							  );
			return (-1);
		}
	} // End of for z.
	
	vdfio3d->CloseVariable();
	
	return(0);
}  // End of CopyConstantVariable3D.
float* calcConst2DVar(float* rawData, const size_t dimsVDC[3], ROMS* roms, float* sample2DVar){
	if (!rawData) return 0;
	float* mappedData = new float[dimsVDC[0]*dimsVDC[1]];
	// use rho-grid for remapping 
	WeightTable *wt = roms->GetWeightTable(3);
		
	wt->interp2D(rawData, mappedData, (float)ROMS::vaporMissingValue(), dimsVDC);
		
	float minval = 1.e30;
	float maxval = -1.e30;
	float minval1 = 1.e30;
	float maxval1 = -1.e30;
	for (int i = 0; i<dimsVDC[0]*dimsVDC[1]; i++){
		assert(rawData[i]> -1.e10 && rawData[i] < 1.e10);
		if(rawData[i]<minval) minval = rawData[i];
		if(rawData[i]>maxval) maxval = rawData[i];
		if (mappedData[i] == (float)ROMS::vaporMissingValue())
			continue;
		if (sample2DVar && sample2DVar[i] == (float)ROMS::vaporMissingValue()){
			mappedData[i] = (float)ROMS::vaporMissingValue();
			continue;
		}
		if(mappedData[i]<minval1) minval1 = mappedData[i];
		if(mappedData[i]>maxval1) maxval1 = mappedData[i];
	}
	delete rawData;
	return mappedData;
}
float * CalcElevation(int Vtransform, float* s_rho, float* Cs_r, float Tcline, float* mappedDepth, const size_t* dimsVDC, float* sample3DVar){
	//Following code to calculate ELEVATION variable was provided by Justin Small (NCAR)
	// The elevation is calculated on the VDC grid. The mappedDepth must already be available
	//The sample3DVar is used to establish a mask for missing values
	if (s_rho[0] < -1. || Cs_r[0] < -1. || Tcline < -1. || Vtransform < -1 || mappedDepth == 0) return 0;
	float * z_r = new float[dimsVDC[0]*dimsVDC[1]*dimsVDC[2]];
	float maxElev = -1.e30f;
	float minElev = 1.e30f;
	int levelSize = dimsVDC[0]*dimsVDC[1];
	//Define lowest z level (seabed):  Note:  this is overwritten.
	for (int i = 0; i<levelSize; i++){
		z_r[i] = mappedDepth[i];
	}
	if (Vtransform == 1){ //Old method
		float* hc = new float[levelSize];
		for (int i = 0; i<levelSize; i++){
			if( -mappedDepth[i] < Tcline) hc[i] = -mappedDepth[i];
			else hc[i] = Tcline;
		}
		for (int k = 0; k<dimsVDC[2]; k++){
			for (int i = 0; i<dimsVDC[1]; i++){
				for (int j = 0; j< dimsVDC[0]; j++){
					if (mappedDepth[j+dimsVDC[0]*i] == (float)ROMS::vaporMissingValue()){
//						z_r[j + dimsVDC[0]*i + dimsVDC[0]*dimsVDC[1]*k] = (float)ROMS::vaporMissingValue();
						z_r[j + dimsVDC[0]*i + dimsVDC[0]*dimsVDC[1]*k] = 0.0;
						continue;
					}
					float cff_r = hc[j+dimsVDC[0]*i]*(s_rho[k] - Cs_r[k]);
					float cff1_r = Cs_r[k];
					//float cff2_r = s_rho[k]+1.;??? not used
					//Depth of sigma coordinate at rho points
					float z_r0 = cff_r - cff1_r*mappedDepth[j+dimsVDC[0]*i];
					z_r[j + dimsVDC[0]*i + dimsVDC[0]*dimsVDC[1]*k] = z_r0;  //Note, if zeta is zero, hinv not needed here
					float val = z_r[j + dimsVDC[0]*i + dimsVDC[0]*dimsVDC[1]*k];
					if (val < minElev) minElev = val;
					if (val > maxElev) maxElev = val;
					
				}
			}
		}
	} else {
		
		for (int k = 0; k< dimsVDC[2]; k++){
			for (int i = 0; i<dimsVDC[1]; i++){
				for (int j = 0; j<dimsVDC[0]; j++){
					if (mappedDepth[j+dimsVDC[0]*i] == (float)ROMS::vaporMissingValue()){
//						z_r[j + dimsVDC[0]*i + dimsVDC[0]*dimsVDC[1]*k] = (float)ROMS::vaporMissingValue();
						z_r[j + dimsVDC[0]*i + dimsVDC[0]*dimsVDC[1]*k] = 0.0;
						continue;
					}
					float hinv = 1./(Tcline -mappedDepth[j+dimsVDC[0]*i]);
					float cff_r = Tcline*s_rho[k];
					float cff1_r = Cs_r[k];
					float cff2_r = (cff_r-cff1_r*mappedDepth[j+dimsVDC[0]*i])*hinv;
					z_r[j + dimsVDC[0]*i + dimsVDC[0]*dimsVDC[1]*k] = -mappedDepth[j+dimsVDC[0]*i]*cff2_r;	
					float val = z_r[j + dimsVDC[0]*i + dimsVDC[0]*dimsVDC[1]*k];
					if (val < minElev) minElev = val;
					if (val > maxElev) maxElev = val;
				}
			}
		}
	}
#ifdef	DEAD
	//Set missing values, get max and min
	maxElev = -1.e30f;
	minElev = 1.e30f;
	if (sample3DVar){
		for (int k = 0; k<dimsVDC[2]; k++){
			for (int i = 0; i<dimsVDC[1]; i++){
				for (int j = 0; j< dimsVDC[0]; j++){
					if (sample3DVar[j + dimsVDC[0]*i + dimsVDC[0]*dimsVDC[1]*k] == (float)ROMS::vaporMissingValue()){
//						z_r[j + dimsVDC[0]*i + dimsVDC[0]*dimsVDC[1]*k] = (float)ROMS::vaporMissingValue();
						z_r[j + dimsVDC[0]*i + dimsVDC[0]*dimsVDC[1]*k] = 0.0;
					}
					else {
						float val = z_r[j + dimsVDC[0]*i + dimsVDC[0]*dimsVDC[1]*k];
						if (val == (float)ROMS::vaporMissingValue()) 
							continue;
						if (val < minElev) minElev = val;
						if (val > maxElev) maxElev = val;
					}
				}
			}
		}
	}
#endif
 	return z_r;
}
//
// Following method is used to create depth variable.  Data is provided in a 2D float array
int CopyConstantVariable2D(
				   const float* dataValues,
				   VDFIOBase *vdfio2d,
				   WaveletBlock3DRegionWriter *wb2dwriter,
				   int level,
				   int lod, 
				   const char* varname,
				   const size_t dim[3],
				   size_t tsVDC) {
	
	int rc;
	rc = vdfio2d->OpenVariableWrite(tsVDC, varname, level, lod);
	if (rc<0) {
		MyBase::SetErrMsg(
						  "Failed to copy ROMS variable %s at time step %d",
						  varname, tsVDC
						  );
		return (-1);
	}
	
 		
	
	if (vdfio2d->WriteRegion(dataValues) < 0) {
		MyBase::SetErrMsg(
						  "Failed to write ROMS variable %s at time step %d",
						  varname, tsVDC
						  );
		return (-1);
	}
	
	vdfio2d->CloseVariable();
	
	return(0);
} // End of CopyConstantVariable2D.


int CopyVariable3D(
	int ncid,
	int varid,
	WeightTable *wt,
	VDFIOBase *vdfio3d,
	int level,
	int lod, 
	string varname,
	const size_t dim[3],
	int ndim[2],
	size_t tsVDC,
	int tsNetCDF,
	float** resultVar,
	bool staggered
	) {

	static float *fsliceBuffer = NULL;
	static float *sliceBuffer2 = NULL;
	static double *dsliceBuffer = NULL;
	static float *prevSliceBuffer = NULL;

	nc_type vartype;
	NC_ERR_READ( nc_inq_vartype(ncid, varid,  &vartype) );
	bool isDouble = (vartype == NC_DOUBLE); //either float or double
			
	int rc;
	
	rc = vdfio3d->OpenVariableWrite(tsVDC, varname.c_str(), level, lod);

	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to open for write ROMS variable %s at time step %d",
			varname.c_str(), tsVDC
		);
		return (-1);
	}
	
	float fmissVal = (float)ROMS::vaporMissingValue();
	double dmissVal = ROMS::vaporMissingValue();
	if (isDouble)
		rc = nc_get_att_double(ncid, varid, "_FillValue", &dmissVal);
	else 
		rc = nc_get_att_float(ncid, varid, "_FillValue", &fmissVal);

	
	size_t slice_sz = dim[0] * dim[1];
	int inputSize = ndim[0]*ndim[1];
	size_t starts[4] = {0,0,0,0};
	size_t counts[4] = {1,1,1,1};
	starts[0]=tsNetCDF;
	counts[2]=ndim[1];
	counts[3]=ndim[0];

	
	if (fsliceBuffer) {
		delete [] fsliceBuffer;
		delete [] sliceBuffer2;
		fsliceBuffer=0;
	}
	if (dsliceBuffer) {
		delete [] dsliceBuffer;
		delete [] sliceBuffer2;
		dsliceBuffer = 0;
	}
	if (prevSliceBuffer){
		delete prevSliceBuffer;
		prevSliceBuffer = 0;
	}
	if(isDouble){
		dsliceBuffer = new double[inputSize];
		sliceBuffer2 = new float[slice_sz];
	} else {
		fsliceBuffer = new float[inputSize];
		sliceBuffer2 = new float[slice_sz];
	}
		
	float minVal = 1.e30f;
	float maxVal = -1.e30f;
	float minVal1 = 1.e30f;
	float maxVal1 = -1.e30f;
	bool makeMask = (*resultVar == 0);
	if (makeMask) *resultVar = new float[dim[0]*dim[1]*dim[2]];
	int zfirst = 0;
	int zlast = dim[2];
	float* outBuffer = sliceBuffer2;
	if (staggered){//start by reading the first slice
		//Always put the current slice as it is read into the "lastslice" buffer
		zfirst = 1;
		zlast = dim[2]+1;
		starts[1]=0;
		prevSliceBuffer = new float[slice_sz];
		if (isDouble){
			NC_ERR_READ(nc_get_vara_double(ncid, varid, starts, counts, dsliceBuffer))
			for (int k = 0; k<slice_sz; k++){
				if (dsliceBuffer[k] != dmissVal){
					if (minVal > dsliceBuffer[k]) minVal = dsliceBuffer[k];
					if (maxVal < dsliceBuffer[k]) maxVal = dsliceBuffer[k];
				}
			}
			wt->interp2D(dsliceBuffer, prevSliceBuffer, dmissVal,dim);
		}
		else {
			NC_ERR_READ(nc_get_vara_float(ncid, varid, starts, counts, fsliceBuffer))
			for (int k = 0; k<slice_sz; k++){
				if (fsliceBuffer[k] != fmissVal){
					if (minVal > fsliceBuffer[k]) minVal = fsliceBuffer[k];
					if (maxVal < fsliceBuffer[k]) maxVal = fsliceBuffer[k];
				}
			}
			wt->interp2D(fsliceBuffer, prevSliceBuffer, fmissVal,dim);
		}
	}
	for (size_t z = zfirst; z<zlast; z++) {
		//
		//Read slice of NetCDF file into slice buffer 2
		//
		starts[1]= z;
		if (isDouble){
			NC_ERR_READ(nc_get_vara_double(ncid, varid, starts, counts, dsliceBuffer))
			for (int k = 0; k<slice_sz; k++){
				if (dsliceBuffer[k] != dmissVal){
					if (minVal > dsliceBuffer[k]) minVal = dsliceBuffer[k];
					if (maxVal < dsliceBuffer[k]) maxVal = dsliceBuffer[k];
				}
			}
			wt->interp2D(dsliceBuffer, sliceBuffer2, dmissVal,dim);
		}
		else {
			NC_ERR_READ(nc_get_vara_float(ncid, varid, starts, counts, fsliceBuffer))
			for (int k = 0; k<slice_sz; k++){
				if (fsliceBuffer[k] != fmissVal){
					if (minVal > fsliceBuffer[k]) minVal = fsliceBuffer[k];
					if (maxVal < fsliceBuffer[k]) maxVal = fsliceBuffer[k];
				}
			}
			wt->interp2D(fsliceBuffer, sliceBuffer2, fmissVal,dim);
		}
		
		for (int k = 0; k<slice_sz; k++){
			if (sliceBuffer2[k] != (float)ROMS::vaporMissingValue()){
				if (minVal1 > sliceBuffer2[k]) minVal1 = sliceBuffer2[k];
				if (maxVal1 < sliceBuffer2[k]) maxVal1 = sliceBuffer2[k];
			}
		}
		//If staggered, average the two slices into prevSliceBuffer;
		if (staggered){
			for (int k = 0; k<slice_sz; k++){
				if (sliceBuffer2[k] == (float)ROMS::vaporMissingValue() ||
					prevSliceBuffer[k] == (float)ROMS::vaporMissingValue())
						prevSliceBuffer[k] = (float)ROMS::vaporMissingValue();
				else 
					prevSliceBuffer[k] = 0.5*(prevSliceBuffer[k]+sliceBuffer2[k]);
			}
			outBuffer = prevSliceBuffer;	
		} 

		if (vdfio3d->WriteSlice(outBuffer) < 0) {
			MyBase::SetErrMsg(
				"Failed to write ROMS variable %s slice at time step %d (vdc2)",
				varname.c_str(), tsVDC
			);
			return (-1);
		}
		if (makeMask){  //copy result slice to resultVar:
			int zlev = z;
			if (staggered) zlev--;
			for (int p = 0; p<slice_sz; p++){
				(*resultVar)[p+zlev*slice_sz] = outBuffer[p];
			}
		}
		if (staggered) //copy to prevSliceBuffer;
			for (int k = 0; k<slice_sz; k++) prevSliceBuffer[k] = sliceBuffer2[k];
		
	} // End of for z.

	vdfio3d->CloseVariable();

	return(0);
}  // End of CopyVariable3D.

int CopyVariable2D(
	int ncid,
	int varid,
	WeightTable* wt,
	VDFIOBase *vdfio2d,
	WaveletBlock3DRegionWriter *wb2dwriter,
	int level,
	int lod, 
	string varname,
	const size_t dim[3],
	int ndim[2],
	size_t tsVDC,
	int tsNetCDF,
	float** sample2DVar
	
) {

	static float *fsliceBuffer = NULL;
	static double *dsliceBuffer = NULL;
	static float *sliceBuffer2 = NULL;

	nc_type vartype;
	NC_ERR_READ( nc_inq_vartype(ncid, varid,  &vartype) );
	bool isDouble = (vartype == NC_DOUBLE); //either float or double

	
	int rc;

	rc = vdfio2d->OpenVariableWrite(tsVDC, varname.c_str(), level, lod);
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to copy ROMS variable %s at time step %d",
			varname.c_str(), tsVDC
		);
		return (-1);
	}
	
	
	double dmissingVal = ROMS::vaporMissingValue();
	float fmissingVal = (float)dmissingVal;
	if (isDouble)
		rc = nc_get_att_double(ncid, varid, "_FillValue", &dmissingVal);
	else 
		rc = nc_get_att_float(ncid, varid, "_FillValue", &fmissingVal);

 	size_t slice_sz = dim[0] * dim[1];
	size_t starts[3] = {0,0,0};
	size_t counts[3];
	starts[0]=tsNetCDF;
	counts[0]=1;
	counts[1]=ndim[1];
	counts[2]=ndim[0];
	

	if (fsliceBuffer) {
		delete [] fsliceBuffer;
		delete [] sliceBuffer2;
		fsliceBuffer=0;
	}
	if (dsliceBuffer) {
		delete [] dsliceBuffer;
		delete [] sliceBuffer2;
		dsliceBuffer = 0;
	}
	if(isDouble){
		dsliceBuffer = new double[ndim[0]*ndim[1]];
		sliceBuffer2 = new float[slice_sz];
	} else {
		fsliceBuffer = new float[ndim[0]*ndim[1]];
		sliceBuffer2 = new float[slice_sz];
	}

	//
	// Read the variable into the temp buffer and interpolate
	//
	if (isDouble){
		NC_ERR_READ(nc_get_vara_double(ncid, varid, starts, counts, dsliceBuffer))
		wt->interp2D(dsliceBuffer, sliceBuffer2, dmissingVal,dim);
	} else {
		NC_ERR_READ(nc_get_vara_float(ncid, varid, starts, counts, fsliceBuffer))
		wt->interp2D(fsliceBuffer, sliceBuffer2, fmissingVal,dim);
	}
	float slicemax = -1.e38;
	float slicemin = 1.e38;
	for (int j = 0; j<dim[0]*dim[1]; j++){
		if (sliceBuffer2[j] == (float)ROMS::vaporMissingValue()) continue;
		if (sliceBuffer2[j]>slicemax) slicemax = sliceBuffer2[j];
		if (sliceBuffer2[j]<slicemin) slicemin = sliceBuffer2[j];
	}
	if (!(*sample2DVar)){
		*sample2DVar = new float[dim[0]*dim[1]];
		for (int j = 0; j< dim[0]*dim[1]; j++) (*sample2DVar)[j] = sliceBuffer2[j]; 
	}
	if (vdfio2d->WriteRegion(sliceBuffer2) < 0) {
		MyBase::SetErrMsg(
			"Failed to write ROMS variable %s at ROMS time step %d",
			varname.c_str(), tsVDC
		);
		return (-1);
	}

	vdfio2d->CloseVariable();

	return(0);
} // End of CopyVariable2D.

void ErrMsgCBHandler(const char *msg, int) {
	cerr << ProgName << " : " << msg << endl;
}

int	main(int argc, char **argv) {
	OptionParser op;

	int estatus = 0;
	
	// Parse command line arguments and check for errors
	ProgName = Basename(argv[0]);
	if (op.AppendOptions(set_opts) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	MyBase::SetErrMsgCB(ErrMsgCBHandler);

	if (opt.help) 
	{
		cerr << "Usage: " << ProgName << " [options] ROMS_netcdf_datafile(s)... ROMS_topo_file vdf_file" << endl << endl;
		op.PrintOptionHelp(stderr);
		exit(0);
	}

	if (argc < 3) {
		cerr << "Usage: " << ProgName << " [options] ROMS_netcdf_datafile(s)... ROMS_topo_file vdf_file" << endl;
		op.PrintOptionHelp(stderr);
		exit(1);
	}

	if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	argv++;
	argc--;
	//
	// At this point there is at least one ROMS file to work with.
	// The format is roms-files + topo-file + vdf-file.
	//
	
	vector<string> romsfiles;
	for (int i=0; i<argc-2; i++) {
		romsfiles.push_back(argv[i]);
	}
	string topofile = string(argv[argc-2]);
	string metafile = string(argv[argc-1]);
	
	vector<string> varnames = opt.varnames;

	WaveletBlock3DBufWriter *wbwriter3d = NULL;
	WaveletBlock3DRegionWriter *wbwriter2d = NULL;
	WaveCodecIO	*wcwriter3d = NULL;
	VDFIOBase* vdfio3d = NULL, *vdfio2d=NULL;
	
	MetadataVDC	*metadataVDC = new MetadataVDC(metafile);
	if (MetadataVDC::GetErrCode() != 0) { 
		MyBase::SetErrMsg("Error processing VDC metafile : %s",metafile.c_str());
		exit(1);
	}

	
	bool vdc1 = (metadataVDC->GetVDCType() == 1);

	if(vdc1) {
		wbwriter3d = new WaveletBlock3DBufWriter(*metadataVDC);
		vdfio3d = wbwriter3d;

		//
		// Ugh! WaveletBlock3DBufWriter class does not handle 2D data!!!  
		// Also does want to work form VDFIOBase.
		//
		wbwriter2d = new WaveletBlock3DRegionWriter(*metadataVDC);
		vdfio2d = wbwriter2d;
	}
	else {
		wcwriter3d = new WaveCodecIO(*metadataVDC, opt.nthreads);
		vdfio3d = wcwriter3d;
		vdfio2d = vdfio3d;
	}
	if (! vdfio3d || ! vdfio2d) {
		MyBase::SetErrMsg("Error processing VDC metafile : %s",metafile.c_str());
		exit(1);
	}
	
	vector <string> varsVDC = metadataVDC->GetVariableNames();
	const size_t *dimsVDC = metadataVDC->GetDimension();
	
	for (vector<string>::iterator itr = opt.varnames.begin(); itr!=opt.varnames.end(); itr++) {
		if (find(varsVDC.begin(),varsVDC.end(),*itr)==varsVDC.end()) {
			MyBase::SetErrMsg(
				"Requested variable \"%s\" not found in VDF file %s",
				itr->c_str(), metafile.c_str());
			exit(1);
		}
	}
		

	//
	// Deal with required variables that may have atypical names
	//
	map <string, string> atypnames;
	string tag("DependentVarNames");
	string s = metadataVDC->GetUserDataString(tag);
	if (! s.empty()) {
		vector <string> svec;
		CvtToStrVec(s.c_str(), &svec);
		if (svec.size() != 12) {
			MyBase::SetErrMsg("Invalid ROMS DependentVarNames tag: %s",tag.c_str());
			return(-1);
		}
		
		atypnames["ocean_time"] = svec[0];
		atypnames["h"] = svec[1];
		atypnames["xi_rho"] = svec[2];
		atypnames["xi_psi"] = svec[3];
		atypnames["xi_u"] = svec[4];
		atypnames["xi_v"] = svec[5];
		atypnames["eta_rho"] = svec[6];
		atypnames["eta_psi"] = svec[7];
		atypnames["eta_u"] = svec[8];
		atypnames["eta_v"] = svec[9];
		atypnames["s_rho"] = svec[10];
		atypnames["s_w"] = svec[11];
		
	}
	//
	// Create a ROMS instance to get info from the topofile
	//
	
	// The ROMS constructor will check the topofile,
	// initialize the variable names
	ROMS* roms = new ROMS(topofile, atypnames, varnames, varnames);
	if (ROMS::GetErrCode() != 0) {
		ROMS::SetErrMsg(
			"Error processing topo file %s, exiting.",topofile.c_str()
					   );
        exit(1);
    }
	
	//
	// Verify that the ROMS matches the VDF file
	//
	
	//
	// Ask the ROMS to create all weighttables
	//
	// There needs to be a weightTable for each combination of geolat/geolon variables
	int rc = roms->MakeWeightTables();
	if (rc) exit (rc);
	
	vector<string> vdcvars2d = metadataVDC->GetVariables2DXY();
	vector<string> vdcvars3d = metadataVDC->GetVariables3D();
	vector<size_t>VDCTimes;	
	
	//Create an array of times in the VDC:
	vector<double> usertimes;
	for (size_t ts = 0; ts < metadataVDC->GetNumTimeSteps(); ts++){
		usertimes.push_back(metadataVDC->GetTSUserTime(ts));
	}

	//initialize values needed for calculation of ELEVATION (s_rho, Cs_r, Vtransform, Tcline)
	int Vtransform = -2;
	float* s_rho = new float[dimsVDC[2]];
	float* Cs_r = new float[dimsVDC[2]];
	for (int j = 0; j< dimsVDC[2]; j++){
		s_rho[j] = -2.;
		Cs_r[j] = -2;
	}
	float Tcline = -2.;
	float * elevation = 0;
	float * sample3DVar = 0;
	float * sample2DVar = 0;
	float* mappedDepth=0;
	float* mappedLats = 0;
	float* mappedAngles = 0;
	//Loop thru romsfiles
	for (int i = 0; i<romsfiles.size(); i++){
		printf("processing file %s\n",romsfiles[i].c_str());	
		
		//For each romsfile:
		//Open the file
		int ncid;
		char nctimename[NC_MAX_NAME+1];
		int timedimid;
		size_t timelen;
		NC_ERR_READ( nc_open( romsfiles[i].c_str(), NC_NOWRITE, &ncid ));
		
		// Read the times from the file
		int timevarid;
		int rc = nc_inq_varid(ncid, atypnames["ocean_time"].c_str(), &timevarid);
		if (rc){
			MyBase::SetErrMsg("Time variable (named: %s) not in file %s, skipping",
				atypnames["ocean_time"].c_str(),romsfiles[i].c_str());
			MyBase::SetErrCode(0);
			continue;
		}
		
		roms->extractStartTime(ncid,timevarid);

		
		NC_ERR_READ(nc_inq_unlimdim(ncid, &timedimid));
		
		NC_ERR_READ(nc_inq_dim(ncid, timedimid, nctimename, &timelen));

		size_t timestart[1] = {0};
		size_t timecount[1];
		timecount[0] = timelen;
		double* fileTimes = new double[timelen];
		VDCTimes.clear();
		NC_ERR_READ(nc_get_vara_double(ncid, timevarid, timestart, timecount, fileTimes));
		
		// map the times to VDC timesteps, using MetadataROMS::GetVDCTimes()
		for (int j = 0; j< timelen; j++){
			size_t ts = roms->GetVDCTimeStep(fileTimes[j],usertimes);
			if (ts == (size_t)-1) {
				MyBase::SetErrMsg(" Time step %d in file %s does not correspond to a valid time in the VDC",
								  j, romsfiles[i].c_str());
				exit (-2);
			}
			VDCTimes.push_back(ts);
		}

		
		//Loop thru its variables (2d & 3d):
		int nvars;
	
		NC_ERR_READ( nc_inq_nvars(ncid, &nvars ) )
		for (int varid = 0; varid<nvars; varid++){
			//place to hold variable name
			char varname[NC_MAX_NAME+1];
			nc_type vartype;
			NC_ERR_READ( nc_inq_vartype(ncid, varid,  &vartype) );
			if (vartype == NC_INT){
				NC_ERR_READ(nc_inq_varname(ncid, varid, varname))
				if (string("Vtransform") != varname) continue;
				NC_ERR_READ(nc_get_var_int(ncid, varid, &Vtransform))
			}
			if (vartype != NC_FLOAT && vartype != NC_DOUBLE) continue;
			int ndims;
			NC_ERR_READ(nc_inq_varndims(ncid, varid, &ndims));
			if (ndims == 0){ //Look for Tcline
				NC_ERR_READ(nc_inq_varname(ncid, varid, varname))
				if (string("Tcline") != varname) continue; //This is the only double or float scalar we read
				if (vartype == NC_FLOAT){
					NC_ERR_READ(nc_get_var_float(ncid, varid, &Tcline))
				} else {
					double dTcline;
					NC_ERR_READ(nc_get_var_double(ncid, varid, &dTcline))
					Tcline = (float) dTcline;
				}
				continue;
			}
			if (ndims == 1){
				
				NC_ERR_READ(nc_inq_varname(ncid, varid, varname))
				if (string("Cs_r") == varname) {
					if (vartype == NC_FLOAT){
						NC_ERR_READ(nc_get_var_float(ncid, varid, Cs_r))
					} else {
						double* dvals = new double[dimsVDC[2]];
						NC_ERR_READ(nc_get_var_double(ncid, varid, dvals))
						for(int j=0; j< dimsVDC[2]; j++) Cs_r[j] = (float)dvals[j];
						delete dvals;
					}
				} else if (atypnames["s_rho"] == varname){
					if (vartype == NC_FLOAT){
						NC_ERR_READ(nc_get_var_float(ncid, varid, s_rho))
					} else {
						double* dvals = new double[dimsVDC[2]];
						NC_ERR_READ(nc_get_var_double(ncid, varid, dvals))
						for(int j=0; j< dimsVDC[2]; j++) s_rho[j] = (float)dvals[j];
						delete dvals;
					}
				}
				continue;
			}
			//All other variables must be 2D + time or 3D + time
			if (ndims <3 || ndims >4) continue;
			
			NC_ERR_READ(nc_inq_varname(ncid, varid, varname))
			
			//Check that the name is in the VDC:
			bool varFound = false;
			if (ndims == 3){
				for (int n=0; n< vdcvars2d.size(); n++){
					if(vdcvars2d[n] == varname) {varFound = true; break;}
				}
			} else {
				for (int n = 0; n < vdcvars3d.size(); n++){
					if (vdcvars3d[n] == varname) {varFound = true; break;}
				}
			}
			if (!varFound) continue;
			//If varnames are specified, check that the variable is in the list
			if (opt.varnames.size()>0){
				bool found = false;
				for (vector<string>::iterator itr = opt.varnames.begin(); itr!=opt.varnames.end(); itr++) {
					if (*itr == varname) {
						found = true;
						break;
					}
				}
				if (!found) continue;
			}
			
			// Determine geolon/geolat vars
			int geolon, geolat;
			if (roms->GetGeoLonLatVar(ncid, varid,&geolon, &geolat)) continue;
			assert(geolon == geolat);
			WeightTable* wt = roms->GetWeightTable(geolon);
			//Determine the x,y dimensions of the source variable.  Start with psi grid, possibly add 1
			int ndim[2];
			ndim[0] = dimsVDC[0];
			ndim[1] = dimsVDC[1];
			if (geolat == 1 || geolat == 3) ndim[1]++; //u grid or rho grid
			if (geolat == 2 || geolat == 3) ndim[0]++; //v grid or rho grid
			bool stag = false;
			if (ndims == 4){ //see if it's staggered in z:
				int dimid[4]; 
				size_t dimsize;
				NC_ERR_READ(nc_inq_vardimid(ncid, varid, dimid));
				NC_ERR_READ(nc_inq_dimlen(ncid, dimid[1], &dimsize));
				if (dimsize == dimsVDC[2]+1) stag = true;
				else if (dimsize != dimsVDC[2]) assert(0);
			}
			//loop thru the times in the file.
			for (int ts = 0; ts < timelen; ts++){
				//for each time convert the variable
				if (ndims == 4) {
					CopyVariable3D(ncid,varid,wt,vdfio3d,opt.level,opt.lod, varname, dimsVDC, ndim,VDCTimes[ts],ts,&sample3DVar,stag );
				}
				else CopyVariable2D(ncid,varid,wt,vdfio2d,wbwriter2d,opt.level,opt.lod,varname, dimsVDC, ndim, VDCTimes[ts],ts, &sample2DVar);
			}
			printf("Converted variable: %s\n", varname);
		} //End loop over variables in file	

		//Create 2D variables, using mask from first 2D variable:
		// Insert Depth, latDeg, and 
		//Create DEPTH variable
		//
		if (!mappedDepth)
			mappedDepth = calcConst2DVar(roms->GetDepths(),dimsVDC, roms, sample2DVar);
		
		if (mappedDepth){
			for( int t = 0; t< VDCTimes.size(); t++){
				int rc = CopyConstantVariable2D(mappedDepth,vdfio2d,wbwriter2d,opt.level,opt.lod, "DEPTH",dimsVDC,VDCTimes[t]);
				if (rc) exit(rc);
			}
			printf("Converted variable: DEPTH\n");
		}

		//Add angle variable (radians)
		if (!mappedAngles) mappedAngles = calcConst2DVar(roms->GetAngles(),dimsVDC, roms, sample2DVar);
		if (mappedAngles){
			for( int t = 0; t< VDCTimes.size(); t++){
				int rc = CopyConstantVariable2D(mappedAngles,vdfio2d,wbwriter2d,opt.level,opt.lod, "angleRAD",dimsVDC,VDCTimes[t]);
				if (rc) exit(rc);
			}
			printf("Converted variable: angleRAD\n");
		}

		//Add latitude variable (degrees)
		
		if (!mappedLats) mappedLats = calcConst2DVar(roms->GetLats(),dimsVDC, roms, sample2DVar);
		if (mappedLats){
			for( int t = 0; t< VDCTimes.size(); t++){
				int rc = CopyConstantVariable2D(mappedLats,vdfio2d,wbwriter2d,opt.level,opt.lod, "latDEG",dimsVDC,VDCTimes[t]);
				if (rc) exit(rc);
			}
			printf("Converted variable: latDEG\n");
		}
	
		//Insert elevation at every timestep 
		if (!elevation) elevation = CalcElevation(Vtransform, s_rho, Cs_r, Tcline, mappedDepth, dimsVDC, sample3DVar);
		if (elevation){
			for (int j = 0; j< VDCTimes.size(); j++){
				int rc = CopyConstantVariable3D(elevation, vdfio3d, opt.level, opt.lod, "ELEVATION", dimsVDC, VDCTimes[j]);
				if (rc) exit(rc);
			}
			printf("Converted variable: ELEVATION\n");
		}
		
	
	} //End input files
	
	
	exit(estatus);
}

