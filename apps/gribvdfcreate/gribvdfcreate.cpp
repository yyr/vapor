
// Why can we compile if we include vapor/DCReaderROMS.h first?!
#include <vapor/DCReaderROMS.h>
#include <vapor/DCReaderGRIB.h>
#include <vapor/DCReader.h>
#include <vapor/WaveCodecIO.h>
#include <vapor/WaveletBlock3DBufWriter.h>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/VDCFactory.h>
#include <vapor/CFuncs.h>

using namespace VetsUtil;
using namespace VAPoR;

OptionParser op; 

MetadataVDC *CreateMetadataVDC(
    const VDCFactory &vdcf,
    DCReaderGRIB *GribData
) {

    size_t dims[3];
    GribData->GetGridDim(dims);

    //  
    // Create default MetadataVDC object based on command line
    // options
    //  
    MetadataVDC *file;
    file = vdcf.New(dims);
    if (MetadataVDC::GetErrCode() != 0) exit(1);

    // Copy values over from DCReaderNCDF to MetadataVDC.
    // Add checking of return values and error messsages.
    //  
    if(file->SetNumTimeSteps(GribData->GetNumTimeSteps())) {
        cerr << "Error populating NumTimeSteps." << endl;
        exit(1);
    }

	file->SetExtents(GribData->GetExtents());

	file->SetMapProjection(GribData->GetMapProjection());

    //  
    //  
    //if (opt.missattr.size() && !opt.misstv) {
    //    file->SetMissingValue(1e37);
    //}   

    //  
    // By default we populate the VDC with all 2D and 3D variables found in 
    // the netCDF files. This can be overriden by the "-vars" option
    //  

    vector <string> candidate_vars, vars;
    candidate_vars = GribData->GetVariables3D();
    vars = candidate_vars;

    if(file->SetVariables3D(vars)) {
        cerr << "Error populating Variables." << endl;
        exit(1);
    }

    candidate_vars = GribData->GetVariables2DXY();
    vars = candidate_vars;
    if(file->SetVariables2DXY(vars)) {
        cerr << "Error populating Variables." << endl;
        exit(1);
    }

    candidate_vars = GribData->GetVariables2DXZ();
    vars = candidate_vars;
    if(file->SetVariables2DXZ(vars)) {
        cerr << "Error populating Variables." << endl;
        exit(1);
    }

    candidate_vars = GribData->GetVariables2DYZ();
    vars = candidate_vars;
    if(file->SetVariables2DYZ(vars)) {
        cerr << "Error populating Variables." << endl;
        exit(1);
    }

    /*if (opt.missattr.size() && opt.misstv) {
        vars = file->GetVariableNames();
        for(int i = 0; i < GribData->GetNumTimeSteps(); i++) {

            for (int j=0; j<vars.size(); j++) {
                file->SetVMissingValue(i,vars[j], 1e37);
            }   
        }   
    }*/

    vector <double> usertime;
    for(int i = 0; i < GribData->GetNumTimeSteps(); i++) {

        usertime.clear();
        usertime.push_back(GribData->GetTSUserTime(i));
        if(file->SetTSUserTime(i, usertime)) {
            cerr << "Error populating TSUserTime." << endl;
            exit(1);
        }
    }

	string gridType = GribData->GetGridType();
	file->SetGridType(gridType);

    return(file);
}

int CopyVar(
    VDFIOBase *vdfio,
    DCReaderGRIB *gribData,
    int vdcTS,
    int gribTS,
    string vdcVar,
    string gribVar,
    int level,
    int lod
) {
    if (gribData->OpenVariableRead(gribTS, gribVar,NULL,NULL) < 0) {
        MyBase::SetErrMsg(
            "Failed to open GRIB variable \"%s\" at time step %d",
            gribVar.c_str(), gribTS
        );
        return(-1);
    }

    if (vdfio->OpenVariableWrite(vdcTS, vdcVar.c_str(), level, lod) < 0) {
        MyBase::SetErrMsg(
            "Failed to open VDC variable \"%s\" at time step %d",
            vdcVar.c_str(), vdcTS
        );
        return(-1);
    }

    size_t dim[3];
    vdfio->GetDim(dim, -1);
    float *buf = new float [dim[0]*dim[1]];

    int rc = 0;
    VDFIOBase::VarType_T vtype = vdfio->GetVarType(vdcVar);
    int n = vtype == Metadata::VAR3D ? dim[2] : 1;
    for (int i=0; i<n; i++) {
        rc = gribData->ReadSlice(buf);
        if (rc==0) {
            MyBase::SetErrMsg(
                "Short read of variable \"%s\" at time step %d",
                gribVar.c_str(), gribTS
            );
            rc = -1;    // Short read is an error
            break;
        }
        if (rc<0) {
            MyBase::SetErrMsg(
                "Error reading GRIB variable \"%s\" at time step %d",
                gribVar.c_str(), gribTS
            );
            break;
        }
        //if (opt.missattr.length()) {
        //    MissingValue(vdfio, ncdfData, vdcVar, ncdfVar, vtype, buf);
        //}   

        /*float min = buf[0];
        float max = buf[0];
        for (size_t j=0; j<dim[0]*dim[1];j++){
            if (buf[j] > max) max = buf[j];
            if (buf[j] < min) min = buf[j];
        }*/

        rc = vdfio->WriteSlice(buf);
        if (rc<0) {
            cout << "WriteSlice(buf) error" << endl;
            MyBase::SetErrMsg(
                "Error writing VDC variable \"%s\" at time step %d",
                gribVar.c_str(), gribTS
            );
            break;
        }
    }

	gribData->CloseVariable();

    if (buf) delete [] buf;
    vdfio->CloseVariable();

    return(rc);
}

