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
#include <vapor/MetadataMOM.h>
#include <vapor/WaveCodecIO.h>
#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace VetsUtil;
using namespace VAPoR;

struct opt_t {
	OptionParser::Dimension3D_T	bs;
	int	level;
	int	nfilter;
	int	nlifting;
	char *comment;
	vector <string> vars3d;
	vector <string> vars2d;
	vector<string> atypvars;
	OptionParser::Boolean_T	vdc2;	
	vector <int> cratios;
	char *wname;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"vars3d",1,	"",	"Colon delimited list of 3D variables to be extracted from MOM data."
		"The default is to extract all 3D variables present."},
	{"vars2d",1,    "",	"Colon delimited list of 2D variables to be extracted from MOM data."
		"The default is to extract all 2D variables present."},
	{"level",	1, 	"2", "Maximum refinement level. 0 => no refinement (default is 2)"},
	{"atypvars",1,	"time:ht:st_ocean:st_edges_ocean:sw_ocean:sw_edges_ocean", 
		"Colon delimited list of atypical names for time:ht:st_ocean:st_edges_ocean:sw_ocean:sw_edges_ocean that appear in MOM file."
		"Acceptable variants: Time or TIME for time; z_t for st_ocean; z_w for st_edges_ocean; HU for ht (in centimeter units)."
	},
	{"comment",	1,	"",	"Top-level comment (optional)"},
	{"bs",		1, 	"-1x-1x-1",	"Internal storage blocking factor "
		"expressed in grid points (NXxNYxNZ). Defaults: 32x32x32 (VDC type 1), "
		"64x64x64 (VDC type 2"},
	{"nfilter",	1, 	"1", "Number of wavelet filter coefficients (default is 1)"},
	{"nlifting",1, 	"1", "Number of wavelet lifting coefficients (default is 1)"},
	{"vdc2",	0,	"",	"Generate a VDC Type 2 .vdf file (default is VDC Type 1)"},
	{"cratios",1,	"",	"Colon delimited list compression ratios. "
		"The default is 1:10:100:500. " 
		"The maximum compression ratio is wavelet and block size dependent."},
	{"wname",	1,	"bior3.3",	"Wavelet family used for compression "
		"(VDC type 2, only). Recommended values are bior1.1, bior1.3, "
		"bior1.5, bior2.2, bior2.4 ,bior2.6, bior2.8, bior3.1, bior3.3, "
		"bior3.5, bior3.7, bior3.9"},
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
	cerr << "Usage: " << ProgName << " [options] MOM_netcdf_datafile(s) ... MOM_topo_file vdf_file" << endl;
	op.PrintOptionHelp(stderr);

}

void ErrMsgCBHandler(const char *msg, int) {
	cerr << ProgName << " : " << msg << endl;
}

