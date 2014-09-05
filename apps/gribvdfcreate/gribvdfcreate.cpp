
// Why can we compile if we include vapor/DCReaderROMS.h first?!
#include <vapor/DCReaderROMS.h>
#include <vapor/GribParser.h>
#include <vapor/DCReader.h>
#include <vapor/WaveCodecIO.h>
#include <vapor/WaveletBlock3DBufWriter.h>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/VDCFactory.h>
#include <vapor/CFuncs.h>

using namespace VetsUtil;
using namespace VAPoR;

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
    //if (! opt.vars.size()) {
    vars = candidate_vars;
    //}   
    /*else {

        vars.clear();
        for (int i=0; i<opt.vars.size(); i++) {
            vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), opt.vars[i]);
            if (itr != candidate_vars.end()) {
                vars.push_back(opt.vars[i]);
            }   
        }   
    } */

    if(file->SetVariables3D(vars)) {
        cerr << "Error populating Variables." << endl;
        exit(1);
    }

    candidate_vars = GribData->GetVariables2DXY();
    //if (! opt.vars.size()) {
        vars = candidate_vars;
    //}   
    /*else {

        vars.clear();
        for (int i=0; i<opt.vars.size(); i++) {
            vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), opt.vars[i]);
            if (itr != candidate_vars.end()) {
                vars.push_back(opt.vars[i]);
            }   
        }   
    } */
    if(file->SetVariables2DXY(vars)) {
        cerr << "Error populating Variables." << endl;
        exit(1);
    }

    candidate_vars = GribData->GetVariables2DXZ();
    //if (! opt.vars.size()) {
        vars = candidate_vars;
    //}   
    /*else {

        vars.clear();
        for (int i=0; i<opt.vars.size(); i++) {
            vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), opt.vars[i]);
            if (itr != candidate_vars.end()) {
                vars.push_back(opt.vars[i]);
            }   
        }   
    } */
    if(file->SetVariables2DXZ(vars)) {
        cerr << "Error populating Variables." << endl;
        exit(1);
    }

    candidate_vars = GribData->GetVariables2DYZ();
    //if (! opt.vars.size()) {
        vars = candidate_vars;
    //}   
    /*else {

        vars.clear();
        for (int i=0; i<opt.vars.size(); i++) {
            vector <string>::iterator itr = find(candidate_vars.begin(), candidate_vars.end(), opt.vars[i]);
            if (itr != candidate_vars.end()) {
                vars.push_back(opt.vars[i]);
            }   
        }   
    } */
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

    string gridtype = file->GetGridType();

    if (gridtype.compare("layered")==0) {
        vector <string> vec;
        vec.push_back("NONE");
        vec.push_back("NONE");
//        vec.push_back(opt.zcoordvar);
        file->SetCoordinateVariables(vec);
    }
/*    else if (gridtype.compare("stretched")==0) {
        NetCDFCollection *ncdfc = GribData->GetNetCDFCollection();
        for(size_t ts = 0; ts < GribData->GetNumTimeSteps(); ts++) {
            vector <double> coords;
            if (! (opt.xcoordvar.compare("NONE")==0)) {
                GetCoordinates(ncdfc, ts, opt.xcoordvar, dims[0], coords);
            }   
            else {
                // Ugh. Stupid hack to fix bug #1007. The VDCFactory
                // object parses the -{x,y,z}coords option and sets 
                // the coordinates, but only for the first time step.
                // Need to grab coordinates from ts=0, and set the rest of
                // the time steps
                //  
                coords = file->GetTSXCoords(0);
            }   
            int rc = file->SetTSXCoords(ts, coords);
            if (rc<0) exit(1);

            if (! (opt.ycoordvar.compare("NONE")==0)) {
                GetCoordinates(ncdfc, ts, opt.ycoordvar, dims[0], coords);
            }   
            else {
                coords = file->GetTSYCoords(0);
            }   
            rc = file->SetTSYCoords(ts, coords);
            if (rc<0) exit(1);

            if (! (opt.zcoordvar.compare("NONE")==0)) {
                GetCoordinates(ncdfc, ts, opt.zcoordvar, dims[0], coords);
            }   
            else {
                coords = file->GetTSZCoords(0);
            }   
            rc = file->SetTSZCoords(ts, coords);
            if (rc<0) exit(1);

        }   
    }*/

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

    cout << vdfio->GetDataRange()[0] << " " << vdfio->GetDataRange()[1] << endl;

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
                "Error reading netCDF variable \"%s\" at time step %d",
                gribVar.c_str(), gribTS
            );
            break;
        }
        //if (opt.missattr.length()) {
        //    MissingValue(vdfio, ncdfData, vdcVar, ncdfVar, vtype, buf);
        //}   

        float min = buf[0];
        float max = buf[0];
        for (size_t j=0; j<dim[0]*dim[1];j++){
            if (buf[j] > max) max = buf[j];
            if (buf[j] < min) min = buf[j];
        }

        cout << min << " " << max << " ";
        cout << vdfio->GetDataRange()[0] << " " << vdfio->GetDataRange()[1] << endl;

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

    //size_t min[3];
    //size_t max[3];
    //int reflevel = 3;
    //vdfio->GetValidRegion(&min, &max, reflevel);

    if (buf) delete [] buf;
    //ncdfData->CloseVariable();
    vdfio->CloseVariable();
    cout << "MyBase::GetErrCode(): " << MyBase::GetErrCode() << endl;
    return(rc);

}

void usage(char* prog) {
    printf("usage: %s _vdfFileName _gribFiles\n",prog);
    exit(1);
}

int main(int argc, char** argv) {
    //int err = 0,i;
    //double *values = NULL;
    //double max,min,average;
    //size_t values_len= 0;
 
    //FILE* in = NULL;
    string gribfile;
	string vdfname;

    GribParser *parser = new GribParser();

    if (argc<3) usage(argv[0]);
    
	vdfname = argv[1];

    for (int i=2; i<argc; i++){
        gribfile = argv[i];
        parser->_LoadRecordKeys(gribfile);
    }    

    parser->_VerifyKeys();
    parser->_InitializeDCReaderGRIB();  

    // Create VDF file      
    DCReaderGRIB *metadata = parser->GetDCReaderGRIB();
    VDCFactory vdcf(1);
    MetadataVDC *file = CreateMetadataVDC(vdcf, metadata);
    file->Write(vdfname);


    // Create VDC
    WaveCodecIO *wcwriter;
    MetadataVDC metadataVDC(vdfname);
    wcwriter = new WaveCodecIO(metadataVDC,1);  
    VDFIOBase *vdfio = NULL;
    vdfio = wcwriter;

    int rc = CopyVar(vdfio,
                    metadata,
                    0,0,"u","u",-1,-1);

    cout << rc << endl;
    const float * drange = vdfio->GetDataRange();
    cout << "data range (" << drange[0] << ", " << drange[1] << ")" << endl;

    metadata->Print3dVars();

    delete parser;
    return 0;

}
