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
//	Description:	Convert a sequence of FLASH AMR data files
//					to a VDC. The AMR files are assumed to represent
//					a single simulation.
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
struct opt_t {
	vector <string> varnames;
	int level;
	int ts;
	OptionParser::Boolean_T	rmap;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"varnames",1, 	"",	"Colon delimited list of variable names in Flash file to convert. The default is to convert variables in the set intersection of those variables found in the VDF file and those in the Flash file."},
	{"level",	1, 	"-1",	"Refinement levels saved. 0=>coarsest, 1=>next refinement, etc. -1=>finest"},
	{"ts",	1, 	"-1",	"Time step of first file. If this option is present the UserTime .vdf metadata element is ignored."},
	{"rmap",	0,	"",	"Include the derived \"refine_level\" variable"},
	{"help",	0,	"",	"Print this message and exit"},
	{"debug",	0,	"",	"Enable debugging"},
	{"quiet",	0,	"",	"Operate quietly"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"varnames", VetsUtil::CvtToStrVec, &opt.varnames, sizeof(opt.varnames)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"ts", VetsUtil::CvtToInt, &opt.ts, sizeof(opt.ts)},
	{"rmap", VetsUtil::CvtToBoolean, &opt.rmap, sizeof(opt.rmap)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};


const char	*ProgName;

