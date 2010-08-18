
#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sstream>
#include <algorithm>
#include <netcdf.h>
#include "proj_api.h"
#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/MetadataWRF.h>
#include <vapor/MetadataSpherical.h>
#include <vapor/WRF.h>
#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace VetsUtil;
using namespace VAPoR;

int	cvtToExtents(
	const char *from, void *to
) {
	float   *fptr   = (float *) to;

	if (! from) {
		fptr[0] = fptr[1] = fptr[2] = fptr[3] = fptr[4] = fptr[5] = 0.0;
	}
	else if (! 
		(sscanf(from,"%f:%f:%f:%f:%f:%f",
		&fptr[0],&fptr[1],&fptr[2],&fptr[3],&fptr[4],&fptr[5]) == 6)) { 

		return(-1);
	}
	return(1);
}

struct opt_t {
	OptionParser::Dimension3D_T	dim;
	OptionParser::Dimension3D_T	bs;
	int	numts;
	int deltat;
	char * startt;
	int	level;
	int	nfilter;
	int	nlifting;
	char *comment;
	char* mapprojection;
	float extents[6];
	vector <string> vars3d;
	vector <string> vars2d;
	vector<string> dervars;
	vector<string> atypvars;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"dimension",1, "512x512x512",	"Volume dimensions (unstaggered) expressed in grid points (NXxNYxNZ) if no sample WRF file is given"},
	{"extents",	1,	"0:0:0:0:0:0",	"Colon delimited 6-element vector specifying domain extents in user coordinates (X0:Y0:Z0:X1:Y1:Z1) if different from that in sample WRF"},
	{"startt",	1,	"", "Starting time stamp, one of (time|SIMULATION_START_DATE|START_DATE), where time has the form : yyyy-mm-dd_hh:mm:ss"},
	{"numts",	1, 	"0",			"Maximum number of VDC time steps"},
	{"deltat",	1,	"0",			"Seconds per VDC time step"},
	{"varnames",1,	"",			"Deprecated. Use -vars3d instead"},
	{"vars3d",1,	"",			"Colon delimited list of 3D variables to be extracted from WRF data"},
	{"vars2d",1,    "",         "Colon delimited list of 2D variables to be extracted from WRF data"},
	{"dervars", 1,	"",	"Colon delimited list of desired derived variables.  Choices are: PHNorm_: normalized geopotential (PH+PHB)/PHB, UVW_: 3D wind speed (U^2+V^2+W^2)^1/2, UV_: 2D wind speed (U^2+V^2)^1/2, omZ_: estimate of vertical vorticity, PFull_: full pressure P+PB, PNorm_: normalized pressure (P+PB)/PB, Theta_: potential temperature T+300, TK_: temp. in Kelvin (T+300)((P+PB))/100000)^0.286"},
	{"level",	1, 	"2",			"Maximum refinement level. 0 => no refinement (default is 2)"},
	{"atypvars",1,	"U:V:W:PH:PHB:P:PB:T",		"Colon delimited list of atypical names for U:V:W:PH:PHB:P:PB:T that appear in WRF file"},
	{"comment",	1,	"",				"Top-level comment (optional)"},
	{"mapprojection",	1,	"",	"A whitespace delineated list of "
		"PROJ4 +paramname=paramvalue pairs. If this is invalid, then time-varying extents "
		"will not be provided in the metadata"},
	{"bs",		1, 	"32x32x32",		"Internal storage blocking factor expressed in grid points (NXxNYxNZ) (default is 32)"},
	{"nfilter",	1, 	"1",			"Number of wavelet filter coefficients (default is 1)"},
	{"nlifting",1, 	"1",			"Number of wavelet lifting coefficients (default is 1)"},
	{"help",	0,	"",				"Print this message and exit"},
	{"quiet",	0,	"",				"Operate quietly"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"dimension", VetsUtil::CvtToDimension3D, &opt.dim, sizeof(opt.dim)},
	{"extents", cvtToExtents, &opt.extents, sizeof(opt.extents)},
	{"startt", VetsUtil::CvtToString, &opt.startt, sizeof(opt.startt)},
	{"numts", VetsUtil::CvtToInt, &opt.numts, sizeof(opt.numts)},
	{"deltat", VetsUtil::CvtToInt, &opt.deltat, sizeof(opt.deltat)},
	{"vars3d", VetsUtil::CvtToStrVec, &opt.vars3d, sizeof(opt.vars3d)},
	{"vars2d", VetsUtil::CvtToStrVec, &opt.vars2d, sizeof(opt.vars2d)},
	{"dervars", VetsUtil::CvtToStrVec, &opt.dervars, sizeof(opt.dervars)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"atypvars", VetsUtil::CvtToStrVec, &opt.atypvars, sizeof(opt.atypvars)},
	{"comment", VetsUtil::CvtToString, &opt.comment, sizeof(opt.comment)},
	{"mapprojection", VetsUtil::CvtToString, &opt.mapprojection, sizeof(opt.mapprojection)},
	{"bs", VetsUtil::CvtToDimension3D, &opt.bs, sizeof(opt.bs)},
	{"nfilter", VetsUtil::CvtToInt, &opt.nfilter, sizeof(opt.nfilter)},
	{"nlifting", VetsUtil::CvtToInt, &opt.nlifting, sizeof(opt.nlifting)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};

