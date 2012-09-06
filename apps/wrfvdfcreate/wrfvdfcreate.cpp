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
	vector<string> dervars;
	vector<string> atypvars;
	OptionParser::Boolean_T	vdc2;	
	OptionParser::Boolean_T	append;	
	vector <int> cratios;
	char *wname;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"varnames",1,	"",	"Deprecated. Use -vars3d instead"},
	{"vars3d",1,	"",	"Colon delimited list of 3D variables to be extracted from WRF data"},
	{"vars2d",1,    "",	"Colon delimited list of 2D variables to be extracted from WRF data"},
	{"dervars", 1,	"",	"Colon delimited list of desired derived variables.  Choices are: PHNorm_: normalized geopotential (PH+PHB)/PHB, UVW_: 3D wind speed (U^2+V^2+W^2)^1/2, UV_: 2D wind speed (U^2+V^2)^1/2, omZ_: estimate of vertical vorticity, PFull_: full pressure P+PB, PNorm_: normalized pressure (P+PB)/PB, Theta_: potential temperature T+300, TK_: temp. in Kelvin (T+300)((P+PB))/100000)^0.286"},
	{"level",	1, 	"2", "Maximum refinement level. 0 => no refinement (default is 2)"},
	{"atypvars",1,	"U:V:W:PH:PHB:P:PB:T", "Colon delimited list of atypical names for U:V:W:PH:PHB:P:PB:T that appear in WRF file"},
	{"comment",	1,	"",	"Top-level comment (optional)"},
	{"bs",		1, 	"-1x-1x-1",	"Internal storage blocking factor "
		"expressed in grid points (NXxNYxNZ). Defaults: 32x32x32 (VDC type 1), "
		"64x64x64 (VDC type 2"},
	{"nfilter",	1, 	"1", "Number of wavelet filter coefficients (default is 1)"},
	{"nlifting",1, 	"1", "Number of wavelet lifting coefficients (default is 1)"},
	{"vdc2",	0,	"",	"Generate a VDC Type 2 .vdf file (default is VDC Type 1)"},
	{"append",	0,	"",	"Append WRF files to an existing .vdfd"},
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
	{"dervars", VetsUtil::CvtToStrVec, &opt.dervars, sizeof(opt.dervars)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"atypvars", VetsUtil::CvtToStrVec, &opt.atypvars, sizeof(opt.atypvars)},
	{"comment", VetsUtil::CvtToString, &opt.comment, sizeof(opt.comment)},
	{"bs", VetsUtil::CvtToDimension3D, &opt.bs, sizeof(opt.bs)},
	{"nfilter", VetsUtil::CvtToInt, &opt.nfilter, sizeof(opt.nfilter)},
	{"nlifting", VetsUtil::CvtToInt, &opt.nlifting, sizeof(opt.nlifting)},
	{"vdc2", VetsUtil::CvtToBoolean, &opt.vdc2, sizeof(opt.vdc2)},
	{"append", VetsUtil::CvtToBoolean, &opt.append, sizeof(opt.append)},
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
	cerr << "Usage: " << ProgName << " [options] wrf_ncdf_file... vdf_file" << endl;
	op.PrintOptionHelp(stderr);

}

void ErrMsgCBHandler(const char *msg, int) {
	cerr << ProgName << " : " << msg << endl;
}

