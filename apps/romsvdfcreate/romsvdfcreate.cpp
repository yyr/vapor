/*#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <netcdf.h>
#include "proj_api.h"
#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/WaveCodecIO.h>
#include <vapor/NetCDFCollection.h>
#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace VetsUtil;
using namespace VAPoR;

string StaggeredDimsTag = "ROMS_StaggeredDims";
string TimeDimsTag = "ROMS_TimeDimNames";
string TimeCoordTag = "ROMS_TimeCoordVariable";
string MissingValTag = "ROMS_MissingValTag";

const float VDCMissingValue(1.e38);

struct opt_t {
	vector <string> stagdims;
	vector <string> timedims;
	char *timecoord;
	char *missing;
	OptionParser::Dimension3D_T	bs;
	int	level;
	int	nfilter;
	int	nlifting;
	char *comment;
	OptionParser::Boolean_T	vdc2;	
	vector <int> cratios;
	char *wname;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"stagdims", 1,	"", "Colon-delimited list of staggered dimension names"},
	{"timedims", 1,	"", "Colon-delimited list of time dimension names"},
	{"timecoord", 1,	"", "Name of time coordinate variable, if present"},
	{"missing", 1,	"", "Name of ROMS variable attribute specifying missing data, if present"},
	{"comment",	1,	"",	"Name of time coordinate variable (optional)"},
	{"bs",		1, 	"-1x-1x-1",	"Internal storage blocking factor "
		"expressed in grid points (NXxNYxNZ). Defaults: 32x32x32 (VDC type 1), "
		"64x64x64 (VDC type 2"},
	{"level",	1, 	"2", "Maximum refinement level. 0 => no refinement (default is 2)"},
	{"nfilter",	1, 	"1", "Number of wavelet filter coefficients (default is 1)"},
	{"nlifting",1, 	"1", "Number of wavelet lifting coefficients (default is 1)"},
	{"comment",	1,	"",	"Top-level comment (optional)"},
	{"vdc2",	0,	"",	"Generate a VDC Type 2 .vdf file (default is VDC Type 1)"},
	{"cratios",1,	"",	"Colon delimited list compression ratios. "
		"The default is 1:10:100:500. " 
		"The maximum compression ratio is wavelet and block size dependent."},
	{"wname",	1,	"bior3.3",	"Wavelet family used for compression "
		"(VDC type 2, only). Recommended values are bior1.1, bior1.3, "
		"bior1.5, bior2.2, bior2.4 ,bior2.6, bior2.8, bior3.1, bior3.3, "
		"bior3.5, bior3.7, bior3.9, bior4.4"},
	{"help",	0,	"",	"Print this message and exit"},
	{"quiet",	0,	"",	"Operate quietly"},
	{NULL}
};

OptionParser::Option_T	get_options[] = {
	{"stagdims", VetsUtil::CvtToStrVec, &opt.stagdims, sizeof(opt.stagdims)},
	{"timedims", VetsUtil::CvtToStrVec, &opt.timedims, sizeof(opt.timedims)},
	{"timecoord", VetsUtil::CvtToString, &opt.timecoord, sizeof(opt.timecoord)},
	{"missing", VetsUtil::CvtToString, &opt.missing, sizeof(opt.missing)},
	{"bs", VetsUtil::CvtToDimension3D, &opt.bs, sizeof(opt.bs)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"comment", VetsUtil::CvtToString, &opt.comment, sizeof(opt.comment)},
	{"nfilter", VetsUtil::CvtToInt, &opt.nfilter, sizeof(opt.nfilter)},
	{"nlifting", VetsUtil::CvtToInt, &opt.nlifting, sizeof(opt.nlifting)},
	{"vdc2", VetsUtil::CvtToBoolean, &opt.vdc2, sizeof(opt.vdc2)},
	{"cratios", VetsUtil::CvtToIntVec, &opt.cratios, sizeof(opt.cratios)},
	{"wname", VetsUtil::CvtToString, &opt.wname, sizeof(opt.wname)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};

const char *ProgName;

void Usage(OptionParser &op, const char * msg) {

	if (msg) {
		cerr << ProgName << " : " << msg << endl;
	}
	cerr << "Usage: " << ProgName << " [options] romsfile [romsfiles...] vdf_file" << endl;
	op.PrintOptionHelp(stderr);

}

//
// Extracts the number of time steps, dimensionality, and 3D and 2D
// variable names from a NetCDFCollection instance.
//
// N.B. assumes the first 3D variable found in "nc" defines 
// the 3D dimensions for the entire data collection. Any other 3D 
// variables whose dimensions to not match the dimensionality of the
// first 3D variable are discarded. Similarly, any 2D variables whose
// dimensions do not mention the two fastest 3D dimensions (NX and NY)
// are discarded.
//
void GetInfo(
	const NetCDFCollection &nc, size_t dims[], 
	vector <string> &vars3d, vector <string> &vars2dxy, vector <double> &times
) {

	//
	// Get 3D variables
	//
	vars3d.clear();
	vars3d = nc.GetVariableNames(3);
	vector <size_t> dims3d;
	if (vars3d.size()) {
		dims3d = nc.GetDims(vars3d[0]);
		vector <string>::iterator itr;
		for (itr = vars3d.begin(); itr != vars3d.end(); ++itr) {
			vector <size_t> mydims = nc.GetDims(*itr);
			if (mydims != dims3d) {
				MyBase::SetErrMsg(
					"Skipping variable \"%s\", dimension mismatch",
					(*itr).c_str()
				);
				MyBase::SetErrCode(0);
				vars3d.erase(itr);
				itr = vars3d.begin();
			}
		}
	}

	//
	// Get 2D variables
	//
	vars2dxy.clear();
	vars2dxy = nc.GetVariableNames(2);
	vector <size_t> dims2d;
	if (vars2dxy.size()) { 
		dims2d = nc.GetDims(vars2dxy[0]);
		vector <string>::iterator itr;
		for (itr = vars2dxy.begin(); itr != vars2dxy.end(); ++itr) {
			vector <size_t> mydims = nc.GetDims(*itr);
			if (mydims != dims2d) {
				MyBase::SetErrMsg(
					"Skipping variable \"%s\", dimension mismatch",
					(*itr).c_str()
				);
				MyBase::SetErrCode(0);
				vars2dxy.erase(itr);
				itr = vars2dxy.begin();
			}
		}
	}

	//
	// Get grid dimensions
	//
	if (! dims3d.size()) {
		//
		// N.B. Dim order is swapped!!
		//
		dims[2] = 1;
		dims[1] = dims2d[0];
		dims[0] = dims2d[1];
	} 
	else {
		if (dims3d[1] != dims2d[0] || dims3d[2] != dims2d[1]) {
			MyBase::SetErrMsg("3D and 2D variable dimensions do not match");
			exit(1);
		}
		dims[2] = dims3d[0];
		dims[1] = dims3d[1];
		dims[0] = dims3d[2];
	}

	times = nc.GetTimes();
}


MetadataVDC *VDCCreate(const size_t dims[3]) {
	//
	// Handle options for VDC2 output.
	//
	vector <size_t> cratios;
	string wname;
	string wmode;
	size_t bs[] = {opt.bs.nx, opt.bs.ny, opt.bs.nz};
	
	if (opt.bs.nx < 0) {
		for (int i=0; i<3; i++) bs[i] = 32;
	}
	else {
		bs[0] = opt.bs.nx; bs[1] = opt.bs.ny; bs[2] = opt.bs.nz;
	}
	
	if (opt.vdc2) {
		wname = opt.wname;

		if ((wname.compare("bior1.1") == 0) ||
			(wname.compare("bior1.3") == 0) ||
			(wname.compare("bior1.5") == 0) ||
			(wname.compare("bior3.3") == 0) ||
			(wname.compare("bior3.5") == 0) ||
			(wname.compare("bior3.7") == 0) ||
			(wname.compare("bior3.9") == 0)) {

			wmode = "symh"; 
		}
		else if ((wname.compare("bior2.2") == 0) ||
			(wname.compare("bior2.4") == 0) ||
			(wname.compare("bior2.6") == 0) ||
			(wname.compare("bior2.8") == 0) ||
			(wname.compare("bior4.4") == 0)) {

			wmode = "symw"; 
		}
		else {
			wmode = "sp0"; 
		}

		if (opt.bs.nx < 0) {
			for (int i=0; i<3; i++) bs[i] = 64;
		}
		else {
			bs[0] = opt.bs.nx; bs[1] = opt.bs.ny; bs[2] = opt.bs.nz;
		}

		for (int i=0;i<opt.cratios.size();i++)cratios.push_back(opt.cratios[i]);

		if (cratios.size() == 0) {
			cratios.push_back(1);
			cratios.push_back(10);
			cratios.push_back(100);
			cratios.push_back(500);
		}

		size_t maxcratio = WaveCodecIO::GetMaxCRatio(bs, wname, wmode);
		for (int i=0;i<cratios.size();i++) {
			if (cratios[i] == 0 || cratios[i] > maxcratio) {
				MyBase::SetErrMsg(
					"Invalid compression ratio (%d) for configuration "
					"(block_size, wavename)", cratios[i]
				);
				exit(1);
			}
		}

	} // End if vdc2
	MetadataVDC *file;
	if (opt.vdc2) {
	 	file = new MetadataVDC(dims, bs, cratios, wname, wmode);
	}
	else {
		file = new MetadataVDC(dims, opt.level, bs, opt.nfilter, opt.nlifting);
	}
	return(file);
}

int	main(int argc, char **argv) {

	OptionParser op;
	string s;

	ProgName = Basename(argv[0]);
	MyBase::SetErrMsgFilePtr(stderr);


	if (op.AppendOptions(set_opts) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		cerr << ProgName << " : " << OptionParser::GetErrMsg();
		exit(1);
	}


	if (opt.help) {
		Usage(op, NULL);
		exit(0);
	}

	argv++;
	argc--;

	if (argc < 2) {
		Usage(op, "No files to process");
		exit(1);
	}


	if (! opt.vdc2) {
		MyBase::SetErrMsg("VDC Type 1 not supported");
		exit(1);
	}
	

	vector<string> romsfiles;
	for (int i=0; i<argc-1; i++) {
		 romsfiles.push_back(argv[i]);
	}

	NetCDFCollection nc;

	string time_coordvar = opt.timecoord;
	int rc = nc.Initialize(romsfiles, opt.timedims, time_coordvar);
	if (rc<0) {
		MyBase::SetErrMsg("Error processing ROMS files");
		exit(1);
	}

	//
	// Identify the staggered dimensions
	//
	if (opt.stagdims.size()) {
		nc.SetStaggeredDims(opt.stagdims);
	}

	size_t dims[3];
	vector <string> vars3d;
	vector <string> vars2dxy;
	vector <double> times;
	GetInfo(nc, dims, vars3d, vars2dxy, times);

	MetadataVDC *file = VDCCreate(dims);
	if (MetadataVDC::GetErrCode() != 0) exit(1);

	file->SetUserDataStringVec(StaggeredDimsTag, opt.stagdims);

	file->SetUserDataStringVec(TimeDimsTag, opt.timedims);

	{
		vector <string> vec;
		vec.push_back(time_coordvar);
		file->SetUserDataStringVec(TimeCoordTag, vec);
	}

	{
		vector <string> vec;
		vec.push_back(opt.missing);
		file->SetUserDataStringVec(MissingValTag, vec);
	}

	file->SetMissingValue(VDCMissingValue);

	if(file->SetNumTimeSteps(times.size())) {
		cerr << "Error populating NumTimeSteps." << endl;
		exit(1);
	}
//	if(file->SetMapProjection(WRFData->GetMapProjection())) {
//		cerr << "Error populating MapProjection." << endl;
//		exit(1);
//	}

	if(file->SetVariables3D(vars3d)) {
		cerr << "Error populating Variables3D." << endl;
		exit(1);
	}

	if(file->SetVariables2DXY(vars2dxy)) {
		cerr << "Error populating Variables2DXY." << endl;
		exit(1);
	}

//	if(file->SetExtents(WRFData->GetExtents())) {
//		cerr << "Error populating Extents." << endl;
//		exit(1);
//	}
//	if(file->SetGridType(WRFData->GetGridType())) {
//		cerr << "Error populating GridType." << endl;
//		exit(1);
//	}


	for (size_t ts = 0; ts<times.size(); ts++) {
		vector <double> usertime;
		usertime.push_back(times[ts]);
		if (file->SetTSUserTime(ts, usertime)) {
			cerr << "Error populating TSUserTime." << endl;
			exit(1);
		}
	}
#ifdef	DEAD
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
		if(file->SetTSExtents(i, WRFData->GetExtents(i))) {
			cerr << "Error populating TSExtents." << endl;
			exit(1);
		}
	}
#endif

	// Handle command line over rides here.

	s.assign(opt.comment);
	if(file->SetComment(s) < 0) {
		cerr << "Error populating Comment." << endl;
		exit(1);
	}

	// Write file.

	if (file->Write(argv[argc-1]) < 0) {
		exit(1);
	}

	if (! opt.quiet && times.size() > 0) {
		cout << "Created VDF file:" << endl;
		cout << "\tNum time steps : " << file->GetNumTimeSteps() << endl;
		cout << "\t3D Variable names : ";
		for (int i = 0; i < file->GetVariables3D().size(); i++) {
			cout << file->GetVariables3D()[i] << " ";
		}
		cout << endl;
		cout << "\t2D Variable names : ";
		for (int i=0; i < file->GetVariables2DXY().size(); i++) {
			cout << file->GetVariables2DXY()[i] << " ";
		}
		cout << endl;

		cout << "\tCoordinate extents : ";
		const vector <double> extptr = file->GetExtents();
		for(int i=0; i<6; i++) {
			cout << extptr[i] << " ";
		}
		cout << endl;
//		if (WRFData->GetminLon() < 1000.f){
//			cout << "\tMin Longitude and Latitude of domain corners: " << WRFData->GetminLon() << " " << WRFData->GetminLat() << endl;
//			cout << "\tMax Longitude and Latitude of domain corners: " << WRFData->GetmaxLon() << " " << WRFData->GetmaxLat() << endl;
//		}
		
	} // End if quiet.

	exit(0);
} // End of main.
*/
//
//      $Id$
//
#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <netcdf.h>
#include "proj_api.h"
#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/MetadataROMS.h>
#include <vapor/WaveCodecIO.h>
#include <vapor/ROMS.h>
#ifdef _WINDOWS 
#include "windows.h"
#pragma warning(disable : 4996)
#endif

