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
#include <vapor/MetadataVDC.h>
#include <vapor/WaveletBlock3DBufWriter.h>
#include <vapor/WaveletBlock3DRegionWriter.h>
#include <vapor/WaveletBlock2DRegionWriter.h>
#ifdef WIN32
#include "windows.h"
#endif

using namespace VetsUtil;
using namespace VAPoR;


//
//	Command line argument stuff
//
struct opt_t {
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
	int staggeredDim;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"ts",		1, 	"0","Timestep of data file starting from 0"},
	{"varname",	1, 	"var1",	"Name of variable"},
	{"level",	1, 	"-1",	"Refinement levels saved. 0 => coarsest, 1 => "
		"next refinement, etc. -1 => all levels defined by the .vdf file"},
	{"help",	0,	"",	"Print this message and exit"},
	{"debug",	0,	"",	"Enable debugging"},
	{"quiet",	0,	"",	"Operate quietly"},
	{"swapbytes",	0,	"",	"Swap bytes in raw data as they are read from disk"},
	{"dbl",	0,	"",	"Input data are 64-bit floats"},
	{"xregion", 1, "-1:-1", "X dimension subregion bounds (min:max)"},
	{"yregion", 1, "-1:-1", "Y dimension subregion bounds (min:max)"},
	{"zregion", 1, "-1:-1", "Z dimension subregion bounds (min:max)"},
	{"stagdim", 1, "0", "1, 2, or 3 indicate staggered in x, y, or z"},
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
	{"stagdim", VetsUtil::CvtToInt, &opt.staggeredDim, sizeof(opt.staggeredDim)},
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
	double *read_timer
) {
	// Get the dimensions of the volume
	//
	const size_t *dim = bufwriter->GetDimension();

	double t0;

	// Allocate a buffer large enough to hold one slice of data,
	// plus one if staggered.
	//
	int element_sz;
	if (opt.dbl) element_sz = sizeof(double);
	else element_sz = sizeof (float);

	//dimx and dimy are the size of the input data:
	size_t dimx,dimy;
	dimx = (opt.staggeredDim == 1) ? dim[0]+1 : dim[0];
	dimy = (opt.staggeredDim == 2) ? dim[1]+1 : dim[1];
	unsigned char *slice = new unsigned char [dimx*dimy*element_sz];
	unsigned char *slice1 = 0;
	//Allocate extra slice if z-staggered:
	if (opt.staggeredDim == 3) slice1 = new unsigned char [dimx*dimy*element_sz];

	//
	// Translate the volume one slice at a time, if z not staggered
	//
	if (opt.staggeredDim != 3){
		for(int z=0; z<dim[2]; z++) {

			if (z%10== 0 && ! opt.quiet) {
				cout << "Reading slice # " << z << endl;
			}

			t0 = bufwriter->GetTime();
			int rc = fread(slice, element_sz, dimx*dimy, fp);
			if (rc != dimx*dimy) {
				if (rc<0) {
					cerr << ProgName << ": Error reading input file : " << 
						strerror(errno) << endl;
				}
				else {
					cerr << ProgName << ": short read" << endl;
				}
				exit(1);
			}

			*read_timer += bufwriter->GetTime() - t0;

			//
			// If the data stored on disk are byte swapped relative
			// to the machine we're running on...
			//
			if (opt.swapbytes) {
				swapbytes(slice, element_sz, dimx*dimy); 
			}

			// Convert data from double to float if needed.
			if (opt.dbl) {
				float *fptr = (float *) slice;
				double *dptr = (double *) slice;
				for(int i=0; i<dimx*dimy; i++) *fptr++ = (float) *dptr++;
			}
			// If staggered in x, average to smaller array.  Need to
			// shrink (in x) as we go, bypassing
			//
			float* fslice = (float*)slice;
			if (opt.staggeredDim == 1){
				size_t inposn = 0;
				//Loop over output positions:
				for (int j = 0; j<dim[1]; j++){
					for (int i = 0; i< dim[0]; i++){
						fslice[i+dim[0]*j] = 
							0.5*(fslice[inposn]+fslice[inposn+1]);
						inposn++;
					}
					//At end of row, skip one position:
					inposn++;
				}
				//at the end, the inposn should be one past the end of the data:
				assert(inposn == dimx*dimy);
			}
			
			//
			// If staggered in y, average each row, ignore the
			// last row
			//
			else if (opt.staggeredDim == 2){
				for (int j = 0; j<dim[1]; j++){
					for (int i = 0; i<dim[0]; i++){
						fslice[i+dim[0]*j] =
							(fslice[i+dim[0]*j]+fslice[i+dim[0]*(j+1)])*0.5;
					}
				}
			}
			// Write a single slice of data
			//
			bufwriter->WriteSlice((float *) slice);
			if (bufwriter->GetErrCode() != 0) {
				cerr << ProgName << ": " << bufwriter->GetErrMsg() << endl;
				exit(1);
			}
		} 
	} else { //Deal with staggered in z dimension
		unsigned char* oldslice = slice;
		unsigned char* newslice = slice1;
		//Read ahead one slice:

		t0 = bufwriter->GetTime();

		int rc = fread(newslice, element_sz, dim[0]*dim[1], fp);
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

		*read_timer += bufwriter->GetTime() - t0;

		if (opt.swapbytes) {
			swapbytes(newslice, element_sz, dim[0]*dim[1]); 
		}

		// Convert data from double to float if needed.
		if (opt.dbl) {
			float *fptr = (float *) newslice;
			double *dptr = (double *) newslice;
			for(int i=0; i<dim[0]*dim[1]; i++) *fptr++ = (float) *dptr++;
		}
		//read one slice ahead
		for(int z=0; z<dim[2]; z++) {
			//swap old and new slice arrays:
			unsigned char* swapslice = oldslice;
			oldslice = newslice;
			newslice = swapslice;

			if (z%10== 0 && ! opt.quiet) {
				cout << "Reading slice # " << z << endl;
			}

			t0 = bufwriter->GetTime();

			int rc = fread(newslice, element_sz, dim[0]*dim[1], fp);
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
			*read_timer += bufwriter->GetTime() - t0;

			//
			// If the data stored on disk are byte swapped relative
			// to the machine we're running on...
			//
			if (opt.swapbytes) {
				swapbytes(newslice, element_sz, dim[0]*dim[1]); 
			}

			// Convert data from double to float if needed.
			if (opt.dbl) {
				float *fptr = (float *) newslice;
				double *dptr = (double *) newslice;
				for(int i=0; i<dim[0]*dim[1]; i++) *fptr++ = (float) *dptr++;
			}
			//Average old and new slices:
			float* outFloat = (float*) oldslice;
			float* newFloat = (float*) newslice;
			for (int i = 0; i< dim[0]*dim[1]; i++){
				*outFloat = 0.5*(*outFloat + *newFloat);
				outFloat++;
				newFloat++;
			}
			//
			// Write a single slice of data
			//
			bufwriter->WriteSlice((float *) oldslice);
			if (bufwriter->GetErrCode() != 0) {
				cerr << ProgName << ": " << bufwriter->GetErrMsg() << endl;
				exit(1);
			}
		}

	}
}


