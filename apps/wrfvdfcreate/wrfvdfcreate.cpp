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
	int deltat;
	char * startt;
	int	level;
	int	nfilter;
	int	nlifting;
	char *comment;
	float extents[6];
	vector <string> varnames;
	vector<string> dervars;
	OptionParser::Boolean_T	help;
	char * smplwrf;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"smplwrf", 1,  "???????",		"Sample WRF file from which to get dimensions,\n\t\t\t\textents, and starting time stamp (optional, see\n\t\t\t\tnext three options)"},
	{"dimension",1, "512x512x512",	"Volume dimensions (unstaggered) expressed in grid\n\t\t\t\tpoints (NXxNYxNZ) if no sample WRF file is\n\t\t\t\tgiven"},
	{"extents",	1,	"0:0:0:0:0:0",	"Colon delimited 6-element vector specifying\n\t\t\t\tdomain extents in user coordinates\n\t\t\t\t(X0:Y0:Z0:X1:Y1:Z1) if different from that in\n\t\t\t\tsample WRF"},
	{"startt",	1,	"1970-01-01_00:00:00", "Starting time stamp, if different from\n\t\t\t\tSTART_DATE attribute in sample WRF\n\t\t\t\t(default if no WRF is Jan 1, 1970)"},
	{"numts",	1, 	"1",			"Number of Vapor time steps (default is 1)"},
	{"deltat",	1,	"1",			"Seconds per Vapor time step (default is 1)"},
	{"varnames",1,	"var1",			"Colon delimited list of all variables to be\n\t\t\t\textracted from WRF data"},
	{"dervars", 1,	"ELEVATION",	"Colon delimited list of desired derived\n\t\t\t\tvariables.  Choices are:\n\t\t\t\tphnorm: normalized geopotential (PH+PHB)/PHB\n\t\t\t\twind3d: 3D wind speed (U^2+V^2+W^2)^1/2\n\t\t\t\twind2d: 2D wind speed (U^2+V^2)^1/2\n\t\t\t\tpfull: full pressure P+PB\n\t\t\t\tpnorm: normalized pressure (P+PB)/PB\n\t\t\t\ttheta: potential temperature T+300\n\t\t\t\ttk: temp. in Kelvin 0.037*Theta*P_full^0.29"},
	{"level",	1, 	"2",			"Maximum refinement level. 0 => no refinement\n\t\t\t\t(default is 2)"},
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
	const char * name; // Name of the variable in the VDF and WRF file
	int varid; // Variable ID in netCDF file
	int dimIndex[3]; // Specifies which netCDF dimension index goes with x,y, and z
	int dimids[NC_MAX_VAR_DIMS]; // Array of dimension IDs that this variable uses
	nc_type xtype; // The type of the variable (float, double, etc.)
	int ndimids; // Number of dimensions of which the variable is a function
	bool stag[3]; // Indicates which of the dimensions are staggered
	size_t sliceSize; // Size of a z slice before any interpolation of staggered grids
};




