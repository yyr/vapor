#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/Metadata.h>
#include <vapor/DataMgr.h>

using namespace VetsUtil;
using namespace VAPoR;


struct {
	int	nts;
	int loop;
	int memsize;
	char *varname;
	int	numxforms;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
	OptionParser::Boolean_T	do_float;
	OptionParser::IntRange_T xregion;
	OptionParser::IntRange_T yregion;
	OptionParser::IntRange_T zregion;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"nts",		1, 	"10","Number of timesteps to process"},
	{"loop",	1, 	"10","Number of loops to execute"},
	{"memsize",	1, 	"100","Cache size in MBs"},
	{"varname",	1, 	"var1",	"Name of variable"},
	{"numxforms",1, "0","Multiresolution volume level. Zero implies native resolution"},
	{"help",	0,	"",	"Print this message and exit"},
	{"quiet",	0,	"",	"Operate quitely"},
	{"float",	0,	"",	"Read native floating point data (no quantization)"},
    {"xregion", 1, "-1:-1", "X dimension subregion bounds (min:max)"},
    {"yregion", 1, "-1:-1", "Y dimension subregion bounds (min:max)"},
    {"zregion", 1, "-1:-1", "Z dimension subregion bounds (min:max)"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"nts", VetsUtil::CvtToInt, &opt.nts, sizeof(opt.nts)},
	{"loop", VetsUtil::CvtToInt, &opt.loop, sizeof(opt.loop)},
	{"memsize", VetsUtil::CvtToInt, &opt.memsize, sizeof(opt.memsize)},
	{"varname", VetsUtil::CvtToString, &opt.varname, sizeof(opt.varname)},
	{"numxforms", VetsUtil::CvtToInt, &opt.numxforms, sizeof(opt.numxforms)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{"float", VetsUtil::CvtToBoolean, &opt.do_float, sizeof(opt.do_float)},
	{"xregion", VetsUtil::CvtToIntRange, &opt.xregion, sizeof(opt.xregion)},
	{"yregion", VetsUtil::CvtToIntRange, &opt.yregion, sizeof(opt.yregion)},
	{"zregion", VetsUtil::CvtToIntRange, &opt.zregion, sizeof(opt.zregion)},
	{NULL}
};

const char	*ProgName;


main(int argc, char **argv) {

	OptionParser op;
	const char	*metafile;

	double	timer = 0.0;
	string	s;

	ProgName = Basename(argv[0]);

	if (op.AppendOptions(set_opts) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	if (opt.help) {
		cerr << "Usage: " << ProgName << " [options] metafile datafile" << endl;
		op.PrintOptionHelp(stderr);
		exit(0);
	}

	if (argc != 2) {
		cerr << "Usage: " << ProgName << " [options] metafile " << endl;
		op.PrintOptionHelp(stderr);
		exit(1);
	}

	metafile = argv[1];

	size_t min[3] = {opt.xregion.min, opt.yregion.min, opt.zregion.min};
	size_t max[3] = {opt.xregion.max, opt.yregion.max, opt.zregion.max};

	Metadata	metadata(metafile);
	if (metadata.GetErrCode() != 0) {
		cerr << ProgName << " : " << metadata.GetErrMsg() << endl;
		exit(1);
	}

	DataMgr	datamgr(&metadata, opt.memsize, 0);

	const WaveletBlock3DRegionReader	*reader = datamgr.GetRegionReader();

	size_t bdim[3];
	reader->GetDimBlk(opt.numxforms, bdim);
	for(int i = 0; i<3; i++) {
		if (min[i] == -1) min[i] = 0;
		if (max[i] == -1) max[i] = bdim[i]-1;
	}


	TIMER_START(t0);

	for(int l = 0; l<opt.loop; l++) {
		cout << "Processing loop " << l << endl;
		for(int ts = 0; ts<opt.nts; ts++) {
			cout << "Processing time step " << ts << endl;

			if (opt.do_float) {
				float *fptr;
				fptr = datamgr.GetRegion(
					ts, opt.varname, opt.numxforms, min, max, 0
				);

				if (! fptr) {
					cerr << ProgName << " : " << datamgr.GetErrMsg() << endl;
					exit(1);
				}
			}
			else {
				unsigned char *ucptr;
				ucptr = datamgr.GetRegionUInt8(
					ts, opt.varname, opt.numxforms, min, max, 0
				);
				if (! ucptr) {
					cerr << ProgName << " : " << datamgr.GetErrMsg() << endl;
					exit(1);
				}
			}
		}
	}


	TIMER_STOP(t0,timer);

	if (! opt.quiet) {

		fprintf(stdout, "total process time : %f\n", timer);
	}

	exit(0);
}

