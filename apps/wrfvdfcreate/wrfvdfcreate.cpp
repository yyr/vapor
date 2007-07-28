#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <netcdf.h>

#include <vapor/OptionParser.h>
#include <vapor/Metadata.h>
#include <vapor/MetadataSpherical.h>

using namespace VetsUtil;
using namespace VAPoR;

int	cvtToExtents(const char *from, void *to);
int	cvtTo3DBool(const char *from, void *to);
int	cvtToOrder(const char *from, void *to);

#define NC_ERR_READ(nc_status) \
    if (nc_status != NC_NOERR) { \
        fprintf(stderr, \
            "Error reading netCDF file at line %d : %s \n",  __LINE__, nc_strerror(nc_status) \
        ); \
    exit(1);\
    }

struct opt_t {
	OptionParser::Dimension3D_T	dim;
	OptionParser::Dimension3D_T	bs;
	int	numts;
	int mindt;
	char * startt;
	int	level;
	int	nfilter;
	int	nlifting;
	char *comment;
	char *coordsystem;
	char *gridtype;
	float extents[6];
	int order[3];
	int periodic[3];
	vector <string> varnames;
	OptionParser::Boolean_T phnorm;
	OptionParser::Boolean_T wind3d;
	OptionParser::Boolean_T wind2d;
	OptionParser::Boolean_T pfull;
	OptionParser::Boolean_T pnorm;
	OptionParser::Boolean_T theta;
	OptionParser::Boolean_T tk;
	OptionParser::Boolean_T	mtkcompat;
	OptionParser::Boolean_T	help;
	char * smplwrf;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"dimension",1, "512x512x512",	"Volume dimensions expressed in grid points\n\t\t\t\t(NXxNYxNZ)"},
	{"numts",	1, 	"1",			"Number of timesteps"},
	{"mindt",	1,	"1",			"Smallest number of seconds between output time\n\t\t\t\tsteps in entire data set (default is 1)"},
	{"startt",	1,	"1970-01-01_00:00:00", "Starting time stamp, if different from that in\n\t\t\t\tsample WRF (default if no WRF is Jan 1, 1970)"},
	{"bs",		1, 	"32x32x32",		"Internal storage blocking factor expressed in\n\t\t\t\tgrid points (NXxNYxNZ)"},
	{"level",	1, 	"0",			"Maximum refinement level. 0 => no refinement"},
	{"nfilter",	1, 	"1",			"Number of wavelet filter coefficients"},
	{"nlifting",1, 	"1",			"Number of wavelet lifting coefficients"},
	{"comment",	1,	"",				"Top-level comment"},
	{"coordsystem",	1,	"cartesian","Top-level comment (cartesian|spherical)"},
	{"gridtype",	1,	"layered",	"Data grid type (regular|layered|stretched|\n\t\t\t\tblock_amr)"},
	{"extents",	1,	"0:0:0:0:0:0",	"Colon delimited 6-element vector specifying\n\t\t\t\tdomain extents in user coordinates\n\t\t\t\t(X0:Y0:Z0:X1:Y1:Z1)"},
	{"order",	1,	"0:1:2",	"Colon delimited 3-element vector specifying\n\t\t\t\tpermutation ordering of raw data on disk "},
	{"periodic",	1,	"0:0:0",	"Colon delimited 3-element boolean\n\t\t\t\t(0=>nonperiodic, 1=>periodic) vector specifying\n\t\t\t\tperiodicity of X,Y,Z coordinate axes (X:Y:Z)"},
	{"varnames",1,	"var1",			"Colon delimited list of variable names"},
	{"phnorm",0,	"", "Enable option of adding normalized geopotential \n\t\t\t\t(PH+PHB)/PHB to VDC"},
	{"wind3d",  0,  "", "Enable option of adding 3D wind\n\t\t\t\tspeed=(U^2+V^2+W^2)^1/2 to VDC"},
	{"wind2d",  0,  "", "Enable option of adding 2D wind\n\t\t\t\tspeed=(U^2+V^2)^1/2 to VDC"},
	{"pfull",	0,	"",	"Enable option of adding full pressure=P+PB to VDC"},
	{"pnorm",	0,	"",	"Enable option of adding normalized\n\t\t\t\tpressure=(P+PB)/PB to VDC"},
	{"theta",	0,	"",	"Enable option of adding Theta=T+300 to VDC"},
	{"tk",		0,	"",	"Enable option of adding temperature in\n\t\t\t\tKelvin=0.037*Theta*P_full^0.29 to VDC"},
	{"mtkcompat",	0,	"",			"Force compatibility with older mtk files"},
	{"help",	0,	"",				"Print this message and exit"},
	{"smplwrf", 1,  "???????",		"Sample WRF file from which to get dimensions,\n\t\t\t\textents, and starting time stamp"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"dimension", VetsUtil::CvtToDimension3D, &opt.dim, sizeof(opt.dim)},
	{"bs", VetsUtil::CvtToDimension3D, &opt.bs, sizeof(opt.bs)},
	{"numts", VetsUtil::CvtToInt, &opt.numts, sizeof(opt.numts)},
	{"mindt", VetsUtil::CvtToInt, &opt.mindt, sizeof(opt.mindt)},
	{"startt", VetsUtil::CvtToString, &opt.startt, sizeof(opt.startt)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"nfilter", VetsUtil::CvtToInt, &opt.nfilter, sizeof(opt.nfilter)},
	{"nlifting", VetsUtil::CvtToInt, &opt.nlifting, sizeof(opt.nlifting)},
	{"comment", VetsUtil::CvtToString, &opt.comment, sizeof(opt.comment)},
	{"coordsystem", VetsUtil::CvtToString, &opt.coordsystem, sizeof(opt.coordsystem)},
	{"gridtype", VetsUtil::CvtToString, &opt.gridtype, sizeof(opt.gridtype)},
	{"extents", cvtToExtents, &opt.extents, sizeof(opt.extents)},
	{"order", cvtToOrder, &opt.order, sizeof(opt.order)},
	{"periodic", cvtTo3DBool, &opt.periodic, sizeof(opt.periodic)},
	{"varnames", VetsUtil::CvtToStrVec, &opt.varnames, sizeof(opt.varnames)},
	{"phnorm", VetsUtil::CvtToBoolean,&opt.phnorm, sizeof(opt.phnorm)},
	{"wind3d", VetsUtil::CvtToBoolean, &opt.wind3d, sizeof(opt.wind3d)},
	{"wind2d", VetsUtil::CvtToBoolean, &opt.wind2d, sizeof(opt.wind2d)},
	{"pfull", VetsUtil::CvtToBoolean, &opt.pfull, sizeof(opt.pfull)},
	{"pnorm", VetsUtil::CvtToBoolean, &opt.pnorm, sizeof(opt.pnorm)},
	{"theta", VetsUtil::CvtToBoolean, &opt.theta, sizeof(opt.theta)},
	{"tk", VetsUtil::CvtToBoolean, &opt.tk, sizeof(opt.tk)},
	{"mtkcompat", VetsUtil::CvtToBoolean, &opt.mtkcompat, sizeof(opt.mtkcompat)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"smplwrf", VetsUtil::CvtToString, &opt.smplwrf, sizeof(opt.smplwrf)},
	{NULL}
};