MetadataVDC *CreateMetadataVDC(
	const MetadataWRF *WRFData
) {
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

	//
	// At this point there is at last one WRF file to work with.
	// The format is wrf-file+ vdf-file.
	//


	MetadataVDC *file;
	if (opt.vdc2) {
	 	file = new MetadataVDC(WRFData->GetDimension(), bs, cratios, wname, wmode);
	}
	else {
		file = new MetadataVDC(WRFData->GetDimension(), opt.level, bs, opt.nfilter, opt.nlifting);
	}
	
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

	vector <string> vars3d = 
		opt.vars3d.size() ? opt.vars3d : WRFData->GetVariables3D();
	vector <string>::iterator itr;
	itr = find(vars3d.begin(), vars3d.end(), "ELEVATION");
	if (itr == vars3d.end()) vars3d.push_back("ELEVATION");

	if(opt.dervars.size() > 0) {
		for(int i = 0; i < opt.dervars.size(); i++) {
			if (opt.dervars[i] == "PFull_") {
				vars3d.push_back("PFull_");
			}
			else if (opt.dervars[i] == "PNorm_") {
				vars3d.push_back("PNorm_");
                        }
			else if (opt.dervars[i] == "PHNorm_") {
				vars3d.push_back("PHNorm_");
                        }
			else if (opt.dervars[i] == "Theta_") {
				vars3d.push_back("Theta_");
                        }
			else if (opt.dervars[i] == "TK_") {
				vars3d.push_back("TK_");
                        }
			else if (opt.dervars[i] == "UV_") {
				vars3d.push_back("UV_");
                        }
			else if (opt.dervars[i] == "UVW_") {
				vars3d.push_back("UVW_");
                        }
			else if (opt.dervars[i] == "omZ_") {
				vars3d.push_back("omZ_");
                        }
			else {
				cerr << ProgName << " : Invalid derived variable : " << opt.dervars[i] << endl;
			}
		} // End of for.
	} // End of if opt.dervars.

	string atypvarstring;
	atypvarstring += opt.atypvars[0]; atypvarstring += ":";
	atypvarstring += opt.atypvars[1]; atypvarstring += ":";
	atypvarstring += opt.atypvars[2]; atypvarstring += ":";
	atypvarstring += opt.atypvars[3]; atypvarstring += ":";
	atypvarstring += opt.atypvars[4]; atypvarstring += ":";
	atypvarstring += opt.atypvars[5]; atypvarstring += ":";
	atypvarstring += opt.atypvars[6]; atypvarstring += ":";
	atypvarstring += opt.atypvars[7];

	if(file->SetVariables3D(vars3d)) {
		cerr << "Error populating Variables3D." << endl;
		exit(1);
	}

	vector <string> vars2d = 
		opt.vars2d.size() ? opt.vars2d : WRFData->GetVariables2DXY();
	itr = find(vars2d.begin(), vars2d.end(), "HGT");
	if (itr == vars2d.end()) vars2d.push_back("HGT");

	if(file->SetVariables2DXY(vars2d)) {
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
	// Handle command line over rides here.

	if(file->SetComment(opt.comment) < 0) {
		cerr << "Error populating Comment." << endl;
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
		if(file->SetTSExtents(i, WRFData->GetExtents(i))) {
			cerr << "Error populating TSExtents." << endl;
			exit(1);
		}
	}

	return(file);
}

MetadataVDC *AppendMetadataVDC(
	const MetadataWRF *WRFData,
	string metafile
) {
	MetadataVDC *file = new MetadataVDC(metafile);
	if (MetadataVDC::GetErrCode() != 0) exit(1);

	size_t tsVDC = file->GetNumTimeSteps();

	file->SetNumTimeSteps(file->GetNumTimeSteps() + WRFData->GetNumTimeSteps());

	string usertimestamp;
	vector <double> usertime;
	for(size_t tsWRF = 0; tsWRF < WRFData->GetNumTimeSteps(); tsWRF++, tsVDC++) {
		WRFData->GetTSUserTimeStamp(tsWRF, usertimestamp);
		if(file->SetTSUserTimeStamp(tsVDC, usertimestamp)) {
			cerr << "Error populating TSUserTimeStamp." << endl;
			exit(1);
		}
		usertime.clear();
		usertime.push_back(WRFData->GetTSUserTime(tsWRF));
		if(file->SetTSUserTime(tsVDC, usertime)) {
			cerr << "Error populating TSUserTime." << endl;
			exit(1);
		}
		if(file->SetTSExtents(tsVDC, WRFData->GetExtents(tsWRF))) {
			cerr << "Error populating TSExtents." << endl;
			exit(1);
		}
	}

	return(file);
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


	argv++;
	argc--;

	if (argc < 2) {
		Usage(op, "No files to process");
		exit(1);
	}

	vector<string> wrffiles;
	for (int i=0; i<argc-1; i++) {
		 wrffiles.push_back(argv[i]);
	}
	WRFData = new MetadataWRF(wrffiles, atypvars);
	
	if(WRFData->GetNumTimeSteps() < 0) {
		cerr << "No output file generated due to no input files processed." << endl;
		exit(0);
	}


	MetadataVDC *file;
	if (! opt.append) {
		file = CreateMetadataVDC(WRFData);
	}
	else {
		file = AppendMetadataVDC(WRFData, argv[argc-1]);
	}


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
