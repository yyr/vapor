#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sstream>
#include <algorithm>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/DCReaderMOM.h>
#include <vapor/DCReaderROMS.h>
#include <vapor/VDCFactory.h>
#include <vapor/CFuncs.h>
#include <vapor/vdfcreate.h>

#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace std;
using namespace VAPoR;

struct opt_t {
	vector <string> vars;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
} optvdfcreate;

OptionParser::OptDescRec_T	set_optsvdfcreate[] = {
	{"vars",1,    "",	"Colon delimited list of variables to be copied "
		"from ncdf data. The default is to copy all 2D and 3D variables"},
	{"help",	0,	"",	"Print this message and exit"},
	{"quiet",	0,	"",	"Operate quietly"},
	{NULL}
};

OptionParser::Option_T	get_optionsvdfcreate[] = {
    {"vars", VetsUtil::CvtToStrVec, &optvdfcreate.vars, sizeof(optvdfcreate.vars)},
    {"help", VetsUtil::CvtToBoolean, &optvdfcreate.help, sizeof(optvdfcreate.help)},
    {"quiet", VetsUtil::CvtToBoolean, &optvdfcreate.quiet, sizeof(optvdfcreate.quiet)},
	{NULL}
};

const char *vdfcreateProgName;

void vdfcreateUsage(OptionParser &op, const char * msg) {

	if (msg) {
        cerr << vdfcreateProgName << " : " << msg << endl;
	}
    cerr << "Usage: " << vdfcreateProgName << " [options] ncdf_file... vdf_file" << endl;
	op.PrintOptionHelp(stderr, 80, false);
}

void populateVariables(vector<string> &vars, vector<string> candidate_vars,
                       MetadataVDC *file, int (MetadataVDC::*SetVarFunction)(const vector<string>&)) {
    if (! optvdfcreate.vars.size()) {
            vars = candidate_vars;
        }
        else {

            vars.clear();
            for (int i=0; i<optvdfcreate.vars.size(); i++) {
                vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), optvdfcreate.vars[i]);
                if (itr != candidate_vars.end()) {
                    vars.push_back(optvdfcreate.vars[i]);
                }
            }
        }
    
    if((file->*SetVarFunction)(vars)) {
        //cerr << "Error populating Variables." << endl;
        file->SetErrMsg(1,"Error populating Variables."); 
		//exit(1);
    }
}

void writeToScreen(DCReader *DCdata, MetadataVDC *file) {
    if (! optvdfcreate.quiet && DCdata->GetNumTimeSteps() > 0) {
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
        for (int i = 0; i < DCdata->GetVariables3DExcluded().size(); i++) {
            cout << DCdata->GetVariables3DExcluded()[i] << " ";
        }
        cout << endl;

        cout << "\tExcluded 2D Variable names : ";
        for (int i = 0; i < DCdata->GetVariables2DExcluded().size(); i++) {
            cout << DCdata->GetVariables2DExcluded()[i] << " ";
        }
        cout << endl;

        cout << "\tCoordinate extents : ";
        const vector <double> extptr = file->GetExtents();
        for(int i=0; i<6; i++) {
            cout << extptr[i] << " ";
        }
        cout << endl;

    }
}

