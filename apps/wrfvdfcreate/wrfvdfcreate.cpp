#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <netcdf.h>

#include <vapor/CFuncs.h>
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
		cerr << "Error reading netCDF file at line " <<  __LINE__ << " : " << nc_strerror(nc_status) << endl; \
		return(-1); \
    }

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
	char * smplwrf;
} opt;

// A struct for storing names of certain WRF variables
struct WRFNames {
	string U;
	string V;
	string W;
	string PH;
	string PHB;
	string P;
	string PB;
	string T;
} wrfNames;

OptionParser::OptDescRec_T	set_opts[] = {
	{"smplwrf", 1,  "",		"Sample WRF file from which to get dimensions,\n\t\t\t\textents, and starting time stamp (optional, see\n\t\t\t\tnext three options)"},
	{"dimension",1, "512x512x512",	"Volume dimensions (unstaggered) expressed in grid\n\t\t\t\tpoints (NXxNYxNZ) if no sample WRF file is\n\t\t\t\tgiven"},
	{"extents",	1,	"0:0:0:0:0:0",	"Colon delimited 6-element vector specifying\n\t\t\t\tdomain extents in user coordinates\n\t\t\t\t(X0:Y0:Z0:X1:Y1:Z1) if different from that in\n\t\t\t\tsample WRF"},
	{"startt",	1,	"", "Starting time stamp, one of \n\t\t\t\t(time|SIMULATION_START_DATE|START_DATE), \n\t\t\t\twhere time has the form : yyyy-mm-dd_hh:mm:ss"},
	{"numts",	1, 	"1",			"Number of Vapor time steps (default is 1)"},
	{"deltat",	1,	"3600",			"Seconds per Vapor time step (default is 3600)"},
	{"varnames",1,	"",			"Colon delimited list of all variables to be\n\t\t\t\textracted from WRF data"},
	{"dervars", 1,	"",	"Colon delimited list of desired derived\n\t\t\t\tvariables.  Choices are:\n\t\t\t\tphnorm: normalized geopotential (PH+PHB)/PHB\n\t\t\t\twind3d: 3D wind speed (U^2+V^2+W^2)^1/2\n\t\t\t\twind2d: 2D wind speed (U^2+V^2)^1/2\n\t\t\t\tpfull: full pressure P+PB\n\t\t\t\tpnorm: normalized pressure (P+PB)/PB\n\t\t\t\ttheta: potential temperature T+300\n\t\t\t\ttk: temp. in Kelvin 0.037*Theta*P_full^0.29"},
	{"level",	1, 	"2",			"Maximum refinement level. 0 => no refinement\n\t\t\t\t(default is 2)"},
	{"atypvars",1,	"",		"Colon delimited list of atypical names for\n\t\t\t\tU:V:W:PH:PHB:P:PB:T that appear in WRF file"},
	{"comment",	1,	"",				"Top-level comment (optional)"},
	{"bs",		1, 	"32x32x32",		"Internal storage blocking factor expressed in\n\t\t\t\tgrid points (NXxNYxNZ) (default is 32)"},
	{"nfilter",	1, 	"1",			"Number of wavelet filter coefficients (default\n\t\t\t\tis 1)"},
	{"nlifting",1, 	"1",			"Number of wavelet lifting coefficients (default\n\t\t\t\tis 1)"},
	{"help",	0,	"",				"Print this message and exit"},
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
	{NULL}
};



// A struct for storing info about a netCDF variable
struct varInfo
{
	string name; // Name of the variable in the VDF and WRF file
	int varid; // Variable ID in netCDF file
	int dimids[NC_MAX_VAR_DIMS]; // Array of dimension IDs that this variable uses
	size_t dimlens[NC_MAX_VAR_DIMS]; // Array of dimensions 
	int ndimids; // Number of dimensions of which the variable is a function
	nc_type xtype; // The type of the variable (float, double, etc.)
	bool stag[3]; // Indicates which of the fastest varying three dimensions 
				// are staggered. N.B. order of array is reversed from
				// netCDF convection. I.e. stag[0] == X, stag[2] == Z.
};