void OpenWrfGetMeta(
					char * wrfName, // Name of the WRF file (in)
					float & dx, // Place to put DX attribute (out)
					float & dy, // Place to put DY attribute (out)
					size_t * dimLens, // Lengths of x, y, and z dimensions
					char * startDate // Place to put START_DATE attribute (out)
					)
{
	int nc_status; // Holds error codes for debugging
	int ncid; // Holds netCDF file ID
	int ndims; // Number of dimensions in netCDF
	int ngatts; // Number of global attributes
	int nvars; // Number of variables (not used)
	int xdimid; // ID of unlimited dimension (not used)
	
	char dimName[NC_MAX_NAME + 1]; // Temporary holder for dimension names
	bool foundX = false; // For error checking
	bool foundY = false;
	bool foundZ = false;

	// Open netCDF file and check for failure
	NC_ERR_READ( nc_status = nc_open( wrfName, NC_NOWRITE, &ncid ) );
	// Find the number of dimensions, variables, and global attributes, and check
	// the existance of the unlimited dimension (not that we need to)
	NC_ERR_READ( nc_status = nc_inq(ncid, &ndims, &nvars, &ngatts, &xdimid ) );

	// Find out dimension lengths.  x <==> west_east, y <==> south_north,
	// z <==> bottom_top.
	for ( int i = 0 ; i < ndims ; i++ )
	{
		NC_ERR_READ( nc_status = nc_inq_dimname( ncid, i, dimName ) );
		if ( strcmp( dimName, "west_east" ) == 0 )
		{
			foundX = true;
			NC_ERR_READ( nc_status = nc_inq_dimlen( ncid, i, &dimLens[0] ) );
			continue;
		}
		if ( strcmp( dimName, "south_north" ) == 0 )
		{
			foundY = true;
			NC_ERR_READ( nc_status = nc_inq_dimlen( ncid, i, &dimLens[1] ) );
			continue;
		}
		if ( strcmp( dimName, "bottom_top" ) == 0 )
		{
			foundZ = true;
			NC_ERR_READ( nc_status = nc_inq_dimlen( ncid, i, &dimLens[2] ) );
			continue;
		}
	}

	// Make sure we found all the dimensions we need
	if ( !foundX || !foundY || !foundZ )
	{
		fprintf( stderr, "Could not find typical dimensions in sample WRF file\n" );
		exit( 1 );
	}

	// Get DX and DY
	NC_ERR_READ( nc_status = nc_get_att_float( ncid, NC_GLOBAL, "DX", &dx ) );
	NC_ERR_READ( nc_status = nc_get_att_float( ncid, NC_GLOBAL, "DY", &dy ) );
	// Get starting time stamp
	NC_ERR_READ( nc_status = nc_get_att_text( ncid, NC_GLOBAL, "START_DATE", startDate ) );

	// Close the WRF file
	NC_ERR_READ( nc_status = nc_close( ncid ) );
}

