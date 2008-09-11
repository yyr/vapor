//
// $Id$
//
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <ctime>
#include <netcdf.h>

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/Metadata.h>
#include <vapor/MetadataSpherical.h>
#include <vapor/WRF.h>

using namespace VetsUtil;
using namespace VAPoR;

#define NC_ERR_READ(X) \
	{ \
	int nc_statusxxx = (X); \
    if (nc_statusxxx != NC_NOERR) { \
		cerr << "Error reading netCDF file at line " <<  __LINE__ << " : " << nc_strerror(nc_statusxxx) << endl; \
		return(-1); \
	} \
    }

// Gets info about a 3D variable and stores that info in thisVar.
int	WRF::GetVarInfo(
	int ncid, // ID of the file we're reading
	const char *name,
	const vector <ncdim_t> &ncdims,
	varInfo & thisVar // Variable info
) {

	thisVar.name.assign(name);

	int nc_status = nc_inq_varid(ncid, thisVar.name.c_str(), &thisVar.varid);
	if (nc_status != NC_NOERR) {
		MyBase::SetErrMsg(
			"Variable %s not found in netCDF file", thisVar.name.c_str()
		);
		return(-1);
	}

	char dummy[NC_MAX_NAME+1]; // Will hold a variable's name
	int natts; // Number of attributes associated with this variable
	NC_ERR_READ( nc_inq_var(
		ncid, thisVar.varid, dummy, &thisVar.xtype, &thisVar.ndimids,
		thisVar.dimids, &natts
	));

	for (int i = 0; i<thisVar.ndimids; i++) {
		thisVar.dimlens[i] = ncdims[thisVar.dimids[i]].size;
	}

	// Determine if variable has staggered dimensions. Only three
	// fastest varying dimensions are checked
	//
	thisVar.stag[0] = thisVar.stag[1] = thisVar.stag[2] =  false;

	if (thisVar.ndimids == 4) { 
		if (strcmp(ncdims[thisVar.dimids[1]].name, "bottom_top_stag") == 0) {
			thisVar.stag[2] = true;
		}
		if (strcmp(ncdims[thisVar.dimids[2]].name, "south_north_stag") == 0) { 
			thisVar.stag[1] = true;
		}
		if (strcmp(ncdims[thisVar.dimids[3]].name, "west_east_stag") == 0) {
			thisVar.stag[0] = true;
		}
	}
	else if (thisVar.ndimids == 3) { 
		if (strcmp(ncdims[thisVar.dimids[1]].name, "south_north_stag") == 0) {
			thisVar.stag[1] = true;
		}
		if (strcmp(ncdims[thisVar.dimids[2]].name, "west_east_stag") == 0) { 
			thisVar.stag[0] = true;
		}
	}

	return(0);
	
}



// Reads a single horizontal slice of netCDF data
int WRF::ReadZSlice4D(
	int ncid, // ID of the netCDF file
	varInfo & thisVar, // Struct for the variable we want
	size_t wrfT, // The WRF time step we want
	size_t z, // Which z slice we want
	float * fbuffer, // Buffer we're going to store slice in
	const size_t * dim // Dimensions from VDF
) {

	size_t start[NC_MAX_DIMS]; // The point from which we start reading netCDF data
	size_t count[NC_MAX_DIMS]; // What interval to read the data

	// Initialize the count and start arrays for extracting slices from the data:
	for (int i = 0; i<thisVar.ndimids; i++){
		start[i] = 0; // Start reading in a corner
		count[i] = 1; // Read every point (changes later)
	}

	start[thisVar.ndimids-4] = wrfT;
	start[thisVar.ndimids-3] = z;
	count[thisVar.ndimids-2] = thisVar.dimlens[thisVar.ndimids-2];
	count[thisVar.ndimids-1] = thisVar.dimlens[thisVar.ndimids-1];

	NC_ERR_READ( nc_get_vara_float(ncid, thisVar.varid, start, count, fbuffer));

	return(0);
}

