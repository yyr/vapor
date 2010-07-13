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
	{"help",	0,	"",	"Print this message and exit"},
	{"debug",	0,	"",	"Enable debugging"},
	{"quiet",	0,	"",	"Operate quietly"},
	{"block",	0,	"",	"Preserve internal blocking in 3D output file"},
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

void	process_volume_vdc1(
	MetadataVDC &metadata, FILE *fp,
	double *read_timer, double *xform_timer, double *write_timer,
	double *timer
) {
	size_t	size;
	float	*buf;
	WaveletBlock3DBufReader *wbreader3D = 0;
	size_t dim[3];
	double t0, t1;

	wbreader3D = new WaveletBlock3DBufReader(metadata);
	if (wbreader3D->GetErrCode() != 0) {
		exit(1);
	}
	if (wbreader3D->OpenVariableRead(opt.ts, opt.varname,opt.level) < 0) {
		exit(1);
	} 
	wbreader3D->GetDim(dim, opt.level);

	t0 = wbreader3D->GetTime();
	*timer = *write_timer = 0.0;

	if (! opt.block) {
		
		size = dim[0] * dim[1];
		buf = new float[size];
		assert (buf != NULL);

		for(int z = 0; z<dim[2]; z++) {

			if (! opt.quiet && z%10 == 0) {
				cerr << "Writing slice # " << z << endl; 
			}

			if (wbreader3D->ReadSlice(buf) < 0) {
				exit(1);
			} 

			t1 = wbreader3D->GetTime();
			if (fwrite(buf, sizeof(*buf), size, fp) != size) {
				MyBase::SetErrMsg("Could not write to output file : %M");
				exit(1);
			}
			*write_timer += wbreader3D->GetTime() - t1;
		}

		if (! opt.quiet) {
			fprintf(stdout, "Wrote %dx%dx%d volume\n", (int) dim[0],(int) dim[1],(int) dim[2]);
		}
	}
	else {
		size_t bdim[3];
		const size_t *bs = metadata.GetBlockSize();
		
		wbreader3D->GetDimBlk(bdim, opt.level);
		
		
		size = bdim[0]*bs[0] * bdim[1]*bs[1] * bs[2]*2;

		buf = new float[size];
		assert (buf != NULL);

		for(int z=0; z<bdim[2]; z+=2) {

			if (! opt.quiet) {
				cerr << "Writing slab # " << z << endl; 
			}
			if (wbreader3D->ReadSlabs(buf,0) < 0) {
				cerr << ProgName << " : " << wbreader3D->GetErrMsg() << endl;
				exit(1);
			}

			// Handle short write on last iteration (i.e. when odd Z dimension)
			//
			if (z+2 > bdim[2])  size = size >> 1;

			t1 = wbreader3D->GetTime();
			if (fwrite(buf, sizeof(buf[0]), size, fp) != size) {
				MyBase::SetErrMsg("Could not write to output file : %M");
				exit(1);
			}
			*write_timer += wbreader3D->GetTime() - t1;

		}
		size = bdim[0]*bs[0] * bdim[1]*bs[1];
		if (! opt.quiet) {
			fprintf(
				stdout, "Wrote %dx%dx%d volume\n", 
				(int) (bdim[0]*bs[0]),(int) (bdim[1]*bs[0]),(int) (bdim[2]*bs[0])
			);
		}
	}

	*timer = wbreader3D->GetTime() - t0;
	*xform_timer = wbreader3D->GetXFormTimer();
	*read_timer = wbreader3D->GetReadTimer();
	wbreader3D->CloseVariable();

	delete wbreader3D;
}