int main(int argc, char **argv) {

	OptionParser op;
    string metafile;
    string flashfile;
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

	if (argc < 3) {
		cerr << "Usage : " << ProgName << " [options] metafile flashfiles... " << endl; 
		op.PrintOptionHelp(stderr);
		exit(1);
	}
	argc--;
	argv++;

	metafile = argv[0]; // Path to a vdf file
	argc--;
	argv++;


	MyBase::SetErrMsgFilePtr(stderr);
	if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);


	// Create an AMRIO object to write the AMR grid to the VDC
	//
	AMRIO amrio(metafile.c_str(), 0);
	if (amrio.GetErrCode() != 0) {
		cerr << ProgName << " : " << amrio.GetErrMsg() << endl;
		exit(1);
	}
	const Metadata *metadata = amrio.GetMetadata();


	// Get user times from VDF file. Convert to 32bit for comparison
	// with 32bit Flash data. Ugh!
	//
	vector <float> vdf_usertimes;	
	for (size_t t=0; t<metadata->GetNumTimeSteps(); t++) {
		const vector <double> &usertime = metadata->GetTSUserTime(t); 
		assert(usertime.size() == 1);	// sanity check
		vdf_usertimes.push_back(usertime[0]);
	}

	vector <string> vdf_vars3d = metadata->GetVariables3D();
		
	int mem_size = 0;
	int *gids = NULL;
	float *bboxes = NULL;
	int *refine_levels = NULL;
	float *variable = NULL;
	for (int arg = 0; arg<argc; arg++) {
		int rc;
		size_t ts;

		TIMER_START(t0);

		flashfile = argv[arg]; // Path to raw data file

		if (! opt.quiet) {
			cout << "Processing file " << flashfile << endl;
		}

		FlashHDFFile	hdffile;
		rc = hdffile.Open(flashfile.c_str());
		if (rc<0) exit(1);

		float ut = hdffile.GetUserTime();
		vector <float>::iterator itr;

		// 
		// Map Flash user time to user time found in the .vdf file. The
		// user time will determine the time step
		//
	
		if (opt.ts < 0 ) {
			itr = find(vdf_usertimes.begin(), vdf_usertimes.end(), ut);
			if (itr == vdf_usertimes.end()) {
				cerr << ProgName << " : no matching user time found for file "<< endl;
				cerr << "skipping file..." << endl; 
				hdffile.Close();
				continue;
			}
			ts = itr - vdf_usertimes.begin();
		}
		else {
			ts = arg + opt.ts;
		}

		if (hdffile.GetNumberOfDimensions() != 3) {
			cerr << ProgName << " : wrong number of dimensions" << endl;
			hdffile.Close();
			continue;
		}

		int dim[3];
		hdffile.GetCellDimensions(dim);

		int total_blocks =  hdffile.GetNumberOfBlocks();

		if (total_blocks > mem_size) {
			mem_size = total_blocks;

			if (gids) delete [] gids;
			gids = new int[total_blocks * 15];
			assert (gids != NULL);

			if (bboxes) delete [] bboxes;
			bboxes = new float[total_blocks * 3 * 2];
			assert (bboxes != NULL);

			if (refine_levels) delete [] refine_levels;
			refine_levels = new int[total_blocks];
			assert (refine_levels != NULL);

			if (variable) delete [] variable;
			variable = new float[total_blocks * dim[0] * dim[1] * dim[2]];
			assert (variable != NULL);
		}

		hdffile.GetGlobalIds(gids, total_blocks);

		hdffile.GetBoundingBoxes(bboxes, total_blocks);

		hdffile.GetRefineLevels(refine_levels, total_blocks);

		int bdim[3];
		hdffile.GetBaseDimensions(bdim);
		const size_t bdim_sz[3] = {bdim[0], bdim[1], bdim[2]};
		
		
		if (! opt.quiet) {
			cout << "	Processing tree (" << total_blocks << " total blocks)\n";
		}
		TIMER_START(t1);

		AMRTree tree(
			bdim_sz, (int (*)[15]) gids, (const float (*)[3][2]) bboxes, 
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
		if (amrio.OpenTreeWrite(ts) < 0) {
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


		const size_t celldim[3] = {dim[0], dim[1], dim[2]};

		// 
		// If a list of variable names was provided on the command line
		// us them. Otherwise generate a variable list from the 
		// intersection of the variables found in the .vdf and flash files
		//
		vector <string> varnames;
		if (opt.varnames.size() && (opt.varnames[0].compare("") != 0)) {
			varnames = opt.varnames;
		}
		else {
			vector <string>::iterator itr; 
			vector <string> flash_varnames;
			hdffile.GetVariableNames(flash_varnames);
			for (int i=0; i<vdf_vars3d.size(); i++) {
				itr = find(
					flash_varnames.begin(), flash_varnames.end(), 
					vdf_vars3d[i]
				);
				if (itr != flash_varnames.end()) varnames.push_back(*itr); 
			}
		}

		if (opt.rmap) {
			vector <string>::iterator itr; 
			itr = find(vdf_vars3d.begin(), vdf_vars3d.end(), "refine_level");
			if (itr != vdf_vars3d.end()) varnames.push_back(*itr); 
		}

		for (int i=0; i<varnames.size(); i++) {

			TIMER_START(t1);

			if (! opt.quiet) {
				cout << "	Processing variable " << varnames[i] << endl;
			}

			//
			// If the current variable is "refine_level" we need 
			// to derive it. Otherwise, read the variable direcly
			// from the hdf file
			// 
			if (varnames[i].compare("refine_level") == 0) {
				AMRTree::cid_t cid;
				int sz = celldim[0]*celldim[1]*celldim[2];
				int index;
				
				for (int idx=0; idx<total_blocks; idx++) {

					const float *b = &bboxes[idx*6];
					double ucoord[3];

					float deltax = (b[1]-b[0]) / (float) celldim[0];
					float deltay = (b[3]-b[2]) / (float) celldim[1];
					float deltaz = (b[5]-b[4]) / (float) celldim[2];

					for (int kk=0; kk<celldim[2]; kk++) {
					for (int jj=0; jj<celldim[1]; jj++) {
					for (int ii=0; ii<celldim[0]; ii++) {
						ucoord[0] = b[0] + (0.5*deltax) + (ii*deltax);
						ucoord[1] = b[2] + (0.5*deltay) + (ii*deltay);
						ucoord[2] = b[4] + (0.5*deltaz) + (ii*deltaz);
						cid = tree.FindCell(ucoord, opt.level);
						index = idx*sz + kk*celldim[0]*celldim[1] + jj*celldim[0] + ii;

						//variable[idx*sz+k] = refine_levels[idx];
						variable[index] = tree.GetCellLevel(cid) + 1;
					}
					}
					}
				}
			}
			else {
				hdffile.GetScalarVariable(
					(char *) varnames[i].c_str(), 0, total_blocks, variable
				);
			}

			TIMER_STOP(t1, read_timer);


			//
			// Create an AMR data structure with the current variable
			//
			AMRData amrdata(
				&tree, celldim, (int (*)[15]) gids, (float (*)[3][2]) bboxes,
				variable, total_blocks, opt.level
			);

			if (! opt.quiet) {
				const float *range = amrdata.GetDataRange();
				cout << "	min and max values of data output: "<< range[0] << 
					" " << range[1] << endl;
			}

			if (AMRData::GetErrCode()) {
				cerr << ProgName << " : " << AMRData::GetErrMsg() << endl; 
				exit(1);
			}

			if (amrio.OpenVariableWrite(ts, varnames[i].c_str(), opt.level) < 0) {
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



		hdffile.Close();
			
	}

	if (! opt.quiet) {
		float   write_timer = amrio.GetWriteTimer();

		fprintf(stdout, "read time : %f\n", read_timer);
		fprintf(stdout, "write time : %f\n", write_timer);
		fprintf(stdout, "total transform time : %f\n", timer);
	}

	exit (0);
}
