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
#include <vapor/MetadataWRF.h>
#include <vapor/MetadataSpherical.h>
#include <vapor/WRF.h>
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
	vector<string> dervars;
	vector<string> atypvars;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"varnames",1,	"",			"Deprecated. Use -vars3d instead"},
	{"vars3d",1,	"",			"Colon delimited list of 3D variables to be extracted from WRF data"},
	{"vars2d",1,    "",         "Colon delimited list of 2D variables to be extracted from WRF data"},
	{"dervars", 1,	"",	"Colon delimited list of desired derived variables.  Choices are: PHNorm_: normalized geopotential (PH+PHB)/PHB, UVW_: 3D wind speed (U^2+V^2+W^2)^1/2, UV_: 2D wind speed (U^2+V^2)^1/2, omZ_: estimate of vertical vorticity, PFull_: full pressure P+PB, PNorm_: normalized pressure (P+PB)/PB, Theta_: potential temperature T+300, TK_: temp. in Kelvin (T+300)((P+PB))/100000)^0.286"},
	{"level",	1, 	"2",			"Maximum refinement level. 0 => no refinement (default is 2)"},
	{"atypvars",1,	"U:V:W:PH:PHB:P:PB:T",		"Colon delimited list of atypical names for U:V:W:PH:PHB:P:PB:T that appear in WRF file"},
	{"comment",	1,	"",				"Top-level comment (optional)"},
	{"bs",		1, 	"32x32x32",		"Internal storage blocking factor expressed in grid points (NXxNYxNZ) (default is 32)"},
	{"nfilter",	1, 	"1",			"Number of wavelet filter coefficients (default is 1)"},
	{"nlifting",1, 	"1",			"Number of wavelet lifting coefficients (default is 1)"},
	{"help",	0,	"",				"Print this message and exit"},
	{"quiet",	0,	"",				"Operate quietly"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"vars3d", VetsUtil::CvtToStrVec, &opt.vars3d, sizeof(opt.vars3d)},
	{"vars2d", VetsUtil::CvtToStrVec, &opt.vars2d, sizeof(opt.vars2d)},
	{"dervars", VetsUtil::CvtToStrVec, &opt.dervars, sizeof(opt.dervars)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"atypvars", VetsUtil::CvtToStrVec, &opt.atypvars, sizeof(opt.atypvars)},
	{"comment", VetsUtil::CvtToString, &opt.comment, sizeof(opt.comment)},
	{"bs", VetsUtil::CvtToDimension3D, &opt.bs, sizeof(opt.bs)},
	{"nfilter", VetsUtil::CvtToInt, &opt.nfilter, sizeof(opt.nfilter)},
	{"nlifting", VetsUtil::CvtToInt, &opt.nlifting, sizeof(opt.nlifting)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};

const char *ProgName;

void Usage(OptionParser &op, const char * msg) {

	if (msg) {
		cerr << ProgName << " : " << msg << endl;
	}
	cerr << "Usage: " << ProgName << " [options] wrf_ncdf_file... vdf_file" << endl;
	op.PrintOptionHelp(stderr);

}

void ErrMsgCBHandler(const char *msg, int) {
	cerr << ProgName << " : " << msg << endl;
}

