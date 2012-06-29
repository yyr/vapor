#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>
#include <cerrno>
#include <cassert>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/WaveletBlock3DBufReader.h>
#include <vapor/WaveletBlock3DReader.h>
#include <vapor/WaveletBlock3DRegionReader.h>
#include <vapor/WaveCodecIO.h>

using namespace VetsUtil;
using namespace VAPoR;


struct opt_t {
	int	ts;
	char *varname;
	int	level;
	int	lod;
	int	nthreads;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	quiet;
	OptionParser::Boolean_T	block;
	OptionParser::IntRange_T xregion;
	OptionParser::IntRange_T yregion;
	OptionParser::IntRange_T zregion;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"ts",		1, 	"0","Timestep of data file starting from 0"},
	{"varname",	1, 	"var1",	"Name of variable"},
	{"level",1, "-1","Multiresolution refinement level. Zero implies coarsest resolution"},
	{"lod",1, "-1","Compression level of detail. Zero implies coarsest approximation"},
	{"nthreads",1, "0","Number of execution threads (0=># processors)"},
	{"help",	0,	"",	"Print this message and exit"},
	{"debug",	0,	"",	"Enable debugging"},
	{"quiet",	0,	"",	"Operate quietly"},
	{"block",	0,	"",	"Preserve internal blocking in 3D output file (deprecated!!!)"},
    {"xregion", 1, "-1:-1", "X dimension subregion bounds (min:max)"},
    {"yregion", 1, "-1:-1", "Y dimension subregion bounds (min:max)"},
    {"zregion", 1, "-1:-1", "Z dimension subregion bounds (min:max)"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"ts", VetsUtil::CvtToInt, &opt.ts, sizeof(opt.ts)},
	{"varname", VetsUtil::CvtToString, &opt.varname, sizeof(opt.varname)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"lod", VetsUtil::CvtToInt, &opt.lod, sizeof(opt.lod)},
	{"nthreads", VetsUtil::CvtToInt, &opt.nthreads, sizeof(opt.nthreads)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{"block", VetsUtil::CvtToBoolean, &opt.block, sizeof(opt.block)},
	{"xregion", VetsUtil::CvtToIntRange, &opt.xregion, sizeof(opt.xregion)},
	{"yregion", VetsUtil::CvtToIntRange, &opt.yregion, sizeof(opt.yregion)},
	{"zregion", VetsUtil::CvtToIntRange, &opt.zregion, sizeof(opt.zregion)},
	{NULL}
};

const char	*ProgName;


void	process_volume(
	VDFIOBase *vdfio, FILE *fp,  Metadata::VarType_T vtype,
	double *read_timer, double *xform_timer, double *write_timer,
	double *timer
) {
	size_t	size;
	float	*buf = NULL;
	size_t dim[3];
	double t0, t1;

	vdfio->GetDim(dim, opt.level);

	t0 = vdfio->GetTime();
	*timer = *write_timer = 0.0;

	size_t dim3d[3] = {0,0,0};

	switch (vtype) {
	case Metadata::VAR2D_XY:
		dim3d[0] = dim[0]; dim3d[1] = dim[1]; dim3d[2] = 1;
	break;

	case Metadata::VAR2D_XZ:
		dim3d[0] = dim[0]; dim3d[1] = dim[2]; dim3d[2] = 1;
	break;
	case Metadata::VAR2D_YZ:
		dim3d[0] = dim[1]; dim3d[1] = dim[2]; dim3d[2] = 1;
	break;
	case Metadata::VAR3D:
		dim3d[0] = dim[0]; dim3d[1] = dim[1]; dim3d[2] = dim[2];
	break;
	default:
	break;

	}
	
	size = dim3d[0] * dim3d[1];
	buf = new float[size];
	assert (buf != NULL);

	for(int z = 0; z<dim3d[2]; z++) {

		if (! opt.quiet && z%10 == 0) {
			cout << "Writing slice # " << z << endl; 
		}

		if (vdfio->ReadSlice(buf) < 0) {
			exit(1);
		} 

		t1 = vdfio->GetTime();
		if (fwrite(buf, sizeof(*buf), size, fp) != size) {
			MyBase::SetErrMsg("Could not write to output file : %M");
			exit(1);
		}
		*write_timer += vdfio->GetTime() - t1;
	}

	if (! opt.quiet) {
		fprintf(stdout, "Wrote %dx%dx%d volume\n", (int) dim3d[0],(int) dim3d[1],(int) dim3d[2]);
	}

	*timer = vdfio->GetTime() - t0;
	*xform_timer = vdfio->GetXFormTimer();
	*read_timer = vdfio->GetReadTimer();

	if (buf) delete [] buf;
}