using namespace VetsUtil;
using namespace VAPoR;
const double VDCMissingValue(1.e38);
struct opt_t {
	OptionParser::Dimension3D_T	bs;
	int	level;
	int	nfilter;
	int	nlifting;
	char *comment;
	vector <string> vars3d;
	vector <string> vars2d;
	vector<string> atypvars;
	OptionParser::Boolean_T	vdc1;	
	vector <int> cratios;
	char *wname;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"vars3d",1,	"",	"Colon delimited list of 3D variables to be extracted from ROMS data."
		"The default is to extract all 3D variables present."},
	{"vars2d",1,    "",	"Colon delimited list of 2D variables to be extracted from ROMS data."
		"The default is to extract all 2D variables present."},
	{"level",	1, 	"2", "Maximum refinement level. 0 => no refinement (default is 2)"},
	{"atypvars",1,	"ocean_time:h:xi_rho:xi_psi:xi_u:xi_v:eta_rho:eta_psi:eta_u:eta_v:s_rho:s_w", 
		"Colon delimited list of atypical names for 12 standard variables"
	},
	{"comment",	1,	"",	"Top-level comment (optional)"},
	{"bs",		1, 	"-1x-1x-1",	"Internal storage blocking factor "
		"expressed in grid points (NXxNYxNZ). Defaults: 32x32x32 (VDC type 1), "
		"64x64x64 (VDC type 2"},
	{"nfilter",	1, 	"1", "Number of wavelet filter coefficients (default is 1)"},
	{"nlifting",1, 	"1", "Number of wavelet lifting coefficients (default is 1)"},
	{"vdc1",	0,	"",	"Generate a VDC Type 1 .vdf file (default is VDC Type 2)"},
	{"cratios",1,	"",	"Colon delimited list compression ratios. "
		"The default is 1:10:100:500. " 
		"The maximum compression ratio is wavelet and block size dependent."},
	{"wname",	1,	"bior3.3",	"Wavelet family used for compression "
		"(VDC type 2, only). Recommended values are bior1.1, bior1.3, "
		"bior1.5, bior2.2, bior2.4 ,bior2.6, bior2.8, bior3.1, bior3.3, "
		"bior3.5, bior3.7, bior3.9, bior4.4"},
	{"help",	0,	"",	"Print this message and exit"},
	{"quiet",	0,	"",	"Operate quietly"},
	{NULL}
};