// Reads a single slice from a 3D (time + space) variable 
int WRF::ReadZSlice3D(
	int ncid, // ID of the netCDF file
	varInfo & thisVar, // Struct for the variable we want
	size_t wrfT, // The WRF time step we want
	float * fbuffer // Buffer we're going to store slice in
) {

	size_t start[NC_MAX_DIMS]; // The point from which we start reading netCDF data
	size_t count[NC_MAX_DIMS]; // What interval to read the data

	// Initialize the count and start arrays for extracting slices from the data:
	for (int i = 0; i<thisVar.ndimids; i++){
		start[i] = 0; // Start reading in a corner
		count[i] = 1; // Read every point (changes later)
	}

	start[thisVar.ndimids-3] = wrfT;
	count[thisVar.ndimids-2] = thisVar.dimlens[thisVar.ndimids-2];
	count[thisVar.ndimids-1] = thisVar.dimlens[thisVar.ndimids-1];

	NC_ERR_READ( nc_get_vara_float(ncid, thisVar.varid, start, count, fbuffer));

	return(0);
}



void WRF::InterpHorizSlice(
	float * fbuffer, // The slice of data to interpolate
	varInfo & thisVar, // Data about the variable
	const size_t *dim // Dimensions from VDF
) {
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


int WRF::GetZSlice(
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
) {
	int rc;
	// Read a slice from the netCDF
	if ( thisVar.ndimids == 3 )
	{
		if (ReadZSlice3D( ncid, thisVar, wrfT, fbuffer) < 0) return(-1);
		// Do horizontal interpolation of staggered grids, if necessary
		if ( thisVar.stag[0] || thisVar.stag[1] )
			InterpHorizSlice( fbuffer, thisVar, dim );
	}

	// Read a slice from the netCDF
	if ( needAnother )
	{
		rc = ReadZSlice4D( ncid, thisVar, wrfT, z, fbuffer, dim);
		if (rc < 0) return(-1);
		// Do horizontal interpolation of staggered grids, if necessary
		if ( thisVar.stag[0] || thisVar.stag[1] )
			InterpHorizSlice( fbuffer, thisVar, dim );
	}

	// If the vertical grid is staggered, we'll need the slice above
	// for interpolation
	if ( thisVar.stag[2] )
	{
		if ( needAnother ) {
			// Now we no longer need to read two slices
			needAnother = false;
		}
		else {
			memcpy(fbuffer, fbufferAbove, dim[0]*dim[1]*sizeof(fbuffer[0]));
		}

		rc =  ReadZSlice4D(ncid, thisVar, wrfT, z+1, fbufferAbove, dim);
		if (rc < 0) return(-1);
		// Iterpolate horizontally, if needed
		if ( thisVar.stag[0] || thisVar.stag[1] )
			InterpHorizSlice( fbufferAbove, thisVar, dim );

		// Now do the vertical interpolation
		for ( size_t l = 0 ; l < dim[0]*dim[1] ; l++ )
		{
			fbuffer[l] = (fbuffer[l] + fbufferAbove[l]) / 2.0;
		}
	}

	return(0);
}

int WRF::WRFTimeStrToEpoch(
	const string &wrftime,
	TIME64_T *seconds
) {

	int rc;
	const char *format = "%4d-%2d-%2d_%2d:%2d:%2d";
	struct tm ts;

	rc = sscanf(
		wrftime.c_str(), format, &ts.tm_year, &ts.tm_mon, &ts.tm_mday, 
		&ts.tm_hour, &ts.tm_min, &ts.tm_sec
	);
	if (rc != 6) {
		MyBase::SetErrMsg("Unrecognized time stamp: %s", wrftime.c_str());
		return(-1);
	}
	
	ts.tm_mon -= 1;
	ts.tm_year -= 1900;
	ts.tm_isdst = -1;	// let mktime() figure out timezone

	*seconds = MkTime64(&ts);

	return(0);
}

int WRF::EpochToWRFTimeStr(
	TIME64_T seconds,
	string &str
) {

	struct tm ts;
    const char *format = "%Y-%m-%d_%H:%M:%S";
	char buf[128];

	GmTime64_r(&seconds, &ts);

	if (strftime(buf,sizeof(buf), format, &ts) == 0) {
		MyBase::SetErrMsg("strftime(%s) : ???, format");
		return(-1);
	}

	str = buf;

	return(0);
}


int WRF::OpenWrfGetMeta(
	const char * wrfName, // Name of the WRF file (in)
	const atypVarNames_t &atypnames,
	float & dx, // Place to put DX attribute (out)
	float & dy, // Place to put DY attribute (out)
	float * vertExts, // Vertical extents (out)
	size_t dimLens[4], // Lengths of x, y, and z dimensions (out)
	string &startDate, // Place to put START_DATE attribute (out)
	vector<string> & wrfVars3d, 
	vector<string> & wrfVars2d, 
	vector <TIME64_T> &timestamps // Time stamps, in seconds (out)
) {
	int ncid; // Holds netCDF file ID
	int ndims; // Number of dimensions in netCDF
	int ngatts; // Number of global attributes
	int nvars; // Number of variables
	int xdimid; // ID of unlimited dimension (not used)
	
	char dimName[NC_MAX_NAME + 1]; // Temporary holder for dimension names

	static char *buf = NULL;
	static size_t bufSize = 0;
	
	wrfVars3d.clear();
	wrfVars2d.clear();
	timestamps.clear();

	// Open netCDF file and check for failure
	NC_ERR_READ( nc_open( wrfName, NC_NOWRITE, &ncid ));
	// Find the number of dimensions, variables, and global attributes, and check
	// the existance of the unlimited dimension (not that we need to)
	NC_ERR_READ( nc_inq(ncid, &ndims, &nvars, &ngatts, &xdimid ) );

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
		NC_ERR_READ( nc_inq_dimname( ncid, i, dimName ) );
		if ( strcmp( dimName, "west_east" ) == 0 )
		{
			weId = i;
			NC_ERR_READ( nc_inq_dimlen( ncid, i, &dimLens[0] ) );
		} else if ( strcmp( dimName, "south_north" ) == 0 )
		{
			snId = i;
			NC_ERR_READ( nc_inq_dimlen( ncid, i, &dimLens[1] ) );
		} else if ( strcmp( dimName, "bottom_top" ) == 0 )
		{
			btId = i;
			NC_ERR_READ( nc_inq_dimlen( ncid, i, &dimLens[2] ) );
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
			NC_ERR_READ( nc_inq_dimlen( ncid, i, &dimLens[3] ) );
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
	NC_ERR_READ( nc_get_att_float( ncid, NC_GLOBAL, "DX", &dx ) );
	NC_ERR_READ( nc_get_att_float( ncid, NC_GLOBAL, "DY", &dy ) );
	
	// Get starting time stamp
	// We'd prefer SIMULATION_START_DATE, but it's okay if it doesn't exist

	size_t attlen;
	char *start_attr = "SIMULATION_START_DATE";
	int nc_status = nc_inq_attlen(ncid, NC_GLOBAL, start_attr, &attlen);
	if (nc_status == NC_ENOTATT) {
		start_attr = "START_DATE";
		nc_status = nc_inq_attlen(ncid, NC_GLOBAL, start_attr, &attlen);
		if (nc_status == NC_ENOTATT) {
			start_attr = NULL;
			startDate.erase();
		}
	}
	if (start_attr) {
		if (bufSize < attlen+1) {
			if (buf) delete [] buf;
			buf = new char[attlen+1];
			bufSize = attlen+1;
		}

		NC_ERR_READ(nc_get_att_text( ncid, NC_GLOBAL, start_attr, buf ));

		startDate.assign(buf, attlen);
	}
		
    // build list of all dimensions found in the file
    //
	vector <ncdim_t> ncdims;
    for (int dimid = 0; dimid < ndims; dimid++) {
        ncdim_t dim;

        NC_ERR_READ(nc_inq_dim( ncid, dimid, dim.name, &dim.size));
        ncdims.push_back(dim);
    }

	// Find names of variables with appropriate dimmensions
	for ( int i = 0 ; i < nvars ; i++ ) {
		varInfo varinfo;
		char name[NC_MAX_NAME+1];
		NC_ERR_READ( nc_inq_varname(ncid, i, name ) );
		if (GetVarInfo(ncid, name, ncdims, varinfo) < 0) continue;

		if ((varinfo.ndimids == 4) && 
			(varinfo.dimids[0] ==  timeId) &&
			((varinfo.dimids[1] ==  btId) || (varinfo.dimids[1] == btsId)) &&
			((varinfo.dimids[2] ==  snId) || (varinfo.dimids[2] == snsId)) &&
			((varinfo.dimids[3] ==  weId) || (varinfo.dimids[3] == wesId))) {

			wrfVars3d.push_back( name );
		}
		else if ((varinfo.ndimids == 3) && 
			(varinfo.dimids[0] ==  timeId) &&
			((varinfo.dimids[1] ==  snId) || (varinfo.dimids[1] == snsId)) &&
			((varinfo.dimids[2] ==  weId) || (varinfo.dimids[2] == wesId))) {

			wrfVars2d.push_back( name );
		}
	}


	// Get vertical extents if requested
	//
	if (vertExts) {

		// Get ready to read PH and PHB
		varInfo phInfo; // structs for variable information
		varInfo phbInfo;

		if (GetVarInfo( ncid, atypnames.PH.c_str(), ncdims, phInfo) < 0) return(-1);
		if (phInfo.ndimids != 4) {
			MyBase::SetErrMsg("Variable %s has wrong # dims", atypnames.PH.c_str());
			return(-1);
		}
		size_t ph_slice_sz = phInfo.dimlens[phInfo.ndimids-1] * 
			phInfo.dimlens[phInfo.ndimids-2];

		if (GetVarInfo( ncid, atypnames.PHB.c_str(), ncdims, phbInfo) < 0) return(-1);
		if (phbInfo.ndimids != 4) {
			MyBase::SetErrMsg("Variable %s has wrong # dims", atypnames.PHB.c_str());
			return(-1);
		}
		size_t phb_slice_sz = phbInfo.dimlens[phbInfo.ndimids-1] * 
			phbInfo.dimlens[phbInfo.ndimids-2];
			

		// Allocate memory
		float * phBuf = new float[ph_slice_sz];
		float * phBufTemp = new float[ph_slice_sz];
		float * phbBuf = new float[phb_slice_sz];
		float * phbBufTemp = new float[phb_slice_sz];
		

		bool first = true;
		for (size_t t = 0; t<dimLens[3]; t++) {
			float height;
			int rc;

			// Dummies needed by function

			// Read bottom slices
			bool needAnotherPh = true;
			bool needAnotherPhb = true;
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

			needAnotherPh = true;
			needAnotherPhb = true;
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
				if (height < vertExts[1] ) vertExts[1] = height;
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
	if (bufSize < sz) {
		if (buf) delete [] buf;
		buf = new char[sz];
		bufSize = sz;
	}
	nc_status = nc_get_var_text(ncid, timeInfo.varid, buf);
	for (int i =0; i<timeInfo.dimlens[0]; i++) {
		string time_fmt(buf+(i*timeInfo.dimlens[1]), timeInfo.dimlens[1]);
		TIME64_T seconds;

		if (WRFTimeStrToEpoch(time_fmt, &seconds) < 0) return(-1);

		timestamps.push_back(seconds);
	}

	// Close the WRF file
	NC_ERR_READ( nc_close( ncid ) );
	return(0);
}
