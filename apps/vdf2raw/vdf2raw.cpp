#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>
#include <cerrno>
#include <cassert>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/Metadata.h>
#include <vapor/WaveletBlock3DBufReader.h>
#include <vapor/WaveletBlock3DReader.h>
#include <vapor/WaveletBlock3DRegionReader.h>

using namespace VetsUtil;
using namespace VAPoR;


struct opt_t {
	int	ts;
	char *varname;
	int	level;
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
	{"level",1, "0","Multiresution refinement level. Zero implies coarsest resolution"},
	{"help",	0,	"",	"Print this message and exit"},
	{"debug",	0,	"",	"Enable debugging"},
	{"quiet",	0,	"",	"Operate quitely"},
	{"block",	0,	"",	"Preserve internal blockign in output file"},
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
	{"block", VetsUtil::CvtToBoolean, &opt.block, sizeof(opt.block)},
	{"xregion", VetsUtil::CvtToIntRange, &opt.xregion, sizeof(opt.xregion)},
	{"yregion", VetsUtil::CvtToIntRange, &opt.yregion, sizeof(opt.yregion)},
	{"zregion", VetsUtil::CvtToIntRange, &opt.zregion, sizeof(opt.zregion)},
	{NULL}
};

const char	*ProgName;

void	process_volume(
	const char *metafile, FILE *fp,
	double *read_timer, double *xform_timer, double *write_timer
) {
	size_t	size;
	float	*buf;
	const Metadata *metadata;

	if (! opt.block) {
		size_t dim[3];

		WaveletBlock3DBufReader	wbreader(metafile, 0);
		if (wbreader.GetErrCode() != 0) {
			cerr << ProgName << " : " << wbreader.GetErrMsg() << endl;
			exit(1);
		}
		metadata = wbreader.GetMetadata();

		if (wbreader.OpenVariableRead(opt.ts, opt.varname,opt.level) < 0) {
			cerr << ProgName << " : " << wbreader.GetErrMsg() << endl;
			exit(1);
		} 

		wbreader.GetDim(dim, opt.level);
		size = dim[0] * dim[1];
		buf = new float[size];
		assert (buf != NULL);

		for(int z = 0; z<dim[2]; z++) {

			if (! opt.quiet && z%10 == 0) {
				cerr << "Writing slice # " << z << endl; 
			}

			if (wbreader.ReadSlice(buf) < 0) {
				cerr << ProgName << " : " << wbreader.GetErrMsg() << endl;
				exit(1);
			} 
			TIMER_START(t0);
			if (fwrite(buf, sizeof(*buf), size, fp) != size) {
				cerr << ProgName << ": Could not write to output file : " 
					<< strerror(errno) << endl;
				exit(1);
			}
			TIMER_STOP(t0, *write_timer);
		}

		*xform_timer = wbreader.GetXFormTimer();
		*read_timer = wbreader.GetReadTimer();
		wbreader.CloseVariable();

		if (! opt.quiet) {
			fprintf(stdout, "Wrote %dx%dx%d volume\n", dim[0],dim[1],dim[2]);
		}
	}
	else {
		size_t bdim[3];

		WaveletBlock3DBufReader	wbreader(metafile, 0);
		if (wbreader.GetErrCode() != 0) {
			cerr << ProgName << " : " << wbreader.GetErrMsg() << endl;
			exit(1);
		}
		metadata = wbreader.GetMetadata();
		const size_t *bs = metadata->GetBlockSize();

		if (wbreader.OpenVariableRead(opt.ts, opt.varname,opt.level) < 0) {
			cerr << ProgName << " : " << wbreader.GetErrMsg() << endl;
			exit(1);
		} 

		wbreader.GetDimBlk(bdim, opt.level);
		
		size = bdim[0]*bs[0] * bdim[1]*bs[1] * bs[2]*2;

		buf = new float[size];
		assert (buf != NULL);

		for(int z=0; z<bdim[2]; z+=2) {

			if (! opt.quiet) {
				cerr << "Writing slab # " << z << endl; 
			}

			if (wbreader.ReadSlabs(buf,0) < 0) {
				cerr << ProgName << " : " << wbreader.GetErrMsg() << endl;
				exit(1);
			}

			// Handle short write on last iteration (i.e. when odd Z dimension)
			//
			if (z+2 > bdim[2])  size = size >> 1;

			TIMER_START(t0);

			if (fwrite(buf, sizeof(buf[0]), size, fp) != size) {
				cerr << ProgName << ": Could not write to output file : " 
					<< strerror(errno) << endl;
				exit(1);
			}

			TIMER_STOP(t0, *write_timer);
		}
		*xform_timer = wbreader.GetXFormTimer();
		*read_timer = wbreader.GetReadTimer();
		wbreader.CloseVariable();

		size = bdim[0]*bs[0] * bdim[1]*bs[1];
		if (! opt.quiet) {
			fprintf(
				stdout, "Wrote %dx%dx%d volume\n", 
				bdim[0]*bs[0],bdim[1]*bs[0],bdim[2]*bs[0]
			);
		}
	}
}