int	main(int argc, char **argv) {

	OptionParser op;

	size_t bs[3];
	size_t dim[3];
	string	s;
	Metadata *file;

	if (op.AppendOptions(set_opts) < 0) {
		cerr << argv[0] << " : " << op.GetErrMsg();
		exit(1);
	}

	if (op.ParseOptions(&argc, argv, get_options) < 0) {
		cerr << argv[0] << " : " << OptionParser::GetErrMsg();
		exit(1);
	}

	if (opt.help) {
		cerr << "Usage: " << argv[0] << " [options] filename" << endl;
		op.PrintOptionHelp(stderr);
		exit(0);
	}

	if (argc != 2) {
		cerr << "Usage: " << argv[0] << " [options] filename" << endl;
		op.PrintOptionHelp(stderr);
		exit(1);
	}

	bs[0] = opt.bs.nx;
	bs[1] = opt.bs.ny;
	bs[2] = opt.bs.nz;
	dim[0] = opt.dim.nx;
	dim[1] = opt.dim.ny;
	dim[2] = opt.dim.nz;
	
	float dx = 1.0; // x grid partition width
	float dy = 1.0; // y grid partition width
	size_t dimLens[3]; // Grid dimensions
	char startDate[20]; // Starting time stamp from WRF sample
	// If this evaluates, we're using the sample WRF file to get the
	// dimensions, extents, and starting time stamp
	if ( strcmp( opt.smplwrf, "???????" ) != 0 )
	{
		OpenWrfGetMeta( opt.smplwrf, dx, dy, dimLens, startDate );
		dim[0] = dimLens[0];
		dim[1] = dimLens[1];
		dim[2] = dimLens[2];
	}

	s.assign(opt.coordsystem);
	/*if (s.compare("spherical") == 0) {
		size_t perm[] = {opt.order[0], opt.order[1], opt.order[2]};

		file = new MetadataSpherical(
			dim,opt.level,bs,perm, opt.nfilter,opt.nlifting
		);
	} */
	{
		if (opt.mtkcompat) {
			file = new Metadata(
				dim,opt.level,bs,opt.nfilter,opt.nlifting, 0, 0
			);
		}
		else {
			file = new Metadata(
				dim,opt.level,bs,opt.nfilter,opt.nlifting
			);
		}
	}

	if (Metadata::GetErrCode()) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}

	if (file->SetNumTimeSteps(opt.numts) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}

	s.assign(opt.comment);
	if (file->SetComment(s) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}

	s.assign(opt.gridtype);
	if (file->SetGridType(s) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}