int	main(int argc, char **argv) {

	OptionParser op;
	string s;
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

	// Handle atypical variable naming
	if ( opt.atypvars.size() != 8 ) {
		cerr << "If -atypvars option is given, colon delimited list must have exactly eight elements specifying names of variables which are typically named U:V:W:PH:PHB:P:PB:T" << endl;
		exit( 1 );
	}

	map <string, string> atypvars;
	atypvars["U"] = opt.atypvars[0];
	atypvars["V"] = opt.atypvars[1];
	atypvars["W"] = opt.atypvars[2];
	atypvars["PH"] = opt.atypvars[3];
	atypvars["PHB"] = opt.atypvars[4];
	atypvars["P"] = opt.atypvars[5];
	atypvars["PB"] = opt.atypvars[6];
	atypvars["T"] = opt.atypvars[7];

	string atypvarstring;
	atypvarstring += opt.atypvars[0]; atypvarstring += ":";
	atypvarstring += opt.atypvars[1]; atypvarstring += ":";
	atypvarstring += opt.atypvars[2]; atypvarstring += ":";
	atypvarstring += opt.atypvars[3]; atypvarstring += ":";
	atypvarstring += opt.atypvars[4]; atypvarstring += ":";
	atypvarstring += opt.atypvars[5]; atypvarstring += ":";
	atypvarstring += opt.atypvars[6]; atypvarstring += ":";
	atypvarstring += opt.atypvars[7];

	argv++;
	argc--;

	if (argc < 2) {
		Usage(op, "No files to process");
		exit(1);
	}

	//
	// At this point there is at last one WRF file to work with.
	// The format is wrf-file+ vdf-file.
	//

	vector<string> wrffiles;
	for (int i=0; i<argc-1; i++) {
		 wrffiles.push_back(argv[i]);
	}
	WRFData = new MetadataWRF(wrffiles, atypvars);


	
	if(WRFData->GetNumTimeSteps() < 0) {
		cerr << "No output file generated due to no input files processed." << endl;
		exit(0);
	}

	size_t bs[] = {opt.bs.nx, opt.bs.ny, opt.bs.nz};
	MetadataVDC *file = new MetadataVDC(WRFData->GetDimension(), opt.level, bs, opt.nfilter, opt.nlifting);
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
		cerr << "Error populating Variables2DXY." << endl;
		exit(1);
	}
	if(file->SetVariables2DXZ(WRFData->GetVariables2DXZ())) {
		cerr << "Error populating Variables2DXZ." << endl;
		exit(1);
	}
	if(file->SetVariables2DYZ(WRFData->GetVariables2DYZ())) {
		cerr << "Error populating Variables2DYZ." << endl;
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
	if(file->SetUserDataString("DependentVarNames", atypvarstring)) {
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

	s.assign(opt.comment);
	if(file->SetComment(s) < 0) {
		cerr << "Error populating Comment." << endl;
		exit(1);
	}

	if(opt.vars3d.size() > 0) {
		vector <string> new3dvars;
		vector <string> cur3dvars;
		cur3dvars = file->GetVariables3D();
		new3dvars.clear();
		for(int i = 0; i < opt.vars3d.size(); i++) {
			int j;
			for (j = 0; j < cur3dvars.size(); j++ ) {
				if (cur3dvars[j] == opt.vars3d[i])
					break;
			} // End of for j.
			if (j < cur3dvars.size()) {
				int k;
				for(k = 0 ; k < new3dvars.size(); k++) {
					if (new3dvars[k] == opt.vars3d[i])
                                	        break;
				} // End of for k.
				if (k < new3dvars.size() || new3dvars.size() == 0)
					new3dvars.push_back(opt.vars3d[i]);
			}
			else {
				cerr << ProgName << " : Invalid variable : " << opt.vars3d[i] << endl;
			}
		} // End of for i.
		if(file->SetVariables3D(new3dvars)) {
                	cerr << "Error populating Variables3D." << endl;
                	exit(1);
        	}
	} // End of opt.vars3d

	if(opt.vars2d.size() > 0) {
		vector <string> new2dvars;
		vector <string> cur2dvars;
		cur2dvars = file->GetVariables2DXY();
		new2dvars.clear();
		for(int i = 0; i < opt.vars2d.size(); i++) {
			int j;
			for (j = 0; j < cur2dvars.size(); j++ ) {
				if (cur2dvars[j] == opt.vars2d[i])
					break;
			} // End of for j.
			if (j < cur2dvars.size()) {
				int k;
				for(k = 0 ; k < new2dvars.size(); k++) {
					if (new2dvars[k] == opt.vars2d[i])
                                	        break;
				} // End of for k.
				if (k < new2dvars.size() || new2dvars.size() == 0)
					new2dvars.push_back(opt.vars2d[i]);
			}
			else {
				cerr << ProgName << " : Invalid variable : " << opt.vars2d[i] << endl;
			}
		} // End of for i.
		if(file->SetVariables2DXY(new2dvars)) {
                	cerr << "Error populating Variables2D." << endl;
                	exit(1);
        	}
	} // End of opt.vars2d

	if(opt.dervars.size() > 0) {
		vector <string> new3dvars;
		new3dvars = file->GetVariables3D();
		for(int i = 0; i < opt.dervars.size(); i++) {
			if (opt.dervars[i] == "PFull_") {
				new3dvars.push_back("PFull_");
			}
			else if (opt.dervars[i] == "PNorm_") {
				new3dvars.push_back("PNorm_");
                        }
			else if (opt.dervars[i] == "PHNorm_") {
				new3dvars.push_back("PHNorm_");
                        }
			else if (opt.dervars[i] == "Theta_") {
				new3dvars.push_back("Theta_");
                        }
			else if (opt.dervars[i] == "TK_") {
				new3dvars.push_back("TK_");
                        }
			else if (opt.dervars[i] == "UV_") {
				new3dvars.push_back("UV_");
                        }
			else if (opt.dervars[i] == "UVW_") {
				new3dvars.push_back("UVW_");
                        }
			else if (opt.dervars[i] == "omZ_") {
				new3dvars.push_back("omZ_");
                        }
			else {
				cerr << ProgName << " : Invalid derived variable : " << opt.dervars[i] << endl;
			}
		} // End of for.
		if(file->SetVariables3D(new3dvars)) {
                	cerr << "Error populating Variables3D." << endl;
                	exit(1);
        	}
	} // End of if opt.dervars.

	// Write file.

	if (file->Write(argv[argc-1]) < 0) {
		exit(1);
	}

	if (! opt.quiet && WRFData->GetNumTimeSteps() > 0) {
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
		if (WRFData->GetminLon() < 1000.f){
			cout << "\tMin Longitude and Latitude of domain corners: " << WRFData->GetminLon() << " " << WRFData->GetminLat() << endl;
			cout << "\tMax Longitude and Latitude of domain corners: " << WRFData->GetmaxLon() << " " << WRFData->GetmaxLat() << endl;
		}
		
	} // End if quiet.

	exit(0);
} // End of main.
