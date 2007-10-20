#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <netcdf.h>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/Metadata.h>
#include <vapor/MetadataSpherical.h>
#include <vapor/WRF.h>

using namespace VetsUtil;
using namespace VAPoR;

int	cvtToExtents(const char *from, void *to);
int	cvtTo3DBool(const char *from, void *to);
int	cvtToOrder(const char *from, void *to);

struct opt_t {
	OptionParser::Dimension3D_T	dim;
	OptionParser::Dimension3D_T	bs;
	int	numts;
	int deltat;
	char * startt;
	int	level;
	int	nfilter;
	int	nlifting;
	char *comment;
	float extents[6];
	vector <string> varnames;
	vector<string> dervars;
	vector<string> atypvars;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
	char * smplwrf;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"smplwrf", 1,  "",		"Sample WRF file from which to get dimensions,\n\t\t\t\textents, and starting time stamp (optional, see\n\t\t\t\tnext three options)"},
	{"dimension",1, "512x512x512",	"Volume dimensions (unstaggered) expressed in grid\n\t\t\t\tpoints (NXxNYxNZ) if no sample WRF file is\n\t\t\t\tgiven"},
	{"extents",	1,	"0:0:0:0:0:0",	"Colon delimited 6-element vector specifying\n\t\t\t\tdomain extents in user coordinates\n\t\t\t\t(X0:Y0:Z0:X1:Y1:Z1) if different from that in\n\t\t\t\tsample WRF"},
	{"startt",	1,	"", "Starting time stamp, one of \n\t\t\t\t(time|SIMULATION_START_DATE|START_DATE), \n\t\t\t\twhere time has the form : yyyy-mm-dd_hh:mm:ss"},
	{"numts",	1, 	"1",			"Number of Vapor time steps (default is 1)"},
	{"deltat",	1,	"3600",			"Seconds per Vapor time step (default is 3600)"},
	{"varnames",1,	"",			"Colon delimited list of all variables to be\n\t\t\t\textracted from WRF data"},
	{"dervars", 1,	"",	"Colon delimited list of desired derived\n\t\t\t\tvariables.  Choices are:\n\t\t\t\tPHNorm_: normalized geopotential (PH+PHB)/PHB\n\t\t\t\tUVW_: 3D wind speed (U^2+V^2+W^2)^1/2\n\t\t\t\tUV_: 2D wind speed (U^2+V^2)^1/2\n\t\t\t\tPFull_: full pressure P+PB\n\t\t\t\tPNorm_: normalized pressure (P+PB)/PB\n\t\t\t\tTheta_: potential temperature T+300\n\t\t\t\tTK_: temp. in Kelvin 0.037*Theta_*PFull_^0.29"},
	{"level",	1, 	"2",			"Maximum refinement level. 0 => no refinement\n\t\t\t\t(default is 2)"},
	{"atypvars",1,	"U:V:W:PH:PHB:P:PB:T",		"Colon delimited list of atypical names for\n\t\t\t\tU:V:W:PH:PHB:P:PB:T that appear in WRF file"},
	{"comment",	1,	"",				"Top-level comment (optional)"},
	{"bs",		1, 	"32x32x32",		"Internal storage blocking factor expressed in\n\t\t\t\tgrid points (NXxNYxNZ) (default is 32)"},
	{"nfilter",	1, 	"1",			"Number of wavelet filter coefficients (default\n\t\t\t\tis 1)"},
	{"nlifting",1, 	"1",			"Number of wavelet lifting coefficients (default\n\t\t\t\tis 1)"},
	{"help",	0,	"",				"Print this message and exit"},
	{"quiet",	0,	"",				"Operate quietly"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"smplwrf", VetsUtil::CvtToString, &opt.smplwrf, sizeof(opt.smplwrf)},
	{"dimension", VetsUtil::CvtToDimension3D, &opt.dim, sizeof(opt.dim)},
	{"extents", cvtToExtents, &opt.extents, sizeof(opt.extents)},
	{"startt", VetsUtil::CvtToString, &opt.startt, sizeof(opt.startt)},
	{"numts", VetsUtil::CvtToInt, &opt.numts, sizeof(opt.numts)},
	{"deltat", VetsUtil::CvtToInt, &opt.deltat, sizeof(opt.deltat)},
	{"varnames", VetsUtil::CvtToStrVec, &opt.varnames, sizeof(opt.varnames)},
	{"dervars", VetsUtil::CvtToStrVec, &opt.dervars, sizeof(opt.dervars)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"atypvars", VetsUtil::CvtToStrVec, &opt.atypvars, sizeof(opt.atypvars)},
	{"comment", VetsUtil::CvtToString, &opt.comment, sizeof(opt.comment)},
	{"bs", VetsUtil::CvtToDimension3D, &opt.bs, sizeof(opt.bs)},
	{"nfilter", VetsUtil::CvtToInt, &opt.nfilter, sizeof(opt.nfilter)},
	{"nlifting", VetsUtil::CvtToInt, &opt.nlifting, sizeof(opt.nlifting)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};

