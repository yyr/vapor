//
//      $Id$
//
//***********************************************************************
//                                                                      *
//                      Copyright (C)  2006                             *
//          University Corporation for Atmospheric Research             *
//                      All Rights Reserved                             *
//                                                                      *
//***********************************************************************
//
//	File:		flashamr2vdf.cpp
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Thu Jan 5 12:10:07 MST 2006
//
//	Description:	Convert a single time step from a FLASH AMR data 
//					set to a VDC
//
//
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cassert>
#include <cstdio>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/AMRTree.h>
#include <vapor/AMRData.h>
#include <vapor/AMRIO.h>
#include <vapor/Metadata.h>

#include "flashhdf5.h"

using namespace VetsUtil;
using namespace VAPoR;

//
//	Command line argument stuff
//
struct {
	int	ts;
	vector <string> varnames;
	int level;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	tree;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"ts",		1, 	"0","Timestep of data file starting from 0"},
	{"varnames",1, 	"var1",	"Name of variable"},
	{"level",	1, 	"-1",	"Refinement levels saved. 0=>coarsest, 1=>next refinement, etc. -1=>finest"},
	{"help",	0,	"",	"Print this message and exit"},
	{"debug",	0,	"",	"Enable debugging"},
	{"quiet",	0,	"",	"Operate quietly"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"ts", VetsUtil::CvtToInt, &opt.ts, sizeof(opt.ts)},
	{"varnames", VetsUtil::CvtToStrVec, &opt.varnames, sizeof(opt.varnames)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};


const char	*ProgName;

main(int argc, char **argv) {

	OptionParser op;
    const char  *metafile;
    const char  *flashfile;
	double	timer = 0.0;
	double	read_timer = 0.0;


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
		cerr << "Usage : " << ProgName << " [options] metafile flashfile " << endl; 
		op.PrintOptionHelp(stderr);
		exit(0);
	}

	if (argc != 3) {
		cerr << "Usage : " << ProgName << " [options] metafile flashfile " << endl; 
		op.PrintOptionHelp(stderr);
		exit(1);
	}

	metafile = argv[1]; // Path to a vdf file
	flashfile = argv[2]; // Path to raw data file

	MyBase::SetErrMsgFilePtr(stderr);
	if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	TIMER_START(t0);

	// Create an AMRIO object to write the AMR grid to the VDC
	//
	AMRIO amrio(metafile, 0);
	if (amrio.GetErrCode() != 0) {
		cerr << ProgName << " : " << amrio.GetErrMsg() << endl;
		exit(1);
	}


	FlashHDFFile	hdffile((char *) flashfile);

	assert(hdffile.GetNumberOfDimensions() == 3);

	int total_blocks =  hdffile.GetNumberOfBlocks();


	int *gids = new int[total_blocks * 15];
	assert (gids != NULL);

	hdffile.GetGlobalIds(gids);

	float *bboxes = new float[total_blocks * 3 * 2];
	assert (bboxes != NULL);

	hdffile.GetBoundingBoxes(bboxes);

	int *refine_levels = new int[total_blocks];
	assert (refine_levels != NULL);

	hdffile.GetRefineLevels(refine_levels);
	
	
	if (! opt.quiet) {
		cout << "Reading tree\n";
	}
	TIMER_START(t1);

	AMRTree tree(
		(int (*)[15]) gids, (float (*)[3][2]) bboxes, 
		refine_levels, total_blocks
	);
	if (AMRData::GetErrCode()) {
		cerr << ProgName << " : " << AMRData::GetErrMsg() << endl; 
		exit(1);
	}

	TIMER_STOP(t1, read_timer);

    //
    // Open a tree for writing at the indicated time step
    //
	if (! opt.quiet) {
		cout << "Writing tree\n";
	}
    if (amrio.OpenTreeWrite(opt.ts) < 0) {
        cerr << ProgName << " : " << amrio.GetErrMsg() << endl;
        exit(1);
    }

    if (amrio.TreeWrite(&tree) < 0) {
        cerr << ProgName << " : " << amrio.GetErrMsg() << endl;
        exit(1);
    }

    if (amrio.CloseTree() < 0) {
        cerr << ProgName << " : " << amrio.GetErrMsg() << endl;
        exit(1);
    }

	int dim[3];
	hdffile.GetCellDimensions(dim);

	float *variable = new float[total_blocks * dim[0] * dim[1] * dim[2]];
	assert (variable != NULL);

	const size_t celldim[3] = {dim[0], dim[1], dim[2]};
	for (int i=0; i<opt.varnames.size(); i++) {

		TIMER_START(t1);

		if (! opt.quiet) {
			cout << "Reading variable " << opt.varnames[i] << endl;
		}
		hdffile.GetScalarVariable(
			(char *) opt.varnames[i].c_str(), 0, total_blocks, variable
		);

		TIMER_STOP(t1, read_timer);


		AMRData amrdata(
			&tree, celldim, (int (*)[15]) gids, variable, total_blocks,
			opt.level
		);

		if (! opt.quiet) {
			const float *range = amrio.GetDataRange();
			cout << "min and max values of data output: "<< range[0] << 
				" " << range[1] << endl;
		}

		if (AMRData::GetErrCode()) {
			cerr << ProgName << " : " << AMRData::GetErrMsg() << endl; 
			exit(1);
		}

		if (! opt.quiet) {
			cout << "Writing variable " << opt.varnames[i] << endl;
		}

		if (amrio.OpenVariableWrite(opt.ts, opt.varnames[i].c_str(), opt.level) < 0) {
			cerr << ProgName << " : " << amrio.GetErrMsg() << endl;
			exit(1);
		}

		if (amrio.VariableWrite(&amrdata) < 0) {
			cerr << ProgName << " : " << amrio.GetErrMsg() << endl;
			exit(1);
		}

		if (amrio.CloseVariable() < 0) {
			cerr << ProgName << " : " << amrio.GetErrMsg() << endl;
			exit(1);
		}
	}
	TIMER_STOP(t0, timer);

	if (! opt.quiet) {
		float   write_timer = amrio.GetWriteTimer();

		fprintf(stdout, "read time : %f\n", read_timer);
		fprintf(stdout, "write time : %f\n", write_timer);
		fprintf(stdout, "total transform time : %f\n", timer);
	}


	exit (0);
		
}

