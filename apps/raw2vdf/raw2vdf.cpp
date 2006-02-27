//
//      $Id$
//
//***********************************************************************
//                                                                       *
//                            Copyright (C)  2005                        *
//            University Corporation for Atmospheric Research            *
//                            All Rights Reserved                        *
//                                                                       *
//***********************************************************************/
//
//      File:		raw2vdf.cpp
//
//      Author:         John Clyne
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           Tue Jun 14 15:01:13 MDT 2005
//
//      Description:	Read a file containing a raw data volume. Translate
//			and append the volume to an existing
//			Vapor Data Collection
//
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cerrno>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/Metadata.h>
#include <vapor/WaveletBlock3DBufWriter.h>
#include <vapor/WaveletBlock3DRegionWriter.h>
#ifdef WIN32
#include "windows.h"
#endif

using namespace VetsUtil;
using namespace VAPoR;


//
//	Command line argument stuff
//
struct {
	int	ts;
	char *varname;
	int level;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	quiet;
	OptionParser::Boolean_T	swapbytes;
	OptionParser::Boolean_T	dbl;
	OptionParser::IntRange_T xregion;
	OptionParser::IntRange_T yregion;
	OptionParser::IntRange_T zregion;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"ts",		1, 	"0","Timestep of data file starting from 0"},
	{"varname",	1, 	"var1",	"Name of variable"},
	{"level",	1, 	"-1",	"Refinement levels saved. 0=>coarsest, 1=>next refinement, etc. -1=>finest"},
	{"help",	0,	"",	"Print this message and exit"},
	{"debug",	0,	"",	"Enable debugging"},
	{"quiet",	0,	"",	"Operate quietly"},
	{"swapbytes",	0,	"",	"Swap bytes in raw data as they are read from disk"},
	{"dbl",	0,	"",	"Input data are 64-bit floats"},
	{"xregion", 1, "-1:-1", "X dimension subregion bounds (min:max)"},
	{"yregion", 1, "-1:-1", "Y dimension subregion bounds (min:max)"},
	{"zregion", 1, "-1:-1", "Z dimension subregion bounds (min:max)"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"ts", VetsUtil::CvtToInt, &opt.ts, sizeof(opt.ts)},
	{"varname", VetsUtil::CvtToString, &opt.varname, sizeof(opt.varname)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{"swapbytes", VetsUtil::CvtToBoolean, &opt.swapbytes, sizeof(opt.swapbytes)},
	{"dbl", VetsUtil::CvtToBoolean, &opt.dbl, sizeof(opt.dbl)},
	{"xregion", VetsUtil::CvtToIntRange, &opt.xregion, sizeof(opt.xregion)},
	{"yregion", VetsUtil::CvtToIntRange, &opt.yregion, sizeof(opt.yregion)},
	{"zregion", VetsUtil::CvtToIntRange, &opt.zregion, sizeof(opt.zregion)},
	{NULL}
};

const char	*ProgName;

//
// Backup a .vdf file
//
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
	
void    swapbytes(
	void *vptr,
	int size,
	int	n
) {
	unsigned char   *ucptr = (unsigned char *) vptr;
	unsigned char   uc;
	int             i,j;

	for (j=0; j<n; j++) {
		for (i=0; i<size/2; i++) {
			uc = ucptr[i];
			ucptr[i] = ucptr[size-i-1];
			ucptr[size-i-1] = uc;
		}
		ucptr += size;
	}
}

void	process_volume(
	WaveletBlock3DBufWriter *bufwriter,
	FILE	*fp, 
	const size_t *dim,
	double *read_timer
) {

	// Allocate a buffer large enough to hold one slice of data
	//
	int element_sz;
	if (opt.dbl) element_sz = sizeof(double);
	else element_sz = sizeof (float);

	unsigned char *slice = new unsigned char [dim[0]*dim[1]*element_sz];

	//
	// Translate the volume one slice at a time
	//
	for(int z=0; z<dim[2]; z++) {

		if (z%10== 0 && ! opt.quiet) {
			cout << "Reading slice # " << z << endl;
		}

		TIMER_START(t1);
		int rc = fread(slice, element_sz, dim[0]*dim[1], fp);
		if (rc != dim[0]*dim[1]) {
			if (rc<0) {
				cerr << ProgName << ": Error reading input file : " << 
					strerror(errno) << endl;
			}
			else {
				cerr << ProgName << ": short read" << endl;
			}
			exit(1);
		}
		TIMER_STOP(t1, *read_timer);

		//
		// If the data stored on disk are byte swapped relative
		// to the machine we're running on...
		//
		if (opt.swapbytes) {
			swapbytes(slice, element_sz, dim[0]*dim[1]); 
		}

		// Convert data from double to float if needed.
		if (opt.dbl) {
			float *fptr = (float *) slice;
			double *dptr = (double *) slice;
			for(int i=0; i<dim[0]*dim[1]; i++) *fptr++ = (float) *dptr++;
		}
		//
		// Write a single slice of data
		//
		bufwriter->WriteSlice((float *) slice);
		if (bufwriter->GetErrCode() != 0) {
			cerr << ProgName << ": " << bufwriter->GetErrMsg() << endl;
			exit(1);
		}

	}
}

