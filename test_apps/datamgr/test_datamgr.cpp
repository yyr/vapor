#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/Metadata.h>
#include <vapor/DataMgr.h>
#include <vapor/WaveletBlock3DIO.h>

using namespace VetsUtil;
using namespace VAPoR;


struct {
	int	nts;
	int	ts0;
	int loop;
	int memsize;
	char *varname;
	char *savefilebase;
	int	level;
	char *dtype;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
	OptionParser::Boolean_T	debug;
	OptionParser::IntRange_T xregion;
	OptionParser::IntRange_T yregion;
	OptionParser::IntRange_T zregion;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"nts",		1, 	"10","Number of timesteps to process"},
	{"ts0",		1, 	"0","First time step to process"},
	{"loop",	1, 	"10","Number of loops to execute"},
	{"memsize",	1, 	"100","Cache size in MBs"},
	{"varname",	1, 	"var1",	"Name of variable"},
	{"savefilebase",	1, 	"",	"Base path name to output file"},
	{"level",1, "0","Multiresution refinement level. Zero implies coarsest resolution"},
	{"dtype",	1,	"float",	"data type (float|uint8|uint16)"},
	{"help",	0,	"",	"Print this message and exit"},
	{"quiet",	0,	"",	"Operate quitely"},
	{"debug",	0,	"",	"Debug mode"},
    {"xregion", 1, "-1:-1", "X dimension subregion bounds (min:max)"},
    {"yregion", 1, "-1:-1", "Y dimension subregion bounds (min:max)"},
    {"zregion", 1, "-1:-1", "Z dimension subregion bounds (min:max)"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"nts", VetsUtil::CvtToInt, &opt.nts, sizeof(opt.nts)},
	{"ts0", VetsUtil::CvtToInt, &opt.ts0, sizeof(opt.ts0)},
	{"loop", VetsUtil::CvtToInt, &opt.loop, sizeof(opt.loop)},
	{"memsize", VetsUtil::CvtToInt, &opt.memsize, sizeof(opt.memsize)},
	{"varname", VetsUtil::CvtToString, &opt.varname, sizeof(opt.varname)},
	{"savefilebase", VetsUtil::CvtToString, &opt.savefilebase, sizeof(opt.savefilebase)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{"dtype", VetsUtil::CvtToString, &opt.dtype, sizeof(opt.dtype)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"xregion", VetsUtil::CvtToIntRange, &opt.xregion, sizeof(opt.xregion)},
	{"yregion", VetsUtil::CvtToIntRange, &opt.yregion, sizeof(opt.yregion)},
	{"zregion", VetsUtil::CvtToIntRange, &opt.zregion, sizeof(opt.zregion)},
	{NULL}
};

const char	*ProgName;


int main(int argc, char **argv) {

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
		cerr << "Usage: " << ProgName << " [options] metafile " << endl;
		op.PrintOptionHelp(stderr);
		exit(0);
	}

	if (opt.debug) {
		MyBase::SetDiagMsgFilePtr(stderr);
	}

	if (argc != 2) {
		cerr << "Usage: " << ProgName << " [options] metafile " << endl;
		op.PrintOptionHelp(stderr);
		exit(1);
	}

	metafile = argv[1];

	size_t min[3] = {opt.xregion.min, opt.yregion.min, opt.zregion.min};
	size_t max[3] = {opt.xregion.max, opt.yregion.max, opt.zregion.max};


	DataMgr	*datamgr;


	TIMER_START(t0);

	datamgr = new DataMgr(metafile, opt.memsize, 1);
	if (DataMgr::GetErrCode() != 0) {
		cerr << "DataMgr::DataMgr() : " << DataMgr::GetErrMsg() << endl;
		exit (1);
	}

	const Metadata *md = datamgr->GetMetadata();
	const size_t *bs = md->GetBlockSize();
	FILE *fp = NULL;

	for(int l = 0; l<opt.loop; l++) {
		cout << "Processing loop " << l << endl;


		const VDFIOBase	*reader = datamgr->GetRegionReader();

		size_t bdim[3];
		reader->GetDimBlk(bdim, opt.level);
		for(int i = 0; i<3; i++) {
			if (min[i] == -1) min[i] = 0;
			if (max[i] == -1) max[i] = bdim[i]-1;
		}

		for(int ts = opt.ts0; ts<opt.ts0+opt.nts; ts++) {
			cout << "Processing time step " << ts << endl;

			if (strlen(opt.savefilebase)) {
				char buf[4+1];
				string path(opt.savefilebase);
				path.append(".");
				sprintf(buf, "%4.4d", l);
				path.append(buf);
				path.append(".");
				sprintf(buf, "%4.4d", ts);
				path.append(buf);

				fp = fopen(path.c_str(), "w");
				if (! fp) {
					cerr << "Can't open output file " << path << endl;
				}
			}

			if (strcmp(opt.dtype, "float") == 0) {
				float *fptr;
				fptr = datamgr->GetRegion(
					ts, opt.varname, opt.level, min, max, 0
				);

				if (! fptr) {
					cerr << ProgName << " : " << datamgr->GetErrMsg() << endl;
					delete datamgr;
					exit(1);
				}

				if (fp) {
					size_t size = (max[0]-min[0]+1) * (max[1]-min[1]+1) * (max[2]-min[2]+1) * bs[0]*bs[1]*bs[2];

					fwrite(fptr, sizeof(fptr[0]), size, fp);
					fclose(fp);
				}
			}
			else if (strcmp(opt.dtype, "uint8") == 0) {
				float qrange[2] = {0.0, 1.0};
				unsigned char *ucptr;
				ucptr = datamgr->GetRegionUInt8(
					ts, opt.varname, opt.level, min, max, qrange, 0
				);
				if (! ucptr) {
					delete datamgr;
					cerr << ProgName << " : " << datamgr->GetErrMsg() << endl;
					exit(1);
				}

				if (fp) {
					size_t size = (max[0]-min[0]+1) * (max[1]-min[1]+1) * (max[2]-min[2]+1) * bs[0]*bs[1]*bs[2];

					fwrite(ucptr, sizeof(ucptr[0]), size, fp);
					fclose(fp);
				}
			}
			else if (strcmp(opt.dtype, "uint16") == 0) {
				float qrange[2] = {0.0, 1.0};
				unsigned char *ucptr;
				ucptr = datamgr->GetRegionUInt16(
					ts, opt.varname, opt.level, min, max, qrange, 0
				);
				if (! ucptr) {
					delete datamgr;
					cerr << ProgName << " : " << datamgr->GetErrMsg() << endl;
					exit(1);
				}

				if (fp) {
					size_t size = (max[0]-min[0]+1) * (max[1]-min[1]+1) * (max[2]-min[2]+1) * bs[0]*bs[1]*bs[2] * 2;

					fwrite(ucptr, sizeof(ucptr[0]), size, fp);
					fclose(fp);
				}
			}
			float r[2];
			datamgr->GetDataRange(ts, opt.varname, r);
			cout << "Data Range : [" << r[0] << ", " << r[1] << "]" << endl;
		}

	}

	delete datamgr;


	TIMER_STOP(t0,timer);

	if (! opt.quiet) {

		fprintf(stdout, "total process time : %f\n", timer);
	}

	exit(0);
}