// structure for storing dimension info
typedef struct {
    char name[NC_MAX_NAME+1];	// dim name
    size_t size;				// dim len
} ncdim_t;



// Gets info about a 3D variable and stores that info in thisVar.
int	GetVarInfo(
	int ncid, // ID of the file we're reading
	const char *name,
	vector <ncdim_t> &ncdims,
	varInfo & thisVar // Variable info
) {
	int nc_status; // Holds error codes

	thisVar.name.assign(name);

	nc_status = nc_inq_varid(ncid, thisVar.name.c_str(), &thisVar.varid);
	if (nc_status != NC_NOERR) {
		MyBase::SetErrMsg(
			"Variable %s not found in netCDF file", thisVar.name.c_str()
		);
		return(-1);
	}

	char dummy[NC_MAX_NAME+1]; // Will hold a variable's name
	int natts; // Number of attributes associated with this variable
	nc_status = nc_inq_var(
		ncid, thisVar.varid, dummy, &thisVar.xtype, &thisVar.ndimids,
		thisVar.dimids, &natts
	);
	NC_ERR_READ(nc_status);

	for (int i = 0; i<thisVar.ndimids; i++) {
		thisVar.dimlens[i] = ncdims[thisVar.dimids[i]].size;
	}

	// Determine if variable has staggered dimensions. Only three
	// fastest varying dimensions are checked
	//
	thisVar.stag[0] = thisVar.stag[1] = thisVar.stag[2] =  false;

	size_t attlen;
	nc_status = nc_inq_attlen(ncid, thisVar.varid, "stagger", &attlen);
	if (nc_status == NC_ENOTATT) {
		return(0);
	}
	NC_ERR_READ(nc_status);

	char *buf = new char[attlen+1];
	nc_status = nc_get_att_text(ncid, thisVar.varid, "stagger", buf);
	buf[attlen] = '\0';

	string stagger(buf);

	string::size_type loc;

	if ((loc = stagger.find("X",0)) != string::npos) {
			thisVar.stag[0] = true;
	}
	if ((loc = stagger.find("Y",0)) != string::npos) {
			thisVar.stag[1] = true;
	}
	if ((loc = stagger.find("Z",0)) != string::npos) {
			thisVar.stag[2] = true;
	}
		
	return(0);
	
}



// Reads a single horizontal slice of netCDF data
int ReadZSlice4D(
				int ncid, // ID of the netCDF file
				varInfo & thisVar, // Struct for the variable we want
				size_t wrfT, // The WRF time step we want
				size_t z, // Which z slice we want
				float * fbuffer, // Buffer we're going to store slice in
				const size_t * dim // Dimensions from VDF
				)
{

	size_t start[NC_MAX_DIMS]; // The point from which we start reading netCDF data
	size_t count[NC_MAX_DIMS]; // What interval to read the data
	int nc_status; // Holds netCDF error codes

	// Initialize the count and start arrays for extracting slices from the data:
	for (int i = 0; i<thisVar.ndimids; i++){
		start[i] = 0; // Start reading in a corner
		count[i] = 1; // Read every point (changes later)
	}

	start[thisVar.ndimids-4] = wrfT;
	start[thisVar.ndimids-3] = z;
	count[thisVar.ndimids-1] = thisVar.dimlens[thisVar.ndimids-1];
	count[thisVar.ndimids-2] = thisVar.dimlens[thisVar.ndimids-2];

	nc_status = nc_get_vara_float(ncid, thisVar.varid, start, count, fbuffer);
	NC_ERR_READ(nc_status);

	return(0);
}