void	process_region(
	WaveletBlock3DRegionWriter *regwriter,
	FILE	*fp, 
	const size_t *dim,
	double *read_timer
) {

	size_t min[3] = {opt.xregion.min, opt.yregion.min, opt.zregion.min};
	size_t max[3] = {opt.xregion.max, opt.yregion.max, opt.zregion.max};
	size_t rdim[3];

	for(int i=0; i<3; i++) {
		if (min[i] == (size_t) -1)  min[i] = 0;
		if (max[i] == (size_t) -1)  max[i] = dim[i]-1;
		rdim[i] = max[i]-min[i]+1;
	}

	// Allocate a buffer large enough to hold entire subregion
	//
	size_t size;
	size_t element_sz;
	if (opt.dbl) {
		element_sz = sizeof(double);
		// extra space to convert a slice of double to float;
		size = rdim[0] * rdim[1] * rdim[2] * sizeof(float) +
			(rdim[0]*rdim[1] * (sizeof(double) - sizeof(float)));

	}
	else {
		element_sz = sizeof(float);
		size = rdim[0] * rdim[1] * rdim[2] * sizeof(float);
	}

	unsigned char *buf = new unsigned char [size];

	//
	// Translate the volume one slice at a time
	//
	unsigned char *slice = buf;
	for(int z=0; z<rdim[2]; z++) {

		if (z%10== 0 && ! opt.quiet) {
			cout << "Reading slice # " << z << endl;
		}

		TIMER_START(t1);
		int rc = fread(slice, element_sz, rdim[0]*rdim[1], fp);
		if (rc != rdim[0]*rdim[1]) {
			if (rc<0) {
				cerr << ProgName << ": Error reading input file : " << 
					strerror(errno) << endl;
			}
			else {
				cerr << ProgName << ": short read" << endl;
			}
			exit(1);
		}
		TIMER_STOP(t1, *read_timer);

		//
		// If the data stored on disk are byte swapped relative
		// to the machine we're running on...
		//
		if (opt.swapbytes) {
			swapbytes(slice, element_sz, rdim[0]*rdim[1]); 
		}

		// Convert data from double to float if needed.
		if (opt.dbl) {
			float *fptr = (float *) slice;
			double *dptr = (double *) slice;
			for(int i=0; i<rdim[0]*rdim[1]; i++) *fptr++ = (float) *dptr++;
		}

		slice += rdim[0]*rdim[1]*element_sz;
	}

	regwriter->WriteRegion((float *) buf, min, max);
	if (regwriter->GetErrCode() != 0) {
		cerr << ProgName << ": " << regwriter->GetErrMsg() << endl;
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
	const Metadata	*metadata;

	//
	// Parse command line arguments
	//
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

	metafile = argv[1];	// Path to a vdf file
	datafile = argv[2];	// Path to raw data file 

    if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	WaveletBlock3DIO	*wbwriter;

	size_t min[3] = {opt.xregion.min, opt.yregion.min, opt.zregion.min};
	size_t max[3] = {opt.xregion.max, opt.yregion.max, opt.zregion.max};

	//
	// Create an appropriate WaveletBlock writer. Initialize with
	// path to .vdf file
	//
	if (min[0] == min[1] && min[1] == min[2] && min[2] == max[0] &&
		max[0] == max[1]  && max[1] == max[2] && max[2] == (size_t) -1)
	{
		wbwriter = new WaveletBlock3DBufWriter(metafile, 0);
	}
	else {
		wbwriter = new WaveletBlock3DRegionWriter(metafile, 0);
	}
	if (wbwriter->GetErrCode() != 0) {
		cerr << ProgName << " : " << wbwriter->GetErrMsg() << endl;
		exit(1);
	}

	// Get a pointer to the Metdata object associated with
	// the WaveletBlock3DBufWriter object
	//
	metadata = wbwriter->GetMetadata();


	//
	// Open a variable for writing at the indicated time step
	//
	if (wbwriter->OpenVariableWrite(opt.ts, opt.varname, opt.level) < 0) {
		cerr << ProgName << " : " << wbwriter->GetErrMsg() << endl;
		exit(1);
	} 

	//
	// If pre version 2, create a backup of the .vdf file. The 
	// translation process will generate a new .vdf file
	//
	if (metadata->GetVDFVersion() < 2) save_file(metafile);

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

	// Get the dimensions of the volume
	//
	const size_t *dim = metadata->GetDimension();

	TIMER_START(t0);

	if (min[0] == min[1] && min[1] == min[2] && min[2] == max[0] &&
		max[0] == max[1]  && max[1] == max[2] && max[2] == (size_t) -1)
	{
		process_volume(
			(WaveletBlock3DBufWriter *) wbwriter, fp, dim, &read_timer
		);
	}
	else {
		process_region(
			(WaveletBlock3DRegionWriter *) wbwriter, fp, dim, &read_timer
		);
	}
	TIMER_STOP(t0,timer);


	// Close the variable. We're done writing.
	//
	wbwriter->CloseVariable();
	if (wbwriter->GetErrCode() != 0) {
		cerr << ProgName << ": " << wbwriter->GetErrMsg() << endl;
		exit(1);
	}

	if (! opt.quiet) {
		float	write_timer = wbwriter->GetWriteTimer();
		float	xform_timer = wbwriter->GetXFormTimer();
		const float *range = wbwriter->GetDataRange();

		fprintf(stdout, "read time : %f\n", read_timer);
		fprintf(stdout, "write time : %f\n", write_timer);
		fprintf(stdout, "transform time : %f\n", xform_timer);
		fprintf(stdout, "total transform time : %f\n", timer);
		fprintf(stdout, "min and max values of data output: %g, %g\n",range[0], range[1]);
	}

	// For pre-version 2 vdf files we need to write out the updated metafile. 
	// If we don't call this then
	// the .vdf file will not be updated with stats gathered from
	// the volume we just translated.
	//
	if (metadata->GetVDFVersion() < 2) {
		Metadata *m = (Metadata *) metadata;
		m->Write(metafile);
	}

	exit(0);
}