void	process_region(
	WaveletBlock3DRegionWriter *regwriter,
	FILE	*fp, 
	Metadata::VarType_T vtype,
	double *read_timer
) {


	//Check that we are not doing staggered dimensions:
	if( opt.staggeredDim != 0){
		cerr << ProgName << ": " << "Staggered dimensions not supported for subregion" << endl;
				exit(1);
	}

	// Get the dimensions of the volume
	//
	const size_t *dim = regwriter->GetDimension();

	double t0;

	size_t min[3];
	size_t max[3];

	switch (vtype) {
	case Metadata::VAR2D_XY:
		min[0] = opt.xregion.min == (size_t) -1 ? 0 : opt.xregion.min;
		max[0] = opt.xregion.max == (size_t) -1 ? dim[0] - 1 : opt.xregion.max;
		min[1] = opt.yregion.min == (size_t) -1 ? 0 : opt.yregion.min;
		max[1] = opt.yregion.max == (size_t) -1 ? dim[1] - 1 : opt.yregion.max;
		min[2] = max[2] = 0;
	break;

	case Metadata::VAR2D_XZ:
		min[0] = opt.xregion.min == (size_t) -1 ? 0 : opt.xregion.min;
		max[0] = opt.xregion.max == (size_t) -1 ? dim[0] - 1 : opt.xregion.max;
		min[1] = opt.zregion.min == (size_t) -1 ? 0 : opt.zregion.min;
		max[1] = opt.zregion.max == (size_t) -1 ? dim[2] - 1 : opt.zregion.max;
		min[2] = max[2] = 0;
	break;
	case Metadata::VAR2D_YZ:
		min[0] = opt.yregion.min == (size_t) -1 ? 0 : opt.yregion.min;
		max[0] = opt.yregion.max == (size_t) -1 ? dim[1] - 1 : opt.yregion.max;
		min[1] = opt.zregion.min == (size_t) -1 ? 0 : opt.zregion.min;
		max[1] = opt.zregion.max == (size_t) -1 ? dim[2] - 1 : opt.zregion.max;
		min[2] = max[2] = 0;
	break;
	case Metadata::VAR3D:
		min[0] = opt.xregion.min == (size_t) -1 ? 0 : opt.xregion.min;
		max[0] = opt.xregion.max == (size_t) -1 ? dim[0] - 1 : opt.xregion.max;
		min[1] = opt.yregion.min == (size_t) -1 ? 0 : opt.yregion.min;
		max[1] = opt.yregion.max == (size_t) -1 ? dim[1] - 1 : opt.yregion.max;
		min[2] = opt.zregion.min == (size_t) -1 ? 0 : opt.zregion.min;
		max[2] = opt.zregion.max == (size_t) -1 ? dim[2] - 1 : opt.zregion.max;
	break;
	default:
	break;

	}

	
	size_t rdim[3];
	for(int i=0; i<3; i++) {
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

		t0 = regwriter->GetTime();
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
		*read_timer += regwriter->GetTime() - t0;

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

void ErrMsgCBHandler(const char *msg, int) {
	cerr << ProgName << " : " << msg << endl;
}


int	main(int argc, char **argv) {

	OptionParser op;
	FILE	*fp;
	const char	*metafile;
	const char	*datafile;

	double	timer = 0.0;
	double	read_timer = 0.0;
	string	s;

	MyBase::SetErrMsgCB(ErrMsgCBHandler);

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

	WaveletBlockIOBase	*wbwriter3D;

	size_t min[3] = {opt.xregion.min, opt.yregion.min, opt.zregion.min};
	size_t max[3] = {opt.xregion.max, opt.yregion.max, opt.zregion.max};

	// Determine if variable is 3D
	//
	MetadataVDC metadata (metafile);
	if (MetadataVDC::GetErrCode() != 0) {
		MyBase::SetErrMsg("Error processing metafile \"%s\"", metafile);
		exit(1);
	}
	Metadata::VarType_T vtype = metadata.GetVarType(opt.varname);
	if (vtype == Metadata::VARUNKNOWN) {
		MyBase::SetErrMsg("Unknown variable \"%s\"", opt.varname);
		exit(1);
	}


	// Create an appropriate WaveletBlock writer. 
	//
	if (min[0] == min[1] && min[1] == min[2] && min[2] == max[0] &&
		max[0] == max[1]  && max[1] == max[2] && max[2] == (size_t) -1 &&
		vtype == Metadata::VAR3D) {

		wbwriter3D = new WaveletBlock3DBufWriter(metadata);
	}
	else {
		wbwriter3D = new WaveletBlock3DRegionWriter(metadata);
	}
	if (wbwriter3D->GetErrCode() != 0) {
		exit(1);
	}


	//
	// Open a variable for writing at the indicated time step
	//
	if (wbwriter3D->OpenVariableWrite(opt.ts, opt.varname, opt.level) < 0) {
		exit(1);
	} 

	//
	// If pre version 2, create a backup of the .vdf file. The 
	// translation process will generate a new .vdf file
	//
	if (wbwriter3D->GetVDFVersion() < 2) save_file(metafile);

	fp = FOPEN64(datafile, "rb");
	if (! fp) {
		MyBase::SetErrMsg("Could not open file \"%s\" : %M", datafile);
		exit(1);
	}


	double t0 = wbwriter3D->GetTime();

	if (min[0] == min[1] && min[1] == min[2] && min[2] == max[0] &&
		max[0] == max[1]  && max[1] == max[2] && max[2] == (size_t) -1 &&
		vtype == Metadata::VAR3D) {

		process_volume(
			(WaveletBlock3DBufWriter *) wbwriter3D, fp, &read_timer
		);
	}
	else {
		process_region(
			(WaveletBlock3DRegionWriter *) wbwriter3D, fp, vtype, &read_timer
		);
	}


	// Close the variable. We're done writing.
	//
	wbwriter3D->CloseVariable();
	if (wbwriter3D->GetErrCode() != 0) {
		exit(1);
	}

	timer = wbwriter3D->GetTime() - t0;

	if (! opt.quiet) {
		float	write_timer = wbwriter3D->GetWriteTimer();
		float	xform_timer = wbwriter3D->GetXFormTimer();
		const float *range = wbwriter3D->GetDataRange();

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
	if (wbwriter3D->GetVDFVersion() < 2) {
		wbwriter3D->Write(metafile);
	}

	exit(0);
}

