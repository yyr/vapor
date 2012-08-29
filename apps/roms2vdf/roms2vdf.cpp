//
// $Id$
//
//
//***********************************************************************
//                                                                       *
//                            Copyright (C)  2005                        *
//            University Corporation for Atmospheric Research            *
//                            All Rights Reserved                        *
//                                                                       *
//***********************************************************************/
//
//      File:		roms2vdf.cpp
//
//      Author:         John Clyne 
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:          	August 2012 
//
//      Description:	Read ROMS files containing a 3D array of floats or doubles
//			and insert the volume into an existing
//			Vapor Data Collection
//


#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cmath>
#include <ctime>
#include <netcdf.h>
#include <algorithm>
#include <climits>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/WaveletBlock3DBufWriter.h>
#include <vapor/WaveletBlock3DRegionWriter.h>
#include <vapor/WaveCodecIO.h>
#include <vapor/NetCDFCollection.h>
#ifdef WIN32
#include "windows.h"
#endif

using namespace VetsUtil;
using namespace VAPoR;

string StaggeredDimsTag = "ROMS_StaggeredDims";
string TimeDimsTag = "ROMS_TimeDimNames";
string TimeCoordTag = "ROMS_TimeCoordVariable";
string MissingValTag = "ROMS_MissingValTag";



//
//	Command line argument stuff
//
struct opt_t {
	vector<string> varnames;
	int numts;
	int level;
	int lod;
	int nthreads;
	float tolerance;
	char *grid;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"varnames",1, 	"",	"Colon delimited list of variable names in ROMS file to convert. The default is to convert variables in the set intersection of those variables found in the VDF file and those in the ROMS file."},
	{"numts",	1,	"-1","Maximum number of time steps that may be converted. A -1 implies the conversion of all time steps found"},
	{"level",	1, 	"-1","Refinement levels saved. 0=>coarsest, 1=>next refinement, etc. -1=>finest"},
	{"lod",	1, 	"-1",	"Compression levels saved. 0 => coarsest, 1 => "
		"next refinement, etc. -1 => all levels defined by the .vdf file"},
	{"nthreads",1, 	"0",	"Number of execution threads (0 => # processors)"},
	{"tolerance",	1, 	"0.0000001","Tolerance for comparing relative user times"},
	{"grid",	1, 	"","Path to grid file"},
	{"help",	0,	"",	"Print this message and exit."},
	{"debug",	0,	"",	"Enable debugging."},
	{"quiet",	0,	"",	"Operate quietly (outputs only vertical extents that are lower than those in the VDF)."},
	{NULL}
};

OptionParser::Option_T	get_options[] = {
	{"varnames", VetsUtil::CvtToStrVec, &opt.varnames, sizeof(opt.varnames)},
	{"numts", VetsUtil::CvtToInt, &opt.numts, sizeof(opt.numts)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"lod", VetsUtil::CvtToInt, &opt.lod, sizeof(opt.lod)},
	{"nthreads", VetsUtil::CvtToInt, &opt.nthreads, sizeof(opt.nthreads)},
	{"tolerance", VetsUtil::CvtToFloat, &opt.tolerance, sizeof(opt.tolerance)},
	{"grid", VetsUtil::CvtToString, &opt.grid, sizeof(opt.grid)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};

const char	*ProgName;

size_t sliceBufferSize = 0;
float *sliceBuffer = NULL;
int CopyVariable(
	NetCDFCollection &nc,
	VDFIOBase *vdfio,
	int level,
	int lod, 
	string varname,
	const size_t dim[3],
	size_t tsROMS,
	size_t tsVDC,
	float roms_mv,
	float vdc_mv,
	bool has_missing
) {


	int rc;
	
	rc = nc.OpenRead(tsROMS, varname);
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to read ROMS variable %s at ROMS time step %d",
			varname.c_str(), tsROMS
		);
		return (-1);
	}

	rc = vdfio->OpenVariableWrite(tsVDC, varname.c_str(), level, lod);

	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to open for write ROMS variable %s at ROMS time step %d",
			varname.c_str(), tsROMS
		);
		return (-1);
	}

	size_t slice_sz = dim[0] * dim[1];

	if (sliceBufferSize < slice_sz) {
		if (sliceBuffer ) delete [] sliceBuffer;
		sliceBuffer = new float[slice_sz];
		sliceBufferSize = slice_sz;
	}

	for (size_t z = 0; z<dim[2]; z++) {
		if (nc.ReadSlice(sliceBuffer) < 0 ) {
			MyBase::SetErrMsg(
				"Failed to read ROMS variable %s slice at ROMS time step %d",
				varname.c_str(), tsROMS
			);
			return (-1);
		}

		if (has_missing) {
			for (size_t i=0; i<dim[0]*dim[1]; i++) {
				if (sliceBuffer[i] == roms_mv) sliceBuffer[i] = vdc_mv;
			}
		}
		
		if (vdfio->WriteSlice(sliceBuffer) < 0) {
			MyBase::SetErrMsg(
				"Failed to write ROMS variable %s slice at ROMS time step %d (vdc2)",
				varname.c_str(), tsROMS
			);
			return (-1);
		}
	} // End of for z.

	nc.Close();
	vdfio->CloseVariable();

	return(0);
}  // End of CopyVariable.

