#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sstream>
#include <algorithm>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/DCReaderNCDF.h>
#include <vapor/WaveCodecIO.h>
#include <vapor/WaveletBlock3DBufWriter.h>
#include <vapor/CFuncs.h>

#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace VetsUtil;
using namespace VAPoR;

struct opt_t {
	vector <string> vars;
	vector <string> timedims;
	vector <string> timevars;
	vector <string> stagdims;
	vector <string>	dimnames;
	vector <string>	cnstnames;
	vector <int>	cnstvals;
	int numts;
	int startts;
    int level;
    int lod;
    int nthreads;
	string missattr;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
	OptionParser::Boolean_T	debug;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"vars",1,    "",	"Colon delimited list of variables to be copied "
		"from ncdf data. The default is to copy all 2D and 3D variables"},
	{"timedims",	1,	"",	"Colon delimited list of time dimension variable "
		"names"},
	{"timevars",    1,  "", "Colon delimited list of time coordinate "
		"variables. If this 1D variable "
		"is present, it specfies the time in user-defined units for each "
		"time step. This option takes precedence over -usertime or -startt."},
	{"stagdims",	1,	"",	"Colon delimited list of staggered dimension names"},
	{"dimnames",	1,	"",	"Deprecated"},
	{"cnstnames",	1,	"",	"Deprecated"},
	{"cnstvals",	1,	"",	"Deprecated"},
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
	{"missattr",	1,	"",	"Name of netCDF attribute specifying missing value "
		" This option takes precdence over -missing"},
	{"help",	0,	"",	"Print this message and exit"},
	{"quiet",	0,	"",	"Operate quietly"},
	{"debug",	0,	"",	"Turn on debugging"},
	{NULL}
};

