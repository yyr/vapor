#include <cstdio>
#include <cstdlib>
#include <vapor/vdfcreate.h>
#include <vapor/MyBase.h>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sstream>
#include <algorithm>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/DCReaderWRF.h>
#include <vapor/VDCFactory.h>
#include <vapor/CFuncs.h>
#include <vapor/WrfVDFcreator.h>

#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace std;
using namespace VetsUtil;
using namespace VAPoR;

wrfvdfcreate::wrfvdfcreate() {
	_progname.clear();
	_vars.clear();
	_dervars.clear();
	_vdc2 = false;;
	_append = false;
	_help = false;
	_quiet = false;
	_debug = false;
}

wrfvdfcreate::~wrfvdfcreate() {
}

void wrfvdfcreate::Usage(OptionParser &op, const char * msg) {

	if (msg) {
		cerr << _progname << " : " << msg << endl;
	}
	cerr << "Usage: " << _progname << " [options] ncdf_file... vdf_file" << endl;
	op.PrintOptionHelp(stderr, 80, false);
}

MetadataVDC *wrfvdfcreate::CreateMetadataVDC(
	const VDCFactory &vdcf,
	const DCReaderWRF *wrfData
	//const vector <string> _vars
) {

	size_t dims[3];
	wrfData->GetGridDim(dims);

	//
	// Create default MetadataVDC object based on command line
	// options
	//
	MetadataVDC *file;
	file = vdcf.New(dims);
	if (MetadataVDC::GetErrCode() != 0) return file;

	// Copy values over from DCReaderWRF to MetadataVDC.
	// Add checking of return values and error messsages.
	//
	if(file->SetNumTimeSteps(wrfData->GetNumTimeSteps())) {
		file->SetErrMsg(0,"Error in GetNumTimeSteps().");
		//cerr << "Error populating NumTimeSteps." << endl;
		return file;
	}

	file->SetExtents(wrfData->GetExtents());
	for(int ts = 0; ts < wrfData->GetNumTimeSteps(); ts++) {
		file->SetTSExtents(ts, wrfData->GetExtents(ts));
	}


	//
	// By default we populate the VDC with all 2D and 3D variables found in 
	// the netCDF files. This can be overriden by the "-vars" option
	// 

	vector <string> candidate_vars, vars;
	candidate_vars = wrfData->GetVariables3D();
	if (! _vars.size()) {
		vars = candidate_vars;
	}
	else {

		vars.clear();
		for (int i=0; i<_vars.size(); i++) {
			vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), _vars[i]);
			if (itr != candidate_vars.end()) {
				vars.push_back(_vars[i]);
			}
		}
	}
	if(file->SetVariables3D(vars)) {
		file->SetErrMsg(0,"Error in SetVariables3D.");
		//cerr << "Error populating Variables." << endl;
		return file;
	}

	candidate_vars = wrfData->GetVariables2DXY();
	if (! _vars.size()) {
		vars = candidate_vars;
	}
	else {

		vars.clear();
		for (int i=0; i<_vars.size(); i++) {
			vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), _vars[i]);
			if (itr != candidate_vars.end()) {
				vars.push_back(_vars[i]);
			}
		}
	}
	if(file->SetVariables2DXY(vars)) {
		file->SetErrMsg(0,"Error in SetVariables2DXY.");
		//cerr << "Error populating Variables." << endl;
		return file;
	}

	candidate_vars = wrfData->GetVariables2DXZ();
	if (! _vars.size()) {
		vars = candidate_vars;
	}
	else {

		vars.clear();
		for (int i=0; i<_vars.size(); i++) {
			vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), _vars[i]);
			if (itr != candidate_vars.end()) {
				vars.push_back(_vars[i]);
			}
		}
	}
	if(file->SetVariables2DXZ(vars)) {
		file->SetErrMsg(0,"Error in SetVariables2DXZ.");
		//cerr << "Error populating Variables." << endl;
		return file;
	}

	candidate_vars = wrfData->GetVariables2DYZ();
	if (! _vars.size()) {
		vars = candidate_vars;
	}
	else {

		vars.clear();
		for (int i=0; i<_vars.size(); i++) {
			vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), _vars[i]);
			if (itr != candidate_vars.end()) {
				vars.push_back(_vars[i]);
			}
		}
	}
	if(file->SetVariables2DYZ(vars)) {
		file->SetErrMsg(0,"Error in SetVariables2DYZ.");
		//cerr << "Error populating Variables." << endl;
		return file;
	}

	vector <double> usertime;
	for(int t = 0; t < wrfData->GetNumTimeSteps(); t++) {

		usertime.clear();
		usertime.push_back(wrfData->GetTSUserTime(t));
		if(file->SetTSUserTime(t, usertime)) {
			file->SetErrMsg(0,"Error in SetTSUserTime.");
			//cerr << "Error populating TSUserTime." << endl;
			return file;
		}

		string timestamp;
		wrfData->GetTSUserTimeStamp(t, timestamp);
		file->SetTSUserTimeStamp(t, timestamp);
	}

	string gridtype = wrfData->GetGridType();
	file->SetGridType(gridtype);

	//
	file->SetMapProjection(wrfData->GetMapProjection());

	return(file);
}

