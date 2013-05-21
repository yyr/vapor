#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <sstream>

#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/WaveCodecIO.h>
#include <vapor/VDCFactory.h>
#include <vapor/CFuncs.h>
#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace VetsUtil;
using namespace VAPoR;


struct opt_t {
	OptionParser::Dimension3D_T	dim;
	OptionParser::Boolean_T	help;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"dimension",1, "512x512x512",	"Data volume dimensions expressed in "
		"grid points (NXxNYxNZ)"},
	{"help",	0,	"",	"Print this message and exit"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"dimension", VetsUtil::CvtToDimension3D, &opt.dim, sizeof(opt.dim)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{NULL}
};

string ProgName;


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

	VDCFactory	vdcf;
	if (vdcf.Parse(&argc, argv) < 0) {
		exit(1);
	}

	if (opt.help) {
		cerr << "Usage: " << ProgName << " [options] filename" << endl;
		op.PrintOptionHelp(stderr, 80, false);
		vdcf.Usage(stderr);
		exit(0);
	}

	if (argc != 2) {
		cerr << "Usage: " << ProgName << " [options] filename" << endl;
		op.PrintOptionHelp(stderr);
		vdcf.Usage(stderr);
		exit(1);
	}

	MetadataVDC *file;

	size_t dims[] = {opt.dim.nx, opt.dim.ny, opt.dim.nz};
	file = vdcf.New(dims);

	if (file->GetVariableNames().size() == 0) {
		vector <string> vars(1, "var1");
		file->SetVariables3D(vars);
	}

	if (! file) exit(1);


	if (file->Write(argv[1]) < 0) {
		exit(1);
	}
	exit(0);
}
