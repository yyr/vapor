#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sstream>
#include <algorithm>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/DCReaderWRF.h>
#include <vapor/WaveCodecIO.h>
#include <vapor/WaveletBlock3DBufWriter.h>
#include <vapor/CFuncs.h>
#include <vapor/WrfVDCcreator.h>

#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace VetsUtil;
using namespace VAPoR;

Wrf2vdf::Wrf2vdf() {
        _progname.clear();
        _vars.clear();
        _numts = 0;
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

        wrfData = NULL;
}

Wrf2vdf::~Wrf2vdf() {
        if (_mvMask3D) delete []  _mvMask3D;
        if (_mvMask2DXY) delete [] _mvMask2DXY;
        if (_mvMask2DXZ) delete [] _mvMask2DXZ;
        if (_mvMask2DYZ) delete [] _mvMask2DYZ;
}

const char *ProgName;

void Wrf2vdf::Usage(OptionParser &op, const char * msg) {

	if (msg) {
		cerr << ProgName << " : " << msg << endl;
	}
	cerr << "Usage: " << ProgName << " [options] vdf_file wrffiles..." << endl;
	op.PrintOptionHelp(stderr, 80, false);

}


//
// Generate a map between VDC time steps and time steps in the DCReaderWRF
// i.e. ncdfTimeStep = timemap[vdcTimeStep]
//
void Wrf2vdf::GetTimeMap(
	const VDFIOBase *vdfio,
	const DCReaderWRF *wrfData,
	int startts,
	int numts,
	map <size_t, size_t> &timemap
) {
	timemap.clear();


	//
	// Get user times from .vdf
	//
	vector <float> ncdftimes;
	for (int i=0; i<wrfData->GetNumTimeSteps(); i++) { 
		ncdftimes.push_back(wrfData->GetTSUserTime(i));
	}

	//
	// Get user times from netCDF files
	//
	vector <float> vdctimes;
	for (int i=0; i<vdfio->GetNumTimeSteps(); i++) { 
		vdctimes.push_back(vdfio->GetTSUserTime(i));
	}

	// If numts<0 then use all ncdf times
	//
	numts = numts < 0 ? wrfData->GetNumTimeSteps() : numts;

	//
	// Cross-reference ncdf and vdc times
	//
	float ncdftime; 
	for (int i=startts; i<numts+startts && i<wrfData->GetNumTimeSteps(); i++) {
		ncdftime = ncdftimes[i];

		vector <float>::iterator itr = find(
			vdctimes.begin(), vdctimes.end(), ncdftime
		);

		if (itr != vdctimes.end()) {
			timemap[itr-vdctimes.begin()] = i;
		}
	}
}
	