OptionParser::Option_T	get_options[] = {
	{"vars3d", VetsUtil::CvtToStrVec, &opt.vars3d, sizeof(opt.vars3d)},
	{"vars2d", VetsUtil::CvtToStrVec, &opt.vars2d, sizeof(opt.vars2d)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"atypvars", VetsUtil::CvtToStrVec, &opt.atypvars, sizeof(opt.atypvars)},
	{"comment", VetsUtil::CvtToString, &opt.comment, sizeof(opt.comment)},
	{"bs", VetsUtil::CvtToDimension3D, &opt.bs, sizeof(opt.bs)},
	{"nfilter", VetsUtil::CvtToInt, &opt.nfilter, sizeof(opt.nfilter)},
	{"nlifting", VetsUtil::CvtToInt, &opt.nlifting, sizeof(opt.nlifting)},
	{"vdc1", VetsUtil::CvtToBoolean, &opt.vdc1, sizeof(opt.vdc1)},
	{"cratios", VetsUtil::CvtToIntVec, &opt.cratios, sizeof(opt.cratios)},
	{"wname", VetsUtil::CvtToString, &opt.wname, sizeof(opt.wname)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};

const char *ProgName;

void Usage(OptionParser &op, const char * msg) {

	if (msg) {
		cerr << ProgName << " : " << msg << endl;
	}
	cerr << "Usage: " << ProgName << " [options] ROMS_netcdf_datafile(s) ... ROMS_grid_file vdf_file" << endl;
	op.PrintOptionHelp(stderr);

}

void ErrMsgCBHandler(const char *msg, int) {
	cerr << ProgName << " : " << msg << endl;
}

int	main(int argc, char **argv) {

	OptionParser op;
	string s;
	MetadataROMS *ROMSData;

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
	size_t bs[] = {opt.bs.nx, opt.bs.ny, opt.bs.nz};

	if (opt.help) {
		Usage(op, NULL);
		exit(0);
	}
	// Handle atypical variable naming
	if ( opt.atypvars.size() != 12 ) {
		cerr << "If -atypvars option is given, colon delimited list must have exactly 12 elements specifying names of variables which are typically named ocean_time:h:xi_rho:xi_psi:xi_u:xi_v:eta_rho:eta_psi:eta_u:eta_v:s_rho:s_w" << endl;
		exit( 1 );
	}

	map <string, string> atypvars;
	atypvars["ocean_time"] = opt.atypvars[0];
	atypvars["h"] = opt.atypvars[1];
	atypvars["xi_rho"] = opt.atypvars[2];
	atypvars["xi_psi"] = opt.atypvars[3];
	atypvars["xi_u"] = opt.atypvars[4];
	atypvars["xi_v"] = opt.atypvars[5];
	atypvars["eta_rho"] = opt.atypvars[6];
	atypvars["eta_psi"] = opt.atypvars[7];
	atypvars["eta_u"] = opt.atypvars[8];
	atypvars["eta_v"] = opt.atypvars[9];
	atypvars["s_rho"] = opt.atypvars[10];
	atypvars["s_w"] = opt.atypvars[11];


	string atypvarstring;
	for (int i = 0; i<12; i++){
		atypvarstring += opt.atypvars[i]; 
		if (i<11) atypvarstring += ":";
	}
	
	argv++;
	argc--;

	if (argc < 3) {
		Usage(op, "Not enough file names to process.  Must specify >0 ROMS data files, one topo file, one vdf file");
		exit(1);
	}

	vector <size_t> cratios;
	string wname;
	string wmode;
	
	if (opt.bs.nx < 0) {
		for (int i=0; i<3; i++) bs[i] = 32;
	}
	else {
		bs[0] = opt.bs.nx; bs[1] = opt.bs.ny; bs[2] = opt.bs.nz;
	}

	//
	// Handle options for VDC2 output.
	//
	
	if (!opt.vdc1) {
		wname = opt.wname;

		if ((wname.compare("bior1.1") == 0) ||
			(wname.compare("bior1.3") == 0) ||
			(wname.compare("bior1.5") == 0) ||
			(wname.compare("bior3.3") == 0) ||
			(wname.compare("bior3.5") == 0) ||
			(wname.compare("bior3.7") == 0) ||
			(wname.compare("bior3.9") == 0)) {

			wmode = "symh"; 
		}
		else if ((wname.compare("bior2.2") == 0) ||
			(wname.compare("bior2.4") == 0) ||
			(wname.compare("bior2.6") == 0) ||
			(wname.compare("bior2.8") == 0)) {

			wmode = "symw"; 
		}
		else {
			wmode = "sp0"; 
		}

		if (opt.bs.nx < 0) {
			for (int i=0; i<3; i++) bs[i] = 64;
		}
		else {
			bs[0] = opt.bs.nx; bs[1] = opt.bs.ny; bs[2] = opt.bs.nz;
		}

		for (int i=0;i<opt.cratios.size();i++)cratios.push_back(opt.cratios[i]);

		if (cratios.size() == 0) {
			cratios.push_back(1);
			cratios.push_back(10);
			cratios.push_back(100);
			cratios.push_back(500);
		}

		size_t maxcratio = WaveCodecIO::GetMaxCRatio(bs, wname, wmode);
		for (int i=0;i<cratios.size();i++) {
			if (cratios[i] == 0 || cratios[i] > maxcratio) {
				MyBase::SetErrMsg(
					"Invalid compression ratio (%d) for configuration "
					"(block_size, wavename)", cratios[i]
				);
				exit(1);
			}
		}

	} // End if !vdc1

	//
	// At this point there is at least one ROMS file to work with.
	// The format is roms-files + grid-file + vdf-file.
	//

	vector<string> romsfiles;
	for (int i=0; i<argc-2; i++) {
		 romsfiles.push_back(argv[i]);
	}
	string topofile = string(argv[argc-2]);
	vector<string> vars3d = opt.vars3d;
	vector<string> vars2d = opt.vars2d;
	ROMSData = new MetadataROMS(romsfiles, topofile, atypvars, vars2d, vars3d);
	
	if(ROMSData->GetNumTimeSteps() < 0) {
		cerr << "No output file generated due to no input files processed." << endl;
		exit(0);
	}

	MetadataVDC *file;
	if (!opt.vdc1) {
	 	file = new MetadataVDC(ROMSData->GetDimension(), bs, cratios, wname, wmode);
	}
	else {
		file = new MetadataVDC(ROMSData->GetDimension(), opt.level, bs, opt.nfilter, opt.nlifting);
	}
	
	if (MetadataVDC::GetErrCode() != 0) exit(1);

	// Copy values over from MetadataROMS to MetadataVDC.
	// Add checking of return values and error messsages.

	if(file->SetNumTimeSteps(ROMSData->GetNumTimeSteps())) {
		cerr << "Error populating NumTimeSteps." << endl;
		exit(1);
	}
	if(file->SetMapProjection(ROMSData->GetMapProjection())) {
		cerr << "Error populating MapProjection." << endl;
		exit(1);
	}

	vector <string> allvars3d = ROMSData->GetVariables3D();
	vector <string>::iterator itr;
	itr = find(allvars3d.begin(), allvars3d.end(), "ELEVATION");
	if (itr == allvars3d.end()) allvars3d.push_back("ELEVATION");

	if(file->SetVariables3D(allvars3d)) {
		cerr << "Error populating Variables3D." << endl;
		exit(1);
	}

	

	vector <string> allvars2d = ROMSData->GetVariables2DXY();
	itr = find(allvars2d.begin(), allvars2d.end(), "DEPTH");
	if (itr == allvars2d.end()) allvars2d.push_back("DEPTH");
	itr = find(allvars2d.begin(), allvars2d.end(), "angleRAD");
	if (itr == allvars2d.end()) allvars2d.push_back("angleRAD");
	itr = find(allvars2d.begin(), allvars2d.end(), "latDEG");
	if (itr == allvars2d.end()) allvars2d.push_back("latDEG");

	if(file->SetVariables2DXY(allvars2d)) {
		cerr << "Error populating Variables2DXY." << endl;
		exit(1);
	}
	if(file->SetVariables2DXZ(ROMSData->GetVariables2DXZ())) {
		cerr << "Error populating Variables2DXZ." << endl;
		exit(1);
	}
	if(file->SetVariables2DYZ(ROMSData->GetVariables2DYZ())) {
		cerr << "Error populating Variables2DYZ." << endl;
		exit(1);
	}
	if(file->SetExtents(ROMSData->GetExtents())) {
		cerr << "Error populating Extents." << endl;
		exit(1);
	}
	if(file->SetGridType(ROMSData->GetGridType())) {
		cerr << "Error populating GridType." << endl;
		exit(1);
	}
	if(file->SetUserDataString("DependentVarNames", atypvarstring)) {
		cerr << "Error populating Dependent Vars." << endl;
		exit(1);
	}
	string usertimestamp;
	vector <double> usertime;
	for(int i = 0; i < ROMSData->GetNumTimeSteps(); i++) {
		ROMSData->GetTSUserTimeStamp(i, usertimestamp);
		if(file->SetTSUserTimeStamp(i, usertimestamp)) {
			cerr << "Error populating TSUserTimeStamp." << endl;
			exit(1);
		}
		usertime.clear();
		usertime.push_back(ROMSData->GetTSUserTime(i));
		if(file->SetTSUserTime(i, usertime)) {
			cerr << "Error populating TSUserTime." << endl;
			exit(1);
		}
	}
	
	file->SetMissingValue(VDCMissingValue);
	// Handle command line over rides here.

	s.assign(opt.comment);
	if(file->SetComment(s) < 0) {
		cerr << "Error populating Comment." << endl;
		exit(1);
	}


	// Write file.

	if (file->Write(argv[argc-1]) < 0) {
		exit(1);
	}

	if (! opt.quiet && ROMSData->GetNumTimeSteps() > 0) {
		cout << "Created VDF file:" << endl;
		cout << "\tNum time steps : " << file->GetNumTimeSteps() << endl;
		cout << "\t3D Variable names : ";
		for (int i = 0; i < file->GetVariables3D().size(); i++) {
			cout << file->GetVariables3D()[i] << " ";
		}
		cout << endl;
		cout << "\t2D Variable names : ";
		for (int i=0; i < file->GetVariables2DXY().size(); i++) {
			cout << file->GetVariables2DXY()[i] << " ";
		}
		cout << endl;

		cout << "\tCoordinate extents : ";
		const vector <double> extptr = file->GetExtents();
		for(int i=0; i<6; i++) {
			cout << extptr[i] << " ";
		}
		cout << endl;
		if (ROMSData->GetMinLon() < 1000.f){
			cout << "\tMin Longitude and Latitude of domain corners: " << ROMSData->GetMinLon() << " " << ROMSData->GetMinLat() << endl;
			cout << "\tMax Longitude and Latitude of domain corners: " << ROMSData->GetMaxLon() << " " << ROMSData->GetMaxLat() << endl;
		}
		
	} // End if quiet.

	exit(0);
} // End of main.