void InterpHorizSlice(
					  float * fbuffer, // The slice of data to interpolate
					  varInfo & thisVar, // Data about the variable
					  const size_t *dim // Dimensions from VDF
					  )
{
	// NOTE: As of July 20, 2007 (when this function was written), interpolating
	// data that is staggered in both horizontal dimensions has not been tested.

	size_t xUnstagDim = dim[0]; // Define these just for convenience
	size_t xStagDim = dim[0] + 1;
	size_t yUnstagDim = dim[1];
	size_t yStagDim = dim[1] + 1;

	size_t xDimWillBe = xUnstagDim; // More convenience
	size_t xDimNow, yDimNow;
	if ( thisVar.stag[0] )
		xDimNow = xStagDim;
	else
		xDimNow = xUnstagDim;

	if ( thisVar.stag[1] )
		yDimNow = yStagDim;
	else
		yDimNow = yUnstagDim;
	
	// Interpolate a staggered (N+1 points) x variable to an unstaggered (N points) x grid
	if ( thisVar.stag[0] )
	{
		size_t x = 0; // x grid point in staggered grid
		size_t y = 0; // y grid point in y's (unstaggered) grid
		float avg = 0.0; // Holds average
		for ( size_t l = 0 ; l < xDimWillBe*yDimNow ; l++ ) // l indexes storage location
		{
			if ( x < xDimWillBe ) // For all but the last x point for a given y
			{
				avg = (fbuffer[x + xDimNow*y] + fbuffer[x + 1 + xDimNow*y])/2.0;
				fbuffer[l] = avg;
				x++;
			}
			else // This is for the last x point for a given y
			{
				y++;
				avg = (fbuffer[y*xDimNow] + fbuffer[1 + y*xDimNow])/2.0;
				fbuffer[l] = avg;
				x=1;
			}
		}
	}

	// Interpolate a staggered (N+1 points) y variable to an unstaggered (N points) y grid
	if ( thisVar.stag[1] )
	{
		size_t x = 0; // x grid point in staggered grid
		size_t y = 0; // y grid point in y's (unstaggered) grid
		float avg = 0.0; // Holds average
		for ( size_t l = 0 ; l < xUnstagDim*(yUnstagDim - 1) ; l++ ) // l indexes storage location
		{
			avg = (fbuffer[x + xUnstagDim*y] + fbuffer[x + (y + 1)*xUnstagDim])/2.0;
			fbuffer[l] = avg;
			if ( x < xUnstagDim - 1 ) // For all but the last x point for a given y point
				x++;
			else // If this happens, you're at the last x value for a given y
			{
				x = 0;
				y++;
			}
		}
	}
}





// Reads a horizontal slice of the variable indicated by thisVar, and interpolates any
// points on a staggered grid to an unstaggered grid
int GetZSlice(
			   int ncid, // ID of netCDF file
			   varInfo & thisVar, // varInfo struct for the variable we're interested in
			   size_t wrfT, // The WRF time step we want
			   size_t z, // The (unstaggered) vertical coordinate of the plane we want
			   float * fbuffer, // Array into which we read the slice
			   float * fbufferAbove, // Pointer to temporary array, needed if variable
									 // is staggered vertically.  If variable is not
									 // staggered vertically, pass a null pointer.
			   bool & needAnother, // If variable is staggered vertically, this will be
								   // set to false after the first z slice is read, to
								   // reduce redundant reads
			   const size_t * dim // Dimensions from VDF
			   )
{
	// Read a slice from the netCDF
	if ( needAnother )
	{
		if (ReadZSlice4D( ncid, thisVar, wrfT, z, fbuffer, dim) < 0) return(-1);
		// Do horizontal interpolation of staggered grids, if necessary
		if ( thisVar.stag[0] || thisVar.stag[1] )
			InterpHorizSlice( fbuffer, thisVar, dim );
	}

	// If the vertical grid is staggered, we'll need the slice above
	// for interpolation
	if ( thisVar.stag[2] )
	{
		// If the vertical grid is staggered, save the slice above the one we
		// just wrote as the one we are going to work on next time
		if ( !needAnother )
		{
			float * tempPtr = fbuffer;
			fbuffer = fbufferAbove;
			fbufferAbove = tempPtr;
		}

		if (ReadZSlice4D(ncid, thisVar, wrfT, z+1, fbufferAbove, dim) < 0) return(-1);
		// Iterpolate horizontally, if needed
		if ( thisVar.stag[0] || thisVar.stag[1] )
			InterpHorizSlice( fbufferAbove, thisVar, dim );
		// Now do the vertical interpolation
		for ( size_t l = 0 ; l < dim[0]*dim[1] ; l++ )
		{
			fbuffer[l] += fbufferAbove[l];
			fbuffer[l] /= 2.0;
		}
		// Now we no longer need to read two slices
		if ( needAnother )
			needAnother = false;
	}

	return(0);
}