void Usage(const char * msg) {
	if (msg) {
		cerr << "gribvdfcreate: " << msg << endl;
	}
	cerr << "Usage: gribvdfcreate [options] grib_file(s)... vdf_file" << endl;
	op.PrintOptionHelp(stderr, 80, false);
    exit(1);
}

void writeToScreen(DCReader *DCdata, MetadataVDC *file) {
    if (DCdata->GetNumTimeSteps() > 0) {
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

int main(int argc, char** argv) {
    
	string gribfile;
	//string vdfname;

    //vector <string> files;
    //for (int i=2; i<argc; i++){
    //    files.push_back(argv[i]);
    //}   

//    if (argc<3) Usage(argv[0]);
    
	//vdfname = argv[1];

	
	//
	// command line options passed to launchVdfCreate
	//
	vector <string> vars;
	VetsUtil::OptionParser::Boolean_T help=false;
	VetsUtil::OptionParser::Boolean_T quiet=false;
	VetsUtil::OptionParser::Boolean_T debug=false;
	string progname = argv[0];

    OptionParser::OptDescRec_T  set_opts[] = { 
        {"vars",1,    "",   "Colon delimited list of variables to be copied "
            "from ncdf data. The default is to copy all 2D and 3D variables"},
        {"help",    0,  "", "Print this message and exit"},
        {"quiet",   0,  "", "Operate quietly"},
        {"debug",   0,  "", "Turn on debugging"},
        {NULL}
    };  

    OptionParser::Option_T  get_options[] = { 
        {"vars", VetsUtil::CvtToStrVec, &vars, sizeof(vars)},
        {"help", VetsUtil::CvtToBoolean, &help, sizeof(help)},
        {"quiet", VetsUtil::CvtToBoolean, &quiet, sizeof(quiet)},
        {"debug", VetsUtil::CvtToBoolean, &debug, sizeof(debug)},
        {NULL}
    }; 

	if (op.AppendOptions(set_opts) < 0) {
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		cout << "Error parsing arguments" << endl;
		MyBase::SetErrMsg("Error parsing arguments");
		exit(1);
	}

	VDCFactory vdcf(1);
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
		cout << "Error parsing arguments" << endl;
		MyBase::SetErrMsg("Error parsing arguments");
		exit(1);
	}

	if (debug) MyBase::SetDiagMsgFilePtr(stderr);

	if (help) {
		Usage("");
		vdcf.Usage(stderr);
		exit(0);
	}

	argv++;
	argc--;

	if (argc < 2) {
		Usage("No files to process");
		vdcf.Usage(stderr);
		MyBase::SetErrMsg("No files to process");
		exit(1);
	}

	vector<string> gribfiles;
	for(int i=0; i<argc-1; i++){
		gribfiles.push_back(argv[i]);
	}

	// Create DCReaderGRIB with record info
	DCReaderGRIB *DCGrib = new DCReaderGRIB(gribfiles);

    // Create VDF file      
    //VDCFactory vdcf(1);
    MetadataVDC *file = CreateMetadataVDC(vdcf, DCGrib);
    file->Write(argv[argc-1]);

	writeToScreen(DCGrib, file);
	if (DCGrib) delete DCGrib;

    // Create VDC
/*    WaveCodecIO *wcwriter;
    MetadataVDC metadataVDC(vdfname);
    wcwriter = new WaveCodecIO(metadataVDC,1);  
    VDFIOBase *vdfio = NULL;
    vdfio = wcwriter;

	//std::vector<string> vars;
	std::vector<string> vars3D = DCGrib->GetVariables3D();
	std::vector<string> vars2D = DCGrib->GetVariables2DXY();

	for (int i=0; i<vars3D.size(); i++) {
		vars.push_back(vars3D[i]);
	}

	//DCGrib->Print2dVars();

	for (int i=0; i<vars2D.size(); i++) {
		vars.push_back(vars2D[i]);
	}

	int numTs = (int) DCGrib->GetNumTimeSteps();
    
	int rc;
	string var;

	for (int varNum=0; varNum<vars.size(); varNum++) {
		var = vars[varNum];
		for(int ts=0; ts<numTs; ts++) {
			cout << "Processing variable " << var << " at timestep " << ts << endl;
			rc = CopyVar(vdfio,
	    	             DCGrib,
	        	         ts,ts,var,var,-1,-1);
		}
	}

	if (DCGrib) delete DCGrib;
	if (wcwriter) delete wcwriter;
*/
    return 0;
}