// Gets info about a variable and stores that info in thisVar.
void GetVarInfo(
				 int ncid, // ID of the file we're reading
				 int ndims, // Number of dimensions
				 varInfo & thisVar, // Variable info
				 const size_t *dim //dimensions from vdf
				 )
{
	int nc_status; // Holds error codes

	size_t* ncdfdims; // The variable's dimension from the netCDF file
	
	//Check on the variable we are going to read; if found, store its ID in varid
	nc_status = nc_inq_varid(ncid, thisVar.name, &thisVar.varid);
	if (nc_status != NC_NOERR){
		fprintf(stderr, "variable %s not found in netcdf file\n", thisVar.name);
		exit(1);
	}

	//allocate array for dimensions in the netcdf file:
	ncdfdims = new size_t[ndims];

	//Get all the dimensions in the file:
	for (int d=0; d<ndims; d++) {
		nc_status = nc_inq_dimlen(ncid, d, &ncdfdims[d]);
		NC_ERR_READ(nc_status);
	}

	char name[NC_MAX_NAME+1]; // Will hold a variable's name
	int natts; // Number of attributes associated with this variable

	// For that variable, find all that stuff above
	nc_status = nc_inq_var( ncid, thisVar.varid, name, &thisVar.xtype, &thisVar.ndimids,
							thisVar.dimids, &natts );
	// Check for failure
	NC_ERR_READ(nc_status);

	// Check for valid variable type
	if ( thisVar.xtype != NC_FLOAT && thisVar.xtype != NC_DOUBLE )
	{
		cerr << ": Invalid variable data type" << endl;
		exit(1);
	}

	// Need at least 3 dimensions
	if (thisVar.ndimids < 3) {
		cerr << ": Insufficient netcdf variable dimensions" << endl;
		exit(1);
	}

	bool foundXDim = false, foundYDim = false, foundZDim = false;
	int dimIDs[3];// dimension ID's (in netcdf file) for each of the 3 dimensions we are using
	
	// Check on constant dimension
	int constDimID;
	if (nc_inq_dimid(ncid, "Time", &constDimID) != NC_NOERR)
	{
		fprintf(stderr, "Time dimension not found in WRF netCDF file\n" );
		exit(1);
	}
					
	//Go through the dimensions looking for the 3 dimensions that we are using,
	for (int i = 0; i<thisVar.ndimids; i++)
	{
		//For each dimension id, get the name associated with it
		nc_status = nc_inq_dimname(ncid, thisVar.dimids[i], name);
		NC_ERR_READ(nc_status);
		//See if that dimension name is a coordinate of the desired array.
		if (!foundXDim){
			if (strcmp(name, "west_east") == 0 || strcmp(name, "west_east_stag") == 0){
				foundXDim = true;
				dimIDs[0] = thisVar.dimids[i];
				thisVar.dimIndex[0] = i;
				//Allow dimension to be off by 1, in case of 
				//staggered dimensions.
				if (ncdfdims[dimIDs[0]] != dim[0] &&
					ncdfdims[dimIDs[0]] != (dim[0]+1)){
					fprintf(stderr, "NetCDF and VDF array do not match in dimension 0\n");
					exit(1);
				}
				// Set a flag if this dimension is staggered
				if ( ncdfdims[dimIDs[0]] == (dim[0] + 1) )
					thisVar.stag[0] = true;
				else
					thisVar.stag[0] = false;
				continue;
			} 
		} 
		if (!foundYDim) {
			if (strcmp(name, "south_north") == 0 || strcmp(name, "south_north_stag") == 0){
				foundYDim = true;
				dimIDs[1] = thisVar.dimids[i];
				thisVar.dimIndex[1] = i;
				if (ncdfdims[dimIDs[1]] != dim[1] &&
					ncdfdims[dimIDs[1]] != (dim[1]+1)){
					fprintf(stderr, "NetCDF and VDF array do not match in dimension 1\n");
					exit(1);
				}
				if ( ncdfdims[dimIDs[1]] == (dim[1] + 1) )
					thisVar.stag[1] = true;
				else
					thisVar.stag[1] = false;
				continue;
			}
		} 
		if (!foundZDim) {
			if (strcmp(name, "bottom_top") == 0 || strcmp(name, "bottom_top_stag") == 0){
				foundZDim = true;
				thisVar.dimIndex[2] = i;
				dimIDs[2] = thisVar.dimids[i];
				if (ncdfdims[dimIDs[2]] != dim[2] &&
					ncdfdims[dimIDs[2]] != (dim[2]+1)){
					fprintf(stderr, "NetCDF and VDF array do not match in dimension 2\n");
					exit(1);
				}
				if ( ncdfdims[dimIDs[2]] == (dim[2] + 1) )
					thisVar.stag[2] = true;
				else
					thisVar.stag[2] = false;
				continue;
			}
		}
		
	}
	if (!foundZDim || !foundYDim || !foundXDim){
		fprintf(stderr, "Specified Netcdf dimension name not found");
		exit(1);
	}


	// The size of a slice before any interpolation
    thisVar.sliceSize = 1;
	if ( thisVar.stag[0] )
		thisVar.sliceSize *= (dim[0] + 1);
	else
		thisVar.sliceSize *= dim[0];
	if ( thisVar.stag[1] )
		thisVar.sliceSize *= (dim[1] + 1);
	else
		thisVar.sliceSize *= dim[1];

	delete [] ncdfdims;
}



