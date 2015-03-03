#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sstream>
#include <algorithm>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/DCReaderGRIB.h>
#include <vapor/DCReaderROMS.h>
#include <vapor/DCReaderMOM.h>
#include <vapor/DCReaderWRF.h>
#include <vapor/DCReader.h>
#include <vapor/WaveCodecIO.h>
#include <vapor/WaveletBlock3DBufWriter.h>
#include <vapor/CFuncs.h>
#include <vapor/Copy2VDF.h>

#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace VetsUtil;
using namespace VAPoR;

Copy2VDF::Copy2VDF() {
	_progname.clear();
	_vars.clear();
	_numts = -1;
	_startts = 0;
	_level = 0;
	_lod = 0;
	_nthreads = 0;
	_help = false;
	_quiet = false;
	_debug = false;

	_mvMask3D = NULL;
	_mvMask2DXY = NULL;
	_mvMask2DXZ = NULL;
	_mvMask2DYZ = NULL;

	wcwriter = NULL;
	DCData = NULL;
}

Copy2VDF::~Copy2VDF() {
	if (_mvMask3D) delete []  _mvMask3D;
	if (_mvMask2DXY) delete [] _mvMask2DXY;
	if (_mvMask2DXZ) delete [] _mvMask2DXZ;
	if (_mvMask2DYZ) delete [] _mvMask2DYZ;
}

void Copy2VDF::Usage(OptionParser &op, const char * msg) {

	if (msg) {
        cerr << _progname << " : " << msg << endl;
	}
    cerr << "Usage: " << _progname << " [options] input_file(s)... vdf_file" << endl;
	op.PrintOptionHelp(stderr, 80, false);

}


//
// Generate a map between VDC time steps and time steps in the DCReader
// i.e. ncdfTimeStep = timemap[vdcTimeStep]
//
void Copy2VDF::GetTimeMap(
	const VDFIOBase *vdfio,
	const DCReader *DCData,
	int startts,
	int numts,
	map <size_t, size_t> &timemap
) {
	timemap.clear();


	//
	// Get user times from .vdf
	//
	//vector <float> ncdftimes;
	vector <double> ncdftimes;
	for (int i=0; i<DCData->GetNumTimeSteps(); i++) { 
		ncdftimes.push_back(DCData->GetTSUserTime(i));
	}

	//
	// Get user times from netCDF files
	//
	//vector <float> vdctimes;
	vector <double> vdctimes;
	for (int i=0; i<vdfio->GetNumTimeSteps(); i++) { 
		vdctimes.push_back(vdfio->GetTSUserTime(i));
	}

	// If numts<0 then use all ncdf times
	//
	numts = numts < 0 ? DCData->GetNumTimeSteps() : numts;

	//
	// Cross-reference ncdf and vdc times
	//
	double ncdftime; 
	for (int i=startts; i<numts+startts && i<DCData->GetNumTimeSteps(); i++) {
		ncdftime = ncdftimes[i];

		vector <double>::iterator itr = find(
			vdctimes.begin(), vdctimes.end(), ncdftime
		);

		if (itr != vdctimes.end()) {
			timemap[itr-vdctimes.begin()] = i;
		}
	}
}
	
