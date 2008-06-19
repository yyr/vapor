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
	vector <string> vars2d;
	vector<string> dervars;
	vector<string> atypvars;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"dimension",1, "512x512x512",	"Volume dimensions (unstaggered) expressed in grid\n\t\t\t\tpoints (NXxNYxNZ) if no sample WRF file is\n\t\t\t\tgiven"},
	{"extents",	1,	"0:0:0:0:0:0",	"Colon delimited 6-element vector specifying\n\t\t\t\tdomain extents in user coordinates\n\t\t\t\t(X0:Y0:Z0:X1:Y1:Z1) if different from that in\n\t\t\t\tsample WRF"},
	{"startt",	1,	"", "Starting time stamp, one of \n\t\t\t\t(time|SIMULATION_START_DATE|START_DATE), \n\t\t\t\twhere time has the form : yyyy-mm-dd_hh:mm:ss"},
	{"numts",	1, 	"0",			"Maximum number of VDC time steps"},
	{"deltat",	1,	"0",			"Seconds per VDC time step"},
	{"varnames",1,	"",			"Colon delimited list of all (2D and 3D) variables to be\n\t\t\t\textracted from WRF data"},
	{"vars2d",1,    "",         "Colon delimited list of 2D variables to be\n\t\t\t\textracted from WRF data"},
	{"dervars", 1,	"",	"Colon delimited list of desired derived\n\t\t\t\tvariables.  Choices are:\n\t\t\t\tPHNorm_: normalized geopotential (PH+PHB)/PHB\n\t\t\t\tUVW_: 3D wind speed (U^2+V^2+W^2)^1/2\n\t\t\t\tUV_: 2D wind speed (U^2+V^2)^1/2\n\t\t\t\tomZ_: estimate of vertical vorticity\n\t\t\t\tPFull_: full pressure P+PB\n\t\t\t\tPNorm_: normalized pressure (P+PB)/PB\n\t\t\t\tTheta_: potential temperature T+300\n\t\t\t\tTK_: temp. in Kelvin\n\t\t\t\t\t(T+300)((P+PB))/100000)^0.286"},
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
	vector <TIME64_T> &timestamps,
	float extents[6],
	size_t dims[3],
	vector <string> &varnames3d,
	vector <string> &varnames2d,
	string startDate
) {
	float _dx = 0.0;
	float _dy = 0.0;
	float _vertExts[2];
	string _startDate;
	
	vector <TIME64_T> ts;
	vector<string> _wrfVars3d; // Holds names of 3d variables in WRF file
	vector<string> _wrfVars2d; // Holds names of 2d variables in WRF file
	vector <TIME64_T> _timestamps;
	size_t _dimLens[3];
	bool first = true;
	bool success = false;

	for (int i=0; i<nfiles; i++) {
		float dx, dy;
		float vertExts[2] = {0.0, 0.0};
		float *vertExtsPtr = vertExts;
		size_t dimLens[4];  //last one holds time, not used
		vector<string> wrfVars3d;
		vector<string> wrfVars2d;
		int rc;

		if (! extents) vertExtsPtr = NULL;

		ts.clear();
		wrfVars3d.clear();
		wrfVars2d.clear();
		rc = WRF::OpenWrfGetMeta(
			files[i], wrfNames, dx, dy, vertExtsPtr, dimLens, 
			startDate, wrfVars3d, wrfVars2d, ts 
		);
		if (rc < 0 || (vertExtsPtr && vertExtsPtr[0] >= vertExtsPtr[1])) {
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
			_wrfVars3d = wrfVars3d;
			_wrfVars2d = wrfVars2d;
			_startDate = startDate;
		}
		else {
			bool mismatch = false;
			if (
				_dx != dx || _dy != dy || _dimLens[0] != dimLens[0] ||
				_dimLens[1] != dimLens[1] || _dimLens[2] != dimLens[2] ||
				(_startDate.compare(startDate)!=0) ) {
	
				mismatch = true;
			}

			if (_wrfVars3d.size() != wrfVars3d.size()) {
				mismatch = true;
			}
			else {

				for (int j=0; j<wrfVars3d.size(); j++) {
					if (wrfVars3d[j].compare(_wrfVars3d[j]) != 0) {
						mismatch = true;
					}
				}
			}
			if (_wrfVars2d.size() != wrfVars2d.size()) {
				mismatch = true;
			}
			else {

				for (int j=0; j<wrfVars2d.size(); j++) {
					if (wrfVars2d[j].compare(_wrfVars2d[j]) != 0) {
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

	varnames3d = _wrfVars3d;
	varnames2d = _wrfVars2d;

	if (success) return(0);
	else return(-1);
}
	

const char *ProgName;

void Usage(OptionParser &op, const char * msg) {

	if (msg) {
		cerr << ProgName << " : " << msg << endl;
	}
	cerr << "Usage: " << ProgName << " [options] wrf_ncdf_file... vdf_file" << endl;
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
	vector <TIME64_T> timestamps;
	vector <string> wrfVarNames3d, wrfVarNames2d,
		wrfVarNames;    // 2d and 3d vars
	vector <string> vdfVarNames;
	vector <string> vdfVarNames2d;
	string startT;

	if (argc == 1) {	// No template file
	
		if (strlen(opt.startt) == 0) {
			Usage(op, "Expected -startt option");
			exit(1);
		}
		if (opt.numts > 1 && opt.deltat < 1) {
			Usage(op, "Invalid -deltat argument value : must be >= 1");
			exit(1);
		}

		// Set the first time stamp
		//
		startT.assign(opt.startt);
		TIME64_T seconds;
		if (WRF::WRFTimeStrToEpoch(opt.startt, &seconds) < 0) exit(1);
		timestamps.push_back(seconds);


		dim[0] = opt.dim.nx;
		dim[1] = opt.dim.ny;
		dim[2] = opt.dim.nz;

		vdfVarNames = opt.varnames;
		vdfVarNames2d = opt.vars2d;

		bool doExtents = false;
		for(int i=0; i<5; i++) {
			if (opt.extents[i] != opt.extents[i+1]) doExtents = true;
		}
		if (doExtents) {
			for(int i=0; i<6; i++) {
				extents[i] = opt.extents[i];
			}
		}
	}
	else if (argc > 1) {
		int rc;

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

		string startDate;
		rc = GetWRFMetadata(
			(const char **)argv, argc-1, wrfNames, timestamps, extentsPtr, 
			dim, wrfVarNames3d, wrfVarNames2d, startDate
		);
		if (rc<0) exit(1);

		if (strlen(opt.startt) != 0) {
			startT.assign(opt.startt);

			if ((startT.compare("SIMULATION_START_DATE") == 0) ||
				(startT.compare("START_DATE") == 0)) { 

				startT = startDate;
			}
		}
		wrfVarNames = wrfVarNames3d;
		for (int i=0; i<wrfVarNames2d.size(); i++) {
			wrfVarNames.push_back(wrfVarNames2d[i]);
		}

		// Check and see if the variable names specified on the command
		// line exist. Issue a warning if they don't, but we still
		// include them.
		//
		if (opt.varnames.size()) {
			vdfVarNames = opt.varnames;

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

		// Check and see if the 2D variable names specified on the command
		// line exist. Issue a warning if they don't, but we still
		// include them.
		//
		if (opt.vars2d.size()) {
			vdfVarNames2d = opt.vars2d;

			for ( int i = 0 ; i < opt.vars2d.size() ; i++ ) {

				bool foundVar = false;
				for ( int j = 0 ; j < wrfVarNames2d.size() ; j++ )
					if ( wrfVarNames2d[j] == opt.vars2d[i] ) {
						foundVar = true;
						break;
					}

				if ( !foundVar ) {
					cerr << ProgName << ": Warning: desired WRF variable " <<
						opt.vars2d[i] << " does not appear in sample WRF file" << endl;
				}
			}
		}
		else {
			vdfVarNames2d = wrfVarNames2d;
		}
	}
	else {
		Usage(op, "Invalid syntax");
		exit(1);
	}

	// If a start time was specified, make sure we have no time stamps
	// earlier than the start time
	//
	if (! startT.empty()) {
		TIME64_T seconds;
		if (WRF::WRFTimeStrToEpoch(startT, &seconds) < 0) exit(1);

		vector <TIME64_T>::iterator itr = timestamps.begin();
		while (*itr < seconds && itr != timestamps.end()) {
			itr++;
		}
		if (itr != timestamps.begin()) {
			timestamps.erase(timestamps.begin(), itr);
		}
	}

	// If an explict time sampling was specified use it
	//
	if (opt.deltat) {
		TIME64_T seconds = timestamps[0];
		int numts = Max(opt.numts, (int) timestamps.size());
		timestamps.clear();
		timestamps.push_back(seconds);

		for (TIME64_T t = 1; t<numts; t++) {
			timestamps.push_back(timestamps[0]+(t*opt.deltat));
		}
	}

	// If a limit was specified on the number of time steps delete
	// excess ones
	//
	if (opt.numts && (opt.numts < timestamps.size())) {
		timestamps.erase(timestamps.begin()+opt.numts, timestamps.end());
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
		} 
		else if ( opt.dervars[i] == "omZ_" ) {
			vdfVarNames.push_back( "omZ_" );
		}
		else {
			cerr << ProgName << " : Invalid derived variable : " <<
				opt.dervars[i]  << endl;
			exit( 1 );
		}
	}

	// Always require the ELEVATION and HGT variables
	//
	vdfVarNames.push_back("ELEVATION");
	vdfVarNames2d.push_back("HGT");
		
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
		}
		if (file->SetExtents(extentsVec) < 0) {
			exit(1);
		}
	}

	if (file->SetVariableNames(vdfVarNames) < 0) {
		exit(1);
	}
	if (file->SetVariables2DXY(vdfVarNames2d) < 0) {
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
		(void) WRF::EpochToWRFTimeStr(timestamps[t], wrftime_str);

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
		cout << "\t3D Variable names : ";
		for (int i=0; i<file->GetVariables3D().size(); i++) {
			cout << file->GetVariables3D()[i] << " ";
		}
		cout << endl;
		cout << "\t2D Variable names : ";
		for (int i=0; i<file->GetVariables2DXY().size(); i++) {
			cout << file->GetVariables2DXY()[i] << " ";
		}
		cout << endl;

		cout << "\tExtents : ";
		const vector <double> extptr = file->GetExtents();
		for(int i=0; i<6; i++) {
			cout << extptr[i] << " ";
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

