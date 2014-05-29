#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cerrno>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/VDCNetCDF.h>

using namespace VetsUtil;
using namespace VAPoR;


//
//	Command line argument stuff
//
struct opt_t {
	string varname;
	int lod;
	int nthreads;
	int ts;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	help;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"varname",	1, 	"var1",	"Name of variable"},
	{"lod",	1, 	"-1",	"Compression levels saved. 0 => coarsest, 1 => "
		"next refinement, etc. -1 => all levels defined by the netcdf file"},
	{"nthreads",	1, 	"0",	"Specify number of execution threads "
		"0 => use number of cores"},
	{"ts",	1, 	"0",	"Specify time step offset"},
	{"debug",	0,	"",	"Enable diagnostic"},
	{"help",	0,	"",	"Print this message and exit"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"varname", VetsUtil::CvtToCPPStr, &opt.varname, sizeof(opt.varname)},
	{"lod", VetsUtil::CvtToInt, &opt.lod, sizeof(opt.lod)},
	{"nthreads", VetsUtil::CvtToInt, &opt.nthreads, sizeof(opt.nthreads)},
	{"ts", VetsUtil::CvtToInt, &opt.ts, sizeof(opt.ts)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{NULL}
};

const char	*ProgName;

	
int	main(int argc, char **argv) {

	OptionParser op;


	MyBase::SetErrMsgFilePtr(stderr);

	//
	// Parse command line arguments
	//
	ProgName = Basename(argv[0]);

	if (op.AppendOptions(set_opts) < 0) {
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		exit(1);
	}

	if (opt.help) {
		cerr << "Usage: " << ProgName << " [options] netcdffile datafile" << endl;
		op.PrintOptionHelp(stderr);
		exit(0);
	}

	if (argc != 3) {
		cerr << "Usage: " << ProgName << " [options] netcdffile datafile" << endl;
		op.PrintOptionHelp(stderr);
		exit(1);
	}

	string master = argv[1];	// Path to VDC master file
	string datafile = argv[2];	// Path to raw data file 

    if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	VDCNetCDF   vdc(opt.nthreads);

	int rc = vdc.Initialize(master, VDC::A, 4*1024*1024);

	VDC::DataVar dvar;
	VDC::CoordVar cvar;
	VDC::VarBase *vptr;
	if (vdc.GetDataVar(opt.varname, dvar)) {
		vptr = &dvar;
	} 
	else if (vdc.GetCoordVar(opt.varname, cvar)) {
		vptr = &cvar;
	}
	else {
		MyBase::SetErrMsg("Invalid variable name: %s", opt.varname.c_str());
		exit(1);
	}

	vector <VDC::Dimension> dimensions = vptr->GetDimensions();
	vector <size_t> dims;
	for (int i=0; i<dimensions.size(); i++) {
		dims.push_back(dimensions[i].GetLength());
	}

	size_t nelements = 1;
	for (int i=0; i<dims.size(); i++) nelements *= dims[i];

	float *data = new float[nelements];

	FILE *fp = fopen(datafile.c_str(), "r");
	if (! fp) {
		MyBase::SetErrMsg("fopen(%s) : %M", datafile.c_str());
		exit(1);
	}

	// UGH!! Need to work out-of-core
	//
	rc = fread(data, sizeof(*data), nelements, fp);
	if (rc != nelements) {
		MyBase::SetErrMsg("fread() : %M");
		exit(1);
	}

	rc = vdc.OpenVariableWrite(opt.ts, opt.varname, opt.lod);
	if (rc<0) exit(1);

	rc = vdc.Write(data);
	if (rc<0) exit(1);

	rc = vdc.CloseVariable();
	if (rc<0) exit(1);
	
	exit(0);
}