int	GetWRFMetadata(
	const char **files,
	int nfiles,
	const WRF::atypVarNames_t wrfNames,
	vector <long> &timestamps,
	float extents[6],
	size_t dims[3],
	vector <string> &varnames,
	string startDate
) {
	float _dx, _dy;
	float _vertExts[2];
	size_t _dimLens[4];
	vector <long> ts;
	vector<string> _wrfVars(0); // Holds names of variables in WRF file
	vector <long> _timestamps;

	bool first = true;
	bool success = false;

	for (int i=0; i<nfiles; i++) {
		float dx, dy;
		float vertExts[2] = {0.0, 0.0};
		float *vertExtsPtr = vertExts;
		size_t dimLens[3];
		vector<string> wrfVars(0); // Holds names of variables in WRF file
		int rc;

		if (! extents) vertExtsPtr = NULL;

		ts.clear();
		wrfVars.clear();
		rc = WRF::OpenWrfGetMeta(
			files[i], wrfNames, dx, dy, vertExtsPtr, dimLens, 
			startDate, wrfVars, ts 
		);
		if (rc < 0 || vertExtsPtr[0] >= vertExtsPtr[1]) {
			cerr << "Error processing file " << files[i] << ", skipping" << endl;
			MyBase::SetErrCode(0);
			continue;
		}
		if (first) {
			_dx = dx;
			_dy = dy;
			_vertExts[0] = vertExts[0];
			_vertExts[1] = vertExts[1];
			_dimLens[0] = dimLens[0];
			_dimLens[1] = dimLens[1];
			_dimLens[2] = dimLens[2];
			_wrfVars = wrfVars;
		}
		else {
			bool mismatch = false;
			if (
				_dx != dx || _dy != dy || _dimLens[0] != dimLens[0] ||
				_dimLens[1] != dimLens[1] || _dimLens[2] != dimLens[2]) {
	
				mismatch = true;
			}

			if (_wrfVars.size() != wrfVars.size()) {
				mismatch = true;
			}
			else {

				for (int j=0; j<wrfVars.size(); j++) {
					if (wrfVars[j].compare(_wrfVars[j]) != 0) {
						mismatch = true;
					}
				}
			}
			if (mismatch) {
				cerr << "File mismatch, skipping " << files[i] <<  endl;
				continue;
			}
		}
		first = false;

		// Want _minimum_ vertical extents for entire data set for bottom
		// and top layer.
		//
		if (vertExts[0] < _vertExts[0]) _vertExts[0] = vertExts[0];
		if (vertExts[1] < _vertExts[1]) _vertExts[1] = vertExts[1];

		for (int j=0; j<ts.size(); j++) {
			_timestamps.push_back(ts[j]);
		}
		success = true;
	}

	// sort the time stamps, remove duplicates
	//
	sort(_timestamps.begin(), _timestamps.end());
	timestamps.clear();
	if (_timestamps.size()) timestamps.push_back(_timestamps[0]);
	for (size_t i=1; i<_timestamps.size(); i++) {
		if (_timestamps[i] != timestamps.back()) {
			timestamps.push_back(_timestamps[i]);	// unique
		}
	}


	dims[0] = _dimLens[0];
	dims[1] = _dimLens[1];
	dims[2] = _dimLens[2];

	if (extents) {
		extents[0] = 0.0;
		extents[1] = 0.0;
		extents[2] = _vertExts[0];
		extents[3] = _dx*_dimLens[0];
		extents[4] = _dy*_dimLens[1];
		extents[5] = _vertExts[1]; 
	}

	varnames = _wrfVars;

	if (success) return(0);
	else return(-1);
}
	

