#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cerrno>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/WASP.h>

using namespace VetsUtil;
using namespace VAPoR;


//
//	Command line argument stuff
//
struct opt_t {
	string varname;
	int lod;
	int level;
	int nthreads;
	vector <int> start;
	vector <int> count;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	help;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"varname",	1, 	"var1",	"Name of variable"},
	{"lod",	1, 	"-1",	"Compression levels saved. 0 => coarsest, 1 => "
		"next refinement, etc. -1 => all levels defined by the netcdf file"},
	{"level",1, "-1","Multiresolution refinement level. Zero implies coarsest "
		"resolution"},
	{"start",1, "","Colon-delimited NetCDF style start coordinate vector"},
	{"count",1, "","Colon-delimited NetCDF style count coordinate vector"},
	{"nthreads",1, "0","Number of execution threads. 0 => use number of cores"},
	{"debug",	0,	"",	"Enable diagnostic"},
	{"help",	0,	"",	"Print this message and exit"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"varname", VetsUtil::CvtToCPPStr, &opt.varname, sizeof(opt.varname)},
	{"lod", VetsUtil::CvtToInt, &opt.lod, sizeof(opt.lod)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"start", VetsUtil::CvtToIntVec, &opt.start, sizeof(opt.start)},
	{"count", VetsUtil::CvtToIntVec, &opt.count, sizeof(opt.count)},
	{"nthreads", VetsUtil::CvtToInt, &opt.nthreads, sizeof(opt.nthreads)},
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

	string ncdffile = argv[1];	// Path to a vdf file
	string datafile = argv[2];	// Path to raw data file 

    if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	WASP   wasp(opt.nthreads);

	int rc = wasp.Open(ncdffile, NC_WRITE);

	vector <size_t> dims;
	rc = wasp.InqVarDimlens(opt.varname, dims, opt.level);
	if (rc<0) exit(1);


	rc = wasp.OpenVarRead(opt.varname, opt.lod, opt.level);
	if (rc<0) exit(1);

	vector <size_t> start(dims.size(), 0);
	if (opt.start.size()) {
		if (opt.start.size() != dims.size()) {
			MyBase::SetErrMsg("Invalid start specification");
			exit(1);
		}
		for (int i=0; i<opt.start.size(); i++) {
			start[i] = opt.start[i];
		}
	}

	vector <size_t> count = dims;
	if (opt.count.size()) {
		if (opt.count.size() != dims.size()) {
			MyBase::SetErrMsg("Invalid count specification");
			exit(1);
		}
		for (int i=0; i<opt.count.size(); i++) {
			count[i] = opt.count[i];
		}
	}

	size_t nelements = 1;
	for (int i=0; i<count.size(); i++) nelements *= count[i];

	float *data = new float[nelements];

	rc = wasp.GetVara(start, count, data);
	if (rc<0) exit(1);

	rc = wasp.CloseVar();
	if (rc<0) exit(1);

	rc = wasp.Close();
	if (rc<0) exit(1);

	FILE *fp = fopen(datafile.c_str(), "w");
	if (! fp) {
		MyBase::SetErrMsg("fopen(%s) : %M", datafile.c_str());
		exit(1);
	}

	rc = fwrite(data, sizeof(*data), nelements, fp);
	if (rc != nelements) {
		MyBase::SetErrMsg("fread() : %M");
		exit(1);
	}

	exit(0);
}
	