int CopyVariable2D(
	NetCDFCollection &nc,
	VDFIOBase *vdfio,
	int level,
	int lod, 
	string varname,
	const size_t dim[3],
	size_t tsROMS,
	size_t tsVDC,
	float roms_mv,
	float vdc_mv,
	bool has_missing
) {

	int rc;

	rc = nc.OpenRead(tsROMS, varname);
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to copy ROMS variable %s at ROMS time step %d",
			varname.c_str(), tsROMS
		);
		return (-1);
	}

	rc = vdfio->OpenVariableWrite(tsVDC, varname.c_str(), level, lod);
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to copy ROMS variable %s at ROMS time step %d",
			varname.c_str(), tsROMS
		);
		return (-1);
	}
 
 	size_t slice_sz = dim[0] * dim[1];

	if (sliceBufferSize < slice_sz) {
		if (sliceBuffer ) delete [] sliceBuffer;
		sliceBuffer = new float[slice_sz];
		sliceBufferSize = slice_sz;
	}

	if (nc.ReadSlice(sliceBuffer) < 0 ) {
		MyBase::SetErrMsg(
			"Failed to read ROMS variable %s at ROMS time step %d",
			varname.c_str(), tsROMS
		);
		return (-1);
	}

	if (has_missing) {
		for (size_t i=0; i<dim[0]*dim[1]; i++) {
			if (sliceBuffer[i] == roms_mv) sliceBuffer[i] = vdc_mv;
		}
	}

	if (vdfio->WriteRegion(sliceBuffer) < 0) {
		MyBase::SetErrMsg(
			"Failed to write ROMS variable %s at ROMS time step %d",
			varname.c_str(), tsROMS
		);
		return (-1);
	}

	nc.Close();
	vdfio->CloseVariable();

	return(0);
} // End of CopyVariable2D.


// Create a list of variables to be translated based on what the user
// wants (via command line option, 'opt.varnames', if not empty, else
// via contents of 'vdf_vars'), and what is available in the 
// ROMS file (roms_vars). The list of variables to translate is
// returned in 'copy_vars'

void SelectVariables(
	const vector <string> &vdf_vars, 
	const vector <string> &roms_vars, 
	const vector <string> &opt_varnames, 
	vector <string> &copy_vars
) {
	copy_vars.clear();

	vector <string> candidates; // Candidate variables for selection

	if (opt_varnames.size()) candidates = opt_varnames;
	else candidates = vdf_vars;

	for (int i=0; i<candidates.size(); i++) {
		string v = candidates[i];

		// Variable is not a derived quantity
		//
		if ((find(roms_vars.begin(),roms_vars.end(),v)!=roms_vars.end()) &&
			(find(copy_vars.begin(),copy_vars.end(),v)==copy_vars.end())
		) {
			copy_vars.push_back(v);
		}
	}  // End of for

} // End of SelectVariables	

bool dims_match(
	const size_t dims1[],
	const size_t dims2[],
	int n
) {
	for (int i=0; i<n; i++) {
		if (dims1[i] != dims2[i]) return(false);
	}
	return(true);
}