const char *ProgName;

void Usage(OptionParser &op, const char * msg) {

	if (msg) {
		cerr << ProgName << " : " << msg << endl;
	}
	cerr << "Usage: " << ProgName << " [options] wrf_ncdf_file vdf_file" << endl;
	cerr << "Usage: " << ProgName << " [options] -samplwrf wrf_ncdf_file vdf_file" << endl;
	cerr << "Usage: " << ProgName << " [options] -startt time vdf_file" << endl;
	op.PrintOptionHelp(stderr);

}

void ErrMsgCBHandler(const char *msg, int) {
	cerr << ProgName << " : " << msg << endl;
}

int	main(int argc, char **argv) {

	OptionParser op;

	size_t bs[3];
	size_t dim[3];
	float extents[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	string	s;
	Metadata *file;

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
		exit(0);
	}

	WRF::atypVarNames_t wrfNames;

	// Handle atypical variable naming
	if ( opt.atypvars.size() != 8 ) {
		cerr << "If -atypvars option is given, colon delimited list must have exactly eight elements specifying names of variables which are typically named U:V:W:PH:PHB:P:PB:T" << endl;
		exit( 1 );
	}

	wrfNames.U = opt.atypvars[0];
	wrfNames.V = opt.atypvars[1];
	wrfNames.W = opt.atypvars[2];
	wrfNames.PH = opt.atypvars[3];
	wrfNames.PHB = opt.atypvars[4];
	wrfNames.P = opt.atypvars[5];
	wrfNames.PB = opt.atypvars[6];
	wrfNames.T = opt.atypvars[7];
	

	argv++;
	argc--;
	vector <long> timestamps;
	vector <string> wrfVarNames;
	vector <string> vdfVarNames;

	if (argc == 1 && strlen(opt.smplwrf) == 0) {	// No template file
	
		if (strlen(opt.startt) == 0) {
			Usage(op, "Expected -startt option");
			exit(1);
		}
		time_t seconds;
		if (WRF::WRFTimeStrToEpoch(opt.startt, &seconds) < 0) exit(1);
		timestamps.push_back((long) seconds);
		for (size_t t = 1; t<opt.numts; t++) {
			timestamps.push_back(timestamps[0]+(t*opt.deltat));
		}

		dim[0] = opt.dim.nx;
		dim[1] = opt.dim.ny;
		dim[2] = opt.dim.nz;

		wrfVarNames = opt.varnames;
		
	}
	else {	// Template file specified
		int rc;
		string startDate;

		// If the extents were provided on the command line, set the
		// extentsPtr to NULL so that we don't have to calulate the
		// extents by examining all the netCDF files
		//
		float *extentsPtr = extents;
		for(int i=0; i<5; i++) {
			if (opt.extents[i] != opt.extents[i+1]) extentsPtr = NULL;
		}
		if (! extentsPtr) {
			for(int i=0; i<6; i++) {
				extents[i] = opt.extents[i];
			}
		}

		if (argc == 1) {	// Single template

			const char **files = (const char **) &opt.smplwrf;
			rc = GetWRFMetadata(
				files, 1, wrfNames, timestamps, extentsPtr, dim, 
				wrfVarNames, startDate
			);
			if (rc<0) exit(1);

			if (strlen(opt.startt) != 0) {
				time_t seconds;
				string startt(opt.startt);

				if ((startt.compare("SIMULATION_START_DATE") == 0) ||
					(startt.compare("START_DATE") == 0)) { 

					startt = startDate;
				}

				if (WRF::WRFTimeStrToEpoch(startt, &seconds) < 0) exit(1);
				timestamps[0] = (long) seconds;
			}

			// Need to compute time stamps - ignore all but the
			// first returned by GetWRFMetadata()
			//
			long tsave = timestamps[0];
			timestamps.clear();
			timestamps.push_back(tsave);
			for (size_t t = 1; t<opt.numts; t++) {
				timestamps.push_back(timestamps[0]+(t*opt.deltat));
			}
		}
		else {
			if (strlen(opt.smplwrf) != 0) {
				Usage(op, "Unexpected -samplwrf option");
				exit(1);
			}
			rc = GetWRFMetadata(
				(const char **)argv, argc-1, wrfNames, timestamps, extentsPtr, 
				dim, wrfVarNames, startDate
			);
			if (rc<0) exit(1);
		}

		if (opt.varnames.size()) {
			vdfVarNames = opt.varnames;

			// Check and see if the variable names specified on the command
			// line exist. Issue a warning if they don't, but we still
			// include them.
			//
			for ( int i = 0 ; i < opt.varnames.size() ; i++ ) {

				bool foundVar = false;
				for ( int j = 0 ; j < wrfVarNames.size() ; j++ )
					if ( wrfVarNames[j] == opt.varnames[i] ) {
						foundVar = true;
						break;
					}

				if ( !foundVar ) {
					cerr << ProgName << ": Warning: desired WRF variable " << 
						opt.varnames[i] << " does not appear in sample WRF file" << endl;
				}
			}
		}
		else {
			vdfVarNames = wrfVarNames;
		}
	}

	// Add derived variables to the list of variables
	for ( int i = 0 ; i < opt.dervars.size() ; i++ )
	{
		if ( opt.dervars[i] == "PFull_" ) {
			vdfVarNames.push_back( "PFull_" );
		}
		else if ( opt.dervars[i] == "PNorm_" ) {
			vdfVarNames.push_back( "PNorm_" );
		}
		else if ( opt.dervars[i] == "PHNorm_" ) {
			vdfVarNames.push_back( "PHNorm_" );
		}
		else if ( opt.dervars[i] == "Theta_" ) {
			vdfVarNames.push_back( "Theta_" );
		}
		else if ( opt.dervars[i] == "TK_" ) {
			vdfVarNames.push_back( "TK_" );
		}
		else if ( opt.dervars[i] == "UV_" ) {
			vdfVarNames.push_back( "UV_" );
		}
		else if ( opt.dervars[i] == "UVW_" ) {
			vdfVarNames.push_back( "UVW_" );
		} else {
			cerr << ProgName << " : Invalid derived variable : " <<
				opt.dervars[i]  << endl;
			exit( 1 );
		}
	}

	// Always require the ELEVATION variable
	//
	vdfVarNames.push_back("ELEVATION");
		
	bs[0] = opt.bs.nx;
	bs[1] = opt.bs.ny;
	bs[2] = opt.bs.nz;
	file = new Metadata( dim,opt.level,bs,opt.nfilter,opt.nlifting );	

	if (Metadata::GetErrCode()) {
		exit(1);
	}

	if (file->SetNumTimeSteps(timestamps.size()) < 0) {
		exit(1);
	}

	s.assign(opt.comment);
	if (file->SetComment(s) < 0) {
		exit(1);
	}

	s.assign("layered");
	if (file->SetGridType(s) < 0) {
		exit(1);
	}

	// let Metadata class calculate extents automatically if not 
	// supplied by user explicitly.
	//
	int doExtents = 0;
    for(int i=0; i<5; i++) {
        if (extents[i] != extents[i+1]) doExtents = 1;
    }
	if (doExtents) {
		vector <double> extentsVec;
		for(int i=0; i<6; i++) {
			extentsVec.push_back(extents[i]);
			cerr << extents[i] << endl;
		}
		if (file->SetExtents(extentsVec) < 0) {
			exit(1);
		}
	}

	if (file->SetVariableNames(vdfVarNames) < 0) {
		exit(1);
	}

    // Set the user time for each time step
    for ( size_t t = 0 ; t < timestamps.size() ; t++ )
    {
		vector<double> tsNow(1, (double) timestamps[t]);
        if ( file->SetTSUserTime( t, tsNow ) < 0) {
            exit( 1 );
        }

		// Add a user readable tag with the time stamp
		string tag("UserTimeStampString");
		string wrftime_str;
		(void) WRF::EpochToWRFTimeStr((time_t) timestamps[t], wrftime_str);

        if ( file->SetTSUserDataString( t, tag, wrftime_str ) < 0) {
            exit( 1 );
        }
    }

	// Specify the atypical var names for dependent variables
	//
	string tag("DependentVarNames");
	string atypnames;
	atypnames.append(wrfNames.U); atypnames.append(":");
	atypnames.append(wrfNames.V); atypnames.append(":");
	atypnames.append(wrfNames.W); atypnames.append(":");
	atypnames.append(wrfNames.PH); atypnames.append(":");
	atypnames.append(wrfNames.PHB); atypnames.append(":");
	atypnames.append(wrfNames.P); atypnames.append(":");
	atypnames.append(wrfNames.PB); atypnames.append(":");
	atypnames.append(wrfNames.T);

	if ( file->SetUserDataString(tag, atypnames ) < 0) {
		exit( 1 );
	}

	if (file->Write(argv[argc-1]) < 0) {
		exit(1);
	}

	if (! opt.quiet) {
		cout << "Created VDF file:" << endl;
		cout << "\tNum time steps : " << timestamps.size() << endl;
		cout << "\tVariable names : ";
		for (int i=0; i<vdfVarNames.size(); i++) {
			cout << vdfVarNames[i] << " ";
		}
		cout << endl;

		cout << "\tExtents : ";
		for(int i=0; i<6; i++) {
			cout << extents[i] << " ";
		}
		cout << endl;
	}

	exit(0);
}

int	cvtToOrder(
	const char *from, void *to
) {
	int   *iptr   = (int *) to;

	if (! from) {
		iptr[0] = iptr[1] = iptr[2];
	}
	else if (!  (sscanf(from,"%d:%d:%d", &iptr[0],&iptr[1],&iptr[2]) == 3)) { 

		return(-1);
	}
	return(1);
}

int	cvtToExtents(
	const char *from, void *to
) {
	float   *fptr   = (float *) to;

	if (! from) {
		fptr[0] = fptr[1] = fptr[2] = fptr[3] = fptr[4] = fptr[5] = 0.0;
	}
	else if (! 
		(sscanf(from,"%f:%f:%f:%f:%f:%f",
		&fptr[0],&fptr[1],&fptr[2],&fptr[3],&fptr[4],&fptr[5]) == 6)) { 

		return(-1);
	}
	return(1);
}

int	cvtTo3DBool(
	const char *from, void *to
) {
	int   *iptr   = (int *) to;

	if (! from) {
		iptr[0] = iptr[1] = iptr[2] = 0;
	}
	else if (! (sscanf(from,"%d:%d:%d", &iptr[0],&iptr[1],&iptr[2]) == 3)) { 
		return(-1);
	}
	return(1);
}

