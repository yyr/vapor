#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <netcdf.h>
#include "proj_api.h"
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
	char* mapprojection;
	float extents[6];
	vector <string> vars3d;
	vector <string> vars2d;
	vector<string> dervars;
	vector<string> atypvars;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"dimension",1, "512x512x512",	"Volume dimensions (unstaggered) expressed in grid points (NXxNYxNZ) if no sample WRF file is given"},
	{"extents",	1,	"0:0:0:0:0:0",	"Colon delimited 6-element vector specifying domain extents in user coordinates (X0:Y0:Z0:X1:Y1:Z1) if different from that in sample WRF"},
	{"startt",	1,	"", "Starting time stamp, one of (time|SIMULATION_START_DATE|START_DATE), where time has the form : yyyy-mm-dd_hh:mm:ss"},
	{"numts",	1, 	"0",			"Maximum number of VDC time steps"},
	{"deltat",	1,	"0",			"Seconds per VDC time step"},
	{"varnames",1,	"",			"Deprecated. Use -vars3d instead"},
	{"vars3d",1,	"",			"Colon delimited list of 3D variables to be extracted from WRF data"},
	{"vars2d",1,    "",         "Colon delimited list of 2D variables to be extracted from WRF data"},
	{"dervars", 1,	"",	"Colon delimited list of desired derived variables.  Choices are: PHNorm_: normalized geopotential (PH+PHB)/PHB, UVW_: 3D wind speed (U^2+V^2+W^2)^1/2, UV_: 2D wind speed (U^2+V^2)^1/2, omZ_: estimate of vertical vorticity, PFull_: full pressure P+PB, PNorm_: normalized pressure (P+PB)/PB, Theta_: potential temperature T+300, TK_: temp. in Kelvin (T+300)((P+PB))/100000)^0.286"},
	{"level",	1, 	"2",			"Maximum refinement level. 0 => no refinement (default is 2)"},
	{"atypvars",1,	"U:V:W:PH:PHB:P:PB:T",		"Colon delimited list of atypical names for U:V:W:PH:PHB:P:PB:T that appear in WRF file"},
	{"comment",	1,	"",				"Top-level comment (optional)"},
	{"mapprojection",	1,	"",	"A whitespace delineated list of "
		"PROJ4 +paramname=paramvalue pairs. If this is invalid, then time-varying extents "
		"will not be provided in the metadata"},
	{"bs",		1, 	"32x32x32",		"Internal storage blocking factor expressed in grid points (NXxNYxNZ) (default is 32)"},
	{"nfilter",	1, 	"1",			"Number of wavelet filter coefficients (default is 1)"},
	{"nlifting",1, 	"1",			"Number of wavelet lifting coefficients (default is 1)"},
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
	{"vars3d", VetsUtil::CvtToStrVec, &opt.vars3d, sizeof(opt.vars3d)},
	{"vars2d", VetsUtil::CvtToStrVec, &opt.vars2d, sizeof(opt.vars2d)},
	{"dervars", VetsUtil::CvtToStrVec, &opt.dervars, sizeof(opt.dervars)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"atypvars", VetsUtil::CvtToStrVec, &opt.atypvars, sizeof(opt.atypvars)},
	{"comment", VetsUtil::CvtToString, &opt.comment, sizeof(opt.comment)},
	{"mapprojection", VetsUtil::CvtToString, &opt.mapprojection, sizeof(opt.mapprojection)},
	{"bs", VetsUtil::CvtToDimension3D, &opt.bs, sizeof(opt.bs)},
	{"nfilter", VetsUtil::CvtToInt, &opt.nfilter, sizeof(opt.nfilter)},
	{"nlifting", VetsUtil::CvtToInt, &opt.nlifting, sizeof(opt.nlifting)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};

//Define ordering of pairs <timestep, extents> so they can be sorted together
bool timeOrdering(pair<TIME64_T, float* > p,pair<TIME64_T, float* > q){
	if (p.first < q.first) return true;
	return false;
}

