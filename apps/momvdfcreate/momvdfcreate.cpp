#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sstream>
#include <algorithm>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/DCReaderMOM.h>
#include <vapor/VDCFactory.h>
#include <vapor/CFuncs.h>

#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace VetsUtil;
using namespace VAPoR;

struct opt_t {
	vector <string> vars;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
	OptionParser::Boolean_T	debug;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"vars",1,    "",	"Colon delimited list of variables to be copied "
		"from ncdf data. The default is to copy all 2D and 3D variables"},
	{"help",	0,	"",	"Print this message and exit"},
	{"quiet",	0,	"",	"Operate quietly"},
	{"debug",	0,	"",	"Turn on debugging"},
	{NULL}
};

OptionParser::Option_T	get_options[] = {
	{"vars", VetsUtil::CvtToStrVec, &opt.vars, sizeof(opt.vars)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{NULL}
};

const char *ProgName;

void Usage(OptionParser &op, const char * msg) {

	if (msg) {
		cerr << ProgName << " : " << msg << endl;
	}
	cerr << "Usage: " << ProgName << " [options] ncdf_file... vdf_file" << endl;
	op.PrintOptionHelp(stderr, 80, false);

}

MetadataVDC *CreateMetadataVDC(
	const VDCFactory &vdcf,
	const DCReaderMOM *momData
) {

	size_t dims[3];
	momData->GetGridDim(dims);

	//
	// Create default MetadataVDC object based on command line
	// options
	//
	MetadataVDC *file;
	file = vdcf.New(dims);
	if (MetadataVDC::GetErrCode() != 0) exit(1);

	if (file->GetVDCType() == 1) {
		cerr << "VDC Type 1 not supported\n";
		exit(1);
	}

	// Copy values over from DCReaderMOM to MetadataVDC.
	// Add checking of return values and error messsages.
	//
	if(file->SetNumTimeSteps(momData->GetNumTimeSteps())) {
		cerr << "Error populating NumTimeSteps." << endl;
		exit(1);
	}

	file->SetExtents(momData->GetExtents());


	//
	// By default we populate the VDC with all 2D and 3D variables found in 
	// the netCDF files. This can be overriden by the "-vars" option
	// 

	vector <string> candidate_vars, vars;
	candidate_vars = momData->GetVariables3D();
	if (! opt.vars.size()) {
		vars = candidate_vars;
	}
	else {

		vars.clear();
		for (int i=0; i<opt.vars.size(); i++) {
			vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), opt.vars[i]);
			if (itr != candidate_vars.end()) {
				vars.push_back(opt.vars[i]);
			}
		}
	}
	if(file->SetVariables3D(vars)) {
		cerr << "Error populating Variables." << endl;
		exit(1);
	}

	candidate_vars = momData->GetVariables2DXY();
	if (! opt.vars.size()) {
		vars = candidate_vars;
	}
	else {

		vars.clear();
		for (int i=0; i<opt.vars.size(); i++) {
			vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), opt.vars[i]);
			if (itr != candidate_vars.end()) {
				vars.push_back(opt.vars[i]);
			}
		}
	}
	if(file->SetVariables2DXY(vars)) {
		cerr << "Error populating Variables." << endl;
		exit(1);
	}

	candidate_vars = momData->GetVariables2DXZ();
	if (! opt.vars.size()) {
		vars = candidate_vars;
	}
	else {

		vars.clear();
		for (int i=0; i<opt.vars.size(); i++) {
			vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), opt.vars[i]);
			if (itr != candidate_vars.end()) {
				vars.push_back(opt.vars[i]);
			}
		}
	}
	if(file->SetVariables2DXZ(vars)) {
		cerr << "Error populating Variables." << endl;
		exit(1);
	}

	candidate_vars = momData->GetVariables2DYZ();
	if (! opt.vars.size()) {
		vars = candidate_vars;
	}
	else {

		vars.clear();
		for (int i=0; i<opt.vars.size(); i++) {
			vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), opt.vars[i]);
			if (itr != candidate_vars.end()) {
				vars.push_back(opt.vars[i]);
			}
		}
	}
	if(file->SetVariables2DYZ(vars)) {
		cerr << "Error populating Variables." << endl;
		exit(1);
	}

	vars = momData->GetVariableNames();
	for(int t = 0; t < momData->GetNumTimeSteps(); t++) {
		for (int j=0; j<vars.size(); j++) {
			float mv;
			bool has_missing = momData->GetMissingValue(vars[j], mv);
			if (has_missing) file->SetVMissingValue(t,vars[j], 1e37);
		}
	}


	vector <double> usertime;
	for(int t = 0; t < momData->GetNumTimeSteps(); t++) {

		usertime.clear();
		usertime.push_back(momData->GetTSUserTime(t));
		if(file->SetTSUserTime(t, usertime)) {
			cerr << "Error populating TSUserTime." << endl;
			exit(1);
		}

		string timestamp;
		momData->GetTSUserTimeStamp(t, timestamp);
		file->SetTSUserTimeStamp(t, timestamp);
	}

	string gridtype = momData->GetGridType();
	file->SetGridType(gridtype);
	if (gridtype.compare("stretched") == 0) {
		for(size_t ts = 0; ts < momData->GetNumTimeSteps(); ts++) {
			int rc = file->SetTSZCoords(ts, momData->GetTSZCoords(ts));
			if (rc<0) exit(1);
		}
	}

	//
	// Map projection is always lat-lon
	//
	file->SetMapProjection("+proj=latlon +ellps=sphere");

	return(file);
}

