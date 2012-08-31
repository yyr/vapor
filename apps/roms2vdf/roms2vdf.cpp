
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

/*
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
*/
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
#include <vapor/MetadataROMS.h>
#include <vapor/ROMS.h>
#include <vapor/WeightTable.h>
#include <vapor/WaveletBlock3DBufWriter.h>
#include <vapor/WaveletBlock3DRegionWriter.h>
#include <vapor/WaveCodecIO.h>

#ifdef _WINDOWS 
#include "windows.h"
#endif

using namespace VetsUtil;
using namespace VAPoR;

#define NC_ERR_READ(nc_status) \
    if (nc_status != NC_NOERR) { \
        cerr <<  ProgName << \
            " : Error reading netCDF file at line " <<  __LINE__  << \
			" : " << nc_strerror(nc_status) << endl; \
		return(-1); \
    }

//
//	Command line argument stuff
//
struct opt_t {
	vector<string> varnames;
	vector<string> missval;
	int numts;
	int level;
	int lod;
	int nthreads;
	float tolerance;
	OptionParser::Boolean_T	noelev;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"varnames",1, 	"",	"Colon delimited list of variable names in ROMS files to convert. The default is to convert variables in the set intersection of those variables found in the VDF file and those in the ROMS files."},
	{"numts",	1,	"-1","Maximum number of time steps that may be converted. A -1 implies the conversion of all time steps found"},
	{"level",	1, 	"-1","Refinement levels saved. 0=>coarsest, 1=>next refinement, etc. -1=>finest"},
	{"lod",	1, 	"-1",	"Compression levels saved. 0 => coarsest, 1 => "
		"next refinement, etc. -1 => all levels defined by the .vdf file"},
	{"nthreads",1, 	"0",	"Number of execution threads (0 => # processors)"},
	{"tolerance",	1, 	"0.0000001","Tolerance for comparing relative user times"},
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
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};

const char	*ProgName;