void Wrf2vdf::GetVariables(
	const VDFIOBase *vdfio,
	const DCReaderWRF *wrfData,
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
	for (int i=0; i<wrfData->GetVariables3D().size(); i++) {
		ncdfvars.push_back(wrfData->GetVariables3D()[i]);
	}
	for (int i=0; i<wrfData->GetVariables2DXY().size(); i++) {
		ncdfvars.push_back(wrfData->GetVariables2DXY()[i]);
	}
	for (int i=0; i<wrfData->GetVariables2DXZ().size(); i++) {
		ncdfvars.push_back(wrfData->GetVariables2DXZ()[i]);
	}
	for (int i=0; i<wrfData->GetVariables2DYZ().size(); i++) {
		ncdfvars.push_back(wrfData->GetVariables2DYZ()[i]);
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


int Wrf2vdf::CopyVar(
	VDFIOBase *vdfio,
	DCReaderWRF *wrfData,
	int vdcTS,
	int ncdfTS,
	string vdcVar,
	string ncdfVar,
	int level,
	int lod
) {
	if (wrfData->OpenVariableRead(ncdfTS, ncdfVar) < 0) {
		MyBase::SetErrMsg(
			"Failed to open netCDF variable \"%s\" at time step %d",
			ncdfVar.c_str(), ncdfTS
		);
		return(-1);
	}

	if (vdfio->OpenVariableWrite(vdcTS, vdcVar.c_str(), level, lod) < 0) {
		MyBase::SetErrMsg(
			"Failed to open VDC variable \"%s\" at time step %d",
			vdcVar.c_str(), vdcTS
		);
		wrfData->CloseVariable();
		return(-1);
	}

	size_t dim[3];
	vdfio->GetDim(dim, -1);
	float *buf = new float [dim[0]*dim[1]];

	int rc = 0;
	VDFIOBase::VarType_T vtype = vdfio->GetVarType(vdcVar);
	int n = vtype == Metadata::VAR3D ? dim[2] : 1;
	for (int i=0; i<n; i++) {

		rc = wrfData->ReadSlice(buf);
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
				"Error reading netCDF variable \"%s\" at time step %d",
				ncdfVar.c_str(), ncdfTS
			);
			break;
		}

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
	wrfData->CloseVariable();
	vdfio->CloseVariable();

	return(rc);

}

int	Wrf2vdf::launchWrf2Vdf(int argc, char **argv) {

	OptionParser::OptDescRec_T      set_opts[] = {
        	{"vars",1,    "",       "Colon delimited list of variables to be copied "
                	"from ncdf data. The default is to copy all 2D and 3D variables"},
        	{
                	"numts",        1,      "-1",   "Maximum number of time steps that may be "
               		"converted. A -1 implies the conversion of all time steps found"
        	},
        	{
                	"startts",      1,      "0",    "Offset of first time step in netCDF files "
        	        " to be converted"
        	},
        	{"level",   1,  "-1","Refinement levels saved. 0=>coarsest, 1=>next refinement, etc. -1=>finest"},
        	{"lod", 1,  "-1",   "Compression levels saved. 0 => coarsest, 1 => "
                "next refinement, etc. -1 => all levels defined by the .vdf file"},
        	{"nthreads",1,  "0",    "Number of execution threads (0 => # processors)"},
        	{"help",        0,      "",     "Print this message and exit"},
        	{"quiet",       0,      "",     "Operate quietly"},
	        {"debug",       0,      "",     "Turn on debugging"},
        	{NULL}
	};

	OptionParser::Option_T  get_options[] = {
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

	ProgName = Basename(argv[0]);

	OptionParser op;
	if (op.AppendOptions(set_opts) < 0) {
		return -1;
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		return -1;
	}

	if (_debug) MyBase::SetDiagMsgFilePtr(stderr);
	if (_help) {
		Usage(op, NULL);
		return 0;
	}

	argv++;
	argc--;

	if (argc < 2) {
		Usage(op, "No files to process");
		return -1;
	}

	string metafile = *argv;
	argv++;
	argc--;

	//
	// list of netCDF files
	//
	vector<string> ncdffiles;
	for (int i=0; i<argc; i++) {
		 ncdffiles.push_back(argv[i]);
	}

	if (wrfData == NULL){
		wrfData = new DCReaderWRF(ncdffiles);
	}

	if (MyBase::GetErrCode() != 0) return -1;

	WaveletBlockIOBase	*wbwriter3D = NULL;	// VDC type 1 writer
	WaveCodecIO	*wcwriter = NULL;	// VDC type 2 writer
	VDFIOBase *vdfio = NULL;

	MetadataVDC metadata (metafile);
	if (MetadataVDC::GetErrCode() != 0) {
		MyBase::SetErrMsg("Error processing metafile \"%s\"", metafile.c_str());
		return -1;
	}
	
	bool vdc1 = (metadata.GetVDCType() == 1);
	if (vdc1) {
		wbwriter3D = new WaveletBlock3DBufWriter(metadata);
		vdfio = wbwriter3D;
	} 
	else {
		wcwriter = new WaveCodecIO(metadata, _nthreads);
		vdfio = wcwriter;
	}
	if (vdfio->GetErrCode() != 0) {
		return -1;
	}

	// 
	// Enable old style WRF time conversion this is a pre-2.3 metafile
	//
    string tag("DependentVarNames");
    string s = metadata.GetUserDataString(tag);
    if (! s.empty()) {
		wrfData->EnableLegacyTimeConversion();
	}


	//
	// Get Mapping between VDC time steps and ncdf time steps
	//
	map <size_t, size_t> timemap;
	GetTimeMap(vdfio, wrfData, _startts, _numts, timemap);

	//
	// Figure out which variables to transform
	//
	vector <string> variables;
	GetVariables(vdfio, wrfData, _vars, variables);

	//
	// Copy (transform) variables
	//
	int fails = 0;
	map <size_t, size_t>::iterator itr;
	for (itr = timemap.begin(); itr != timemap.end(); ++itr) {
		if (! _quiet) {
			cout << "Processing VDC time step " << itr->first << endl;
		}
		for (int v = 0; v < variables.size(); v++) {
			if (! _quiet) {
				cout << " Processing variable " << variables[v] << ", ";
			}
			if (! wrfData->VariableExists(itr->second, variables[v])) {
				cout << "does not exist!" << endl;
				continue;
			}

			int rc = CopyVar(
				vdfio, wrfData, itr->first, itr->second, 
				variables[v], variables[v],
				_level, _lod
			);
			if (rc<0) {
				MyBase::SetErrCode(0); 	// must clear error code
				cout << endl;
				fails++;
			}
			const float * drange = vdfio->GetDataRange();
			//cout << "data range (" << drange[0] << ", " << drange[1]<< ")\n";

		}
	}

	int estatus = 0;
	if (fails) {
		cerr << "Failed to copy " << fails << " variables" << endl;
		estatus = 1;
	}
	//exit(estatus);
	delete wcwriter;
	return estatus;
}
