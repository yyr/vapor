#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include <vapor/OptionParser.h>
#include <vapor/Metadata.h>

using namespace VetsUtil;
using namespace VAPoR;

int	cvtToExtents(const char *from, void *to);

struct {
	OptionParser::Dimension3D_T	dim;
	int	numts;
	int	bs;
	int	nxforms;
	int	nfilter;
	int	nlifting;
	char *comment;
	char *coordsystem;
	char *gridtype;
	float extents[6];
	vector <string> varnames;
	OptionParser::Boolean_T	mtkcompat;
	OptionParser::Boolean_T	help;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"dimension",1, "512x512x512",	"Volume dimensions (NXxNYxNZ)"},
	{"numts",	1, 	"1",			"Number of timesteps"},
	{"bs",		1, 	"32",			"Internal storage blocking factor"},
	{"nxforms",	1, 	"0",			"Number of forward wavelet transforms to apply"},
	{"nfilter",	1, 	"1",			"Number of wavelet filter coefficients"},
	{"nlifting",1, 	"1",			"Number of wavelet lifting coefficients"},
	{"comment",	1,	"",				"Top-level comment"},
	{"gridtype",	1,	"regular",	"Data grid type (regular|streched)"}, 
	{"coordsystem",	1,	"cartesian","Top-level comment (cartesian|spherical)"},
	{"extents",	1,	"0:0:0:0:0:0",	"Domain extents in user coordinates"},
	{"varnames",1,	"var1",			"Colon delimited list of variable names"},
	{"mtkcompat",	0,	"",			"Force compatibility with older mtk files"},
	{"help",	0,	"",				"Print this message and exit"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"dimension", VetsUtil::CvtToDimension3D, &opt.dim, sizeof(opt.dim)},
	{"numts", VetsUtil::CvtToInt, &opt.numts, sizeof(opt.numts)},
	{"bs", VetsUtil::CvtToInt, &opt.bs, sizeof(opt.bs)},
	{"nxforms", VetsUtil::CvtToInt, &opt.nxforms, sizeof(opt.nxforms)},
	{"nfilter", VetsUtil::CvtToInt, &opt.nfilter, sizeof(opt.nfilter)},
	{"nlifting", VetsUtil::CvtToInt, &opt.nlifting, sizeof(opt.nlifting)},
	{"comment", VetsUtil::CvtToString, &opt.comment, sizeof(opt.comment)},
	{"gridtype", VetsUtil::CvtToString, &opt.gridtype, sizeof(opt.gridtype)},
	{"coordsystem", VetsUtil::CvtToString, &opt.coordsystem, sizeof(opt.coordsystem)},
	{"extents", cvtToExtents, &opt.extents, sizeof(opt.extents)},
	{"varnames", VetsUtil::CvtToStrVec, &opt.varnames, sizeof(opt.varnames)},
	{"mtkcompat", VetsUtil::CvtToBoolean, &opt.mtkcompat, sizeof(opt.mtkcompat)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{NULL}
};

int	main(int argc, char **argv) {

	OptionParser op;

	size_t bs;
	size_t dim[3];
	string	s;
	Metadata *file;


	if (op.AppendOptions(set_opts) < 0) {
		cerr << argv[0] << " : " << op.GetErrMsg();
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		cerr << argv[0] << " : " << OptionParser::GetErrMsg();
		exit(1);
	}

	if (opt.help) {
		cerr << "Usage: " << argv[0] << " [options] filename" << endl;
		op.PrintOptionHelp(stderr);
		exit(0);
	}

	if (argc != 2) {
		cerr << "Usage: " << argv[0] << " [options] filename" << endl;
		op.PrintOptionHelp(stderr);
		exit(1);
	}

	bs = opt.bs;
	dim[0] = opt.dim.nx;
	dim[1] = opt.dim.ny;
	dim[2] = opt.dim.nz;

	if (opt.mtkcompat) {
		file = new Metadata(dim,opt.nxforms,bs,opt.nfilter,opt.nlifting, 0, 0);
	}
	else {
		file = new Metadata(dim,opt.nxforms,bs,opt.nfilter,opt.nlifting);
	}

	if (Metadata::GetErrCode()) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}

	if (file->SetNumTimeSteps(opt.numts) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}

	s.assign(opt.comment);
	if (file->SetComment(s) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}

	s.assign(opt.gridtype);
	if (file->SetGridType(s) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}

	s.assign(opt.coordsystem);
	if (file->SetCoordSystemType(s) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}

	int doExtents = 0;
	for(int i=0; i<5; i++) {
		if (opt.extents[i] != opt.extents[i+1]) doExtents = 1;
	}

	// let Metadata class calculate extents automatically if not 
	// supplied by user explicitly.
	//
	if (doExtents) {
		vector <double> extents;
		for(int i=0; i<6; i++) {
			extents.push_back(opt.extents[i]);
		}
		if (file->SetExtents(extents) < 0) {
			cerr << Metadata::GetErrMsg() << endl;
			exit(1);
		}
	}
	

	if (file->SetVariableNames(opt.varnames) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}


	if (file->Write(argv[1]) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}

	
}

int	cvtToExtents(
	const char *from, void *to
) {
	float   *fptr   = (float *) to;

	if (! from) {
		fptr[0] = fptr[1] = fptr[2] = fptr[3] = fptr[4] = fptr[5] = 0.0;
	}
	else if (! 
		(sscanf(from,"%f:%f:%f:%f:%f:%f",
		&fptr[0],&fptr[1],&fptr[2],&fptr[3],&fptr[4],&fptr[5]) == 6)) { 

		return(-1);
	}
	return(1);
}