//Method to be used when a constant float value is used for a 3D array.
//The input array vertValues must have one value for each vertical level
int CopyConstantVariable3D(
				 const float *vertValues,
				 VDFIOBase *vdfio3d,
				 int level,
				 int lod, 
				 const char* varname,
				 const size_t dim[3],
				 size_t tsVDC) {
	
	static size_t sliceBufferSize = 0;
	static float *sliceBuffer = NULL;
	
	int rc;
	
	
	rc = vdfio3d->OpenVariableWrite(tsVDC, varname, level);
	
	if (rc<0) {
		MyBase::SetErrMsg(
						  "Failed to open for write variable %s at time step %d",
						  varname, tsVDC
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
		//fill the buffer with the appropriate value:
		for (size_t i = 0; i< slice_sz; i++){
			sliceBuffer[i] = vertValues[z];
		}
		if (vdfio3d->WriteSlice(sliceBuffer) < 0) {
			MyBase::SetErrMsg(
							  "Failed to write ROMS variable %s slice at time step %d ",
							  varname, tsVDC
							  );
			return (-1);
		}
	} // End of for z.
	
	vdfio3d->CloseVariable();
	
	return(0);
}  // End of CopyConstantVariable3D.
//
// Following method is used to create depth variable.  Data is provided in a 2D float array
int CopyConstantVariable2D(
				   const float* dataValues,
				   VDFIOBase *vdfio2d,
				   WaveletBlock3DRegionWriter *wb2dwriter,
				   int level,
				   int lod, 
				   const char* varname,
				   const size_t dim[3],
				   size_t tsVDC) {
	
	int rc;
	
	rc = vdfio2d->OpenVariableWrite(tsVDC, varname, level, lod);
	if (rc<0) {
		MyBase::SetErrMsg(
						  "Failed to copy ROMS variable %s at time step %d",
						  varname, tsVDC
						  );
		return (-1);
	}
	
 		
	
	if (vdfio2d->WriteRegion(dataValues) < 0) {
		MyBase::SetErrMsg(
						  "Failed to write ROMS variable %s at time step %d",
						  varname, tsVDC
						  );
		return (-1);
	}
	
	vdfio2d->CloseVariable();
	
	return(0);
} // End of CopyConstantVariable2D.


int CopyVariable3D(
	int ncid,
	int varid,
	WeightTable *wt,
	VDFIOBase *vdfio3d,
	int level,
	int lod, 
	string varname,
	const size_t dim[3],
	size_t tsVDC,
	int tsNetCDF
	
	) {

	static size_t sliceBufferSize = 0;
	static float *sliceBuffer = NULL;
	static float *sliceBuffer2 = NULL;

	int rc;
	
	rc = vdfio3d->OpenVariableWrite(tsVDC, varname.c_str(), level, lod);

	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to open for write ROMS variable %s at time step %d",
			varname.c_str(), tsVDC
		);
		return (-1);
	}
	
	float missVal = (float)ROMS::vaporMissingValue();
	rc = nc_get_att_float(ncid, varid, "missing_value", &missVal);

	
	size_t slice_sz = dim[0] * dim[1];
	size_t starts[4] = {0,0,0,0};
	size_t counts[4] = {1,1,1,1};
	starts[0]=tsNetCDF;
	counts[2]=dim[1];
	counts[3]=dim[0];

	if (sliceBufferSize < slice_sz) {
		if (sliceBuffer) {
			delete [] sliceBuffer;
			delete [] sliceBuffer2;
		}
		sliceBuffer = new float[slice_sz];
		sliceBuffer2 = new float[slice_sz];
		sliceBufferSize = slice_sz;
	}
	
	float minVal = 1.e30f;
	float maxVal = -1.e30f;
	float minVal1 = 1.e30f;
	float maxVal1 = -1.e30f;
	for (size_t z = 0; z<dim[2]; z++) {
		//
		//Read slice of NetCDF file into slice buffer
		//
		starts[1]= dim[2]-z-1;
		NC_ERR_READ(nc_get_vara_float(ncid, varid, starts, counts, sliceBuffer))
		
		for (int k = 0; k<slice_sz; k++){
			if (sliceBuffer[k] != missVal){
				if (minVal > sliceBuffer[k]) minVal = sliceBuffer[k];
				if (maxVal < sliceBuffer[k]) maxVal = sliceBuffer[k];
			}
		}
 	
		wt->interp2D(sliceBuffer, sliceBuffer2, missVal);
		for (int k = 0; k<slice_sz; k++){
			if (sliceBuffer2[k] != (float)ROMS::vaporMissingValue()){
				if (minVal1 > sliceBuffer2[k]) minVal1 = sliceBuffer2[k];
				if (maxVal1 < sliceBuffer2[k]) maxVal1 = sliceBuffer2[k];
			}
		}
		
		
		if (vdfio3d->WriteSlice(sliceBuffer2) < 0) {
			MyBase::SetErrMsg(
				"Failed to write ROMS variable %s slice at time step %d (vdc2)",
				varname.c_str(), tsVDC
			);
			return (-1);
		}
	} // End of for z.
	printf(" variable %s time %d: min, max original data: %g %g\n", varname.c_str(), (int)tsVDC, minVal, maxVal);
	printf(" variable %s time %d: min, max interpolated data: %g %g\n", varname.c_str(), (int)tsVDC, minVal1, maxVal1);

		vdfio3d->CloseVariable();

	return(0);
}  // End of CopyVariable3D.

int CopyVariable2D(
	int ncid,
	int varid,
	WeightTable* wt,
	VDFIOBase *vdfio2d,
	WaveletBlock3DRegionWriter *wb2dwriter,
	int level,
	int lod, 
	string varname,
	const size_t dim[3],
	size_t tsVDC,
	int tsNetCDF
	
) {

	static size_t sliceBufferSize = 0;
	static float *sliceBuffer = NULL;
	static float *sliceBuffer2 = NULL;

	int rc;

	rc = vdfio2d->OpenVariableWrite(tsVDC, varname.c_str(), level, lod);
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to copy ROMS variable %s at time step %d",
			varname.c_str(), tsVDC
		);
		return (-1);
	}
	
	float missingVal = -1.e30f;
	rc = nc_get_att_float(ncid, varid, "missing_value", &missingVal);
 
 	size_t slice_sz = dim[0] * dim[1];
	size_t starts[3] = {0,0,0};
	size_t counts[3];
	starts[0]=tsNetCDF;
	counts[0]=1;
	counts[1]=dim[1];
	counts[2]=dim[0];
	

	if (sliceBufferSize < slice_sz) {
		if (sliceBuffer) {
			delete [] sliceBuffer;
			delete [] sliceBuffer2;
		}
		sliceBuffer = new float[slice_sz];
		sliceBuffer2 = new float[slice_sz];
		sliceBufferSize = slice_sz;
	}

	//
	// Read the variable into the temp buffer
	//
	NC_ERR_READ(nc_get_vara_float(ncid, varid, starts, counts, sliceBuffer))
	
	
	
	//
	// Remap it 
	//
	
	wt->interp2D(sliceBuffer, sliceBuffer2, missingVal);
				
	if (vdfio2d->WriteRegion(sliceBuffer2) < 0) {
		MyBase::SetErrMsg(
			"Failed to write ROMS variable %s at ROMS time step %d",
			varname.c_str(), tsVDC
		);
		return (-1);
	}

	vdfio2d->CloseVariable();

	return(0);
} // End of CopyVariable2D.

