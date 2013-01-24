//
//      $Id$
//
//***********************************************************************
//                                                                      *
//                      Copyright (C)  2008	                        *
//          University Corporation for Atmospheric Research             *
//                      All Rights Reserved                             *
//                                                                      *
//***********************************************************************
//
//	File:		vdfedit
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Fri Aug 29 14:52:09 MDT 2008
//
//	Description:	
//
//
#include <iostream>
#include <cstring>
#include <cstdio>
#include <vector>
#include <sstream>
#include <algorithm>
#include <netcdf.h>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/MetadataSpherical.h>
#include <vapor/WRF.h>
#ifdef WIN32
#include "windows.h"
#endif

using namespace VetsUtil;
using namespace VAPoR;

int	cvtToExtents(const char *from, void *to);
int	cvtTo3DBool(const char *from, void *to);
int	cvtToOrder(const char *from, void *to);

struct opt_t {
	char *mapprojection;
	vector <string> addvars3d;
	vector <string> addvars2dxy;
	vector <string> addvars2dxz;
	vector <string> addvars2dyz;
	vector <string> delvars;
	int addts;
	float timeincr;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"mapprojection",1,	"",			"A whitespace delineated, quoted list "
        "of PROJ key/value pairs of the form '+paramname=paramvalue'.  vdfedit "
        "does not validate the string for correctness in any way"},
	{"addvars3d",1,	"",			"Colon delimited list of 3D variables to be\n\t\t\t\tadded to the VDF file"},
	{"addvars2dxy",1,	"",			"Colon delimited list of 2D XY-plane variables to be\n\t\t\t\tadded to the VDF file"},
	{"addvars2dxz",1,	"",			"Colon delimited list of 2D XZ-plane variables to be\n\t\t\t\tadded to the VDF file"},
	{"addvars2dyz",1,	"",			"Colon delimited list of 2D YZ-plane variables to be\n\t\t\t\tadded to the VDF file"},
	{"delvars",1,	"",			"Colon delimited list of (3D or 2D) variables to be\n\t\t\t\tdeleted from the VDF file"},
	{"addts",1,	"0",			"Number of time steps to be added (or subtracted if negative)"},
	{"timeincr",1,	"1.0",		"Increment used to compute time stamps \n\t\t\t\tfor new timesteps (see -addts)."},
	{"help",	0,	"",				"Print this message and exit"},
	{"quiet",	0,	"",				"Operate quietly"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"mapprojection", VetsUtil::CvtToString, &opt.mapprojection, sizeof(opt.mapprojection)},
	{"addvars3d", VetsUtil::CvtToStrVec, &opt.addvars3d, sizeof(opt.addvars3d)},
	{"addvars2dxy", VetsUtil::CvtToStrVec, &opt.addvars2dxy, sizeof(opt.addvars2dxy)},
	{"addvars2dxz", VetsUtil::CvtToStrVec, &opt.addvars2dxz, sizeof(opt.addvars2dxz)},
	{"addvars2dyz", VetsUtil::CvtToStrVec, &opt.addvars2dyz, sizeof(opt.addvars2dyz)},
	{"delvars", VetsUtil::CvtToStrVec, &opt.delvars, sizeof(opt.delvars)},
	{"addts", VetsUtil::CvtToInt, &opt.addts, sizeof(opt.addts)},
	{"timeincr", VetsUtil::CvtToFloat, &opt.timeincr, sizeof(opt.timeincr)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};


const char *ProgName;

void Usage(OptionParser &op, const char * msg) {

	if (msg) {
		cerr << ProgName << " : " << msg << endl;
	}
	cerr << "Usage: " << ProgName << " [options] vdf_file" << endl;
	op.PrintOptionHelp(stderr);

}

void ErrMsgCBHandler(const char *msg, int) {
	cerr << ProgName << " : " << msg << endl;
}

//
// Delete 'value' from 'vec' if 'value' is contained in 'vec'. Otherwise
// 'vec' is unchanged.
//
void vector_delete (vector <string> &vec, const string &value) {

	vector<string>::iterator itr;
	for (itr = vec.begin(); itr != vec.end(); ) {
		if (itr->compare(value) == 0) {
			vec.erase(itr);
			itr = vec.begin();
		}
		else {
			itr++;
		}
	}
}

