#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>
#include <cerrno>
#include <cassert>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/VDCNetCDF.h>

using namespace VetsUtil;
using namespace VAPoR;


struct opt_t {
	int	ts;
	string varname;
	string type;
	int	level;
	int	lod;
	int	nthreads;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	quiet;
	OptionParser::IntRange_T xregion;
	OptionParser::IntRange_T yregion;
	OptionParser::IntRange_T zregion;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"ts",		1, 	"0","Timestep of data file starting from 0"},
	{"varname",	1, 	"var1",	"Name of variable"},
	{"type",    1,  "float32",  "Primitive type of output data"},
	{"level",1, "-1","Multiresolution refinement level. Zero implies coarsest resolution"},
	{"lod",1, "-1","Compression level of detail. Zero implies coarsest approximation"},
	{"nthreads",1, "0","Number of execution threads (0=># processors)"},
	{"help",	0,	"",	"Print this message and exit"},
	{"debug",	0,	"",	"Enable debugging"},
	{"quiet",	0,	"",	"Operate quietly"},
	{"xregion", 1, "-1:-1", "X dimension subregion bounds (min:max)"},
	{"yregion", 1, "-1:-1", "Y dimension subregion bounds (min:max)"},
	{"zregion", 1, "-1:-1", "Z dimension subregion bounds (min:max)"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"ts", VetsUtil::CvtToInt, &opt.ts, sizeof(opt.ts)},
	{"varname", VetsUtil::CvtToCPPStr, &opt.varname, sizeof(opt.varname)},
	{"type", VetsUtil::CvtToCPPStr, &opt.type, sizeof(opt.type)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"lod", VetsUtil::CvtToInt, &opt.lod, sizeof(opt.lod)},
	{"nthreads", VetsUtil::CvtToInt, &opt.nthreads, sizeof(opt.nthreads)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{"xregion", VetsUtil::CvtToIntRange, &opt.xregion, sizeof(opt.xregion)},
	{"yregion", VetsUtil::CvtToIntRange, &opt.yregion, sizeof(opt.yregion)},
	{"zregion", VetsUtil::CvtToIntRange, &opt.zregion, sizeof(opt.zregion)},
	{NULL}
};

size_t size_of_type(string type) {
	if (type.compare("float32") == 0) return(4);
	if (type.compare("float64") == 0) return(8);
	if (type.compare("int8") == 0) return(1);
	return(1);
}

int write_data(
	FILE	*fp, 
	string type,	// element type
	size_t n,
	const float *slice
) {

	static SmartBuf smart_buf;

	size_t element_sz = size_of_type(type);
	unsigned char *buffer = (unsigned char *) smart_buf.Alloc(n * element_sz);
	

	const float *sptr = slice;
	if (type.compare("float32")) {
		float *dptr = (float *) buffer;
		for (size_t i=0; i<n; i++) {
			*dptr++ = (float) *sptr++;
		}
	}
	else if (type.compare("float64")) {
		double *dptr = (double *) buffer;
		for (size_t i=0; i<n; i++) {
			*dptr++ = (double) *sptr++;
		}
	}
	else if (type.compare("int8")) {
		char *dptr = (char *) buffer;
		for (size_t i=0; i<n; i++) {
			*dptr++ = (char) *sptr++;
		}
	}

	int rc = fwrite(buffer, element_sz, n, fp);
	if (rc != n) {
		MyBase::SetErrMsg("Error writing output file : %M");
		return(-1);
	}

	return(0);
}

const char	*ProgName;

int	main(int argc, char **argv) {

	OptionParser op;

	ProgName = Basename(argv[0]);
	MyBase::SetErrMsgFilePtr(stderr);

	if (op.AppendOptions(set_opts) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	if (opt.help) {
		cerr << "Usage: " << ProgName << " [options] vdcmaster datafile" << endl;
		op.PrintOptionHelp(stderr);
		exit(0);
	}

	if (argc != 3) {
		cerr << "Usage: " << ProgName << " [options] vdcmaster datafile" << endl;
		op.PrintOptionHelp(stderr);
		exit(1);
	}

	string vdcmaster = argv[1];
	string datafile = argv[2];

	FILE *fp = fopen(datafile.c_str(), "w");
	if (! fp) {
		MyBase::SetErrMsg("Could not open file \"%s\" : %M", datafile.c_str());
		exit(1);
	}

	VDCNetCDF vdc(opt.nthreads);

	int rc = vdc.Initialize(vdcmaster, VDC::R, 4*1024*1024);
	if (rc<0) exit(1);

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


	cout << "-level not supported!!!!!\n";

	vector <size_t> sdims;
	size_t numts;
	if (! vdc.ParseDimensions(vptr->GetDimensions(), sdims, numts)) {
		MyBase::SetErrMsg("VDC corrupt, invalid dimensions");
		exit(1);
	}

	if (sdims.size() < 1) {
		MyBase::SetErrMsg("Variable must be 1D, 2D or 3D");
		exit(1);
	}

	size_t nelements = sdims[0];
	if (sdims.size() > 1) nelements *= sdims[1];

	float *slice = new float[nelements];

	//
	rc = vdc.OpenVariableRead(opt.ts, opt.varname, opt.level, opt.lod);
	if (rc<0) exit(1);

    size_t nz = sdims.size() > 2 ? sdims[2] : 1;

	for (size_t i=0; i<nz; i++) {
		rc = vdc.ReadSlice(slice);
		if (rc<0) exit(1);

		int rc = write_data(fp, opt.type, nelements, slice);
		if (rc<0) exit(1);
	}

	rc = vdc.CloseVariable();
	if (rc<0) exit(1);

	fclose(fp);

	exit(0);
}
	



