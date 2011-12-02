//
//***********************************************************************
//                                                                       *
//                            Copyright (C)  2005                        *
//            University Corporation for Atmospheric Research            *
//                            All Rights Reserved                        *
//                                                                       *
//***********************************************************************/
//
//      File:		mom2vdf.cpp
//
//      Author:         Alan Norton
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           November 2011
//
//      Description:	Read and convert a collection of MOM output files
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
#include <vapor/MetadataMOM.h>
#include <vapor/MOM.h>
#include <vapor/WaveletBlock3DBufWriter.h>
#include <vapor/WaveletBlock3DRegionWriter.h>
#include <vapor/WaveCodecIO.h>
#ifdef WIN32
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
	{"varnames",1, 	"",	"Colon delimited list of variable names in MOM files to convert. The default is to convert variables in the set intersection of those variables found in the VDF file and those in the MOM files."},
	{"missval", 1,  "", "Colon delimited list of variable names and associated values to remap the missing values.  Each variable name is followed by the associated value"},
	{"numts",	1,	"-1","Maximum number of time steps that may be converted. A -1 implies the conversion of all time steps found"},
	{"level",	1, 	"-1","Refinement levels saved. 0=>coarsest, 1=>next refinement, etc. -1=>finest"},
	{"lod",	1, 	"-1",	"Compression levels saved. 0 => coarsest, 1 => "
		"next refinement, etc. -1 => all levels defined by the .vdf file"},
	{"nthreads",1, 	"0",	"Number of execution threads (0 => # processors)"},
	{"tolerance",	1, 	"0.0000001","Tolerance for comparing relative user times"},
	{"noelev",	0,	"",	"Do not generate the ELEVATION variable required by vaporgui."},
	{"help",	0,	"",	"Print this message and exit."},
	{"debug",	0,	"",	"Enable debugging."},
	{"quiet",	0,	"",	"Operate quietly (outputs only vertical extents that are lower than those in the VDF)."},
	{NULL}
};