// Reads a single horizontal slice of netCDF data
void ReadZSlice(
				int ncid, // ID of the netCDF file
				varInfo & thisVar, // Struct for the variable we want
				size_t wrfT, // The WRF time step we want
				int z, // Which z slice we want
				float * fbuffer, // Buffer we're going to store slice in
				const size_t * dim // Dimensions from VDF
				)
{
	// Make sure this is an OK z slice before we waste any time
	if ( z > dim[2] )
		if ( !thisVar.stag[2] || (thisVar.stag[2] && z > dim[2] + 1) )
		{
			fprintf( stderr, "z slice %d exceeds dimension length\n", z );
			exit( 1 );
		}
		

	size_t* start; // The point from which we start reading netCDF data
	size_t* count; // What interval to read the data
	int nc_status; // Holds netCDF error codes

	// Initialize the count and start arrays for extracting slices from the data:
	count = new size_t[thisVar.ndimids];
	start = new size_t[thisVar.ndimids];
	for (int i = 0; i<thisVar.ndimids; i++){
		start[i] = 0; // Start reading in a corner
		count[i] = 1; // Read every point (changes later)
	}

	char name[NC_MAX_NAME+1]; // Will hold the dimension's name

	// Set start so that we start at the correct value of the constant dimensions
	for ( int i = 0 ; i < thisVar.ndimids ; i++ )
	{
		//For each dimension id, get the name associated with it
		NC_ERR_READ( nc_status = nc_inq_dimname(ncid, thisVar.dimids[i], name) );
		if (strcmp( name, "Time" ) == 0)
		{
			start[i] = wrfT;
			break;
		}
	}

	// Need this array in case we're reading in doubles instead of floats
	double *dbuffer = 0;
	if (thisVar.xtype == NC_DOUBLE) 
		dbuffer = new double[thisVar.sliceSize];

	//
	// Translate the volume one slice at a time
	//
	// Set up counts to only grab a z-slice
	for (int i = 0; i < thisVar.ndimids; i++)
	{
		count[i] = 1;
		if ( i == thisVar.dimIndex[0] )
		{
			if ( thisVar.stag[0] ) // Grab all x values
				count[i] = dim[0] + 1;
			else
				count[i] = dim[0]; 
		}
		if ( i == thisVar.dimIndex[1] )
		{
			if ( thisVar.stag[1] ) // Grab all y values
				count[i] = dim[1] + 1;
			else
				count[i] = dim[1];
		}
	}
	count[thisVar.dimIndex[2]] = 1; // Grab only 1 z value

//	TIMER_START(t1);
	start[thisVar.dimIndex[2]] = z;
		
	if (thisVar.xtype == NC_FLOAT)
	{
		NC_ERR_READ( nc_status = nc_get_vara_float( ncid, thisVar.varid, start, count, fbuffer ) );
	}
	else if (thisVar.xtype == NC_DOUBLE)
	{
		NC_ERR_READ( nc_status = nc_get_vara_double( ncid, thisVar.varid, start, count, dbuffer ) );
		//Convert to float:
		for(int i=0; i<dim[0]*dim[1]; i++)
			fbuffer[i] = (float)dbuffer[i];
	}
//	TIMER_STOP(t1, *read_timer);

	delete [] count;
	delete [] start;
	if ( dbuffer )
		delete [] dbuffer;
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
void GetZSlice(
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
		ReadZSlice( ncid, thisVar, wrfT, z, fbuffer, dim );
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

		ReadZSlice( ncid, thisVar, wrfT, z+1, fbufferAbove, dim );
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
}



void OpenWrfGetMeta(
					char * wrfName, // Name of the WRF file (in)
					float & dx, // Place to put DX attribute (out)
					float & dy, // Place to put DY attribute (out)
					float * vertExts, // Vertical extents (out)
					size_t * dimLens, // Lengths of x, y, and z dimensions (out)
					char * startDate, // Place to put START_DATE attribute (out)
					vector<string> & wrfVars // Variable names in WRF file (out)
					)
{
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

	// Find out variable names
	for ( int i = 0 ; i < nvars ; i++ )
	{
		// Might as well use dimName for now
		NC_ERR_READ( nc_status = nc_inq_varname( ncid, i, dimName ) );
		wrfVars.push_back( dimName );
	}


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
			continue;
		}
		if ( strcmp( dimName, "south_north" ) == 0 )
		{
			snId = i;
			NC_ERR_READ( nc_status = nc_inq_dimlen( ncid, i, &dimLens[1] ) );
			continue;
		}
		if ( strcmp( dimName, "bottom_top" ) == 0 )
		{
			btId = i;
			NC_ERR_READ( nc_status = nc_inq_dimlen( ncid, i, &dimLens[2] ) );
			continue;
		}
		if ( strcmp( dimName, "west_east_stag" ) == 0 )
		{
			wesId = i;
			continue;
		}
		if ( strcmp( dimName, "south_north_stag" ) == 0 )
		{
			snsId = i;
			continue;
		}
		if ( strcmp( dimName, "bottom_top_stag") == 0 )
		{
			btsId = i;
			continue;
		}
		if ( strcmp( dimName, "Time" ) == 0 )
			timeId = i;
	}

	// Make sure we found all the dimensions we need
	if ( weId < 0 || snId < 0 || btId < 0 )
	{
		fprintf( stderr, "Could not find typical dimensions in sample WRF file\n" );
		exit( 1 );
	}
	if ( wesId < 0 || snsId < 0 || btsId < 0 )
		fprintf( stderr, "Caution: could not find staggered dimensions in WRF file\n" );

	// Get DX and DY
	NC_ERR_READ( nc_status = nc_get_att_float( ncid, NC_GLOBAL, "DX", &dx ) );
	NC_ERR_READ( nc_status = nc_get_att_float( ncid, NC_GLOBAL, "DY", &dy ) );
	// Get starting time stamp
	NC_ERR_READ( nc_status = nc_get_att_text( ncid, NC_GLOBAL, "START_DATE", startDate ) );

	// Get ready to read PH and PHB
	varInfo phInfo; // structs for variable information
	varInfo phbInfo;
	phInfo.name = "PH";
	phbInfo.name = "PHB";

	GetVarInfo( ncid, ndims, phInfo, dimLens );
	GetVarInfo( ncid, ndims, phbInfo, dimLens );

	// Allocate memory
	float * phBuf = new float[phInfo.sliceSize];
	float * phBufTemp = new float[phInfo.sliceSize];
	float * phbBuf = new float[phbInfo.sliceSize];
	float * phbBufTemp = new float[phbInfo.sliceSize];
	float * workBuf = new float[dimLens[0]*dimLens[1]];
	
	// Dummies needed by function
	bool needAnotherPh = true;
	bool needAnotherPhb = true;
	vertExts[0] = vertExts[1] = 400000.0;

	for ( size_t z = 0 ; z < dimLens[2] ; z++ )
	{
		// Read needed slices
		GetZSlice( ncid, phInfo, 0, z, phBuf, phBufTemp, needAnotherPh, dimLens );
		GetZSlice( ncid, phbInfo, 0, z, phbBuf, phbBufTemp, needAnotherPhb, dimLens );
	
		for ( size_t i = 0 ; i < dimLens[0]*dimLens[1] ; i++ )
		{
			workBuf[i] = (phBuf[i] + phbBuf[i])/9.81;
			// Want to find the bottom of the bottom layer and the bottom of the
			// top layer so that we can output them
			if ( z == 0 && workBuf[i] < vertExts[0] )
				vertExts[0] = workBuf[i];
			if ( z == dimLens[2] - 1 && workBuf[i] < vertExts[1] )
				vertExts[1] = workBuf[i];
		}
						
	}

	delete [] phBuf;
	delete [] phBufTemp;
	delete [] phbBuf;
	delete [] phbBufTemp;
	delete [] workBuf;

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
	float vertExts[2]; // Find out vertical extents
	vector<string> wrfVars(0); // Holds names of variables in WRF file
	if ( strcmp( opt.smplwrf, "???????" ) != 0 )
	{
		OpenWrfGetMeta( opt.smplwrf, dx, dy, vertExts, dimLens, startDate, wrfVars );
		dim[0] = dimLens[0];
		dim[1] = dimLens[1];
		dim[2] = dimLens[2];
	}
	// In Windows, the final character of startDate is really ugly
	startDate[19]='\0';

	// Check to see if all the desired WRF variables are actually in the WRF file
	bool foundVar = false;
	if ( strcmp( opt.smplwrf, "???????" ) != 0 )
	{
		for ( int i = 0 ; i < opt.varnames.size() ; i++ )
		{
			foundVar = false;
			for ( int j = 0 ; j < wrfVars.size() ; j++ )
				if ( wrfVars[j] == opt.varnames[i] )
				{
					foundVar = true;
					break;
				}

			if ( !foundVar )
				cerr << argv[0] << ": Warning: desired WRF variable " << opt.varnames[i]
					 << "\n\tdoes not appear in sample WRF file." << endl;
		}
	}

	char coordsystem[] = "cartesian";
	s.assign(coordsystem);

	file = new Metadata( dim,opt.level,bs,opt.nfilter,opt.nlifting );	

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

	char gridtype[] = "layered";
	s.assign(gridtype);
	if (file->SetGridType(s) < 0) {
		cerr << Metadata::GetErrMsg() << endl;
		exit(1);
	}

