#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sstream>
#include <algorithm>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/DCReaderGRIB.h>
#include <vapor/DCReaderMOM.h>
#include <vapor/DCReaderROMS.h>
#include <vapor/DCReaderWRF.h>
#include <vapor/VDCFactory.h>
#include <vapor/CFuncs.h>
#include <vapor/vdfcreate.h>

#ifdef WIN32
#pragma warning(disable : 4996)
#endif

using namespace std;
using namespace VAPoR;
using namespace VetsUtil;


vdfcreate::vdfcreate() {
	_progname.clear();
	_vars.clear();
	_help = false;
	_quiet = false;
	_debug = false;
}

vdfcreate::~vdfcreate() {
}

void vdfcreate::Usage(OptionParser &op, const char * msg) {

	if (msg) {
        cerr << _progname << " : " << msg << endl;
	}
    cerr << "Usage: " << _progname << " [options] ncdf_file... vdf_file" << endl;
	op.PrintOptionHelp(stderr, 80, false);
}

void vdfcreate::populateVariables(
	vector <string> cml_vars, vector<string> candidate_vars,
	MetadataVDC *file, int (MetadataVDC::*SetVarFunction)(const vector<string>&)) {
	vector <string> vars;
    if (! cml_vars.size()) {
		vars = candidate_vars;
	}
    else {
        for (int i=0; i<cml_vars.size(); i++) {
            vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), cml_vars[i]);
            if (itr != candidate_vars.end()) {
                vars.push_back(cml_vars[i]);
            }
        }
    }
    
    if((file->*SetVarFunction)(vars)<0) {
        file->SetErrMsg(1,"Error populating Variables."); 
    }
}

void vdfcreate::writeToScreen(DCReader *DCdata, MetadataVDC *file) {
    if (! _quiet && DCdata->GetNumTimeSteps() > 0) {
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
		double lon[2], lat[2];
		DCdata->GetLatLonExtents(0, lon, lat);
		cout << "\tMin Longitude and Latitude : " <<lon[0]<< " " <<lat[0]<< endl;
		cout << "\tMax Longitude and Latitude : " <<lon[1]<< " " <<lat[1]<< endl;

    }
}

MetadataVDC *vdfcreate::CreateMetadataVDC(
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
	if (MetadataVDC::GetErrCode() != 0) return (NULL);

	if (file->GetVDCType() == 1) {
		//cerr << "VDC Type 1 not supported\n";
		file->SetErrMsg(1,"VDC Type 1 not supported");
		return (NULL);
	}

	// Copy values over from DCReader to MetadataVDC.
	// Add checking of return values and error messsages.
	//
	int numTimeSteps;
    if (_numTS == -1) numTimeSteps = DCdata->GetNumTimeSteps();
	else numTimeSteps = _numTS;
	if(file->SetNumTimeSteps(numTimeSteps)) {
		file->SetErrMsg(2,"Error populating NumTimeSteps.");
		return (NULL);
		//exit(1);
	}

    vector <double> usertime;
	double delta = DCdata->GetTSUserTime(1) - DCdata->GetTSUserTime(0);
    for(int t = 0; t < numTimeSteps; t++) {

        usertime.clear();
        //usertime.push_back(DCdata->GetTSUserTime(t));
        usertime.push_back(DCdata->GetTSUserTime(0) + delta*t);
		if(file->SetTSUserTime(t, usertime)) {
            file->SetErrMsg(1,"Error populating TSUserTime.");
            return (NULL);
        }   
		cout << DCdata->GetTSUserTime(0) << " " << delta << " " << usertime[0] << endl;
        string timestamp;
        DCdata->GetTSUserTimeStamp(t, timestamp);
        file->SetTSUserTimeStamp(t, timestamp);
    } 

    file->SetExtents(DCdata->GetExtents());

	//
	// By default we populate the VDC with all 2D and 3D variables found in 
	// the netCDF files. This can be overriden by the "-vars" option
	// 
	vector <string> candidate_vars;
    candidate_vars = DCdata->GetVariables3D();
    populateVariables(_vars,candidate_vars,file,&MetadataVDC::SetVariables3D);
    candidate_vars = DCdata->GetVariables2DXY();
    populateVariables(_vars,candidate_vars,file,&MetadataVDC::SetVariables2DXY);
    candidate_vars = DCdata->GetVariables2DXZ();
    populateVariables(_vars,candidate_vars,file,&MetadataVDC::SetVariables2DXZ);
    candidate_vars = DCdata->GetVariables2DYZ();
    populateVariables(_vars,candidate_vars,file,&MetadataVDC::SetVariables2DYZ);


    vector <string> vars = file->GetVariableNames();
    for(int t = 0; t < DCdata->GetNumTimeSteps(); t++) {
		for (int j=0; j<vars.size(); j++) {
			float mv;
            bool has_missing = DCdata->GetMissingValue(vars[j], mv);
			if (has_missing){
				file->SetVMissingValue(t,vars[j], 1e37);
			}
		}
	}

    string gridtype = DCdata->GetGridType();
	file->SetGridType(gridtype);
	if (gridtype.compare("stretched") == 0) {
        for(size_t ts = 0; ts < DCdata->GetNumTimeSteps(); ts++) {
            int rc = file->SetTSZCoords(ts, DCdata->GetTSZCoords(ts));
			if (rc<0) {
            	file->SetErrMsg(1,"SetTSZCoords() failed.");
		    	return (NULL);
			}
	    }
	}

	file->SetMapProjection(DCdata->GetMapProjection());

	return(file);
}