void	process_volume_vdc2(
	MetadataVDC &metadata, FILE *fp,  Metadata::VarType_T vtype,
	double *read_timer, double *xform_timer, double *write_timer,
	double *timer
) {
	size_t	size;
	float	*buf;
	WaveCodecIO *wcreader = NULL;
	size_t dim[3];
	double t0, t1;

	wcreader = new WaveCodecIO(metadata);
	if (wcreader->GetErrCode() != 0) {
		exit(1);
	}
	if (wcreader->OpenVariableRead(opt.ts, opt.varname,opt.level, opt.lod) < 0) {
		exit(1);
	} 
	wcreader->GetDim(dim, opt.level);

	t0 = wcreader->GetTime();
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

		if (wcreader->ReadSlice(buf) < 0) {
			exit(1);
		} 

		t1 = wcreader->GetTime();
		if (fwrite(buf, sizeof(*buf), size, fp) != size) {
			MyBase::SetErrMsg("Could not write to output file : %M");
			exit(1);
		}
		*write_timer += wcreader->GetTime() - t1;
	}

	if (! opt.quiet) {
		fprintf(stdout, "Wrote %dx%dx%d volume\n", (int) dim3d[0],(int) dim3d[1],(int) dim3d[2]);
	}

	*timer = wcreader->GetTime() - t0;
	*xform_timer = wcreader->GetXFormTimer();
	*read_timer = wcreader->GetReadTimer();
	wcreader->CloseVariable();

	delete wcreader;
}

void	process_region_vdc2(
	MetadataVDC &metadata, FILE *fp,  Metadata::VarType_T vtype,
	size_t min[3], size_t max[3],
	double *read_timer, double *xform_timer, double *write_timer,
	double *timer
) {
	size_t	size;
	float	*buf;
	WaveCodecIO *wcreader = NULL;
	size_t dim[3];
	double t0, t1;

	wcreader = new WaveCodecIO(metadata);
	if (wcreader->GetErrCode() != 0) {
		exit(1);
	}
	if (wcreader->OpenVariableRead(opt.ts, opt.varname,opt.level, opt.lod) < 0) {
		exit(1);
	} 
	wcreader->GetDim(dim, opt.level);

	t0 = wcreader->GetTime();
	*timer = *write_timer = 0.0;

	size_t dim3d[3] = {0,0,0};

	switch (vtype) {
	case Metadata::VAR2D_XY:
	case Metadata::VAR2D_XZ:
	case Metadata::VAR2D_YZ:
		dim3d[0] = max[0] - min[0] + 1;
		dim3d[1] = max[1] - min[1] + 1;
		dim3d[2] = 1;
	break;
	case Metadata::VAR3D:
		dim3d[0] = max[0] - min[0] + 1;
		dim3d[1] = max[1] - min[1] + 1;
		dim3d[2] = max[2] - min[2] + 1;
	break;
	default:
	break;

	}
	
	size = dim3d[0] * dim3d[1] * dim3d[2];
	buf = new float[size];
	assert (buf != NULL);

	if (wcreader->ReadRegion(min, max, buf) < 0) {
		exit(1);
	} 

	t1 = wcreader->GetTime();
	if (fwrite(buf, sizeof(*buf), size, fp) != size) {
		MyBase::SetErrMsg("Could not write to output file : %M");
		exit(1);
	}
	*write_timer += wcreader->GetTime() - t1;

	if (! opt.quiet) {
		fprintf(stdout, "Wrote %dx%dx%d volume\n", (int) dim3d[0],(int) dim3d[1],(int) dim3d[2]);
	}

	*timer = wcreader->GetTime() - t0;
	*xform_timer = wcreader->GetXFormTimer();
	*read_timer = wcreader->GetReadTimer();
	wcreader->CloseVariable();

	delete wcreader;
}