int	main(int argc, char **argv) {

	OptionParser op;
	string s;
	MetadataMOM *MOMData;

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
	if ( opt.atypvars.size() != 6 ) {
		cerr << "If -atypvars option is given, colon delimited list must have exactly six elements specifying names of variables which are typically named time:ht:st_ocean:st_edges_ocean:sw_ocean:sw_edges_ocean" << endl;
		exit( 1 );
	}

	map <string, string> atypvars;
	atypvars["time"] = opt.atypvars[0];
	atypvars["ht"] = opt.atypvars[1];
	atypvars["st_ocean"] = opt.atypvars[2];
	atypvars["st_edges_ocean"] = opt.atypvars[3];
	atypvars["sw_ocean"] = opt.atypvars[4];
	atypvars["sw_edges_ocean"] = opt.atypvars[5];

	string atypvarstring;
	atypvarstring += opt.atypvars[0]; atypvarstring += ":";
	atypvarstring += opt.atypvars[1]; atypvarstring += ":";
	atypvarstring += opt.atypvars[2]; atypvarstring += ":";
	atypvarstring += opt.atypvars[3]; atypvarstring += ":";
	atypvarstring += opt.atypvars[4]; atypvarstring += ":";
	atypvarstring += opt.atypvars[5]; 

	argv++;
	argc--;

	if (argc < 3) {
		Usage(op, "Not enough file names to process.  Must specify >0 MOM data files,  one topo file, one vdf file");
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

	} // End if vdc2

	//
	// At this point there is at least one MOM file to work with.
	// The format is mom-files + topo-file + vdf-file.
	//

	vector<string> momfiles;
	for (int i=0; i<argc-2; i++) {
		 momfiles.push_back(argv[i]);
	}
	string topofile = string(argv[argc-2]);
	vector<string> vars3d = opt.vars3d;
	vector<string> vars2d = opt.vars2d;
	MOMData = new MetadataMOM(momfiles, topofile, atypvars, vars2d, vars3d);
	
	if(MOMData->GetNumTimeSteps() < 0) {
		cerr << "No output file generated due to no input files processed." << endl;
		exit(0);
	}

	MetadataVDC *file;
	if (opt.vdc2) {
	 	file = new MetadataVDC(MOMData->GetDimension(), bs, cratios, wname, wmode);
	}
	else {
		file = new MetadataVDC(MOMData->GetDimension(), opt.level, bs, opt.nfilter, opt.nlifting);
	}
	
	if (MetadataVDC::GetErrCode() != 0) exit(1);

	// Copy values over from MetadataMOM to MetadataVDC.
	// Add checking of return values and error messsages.

	if(file->SetNumTimeSteps(MOMData->GetNumTimeSteps())) {
		cerr << "Error populating NumTimeSteps." << endl;
		exit(1);
	}
	if(file->SetMapProjection(MOMData->GetMapProjection())) {
		cerr << "Error populating MapProjection." << endl;
		exit(1);
	}

	vector <string> allvars3d = 
		MOMData->GetVariables3D();
	vector <string>::iterator itr;
	itr = find(allvars3d.begin(), allvars3d.end(), "ELEVATION");
	if (itr == allvars3d.end()) allvars3d.push_back("ELEVATION");

	if(file->SetVariables3D(allvars3d)) {
		cerr << "Error populating Variables3D." << endl;
		exit(1);
	}

	vector <string> allvars2d = MOMData->GetVariables2DXY();
	itr = find(allvars2d.begin(), allvars2d.end(), "DEPTH");
	if (itr == allvars2d.end()) allvars2d.push_back("DEPTH");

	if(file->SetVariables2DXY(allvars2d)) {
		cerr << "Error populating Variables2DXY." << endl;
		exit(1);
	}
	if(file->SetVariables2DXZ(MOMData->GetVariables2DXZ())) {
		cerr << "Error populating Variables2DXZ." << endl;
		exit(1);
	}
	if(file->SetVariables2DYZ(MOMData->GetVariables2DYZ())) {
		cerr << "Error populating Variables2DYZ." << endl;
		exit(1);
	}
	if(file->SetExtents(MOMData->GetExtents())) {
		cerr << "Error populating Extents." << endl;
		exit(1);
	}
	if(file->SetGridType(MOMData->GetGridType())) {
		cerr << "Error populating GridType." << endl;
		exit(1);
	}
	if(file->SetUserDataString("DependentVarNames", atypvarstring)) {
		cerr << "Error populating Dependent Vars." << endl;
		exit(1);
	}
	string usertimestamp;
	vector <double> usertime;
	for(int i = 0; i < MOMData->GetNumTimeSteps(); i++) {
		MOMData->GetTSUserTimeStamp(i, usertimestamp);
		if(file->SetTSUserTimeStamp(i, usertimestamp)) {
			cerr << "Error populating TSUserTimeStamp." << endl;
			exit(1);
		}
		usertime.clear();
		usertime.push_back(MOMData->GetTSUserTime(i));
		if(file->SetTSUserTime(i, usertime)) {
			cerr << "Error populating TSUserTime." << endl;
			exit(1);
		}
		
	}

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

	if (! opt.quiet && MOMData->GetNumTimeSteps() > 0) {
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
		if (MOMData->GetminLon() < 1000.f){
			cout << "\tMin Longitude and Latitude of domain corners: " << MOMData->GetminLon() << " " << MOMData->GetminLat() << endl;
			cout << "\tMax Longitude and Latitude of domain corners: " << MOMData->GetmaxLon() << " " << MOMData->GetmaxLat() << endl;
		}
		
	} // End if quiet.

	exit(0);
} // End of main.