MetadataVDC *CreateMetadataVDC(
	const VDCFactory &vdcf,
    const DCReader *DCdata
) {

    size_t dims[3];
    DCdata->GetGridDim(dims);

	//
	// Create default MetadataVDC object based on command line
	// options
	//
	MetadataVDC *file;
	file = vdcf.New(dims);
	if (MetadataVDC::GetErrCode() != 0) return file;//exit(1);

	if (file->GetVDCType() == 1) {
		//cerr << "VDC Type 1 not supported\n";
		file->SetErrMsg(1,"VDC Type 1 not supported");
		return file;
		//exit(1);
	}

	// Copy values over from DCReaderMOM to MetadataVDC.
	// Add checking of return values and error messsages.
	//
    if(file->SetNumTimeSteps(DCdata->GetNumTimeSteps())) {
		cerr << "Error populating NumTimeSteps." << endl;
		file->SetErrMsg(2,"Error populating NumTimeSteps.");
		return file;
		//exit(1);
	}

    file->SetExtents(DCdata->GetExtents());

	//
	// By default we populate the VDC with all 2D and 3D variables found in 
	// the netCDF files. This can be overriden by the "-vars" option
	// 
	vector <string> candidate_vars, vars;
    candidate_vars = DCdata->GetVariables3D();
    populateVariables(vars,candidate_vars,file,&MetadataVDC::SetVariables3D);
    candidate_vars = DCdata->GetVariables2DXY();
    populateVariables(vars,candidate_vars,file,&MetadataVDC::SetVariables2DXY);
    candidate_vars = DCdata->GetVariables2DXZ();
    populateVariables(vars,candidate_vars,file,&MetadataVDC::SetVariables2DXZ);
    candidate_vars = DCdata->GetVariables2DYZ();
    populateVariables(vars,candidate_vars,file,&MetadataVDC::SetVariables2DYZ);


    vars = DCdata->GetVariableNames();
    for(int t = 0; t < DCdata->GetNumTimeSteps(); t++) {
		for (int j=0; j<vars.size(); j++) {
			float mv;
            bool has_missing = DCdata->GetMissingValue(vars[j], mv);
			if (has_missing) file->SetVMissingValue(t,vars[j], 1e37);
		}
	}


	vector <double> usertime;
    for(int t = 0; t < DCdata->GetNumTimeSteps(); t++) {

		usertime.clear();
        usertime.push_back(DCdata->GetTSUserTime(t));
		if(file->SetTSUserTime(t, usertime)) {
			cerr << "Error populating TSUserTime." << endl;
			file->SetErrMsg(1,"Error populating TSUserTime.");
			return file;//exit(1);
		}

		string timestamp;
        DCdata->GetTSUserTimeStamp(t, timestamp);
		file->SetTSUserTimeStamp(t, timestamp);
	}

    string gridtype = DCdata->GetGridType();
	file->SetGridType(gridtype);
	if (gridtype.compare("stretched") == 0) {
        for(size_t ts = 0; ts < DCdata->GetNumTimeSteps(); ts++) {
            int rc = file->SetTSZCoords(ts, DCdata->GetTSZCoords(ts));
			if (rc<0) {
            	file->SetErrMsg(1,"SetTSZCoords() failed.");
		    	return file;//exit(1);
			}
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

int launchVdfCreate(int argc, char **argv, string NetCDFtype) {

    for(int i=0;i<argc;i++) cout << argv[i] << " ";

    //not for production - ok for command line
    //MyBase::SetErrMsgFilePtr(stderr);

    OptionParser op;

    string s;
    DCReader *DCdata;

    vdfcreateProgName = Basename(argv[0]);

    // Ugh. All this just to add a default option to argv
    //
    char **myargv;
    int myargc;
    char *argv2[] = { (char *) "-vdc2", NULL };
    myargv = argv_merge(argc, argv, 1, argv2, myargc);


    if (op.AppendOptions(set_optsvdfcreate) < 0) {
	    return 1;
	}

    if (op.ParseOptions(&myargc, myargv, get_optionsvdfcreate) < 0) {
	    return 1;
	}

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
	    return 1;
	    //exit(1);
	}

    if (optvdfcreate.help) {
        vdfcreateUsage(op, NULL);
        vdcf.Usage(stderr);
	return 0;//exit(0);
	}


	myargv++;
	myargc--;

	if (myargc < 2) {
        vdfcreateUsage(op, "No files to process");
        vdcf.Usage(stderr);
	MyBase::SetErrMsg("No files to process");
	return 1;//exit(1);
	}

	vector<string> ncdffiles;
	for (int i=0; i<myargc-1; i++) {
		 ncdffiles.push_back(myargv[i]);
	}
	
    if (NetCDFtype == "roms") DCdata = new DCReaderROMS(ncdffiles);
    else DCdata = new DCReaderMOM(ncdffiles);

    //DCdata = new DCReaderMOM(ncdffiles);
    if (MyBase::GetErrCode() != 0) return 1;

    if(DCdata->GetNumTimeSteps() < 0) {
		cerr << "No output file generated due to no input files processed." << endl;
		MyBase::SetErrMsg("No output file generated due to no input files processed.");
		return 0;
	}

	//
	// Create a MetadataVDC object
	//
	MetadataVDC *file;
    file = CreateMetadataVDC(vdcf, DCdata);

	// Write file.
	if (file->Write(myargv[myargc-1]) < 0) {
	    //MyBase::SetErrMsg already called here?
	    return 1;//file->GetErrMsg();	
	    //exit(1);
	}

    writeToScreen(DCdata,file);
    return 0;
}