#ifdef	DEAD
	s.assign(opt.coordsystem);
	if (file->SetCoordSystemType(s) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}
#endif

	int doExtents = 0;
	for(int i=0; i<5; i++) {
		if (opt.extents[i] != opt.extents[i+1]) doExtents = 1;
	}

	// If sample WRF file given, calculate horizontal extents from it
	// and guess at vertical extents, but not if user specified extents explicity
	if ( (strcmp( opt.smplwrf, "???????" ) != 0) && doExtents != 1)
	{
		opt.extents[0] = opt.extents[1] = opt.extents[2] = 0.0;
		opt.extents[3] = dx*dimLens[0];
		opt.extents[4] = dy*dimLens[1];
		opt.extents[5] = 19000.0; // Usually a good guess for WRF data
		doExtents = 1;
	}

	// let Metadata class calculate extents automatically if not 
	// supplied by user explicitly.
	//
	if ( doExtents ) {
		vector <double> extents;
		for(int i=0; i<6; i++) {
			extents.push_back(opt.extents[i]);
		}
		if (file->SetExtents(extents) < 0) {
			cerr << Metadata::GetErrMsg() << endl;
			exit(1);
		}
	}

	{
		vector <long> periodic_vec;

		for (int i=0; i<3; i++) periodic_vec.push_back(opt.periodic[i]);

		if (file->SetPeriodicBoundary(periodic_vec) < 0) {
			cerr << Metadata::GetErrMsg() << endl;
			exit(1);
		}
	}
	

	if (file->GetGridType().compare("layered") == 0){
		//Make sure there's an ELEVATION variable in the vdf
		bool hasElevation = false;
		for (int i = 0; i<opt.varnames.size(); i++){
			if (opt.varnames[i].compare("ELEVATION") == 0){
				hasElevation = true;
				break;
			}
		}
		if (!hasElevation){
			opt.varnames.push_back("ELEVATION");
		}
	}

	// Add derived quantities to the list of variables
	if ( opt.pfull )
		opt.varnames.push_back( "P_full" );
	if ( opt.pnorm )
		opt.varnames.push_back( "P_norm" );
	if ( opt.phnorm )
		opt.varnames.push_back( "PH_norm" );
	if ( opt.theta )
		opt.varnames.push_back( "Theta" );
	if ( opt.tk )
		opt.varnames.push_back( "TK" );
	if ( opt.wind2d )
		opt.varnames.push_back( "Wind_UV" );
	if ( opt.wind3d )
		opt.varnames.push_back( "Wind_UVW" );

	if (file->SetVariableNames(opt.varnames) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}

	// Set the time-related attributes

	// If there's a WRF file and no startt option, use WRF file's START_TIME
	if ( (strcmp( opt.smplwrf, "???????" ) != 0)
		 && (strcmp( opt.startt, "1970-01-01_00:00:00" ) == 0) )
		 opt.startt = startDate;
	// Else, use Unix epoch
	// Set the WRF_START_TIME user attribute
	if ( file->SetUserDataString( "WRF_START_TIME", opt.startt ) < 0)
	{
		cerr << Metadata::GetErrMsg() << endl;
		exit( 1 );
	}
	// Set the WRF_MIN_DT user attribute--it has to be a vector
	vector<long> mindtVec(1);
	mindtVec[0] = (long)opt.mindt;
	if ( file->SetUserDataLong( "WRF_MIN_DT", mindtVec ) < 0)
	{
		cerr << Metadata::GetErrMsg() << endl;
		exit( 1 );
	}


	if (file->Write(argv[1]) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}

	
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