void Copy2VDF::GetVariables(
	const VDFIOBase *vdfio,
	const DCReader *DCData,
	const vector <string> &in_varnames,
	vector <string> &out_varnames
) {

	out_varnames.clear();

	//
	// If variable names provided on the command line we use them
	//
	if (in_varnames.size()) {
		out_varnames = in_varnames;
		return;
	} 

	//
	// Find intersection between variables found in the VDC and the
	// netCDF collection
	//
	vector <string> vdcvars;	// all the VDC vars
	for (int i=0; i<vdfio->GetVariables3D().size(); i++) {
		vdcvars.push_back(vdfio->GetVariables3D()[i]);
	}
	for (int i=0; i<vdfio->GetVariables2DXY().size(); i++) {
		vdcvars.push_back(vdfio->GetVariables2DXY()[i]);
	}
	for (int i=0; i<vdfio->GetVariables2DXZ().size(); i++) {
		vdcvars.push_back(vdfio->GetVariables2DXZ()[i]);
	}
	for (int i=0; i<vdfio->GetVariables2DYZ().size(); i++) {
		vdcvars.push_back(vdfio->GetVariables2DYZ()[i]);
	}

	vector <string> ncdfvars;	// all the netCDF vars
	for (int i=0; i<DCData->GetVariables3D().size(); i++) {
		ncdfvars.push_back(DCData->GetVariables3D()[i]);
	}
	for (int i=0; i<DCData->GetVariables2DXY().size(); i++) {
		ncdfvars.push_back(DCData->GetVariables2DXY()[i]);
	}
	for (int i=0; i<DCData->GetVariables2DXZ().size(); i++) {
		ncdfvars.push_back(DCData->GetVariables2DXZ()[i]);
	}
	for (int i=0; i<DCData->GetVariables2DYZ().size(); i++) {
		ncdfvars.push_back(DCData->GetVariables2DYZ()[i]);
	}

	//
	// Now find intersection
	//
	for (int i=0; i<ncdfvars.size(); i++) {
		vector <string>::iterator itr = find(
			vdcvars.begin(), vdcvars.end(), ncdfvars[i]
		);
		if (itr != vdcvars.end()) {
			out_varnames.push_back(ncdfvars[i]);
		}
	}
}


//
// The VDC requires a single missing value for all variables. netCDF variables
// may use different missing values for different values. This function
// maps all netCDF missing values to the same VDC missing value
//
void Copy2VDF::MissingValue(
	VDFIOBase *vdfio,
	DCReader *DCData,
	size_t vdcTS,
	string vdcVar,
	string ncdfVar,
	VDFIOBase::VarType_T vtype,
	int slice,
	float *buf
) {

	// 
	// Get missing values from VDC and netCDF. In general, the values
	// are different
	//
	float ncdfMV;
	bool hasmiss = DCData->GetMissingValue(ncdfVar, ncdfMV);
	if (! hasmiss) return;

	float vdcMV;
	if (vdfio->GetMissingValue().size()) {
		vdcMV = (float) vdfio->GetMissingValue()[0];
	}
	else if (vdfio->GetTSMissingValue(vdcTS).size()) {
		vdcMV = (float) vdfio->GetTSMissingValue(vdcTS)[0];
	}
	else if (vdfio->GetVMissingValue(vdcTS,vdcVar).size()) {
		vdcMV = (float) vdfio->GetVMissingValue(vdcTS, vdcVar)[0];
	}
	else {
		return;
	}

	size_t dim[3];
	vdfio->GetDim(dim, -1);

	size_t size = 0;
	size_t slice_sz = 0;

	bool first = false;
	unsigned char *mask = NULL;
	if (vtype == Metadata::VAR3D) {
		size = dim[0]*dim[1]*dim[2];
		slice_sz = dim[0]*dim[1];
		if ( ! _mvMask3D) {
			_mvMask3D = new unsigned char[size];
			first = true;
		}
		mask = _mvMask3D;
	}
	else if (vtype == Metadata::VAR2D_XY) {
		size = dim[0]*dim[1];
		slice_sz = size;
		if ( ! _mvMask2DXY) {
			_mvMask2DXY = new unsigned char[size];
			first = true;
		}
		mask = _mvMask2DXY;
	}
	else if (vtype == Metadata::VAR2D_XZ) {
		size = dim[0]*dim[2];
		slice_sz = size;
		if ( ! _mvMask2DXZ) {
			_mvMask2DXZ = new unsigned char[size];
			first = true;
		}
		mask = _mvMask2DXZ;
	}
	else if (vtype == Metadata::VAR2D_YZ) {
		size = dim[1]*dim[2];
		slice_sz = size;
		if ( ! _mvMask2DYZ) {
			_mvMask2DYZ = new unsigned char[size];
			first = true;
		}
		mask = _mvMask2DYZ;
	}

	bool mismatch = false;	// true if missing value locations change

	// If first mask for this variable type
	//
	if (first) {
		memset(mask, 0, size);
		for (size_t i=0; i<slice_sz; i++) {
			if (buf[i] == ncdfMV) {
				buf[i] = vdcMV;
				mask[slice*slice_sz+i] = 1;
			}
		}
	}
	else {
		for (size_t i=0; i<slice_sz; i++) {
			if (mask[slice*slice_sz+i]) {
				if (buf[i] != ncdfMV) mismatch = true;
				buf[i] = vdcMV;
			}
			else {
				if (buf[i] == ncdfMV) {
					buf[i] = vdcMV;
					mismatch = true;
				}
			}
		}
	}

}