char ** argv_merge(
	int argc1, char **argv1, int argc2, char **argv2,
	int &newargc
) {
	char **newargv = new char * [argc1+argc2+1];
	newargc = 0;
	for (int i=0; i<argc1; i++) newargv[newargc++] = argv1[i];
	for (int i=0; i<argc2; i++) newargv[newargc++] = argv2[i];
	newargv[newargc] = NULL;
	return(newargv);
}

int	main(int argc, char **argv) {

    MyBase::SetErrMsgFilePtr(stderr);

	OptionParser op;

	string s;
	DCReaderMOM *momData;

	ProgName = Basename(argv[0]);

	// Ugh. All this just to add a default option to argv
	//
	char **myargv;
	int myargc;
	char *argv2[] = { (char *) "-vdc2", NULL };
	myargv = argv_merge(argc, argv, 1, argv2, myargc);


	if (op.AppendOptions(set_opts) < 0) {
		exit(1);
	}

	if (op.ParseOptions(&myargc, myargv, get_options) < 0) {
		exit(1);
	}
	if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	VDCFactory vdcf;
	vector <string> rmopts;
	rmopts.push_back("nfilter");
	rmopts.push_back("nlifting");
	rmopts.push_back("varnames");
	rmopts.push_back("vars3d");
	rmopts.push_back("vars2dxy");
	rmopts.push_back("vars2dxz");
	rmopts.push_back("vars2dyz");
	rmopts.push_back("missing");
	rmopts.push_back("level");
	rmopts.push_back("order");
	rmopts.push_back("start");
	rmopts.push_back("deltat");
	rmopts.push_back("gridtype");
	rmopts.push_back("usertimes");
	rmopts.push_back("xcoords");
	rmopts.push_back("ycoords");
	rmopts.push_back("zcoords");
	rmopts.push_back("mapprojection");
	rmopts.push_back("coordsystem");
	rmopts.push_back("extents");

    vdcf.RemoveOptions(rmopts);

	if (vdcf.Parse(&myargc, myargv) < 0) {
		exit(1);
	}

	if (opt.help) {
		Usage(op, NULL);
		vdcf.Usage(stderr);
		exit(0);
	}


	myargv++;
	myargc--;

	if (myargc < 2) {
		Usage(op, "No files to process");
		vdcf.Usage(stderr);
		exit(1);
	}

	vector<string> ncdffiles;
	for (int i=0; i<myargc-1; i++) {
		 ncdffiles.push_back(myargv[i]);
	}
	
	momData = new DCReaderMOM(ncdffiles);
	if (MyBase::GetErrCode() != 0) exit(1);

	if(momData->GetNumTimeSteps() < 0) {
		cerr << "No output file generated due to no input files processed." << endl;
		exit(0);
	}

	//
	// Create a MetadataVDC object
	//
	MetadataVDC *file;
	file = CreateMetadataVDC(vdcf, momData);

	// Write file.
	if (file->Write(myargv[myargc-1]) < 0) {
		exit(1);
	}

	if (! opt.quiet && momData->GetNumTimeSteps() > 0) {
		cout << "Created VDF file:" << endl;
		cout << "\tNum time steps : " << file->GetNumTimeSteps() << endl;

		cout << "\t3D Variable names : ";
		for (int i = 0; i < file->GetVariables3D().size(); i++) {
			cout << file->GetVariables3D()[i] << " ";
		}
		cout << endl;

		cout << "\t2DXY Variable names : ";
		for (int i=0; i < file->GetVariables2DXY().size(); i++) {
			cout << file->GetVariables2DXY()[i] << " ";
		}
		cout << endl;

		cout << "\t2DXZ Variable names : ";
		for (int i=0; i < file->GetVariables2DXZ().size(); i++) {
			cout << file->GetVariables2DXZ()[i] << " ";
		}
		cout << endl;

		cout << "\t2DYZ Variable names : ";
		for (int i=0; i < file->GetVariables2DYZ().size(); i++) {
			cout << file->GetVariables2DYZ()[i] << " ";
		}
		cout << endl;

		cout << "\tExcluded 3D Variable names : ";
		for (int i = 0; i < momData->GetVariables3DExcluded().size(); i++) {
			cout << momData->GetVariables3DExcluded()[i] << " ";
		}
		cout << endl;

		cout << "\tExcluded 2D Variable names : ";
		for (int i = 0; i < momData->GetVariables2DExcluded().size(); i++) {
			cout << momData->GetVariables2DExcluded()[i] << " ";
		}
		cout << endl;

		cout << "\tCoordinate extents : ";
		const vector <double> extptr = file->GetExtents();
		for(int i=0; i<6; i++) {
			cout << extptr[i] << " ";
		}
		cout << endl;
		
	} // End if quiet.

	exit(0);
} // End of main.