OptionParser::Option_T	get_options[] = {
	{"varnames", VetsUtil::CvtToStrVec, &opt.varnames, sizeof(opt.varnames)},
	{"missval", VetsUtil::CvtToStrVec, &opt.missval, sizeof(opt.missval)},
	{"numts", VetsUtil::CvtToInt, &opt.numts, sizeof(opt.numts)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"lod", VetsUtil::CvtToInt, &opt.lod, sizeof(opt.lod)},
	{"nthreads", VetsUtil::CvtToInt, &opt.nthreads, sizeof(opt.nthreads)},
	{"tolerance", VetsUtil::CvtToFloat, &opt.tolerance, sizeof(opt.tolerance)},
	{"noelev", VetsUtil::CvtToBoolean, &opt.noelev, sizeof(opt.noelev)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};

const char	*ProgName;

//Method to be used when a constant float value is used for a 3D array (i.e. ELEVATION)
//The input array vertValues must have one value for each vertical layer
int CopyConstantVariable3D(
				 const float *vertValues,
				 VDFIOBase *vdfio,
				 int level,
				 int lod, 
				 bool vdc1,	
				 const char* varname,
				 const size_t dim[3],
				 size_t tsVDC) {
	
	static size_t sliceBufferSize = 0;
	static float *sliceBuffer = NULL;
	WaveletBlock3DBufWriter *wbwriter = NULL;
	WaveCodecIO *wcwriter = NULL;
	
	int rc;
	
	if (vdc1) {
		wbwriter = (WaveletBlock3DBufWriter *) vdfio;
	}
	else {
		wcwriter = (WaveCodecIO *) vdfio;
	}
	
	
	
	if (vdc1) {
		rc = wbwriter->OpenVariableWrite(tsVDC, varname, level);
	}
	else {
		rc = wcwriter->OpenVariableWrite(tsVDC, varname, level, lod);
	}
	
	if (rc<0) {
		MyBase::SetErrMsg(
						  "Failed to open for write variable %s at time step %d",
						  varname, tsVDC
						  );
		return (-1);
	}
	
	size_t slice_sz = dim[0] * dim[1];
	
	if (sliceBufferSize < slice_sz) {
		if (sliceBuffer ) delete [] sliceBuffer;
		sliceBuffer = new float[slice_sz];
		sliceBufferSize = slice_sz;
	}
	
	for (size_t z = 0; z<dim[2]; z++) {
		//fill the buffer with the appropriate value:
		for (size_t i = 0; i< slice_sz; i++){
			sliceBuffer[i] = vertValues[z];
		}
		if (vdc1) {
			if (wbwriter->WriteSlice(sliceBuffer) < 0) {
				MyBase::SetErrMsg(
								  "Failed to write MOM variable %s slice at time step %d (vdc1)",
								  varname, tsVDC
								  );
				return (-1);
			}
		} // If vdc1
		else {
			if (wcwriter->WriteSlice(sliceBuffer) < 0) {
				MyBase::SetErrMsg(
								  "Failed to write MOM variable %s slice at time step %d (vdc2)",
								  varname, tsVDC
								  );
				return (-1);
			}
		} // Else vdc1
	} // End of for z.
	
	if (vdc1)
		wbwriter->CloseVariable();
	else
		wcwriter->CloseVariable();
	
	return(0);
}  // End of CopyConstantVariable3D.
//
// Following method is used to create depth variable.  Data is provided in a 2D float array
int CopyConstantVariable2D(
				   const float* dataValues,
				   VDFIOBase *vdfio,
				   WaveletBlock3DRegionWriter *wb2dwriter,
				   int level,
				   int lod, 
				   bool vdc1,	
				   const char* varname,
				   const size_t dim[3],
				   size_t tsVDC) {
	
	
	WaveCodecIO *wc2dwriter = NULL;
	
	int rc;
	if (vdc1) {
		//		wb2dwriter = (WaveletBlock3DRegionWriter *) vdfio;
	}
	else {
		wc2dwriter = (WaveCodecIO *) vdfio;
	}
	
	
	if (vdc1) {
		rc = wb2dwriter->OpenVariableWrite(tsVDC, varname, level);
	}
	else {
		rc = wc2dwriter->OpenVariableWrite(tsVDC, varname, level, lod);
	}
	if (rc<0) {
		MyBase::SetErrMsg(
						  "Failed to copy MOM variable %s at time step %d",
						  varname, tsVDC
						  );
		return (-1);
	}
	
 		
	
	if (vdc1) {
		if (wb2dwriter->WriteRegion(dataValues) < 0) {
			MyBase::SetErrMsg(
							  "Failed to write MOM variable %s at time step %d",
							  varname, tsVDC
							  );
			return (-1);
		}
	}
	else {
		if (wc2dwriter->WriteSlice(dataValues) < 0) {
			MyBase::SetErrMsg(
							  "Failed to write MOM variable %s at time step %d",
							  varname, tsVDC
							  );
			return (-1);
		}
	}
	
	if (vdc1)
		wb2dwriter->CloseVariable();
	else
		wc2dwriter->CloseVariable();
	
	return(0);
} // End of CopyConstantVariable2D.


int CopyVariable3D(
	int ncid,
	int varid,
	WeightTable *wt,
	VDFIOBase *vdfio,
	int level,
	int lod, 
	bool vdc1,	
	string varname,
	float* missMap,
	const size_t dim[3],
	size_t tsVDC,
	int tsNetCDF
	
	) {

	static size_t sliceBufferSize = 0;
	static float *sliceBuffer = NULL;
	static float *sliceBuffer2 = NULL;
	WaveletBlock3DBufWriter *wbwriter = NULL;
	WaveCodecIO *wcwriter = NULL;

	int rc;
	
	if (vdc1) {
		wbwriter = (WaveletBlock3DBufWriter *) vdfio;
	}
	else {
		wcwriter = (WaveCodecIO *) vdfio;
	}
	
	if (vdc1) {
		rc = wbwriter->OpenVariableWrite(tsVDC, varname.c_str(), level);
	}
	else {
		rc = wcwriter->OpenVariableWrite(tsVDC, varname.c_str(), level, lod);
	}

	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to open for write MOM variable %s at time step %d",
			varname.c_str(), tsVDC
		);
		return (-1);
	}
	
	float mv = -1.e30f;
	rc = nc_get_att_float(ncid, varid, "missing_value", &mv);
	
	float missMapVal = mv;
	if (missMap) missMapVal = *missMap;
	
	size_t slice_sz = dim[0] * dim[1];
	size_t starts[4] = {0,0,0,0};
	size_t counts[4] = {1,1,1,1};
	starts[0]=tsNetCDF;
	counts[2]=dim[1];
	counts[3]=dim[0];

	if (sliceBufferSize < slice_sz) {
		if (sliceBuffer) {
			delete [] sliceBuffer;
			delete [] sliceBuffer2;
		}
		sliceBuffer = new float[slice_sz];
		sliceBuffer2 = new float[slice_sz];
		sliceBufferSize = slice_sz;
	}
	
	for (size_t z = 0; z<dim[2]; z++) {
		//
		//Read slice of NetCDF file into slice buffer
		//
		starts[1]= dim[2]-z-1;
		NC_ERR_READ(nc_get_vara_float(ncid, varid, starts, counts, sliceBuffer))
		
		//
		// Remap it 
		//
		
		wt->interp2D(sliceBuffer, sliceBuffer2, mv, missMapVal);
		
		
		if (vdc1) {
			if (wbwriter->WriteSlice(sliceBuffer2) < 0) {
				MyBase::SetErrMsg(
					"Failed to write MOM variable %s slice at time step %d (vdc1)",
					varname.c_str(), tsVDC
				);
				return (-1);
			}
		} // If vdc1
		else {
			if (wcwriter->WriteSlice(sliceBuffer2) < 0) {
				MyBase::SetErrMsg(
					"Failed to write MOM variable %s slice at time step %d (vdc2)",
					varname.c_str(), tsVDC
				);
				return (-1);
			}
		} // Else vdc1
	} // End of for z.

	if (vdc1)
		wbwriter->CloseVariable();
	else
		wcwriter->CloseVariable();

	return(0);
}  // End of CopyVariable3D.

