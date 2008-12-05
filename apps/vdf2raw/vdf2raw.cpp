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
#include <vapor/WaveletBlock2DRegionReader.h>

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
	{"level",1, "0","Multiresolution refinement level. Zero implies coarsest resolution"},
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
	const char *metafile, FILE *fp, bool is3D,
	double *read_timer, double *xform_timer, double *write_timer
) {
	size_t	size;
	float	*buf;
	const Metadata *metadata;
	WaveletBlock3DBufReader *wbreader3D = 0;
	WaveletBlock2DRegionReader *wbreader2D = 0;
	size_t dim[3];
	if (is3D) {
		wbreader3D = new WaveletBlock3DBufReader(metafile, 0);
		if (wbreader3D->GetErrCode() != 0) {
			cerr << ProgName << " : " << wbreader3D->GetErrMsg() << endl;
			exit(1);
		}
		metadata = wbreader3D->GetMetadata();
		if (wbreader3D->OpenVariableRead(opt.ts, opt.varname,opt.level) < 0) {
			cerr << ProgName << " : " << wbreader3D->GetErrMsg() << endl;
			exit(1);
		} 
		wbreader3D->GetDim(dim, opt.level);
	} else {
		wbreader2D = new WaveletBlock2DRegionReader(metafile);
		if (wbreader2D->GetErrCode() != 0) {
			cerr << ProgName << " : " << wbreader2D->GetErrMsg() << endl;
			exit(1);
		}
		metadata = wbreader2D->GetMetadata();
		if (wbreader2D->OpenVariableRead(opt.ts, opt.varname,opt.level) < 0) {
			cerr << ProgName << " : " << wbreader2D->GetErrMsg() << endl;
			exit(1);
		} 
		wbreader2D->GetDim(dim, opt.level);
	}
	if (!is3D) {
		if (opt.block){
			cerr << "Block option ignored with 2D data" << endl;
		}
		size = dim[0] * dim[1];
		buf = new float[size];
		assert (buf != NULL);

		if (wbreader2D->ReadRegion(buf) < 0) {
			cerr << ProgName << " : " << wbreader2D->GetErrMsg() << endl;
			exit(1);
		} 
		TIMER_START(t0);
		if (fwrite(buf, sizeof(*buf), size, fp) != size) {
			cerr << ProgName << ": Could not write to output file : " 
				<< strerror(errno) << endl;
			exit(1);
		}
		TIMER_STOP(t0, *write_timer);

		*xform_timer = wbreader2D->GetXFormTimer();
		*read_timer = wbreader2D->GetReadTimer();
		wbreader2D->CloseVariable();

		if (! opt.quiet) {
			fprintf(stdout, "Wrote %dx%d plane\n", dim[0],dim[1]);
		}
		delete wbreader2D;
		return;
	}
	if (! opt.block) {
		
		size = dim[0] * dim[1];
		buf = new float[size];
		assert (buf != NULL);

		for(int z = 0; z<dim[2]; z++) {

			if (! opt.quiet && z%10 == 0) {
				cerr << "Writing slice # " << z << endl; 
			}

			if (wbreader3D->ReadSlice(buf) < 0) {
				cerr << ProgName << " : " << wbreader3D->GetErrMsg() << endl;
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

		*xform_timer = wbreader3D->GetXFormTimer();
		*read_timer = wbreader3D->GetReadTimer();
		wbreader3D->CloseVariable();

		if (! opt.quiet) {
			fprintf(stdout, "Wrote %dx%dx%d volume\n", dim[0],dim[1],dim[2]);
		}
	}
	else {
		size_t bdim[3];
		const size_t *bs = metadata->GetBlockSize();
		
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

			TIMER_START(t0);

			if (fwrite(buf, sizeof(buf[0]), size, fp) != size) {
				cerr << ProgName << ": Could not write to output file : " 
					<< strerror(errno) << endl;
				exit(1);
			}

			TIMER_STOP(t0, *write_timer);
		}
		*xform_timer = wbreader3D->GetXFormTimer();
		*read_timer = wbreader3D->GetReadTimer();
		wbreader3D->CloseVariable();
		delete wbreader3D;
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
	const char *metafile, FILE *fp, bool is3D,
	double *read_timer, double *xform_timer, double *write_timer
) {
	size_t	size;
	float	*buf;
	const Metadata *metadata;

	WaveletBlock3DRegionReader *wbreader3D = 0;
	WaveletBlock2DRegionReader *wbreader2D = 0;
	size_t dim[3];
	if (is3D) {
		wbreader3D = new WaveletBlock3DRegionReader(metafile, 0);
		if (wbreader3D->GetErrCode() != 0) {
			cerr << ProgName << " : " << wbreader3D->GetErrMsg() << endl;
			exit(1);
		}
		metadata = wbreader3D->GetMetadata();
		if (wbreader3D->OpenVariableRead(opt.ts, opt.varname,opt.level) < 0) {
			cerr << ProgName << " : " << wbreader3D->GetErrMsg() << endl;
			exit(1);
		} 
		wbreader2D->GetDim(dim, opt.level);
	} else {
		wbreader2D = new WaveletBlock2DRegionReader(metafile);
		if (wbreader2D->GetErrCode() != 0) {
			cerr << ProgName << " : " << wbreader2D->GetErrMsg() << endl;
			exit(1);
		}
		metadata = wbreader2D->GetMetadata();
		if (wbreader2D->OpenVariableRead(opt.ts, opt.varname,opt.level) < 0) {
			cerr << ProgName << " : " << wbreader2D->GetErrMsg() << endl;
			exit(1);
		} 
		wbreader2D->GetDim(dim, opt.level);
	}
	
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
		
		if (is3D){
			if (wbreader3D->ReadRegion(min,max,buf) < 0) {
				cerr << ProgName << " : " << wbreader3D->GetErrMsg() << endl;
				exit(1);
			} 
		} else {
			if (wbreader2D->ReadRegion(min,max,buf) < 0) {
				cerr << ProgName << " : " << wbreader2D->GetErrMsg() << endl;
				exit(1);
			} 
		}

		TIMER_START(t0);

		if (fwrite(buf, sizeof(*buf), size, fp) != size) {
			cerr << ProgName << ": Could not write to output file : " 
				<< strerror(errno) << endl;
			exit(1);
		}

		TIMER_STOP(t0, *write_timer);

		if (! opt.quiet) {
			if(is3D) fprintf(stdout, "Wrote %dx%dx%d volume\n", dim[0],dim[1],dim[2]);
			else fprintf(stdout, "Wrote %dx%d plane\n", dim[0],dim[1]);
		}
	}
	else {
		size_t bmin[3];
		size_t bmax[3];
		const size_t *bs = metadata->GetBlockSize();

		if (is3D){
			wbreader3D->MapVoxToBlk(min,bmin);
		} else {
			wbreader2D->MapVoxToBlk(min,bmin);
		}

		size_t bdim[3] = {
			bmax[0]-bmin[0]+1,
			bmax[1]-bmin[1]+1,
			bmax[2]-bmin[2]+1
		};

		size = bdim[0]*bs[0] * bdim[1]*bs[1] * bdim[2]*bs[2];
		buf = new float[size];
		assert (buf != NULL);

		if (is3D) {
			if (wbreader3D->BlockReadRegion(bmin,bmax,buf,0) < 0) {
				cerr << ProgName << " : " << wbreader3D->GetErrMsg() << endl;
				exit(1);
			} 
		} else {
			if (wbreader2D->BlockReadRegion(bmin,bmax,buf,0) < 0) {
				cerr << ProgName << " : " << wbreader2D->GetErrMsg() << endl;
				exit(1);
			} 
		}

		TIMER_START(t0);

		if (fwrite(buf, sizeof(*buf), size, fp) != size) {
			cerr << ProgName << ": Could not write to output file : " 
				<< strerror(errno) << endl;
			exit(1);
		}

		TIMER_STOP(t0, *write_timer);

		if (! opt.quiet) {
			if (is3D) fprintf(
				stdout, "Wrote %dx%dx%d volume\n", 
				bdim[0]*bs[0],bdim[1]*bs[0],bdim[2]*bs[0]
			);
			else fprintf(
				stdout, "Wrote %dx%d plane\n", 
				bdim[0]*bs[0],bdim[1]*bs[0]
			);
		}
	}
	if (is3D) {
		*xform_timer = wbreader3D->GetXFormTimer();
		*read_timer = wbreader3D->GetReadTimer();
		wbreader3D->CloseVariable();
		delete wbreader3D;
	} else {
		*xform_timer = wbreader2D->GetXFormTimer();
		*read_timer = wbreader2D->GetReadTimer();
		wbreader2D->CloseVariable();
		delete wbreader2D;
	}
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
			datafile << "\" : " << strerror(errno) << endl;

		exit(1);
	}

	size_t min[3] = {opt.xregion.min, opt.yregion.min, opt.zregion.min};
	size_t max[3] = {opt.xregion.max, opt.yregion.max, opt.zregion.max};

	//Determine if variable is 2D or 3D:
	//Determine if variable is 3D, create a temporary metadata:
	Metadata mdTemp (metafile);
	const vector<string> vars3d = mdTemp.GetVariables3D();

	bool is3D = false;
	for (int i = 0; i<vars3d.size(); i++){
		if (vars3d[i] == opt.varname) {
			is3D = true;
			break;
		}
	}
	if (!is3D){
		//Make sure the orientation is horizontal:
		const vector<string> vars2d = mdTemp.GetVariables2DXY();
		bool isOK = false;
		for (int i = 0; i<vars2d.size(); i++){
			if (vars2d[i] == opt.varname) {
				isOK = true;
				break;
			}
		}
		if (!isOK){
			cerr << "Variable named " << opt.varname << " is neither 3D nor horizontal." << endl;
			cerr << "Conversion not supported." << endl;
			exit(1);
		}
	}

	TIMER_START(t0);

	if (min[0] == min[1] && min[1] == min[2] && min[2] == max[0] && 
		max[0] == max[1]  && max[1] == max[2] && max[2] == (size_t) -1) 
	{
		process_volume(metafile, fp, is3D, &read_timer, &xform_timer, &write_timer);
	}
	else {
		process_region(metafile, fp, is3D, &read_timer, &xform_timer, &write_timer);
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