int WRFTimeStrToEpoch(
	string wrftime,
	long *etime
) {
	char *format = "%Y-%m-%d_%H:%M:%S";
	struct tm ts;

	if (strptime(wrftime.c_str(), format, &ts) == NULL) {
		MyBase::SetErrMsg("Unrecognized time stamp: %s", wrftime.c_str());
		return(-1);
	}

	*etime = mktime(&ts);
	if (*etime == (time_t) -1) {
		MyBase::SetErrMsg("Unrecognized time stamp: %s", wrftime.c_str());
		return(-1);
	}

	return(0);
}

int EpochToWRFTimeStr(
	long etime,
	string &wrftime
) {
	char *format = "%Y-%m-%d_%H:%M:%S";
	struct tm ts;

	if (localtime_r(&etime, &ts) == NULL) {
		MyBase::SetErrMsg("Unrecognized time stamp: %d", etime);
		return(-1);
	}

	char buf[128];
	strftime(buf, sizeof(buf), format, &ts);
	wrftime.assign(buf);

	return(0);
}


int OpenWrfGetMeta(
	const char * wrfName, // Name of the WRF file (in)
	float & dx, // Place to put DX attribute (out)
	float & dy, // Place to put DY attribute (out)
	float * vertExts, // Vertical extents (out)
	size_t * dimLens, // Lengths of x, y, and z dimensions (out)
	string &startDate, // Place to put START_DATE attribute (out)
	vector<string> & wrfVars, // Variable names in WRF file (out)
	vector <long> &timestamps // Time stamps, in seconds (out)
) {
	int nc_status; // Holds error codes for debugging
	int ncid; // Holds netCDF file ID
	int ndims; // Number of dimensions in netCDF
	int ngatts; // Number of global attributes
	int nvars; // Number of variables
	int xdimid; // ID of unlimited dimension (not used)
	
	char dimName[NC_MAX_NAME + 1]; // Temporary holder for dimension names
	
	// Open netCDF file and check for failure
	NC_ERR_READ( nc_status = nc_open( wrfName, NC_NOWRITE, &ncid ) );
	// Find the number of dimensions, variables, and global attributes, and check
	// the existance of the unlimited dimension (not that we need to)
	NC_ERR_READ( nc_status = nc_inq(ncid, &ndims, &nvars, &ngatts, &xdimid ) );

#ifdef	DEAD
	// Find out variable names
	for ( int i = 0 ; i < nvars ; i++ ) {
		char name[NC_MAX_NAME + 1];

		NC_ERR_READ( nc_status = nc_inq_varname( ncid, i, name ) );
		wrfVars.push_back( name );
	}
#endif


	// Find out dimension lengths.  x <==> west_east, y <==> south_north,
	// z <==> bottom_top.  Need names, too, for finding vertical extents.
	int timeId = -1;
	int weId = -1;
	int snId = -1;
	int btId = -1;
	int wesId = -1;
	int snsId = -1;
	int btsId = -1;
	for ( int i = 0 ; i < ndims ; i++ )
	{
		NC_ERR_READ( nc_status = nc_inq_dimname( ncid, i, dimName ) );
		if ( strcmp( dimName, "west_east" ) == 0 )
		{
			weId = i;
			NC_ERR_READ( nc_status = nc_inq_dimlen( ncid, i, &dimLens[0] ) );
		} else if ( strcmp( dimName, "south_north" ) == 0 )
		{
			snId = i;
			NC_ERR_READ( nc_status = nc_inq_dimlen( ncid, i, &dimLens[1] ) );
		} else if ( strcmp( dimName, "bottom_top" ) == 0 )
		{
			btId = i;
			NC_ERR_READ( nc_status = nc_inq_dimlen( ncid, i, &dimLens[2] ) );
		} else if ( strcmp( dimName, "west_east_stag" ) == 0 )
		{
			wesId = i;
		} else if ( strcmp( dimName, "south_north_stag" ) == 0 )
		{
			snsId = i;
		} else if ( strcmp( dimName, "bottom_top_stag") == 0 )
		{
			btsId = i;
		} else if ( strcmp( dimName, "Time" ) == 0 )
		{
			NC_ERR_READ( nc_status = nc_inq_dimlen( ncid, i, &dimLens[3] ) );
			timeId = i;
		}
	}

	// Make sure we found all the dimensions we need
	if ( weId < 0 || snId < 0 || btId < 0 )
	{
		MyBase::SetErrMsg("Could not find expected dimensions WRF file");
		return(-1);
	}
	if ( wesId < 0 || snsId < 0 || btsId < 0 ) {
		cerr << "Caution: could not find staggered dimensions in WRF file\n";
	}


	// Get DX and DY
	NC_ERR_READ( nc_status = nc_get_att_float( ncid, NC_GLOBAL, "DX", &dx ) );
	NC_ERR_READ( nc_status = nc_get_att_float( ncid, NC_GLOBAL, "DY", &dy ) );
	
	// Get starting time stamp
	// We'd prefer SIMULATION_START_DATE, but it's okay if it doesn't exist

	size_t attlen;
	char *start_attr = "SIMULATION_START_DATE";
	nc_status = nc_inq_attlen(ncid, NC_GLOBAL, start_attr, &attlen);
	if (nc_status == NC_ENOTATT) {
		start_attr = "START_DATE";
		nc_status = nc_inq_attlen(ncid, NC_GLOBAL, start_attr, &attlen);
		if (nc_status == NC_ENOTATT) {
			start_attr = NULL;
			startDate.erase();
		}
	}
	if (start_attr) {
		char *buf = new char[attlen];

		nc_status = nc_get_att_text( ncid, NC_GLOBAL, start_attr, buf );
		NC_ERR_READ(nc_status);

		startDate.assign(buf, attlen);
	}
		
    // build list of all dimensions found in the file
    //
	vector <ncdim_t> ncdims;
    for (int dimid = 0; dimid < ndims; dimid++) {
        ncdim_t dim;

        nc_status = nc_inq_dim(
            ncid, dimid, dim.name, &dim.size
        );
        NC_ERR_READ(nc_status);
        ncdims.push_back(dim);
    }

	// Find names of variables with appropriate dimmensions
	for ( int i = 0 ; i < nvars ; i++ ) {
		varInfo varinfo;
		char name[NC_MAX_NAME+1];
		NC_ERR_READ( nc_status = nc_inq_varname(ncid, i, name ) );
		if (GetVarInfo(ncid, name, ncdims, varinfo) < 0) continue;

		if ((varinfo.ndimids == 4) && 
			(varinfo.dimids[0] ==  timeId) &&
			((varinfo.dimids[1] ==  btId) || (varinfo.dimids[1] == btsId)) &&
			((varinfo.dimids[2] ==  snId) || (varinfo.dimids[2] == snsId)) &&
			((varinfo.dimids[3] ==  weId) || (varinfo.dimids[3] == wesId))) {

			wrfVars.push_back( name );
		}
	}


	// Get vertical extents if requested
	//
	if (vertExts) {

		// Get ready to read PH and PHB
		varInfo phInfo; // structs for variable information
		varInfo phbInfo;

		if (GetVarInfo( ncid, wrfNames.PH.c_str(), ncdims, phInfo) < 0) return(-1);
		if (phInfo.ndimids != 4) {
			MyBase::SetErrMsg("Variable %s has wrong # dims", wrfNames.PH.c_str());
			return(-1);
		}
		size_t ph_slice_sz = phInfo.dimlens[phInfo.ndimids-1] * 
			phInfo.dimlens[phInfo.ndimids-2];

		if (GetVarInfo( ncid, wrfNames.PHB.c_str(), ncdims, phbInfo) < 0) return(-1);
		if (phbInfo.ndimids != 4) {
			MyBase::SetErrMsg("Variable %s has wrong # dims", wrfNames.PHB.c_str());
			return(-1);
		}
		size_t phb_slice_sz = phbInfo.dimlens[phbInfo.ndimids-1] * 
			phbInfo.dimlens[phbInfo.ndimids-2];
			

		// Allocate memory
		float * phBuf = new float[ph_slice_sz];
		float * phBufTemp = new float[ph_slice_sz];
		float * phbBuf = new float[phb_slice_sz];
		float * phbBufTemp = new float[phb_slice_sz];
		
		// Dummies needed by function
		bool needAnotherPh = true;
		bool needAnotherPhb = true;

		for (size_t t = 0; t<dimLens[3]; t++) {
			bool first = true;
			float height;
			int rc;

			// Read bottom slices
			rc = GetZSlice(
				ncid, phInfo, t, 0, phBuf, phBufTemp, needAnotherPh, dimLens
			);
			if (rc<0) return (rc);

			rc = GetZSlice(
				ncid, phbInfo, t, 0, phbBuf, phbBufTemp, needAnotherPhb, 
				dimLens
			);
			if (rc<0) return (rc);

			if (first) {
				vertExts[0] = (phBuf[0] + phbBuf[0])/9.81;
			}
			
			for ( size_t i = 0 ; i < dimLens[0]*dimLens[1] ; i++ ) {

				height = (phBuf[i] + phbBuf[i])/9.81;
				// Want to find the bottom of the bottom layer and the bottom 
				// of the
				// top layer so that we can output them
				if (height < vertExts[0] ) vertExts[0] = height;
			}


			//  Read the top slices
			rc = GetZSlice(
				ncid, phInfo, t, dimLens[2] - 1, phBuf, phBufTemp, 
				needAnotherPh, dimLens
			);
			if (rc<0) return (rc);

			rc = GetZSlice(
				ncid, phbInfo, t, dimLens[2] - 1, phbBuf, phbBufTemp, 
				needAnotherPhb, dimLens
			);
			if (rc<0) return (rc);

			if (first) {
				vertExts[1] = (phBuf[0] + phbBuf[0])/9.81;
			}
			
			for ( size_t i = 0 ; i < dimLens[0]*dimLens[1] ; i++ ) {

				height = (phBuf[i] + phbBuf[i])/9.81;
				// Want to find the bottom of the bottom layer and the bottom 
				// of the
				// top layer so that we can output them
				if (height > vertExts[1] ) vertExts[1] = height;
			}
			first = false;

		}

		delete [] phBuf;
		delete [] phBufTemp;
		delete [] phbBuf;
		delete [] phbBufTemp;

	}

	// Get time stamps
	//
	varInfo timeInfo; // structs for variable information

	if (GetVarInfo( ncid, "Times", ncdims, timeInfo) < 0) return(-1);
	if (timeInfo.ndimids != 2) {
		MyBase::SetErrMsg("Variable %s has wrong # dims", "Times");
		return(-1);
	}
	size_t sz = timeInfo.dimlens[0] * timeInfo.dimlens[1];
	char *timesBuf = new char[sz];
	nc_status = nc_get_var_text(ncid, timeInfo.varid, timesBuf);
	for (int i =0; i<timeInfo.dimlens[0]; i++) {
		string time_fmt(timesBuf+(i*timeInfo.dimlens[1]), timeInfo.dimlens[1]);
		long etime;

		if (WRFTimeStrToEpoch(time_fmt, &etime) < 0) continue;

		timestamps.push_back(etime);
	}
	delete [] timesBuf;

	// Close the WRF file
	NC_ERR_READ( nc_status = nc_close( ncid ) );
	return(0);
}