int CopyVariable2D(
	int ncid,
	int varid,
	WeightTable* wt,
	VDFIOBase *vdfio,
	WaveletBlock3DRegionWriter *wb2dwriter,
	int level,
	int lod, 
	bool vdc1,	
	string varname,
	float* missMap,
	const size_t dim[3],
	size_t tsVDC,
	int tsNetCDF
	
) {

	static size_t sliceBufferSize = 0;
	static float *sliceBuffer = NULL;
	static float *sliceBuffer2 = NULL;

	WaveCodecIO *wc2dwriter = NULL;

	int rc;
	if (vdc1) {
//		wb2dwriter = (WaveletBlock3DRegionWriter *) vdfio;
	}
	else {
		wc2dwriter = (WaveCodecIO *) vdfio;
	}


	if (vdc1) {
		rc = wb2dwriter->OpenVariableWrite(tsVDC, varname.c_str(), level);
	}
	else {
		rc = wc2dwriter->OpenVariableWrite(tsVDC, varname.c_str(), level, lod);
	}
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to copy MOM variable %s at time step %d",
			varname.c_str(), tsVDC
		);
		return (-1);
	}
	
	float mv = -1.e30f;
	rc = nc_get_att_float(ncid, varid, "missing_value", &mv);
 
 	size_t slice_sz = dim[0] * dim[1];
	size_t starts[3] = {0,0,0};
	size_t counts[3];
	starts[0]=tsNetCDF;
	counts[0]=1;
	counts[1]=dim[1];
	counts[2]=dim[0];
	

	if (sliceBufferSize < slice_sz) {
		if (sliceBuffer) {
			delete [] sliceBuffer;
			delete [] sliceBuffer2;
		}
		sliceBuffer = new float[slice_sz];
		sliceBuffer2 = new float[slice_sz];
		sliceBufferSize = slice_sz;
	}

	//
	// Read the variable into the temp buffer
	//
	NC_ERR_READ(nc_get_vara_float(ncid, varid, starts, counts, sliceBuffer))
	
	// Determine if this missing value is to be remapped
	float missMapVal = mv;
	if(missMap) missMapVal = *missMap; 
	
	//
	// Remap it 
	//
	
	wt->interp2D(sliceBuffer, sliceBuffer2, mv, missMapVal);
				
	if (vdc1) {
		if (wb2dwriter->WriteRegion(sliceBuffer2) < 0) {
			MyBase::SetErrMsg(
				"Failed to write MOM variable %s at time step %d",
				varname.c_str(), tsVDC
			);
			return (-1);
		}
	}
	else {
		if (wc2dwriter->WriteSlice(sliceBuffer2) < 0) {
			MyBase::SetErrMsg(
				"Failed to write MOM variable %s at MOM time step %d",
				varname.c_str(), tsVDC
			);
			return (-1);
		}
	}

	if (vdc1)
		wb2dwriter->CloseVariable();
	else
		wc2dwriter->CloseVariable();

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
		cerr << "Usage: " << ProgName << " [options] MOM_netcdf_datafile(s)... MOM_topo_file vdf_file" << endl << endl;
		op.PrintOptionHelp(stderr);
		exit(0);
	}

	if (argc < 3) {
		cerr << "Usage: " << ProgName << " [options] MOM_netcdf_datafile(s)... MOM_topo_file vdf_file" << endl;
		op.PrintOptionHelp(stderr);
		exit(1);
	}

	if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	argv++;
	argc--;
	//
	// At this point there is at least one MOM file to work with.
	// The format is mom-files + topo-file + vdf-file.
	//
	
	vector<string> momfiles;
	for (int i=0; i<argc-2; i++) {
		momfiles.push_back(argv[i]);
	}
	string topofile = string(argv[argc-2]);
	string metafile = string(argv[argc-1]);
	
	vector<string> varnames = opt.varnames;
	
	vector<string> missMapNames;
	vector<float> missMapValues;
	if (2*(opt.missval.size()/2) != opt.missval.size()){
		cerr << "Error: missval option requires an even number of colon-separated entries";
		op.PrintOptionHelp(stderr);
		exit(1);
	}
		
	for (int i = 0; i<opt.missval.size(); i+=2){
		missMapNames.push_back(opt.missval[i]);
		missMapValues.push_back(strtod(opt.missval[i+1].c_str(),0));
	}

	WaveletBlock3DBufWriter *wbwriter3d = NULL;
	WaveCodecIO	*wcwriter3d = NULL;
	VDFIOBase *vdfio = NULL;
	
	MetadataVDC	*metadataVDC = new MetadataVDC(metafile);
	if (MetadataVDC::GetErrCode() != 0) { 
		MyBase::SetErrMsg("Error processing VDC metafile : %s",metafile.c_str());
		exit(1);
	}

	bool vdc1 = (metadataVDC->GetVDCType() == 1);

	if(vdc1) {
		wbwriter3d = new WaveletBlock3DBufWriter(*metadataVDC);
		vdfio = wbwriter3d;
	}
	else {
		wcwriter3d = new WaveCodecIO(*metadataVDC, opt.nthreads);
		vdfio = wcwriter3d;
	}
	
	if (WaveletBlock3DBufWriter::GetErrCode() != 0) { 
		MyBase::SetErrMsg("Error processing VDC metafile : %s",metafile.c_str());
		exit(1);
	}

	//
	// Ugh! WaveletBlock3DBufWriter class does not handle 2D data!!!  
	// Also does want to work form VDFIOBase.
	//
	WaveletBlock3DRegionWriter *wb2dwriter = new WaveletBlock3DRegionWriter(*metadataVDC);
	if (WaveletBlock3DRegionWriter::GetErrCode() != 0) { 
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
		if (svec.size() != 6) {
			MyBase::SetErrMsg("Invalid DependentVarNames tag: %s",tag.c_str());
			return(-1);
		}
		atypnames["time"] = svec[0];
		atypnames["ht"] = svec[1];
		atypnames["st_ocean"] = svec[2];
		atypnames["st_edges_ocean"] = svec[3];
		atypnames["sw_ocean"] = svec[4];
		atypnames["sw_edges_ocean"] = svec[5];
		
	}
	//
	// Create a MOM instance to get info from the topofile
	//
	
	// The MOM constructor will check the topofile,
	// initialize the variable names
	MOM* mom = new MOM(topofile, atypnames, varnames, varnames);
	if (MOM::GetErrCode() != 0) {
		MOM::SetErrMsg(
			"Error processing topo file %s, exiting.",topofile.c_str()
					   );
        exit(1);
    }
	//
	// Verify that the MOM matches the VDF file
	//
	
	//
	// Ask the MOM to create all weighttables
	//
	// There needs to be a weightTable for each combination of geolat/geolon variables
	int rc = mom->MakeWeightTables();
	if (rc) exit (rc);
		
	//
	//Create ELEVATION and DEPTH variables
	//
	size_t numTimeSteps = metadataVDC->GetNumTimeSteps();
	if (!opt.noelev){
		//Create an ELEVATION array, 
		//Add it to all the time-steps in the VDC
		const float* elevData = mom->GetElevations();
		
		for( size_t t = 0; t< numTimeSteps; t++){
			int rc = CopyConstantVariable3D(elevData,vdfio,opt.level,opt.lod, vdc1,"ELEVATION",dimsVDC,t);
			if (rc) exit(rc);
		}
	}
	//Add depth variable
	float* depth = mom->GetDepths();
	
	if (depth){
		// use t-grid for remapping depth
		WeightTable *wt = mom->GetWeightTable(0,0);
		float* mappedDepth = new float[dimsVDC[0]*dimsVDC[1]];
		wt->interp2D(depth,mappedDepth, -1.e30, -1.e30);
		float minval = 1.e30;
		float maxval = -1.e30;
		float minval1 = 1.e30;
		float maxval1 = -1.e30;
		for (int i = 0; i<dimsVDC[0]*dimsVDC[1]; i++){
			if(depth[i]<minval) minval = depth[i];
			if(depth[i]>maxval) maxval = depth[i];
			if(mappedDepth[i]<minval1) minval1 = mappedDepth[i];
			if(mappedDepth[i]>maxval1) maxval1 = mappedDepth[i];
		}

		for( size_t t = 0; t< numTimeSteps; t++){
			int rc = CopyConstantVariable2D(mappedDepth,vdfio,wb2dwriter,opt.level,opt.lod, vdc1,"DEPTH",dimsVDC,t);
			if (rc) exit(rc);
		}
	}
	delete depth;
	vector<string> vdcvars2d = metadataVDC->GetVariables2DXY();
	vector<string> vdcvars3d = metadataVDC->GetVariables3D();
	vector<size_t>VDCTimes;	
	
	//Create an array of times in the VDC:
	vector<double> usertimes;
	for (size_t ts = 0; ts < metadataVDC->GetNumTimeSteps(); ts++){
		usertimes.push_back(metadataVDC->GetTSUserTime(ts));
	}
	//Loop thru momfiles
	for (int i = 0; i<momfiles.size(); i++){
		printf("processing file %s\n",momfiles[i].c_str());	
		
		//For each momfile:
		//Open the file
		int ncid;
		char nctimename[NC_MAX_NAME+1];
		int timedimid;
		size_t timelen;
		NC_ERR_READ( nc_open( momfiles[i].c_str(), NC_NOWRITE, &ncid ));
		
		// Read the times from the file
		int timevarid;
		int rc = nc_inq_varid(ncid, atypnames["time"].c_str(), &timevarid);
		if (rc){
			MyBase::SetErrMsg("Time variable (named: %s) not in file %s, skipping",
				atypnames["time"].c_str(),momfiles[i].c_str());
			MyBase::SetErrCode(0);
			continue;
		}
		
		mom->extractStartTime(ncid,timevarid);
		
		NC_ERR_READ(nc_inq_unlimdim(ncid, &timedimid));
		
		NC_ERR_READ(nc_inq_dim(ncid, timedimid, nctimename, &timelen));

		size_t timestart[1] = {0};
		size_t timecount[1];
		timecount[0] = timelen;
		double* fileTimes = new double[timelen];
		VDCTimes.clear();
		NC_ERR_READ(nc_get_vara_double(ncid, timevarid, timestart, timecount, fileTimes));
		
		// map the times to VDC timesteps, using MetadataMOM::GetVDCTimes()
		for (int j = 0; j< timelen; j++){
			size_t ts = mom->GetVDCTimeStep(fileTimes[j],usertimes);
			if (ts == (size_t)-1) {
				MyBase::SetErrMsg(" Time step %d in file %s does not correspond to a valid time in the VDC",
								  j, momfiles[i].c_str());
				exit (-2);
			}
			VDCTimes.push_back(ts);
		}
		//Loop thru its variables (2d & 3d):
		int nvars;
	
		NC_ERR_READ( nc_inq_nvars(ncid, &nvars ) )
		for (int varid = 0; varid<nvars; varid++){
			
			nc_type vartype;
			NC_ERR_READ( nc_inq_vartype(ncid, varid,  &vartype) );
			if (vartype != NC_FLOAT && vartype != NC_DOUBLE) continue;
			int ndims;
			NC_ERR_READ(nc_inq_varndims(ncid, varid, &ndims));
			if (ndims <3 || ndims >4) continue;
			//Get the variable name
			char varname[NC_MAX_NAME+1];
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
			//Determine if missing value is remapped:
			float * missValPtr = 0;
			float missVal;
			for (int j=0; j<missMapNames.size(); j++){
				if (missMapNames[j] == varname){
					missVal = missMapValues[j];
					missValPtr = &missVal;
					break;
				}
			}
			// Determine geolon/geolat vars
			int geolon, geolat;
			if (mom->GetGeoLonLatVar(ncid, varid,&geolon, &geolat)) continue;
			WeightTable* wt = mom->GetWeightTable(geolon, geolat);
			//loop thru the times in the file.
			for (int j = 0; j < timelen; j++){
				//for each time convert the variable
				if (ndims == 4)CopyVariable3D(ncid,varid,wt,vdfio,opt.level,opt.lod, vdc1, varname, missValPtr, dimsVDC,VDCTimes[j],j);
				else CopyVariable2D(ncid,varid,wt,vdfio,wb2dwriter,opt.level,opt.lod, vdc1, varname, missValPtr, dimsVDC,VDCTimes[j],j);
			}
		} //End loop over variables in file
			
					
				
	}
		
			
		exit(estatus);
}
