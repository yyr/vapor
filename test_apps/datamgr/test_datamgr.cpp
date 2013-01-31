#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>
#include <cassert>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/DataMgr.h>
#include <vapor/DataMgrFactory.h>

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
	int	lod;
	char *ftype;
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
	{"varname",	1, 	"",	"Name of variable"},
	{"savefilebase",	1, 	"",	"Base path name to output file"},
	{"level",1, "0","Multiresution refinement level. Zero implies coarsest resolution"},
	{"lod",1, "0","Level of detail. Zero implies coarsest resolution"},
	{"ftype",	1,	"vdf",	"data set type (vdf|wrf)"},
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
	{"lod", VetsUtil::CvtToInt, &opt.lod, sizeof(opt.lod)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{"ftype", VetsUtil::CvtToString, &opt.ftype, sizeof(opt.ftype)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"xregion", VetsUtil::CvtToIntRange, &opt.xregion, sizeof(opt.xregion)},
	{"yregion", VetsUtil::CvtToIntRange, &opt.yregion, sizeof(opt.yregion)},
	{"zregion", VetsUtil::CvtToIntRange, &opt.zregion, sizeof(opt.zregion)},
	{NULL}
};

const char	*ProgName;


void ErrMsgCBHandler(const char *msg, int) {
    cerr << ProgName << " : " << msg << endl;
}
void print_info(DataMgr *datamgr) {

	int num_transforms = datamgr->GetNumTransforms();
	cout << "Num transforms: " << num_transforms << endl;

	for (int i=0; i<= num_transforms; i++) {
		size_t dim[3];
		datamgr->GetDim(dim, i);

		cout << "Grid " << i << ":" << dim[0] << " " << dim[1] << " " << dim[2];
		cout << endl;
	}

	cout << "3D variables: ";
	for (int i=0; i<datamgr->GetVariables3D().size(); i++) {
		cout << datamgr->GetVariables3D()[i] << " ";
	}
	cout << endl;

	cout << "2DXY variables: ";
	for (int i=0; i<datamgr->GetVariables2DXY().size(); i++) {
		cout << datamgr->GetVariables2DXY()[i] << " ";
	}
	cout << endl;

	cout << "2DXZ variables: ";
	for (int i=0; i<datamgr->GetVariables2DXZ().size(); i++) {
		cout << datamgr->GetVariables2DXZ()[i] << " ";
	}
	cout << endl;

	cout << "2DYZ variables: ";
	for (int i=0; i<datamgr->GetVariables2DYZ().size(); i++) {
		cout << datamgr->GetVariables2DYZ()[i] << " ";
	}
	cout << endl;

	cout << "Compression ratios: ";
	for (int i=0; i<datamgr->GetCRatios().size(); i++) {
		cout << datamgr->GetCRatios()[i] << " ";
	}
	cout << endl;

	cout << "Extents: ";
	for (int i=0; i<datamgr->GetExtents().size(); i++) {
		cout << datamgr->GetExtents()[i] << " ";
	}
	cout << endl;

	cout << "Num timesteps: " << datamgr->GetNumTimeSteps() << endl;

	cout << "Map projection: " << datamgr->GetMapProjection() << endl;

	cout << endl << endl;

}

int main(int argc, char **argv) {

	OptionParser op;
	double	timer = 0.0;
	string	s;

	ProgName = Basename(argv[0]);

	MyBase::SetErrMsgCB(ErrMsgCBHandler);


	if (op.AppendOptions(set_opts) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	if (opt.help) {
		cerr << "Usage: " << ProgName << " [options] metafiles " << endl;
		op.PrintOptionHelp(stderr);
		exit(0);
	}

	if (opt.debug) {
		MyBase::SetDiagMsgFilePtr(stderr);
	}

	if (argc < 2) {
		cerr << "Usage: " << ProgName << " [options] metafiles " << endl;
		op.PrintOptionHelp(stderr);
		exit(1);
	}

	vector <string> metafiles;
	for (int i=1; i<argc; i++) {
		metafiles.push_back(argv[i]);
	}
		
	size_t min[3] = {opt.xregion.min, opt.yregion.min, opt.zregion.min};
	size_t max[3] = {opt.xregion.max, opt.yregion.max, opt.zregion.max};


	DataMgr	*datamgr;

	datamgr = DataMgrFactory::New(metafiles, opt.memsize, opt.ftype);
	if (DataMgrFactory::GetErrCode() != 0) {
		exit (1);
	}

	print_info(datamgr);

	string vname = opt.varname;
	if (vname.length() == 0) {
		vname = datamgr->GetVariableNames()[0];
	}


	FILE *fp = NULL;

	cout << "Variable name : " << vname << endl;

	int nts = datamgr->GetNumTimeSteps();
	for(int l = 0; l<opt.loop; l++) {
		cout << "Processing loop " << l << endl;


		size_t dim[3];
		datamgr->GetDim(dim, opt.level);
		for(int i = 0; i<3; i++) {
			if (min[i] == -1) min[i] = 0;
			if (max[i] == -1) max[i] = dim[i]-1;
		}

		for(int ts = opt.ts0; ts<opt.ts0+opt.nts && ts < nts; ts++) {
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
			if (! datamgr->VariableExists(ts,vname.c_str(),opt.level,opt.lod)) {
				cerr << "Variable " << vname << " does not exist" << endl;
				break;
			}

			RegularGrid *rg;
			rg = datamgr->GetGrid(
				ts, vname, opt.level, opt.lod, min, max, 0
			);

			if (! rg) {
				delete datamgr;
				exit(1);
			}

			if (fp) {
				RegularGrid::Iterator itr;
				float v;
				for (itr = rg->begin(); itr!=rg->end(); ++itr) {
					v = *itr;
					fwrite(&v, sizeof(v), 1, fp);
				}
				fclose(fp);
			}

			size_t min[3], max[3];
			int rc = datamgr->GetValidRegion(ts,vname.c_str(),opt.level,min,max);
			assert(rc >= 0);
			cout << "Valid Region : [" 
				<< min[0] << ", " << min[1] << ", " << min[2] << "] ["
				<< max[0] << ", " << max[1] << ", " << max[2] << "]" << endl;


			float r[2];
			datamgr->GetDataRange(ts, vname.c_str(), r);
			cout << "Data Range : [" << r[0] << ", " << r[1] << "]" << endl;

			string stamp;
			datamgr->GetTSUserTimeStamp(ts, stamp);
			cout << "Time stamp: " << stamp << endl;

			cout << "Has missing data : " << rg->HasMissingData() << endl;
			if (rg->HasMissingData()) {

				cout << "Missing data value : " << rg->GetMissingValue() << endl;
				RegularGrid::Iterator itr;
				float mv = rg->GetMissingValue();
				int count = 0;
				for (itr = rg->begin(); itr!=rg->end(); ++itr) {
					if (*itr == mv) count++;
				}
				cout << "Num missing values : " << count << endl;
			}

			cout << setprecision (16) << "User time: " << datamgr->GetTSUserTime(ts) << endl;
			cout << endl;
		}

	}

	delete datamgr;

	if (! opt.quiet) {

		fprintf(stdout, "total process time : %f\n", timer);
	}

	exit(0);
}

