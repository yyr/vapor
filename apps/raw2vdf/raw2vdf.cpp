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
	int nxforms;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
	OptionParser::Boolean_T	swapbytes;
	OptionParser::Boolean_T	dbl;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"ts",		1, 	"0","Timestep of data file starting from 0"},
	{"varname",	1, 	"var1",	"Name of variable"},
	{"nxforms",	1, 	"0",	"Transformation levels saved. 0=>all, 1=>all but finest, etc."},
	{"help",	0,	"",	"Print this message and exit"},
	{"quiet",	0,	"",	"Operate quietly"},
	{"swapbytes",	0,	"",	"Swap bytes in raw data as they are read from disk"},
	{"dbl",	0,	"",	"Input data are 64-bit floats"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"ts", VetsUtil::CvtToInt, &opt.ts, sizeof(opt.ts)},
	{"varname", VetsUtil::CvtToString, &opt.varname, sizeof(opt.varname)},
	{"nxforms", VetsUtil::CvtToInt, &opt.nxforms, sizeof(opt.nxforms)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{"swapbytes", VetsUtil::CvtToBoolean, &opt.swapbytes, sizeof(opt.swapbytes)},
	{"dbl", VetsUtil::CvtToBoolean, &opt.dbl, sizeof(opt.dbl)},
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

	
void    calcMaxMin(
	float* data,
	int	n,
	float* maxVal,
	float* minVal
) {

	int j;
	float mx = *maxVal;
	float mn = *minVal;
	for (j=0; j<n; j++) {
		if(data[j] > mx) mx = data[j];
		if(data[j] < mn) mn = data[j];
	}
	*maxVal = mx;
	*minVal = mn;
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
	float maxData = -1.e35f;
	float minData = 1.e35f;

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


	//
	// Create a buffered WaveletBlock writer. Initialize with
	// path to .vdf file
	//
	WaveletBlock3DBufWriter	wbwriter(metafile, 0);
	if (wbwriter.GetErrCode() != 0) {
		cerr << ProgName << " : " << wbwriter.GetErrMsg() << endl;
		exit(1);
	}

	// Get a pointer to the Metdata object associated with
	// the WaveletBlock3DBufWriter object
	//
	metadata = wbwriter.GetMetadata();


	//
	// Open a variable for writing at the indicated time step
	//
	if (wbwriter.OpenVariableWrite(opt.ts, opt.varname, opt.nxforms) < 0) {
		cerr << ProgName << " : " << wbwriter.GetErrMsg() << endl;
		exit(1);
	} 

	//
	// Create a backup of the .vdf file. The translation process will
	// generate a new .vdf file
	//
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

	// Get the dimensions of the volume
	//
	const size_t *dim = metadata->GetDimension();

	// Allocate a buffer large enough to hold one slice of data
	//
	int element_sz;
	if (opt.dbl) element_sz = sizeof(double);
	else element_sz = sizeof (float);

	unsigned char *slice = new unsigned char [dim[0]*dim[1]*element_sz];


	//
	// Translate the volume one slice at a time
	//
	TIMER_START(t0);
	for(int z=0; z<dim[2]; z++) {

		if (z%10== 0 && ! opt.quiet) {
			cout << "Reading slice # " << z << endl;
		}

		TIMER_START(t1);
		rc = fread(slice, element_sz, dim[0]*dim[1], fp);
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
		if(!opt.quiet){
			calcMaxMin((float *) slice, dim[0]*dim[1], &maxData, &minData);
		}
		//
		// Write a single slice of data
		//
		wbwriter.WriteSlice((float *) slice);
		if (wbwriter.GetErrCode() != 0) {
			cerr << ProgName << ": " << wbwriter.GetErrMsg() << endl;
			exit(1);
		}

	}
	TIMER_STOP(t0,timer);

	// Close the variable. We're done writing.
	//
	wbwriter.CloseVariable();

	if (! opt.quiet) {
		float	write_timer = wbwriter.GetWriteTimer();
		float	xform_timer = wbwriter.GetXFormTimer();

		fprintf(stdout, "read time : %f\n", read_timer);
		fprintf(stdout, "write time : %f\n", write_timer);
		fprintf(stdout, "transform time : %f\n", xform_timer - write_timer);
		fprintf(stdout, "total transform time : %f\n", timer);
		fprintf(stdout, "min and max values of data output: %g, %g\n",minData, maxData);
	}

	// Write out the updated metafile. If we don't call this then
	// the .vdf file will not be updated with stats gathered from
	// the volume we just translated.
	//
	metadata->Write(metafile);

	exit(0);
}

