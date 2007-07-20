#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include <vapor/OptionParser.h>
#include <vapor/Metadata.h>
#include <vapor/MetadataSpherical.h>

using namespace VetsUtil;
using namespace VAPoR;

int	cvtToExtents(const char *from, void *to);
int	cvtTo3DBool(const char *from, void *to);
int	cvtToOrder(const char *from, void *to);

struct opt_t {
	OptionParser::Dimension3D_T	dim;
	OptionParser::Dimension3D_T	bs;
	int	numts;
	int	level;
	int	nfilter;
	int	nlifting;
	char *comment;
	char *coordsystem;
	char *gridtype;
	float extents[6];
	int order[3];
	int periodic[3];
	vector <string> varnames;
	OptionParser::Boolean_T	mtkcompat;
	OptionParser::Boolean_T	help;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"dimension",1, "512x512x512",	"Volume dimensions expressed in grid points (NXxNYxNZ)"},
	{"numts",	1, 	"1",			"Number of timesteps"},
	{"bs",		1, 	"32x32x32",		"Internal storage blocking factor expressed in grid points (NXxNYxNZ)"},
	{"level",	1, 	"0",			"Maximum refinement level. 0 => no refinement"},
	{"nfilter",	1, 	"1",			"Number of wavelet filter coefficients"},
	{"nlifting",1, 	"1",			"Number of wavelet lifting coefficients"},
	{"comment",	1,	"",				"Top-level comment"},
	{"gridtype",	1,	"regular",	"Data grid type (regular|layered|stretched|block_amr)"}, 
	{"coordsystem",	1,	"cartesian","Top-level comment (cartesian|spherical)"},
	{"extents",	1,	"0:0:0:0:0:0",	"Colon delimited 6-element vector specifying domain extents in user coordinates (X0:Y0:Z0:X1:Y1:Z1)"},
	{"order",	1,	"0:1:2",	"Colon delimited 3-element vector specifying permutation ordering of raw data on disk "},
	{"periodic",	1,	"0:0:0",	"Colon delimited 3-element boolean (0=>nonperiodic, 1=>periodic) vector specifying periodicity of X,Y,Z coordinate axes (X:Y:Z)"},
	{"varnames",1,	"var1",			"Colon delimited list of variable names"},
	{"mtkcompat",	0,	"",			"Force compatibility with older mtk files"},
	{"help",	0,	"",				"Print this message and exit"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"dimension", VetsUtil::CvtToDimension3D, &opt.dim, sizeof(opt.dim)},
	{"bs", VetsUtil::CvtToDimension3D, &opt.bs, sizeof(opt.bs)},
	{"numts", VetsUtil::CvtToInt, &opt.numts, sizeof(opt.numts)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"nfilter", VetsUtil::CvtToInt, &opt.nfilter, sizeof(opt.nfilter)},
	{"nlifting", VetsUtil::CvtToInt, &opt.nlifting, sizeof(opt.nlifting)},
	{"comment", VetsUtil::CvtToString, &opt.comment, sizeof(opt.comment)},
	{"gridtype", VetsUtil::CvtToString, &opt.gridtype, sizeof(opt.gridtype)},
	{"coordsystem", VetsUtil::CvtToString, &opt.coordsystem, sizeof(opt.coordsystem)},
	{"extents", cvtToExtents, &opt.extents, sizeof(opt.extents)},
	{"order", cvtToOrder, &opt.order, sizeof(opt.order)},
	{"periodic", cvtTo3DBool, &opt.periodic, sizeof(opt.periodic)},
	{"varnames", VetsUtil::CvtToStrVec, &opt.varnames, sizeof(opt.varnames)},
	{"mtkcompat", VetsUtil::CvtToBoolean, &opt.mtkcompat, sizeof(opt.mtkcompat)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{NULL}
};

int	main(int argc, char **argv) {

	OptionParser op;

	size_t bs[3];
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

	bs[0] = opt.bs.nx;
	bs[1] = opt.bs.ny;
	bs[2] = opt.bs.nz;
	dim[0] = opt.dim.nx;
	dim[1] = opt.dim.ny;
	dim[2] = opt.dim.nz;

	s.assign(opt.coordsystem);
	if (s.compare("spherical") == 0) {
		size_t perm[] = {opt.order[0], opt.order[1], opt.order[2]};

		file = new MetadataSpherical(
			dim,opt.level,bs,perm, opt.nfilter,opt.nlifting
		);
	}
	else {
		if (opt.mtkcompat) {
			file = new Metadata(
				dim,opt.level,bs,opt.nfilter,opt.nlifting, 0, 0
			);
		}
		else {
			file = new Metadata(
				dim,opt.level,bs,opt.nfilter,opt.nlifting
			);
		}
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

#ifdef	DEAD
	s.assign(opt.coordsystem);
	if (file->SetCoordSystemType(s) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}
#endif

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

	{
		vector <long> periodic_vec;

		for (int i=0; i<3; i++) periodic_vec.push_back(opt.periodic[i]);

		if (file->SetPeriodicBoundary(periodic_vec) < 0) {
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

int	cvtToOrder(
	const char *from, void *to
) {
	int   *iptr   = (int *) to;

	if (! from) {
		iptr[0] = iptr[1] = iptr[2];
	}
	else if (!  (sscanf(from,"%d:%d:%d", &iptr[0],&iptr[1],&iptr[2]) == 3)) { 

		return(-1);
	}
	return(1);
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

int	cvtTo3DBool(
	const char *from, void *to
) {
	int   *iptr   = (int *) to;

	if (! from) {
		iptr[0] = iptr[1] = iptr[2] = 0;
	}
	else if (! (sscanf(from,"%d:%d:%d", &iptr[0],&iptr[1],&iptr[2]) == 3)) { 
		return(-1);
	}
	return(1);
}