int Copy2VDF::CopyVar(
	VDFIOBase *vdfio,
	DCReader *DCData,
	int vdcTS,
	int ncdfTS,
	string vdcVar,
	string ncdfVar,
	int level,
	int lod
) {
	if (DCData->OpenVariableRead(ncdfTS, ncdfVar) < 0) {
		MyBase::SetErrMsg(
			"Failed to open input data variable \"%s\" at time step %d",
			ncdfVar.c_str(), ncdfTS
		);
		return(-1);
	}

	if (vdfio->OpenVariableWrite(vdcTS, vdcVar.c_str(), level, lod) < 0) {
		MyBase::SetErrMsg(
			"Failed to open VDC variable \"%s\" at time step %d",
			vdcVar.c_str(), vdcTS
		);
		DCData->CloseVariable();
		return(-1);
	}

	size_t dim[3];
	vdfio->GetDim(dim, -1);
	float *buf = new float [dim[0]*dim[1]];

	int rc = 0;
	VDFIOBase::VarType_T vtype = vdfio->GetVarType(vdcVar);
	int n = vtype == Metadata::VAR3D ? dim[2] : 1;
	for (int i=0; i<n; i++) {

		rc = DCData->ReadSlice(buf);
		if (rc==0) {
			MyBase::SetErrMsg(
				"Short read of variable \"%s\" at time step %d",
				ncdfVar.c_str(), ncdfTS
			);
			rc = -1;	// Short read is an error
			break;
		}
		if (rc<0) {
			MyBase::SetErrMsg(
				"Error reading input data variable \"%s\" at time step %d",
				ncdfVar.c_str(), ncdfTS
			);
			break;
		}
		MissingValue(vdfio, DCData, vdcTS, vdcVar, ncdfVar, vtype, i, buf);

		rc = vdfio->WriteSlice(buf);
		if (rc<0) {
			MyBase::SetErrMsg(
				"Error writing VDC variable \"%s\" at time step %d",
				ncdfVar.c_str(), ncdfTS
			);
			break;
		}
	}
	
	if (buf) delete [] buf;
	DCData->CloseVariable();
	vdfio->CloseVariable();

	return(rc);

}

void Copy2VDF::deleteObjects(){
	delete DCData;
	delete wcwriter;
	wcwriter = NULL;
	DCData = NULL;
}

