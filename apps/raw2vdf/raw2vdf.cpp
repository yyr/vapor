#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cerrno>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/Metadata.h>
#include <vapor/WaveletBlock3DBufWriter.h>
#ifdef WIN32
#include "windows.h"
#endif

using namespace VetsUtil;
using namespace VAPoR;


struct {
	int	ts;
	char *varname;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"ts",		1, 	"0","Timestep of data file starting from 0"},
	{"varname",	1, 	"var1",	"Name of variable"},
	{"help",	0,	"",	"Print this message and exit"},
	{"quiet",	0,	"",	"Operate quitely"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"ts", VetsUtil::CvtToInt, &opt.ts, sizeof(opt.ts)},
	{"varname", VetsUtil::CvtToString, &opt.varname, sizeof(opt.varname)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};

const char	*ProgName;

void save_file(const char *file) {
	FILE	*ifp, *ofp;
	int	c;

	string oldfile(file);
	oldfile.append(".old");

	ifp = fopen(file, "rb");
	if (! ifp) {
		cerr << ProgName << ": Could not open file \"" << 
			file << "\" : " <<strerror(errno) << endl;

		exit(1);
	}

	ofp = fopen(oldfile.c_str(), "wb");
	if (! ifp) {
		cerr << ProgName << ": Could not open file \"" << 
			oldfile << "\" : " <<strerror(errno) << endl;

		exit(1);
	}

	do {
		c = fgetc(ifp);
		if (c != EOF) c = fputc(c,ofp); 

	} while (c != EOF);

	if (ferror(ifp)) {
		cerr << ProgName << ": Error reading file \"" << 
			file << "\" : " <<strerror(errno) << endl;

		exit(1);
	}

	if (ferror(ofp)) {
		cerr << ProgName << ": Error writing file \"" << 
			oldfile << "\" : " <<strerror(errno) << endl;

		exit(1);
	}
}
	

	

int	main(int argc, char **argv) {

	OptionParser op;
	FILE	*fp;
	const char	*metafile;
	const char	*datafile;

	double	timer = 0.0;
	double	read_timer = 0.0;
	string	s;
	int	rc;
	const Metadata	*metadata;

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

	if (argc != 3) {
		cerr << "Usage: " << ProgName << " [options] metafile datafile" << endl;
		op.PrintOptionHelp(stderr);
		exit(1);
	}

	metafile = argv[1];
	datafile = argv[2];


	WaveletBlock3DBufWriter	wbwriter(metafile, 0);
	if (wbwriter.GetErrCode() != 0) {
		cerr << ProgName << " : " << wbwriter.GetErrMsg() << endl;
		exit(1);
	}
	metadata = wbwriter.GetMetadata();

	if (wbwriter.OpenVariableWrite(opt.ts, opt.varname) < 0) {
		cerr << ProgName << " : " << wbwriter.GetErrMsg() << endl;
		exit(1);
	} 

	save_file(metafile);

#ifndef WIN32
	fp = fopen64(datafile, "r");
#else
	fp = fopen(datafile, "rb");
#endif
	if (! fp) {
		cerr << ProgName << ": Could not open file \"" << 
			datafile << "\" : " <<strerror(errno) << endl;

		exit(1);
	}

	const size_t *dim = metadata->GetDimension();
	float *slice = new float[dim[0]*dim[1]];


	TIMER_START(t0);
	for(int z=0; z<dim[2]; z++) {

		if (z%10== 0 && ! opt.quiet) {
			cout << "Reading slice # " << z << endl;
		}

		TIMER_START(t1);
		rc = fread(slice, sizeof(slice[0]), dim[0]*dim[1], fp);
		if (rc != dim[0]*dim[1]) {
			if (rc<0) {
				cerr << ProgName << ": Could not read data file \"" << 
					datafile << "\" : " <<strerror(errno) << endl;
			}
			else {
				cerr << ProgName << ": short read" << endl;
			}
			exit(1);
		}
		TIMER_STOP(t1, read_timer);

        wbwriter.WriteSlice(slice);
        if (wbwriter.GetErrCode() != 0) {
			cerr << ProgName << ": " << wbwriter.GetErrMsg() << endl;
            exit(1);
        }

	}
	TIMER_STOP(t0,timer);

	wbwriter.CloseVariable();

	if (! opt.quiet) {
		float	write_timer = wbwriter.GetWriteTimer();
		float	xform_timer = wbwriter.GetXFormTimer();

		fprintf(stdout, "read time : %f\n", read_timer);
		fprintf(stdout, "write time : %f\n", write_timer);
		fprintf(stdout, "transform time : %f\n", xform_timer - write_timer);
		fprintf(stdout, "total transform time : %f\n", timer);
	}

	// Write out the updated metafile
	//
	metadata->Write(metafile);

	exit(0);
}