void ErrMsgCBHandler(const char *msg, int) {
	cerr << ProgName << " : " << msg << endl;
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

	MyBase::SetErrMsgCB(ErrMsgCBHandler);

	if (opt.help) 
	{
		cerr << "Usage: " << ProgName << " [options] ROMS_netcdf_datafile(s)... ROMS_topo_file vdf_file" << endl << endl;
		op.PrintOptionHelp(stderr);
		exit(0);
	}

	if (argc < 3) {
		cerr << "Usage: " << ProgName << " [options] ROMS_netcdf_datafile(s)... ROMS_topo_file vdf_file" << endl;
		op.PrintOptionHelp(stderr);
		exit(1);
	}

	if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	argv++;
	argc--;
	//
	// At this point there is at least one ROMS file to work with.
	// The format is roms-files + topo-file + vdf-file.
	//
	
	vector<string> romsfiles;
	for (int i=0; i<argc-2; i++) {
		romsfiles.push_back(argv[i]);
	}
	string topofile = string(argv[argc-2]);
	string metafile = string(argv[argc-1]);
	
	vector<string> varnames = opt.varnames;

	WaveletBlock3DBufWriter *wbwriter3d = NULL;
	WaveletBlock3DRegionWriter *wbwriter2d = NULL;
	WaveCodecIO	*wcwriter3d = NULL;
	VDFIOBase* vdfio3d = NULL, *vdfio2d=NULL;
	
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
		

	//
	// Deal with required variables that may have atypical names
	//
	map <string, string> atypnames;
	string tag("DependentVarNames");
	string s = metadataVDC->GetUserDataString(tag);
	if (! s.empty()) {
		vector <string> svec;
		CvtToStrVec(s.c_str(), &svec);
		if (svec.size() != 6) {
			MyBase::SetErrMsg("Invalid DependentVarNames tag: %s",tag.c_str());
			return(-1);
		}
		atypnames["time"] = svec[0];
		atypnames["ht"] = svec[1];
		atypnames["st_ocean"] = svec[2];
		atypnames["st_edges_ocean"] = svec[3];
		atypnames["sw_ocean"] = svec[4];
		atypnames["sw_edges_ocean"] = svec[5];
		
	}
	//
	// Create a ROMS instance to get info from the topofile
	//
	
	// The ROMS constructor will check the topofile,
	// initialize the variable names
	ROMS* roms = new ROMS(topofile, atypnames, varnames, varnames);
	if (ROMS::GetErrCode() != 0) {
		ROMS::SetErrMsg(
			"Error processing topo file %s, exiting.",topofile.c_str()
					   );
        exit(1);
    }
	//
	// Verify that the ROMS matches the VDF file
	//
	
	//
	// Ask the ROMS to create all weighttables
	//
	// There needs to be a weightTable for each combination of geolat/geolon variables
	int rc = roms->MakeWeightTables();
	if (rc) exit (rc);
		
	//
	//Create DEPTH variable
	//
	size_t numTimeSteps = metadataVDC->GetNumTimeSteps();
	
	//Add depth variable
	float* depth = roms->GetDepths();
	
	if (depth){
		// use t-grid for remapping depth
		WeightTable *wt = roms->GetWeightTable(0,0);
		float* mappedDepth = new float[dimsVDC[0]*dimsVDC[1]];
		wt->interp2D(depth,mappedDepth, (float)ROMS::vaporMissingValue());
		float minval = 1.e30;
		float maxval = -1.e30;
		float minval1 = 1.e30;
		float maxval1 = -1.e30;
		for (int i = 0; i<dimsVDC[0]*dimsVDC[1]; i++){
			if(depth[i]<minval) minval = depth[i];
			if(depth[i]>maxval) maxval = depth[i];
			if(mappedDepth[i]<minval1) minval1 = mappedDepth[i];
			if(mappedDepth[i]>maxval1 && mappedDepth[i] != (float)ROMS::vaporMissingValue()) maxval1 = mappedDepth[i];
		}

		for( size_t t = 0; t< numTimeSteps; t++){
			int rc = CopyConstantVariable2D(mappedDepth,vdfio2d,wbwriter2d,opt.level,opt.lod, "DEPTH",dimsVDC,t);
			if (rc) exit(rc);
		}
	}
	delete depth;
	vector<string> vdcvars2d = metadataVDC->GetVariables2DXY();
	vector<string> vdcvars3d = metadataVDC->GetVariables3D();
	vector<size_t>VDCTimes;	
	
	//Create an array of times in the VDC:
	vector<double> usertimes;
	for (size_t ts = 0; ts < metadataVDC->GetNumTimeSteps(); ts++){
		usertimes.push_back(metadataVDC->GetTSUserTime(ts));
	}
	//Loop thru romsfiles
	for (int i = 0; i<romsfiles.size(); i++){
		printf("processing file %s\n",romsfiles[i].c_str());	
		
		//For each romsfile:
		//Open the file
		int ncid;
		char nctimename[NC_MAX_NAME+1];
		int timedimid;
		size_t timelen;
		NC_ERR_READ( nc_open( romsfiles[i].c_str(), NC_NOWRITE, &ncid ));
		
		// Read the times from the file
		int timevarid;
		int rc = nc_inq_varid(ncid, atypnames["time"].c_str(), &timevarid);
		if (rc){
			MyBase::SetErrMsg("Time variable (named: %s) not in file %s, skipping",
				atypnames["time"].c_str(),romsfiles[i].c_str());
			MyBase::SetErrCode(0);
			continue;
		}
		
		roms->extractStartTime(ncid,timevarid);
		
		NC_ERR_READ(nc_inq_unlimdim(ncid, &timedimid));
		
		NC_ERR_READ(nc_inq_dim(ncid, timedimid, nctimename, &timelen));

		size_t timestart[1] = {0};
		size_t timecount[1];
		timecount[0] = timelen;
		double* fileTimes = new double[timelen];
		VDCTimes.clear();
		NC_ERR_READ(nc_get_vara_double(ncid, timevarid, timestart, timecount, fileTimes));
		
		// map the times to VDC timesteps, using MetadataROMS::GetVDCTimes()
		for (int j = 0; j< timelen; j++){
			size_t ts = roms->GetVDCTimeStep(fileTimes[j],usertimes);
			if (ts == (size_t)-1) {
				MyBase::SetErrMsg(" Time step %d in file %s does not correspond to a valid time in the VDC",
								  j, romsfiles[i].c_str());
				exit (-2);
			}
			VDCTimes.push_back(ts);
		}
		//Loop thru its variables (2d & 3d):
		int nvars;
	
		NC_ERR_READ( nc_inq_nvars(ncid, &nvars ) )
		for (int varid = 0; varid<nvars; varid++){
			
			nc_type vartype;
			NC_ERR_READ( nc_inq_vartype(ncid, varid,  &vartype) );
			if (vartype != NC_FLOAT && vartype != NC_DOUBLE) continue;
			int ndims;
			NC_ERR_READ(nc_inq_varndims(ncid, varid, &ndims));
			if (ndims <3 || ndims >4) continue;
			//Get the variable name
			char varname[NC_MAX_NAME+1];
			NC_ERR_READ(nc_inq_varname(ncid, varid, varname))
			
			//Check that the name is in the VDC:
			bool varFound = false;
			if (ndims == 3){
				for (int n=0; n< vdcvars2d.size(); n++){
					if(vdcvars2d[n] == varname) {varFound = true; break;}
				}
			} else {
				for (int n = 0; n < vdcvars3d.size(); n++){
					if (vdcvars3d[n] == varname) {varFound = true; break;}
				}
			}
			if (!varFound) continue;
			//If varnames are specified, check that the variable is in the list
			if (opt.varnames.size()>0){
				bool found = false;
				for (vector<string>::iterator itr = opt.varnames.begin(); itr!=opt.varnames.end(); itr++) {
					if (*itr == varname) {
						found = true;
						break;
					}
				}
				if (!found) continue;
			}
			
			// Determine geolon/geolat vars
			int geolon, geolat;
			if (roms->GetGeoLonLatVar(ncid, varid,&geolon, &geolat)) continue;
			WeightTable* wt = roms->GetWeightTable(geolon, geolat);
			//loop thru the times in the file.
			for (int j = 0; j < timelen; j++){
				//for each time convert the variable
				if (ndims == 4)CopyVariable3D(ncid,varid,wt,vdfio3d,opt.level,opt.lod,  varname, dimsVDC,VDCTimes[j],j);
				else CopyVariable2D(ncid,varid,wt,vdfio2d,wbwriter2d,opt.level,opt.lod,  varname, dimsVDC,VDCTimes[j],j);
			}
		} //End loop over variables in file
			
					
				
	}
		
			
		exit(estatus);
}