void	process_region(
	MetadataVDC &metadata, FILE *fp, Metadata::VarType_T vtype,
	double *read_timer, double *xform_timer, double *write_timer,
	double *timer
) {
	size_t	size;
	float	*buf;

	WaveletBlock3DRegionReader *wbreader3D;
	size_t dim[3];
	double t0, t1;

	wbreader3D = new WaveletBlock3DRegionReader(metadata);
	if (wbreader3D->GetErrCode() != 0) {
		exit(1);
	}
	if (wbreader3D->OpenVariableRead(opt.ts, opt.varname,opt.level) < 0) {
		exit(1);
	} 
	wbreader3D->GetDim(dim, opt.level);
	
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

	t0 = wbreader3D->GetTime();
	*timer = *write_timer = 0.0;

	if (! opt.block) {

		size_t rdim[3];
		for(int i=0; i<3; i++) {
			rdim[i] = max[i]-min[i]+1;
		}

		size = rdim[0] * rdim[1] * rdim[2];
		buf = new float[size];
		assert (buf != NULL);
		
		if (wbreader3D->ReadRegion(min,max,buf) < 0) {
			exit(1);
		} 

		t1 = wbreader3D->GetTime();
		if (fwrite(buf, sizeof(*buf), size, fp) != size) {
			MyBase::SetErrMsg("Could not write to output file : %M");
			exit(1);
		}
		*write_timer = wbreader3D->GetTime() - t1;

		if (! opt.quiet) {
			if (vtype == Metadata::VAR3D) {
				fprintf(stdout, "Wrote %dx%dx%d volume\n", (int) rdim[0],(int) rdim[1],(int) rdim[2]);
			}
			else {
				fprintf(stdout, "Wrote %dx%d plane\n", (int) rdim[0],(int) rdim[1]);
			}
		}
	}
	else {
		size_t bmin[3];
		size_t bmax[3];
		const size_t *bs = metadata.GetBlockSize();

		wbreader3D->MapVoxToBlk(min,bmin);
		wbreader3D->MapVoxToBlk(max,bmax);

		size_t bdim[3] = {
			bmax[0]-bmin[0]+1,
			bmax[1]-bmin[1]+1,
			bmax[2]-bmin[2]+1
		};

		if (vtype == Metadata::VAR3D) {
			size = bdim[0]*bs[0] * bdim[1]*bs[1] * bdim[2]*bs[2];
		}
		else {
			size = bdim[0]*bs[0] * bdim[1]*bs[1];
		}

		buf = new float[size];
		assert (buf != NULL);

		if (wbreader3D->BlockReadRegion(bmin,bmax,buf,0) < 0) {
			exit(1);
		} 

		t1 = wbreader3D->GetTime();
		if (fwrite(buf, sizeof(*buf), size, fp) != size) {
			MyBase::SetErrMsg("Could not write to output file : %M");
			exit(1);
		}
		*write_timer = wbreader3D->GetTime() - t1;

		if (! opt.quiet) {
			if (vtype == Metadata::VAR3D) {
				fprintf(
					stdout, "Wrote %dx%dx%d volume\n", 
					bdim[0]*bs[0], bdim[1]*bs[1],bdim[2]*bs[2]
				);
			}
			else {
				fprintf(
					stdout, "Wrote %dx%d plane\n", bdim[0]*bs[0], bdim[1]*bs[1]
				);
			}
		}
	}
	*timer = wbreader3D->GetTime() - t0;

	*xform_timer = wbreader3D->GetXFormTimer();
	*read_timer = wbreader3D->GetReadTimer();
	wbreader3D->CloseVariable();
	delete wbreader3D;
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

    bool vdc1 = (metadata.GetVDCType() == 1);

	if (vdc1) {
		if (min[0] == min[1] && min[1] == min[2] && min[2] == max[0] && 
			max[0] == max[1]  && max[1] == max[2] && max[2] == (size_t) -1 &&
			vtype == Metadata::VAR3D) {

			process_volume_vdc1(
				metadata, fp, &read_timer, &xform_timer, &write_timer, &timer
			);
		}
		else {
			process_region(
				metadata, fp, vtype, &read_timer, &xform_timer, &write_timer, &timer
			);
		}
	}
	else {
		if (min[0] == min[1] && min[1] == min[2] && min[2] == max[0] && 
			max[0] == max[1]  && max[1] == max[2] && max[2] == (size_t) -1) {

			process_volume_vdc2(
				metadata, fp, vtype, 
				&read_timer, &xform_timer, &write_timer, &timer
			);
		}
		else {
			process_region_vdc2(
				metadata, fp, vtype, min, max,
				&read_timer, &xform_timer, &write_timer, &timer
			);
		}
	}

	fclose(fp);

	if (! opt.quiet) {

		fprintf(stdout, "read time : %f\n", read_timer);
		fprintf(stdout, "write time : %f\n", write_timer);
		fprintf(stdout, "transform time : %f\n", xform_timer);
		fprintf(stdout, "total transform time : %f\n", timer);
	}

	exit(0);
}

