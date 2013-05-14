#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sstream>
#include <algorithm>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/DCReaderROMS.h>
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
	int numts;
	int startts;
    int level;
    int lod;
    int nthreads;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"vars",1,    "",	"Colon delimited list of variables to be copied "
		"from ncdf data. The default is to copy all 2D and 3D variables"},
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
	{NULL}
};

OptionParser::Option_T	get_options[] = {
	{"vars", VetsUtil::CvtToStrVec, &opt.vars, sizeof(opt.vars)},
	{"numts", VetsUtil::CvtToInt, &opt.numts, sizeof(opt.numts)},
	{"startts", VetsUtil::CvtToInt, &opt.startts, sizeof(opt.startts)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"lod", VetsUtil::CvtToInt, &opt.lod, sizeof(opt.lod)},
	{"nthreads", VetsUtil::CvtToInt, &opt.nthreads, sizeof(opt.nthreads)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
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
// Generate a map between VDC time steps and time steps in the DCReaderROMS
// i.e. ncdfTimeStep = timemap[vdcTimeStep]
//
void GetTimeMap(
	const VDFIOBase *vdfio,
	const DCReaderROMS *momData,
	int startts,
	int numts,
	map <size_t, size_t> &timemap
) {
	timemap.clear();


	//
	// Get user times from .vdf
	//
	vector <float> ncdftimes;
	for (int i=0; i<momData->GetNumTimeSteps(); i++) { 
		ncdftimes.push_back(momData->GetTSUserTime(i));
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
	numts = numts < 0 ? momData->GetNumTimeSteps() : numts;

	//
	// Cross-reference ncdf and vdc times
	//
	float ncdftime; 
	for (int i=startts; i<numts && i<momData->GetNumTimeSteps(); i++) {
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
	const DCReaderROMS *momData,
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
	for (int i=0; i<momData->GetVariables3D().size(); i++) {
		ncdfvars.push_back(momData->GetVariables3D()[i]);
	}
	for (int i=0; i<momData->GetVariables2DXY().size(); i++) {
		ncdfvars.push_back(momData->GetVariables2DXY()[i]);
	}
	for (int i=0; i<momData->GetVariables2DXZ().size(); i++) {
		ncdfvars.push_back(momData->GetVariables2DXZ()[i]);
	}
	for (int i=0; i<momData->GetVariables2DYZ().size(); i++) {
		ncdfvars.push_back(momData->GetVariables2DYZ()[i]);
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
// Missing data masks
//
unsigned char *mvMask3D = NULL;
unsigned char *mvMask2DXY = NULL;
unsigned char *mvMask2DXZ = NULL;
unsigned char *mvMask2DYZ = NULL;

//
// The VDC requires a single missing value for all variables. netCDF variables
// may use different missing values for different values. This function
// maps all netCDF missing values to the same VDC missing value
//
void MissingValue(
	VDFIOBase *vdfio,
	DCReaderROMS *momData,
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
	bool hasmiss = momData->GetMissingValue(ncdfVar, ncdfMV);
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
		if ( ! mvMask3D) {
			mvMask3D = new unsigned char[size];
			first = true;
		}
		mask = mvMask3D;
	}
	else if (vtype == Metadata::VAR2D_XY) {
		size = dim[0]*dim[1];
		slice_sz = size;
		if ( ! mvMask2DXY) {
			mvMask2DXY = new unsigned char[size];
			first = true;
		}
		mask = mvMask2DXY;
	}
	else if (vtype == Metadata::VAR2D_XZ) {
		size = dim[0]*dim[2];
		slice_sz = size;
		if ( ! mvMask2DXZ) {
			mvMask2DXZ = new unsigned char[size];
			first = true;
		}
		mask = mvMask2DXZ;
	}
	else if (vtype == Metadata::VAR2D_YZ) {
		size = dim[1]*dim[2];
		slice_sz = size;
		if ( ! mvMask2DYZ) {
			mvMask2DYZ = new unsigned char[size];
			first = true;
		}
		mask = mvMask2DYZ;
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

//	if (mismatch) {
//		cerr << ProgName << " : Warning: missing value locations changed\n";
//	}
}

int CopyVar(
	VDFIOBase *vdfio,
	DCReaderROMS *momData,
	int vdcTS,
	int ncdfTS,
	string vdcVar,
	string ncdfVar,
	int level,
	int lod
) {
	if (momData->OpenVariableRead(ncdfTS, ncdfVar) < 0) {
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
		momData->CloseVariable();
		return(-1);
	}

	size_t dim[3];
	vdfio->GetDim(dim, -1);
	float *buf = new float [dim[0]*dim[1]];

	int rc = 0;
	VDFIOBase::VarType_T vtype = vdfio->GetVarType(vdcVar);
	int n = vtype == Metadata::VAR3D ? dim[2] : 1;
	for (int i=0; i<n; i++) {

		rc = momData->ReadSlice(buf);
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
		MissingValue(vdfio, momData, vdcTS, vdcVar, ncdfVar, vtype, i, buf);

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
	momData->CloseVariable();
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

	if (opt.help) {
		Usage(op, NULL);
		exit(0);
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
	
	DCReaderROMS *momData = new DCReaderROMS(ncdffiles);

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
	GetTimeMap(vdfio, momData, opt.startts, opt.numts, timemap);

	//
	// Figure out which variables to transform
	//
	vector <string> variables;
	GetVariables(vdfio, momData, opt.vars, variables);

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
			if (! momData->VariableExists(itr->second, variables[v])) {
				continue;
			}

			int rc = CopyVar(
				vdfio, momData, itr->first, itr->second, 
				variables[v], variables[v],
				opt.level, opt.lod
			);
			if (rc<0) {
				MyBase::SetErrCode(0); 	// must clear error code
				fails++;
			}
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
