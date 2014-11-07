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
//	Description:	Create a .vdf file from one or more Flash AMR files
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
#include <vapor/MetadataVDC.h>

#include "flashhdf5.h"

using namespace VetsUtil;
using namespace VAPoR;


using namespace VetsUtil;
using namespace VAPoR;

struct opt_t {
	int	numts;
	float deltat;
	int	level;
	char *comment;
	vector <string> vars3d;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"numts",	1, 	"0",		"Total number of time steps. Default is to determine number by the number of flash files on the command line"},
	{"deltat",	1,	"1.0",		"Time sampling used when numts option exceeds the number of flash files on the command line"},
	{"vars3d",1,	"",			"Colon delimited list of 3D variables to be extracted from Flash data"},
	{"level",	1, 	"-1",		"Maximum refinement level. -1 => use Flash lrefine_max paramater"},
	{"comment",	1,	"",			"Top-level comment (optional)"},
	{"help",	0,	"",			"Print this message and exit"},
	{"quiet",	0,	"",			"Operate quietly"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"numts", VetsUtil::CvtToInt, &opt.numts, sizeof(opt.numts)},
	{"deltat", VetsUtil::CvtToInt, &opt.deltat, sizeof(opt.deltat)},
	{"vars3d", VetsUtil::CvtToStrVec, &opt.vars3d, sizeof(opt.vars3d)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"comment", VetsUtil::CvtToString, &opt.comment, sizeof(opt.comment)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};

int	GetFlashMetadata(
	const char **files,
	int nfiles,
	vector <double> &exts,	// user extents
	size_t base_dims[3],	// dimension of base (coarsest grid) in blocks
	size_t cell_dims[3],	// dimension of a single block (cell)
	vector <string> &varnames3d,	// names of variables
	vector <double> &usertimes,		// user times associated with each file
	int *max_reflevel	// maximum refinement level
) {
	
	vector<string> flashVars3d, _flashVars3d;
	double extents[6], _extents[6];
	int _cell_dimensions[3], cell_dimensions[3];
	int _base_dimensions[3], base_dimensions[3];
	int _max_ref = 0, max_ref;
	float usertime;
	
	bool first = true;
	bool success = false;

	FlashHDFFile *hdffile = new FlashHDFFile();

	for (int fileidx=0; fileidx<nfiles; fileidx++) {
		int rc;

		flashVars3d.clear();

		rc = hdffile->Open(files[fileidx]);
		if (rc < 0) {
			cerr << "Error processing file " <<files[fileidx] << ", skipping" << endl;
			continue;
		}

		hdffile->GetVariableNames(flashVars3d);

		hdffile->GetBaseDimensions(base_dimensions);
		hdffile->GetCellDimensions(cell_dimensions);
		hdffile->GetExtents(extents);
		usertime = hdffile->GetUserTime();
		max_ref = hdffile->GetMaxRefineParam();

		if (first) {
			_flashVars3d = flashVars3d;
			for (int i=0; i<6; i++) _extents[i] = extents[i];
			for (int i=0; i<3; i++) _base_dimensions[i] = base_dimensions[i];
			for (int i=0; i<3; i++) _cell_dimensions[i] = cell_dimensions[i];
			_max_ref = max_ref;

		}
		else {
			bool mismatch = false;

			for(int i=0; i<6; i++) {
				if (_extents[i] != extents[i]) mismatch = true;
			}

			for(int i=0; i<3; i++) {
				if (_base_dimensions[i] != base_dimensions[i]) mismatch = true;
			}

			for(int i=0; i<3; i++) {
				if (_cell_dimensions[i] != cell_dimensions[i]) mismatch = true;
			}

			if (max_ref != _max_ref) mismatch = true;

			if (_flashVars3d.size() != flashVars3d.size()) {
				mismatch = true;
			}
			else {

				for (int j=0; j<flashVars3d.size(); j++) {
					if (flashVars3d[j].compare(_flashVars3d[j]) != 0) {
						mismatch = true;
					}
				}
			}

			if (mismatch) {
				cerr << "File mismatch, skipping " << files[fileidx] <<  endl;
				continue;
			}
		}
		first = false;

		usertimes.push_back(usertime);

		success = true;
	}

	exts.clear();
	for (int i=0; i<6; i++) exts.push_back(_extents[i]);
	for (int i=0; i<3; i++) cell_dims[i] = _cell_dimensions[i];
	for (int i=0; i<3; i++) base_dims[i] = _base_dimensions[i];
	varnames3d = _flashVars3d;
	*max_reflevel = _max_ref - 1;

	if (success) return(0);
	else return(-1);
}
	

const char *ProgName;