int	GetWRFMetadata(
	const char **files,
	int nfiles,
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
		rc = OpenWrfGetMeta(
			 files[i], dx, dy, vertExtsPtr, dimLens, startDate, wrfVars, ts 
		);
		if (rc < 0) {
			cerr << "Error processing file " << files[i] << "(" << MyBase::GetErrMsg() << "), skipping" << endl;
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

		// Want min and max extents for entire data set
		//
		if (vertExts[0] < _vertExts[0]) _vertExts[0] = vertExts[0];
		if (vertExts[1] > _vertExts[1]) _vertExts[1] = vertExts[1];

		for (int j=0; j<ts.size(); j++) {
			_timestamps.push_back(ts[j]);
		}
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

	return(0);
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

	if (opt.help) {
		Usage(op, NULL);
		exit(0);
	}

	// Handle atypical variable naming
	if (! opt.atypvars.size()) {
		// Default values
		wrfNames.U = "U";
		wrfNames.V = "V";
		wrfNames.W = "W";
		wrfNames.PH = "PH";
		wrfNames.PHB = "PHB";
		wrfNames.P = "P";
		wrfNames.PB = "PB";
		wrfNames.T = "T";
	} 
	else if ( opt.atypvars.size() != 8 ) {
		cerr << "If -atypvars option is given, colon delimited list must have exactly eight elements specifying names of variables which are typically named U:V:W:PH:PHB:P:PB:T" << endl;
		exit( 1 );
	}
	else {
		wrfNames.U = opt.atypvars[0];
		wrfNames.V = opt.atypvars[1];
		wrfNames.W = opt.atypvars[2];
		wrfNames.PH = opt.atypvars[3];
		wrfNames.PHB = opt.atypvars[4];
		wrfNames.P = opt.atypvars[5];
		wrfNames.PB = opt.atypvars[6];
		wrfNames.T = opt.atypvars[7];
	}
	

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
		long t;
		if (WRFTimeStrToEpoch(opt.startt, &t) < 0) exit(1);
		timestamps.push_back(t);
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
				files, 1, timestamps, extentsPtr, dim, wrfVarNames, startDate
			);
			if (rc<0) exit(1);

			long t;
			if (strlen(opt.startt) != 0) {
				string startt(opt.startt);

				if ((startt.compare("SIMULATION_START_DATE") == 0) ||
					(startt.compare("START_DATE") == 0)) { 

					startt = startDate;
				}

				if (WRFTimeStrToEpoch(startt, &t) < 0) exit(1);
				timestamps[0] = t;
			}

			// Need to compute time stamps - ignore all but the
			// first returned by GetWRFMetadata()
			//
			t = timestamps[0];
			timestamps.clear();
			timestamps.push_back(t);
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
				(const char **)argv, argc-1, timestamps, extentsPtr, 
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
		if ( opt.dervars[i] == "pfull" ) {
			vdfVarNames.push_back( "P_full" );
		}
		else if ( opt.dervars[i] == "pnorm" ) {
			vdfVarNames.push_back( "P_norm" );
		}
		else if ( opt.dervars[i] == "phnorm" ) {
			vdfVarNames.push_back( "PH_norm" );
		}
		else if ( opt.dervars[i] == "theta" ) {
			vdfVarNames.push_back( "Theta" );
		}
		else if ( opt.dervars[i] == "tk" ) {
			vdfVarNames.push_back( "TK" );
		}
		else if ( opt.dervars[i] == "wind2d" ) {
			vdfVarNames.push_back( "Wind_UV" );
		}
		else if ( opt.dervars[i] == "wind3d" ) {
			vdfVarNames.push_back( "Wind_UVW" );
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
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}

	if (file->SetNumTimeSteps(timestamps.size()) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}

	s.assign(opt.comment);
	if (file->SetComment(s) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}

	s.assign("layered");
	if (file->SetGridType(s) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
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
			cerr << Metadata::GetErrMsg() << endl;
			exit(1);
		}
	}

	if (file->SetVariableNames(vdfVarNames) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}

    // Set the user time for each time step
    for ( size_t t = 0 ; t < timestamps.size() ; t++ )
    {
		vector<double> tsNow(1, (double) timestamps[t]);
        if ( file->SetTSUserTime( t, tsNow ) < 0) {
            cerr << Metadata::GetErrMsg() << endl;
            exit( 1 );
        }

		// Add a user readable comment with the time stamp
		string comment("User Time String : ");
		string wrftime_str;
		(void) EpochToWRFTimeStr(timestamps[t], wrftime_str);
		comment.append(wrftime_str);

        if ( file->SetTSComment( t, comment ) < 0) {
            cerr << Metadata::GetErrMsg() << endl;
            exit( 1 );
        }
    }

	if (file->Write(argv[argc-1]) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
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