MetadataVDC *AppendMetadataVDC(
	const DCReaderWRF *wrfData,
	string metafile
) {
	MetadataVDC *file = new MetadataVDC(metafile);
	if (MetadataVDC::GetErrCode() != 0) return file;

	size_t tsVDC = file->GetNumTimeSteps();

	file->SetNumTimeSteps(file->GetNumTimeSteps() + wrfData->GetNumTimeSteps());

	string usertimestamp;
	vector <double> usertime;
	for(size_t tsWRF = 0; tsWRF < wrfData->GetNumTimeSteps(); tsWRF++, tsVDC++){

		wrfData->GetTSUserTimeStamp(tsWRF, usertimestamp);
		if(file->SetTSUserTimeStamp(tsVDC, usertimestamp)) {
			file->SetErrMsg(0,"Error populating TSUserTimeStamp.");
			return file;
		}
		usertime.clear();
		usertime.push_back(wrfData->GetTSUserTime(tsWRF));
		if(file->SetTSUserTime(tsVDC, usertime)) {
			file->SetErrMsg(0,"Error populating TSUserTime.");
			return file;
		}
		if(file->SetTSExtents(tsVDC, wrfData->GetExtents(tsWRF))) {
			file->SetErrMsg(0,"Error populating TSExtents.");
			return file;
		}
	}

	return(file);
}


char ** wrfvdfcreate::argv_merge(
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

int	wrfvdfcreate::launchVdfCreate(int argc, char **argv) {

	OptionParser::OptDescRec_T  set_opts[] = {
	    {"vars",1,    "",   "Colon delimited list of variables to be copied "
	        "from ncdf data. The default is to copy all 2D and 3D variables"},
	    {"dervars", 1,    "",   "Deprecated"},
	    {
	        "vdc2", 0,  "",
	        "Generate a VDC Type 2 .vdf file (default is VDC Type 1)"
	    },
	    {"append",  0,  "", "Append WRF files to an existing .vdfd"},
	    {"help",    0,  "", "Print this message and exit"},
	    {"quiet",   0,  "", "Operate quietly"},
	    {"debug",   0,  "", "Turn on debugging"},
	    {NULL}
	};

	OptionParser::Option_T  get_options[] = {
	    {"vars", VetsUtil::CvtToStrVec, &_vars, sizeof(_vars)},
	    {"dervars", VetsUtil::CvtToStrVec, &_dervars, sizeof(_dervars)},
	    {"vdc2", VetsUtil::CvtToBoolean, &_vdc2, sizeof(_vdc2)},
	    {"append", VetsUtil::CvtToBoolean, &_append, sizeof(_append)},
	    {"help", VetsUtil::CvtToBoolean, &_help, sizeof(_help)},
	    {"quiet", VetsUtil::CvtToBoolean, &_quiet, sizeof(_quiet)},
	    {"debug", VetsUtil::CvtToBoolean, &_debug, sizeof(_debug)},
	    {NULL}
	};

	for (size_t a=0; a<argc; a++){
		cout << argv[a] << " ";
	}
	cout << endl;

    MyBase::SetErrMsgFilePtr(stderr);

	OptionParser op;

	string s;
	DCReaderWRF *wrfData;

	_progname = Basename(argv[0]);

	if (op.AppendOptions(set_opts) < 0) {
		return (-1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		return (-1);
	}
	if (_debug) MyBase::SetDiagMsgFilePtr(stderr);

	VDCFactory vdcf(_vdc2);
	vector <string> rmopts;
	rmopts.push_back("nfilter");
	rmopts.push_back("nlifting");
	rmopts.push_back("varnames");
	rmopts.push_back("vars3d");
	rmopts.push_back("vars2dxy");
	rmopts.push_back("vars2dxz");
	rmopts.push_back("vars2dyz");
	rmopts.push_back("missing");
//	rmopts.push_back("level");
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
	rmopts.push_back("startt");
	rmopts.push_back("numts");

	vdcf.RemoveOptions(rmopts);
	if (vdcf.Parse(&argc, argv) < 0) {
		return (-1);
	}

	if (_help) {
		Usage(op, NULL);
		vdcf.Usage(stderr);
		return 0;
	}

    if (_dervars.size()) {
		MyBase::SetErrMsg("Option \"-dervars\" is deprecated");
    }

	argv++;
	argc--;

	if (argc < 2) {
		Usage(op, "No files to process");
		vdcf.Usage(stderr);
		return (-1);
	}

	vector<string> ncdffiles;
	for (int i=0; i<argc-1; i++) {
		 ncdffiles.push_back(argv[i]);
	}
	
	wrfData = new DCReaderWRF(ncdffiles);
	if (MyBase::GetErrCode() != 0) return (-1);

	if(wrfData->GetNumTimeSteps() < 0) {
		cerr << "No output file generated due to no input files processed." << endl;
		return 0;
	}

	//
	// Create a MetadataVDC object
	//
	MetadataVDC *file;
	file = CreateMetadataVDC(vdcf, wrfData);//,_vars);

	// Write file.
	if (file->Write(argv[argc-1]) < 0) {
		return (-1);
	}

	if (! _quiet && file->GetNumTimeSteps() > 0) {
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
		for (int i = 0; i < wrfData->GetVariables3DExcluded().size(); i++) {
			cout << wrfData->GetVariables3DExcluded()[i] << " ";
		}
		cout << endl;

		cout << "\tExcluded 2D Variable names : ";
		for (int i = 0; i < wrfData->GetVariables2DExcluded().size(); i++) {
			cout << wrfData->GetVariables2DExcluded()[i] << " ";
		}
		cout << endl;

		cout << "\tCoordinate extents : ";
		const vector <double> extptr = file->GetExtents();
		for(int i=0; i<6; i++) {
			cout << extptr[i] << " ";
		}
		cout << endl;
		
	} // End if quiet.

	return 0;
} // End of main.

/*int main(int argc, char **argv) {
    MyBase::SetErrMsgFilePtr(stderr);
    std::string command = "wrf";
    wrfvdfcreate launcher;
    if (launcher.launchVdfCreate(argc, argv) < 0) return (-1);
    return 0;
}*/