void Usage(OptionParser &op, const char * msg) {

	if (msg) {
		cerr << ProgName << " : " << msg << endl;
	}
	cerr << "Usage: " << ProgName << " [options] flash_hdf_file... vdf_file" << endl;
	op.PrintOptionHelp(stderr);

}

void ErrMsgCBHandler(const char *msg, int) {
	cerr << ProgName << " : " << msg << endl;
}

int	main(int argc, char **argv) {

	OptionParser op;

	string	s;
	MetadataVDC *metadata;

	ProgName = Basename(argv[0]);

	if (op.AppendOptions(set_opts) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		cerr << ProgName << " : " << OptionParser::GetErrMsg();
		exit(1);
	}

	MyBase::SetErrMsgCB(ErrMsgCBHandler);


	if (opt.help) {
		Usage(op, NULL);
		return(0);
	}

	argv++;
	argc--;
	vector <double> extents;
	vector <double> usertimes;
	vector <string> flashVarNames3d;
	vector <string> vdfVarNames3d;
	size_t base_dims[3];
	size_t cell_dims[3];
	int max_reflevel;

	// 
	// Get metadata from all flash files
	//
	int rc = GetFlashMetadata(
		(const char **)argv, argc-1, extents, 
		base_dims, cell_dims, flashVarNames3d, usertimes, &max_reflevel
	);

	if (rc<0) exit(1);


	// Check and see if the variable names specified on the command
	// line exist in the flash files. Issue a warning if they don't, 
	// but we still
	// include them.
	//
	if (opt.vars3d.size()) {
		vdfVarNames3d = opt.vars3d;

		for ( int i = 0 ; i < opt.vars3d.size() ; i++ ) {

			bool foundVar = false;
			for ( int j = 0 ; j < flashVarNames3d.size() ; j++ )
				if ( flashVarNames3d[j] == opt.vars3d[i] ) {
					foundVar = true;
					break;
				}

			if ( !foundVar ) {
				cerr << ProgName << ": Warning: desired Flash 3D variable " << 
					opt.vars3d[i] << " does not appear in sample Flash file" << endl;
			}
		}
	}
	else {
		vdfVarNames3d = flashVarNames3d;
		vdfVarNames3d.push_back("refine_level");
	}

	// If more time steps were requested than Flash data files 
	// were supplied via the command line add additional time steps 
	// with a user time step determined by the deltat option. 
	//
	if (opt.numts && (opt.numts > usertimes.size())) {
		double t0 = 0;
		int n = opt.numts;
		if (usertimes.size()) {
			t0 = usertimes[usertimes.size()-1] + opt.deltat;
			n -= usertimes.size();
		}
		else {
			t0 = 0.0;
		}
		for (int i=0; i<n; i++) {
			usertimes.push_back(t0 + (i * opt.deltat));
		}
	}

	//
	// Compute the dimensions of the AMR grid if fully refined at the
	// maximum refinement level
	//
	size_t dim[3];
	for (int i=0; i<3; i++) {
		dim[i] = base_dims[i] * cell_dims[i];
	}
	for (int i=0; i<max_reflevel; i++) {
		dim[0] *= 2;
		dim[1] *= 2;
		dim[2] *= 2;
	}
		
	metadata = new MetadataVDC(dim,max_reflevel,cell_dims);

	if (MetadataVDC::GetErrCode()) {
		exit(1);
	}

	if (metadata->SetNumTimeSteps(usertimes.size()) < 0) {
		exit(1);
	}

	s.assign(opt.comment);
	if (metadata->SetComment(s) < 0) {
		exit(1);
	}

	s.assign("block_amr");
	if (metadata->SetGridType(s) < 0) {
		exit(1);
	}

	if (metadata->SetVariableNames(vdfVarNames3d) < 0) {
		exit(1);
	}

	
	if (metadata->SetExtents(extents) < 0) {
		exit(1);
	}
	
    // Set the user time for each time step
    for (size_t t = 0 ; t < usertimes.size() ; t++ )
    {
		vector<double> tsNow(1, (double) usertimes[t]);
        if ( metadata->SetTSUserTime( t, tsNow ) < 0) {
            exit( 1 );
        }
    }

	if (metadata->Write(argv[argc-1]) < 0) {
		exit(1);
	}

	if (! opt.quiet) {
		cout << "Created VDF file:" << endl;
		cout << "\tNum time steps : " << usertimes.size() << endl;
		cout << "\t3D Variable names : ";
		for (int i=0; i<metadata->GetVariables3D().size(); i++) {
			cout << metadata->GetVariables3D()[i] << " ";
		}
		cout << endl;

		cout << "\tExtents : ";
		const vector <double> extptr = metadata->GetExtents();
		for(int i=0; i<6; i++) {
			cout << extptr[i] << " ";
		}
		cout << endl;
	}

	return(0);
}