int Copy2VDF::launch2vdf(int argc, char **argv, string dataType, DCReader *optionalReader) {

	OptionParser::OptDescRec_T	set_opts[] = {
		{"vars",1,    "",	"Colon delimited list of variables to be copied "
			"from input data. The default is to copy all 2D and 3D variables"},
		{
			"numts",	1,	"-1",	"Maximum number of time steps that may be "
			"converted. A -1 implies the conversion of all time steps found"
		},
		{
			"startts",	1,	"0",	"Offset of first time step in netCDF files "
			" to be converted"
		},
		{"level",   1,  "-1","Refinement levels saved. 0=>coarsest, 1=>next refinement, etc. -1=>finest"},
		{"lod", 1,  "-1",   "Compression levels saved. 0 => coarsest, 1 => "
			"next refinement, etc. -1 => all levels defined by the .vdf file"},
		{"nthreads",1,  "0",    "Number of execution threads (0 => # processors)"},
		{"help",	0,	"",	"Print this message and exit"},
		{"quiet",	0,	"",	"Operate quietly"},
		{"debug",	0,	"",	"Turn on debugging"},
		{NULL}
	};

	OptionParser::Option_T	get_options[] = {
		{"vars", VetsUtil::CvtToStrVec, &_vars, sizeof(_vars)},
		{"numts", VetsUtil::CvtToInt, &_numts, sizeof(_numts)},
		{"startts", VetsUtil::CvtToInt, &_startts, sizeof(_startts)},
		{"level", VetsUtil::CvtToInt, &_level, sizeof(_level)},
		{"lod", VetsUtil::CvtToInt, &_lod, sizeof(_lod)},
		{"nthreads", VetsUtil::CvtToInt, &_nthreads, sizeof(_nthreads)},
		{"help", VetsUtil::CvtToBoolean, &_help, sizeof(_help)},
		{"quiet", VetsUtil::CvtToBoolean, &_quiet, sizeof(_quiet)},
		{"debug", VetsUtil::CvtToBoolean, &_debug, sizeof(_debug)},
		{NULL}
	};


    MyBase::SetErrMsgFilePtr(stderr);

    _progname = Basename(argv[0]);
	OptionParser op;

	op.AppendOptions(set_opts);

    if (op.ParseOptions(&argc, argv, get_options) < 0) {
		return(-1);
	}
    if (_debug) MyBase::SetDiagMsgFilePtr(stderr);

    if (_help) {
        Usage(op, NULL);
		return(0);
	}

	argv++;
	argc--;

	if (argc < 2) {
        Usage(op, "No files to process");
		return(-1);
	}

	//
	// list of netCDF files
	//
	vector<string> ncdffiles;
	for (int i=0; i<argc-1; i++) {
		 ncdffiles.push_back(argv[i]);
	}
	string metafile = argv[argc-1];
	
	// If we are receiving an optionalReader, we're using the wizard
	// Therefore we want to clear any previous wcwriter that may have
	// been created in a previous conversion
	if (optionalReader != NULL) {
		DCData = optionalReader;
		if (wcwriter) delete wcwriter;
		wcwriter = NULL;
	}
	if (DCData==NULL){
		if ((dataType == "ROMS") || (dataType == "CAM")) DCData = new DCReaderROMS(ncdffiles); 
		else if (dataType == "GRIB") DCData = new DCReaderGRIB(ncdffiles);
		else DCData = new DCReaderMOM(ncdffiles);
	}

	if (MyBase::GetErrCode() != 0) return(-1);

	WaveletBlockIOBase	*wbwriter3D = NULL;	// VDC type 1 writer
	VDFIOBase *vdfio = NULL;

	MetadataVDC metadata (metafile);
	if (MetadataVDC::GetErrCode() != 0) {
		MyBase::SetErrMsg("Error processing metafile \"%s\"", metafile.c_str());
		return(-1);
	}
	
	bool vdc1 = (metadata.GetVDCType() == 1);
	if (vdc1) {
		wbwriter3D = new WaveletBlock3DBufWriter(metadata);
		vdfio = wbwriter3D;
	} 
	else {
        if (wcwriter == NULL){
			wcwriter = new WaveCodecIO(metadata, _nthreads);
		}
		vdfio = wcwriter;
	}
	if (vdfio->GetErrCode() != 0) {
		return(-1);
	}

	//
	// Get Mapping between VDC time steps and ncdf time steps
	//
	map <size_t, size_t> timemap;
	GetTimeMap(vdfio, DCData, _startts, _numts, timemap);

	//
	// Figure out which variables to transform
	//
	vector <string> variables;
    GetVariables(vdfio, DCData, _vars, variables);

	//
	// Copy (transform) variables
	//
	int fails = 0;
	map <size_t, size_t>::iterator itr;
	for (itr = timemap.begin(); itr != timemap.end(); ++itr) {
    cout << &itr << endl;
	    if (! _quiet) {
			cout << "Processing VDC time step " << itr->first << endl;
		}
		for (int v = 0; v < variables.size(); v++) {
            if (! _quiet) {
				cout << " Processing variable " << variables[v] << ", ";
			}
			if (! DCData->VariableExists(itr->second, variables[v])) {
				SetErrMsg(
					"Variable \"%s\"does not exist at timestep %i", variables[v].c_str(), itr->first
				); 
				MyBase::SetErrCode(0); 	// must clear error code
				continue;
			}

			int rc = CopyVar(
				vdfio, DCData, itr->first, itr->second, 
				variables[v], variables[v],
                _level, _lod
			);
			if (rc<0) {
				MyBase::SetErrCode(0); 	// must clear error code
				if (! _quiet) cout << endl;
				fails++;
			}
			const float * drange = vdfio->GetDataRange();
			if (! _quiet) {
				cout << "data range (" << drange[0] << ", " << drange[1]<< ")\n";
			}
		}
	}

	int estatus = 0;
	if (fails) {
		SetErrMsg("Failed to copy %d variables", fails);
		estatus = 1;
	}
	return estatus;
}