int vdfcreate::launchVdfCreate(int argc, char **argv, string NetCDFtype) {

	OptionParser::OptDescRec_T	set_opts[] = {
		{"vars",1,    "",	"Colon delimited list of variables to be copied "
			"from ncdf data. The default is to copy all 2D and 3D variables"},
		{"help",	0,	"",	"Print this message and exit"},
		{"quiet",	0,	"",	"Operate quietly"},
		{"debug",   0,  "", "Turn on debugging"},
		{"fastMode",1,  "-1", "Enable fast mode"},
		{NULL}
	};

	OptionParser::Option_T	get_options[] = {
		{"vars", VetsUtil::CvtToStrVec, &_vars, sizeof(_vars)},
		{"help", VetsUtil::CvtToBoolean, &_help, sizeof(_help)},
		{"quiet", VetsUtil::CvtToBoolean, &_quiet, sizeof(_quiet)},
		{"debug", VetsUtil::CvtToBoolean, &_debug, sizeof(_debug)},
		{"fastMode", VetsUtil::CvtToInt, &_numTS, sizeof(_numTS)},
		{NULL}
	};

    //not for production - ok for command line
    //MyBase::SetErrMsgFilePtr(stderr);

    OptionParser op;

    string s;
    DCReader *DCdata;

    _progname = Basename(argv[0]);

    if (op.AppendOptions(set_opts) < 0) {
	    return (-1);
	}

    if (op.ParseOptions(&argc, argv, get_options) < 0) {
	    return (-1);
	}

	VDCFactory vdcf(true);
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
	    //exit(1);
	}
    if (_debug) MyBase::SetDiagMsgFilePtr(stderr);


    if (_help) {
		Usage(op, NULL);
		vdcf.Usage(stderr);
		return 0;//exit(0);
	}

	cout << _numTS << endl;

	argv++;
	argc--;

	if (argc < 2) {
        Usage(op, "No files to process");
        vdcf.Usage(stderr);
		MyBase::SetErrMsg("No files to process");
		return (-1);
	}

	vector<string> ncdffiles;
	for (int i=0; i<argc-1; i++) {
		 ncdffiles.push_back(argv[i]);
	}
	
    if (NetCDFtype == "ROMS") DCdata = new DCReaderROMS(ncdffiles);
    else if (NetCDFtype == "GRIMs") {
		if (_numTS != -1){
			vector<string> twoFiles;
			twoFiles.push_back(ncdffiles[0]);
			twoFiles.push_back(ncdffiles[1]);
			DCdata = new DCReaderGRIB(twoFiles);
		}
		else {
			DCdata = new DCReaderGRIB(ncdffiles);
		}
	}
	else if (NetCDFtype == "WRF") {
		DCdata = new DCReaderWRF(ncdffiles);
	}
	else DCdata = new DCReaderMOM(ncdffiles);

    if (MyBase::GetErrCode() != 0) return (-1);

    if(DCdata->GetNumTimeSteps() < 0) {
		MyBase::SetErrMsg("No output file generated due to no input files processed.");
		return 0;
	}

	//
	// Create a MetadataVDC object
	//
	MetadataVDC *file;
    file = CreateMetadataVDC(vdcf, DCdata);
	if (! file) return(-1);

	// Write file.
	if (file->Write(argv[argc-1]) < 0) {
	    //MyBase::SetErrMsg already called here?
	    return (-1);//file->GetErrMsg();	
	    //exit(1);
	}

    writeToScreen(DCdata,file);

	if (DCdata) delete DCdata;
    return 0;
}