int	GetWRFMetadata(
	const char **files,
	int nfiles,
	const WRF::atypVarNames_t wrfNames,
	
	float extents[6],
	size_t dims[3],
	vector <string> &varnames3d,
	vector <string> &varnames2d,
	string startDate,
	string &mapProjection,
	vector<pair<TIME64_T, float*> > &tStepExtents
) {
	float _dx = 0.0;
	float _dy = 0.0;
	float _vertExts[2] = {0.0,0.0};
	string _startDate;
	string _mapProjection;
	
	vector<pair<TIME64_T,float*> > tsExtents; //one file's pairs of times and corners
	vector<string> _wrfVars3d; // Holds names of 3d variables in WRF file
	vector<string> _wrfVars2d; // Holds names of 2d variables in WRF file
	
	vector<pair<TIME64_T,float*> > _tStepExtents; //accumulate times/extents as obtained.
	size_t _dimLens[3] = {0,0,0};
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

		tsExtents.clear();
		wrfVars3d.clear();
		wrfVars2d.clear();
		rc = WRF::OpenWrfGetMeta(
			files[i], wrfNames, dx, dy, vertExtsPtr, dimLens, 
			startDate, mapProjection, wrfVars3d, wrfVars2d, 
			tsExtents
		);
		if (rc < 0 || (vertExtsPtr && vertExtsPtr[0] >= vertExtsPtr[1])) {
			cerr << "Error processing file " << files[i] << ", skipping" << endl;
			MyBase::SetErrCode(0);
			continue;
		}
		if (dx < 0.f || dy < 0.f) {
			cerr << "Error: DX and DY attributes not found in " << files[i] << ", skipping" << endl;
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
			_mapProjection = mapProjection;
		}
		else {
			bool mismatch = false;
			if (
				_dx != dx || _dy != dy || _dimLens[0] != dimLens[0] ||
				_dimLens[1] != dimLens[1] || _dimLens[2] != dimLens[2]) {
	
				mismatch = true;
			}

			if (mapProjection != _mapProjection) mismatch = true;

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

		//Copy all the timesteps & extents acquired from this file
		for (int j=0; j< tsExtents.size(); j++) {
			_tStepExtents.push_back(tsExtents[j]);
		}
		success = true;
	}

	// sort the time stamps, remove duplicates
	// time stamps are paired with time-varying extents
	//
	
	sort (_tStepExtents.begin(), _tStepExtents.end(), timeOrdering);
	tStepExtents.clear();
	if (_tStepExtents.size()) tStepExtents.push_back(_tStepExtents[0]);
	for (size_t i=1; i<_tStepExtents.size(); i++) {
		if (_tStepExtents[i].first != tStepExtents.back().first) {
			tStepExtents.push_back(_tStepExtents[i]);	// unique times
			
		}
	}

	dims[0] = _dimLens[0];
	dims[1] = _dimLens[1];
	dims[2] = _dimLens[2];

	if (extents) {

		extents[0] = 0.0;
		extents[1] = 0.0;
		extents[2] = _vertExts[0];
		extents[3] = _dx*(_dimLens[0]-1.);
		extents[4] = _dy*(_dimLens[1]-1.);
		extents[5] = _vertExts[1]; 
	}
	
	
	varnames3d = _wrfVars3d;
	varnames2d = _wrfVars2d;

	mapProjection = _mapProjection;

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
	vector <pair<TIME64_T,float*> > tStepExtents; //pairs of timestamps, extents
	vector <string> wrfVarNames3d, wrfVarNames2d;    // 2d and 3d vars
	vector <string> vdfVarNames3d;
	vector <string> vdfVarNames2d;
	string startT;
	string mapProj;
	bool userSpecifiedExtents = false;  //set true if user specifies on command line
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
		pair<TIME64_T,float*>pr;
		pr = make_pair(seconds, (float*)0);
		tStepExtents.push_back(pr);


		dim[0] = opt.dim.nx;
		dim[1] = opt.dim.ny;
		dim[2] = opt.dim.nz;

		vdfVarNames3d = opt.vars3d;
		vdfVarNames2d = opt.vars2d;

		
		for(int i=0; i<5; i++) {
			if (opt.extents[i] != opt.extents[i+1]) userSpecifiedExtents = true;
		}
		if (userSpecifiedExtents) {
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
			if (opt.extents[i] != opt.extents[i+1]) {
				extentsPtr = NULL;
				userSpecifiedExtents = true;
			}
		}
		if (! extentsPtr) {
			for(int i=0; i<6; i++) {
				extents[i] = opt.extents[i];
			}
		}

		string startDate;
		
		rc = GetWRFMetadata(
			(const char **)argv, argc-1, wrfNames, extentsPtr, 
			dim, wrfVarNames3d, wrfVarNames2d, startDate, mapProj, tStepExtents
		);
		if (rc<0) exit(1);

		
		if (strlen(opt.startt) != 0) {
			startT.assign(opt.startt);

			if ((startT.compare("SIMULATION_START_DATE") == 0) ||
				(startT.compare("START_DATE") == 0)) { 

				startT = startDate;
			}
		}

		// Check and see if the variable names specified on the command
		// line exist. Issue a warning if they don't, but we still
		// include them.
		//
		if (opt.vars3d.size()) {
			vdfVarNames3d = opt.vars3d;

			for ( int i = 0 ; i < opt.vars3d.size() ; i++ ) {

				bool foundVar = false;
				for ( int j = 0 ; j < wrfVarNames3d.size() ; j++ )
					if ( wrfVarNames3d[j] == opt.vars3d[i] ) {
						foundVar = true;
						break;
					}

				if ( !foundVar ) {
					cerr << ProgName << ": Warning: desired WRF 3D variable " << 
						opt.vars3d[i] << " does not appear in sample WRF file" << endl;
				}
			}
		}
		else {
			vdfVarNames3d = wrfVarNames3d;
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
					cerr << ProgName << ": Warning: desired WRF 2D variable " <<
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
	
		vector <pair<TIME64_T,float*> >::iterator itr = tStepExtents.begin();
		while ((*itr).first < seconds && itr != tStepExtents.end()) {
			itr++;
		}
		if (itr != tStepExtents.begin()) {
			tStepExtents.erase(tStepExtents.begin(), itr);
		}
	}

	// If an explict time sampling was specified use it
	//
	if (opt.deltat) {
		TIME64_T seconds = tStepExtents[0].first;
		int numts = Max(opt.numts, (int) tStepExtents.size());
		tStepExtents.clear();
		pair<TIME64_T,float*> pr;
		pr = make_pair(seconds,(float*)0);
		tStepExtents.push_back(pr);

		for (TIME64_T t = 1; t<numts; t++) {
			pr = make_pair(tStepExtents[0].first+(t*opt.deltat),(float*)0);
			tStepExtents.push_back(pr);
		}
	}

	// If a limit was specified on the number of time steps delete
	// excess ones
	//
	if (opt.numts && (opt.numts < tStepExtents.size())) {
		tStepExtents.erase(tStepExtents.begin()+opt.numts, tStepExtents.end());
	}
		
			
	// Add derived variables to the list of variables
	for ( int i = 0 ; i < opt.dervars.size() ; i++ )
	{
		if ( opt.dervars[i] == "PFull_" ) {
			vdfVarNames3d.push_back( "PFull_" );
		}
		else if ( opt.dervars[i] == "PNorm_" ) {
			vdfVarNames3d.push_back( "PNorm_" );
		}
		else if ( opt.dervars[i] == "PHNorm_" ) {
			vdfVarNames3d.push_back( "PHNorm_" );
		}
		else if ( opt.dervars[i] == "Theta_" ) {
			vdfVarNames3d.push_back( "Theta_" );
		}
		else if ( opt.dervars[i] == "TK_" ) {
			vdfVarNames3d.push_back( "TK_" );
		}
		else if ( opt.dervars[i] == "UV_" ) {
			vdfVarNames3d.push_back( "UV_" );
		}
		else if ( opt.dervars[i] == "UVW_" ) {
			vdfVarNames3d.push_back( "UVW_" );
		} 
		else if ( opt.dervars[i] == "omZ_" ) {
			vdfVarNames3d.push_back( "omZ_" );
		}
		else {
			cerr << ProgName << " : Invalid derived variable : " <<
				opt.dervars[i]  << endl;
			exit( 1 );
		}
	}

	// Always require the ELEVATION and HGT variables
	//
	vdfVarNames3d.push_back("ELEVATION");
	vdfVarNames2d.push_back("HGT");
		
	bs[0] = opt.bs.nx;
	bs[1] = opt.bs.ny;
	bs[2] = opt.bs.nz;
	file = new Metadata( dim,opt.level,bs,opt.nfilter,opt.nlifting );	

	if (Metadata::GetErrCode()) {
		exit(1);
	}

	if (file->SetNumTimeSteps(tStepExtents.size()) < 0) {
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

	

	// need a list of 2d and 3d varnames together
	//
	vector <string> varnames = vdfVarNames3d;
	for (int i=0; i<vdfVarNames2d.size(); i++) {
		varnames.push_back(vdfVarNames2d[i]);
	}
	if (file->SetVariableNames(varnames) < 0) {
		exit(1);
	}
	if (file->SetVariables2DXY(vdfVarNames2d) < 0) {
		exit(1);
	}

	//Set the map projection from the command line if it was there:
	if (strlen(opt.mapprojection)) {
		mapProj = opt.mapprojection;
	} //or set it from the WRF file
	if (mapProj.size() > 0) {
		if (file->SetMapProjection(mapProj) < 0) {
			cerr << Metadata::GetErrMsg() << endl;
			exit(1);
		}
	}
	// reproject the latlon to box coords
	int firstAvailTime = -1;
	double mappedExtents[4];
	if (mapProj.size()>0){
		projPJ p = pj_init_plus(mapProj.c_str());
		
		if (!p  && !opt.quiet){
			//Invalid string. Get the error code:
			int *pjerrnum = pj_get_errno_ref();
			fprintf(stderr, "Invalid geo-referencing in WRF output\n %s\n",
				pj_strerrno(*pjerrnum));
		}
		
		
		if (p){
			//reproject latlon to current coord projection.
			//Must convert to radians:
			const double DEG2RAD = 3.141592653589793/180.;
			const char* latLongProjString = "+proj=latlong +ellps=sphere";
			projPJ latlon_p = pj_init_plus(latLongProjString);
			
			double dbextents[4];
			vector<double> currExtents;
			
			for ( size_t t = 0 ; t < tStepExtents.size() ; t++ ){
				float* exts = tStepExtents[t].second;
				if (!exts) continue;
				//exts 0,1,2,3 are the lonlat extents, other 4 corners can be ignored here
				for (int j = 0; j<4; j++) dbextents[j] = exts[j]*DEG2RAD;
				int rc = pj_transform(latlon_p,p,2,2, dbextents,dbextents+1, 0);
				if (rc && opt.quiet){
					int *pjerrnum = pj_get_errno_ref();
					fprintf(stderr, "Error geo-referencing WRF domain extents\n %s\n",
						pj_strerrno(*pjerrnum));
				}
				//Put the reprojected extents into the vdf
				//Use the global specified extents for vertical coords
				if (!rc){
					currExtents.clear();
					currExtents.push_back(dbextents[0]);
					currExtents.push_back(dbextents[1]);
					currExtents.push_back(extents[2]);
					currExtents.push_back(dbextents[2]);
					currExtents.push_back(dbextents[3]);
					currExtents.push_back(extents[5]);
					file->SetTSExtents(t, currExtents);
					if (firstAvailTime == -1) {
						for (int k = 0; k<4; k++) mappedExtents[k] = dbextents[k];
						firstAvailTime = t;
					}

				}
			}

		}
	}
	//If the user specified the (global)extents, use those unmodified.
	//If the user didn't specify them, and specified no sample file, 
	//Then don't put them in the metadata either; 
	//so the metadata class can calculate.
	bool calcExtentsFromData = false;
	if (!userSpecifiedExtents) {
		//see if extents were calculated from data
		for(int i=0; i<5; i++) {
			if (extents[i] != extents[i+1]) {
				calcExtentsFromData = true;
			}
		}
	}
	
	//then put into metadata:
	if (userSpecifiedExtents || calcExtentsFromData){
		vector<double> extentsVec;
		for(int i=0; i<6; i++) {
			extentsVec.push_back(extents[i]);
		}
		if (file->SetExtents(extentsVec) < 0) {
			exit(1);
		}
	}
	
    // Set the user time for each time step.
	// In the meantime note the min/max latitude and longitude extents
	float minLat = 1000.f, minLon = 1000.f, maxLat = -1000.f, maxLon = -1000.f;
	
    for ( size_t t = 0 ; t < tStepExtents.size() ; t++ )
    {
		vector<double> tsNow(1, (double) tStepExtents[t].first);
        if ( file->SetTSUserTime( t, tsNow ) < 0) {
            exit( 1 );
        }

		// Add a user readable tag with the time stamp
		string tag("UserTimeStampString");
		string wrftime_str;
		(void) WRF::EpochToWRFTimeStr(tStepExtents[t].first, wrftime_str);

        if ( file->SetTSUserDataString( t, tag, wrftime_str ) < 0) {
            exit( 1 );
        }
		float* exts = tStepExtents[t].second;
		if (exts){ //Check all 4 corners for max and min longitude/latitude
			for (int cor = 0; cor < 4; cor++){
				if (exts[cor*2] < minLon) minLon = exts[cor*2];
				if (exts[cor*2] > maxLon) maxLon = exts[cor*2];
				if (exts[cor*2+1] < minLat) minLat = exts[cor*2+1];
				if (exts[cor*2+1] > maxLat) maxLat = exts[cor*2+1];
			}
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
		cout << "\tNum time steps : " << tStepExtents.size() << endl;
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

		cout << "\tCoordinate extents : ";
		const vector <double> extptr = file->GetExtents();
		for(int i=0; i<6; i++) {
			cout << extptr[i] << " ";
		}
		cout << endl;
		if (minLon < 1000.f){
			cout << "\tMin Longitude and Latitude of domain corners: " << minLon << " " << minLat << endl;
			cout << "\tMax Longitude and Latitude of domain corners: " << maxLon << " " << maxLat << endl;
		}
		
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