void	process_region(
	VDFIOBase *vdfio, FILE *fp,  Metadata::VarType_T vtype,
	size_t min[3], size_t max[3],
	double *read_timer, double *xform_timer, double *write_timer,
	double *timer
) {
	size_t	size;
	float	*buf;
	size_t dim[3];
	double t0, t1;

	vdfio->GetDim(dim, opt.level);

	t0 = vdfio->GetTime();
	*timer = *write_timer = 0.0;

	size_t dim3d[3] = {0,0,0};

	size_t min3d[3], max3d[3];

	switch (vtype) {
	case Metadata::VAR2D_XY:
        min3d[0] = min[0] == (size_t) -1 ? 0 : min[0];
        max3d[0] = max[0] == (size_t) -1 ? dim[0] - 1 : max[0];
        min3d[1] = min[1] == (size_t) -1 ? 0 : min[1];
        max3d[1] = max[1] == (size_t) -1 ? dim[1] - 1 : max[1];
        min3d[2] = max3d[2] = 0;
	break;

	case Metadata::VAR2D_XZ:
        min3d[0] = min[0] == (size_t) -1 ? 0 : min[0];
        max3d[0] = max[0] == (size_t) -1 ? dim[0] - 1 : max[0];
        min3d[1] = min[2] == (size_t) -1 ? 0 : min[2];
        max3d[1] = max[2] == (size_t) -1 ? dim[2] - 1 : max[2];
        min3d[2] = max3d[2] = 0;
	break;

	case Metadata::VAR2D_YZ:
        min3d[0] = min[1] == (size_t) -1 ? 0 : min[1];
        max3d[0] = max[1] == (size_t) -1 ? dim[1] - 1 : max[1];
        min3d[1] = min[2] == (size_t) -1 ? 0 : min[2];
        max3d[1] = max[2] == (size_t) -1 ? dim[2] - 1 : max[2];
        min3d[2] = max3d[2] = 0;
	break;

	case Metadata::VAR3D:
        min3d[0] = min[0] == (size_t) -1 ? 0 : min[0];
        max3d[0] = max[0] == (size_t) -1 ? dim[0] - 1 : max[0];
        min3d[1] = min[1] == (size_t) -1 ? 0 : min[1];
        max3d[1] = max[1] == (size_t) -1 ? dim[1] - 1 : max[1];
        min3d[2] = min[2] == (size_t) -1 ? 0 : min[2];
        max3d[2] = max[2] == (size_t) -1 ? dim[2] - 1 : max[2];
	break;

	default:
	break;

	}
	dim3d[0] = max3d[0] - min3d[0] + 1;
	dim3d[1] = max3d[1] - min3d[1] + 1;
	dim3d[2] = max3d[2] - min3d[2] + 1;
	
	size = dim3d[0] * dim3d[1] * dim3d[2];
	buf = new float[size];
	assert (buf != NULL);

	if (vdfio->ReadRegion(min3d, max3d, buf) < 0) {
		exit(1);
	} 

	t1 = vdfio->GetTime();
	if (fwrite(buf, sizeof(*buf), size, fp) != size) {
		MyBase::SetErrMsg("Could not write to output file : %M");
		exit(1);
	}
	*write_timer += vdfio->GetTime() - t1;

	if (! opt.quiet) {
		fprintf(stdout, "Wrote %dx%dx%d volume\n", (int) dim3d[0],(int) dim3d[1],(int) dim3d[2]);
	}

	*timer = vdfio->GetTime() - t0;
	*xform_timer = vdfio->GetXFormTimer();
	*read_timer = vdfio->GetReadTimer();

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
	double	write_timer = 0.0;
	double	xform_timer = 0.0;
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

	if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);


	fp = FOPEN64(datafile, "wb");
	if (! fp) {
		MyBase::SetErrMsg("Could not open file \"%s\" : %M", datafile);
		exit(1);
	}

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

	WaveletBlockIOBase *wbreader = NULL;
	WaveCodecIO *wcreader = NULL;
	VDFIOBase *vdfio = NULL;

    bool vdc1 = (metadata.GetVDCType() == 1);
	if (vdc1) {
		if (min[0] == min[1] && min[1] == min[2] && min[2] == max[0] && 
			max[0] == max[1]  && max[1] == max[2] && max[2] == (size_t) -1 &&
			vtype == Metadata::VAR3D) {

			wbreader = new WaveletBlock3DBufReader(metadata);
		}
		else {
			wbreader = new WaveletBlock3DRegionReader(metadata);
		}
		vdfio = wbreader;
	}
	else {
		wcreader = new WaveCodecIO(metadata, opt.nthreads);
		vdfio = wcreader;
	}
	if (vdfio->GetErrCode() != 0) {
		exit(1);
    }

	if (vdfio->OpenVariableRead(opt.ts, opt.varname,opt.level, opt.lod) < 0) {
		exit(1);
	} 

	if (min[0] == min[1] && min[1] == min[2] && min[2] == max[0] &&
		max[0] == max[1]  && max[1] == max[2] && max[2] == (size_t) -1 &&
		vtype == Metadata::VAR3D) {

		process_volume(
			vdfio, fp, vtype, 
			&read_timer, &xform_timer, &write_timer, &timer
		);
	}
	else {
		process_region(
			vdfio, fp, vtype, min, max,
			&read_timer, &xform_timer, &write_timer, &timer
		);
	}

	fclose(fp);
	vdfio->CloseVariable();
	delete vdfio;

	if (! opt.quiet) {

		fprintf(stdout, "read time : %f\n", read_timer);
		fprintf(stdout, "write time : %f\n", write_timer);
		fprintf(stdout, "transform time : %f\n", xform_timer);
		fprintf(stdout, "total transform time : %f\n", timer);
	}

	exit(0);
}