#ifdef	DEAD
	s.assign(coordsystem);
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
		opt.extents[0] = opt.extents[1] = 0.0;
		opt.extents[2] = vertExts[0];
		opt.extents[3] = dx*dimLens[0];
		opt.extents[4] = dy*dimLens[1];
		opt.extents[5] = vertExts[1]; 
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

		if (file->SetPeriodicBoundary(periodic_vec) < 0) {
			cerr << Metadata::GetErrMsg() << endl;
			exit(1);
		}
	}
	
	// Add derived variables to the list of variables
	for ( int i = 0 ; i < opt.dervars.size() ; i++ )
	{
		if ( opt.dervars[i] == "pfull" )
			opt.varnames.push_back( "P_full" );
		else if ( opt.dervars[i] == "pnorm" )
			opt.varnames.push_back( "P_norm" );
		else if ( opt.dervars[i] == "phnorm" )
			opt.varnames.push_back( "PH_norm" );
		else if ( opt.dervars[i] == "theta" )
			opt.varnames.push_back( "Theta" );
		else if ( opt.dervars[i] == "tk" )
			opt.varnames.push_back( "TK" );
		else if ( opt.dervars[i] == "wind2d" )
			opt.varnames.push_back( "Wind_UV" );
		else if ( opt.dervars[i] == "wind3d" )
			opt.varnames.push_back( "Wind_UVW" );
		else if ( opt.dervars[i] == "ELEVATION" )
			opt.varnames.push_back( "ELEVATION" );
		else
		{
			fprintf( stderr, "Invalid derived variable\n" );
			exit( 1 );
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
	s.assign( opt.startt );
	if ( file->SetUserDataString( "WRF_START_TIME", s ) < 0)
	{
		cerr << Metadata::GetErrMsg() << endl;
		exit( 1 );
	}
	// Set the WRF_MIN_DT user attribute--it has to be a vector
	vector<long> mindtVec(1);
	mindtVec[0] = (long)opt.deltat;
	if ( file->SetUserDataLong( "WRF_MIN_DT", mindtVec ) < 0)
	{
		cerr << Metadata::GetErrMsg() << endl;
		exit( 1 );
	}

	// Set the user time for each time step
	vector<double> tsNow(1);
	for ( size_t i = 0 ; i < opt.numts ; i++ )
	{
		tsNow[0] = (double)( i*(opt.deltat) );
		if ( file->SetTSUserTime( i, tsNow ) < 0)
		{
			cerr << Metadata::GetErrMsg() << endl;
			exit( 1 );
		}
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