int	main(int argc, char **argv) {

	OptionParser op;

	ProgName = Basename(argv[0]);


	MyBase::SetErrMsgCB(ErrMsgCBHandler);

	if (op.AppendOptions(set_opts) < 0) {
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		exit(1);
	}

	if (opt.help) {
		Usage(op, NULL);
		exit(0);
	}
	if (argc != 2) {
		Usage(op, "Wrong number of arguments");
		exit(0);
	}


	argv++;
	argc--;

	string metafile(*argv);

	MetadataVDC *metadata = new MetadataVDC(metafile);	
	if (MetadataVDC::GetErrCode()) {
		exit(1);
	}

	if (metadata->GetVDFVersion() < VDF_VERSION) {
		if (metadata->MakeCurrent() < 0) exit(1);
	}
		

	string bkupfile(metafile);
	bkupfile += ".bak";

	if (metadata->Write(bkupfile) < 0) {
		exit(1);
	}

	if (strlen(opt.mapprojection)) {
		if (metadata->SetMapProjection(opt.mapprojection) < 0) {
			cerr << MetadataVDC::GetErrMsg() << endl;
			exit(1);
		}
	}

	//
	// Variable additions and deletions
	//
	const vector <string> oldnames = metadata->GetVariableNames();
	const vector <string> oldnames3d = metadata->GetVariables3D();
	const vector <string> oldnames2dxy = metadata->GetVariables2DXY();
	const vector <string> oldnames2dxz = metadata->GetVariables2DXZ();
	const vector <string> oldnames2dyz = metadata->GetVariables2DYZ();

	vector <string> newnames = oldnames;
	vector <string> newnames3d = oldnames3d;
	vector <string> newnames2dxy = oldnames2dxy;
	vector <string> newnames2dxz = oldnames2dxz;
	vector <string> newnames2dyz = oldnames2dyz;

	for (int i=0; i<opt.addvars3d.size(); i++) {
		newnames.push_back(opt.addvars3d[i]);
		newnames3d.push_back(opt.addvars3d[i]);
	}
	for (int i=0; i<opt.addvars2dxy.size(); i++) {
		newnames.push_back(opt.addvars2dxy[i]);
		newnames2dxy.push_back(opt.addvars2dxy[i]);
	}
	for (int i=0; i<opt.addvars2dxz.size(); i++) {
		newnames.push_back(opt.addvars2dxz[i]);
		newnames2dxz.push_back(opt.addvars2dxz[i]);
	}
	for (int i=0; i<opt.addvars2dyz.size(); i++) {
		newnames.push_back(opt.addvars2dyz[i]);
		newnames2dyz.push_back(opt.addvars2dyz[i]);
	}

	for (int i=0; i<opt.delvars.size(); i++) {
		vector_delete(newnames, opt.delvars[i]);
		vector_delete(newnames3d, opt.delvars[i]);
		vector_delete(newnames2dxy, opt.delvars[i]);
		vector_delete(newnames2dxz, opt.delvars[i]);
		vector_delete(newnames2dyz, opt.delvars[i]);
	}

	if (metadata->SetVariableNames(newnames)<0) exit(1);
	if (metadata->SetVariables3D(newnames3d)<0) exit(1);
	if (metadata->SetVariables2DXY(newnames2dxy)<0) exit(1);
	if (metadata->SetVariables2DXZ(newnames2dxz)<0) exit(1);
	if (metadata->SetVariables2DYZ(newnames2dyz)<0) exit(1);
		

	//
	// Time step changes
	//
	if (opt.addts) {
		if (! opt.quiet) {
			cerr << ProgName << " : Warning: the \"-addts\" option may produce unexpected or even incorrect results if time-varying attributes are present\n";
		}
		long numts = metadata->GetNumTimeSteps();
		if (numts < 0) exit(1);

		vector <double> timestamps;
		vector <string> timestampstrings;
		const string &gridtype = metadata->GetGridType();

		if (gridtype.compare("layered") == 0) {

			for (size_t t = 0; t<numts; t++) {

				string tag("UserTimeStampString");
				string s = metadata->GetTSUserDataString(t, tag);
				StrRmWhiteSpace(s);
				
				TIME64_T t0;
				if (WRF::WRFTimeStrToEpoch(s, &t0) < 0) return(-1);
				timestamps.push_back((double) t0); 
				timestampstrings.push_back(s);
			}
		}
		else {
			for (size_t t = 0; t<numts; t++) {
				double v = metadata->GetTSUserTime(t);
				timestamps.push_back((TIME64_T) v); 
			}
		}

		if (opt.addts > 0) {
			double ts = timestamps.back();	// last element
			for (size_t t=0; t<opt.addts; t++) {
				ts += opt.timeincr;
				timestamps.push_back(ts);

				if (gridtype.compare("layered") == 0) {
					string s;
					WRF::EpochToWRFTimeStr((TIME64_T)ts, s);
					timestampstrings.push_back(s);
				}
			}
		}

		numts = numts + opt.addts;
		
		metadata->SetNumTimeSteps(numts);

		// 
		// Finally restore the user time stamps
		//
		for (size_t t=0; t<numts; t++)  {

			vector <double> tsNow(1, (double) timestamps[t]);
			metadata->SetTSUserTime(t, tsNow);

			if (gridtype.compare("layered") == 0) {
				string tag("UserTimeStampString");

				metadata->SetTSUserDataString(t, tag, timestampstrings[t]);
			}
		}
    }

	if (metadata->Write(metafile) < 0) {
		exit(1);
	}
}