bool map_VDF2ROMS_time(
	double romstime,
    MetadataVDC *metadataVDC,
    size_t *tsVDC,
    double tolerance
) {

    for (size_t ts = 0; ts<metadataVDC->GetNumTimeSteps(); ts++) {
        double userTimeVDC = metadataVDC->GetTSUserTime(ts);
        if (abs((romstime - userTimeVDC) ) < tolerance) {
            *tsVDC = ts;
            return(true);
        }
    }
    return(false);
}


void Usage(OptionParser &op, const char * msg) {

    if (msg) {
        cerr << ProgName << " : " << msg << endl;
    }
    cerr << "Usage: " << ProgName << " [options] vdffile [romsfiles...]" << endl;
    op.PrintOptionHelp(stderr);
}


int	main(int argc, char **argv) {
	OptionParser op;

	int estatus = 0;
	
	// Parse command line arguments and check for errors
	ProgName = Basename(argv[0]);
	if (op.AppendOptions(set_opts) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		cerr << ProgName << " : " << op.GetErrMsg();
		exit(1);
	}

	MyBase::SetErrMsgFilePtr(stderr);

	if (opt.help) 
	{
		Usage(op, "");
		exit(0);
	}

	if (argc < 3) {
		Usage(op, "Insufficient arguments");
		exit(1);
	}

	if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	argv++;
	argc--;

	string metafile = *argv;

	WaveletBlock3DBufWriter *wbwriter3d = NULL;
	WaveCodecIO	*wcwriter3d = NULL;
	WaveletBlock3DRegionWriter *wbwriter2d = NULL;
	VDFIOBase *vdfio3d = NULL;
	VDFIOBase *vdfio2d = NULL;
	
	MetadataVDC	*metadataVDC = new MetadataVDC(metafile);
	if (MetadataVDC::GetErrCode() != 0) { 
		MyBase::SetErrMsg("Error processing VDC metafile : %s",metafile.c_str());
		exit(1);
	}

	bool vdc1 = (metadataVDC->GetVDCType() == 1);

	if(vdc1) {
		wbwriter3d = new WaveletBlock3DBufWriter(*metadataVDC);
		vdfio3d = wbwriter3d;

		//
		// Ugh! WaveletBlock3DBufWriter class does not handle 2D data!!!  
		// Also does want to work form VDFIOBase.
		//
		wbwriter2d = new WaveletBlock3DRegionWriter(*metadataVDC);
		vdfio2d = wbwriter2d;
	}
	else {
		wcwriter3d = new WaveCodecIO(*metadataVDC, opt.nthreads);
		vdfio3d = wcwriter3d;
		vdfio2d = vdfio3d;
	}
	
	if (! vdfio3d || ! vdfio2d) {
		MyBase::SetErrMsg("Error processing VDC metafile : %s",metafile.c_str());
		exit(1);
	}


	vector <string> varsVDC = metadataVDC->GetVariableNames();
	const size_t *dimsVDC = metadataVDC->GetDimension();

	for (vector<string>::iterator itr = opt.varnames.begin(); itr!=opt.varnames.end(); itr++) {
		if (find(varsVDC.begin(),varsVDC.end(),*itr)==varsVDC.end()) {
			MyBase::SetErrMsg(
				"Requested variable \"%s\" not found in VDF file %s",
				itr->c_str(), metafile.c_str());
			exit(1);
		}
	}
		
	argv++;
	argc--;

	//
	// The grid file contains info about the grid (e.g. lat and long 
	// coorindate variables, etc.
	//
	NetCDFSimple gridfile;
	if (strlen(opt.grid)) gridfile.Initialize(opt.grid);



	//
	// Extract various bits of info that we need from the .vdf file
	// Should probably provide command line options to overide what
	// is specified in the .vdf
	//
	vector <string> stagdims;
	stagdims = metadataVDC->GetUserDataStringVec(StaggeredDimsTag);

	vector <string> timedims;
	timedims = metadataVDC->GetUserDataStringVec(TimeDimsTag);

	vector <string> timecoordvec;
	timecoordvec = metadataVDC->GetUserDataStringVec(TimeCoordTag);
	string timecoord = timecoordvec.size() ? timecoordvec[0] : "";

	vector <string> missingvec;
	missingvec = metadataVDC->GetUserDataStringVec(MissingValTag);
	string missing = missingvec.size() ? missingvec[0] : "";

	vector <string> romsfiles;
	for (int i=0; i<argc; i++) romsfiles.push_back(argv[i]);

	NetCDFCollection nc;
	int rc = nc.Initialize(romsfiles, timedims, timecoord);
	if (rc<0) {
		MyBase::SetErrMsg("Error processing ROMS files");
		exit(1);
	}

	//
	// Identify the staggered dimensions
	//
	nc.SetStaggeredDims(stagdims);

	//
	// Identify the name of the netCDF variable attribute the specifies 
	// missing value.
	//
	nc.SetMissingValueAttName(missing);

	vector <string> varsROMS = nc.GetVariableNames(3);
	vector <string> vec = nc.GetVariableNames(2);
	for (int i=0; i<vec.size(); i++) varsROMS.push_back(vec[i]);

	//
	// Figure out which ROMS variables we will process
	//
	vector <string> copy_vars;
	SelectVariables(
		varsVDC, varsROMS, opt.varnames, copy_vars
	);

	//
	// Number of time steps  
	//
	vector <double> romstimes = nc.GetTimes();

	//
	// Loop over each of the ROMS variables and process them
	//
	for (int i=0; i<copy_vars.size(); i++) {
		if (! opt.quiet) {
			cout << "Processing variable : " << copy_vars[i] << endl;
		}



		vector <size_t> dimsvec = nc.GetDims(copy_vars[i]);
		size_t dims[3];
		bool var3d = dimsvec.size() == 3;

		//
		// Need to swap the dimension order!!!
		//
		if (var3d) {
			dims[2] = dimsvec[0];
			dims[1] = dimsvec[1];
			dims[0] = dimsvec[2];
		}
		else {
			dims[1] = dimsvec[0];
			dims[0] = dimsvec[1];
		}

		if (! dims_match(dims, dimsVDC, dimsvec.size())) {
			MyBase::SetErrMsg(
				"Error processing ROMS variable %s, : dimension mismatch, skipping",
				copy_vars[i].c_str()
			);
			MyBase::SetErrCode(0);
			estatus++;
			continue;
		} 


		for (size_t ts=0; ts<romstimes.size(); ts++) {
			
			if (! nc.VariableExists(ts, copy_vars[i])) continue; 

			if (! opt.quiet) {
				cout << "Processing time step : " << ts << endl;
			}

			//
			// See if variable has missing data
			//
			NetCDFSimple::Variable varinfo;
			nc.GetVariableInfo(ts, copy_vars[i], varinfo);
			vector <double> roms_mvvec;
			varinfo.GetAtt(missing, roms_mvvec);
			bool has_missing = false;
			float roms_mv, vdc_mv; 
			if (roms_mvvec.size()) {
				has_missing = true;
				vector <double> vdc_mvvec = metadataVDC->GetMissingValue();
				if (! vdc_mvvec.size()) {
					MyBase::SetErrMsg(".vdf file missing missing value");
					exit(1);
				}
				roms_mv = roms_mvvec[0];
				vdc_mv = vdc_mvvec[0];
			}
				 

			double romstime = romstimes[ts];
			size_t tsVDC;
            if (!map_VDF2ROMS_time(romstime, metadataVDC, &tsVDC, opt.tolerance)) {
                MyBase::SetErrMsg(
                    "Error processing ROMS variable %s, at time step %d : no matching VDC user time, skipping",
                    copy_vars[i].c_str(), ts
                );
                MyBase::SetErrCode(0);
                estatus++;
                continue;
            }

		
			if (var3d) {
				CopyVariable(
					nc, vdfio3d, opt.level, opt.lod, copy_vars[i], 
					dims, ts, tsVDC, roms_mv, vdc_mv, has_missing
				);
			}
			else {
				CopyVariable2D(
					nc, vdfio2d, opt.level, opt.lod, copy_vars[i], 
					dims, ts, tsVDC, roms_mv, vdc_mv, has_missing
				);
			}
		}
	}

	exit (estatus);

}