void	process_region(
	const char *metafile, FILE *fp,
	double *read_timer, double *xform_timer, double *write_timer
) {
	size_t	size;
	float	*buf;
	const Metadata *metadata;


	WaveletBlock3DRegionReader	wbreader(metafile, 0);
	if (wbreader.GetErrCode() != 0) {
		cerr << ProgName << " : " << wbreader.GetErrMsg() << endl;
		exit(1);
	}
	metadata = wbreader.GetMetadata();

	if (wbreader.OpenVariableRead(opt.ts, opt.varname,opt.level) < 0) {
		cerr << ProgName << " : " << wbreader.GetErrMsg() << endl;
		exit(1);
	} 

	size_t dim[3];
	wbreader.GetDim(dim, opt.level);
	size_t min[3] = {opt.xregion.min, opt.yregion.min, opt.zregion.min};
	size_t max[3] = {opt.xregion.max, opt.yregion.max, opt.zregion.max};

	for(int i=0; i<3; i++) {
		if (min[i] == (size_t) -1)  min[i] = 0;
		if (max[i] == (size_t) -1)  max[i] = dim[i]-1;
	}


	if (! opt.block) {
		size_t dim[3] = {max[0]-min[0]+1, max[1]-min[1]+1, max[2]-min[2]+1};

		size = dim[0] * dim[1] * dim[2];
		buf = new float[size];
		assert (buf != NULL);

		if (wbreader.ReadRegion(min,max,buf) < 0) {
			cerr << ProgName << " : " << wbreader.GetErrMsg() << endl;
			exit(1);
		} 

		TIMER_START(t0);

		if (fwrite(buf, sizeof(*buf), size, fp) != size) {
			cerr << ProgName << ": Could not write to output file : " 
				<< strerror(errno) << endl;
			exit(1);
		}

		TIMER_STOP(t0, *write_timer);

		if (! opt.quiet) {
			fprintf(stdout, "Wrote %dx%dx%d volume\n", dim[0],dim[1],dim[2]);
		}
	}
	else {
		size_t bmin[3];
		size_t bmax[3];
		const size_t *bs = metadata->GetBlockSize();

		wbreader.MapVoxToBlk(min,bmin);
		wbreader.MapVoxToBlk(max,bmax);

		size_t bdim[3] = {
			bmax[0]-bmin[0]+1,
			bmax[1]-bmin[1]+1,
			bmax[2]-bmin[2]+1
		};

		size = bdim[0]*bs[0] * bdim[1]*bs[1] * bdim[2]*bs[2];
		buf = new float[size];
		assert (buf != NULL);

		if (wbreader.BlockReadRegion(bmin,bmax,buf,0) < 0) {
			cerr << ProgName << " : " << wbreader.GetErrMsg() << endl;
			exit(1);
		} 

		TIMER_START(t0);

		if (fwrite(buf, sizeof(*buf), size, fp) != size) {
			cerr << ProgName << ": Could not write to output file : " 
				<< strerror(errno) << endl;
			exit(1);
		}

		TIMER_STOP(t0, *write_timer);

		if (! opt.quiet) {
			fprintf(
				stdout, "Wrote %dx%dx%d volume\n", 
				bdim[0]*bs[0],bdim[1]*bs[0],bdim[2]*bs[0]
			);
		}
	}
	*xform_timer = wbreader.GetXFormTimer();
	*read_timer = wbreader.GetReadTimer();
	wbreader.CloseVariable();
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
		cerr << ProgName << ": Could not open file \"" << 
			datafile << "\" : " <<strerror(errno) << endl;

		exit(1);
	}

	size_t min[3] = {opt.xregion.min, opt.yregion.min, opt.zregion.min};
	size_t max[3] = {opt.xregion.max, opt.yregion.max, opt.zregion.max};

	TIMER_START(t0);

	if (min[0] == min[1] && min[1] == min[2] && min[2] == max[0] && 
		max[0] == max[1]  && max[1] == max[2] && max[2] == (size_t) -1) 
	{
		process_volume(metafile, fp, &read_timer, &xform_timer, &write_timer);
	}
	else {
		process_region(metafile, fp, &read_timer, &xform_timer, &write_timer);
	}

	fclose(fp);

	TIMER_STOP(t0,timer);

	if (! opt.quiet) {

		fprintf(stdout, "read time : %f\n", read_timer);
		fprintf(stdout, "write time : %f\n", write_timer);
		fprintf(stdout, "transform time : %f\n", xform_timer);
		fprintf(stdout, "total transform time : %f\n", timer);
	}

	exit(0);
}