OptionParser::Option_T	get_options[] = {
	{"vars", VetsUtil::CvtToStrVec, &opt.vars, sizeof(opt.vars)},
	{"timedims", VetsUtil::CvtToStrVec, &opt.timedims, sizeof(opt.timedims)},
	{"timevars", VetsUtil::CvtToStrVec, &opt.timevars, sizeof(opt.timevars)},
	{"stagdims", VetsUtil::CvtToStrVec, &opt.stagdims, sizeof(opt.stagdims)},
	{"dimnames", VetsUtil::CvtToStrVec, &opt.dimnames, sizeof(opt.dimnames)},
	{"cnstnames", VetsUtil::CvtToStrVec, &opt.cnstnames, sizeof(opt.cnstnames)},
	{"cnstvals", VetsUtil::CvtToIntVec, &opt.cnstvals, sizeof(opt.cnstvals)},
	{"numts", VetsUtil::CvtToInt, &opt.numts, sizeof(opt.numts)},
	{"startts", VetsUtil::CvtToInt, &opt.startts, sizeof(opt.startts)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"lod", VetsUtil::CvtToInt, &opt.lod, sizeof(opt.lod)},
	{"nthreads", VetsUtil::CvtToInt, &opt.nthreads, sizeof(opt.nthreads)},
	{"missattr", VetsUtil::CvtToCPPStr, &opt.missattr, sizeof(opt.missattr)},
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


//
// Generate a map between VDC time steps and time steps in the DCReaderNCDF
// i.e. ncdfTimeStep = timemap[vdcTimeStep]
//
void GetTimeMap(
	const VDFIOBase *vdfio,
	const DCReaderNCDF *ncdfData,
	int startts,
	int numts,
	map <size_t, size_t> &timemap
) {
	timemap.clear();


	//
	// Get user times from .vdf
	//
	vector <float> ncdftimes;
	for (int i=0; i<ncdfData->GetNumTimeSteps(); i++) { 
		ncdftimes.push_back(ncdfData->GetTSUserTime(i));
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
	numts = numts < 0 ? ncdfData->GetNumTimeSteps() : numts;

	//
	// Cross-reference ncdf and vdc times
	//
	float ncdftime; 
	for (int i=startts; i<numts && i<ncdfData->GetNumTimeSteps(); i++) {
		ncdftime = ncdftimes[i];

		vector <float>::iterator itr = find(
			vdctimes.begin(), vdctimes.end(), ncdftime
		);

		if (itr != vdctimes.end()) {
			timemap[itr-vdctimes.begin()] = i;
		}
	}
}
	
void GetVariables(
	const VDFIOBase *vdfio,
	const DCReaderNCDF *ncdfData,
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
	for (int i=0; i<ncdfData->GetVariables3D().size(); i++) {
		ncdfvars.push_back(ncdfData->GetVariables3D()[i]);
	}
	for (int i=0; i<ncdfData->GetVariables2DXY().size(); i++) {
		ncdfvars.push_back(ncdfData->GetVariables2DXY()[i]);
	}
	for (int i=0; i<ncdfData->GetVariables2DXZ().size(); i++) {
		ncdfvars.push_back(ncdfData->GetVariables2DXZ()[i]);
	}
	for (int i=0; i<ncdfData->GetVariables2DYZ().size(); i++) {
		ncdfvars.push_back(ncdfData->GetVariables2DYZ()[i]);
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
void MissingValue(
	VDFIOBase *vdfio,
	DCReaderNCDF *ncdfData,
	string vdcVar,
	string ncdfVar,
	VDFIOBase::VarType_T vtype,
	float *buf
) {

	// 
	// Get missing values from VDC and netCDF. In general, the values
	// are different
	//
	float ncdfMV;
	bool hasmiss = ncdfData->GetMissingValue(ncdfVar, ncdfMV);
	if (! hasmiss) return;

	vector <double> vec;
    if (vdfio->GetVMissingValue(0, vdcVar).size() == 1) {
         vec = vdfio->GetVMissingValue(0,vdcVar);
    }
	else if (vdfio->GetTSMissingValue(0).size() == 1) {
         vec = vdfio->GetTSMissingValue(0);
    }
	else if (vdfio->GetMissingValue().size() == 1) {
         vec =  vdfio->GetMissingValue();
    }
	else {
		return;
	}



	float vdcMV = (float) vec[0];

	size_t dim[3];
	vdfio->GetDim(dim, -1);

	size_t slice_sz = 0;

	if (vtype == Metadata::VAR3D) {
		slice_sz = dim[0]*dim[1];
	}
	else if (vtype == Metadata::VAR2D_XY) {
		slice_sz = dim[0]*dim[1];
	}
	else if (vtype == Metadata::VAR2D_XZ) {
		slice_sz = dim[0]*dim[2];
	}
	else if (vtype == Metadata::VAR2D_YZ) {
		slice_sz = dim[1]*dim[2];
	}

	for (size_t i=0; i<slice_sz; i++) {
		if (buf[i] == ncdfMV) {
			buf[i] = vdcMV;
		}
	}
}

int CopyVar(
	VDFIOBase *vdfio,
	DCReaderNCDF *ncdfData,
	int vdcTS,
	int ncdfTS,
	string vdcVar,
	string ncdfVar,
	int level,
	int lod
) {
	if (ncdfData->OpenVariableRead(ncdfTS, ncdfVar) < 0) {
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
		ncdfData->CloseVariable();
		return(-1);
	}

	size_t dim[3];
	vdfio->GetDim(dim, -1);
	float *buf = new float [dim[0]*dim[1]];

	int rc = 0;
	VDFIOBase::VarType_T vtype = vdfio->GetVarType(vdcVar);
	int n = vtype == Metadata::VAR3D ? dim[2] : 1;
	for (int i=0; i<n; i++) {

		rc = ncdfData->ReadSlice(buf);
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
		if (opt.missattr.length()) {
			MissingValue(vdfio, ncdfData, vdcVar, ncdfVar, vtype, buf);
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
	ncdfData->CloseVariable();
	vdfio->CloseVariable();

	return(rc);

}

int	main(int argc, char **argv) {

    MyBase::SetErrMsgFilePtr(stderr);
	ProgName = Basename(argv[0]);

	OptionParser op;
	if (op.AppendOptions(set_opts) < 0) {
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		exit(1);
	}

	if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);
	if (opt.help) {
		Usage(op, NULL);
		exit(0);
	}
	if (opt.dimnames.size()) {
		MyBase::SetErrMsg("Option \"-dimnames\" is deprecated");
	}
	if (opt.cnstnames.size()) {
		MyBase::SetErrMsg("Option \"-cnstnames\" is deprecated");
	}
	if (opt.cnstvals.size()) {
		MyBase::SetErrMsg("Option \"-cnstvals\" is deprecated");
	}


	argv++;
	argc--;

	if (argc < 2) {
		Usage(op, "No files to process");
		exit(1);
	}

	//
	// list of netCDF files
	//
	vector<string> ncdffiles;
	for (int i=0; i<argc-1; i++) {
		 ncdffiles.push_back(argv[i]);
	}
	string metafile = argv[argc-1];
	
	DCReaderNCDF *ncdfData = new DCReaderNCDF(
		ncdffiles, opt.timedims, opt.timevars, opt.stagdims, opt.missattr,
		NULL
	);
	if (MyBase::GetErrCode() != 0) exit(1);

	WaveletBlockIOBase	*wbwriter3D = NULL;	// VDC type 1 writer
	WaveCodecIO	*wcwriter = NULL;	// VDC type 2 writer
	VDFIOBase *vdfio = NULL;

	MetadataVDC metadata (metafile);
	if (MetadataVDC::GetErrCode() != 0) {
		MyBase::SetErrMsg("Error processing metafile \"%s\"", metafile.c_str());
		exit(1);
	}
	
	bool vdc1 = (metadata.GetVDCType() == 1);
	if (vdc1) {
		wbwriter3D = new WaveletBlock3DBufWriter(metadata);
		vdfio = wbwriter3D;
	} 
	else {
		wcwriter = new WaveCodecIO(metadata, opt.nthreads);
		vdfio = wcwriter;
	}
	if (vdfio->GetErrCode() != 0) {
		exit(1);
	}

	//
	// Get Mapping between VDC time steps and ncdf time steps
	//
	map <size_t, size_t> timemap;
	GetTimeMap(vdfio, ncdfData, opt.startts, opt.numts, timemap);

	//
	// Figure out which variables to transform
	//
	vector <string> variables;
	GetVariables(vdfio, ncdfData, opt.vars, variables);

	//
	// Copy (transform) variables
	//
	int fails = 0;
	map <size_t, size_t>::iterator itr;
	for (itr = timemap.begin(); itr != timemap.end(); ++itr) {
		if (! opt.quiet) {
			cout << "Processing VDC time step " << itr->first << endl;
		}
		for (int v = 0; v < variables.size(); v++) {
			if (! opt.quiet) {
				cout << " Processing variable " << variables[v] << ", ";
			}

			int rc = CopyVar(
				vdfio, ncdfData, itr->first, itr->second, 
				variables[v], variables[v],
				opt.level, opt.lod
			);
			if (rc<0) fails++;
			const float * drange = vdfio->GetDataRange();
			cout << "data range (" << drange[0] << ", " << drange[1]<< ")\n";

		}
	}

	int estatus = 0;
	if (fails) {
		cerr << "Failed to copy " << fails << " variables" << endl;
		estatus = 1;
	}
	exit(estatus);

} // End of main.