//Define ordering of pairs <timestep, extents> so they can be sorted together
bool timeOrdering(pair<TIME64_T, float* > p,pair<TIME64_T, float* > q){
	if (p.first < q.first) return true;
	return false;
}

const char *ProgName;

void Usage(OptionParser &op, const char * msg) {

	if (msg) {
		cerr << ProgName << " : " << msg << endl;
	}
	cerr << "Usage: " << ProgName << " [options] wrf_ncdf_file... vdf_file" << endl;
	cerr << "Usage: " << ProgName << " [options] -startt time vdf_file" << endl;
	op.PrintOptionHelp(stderr);

}

void ErrMsgCBHandler(const char *msg, int) {
	cerr << ProgName << " : " << msg << endl;
}

int	main(int argc, char **argv) {

	OptionParser op;


	size_t dim[3];
	float extents[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	string	s;
	MetadataVDC *file;
	MetadataWRF *WRFData;

	ProgName = Basename(argv[0]);

	if (op.AppendOptions(set_opts) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		cerr << ProgName << " : " << OptionParser::GetErrMsg();
		exit(1);
	}

	MyBase::SetErrMsgCB(ErrMsgCBHandler);


	if (opt.help) {
		Usage(op, NULL);
		exit(0);
	}

	WRF::atypVarNames_t wrfNames;

	// Handle atypical variable naming
	if ( opt.atypvars.size() != 8 ) {
		cerr << "If -atypvars option is given, colon delimited list must have exactly eight elements specifying names of variables which are typically named U:V:W:PH:PHB:P:PB:T" << endl;
		exit( 1 );
	}

	wrfNames.U = opt.atypvars[0];
	wrfNames.V = opt.atypvars[1];
	wrfNames.W = opt.atypvars[2];
	wrfNames.PH = opt.atypvars[3];
	wrfNames.PHB = opt.atypvars[4];
	wrfNames.P = opt.atypvars[5];
	wrfNames.PB = opt.atypvars[6];
	wrfNames.T = opt.atypvars[7];
	

	argv++;
	argc--;
	vector <pair<TIME64_T,float*> > tStepExtents; //pairs of timestamps, extents
	vector <string> wrfVarNames3d, wrfVarNames2d;    // 2d and 3d vars
	vector <string> vdfVarNames3d;
	vector <string> vdfVarNames2d;
	string startT;
	string mapProj;
	bool userSpecifiedExtents = false;  //set true if user specifies on command line
	if (argc == 1) {	// No template file
	
		if (strlen(opt.startt) == 0) {
			Usage(op, "Expected -startt option");
			exit(1);
		}
		if (opt.numts > 1 && opt.deltat < 1) {
			Usage(op, "Invalid -deltat argument value : must be >= 1");
			exit(1);
		}

		// Set the first time stamp
		//
		startT.assign(opt.startt);
		TIME64_T seconds;
		if (WRF::WRFTimeStrToEpoch(opt.startt, &seconds) < 0) exit(1);
		pair<TIME64_T,float*>pr;
		pr = make_pair(seconds, (float*)0);
		tStepExtents.push_back(pr);


		dim[0] = opt.dim.nx;
		dim[1] = opt.dim.ny;
		dim[2] = opt.dim.nz;

		vdfVarNames3d = opt.vars3d;
		vdfVarNames2d = opt.vars2d;

		
		for(int i=0; i<5; i++) {
			if (opt.extents[i] != opt.extents[i+1]) userSpecifiedExtents = true;
		}
		if (userSpecifiedExtents) {
			for(int i=0; i<6; i++) {
				extents[i] = opt.extents[i];
			}
		}

	}
	else if (argc > 1) {
		//
		// At this point there is at last one WRF file to work with.
		// The format is wrf-file+ vdf-file.
		//

		vector<string> wrffiles;
		for (int i=0; i<argc-1; i++) {
			 wrffiles.push_back(argv[i]);
		}
		WRFData = new MetadataWRF(wrffiles);

		//
		// If the extents were provided on the command line, set the
		// extentsPtr to NULL so that we don't have to calulate the
		// extents by examining all the netCDF files
		//

		float *extentsPtr = extents;
		for(int i=0; i<5; i++) {
			if (opt.extents[i] != opt.extents[i+1]) {
				extentsPtr = NULL;
				userSpecifiedExtents = true;
			}
		}
		if (! extentsPtr) {
			for(int i=0; i<6; i++) {
				extents[i] = opt.extents[i];
			}
		}

//		string startDate;
		
		
	}

	if(WRFData->GetNumTimeSteps() > 0) {

		size_t bs[] = {opt.bs.nx, opt.bs.ny, opt.bs.nz};
		file = new MetadataVDC(WRFData->GetDimension(), opt.level, bs);
		if (MetadataVDC::GetErrCode() != 0) exit(1);

        // Copy values over from MetadataWRF to MetadataVDC.
        // Add checking of return values and error messsages.

		if(file->SetNumTimeSteps(WRFData->GetNumTimeSteps())) {
			cerr << "Error populating NumTimeSteps." << endl;
			exit(1);
		}
		if(file->SetMapProjection(WRFData->GetMapProjection())) {
			cerr << "Error populating MapProjection." << endl;
			exit(1);
		}
		if(file->SetVariables3D(WRFData->GetVariables3D())) {
			cerr << "Error populating Variables3D." << endl;
			exit(1);
		}
		if(file->SetVariables2DXY(WRFData->GetVariables2DXY())) {
			cerr << "Error populating Variables3D." << endl;
			exit(1);
		}
		if(file->SetVariables2DXZ(WRFData->GetVariables2DXZ())) {
			cerr << "Error populating Variables3D." << endl;
			exit(1);
		}
		if(file->SetVariables2DYZ(WRFData->GetVariables2DYZ())) {
			cerr << "Error populating Variables3D." << endl;
			exit(1);
		}
		if(file->SetExtents(WRFData->GetExtents())) {
			cerr << "Error populating Extents." << endl;
			exit(1);
		}
	        if(file->SetGridType(WRFData->GetGridType())) {
			cerr << "Error populating GridType." << endl;
			exit(1);
		}
        	if(file->SetUserDataString("DependentVarNames", "U:V:W:PH:PHB:P:PB:T")) {
			cerr << "Error populating Dependent Vars." << endl;
			exit(1);
		}
		string usertimestamp;
		vector <double> usertime;
		for(int i = 0; i < WRFData->GetNumTimeSteps(); i++) {
			WRFData->GetTSUserTimeStamp(i, usertimestamp);
			if(file->SetTSUserTimeStamp(i, usertimestamp)) {
				cerr << "Error populating TSUserTimeStamp." << endl;
				exit(1);
			}
			usertime.clear();
			usertime.push_back(WRFData->GetTSUserTime(i));
			if(file->SetTSUserTime(i, usertime)) {
				cerr << "Error populating TSUserTime." << endl;
				exit(1);
			}
			if(file->SetTSExtents(i, WRFData->GetTSExtents(i))) {
				cerr << "Error populating TSExtents." << endl;
				exit(1);
			}
		}
	
        // Handle command line over rides here.

		if (file->Write(argv[argc-1]) < 0) {
			exit(1);
		}
	} // End if Number of timesteps.
	else {
		cerr << "No output file generated due to no input files processed." << endl;
	}

	if (! opt.quiet && WRFData->GetNumTimeSteps() > 0) {
		cout << "Created VDF file:" << endl;
		cout << "\tNum time steps : " << WRFData->GetNumTimeSteps() << endl;
		cout << "\t3D Variable names : ";
		for (int i = 0; i < WRFData->GetVariables3D().size(); i++) {
			cout << WRFData->GetVariables3D()[i] << " ";
		}
		cout << endl;
		cout << "\t2D Variable names : ";
		for (int i=0; i < WRFData->GetVariables2DXY().size(); i++) {
			cout << WRFData->GetVariables2DXY()[i] << " ";
		}
		cout << endl;

		cout << "\tCoordinate extents : ";
		const vector <double> extptr = WRFData->GetExtents();
		for(int i=0; i<6; i++) {
			cout << extptr[i] << " ";
		}
		cout << endl;
		if (WRFData->GetminLon() < 1000.f){
			cout << "\tMin Longitude and Latitude of domain corners: " << WRFData->GetminLon() << " " << WRFData->GetminLat() << endl;
			cout << "\tMax Longitude and Latitude of domain corners: " << WRFData->GetmaxLon() << " " << WRFData->GetmaxLat() << endl;
		}
		
	} // End if quiet.

	exit(0);
} // End of main.


