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
//      File:		wrf2vdf.cpp
//
//      Author:         Alan Norton
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           May 7, 2007
//
//      Description:	Read a WRF file containing a 3D array of floats or doubles
//			and insert the volume into an existing
//			Vapor Data Collection
//
//		Heavily modified: July 2007, Victor Snyder


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
#include <vapor/MetadataWRF.h>
#include <vapor/WRFReader.h>
#include <vapor/WaveletBlock3DBufWriter.h>
#include <vapor/WaveletBlock3DRegionWriter.h>
#include <vapor/WRF.h>
#ifdef WIN32
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
	int tsincr;
	int numts;
	char * tsstart;
	char * tsend;
	int level;
	OptionParser::Boolean_T	noelev;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"varnames",1, 	"",	"Colon delimited list of variable names in WRF file to convert. The default is to convert variables in the set intersection of those variables found in the VDF file and those in the WRF file."},
	{"tsincr",	1,	"1","Increment between Vapor times steps to convert (e.g., 3=every third), from Vapor time step 0 (NOT CURRENTLY SUPPORTED!!!)"},
	{"numts",	1,	"-1","Maximum number of time steps that may be converted. A -1 implies the conversion of all time steps found"},
	{"tsstart", 1,  "", "Starting time stamp for conversion (default is found in VDF)"},
	{"tsend",	1,  "", "Last time stamp to convert (default is latest time stamp)"},
	{"level",	1, 	"-1","Refinement levels saved. 0=>coarsest, 1=>next refinement, etc. -1=>finest"},
	{"noelev",	0,	"",	"Do not generate the ELEVATION variable required by vaporgui."},
	{"help",	0,	"",	"Print this message and exit."},
	{"debug",	0,	"",	"Enable debugging."},
	{"quiet",	0,	"",	"Operate quietly (outputs only vertical extents that are lower than those in the VDF)."},
	{NULL}
};

OptionParser::Option_T	get_options[] = {
	{"varnames", VetsUtil::CvtToStrVec, &opt.varnames, sizeof(opt.varnames)},
	{"tsincr", VetsUtil::CvtToInt, &opt.tsincr, sizeof(opt.tsincr)},
	{"numts", VetsUtil::CvtToInt, &opt.numts, sizeof(opt.numts)},
	{"tsstart", VetsUtil::CvtToString, &opt.tsstart, sizeof(opt.tsstart)},
	{"tsend", VetsUtil::CvtToString, &opt.tsend, sizeof(opt.tsend)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"noelev", VetsUtil::CvtToBoolean, &opt.noelev, sizeof(opt.noelev)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};

const char	*ProgName;

//
// Backup a .vdf file
//
void save_file(const char *file) {
	FILE	*ifp, *ofp;
	int	c;

	string oldfile(file);
	oldfile.append(".old");

	ifp = fopen(file, "rb");
	if (! ifp) {
		cerr << ProgName << ": Could not open file \"" << 
			file << "\" : " <<strerror(errno) << endl;
		exit(1);
	}

	ofp = fopen(oldfile.c_str(), "wb");
	if (! ifp) {
		cerr << ProgName << ": Could not open file \"" << 
			oldfile << "\" : " <<strerror(errno) << endl;
		exit(1);
	}

	do {
		c = fgetc(ifp);
		if (c != EOF) c = fputc(c,ofp); 

	} while (c != EOF);

	if (ferror(ifp)) {
		cerr << ProgName << ": Error reading file \"" << 
			file << "\" : " <<strerror(errno) << endl;
		exit(1);
	}

	if (ferror(ofp)) {
		cerr << ProgName << ": Error writing file \"" << 
			oldfile << "\" : " <<strerror(errno) << endl;
		exit(1);
	}
}

// Takes the start through the (start+lenth-1) elements of fullTime and puts
// them into pieceTime, with a null character on the end.  N.B., pieceTime must
// be of size at least length+1.
// Opens a WRF netCDF file, gets the file's ID, the number of dimensions,

int OpenWrfFile(
	const char * netCDFfile, // Path of WRF output file to be opened (input)
	int & ncid // ID of netCDF file (output)
) {
	
	int nc_status = nc_open(netCDFfile, NC_NOWRITE, &ncid );
	NC_ERR_READ( nc_status );

	return(0);
}

// Calculates needed and desired quantities involving PH and 
// PHB--elevation (needed for
// on-the-fly interpolation in Vapor) and normalized geopotential,
// and the original variables.

int DoGeopotStuff(
	WRFReader * WRFRead,
	const size_t dim[3],
	const map<string, WRF::varInfo_t *> &var_info_map, 
	size_t aVaporTs, 
	int wrfT, 
	MetadataVDC &metadata, 
	const WRF::atypVarNames_t &wrfNames,
	vector<string> &varnames, 
	double extents[2],
	double grav
) {
	int rc = 0;
	bool wantEle = find(varnames.begin(),varnames.end(),"ELEVATION")!=varnames.end();
	bool wantPh = find(varnames.begin(),varnames.end(),wrfNames.PH)!=varnames.end();
	bool wantPhb = find(varnames.begin(),varnames.end(),wrfNames.PHB)!=varnames.end();
	bool wantPhnorm = find(varnames.begin(),varnames.end(),"PHNorm_")!=varnames.end();

	if (! (wantEle || wantPh || wantPhb || wantPhnorm)) return(0);

	map<string, WRF::varInfo_t *>::const_iterator itr;

	itr = var_info_map.find(wrfNames.PH);
	WRF::varInfo_t  *phInfoPtr = itr==var_info_map.end() ? NULL : itr->second; 

	itr = var_info_map.find(wrfNames.PHB);
	WRF::varInfo_t  *phbInfoPtr = itr==var_info_map.end() ? NULL : itr->second; 

	assert(phInfoPtr != NULL);
	assert(phbInfoPtr != NULL);
	
	// Allocate static storage array

	static float * phBuffer = NULL; // PH
	static float * phbBuffer = NULL; // Holds a slice of PHB
	static float * workBuffer = NULL; // ELEVATION and PHNorm_
	size_t slice_sz;

	slice_sz = dim[0] * dim[1];
	if (! phBuffer && phInfoPtr) {
		if (! phBuffer) phBuffer = new float[slice_sz];

	}
	if (! phbBuffer && phbInfoPtr) {
		if (! phbBuffer) phbBuffer = new float[slice_sz];
	}
	if (! workBuffer ) {
		if (! workBuffer) workBuffer = new float[slice_sz];
	}

	// Prepare wavelet block writers
	
	WaveletBlock3DBufWriter * eleWriter = NULL;
	WaveletBlock3DBufWriter * phWriter = NULL;
	WaveletBlock3DBufWriter * phbWriter = NULL;
	WaveletBlock3DBufWriter * phnormWriter = NULL;

	if ( wantEle ) {
		eleWriter = new WaveletBlock3DBufWriter(metadata);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		eleWriter->OpenVariableWrite(aVaporTs, "ELEVATION", opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), "ELEVATION"));
	}

	if ( wantPh ) {
		phWriter = new WaveletBlock3DBufWriter(metadata);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		phWriter->OpenVariableWrite(aVaporTs, wrfNames.PH.c_str(), opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), wrfNames.PH));
	}

	if ( wantPhb ) {
		phbWriter = new WaveletBlock3DBufWriter(metadata);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		phbWriter->OpenVariableWrite(aVaporTs, wrfNames.PHB.c_str(), opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), wrfNames.PHB));
	}

	if ( wantPhnorm ) {
		phnormWriter = new WaveletBlock3DBufWriter(metadata);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		phnormWriter->OpenVariableWrite(aVaporTs, "PHNorm_", opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), "PHNorm_"));
	}

	// Loop over z slices

	for ( size_t z = 0 ; z < dim[2] ; z++ ) {

		// Read needed slices

		if (WRFRead->OpenVariableRead(aVaporTs, wrfNames.PH.c_str()) != 0 ) {
			WRF::SetErrMsg("Error opening variable \"%s\"", wrfNames.PH.c_str());
			WRF::SetErrCode(0);
			break;
		}

		if (WRFRead->ReadSlice(z, phBuffer) != 0 ) {
			WRF::SetErrMsg("Error reading variable \"%s\"", wrfNames.PH.c_str());
			WRF::SetErrCode(0);
			break;
		}

		if (WRFRead->CloseVariable() != 0 ) {
			WRF::SetErrMsg("Error closing variable \"%s\"", wrfNames.PH.c_str());
			WRF::SetErrCode(0);
			break;
		}

		if (WRFRead->OpenVariableRead(aVaporTs, wrfNames.PHB.c_str()) != 0 ) {
			WRF::SetErrMsg("Error opening variable \"%s\"", wrfNames.PHB.c_str());
			WRF::SetErrCode(0);
			break;
		}

		if (WRFRead->ReadSlice(z, phbBuffer) != 0 ) {
			WRF::SetErrMsg("Error reading variable \"%s\"", wrfNames.PHB.c_str());
			WRF::SetErrCode(0);
			break;
		}

		if (WRFRead->CloseVariable() != 0 ) {
			WRF::SetErrMsg("Error closing variable \"%s\"", wrfNames.PHB.c_str());
			WRF::SetErrCode(0);
			break;
		}
	
		// Write PH and/or PHB, if desired
		if ( wantPh ) phWriter->WriteSlice( phBuffer );
		if ( wantPhb ) phbWriter->WriteSlice( phbBuffer );

		// Write geopotential height, with name ELEVATION.  This is required by Vapor
		// to handle leveled data.
		for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ ) {
			workBuffer[i] = (phBuffer[i] + phbBuffer[i])/grav;
		}

		// Want to find the bottom of the bottom layer and the bottom of the
		// top layer so that we can output them
		if (z == 0) {
			extents[0] = workBuffer[0]; 
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ ) {
				if (workBuffer[i] < extents[0] ) {
					// Bottom of bottom for this time step
					extents[0] = workBuffer[i]; 
				}
			}
		}
		if ( z == dim[2] - 1 ) {
			extents[1] = workBuffer[0]; 
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ ) {
				if ( workBuffer[i] < extents[1] ) {
					// Bottom of top for this time step
					extents[1] = workBuffer[i]; 
				}
			}
		}

		if ( wantEle ) eleWriter->WriteSlice( workBuffer );

				
		// Find and write normalized geopotential, if desired
		if ( wantPhnorm )
		{
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				workBuffer[i] *= grav/phbBuffer[i];
			phnormWriter->WriteSlice( workBuffer );
		}

	}

	if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);

	// Close the variables. We're done writing.
	if (eleWriter) {
		eleWriter->CloseVariable();
		delete eleWriter;
	}

	if (phWriter) {
		phWriter->CloseVariable();
		delete phWriter;
	}

	if (phbWriter) {
		phbWriter->CloseVariable();
		delete phbWriter;
	}

	if (phnormWriter) {
		phnormWriter->CloseVariable();
		delete phnormWriter;
	}

	return(rc);
}

// Reads/calculates and outputs any of these quantities: U, V, W,
// 2D wind speed (U-V plane), 3D wind speed, and vertical vorticity (relative to
// a WRF level, i.e., not vertically interpolated to a Cartesian grid)
int DoWindStuff(
	WRFReader * WRFRead,
	const size_t dim[3],
	double dx, // Grid spacings from WRF file
	double dy,
	const map<string, WRF::varInfo_t *> &var_info_map, 
	size_t aVaporTs, 
	int wrfT, 
	MetadataVDC &metadata,
	const WRF::atypVarNames_t &wrfNames,
	vector<string> &varnames
) {
	int rc = 0;
	bool wantU = find(varnames.begin(),varnames.end(),wrfNames.U)!=varnames.end();
	bool wantV = find(varnames.begin(),varnames.end(),wrfNames.V)!=varnames.end();
	bool wantW = find(varnames.begin(),varnames.end(),wrfNames.W)!=varnames.end();
	bool wantUV = find(varnames.begin(),varnames.end(),"UV_")!=varnames.end();
	bool wantUVW = find(varnames.begin(),varnames.end(),"UVW_")!=varnames.end();
	bool wantOmZ = find(varnames.begin(),varnames.end(),"omZ_")!=varnames.end();

	// Make sure we have work to do.

	if (! (wantU || wantV || wantW || wantUV || wantUVW || wantOmZ)) return(0);

	map<string, WRF::varInfo_t *>::const_iterator itr;

	// Allocate static storage array
	static float * uBuffer = NULL; 
	static float * vBuffer = NULL;
	static float * wBuffer = NULL;
	static float * uvBuffer = NULL;
	static float * uvwBuffer = NULL;
	static float * omZBuffer = NULL;
	size_t slice_sz = dim[0] * dim[1];

	if (wantU || wantUV || wantUVW || wantOmZ) {
		if (! uBuffer) uBuffer = new float[slice_sz];
	}
	if (wantV || wantUV || wantUVW || wantOmZ) {
		if (! vBuffer) vBuffer = new float[slice_sz];
	}
	if (wantW || wantUVW) {
		if (! wBuffer) wBuffer = new float[slice_sz];
	}
	if ( wantUV ) {
		if (! uvBuffer) uvBuffer = new float[slice_sz];
	}
	if ( wantUVW ) {
		if (! uvwBuffer) uvwBuffer = new float[slice_sz];
	}
	if ( wantOmZ ) {
		if (! omZBuffer) omZBuffer = new float[slice_sz];
	}

	// Prepare wavelet block writers

	WaveletBlock3DBufWriter * uWriter = NULL;
	WaveletBlock3DBufWriter * vWriter = NULL;
	WaveletBlock3DBufWriter * wWriter = NULL;
	WaveletBlock3DBufWriter * uvWriter = NULL;
	WaveletBlock3DBufWriter * uvwWriter = NULL;
	WaveletBlock3DBufWriter * omZWriter = NULL;

	if ( wantU ) {
		uWriter = new WaveletBlock3DBufWriter(metadata);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		uWriter->OpenVariableWrite(aVaporTs, wrfNames.U.c_str(), opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), wrfNames.U));
	}
	if ( wantV ) {
		vWriter = new WaveletBlock3DBufWriter(metadata);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		vWriter->OpenVariableWrite(aVaporTs, wrfNames.V.c_str(), opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), wrfNames.V));
	}
	if ( wantW ) {
		wWriter = new WaveletBlock3DBufWriter(metadata);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		wWriter->OpenVariableWrite(aVaporTs, wrfNames.W.c_str(), opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), wrfNames.W));
	}
	if ( wantUV ) {
		uvWriter = new WaveletBlock3DBufWriter(metadata);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		uvWriter->OpenVariableWrite(aVaporTs, "UV_", opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), "UV_"));
	}
	if ( wantUVW ) {
		uvwWriter = new WaveletBlock3DBufWriter(metadata);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		uvwWriter->OpenVariableWrite(aVaporTs, "UVW_", opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), "UVW_"));
	}
	if ( wantOmZ ) {
		omZWriter = new WaveletBlock3DBufWriter(metadata);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		omZWriter->OpenVariableWrite(aVaporTs, "omZ_", opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), "omZ_"));
	}

	// Read, calculate, and write the data

	for ( size_t z = 0 ; z < dim[2] ; z++ )
	{
		// Read (if desired or needed)
		if ( wantU || wantUV || wantUVW || wantOmZ ) {
			if (WRFRead->OpenVariableRead(aVaporTs, wrfNames.U.c_str()) != 0 ) {
				WRF::SetErrMsg("Error opening variable \"%s\"", wrfNames.U.c_str());
				WRF::SetErrCode(0);
				break;
			}

			if (WRFRead->ReadSlice(z, uBuffer) != 0 ) {
				WRF::SetErrMsg("Error reading variable \"%s\"", wrfNames.U.c_str());
				WRF::SetErrCode(0);
				break;
			}

			if (WRFRead->CloseVariable() != 0 ) {
				WRF::SetErrMsg("Error closing variable \"%s\"", wrfNames.U.c_str());
				WRF::SetErrCode(0);
				break;
			}
		}

		if ( wantV || wantUV || wantUVW || wantOmZ ) {
			if (WRFRead->OpenVariableRead(aVaporTs, wrfNames.V.c_str()) != 0 ) {
				WRF::SetErrMsg("Error opening variable \"%s\"", wrfNames.V.c_str());
				WRF::SetErrCode(0);
				break;
			}

			if (WRFRead->ReadSlice(z, vBuffer) != 0 ) {
				WRF::SetErrMsg("Error reading variable \"%s\"", wrfNames.V.c_str());
				WRF::SetErrCode(0);
				break;
			}

			if (WRFRead->CloseVariable() != 0 ) {
				WRF::SetErrMsg("Error closing variable \"%s\"", wrfNames.V.c_str());
				WRF::SetErrCode(0);
				break;
			}
		}

		if ( wantW || wantUVW ) {
			if (WRFRead->OpenVariableRead(aVaporTs, wrfNames.W.c_str()) != 0 ) {
				WRF::SetErrMsg("Error opening variable \"%s\"", wrfNames.W.c_str());
				WRF::SetErrCode(0);
				break;
			}

			if (WRFRead->ReadSlice(z, wBuffer) != 0 ) {
				WRF::SetErrMsg("Error reading variable \"%s\"", wrfNames.W.c_str());
				WRF::SetErrCode(0);
				break;
			}

			if (WRFRead->CloseVariable() != 0 ) {
				WRF::SetErrMsg("Error closing variable \"%s\"", wrfNames.W.c_str());
				WRF::SetErrCode(0);
				break;
			}
		}

		// Write U, V, W (if desired)
		if ( wantU ) uWriter->WriteSlice( uBuffer );
		if ( wantV ) vWriter->WriteSlice( vBuffer );
		if ( wantW ) wWriter->WriteSlice( wBuffer );

		// Calculate and write 2D wind (if desired or needed)
		if ( wantUV ) {
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				uvBuffer[i] = sqrt( uBuffer[i]*uBuffer[i] + vBuffer[i]*vBuffer[i] );
			uvWriter->WriteSlice( uvBuffer );
		}
		// Calculate and write 3D wind (if desired)
		if ( wantUVW ) {
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				uvwBuffer[i] = sqrt( uBuffer[i]*uBuffer[i] + vBuffer[i]*vBuffer[i] + wBuffer[i]*wBuffer[i] );
			uvwWriter->WriteSlice( uvwBuffer );
		}
		// Calculate and write vertical vorticity (if desired)
		if ( wantOmZ ) {
			double dVdx, dUdy; // Holds derivatives used in calculation of curl
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
			{
				// On the boundaries of the domain, use forward or backward
				// finite differences
				if ( i % dim[0] == 0 ) // On left edge of domain
					dVdx = (vBuffer[i + 1] - vBuffer[i])/dx;
				else if ( (i + 1) % dim[0] == 0 ) // On right edge
					dVdx = (vBuffer[i] - vBuffer[i - 1])/dx;
				else // In the middle--use centered difference
					dVdx = (vBuffer[i + 1] - vBuffer[i - 1])/(2.0*dx);
				if ( i >= 0 && i < dim[0] ) // On the bottom edge
					dUdy = (uBuffer[i + dim[0]] - uBuffer[i])/dy;
				else if ( i >= dim[0]*(dim[1] - 1) ) // On top edge
					dUdy = (uBuffer[i] - uBuffer[i - dim[0]])/dy;
				else // In the middle
					dUdy = (uBuffer[i + dim[0]] - uBuffer[i - dim[0]])/(2.0*dy);
				// Calculate the vertical vorticity at that grid point
				omZBuffer[i] = dVdx - dUdy;
			}
			omZWriter->WriteSlice( omZBuffer );
		}
	}

	if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);

	// Close the variables. We're done writing.
	if (uWriter) {
		uWriter->CloseVariable();
		delete uWriter;
	}
	if (vWriter) {
		vWriter->CloseVariable();
		delete vWriter;
	}
	if (wWriter) {
		wWriter->CloseVariable();
		delete wWriter;
	}
	if (uvWriter) {
		uvWriter->CloseVariable();
		delete uvWriter;
	}
	if (uvwWriter) {
		uvwWriter->CloseVariable();
		delete uvwWriter;
	}
	if (omZWriter) {
		omZWriter->CloseVariable();
		delete omZWriter;
	}

	return(rc);
}

// Read/calculates and outputs some or all of P, PB, PFull_, PNorm_,
// T, Theta_, and TK_.  Sets flags so that variables are not read twice.
int DoPTStuff(
	WRFReader * WRFRead,
	const size_t dim[3],
	const map<string, WRF::varInfo_t *> &var_info_map, 
	size_t aVaporTs, 
	int wrfT, 
	MetadataVDC &metadata,
	const WRF::atypVarNames_t &wrfNames,
	vector<string> &varnames
) {
	bool wantP = find(varnames.begin(),varnames.end(),wrfNames.P)!=varnames.end();
	bool wantPb = find(varnames.begin(),varnames.end(),wrfNames.PB)!=varnames.end();
	bool wantT = find(varnames.begin(),varnames.end(),wrfNames.T)!=varnames.end();
	bool wantPfull = find(varnames.begin(),varnames.end(),"PFull_")!=varnames.end();
	bool wantPnorm = find(varnames.begin(),varnames.end(),"PNorm_")!=varnames.end();
	bool wantTheta = find(varnames.begin(),varnames.end(),"Theta_")!=varnames.end();
	bool wantTk = find(varnames.begin(),varnames.end(),"TK_")!=varnames.end();

	size_t slice_sz = dim[0] * dim[1];

	// Make sure we have work to do.

	if (! (wantP || wantPb || wantT || wantPfull || wantPnorm || wantTheta || wantTk)) return(0);

	// Allocate static storage array
	static float * pBuffer = NULL; 
	static float * pbBuffer = NULL;
	static float * tBuffer = NULL;
	static float * workBuffer = NULL;

	if ( wantP || wantPfull || wantPnorm || wantTk ) {
		if (! pBuffer) pBuffer = new float[slice_sz];
	}
	if ( wantPb || wantPfull || wantPnorm || wantTk ) {
		if (! pbBuffer) pbBuffer = new float[slice_sz];
	}
	if ( wantT || wantTheta || wantTk ) {
		if (! tBuffer) tBuffer = new float[slice_sz];
	}
	if ( wantPfull || wantPnorm || wantTheta || wantTk ) {
		if (! workBuffer) workBuffer = new float[dim[0]*dim[1]];
	}

	// Prepare wavelet block writers
	//
	WaveletBlock3DBufWriter * pWriter = NULL;
	WaveletBlock3DBufWriter * pbWriter = NULL;
	WaveletBlock3DBufWriter * tWriter = NULL;
	WaveletBlock3DBufWriter * pfullWriter = NULL;
	WaveletBlock3DBufWriter * pnormWriter = NULL;
	WaveletBlock3DBufWriter * thetaWriter = NULL;
	WaveletBlock3DBufWriter * tkWriter = NULL;

	if ( wantP ) {
		pWriter = new WaveletBlock3DBufWriter(metadata);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		pWriter->OpenVariableWrite(aVaporTs, wrfNames.P.c_str(), opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), wrfNames.P));
	}
	if ( wantPb ) {
		pbWriter = new WaveletBlock3DBufWriter(metadata);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		pbWriter->OpenVariableWrite(aVaporTs, wrfNames.PB.c_str(), opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), wrfNames.PB));
	}
	if ( wantT ) {
		tWriter = new WaveletBlock3DBufWriter(metadata);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		tWriter->OpenVariableWrite(aVaporTs, wrfNames.T.c_str(), opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), wrfNames.T));
	}
	if ( wantPfull ) {
		pfullWriter = new WaveletBlock3DBufWriter(metadata);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		pfullWriter->OpenVariableWrite(aVaporTs, "PFull_", opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), "PFull_"));
	}
	if ( wantPnorm ) {
		pnormWriter = new WaveletBlock3DBufWriter(metadata);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		pnormWriter->OpenVariableWrite(aVaporTs, "PNorm_", opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), "PNorm_"));
	}
	if ( wantTheta ) {
		thetaWriter = new WaveletBlock3DBufWriter(metadata);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		thetaWriter->OpenVariableWrite(aVaporTs, "Theta_", opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), "Theta_"));
	}
	if ( wantTk ) {
		tkWriter = new WaveletBlock3DBufWriter(metadata);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		tkWriter->OpenVariableWrite(aVaporTs, "TK_", opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), "TK_"));
	}

	// Read, calculate, and write
	int rc = 0;
	for ( size_t z = 0 ; z < dim[2] ; z++ )
	{
		// Read
		if ( wantP || wantPfull || wantPnorm || wantTk ) {
			if (WRFRead->OpenVariableRead(aVaporTs, wrfNames.P.c_str()) != 0 ) {
				WRF::SetErrMsg("Error opening variable \"%s\"", wrfNames.P.c_str());
				WRF::SetErrCode(0);
				break;
			}

			if (WRFRead->ReadSlice(z, pBuffer) != 0 ) {
				WRF::SetErrMsg("Error reading variable \"%s\"", wrfNames.P.c_str());
				WRF::SetErrCode(0);
				break;
			}

			if (WRFRead->CloseVariable() != 0 ) {
				WRF::SetErrMsg("Error closing variable \"%s\"", wrfNames.P.c_str());
				WRF::SetErrCode(0);
				break;
			}
		}
		if ( wantPb || wantPfull || wantPnorm || wantTk ) {
			if (WRFRead->OpenVariableRead(aVaporTs, wrfNames.PB.c_str()) != 0 ) {
				WRF::SetErrMsg("Error opening variable \"%s\"", wrfNames.PB.c_str());
				WRF::SetErrCode(0);
				break;
			}

			if (WRFRead->ReadSlice(z, pbBuffer) != 0 ) {
				WRF::SetErrMsg("Error reading variable \"%s\"", wrfNames.PB.c_str());
				WRF::SetErrCode(0);
				break;
			}

			if (WRFRead->CloseVariable() != 0 ) {
				WRF::SetErrMsg("Error closing variable \"%s\"", wrfNames.PB.c_str());
				WRF::SetErrCode(0);
				break;
			}
		}
		if ( wantT || wantTheta || wantTk ) {
			if (WRFRead->OpenVariableRead(aVaporTs, wrfNames.T.c_str()) != 0 ) {
				WRF::SetErrMsg("Error opening variable \"%s\"", wrfNames.T.c_str());
				WRF::SetErrCode(0);
				break;
			}

			if (WRFRead->ReadSlice(z, tBuffer) != 0 ) {
				WRF::SetErrMsg("Error reading variable \"%s\"", wrfNames.T.c_str());
				WRF::SetErrCode(0);
				break;
			}

			if (WRFRead->CloseVariable() != 0 ) {
				WRF::SetErrMsg("Error closing variable \"%s\"", wrfNames.T.c_str());
				WRF::SetErrCode(0);
				break;
			}
		}

		// Write out any desired WRF variables
		if ( wantP ) pWriter->WriteSlice( pBuffer );
		if ( wantPb ) pbWriter->WriteSlice( pbBuffer );
		if ( wantT ) tWriter->WriteSlice( tBuffer );

		// Find and write Theta (if desired)
		if ( wantTheta ) {
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				workBuffer[i] = tBuffer[i] + 300.0;
			thetaWriter->WriteSlice( workBuffer );
		}

		// Find and write PNorm_ (if desired)
		if ( wantPnorm ) {
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				workBuffer[i] = pBuffer[i]/pbBuffer[i] + 1.0;
			pnormWriter->WriteSlice( workBuffer );
		}
		// Find and write PFull_ (if desired)
		if ( wantPfull && wantPnorm) {
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				workBuffer[i] *= pbBuffer[i];
			pfullWriter->WriteSlice( workBuffer );
		}
		if ( wantPfull && !wantPnorm ) {
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				workBuffer[i] = pBuffer[i] + pbBuffer[i];
			pfullWriter->WriteSlice( workBuffer );
		}
		// Find and write TK_ (if desired).  Forget optimizations.
		if ( wantTk ) {	
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				workBuffer[i] = (tBuffer[i] + 300.0)
								* pow( (float)(pBuffer[i] + pbBuffer[i]), 0.286f )
								/ pow( 100000.0f, 0.286f );
			tkWriter->WriteSlice( workBuffer );
		}
	}

	if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);

	// Close the variables. We're done writing.
	if (pWriter) {
		pWriter->CloseVariable();
		delete pWriter;
	}
	if (pbWriter) {
		pbWriter->CloseVariable();
		delete pbWriter;
	}
	if (tWriter) {
		tWriter->CloseVariable();
		delete tWriter;
	}
	if (pfullWriter) {
		pfullWriter->CloseVariable();
		delete pfullWriter;
	}
	if (pnormWriter) {
		pnormWriter->CloseVariable();
		delete pnormWriter;
	}
	if (thetaWriter) {
		thetaWriter->CloseVariable();
		delete thetaWriter;
	}
	if (tkWriter) {
		tkWriter->CloseVariable();
		delete tkWriter;
	}

	return(rc);
}

int DoIndependentVars3d(
	WRFReader * WRFRead,
	const size_t dim[3],
	const map<string, WRF::varInfo_t *> &var_info_map, 
	size_t aVaporTs, 
	int wrfT, 
	MetadataVDC &metadata,
	const WRF::atypVarNames_t &wrfNames,
	vector<string> &varnames

) {
	static float * varBuffer = NULL; 
	WaveletBlock3DBufWriter * varWriter = NULL;

	varWriter = new WaveletBlock3DBufWriter(metadata);
	if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);

	if (! varBuffer) {
		// allocate buffer for a slice. Leave space for horizontally staggered
		// vars.
		//
		varBuffer = new float[(dim[0]+1)*(dim[1]+1)]; 
	}

	int rc = 0;

	vector <string> vn_copy = varnames;
	for (int i=0; i<vn_copy.size(); i++) {

		VDFIOBase::VarType_T vtype = metadata.GetVarType(vn_copy[i]);

		if (vtype != VDFIOBase::VAR3D) continue;

		map<string, WRF::varInfo_t *>::const_iterator itr;

		itr = var_info_map.find(vn_copy[i]);
		WRF::varInfo_t  *varInfoPtr = itr==var_info_map.end() ? NULL : itr->second; 

		assert(varInfoPtr != NULL);

		varWriter->OpenVariableWrite(aVaporTs, vn_copy[i].c_str(), opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), vn_copy[i]));

		// Switches to prevent redundant reads in the case of 
		// vertical staggering

		// Read and write
		for ( size_t z = 0 ; z < dim[2] ; z++ ) {
			// Read
			if (WRFRead->OpenVariableRead(aVaporTs, vn_copy[i].c_str()) != 0 ) {
				WRF::SetErrMsg("Error opening variable \"%s\"", vn_copy[i].c_str());
				WRF::SetErrCode(0);
				break;
			}

			if (WRFRead->ReadSlice(z, varBuffer) != 0 ) {
				WRF::SetErrMsg("Error reading variable \"%s\"", vn_copy[i].c_str());
				WRF::SetErrCode(0);
				break;
			}

			if (WRFRead->CloseVariable() != 0 ) {
				WRF::SetErrMsg("Error closing variable \"%s\"", vn_copy[i].c_str());
				WRF::SetErrCode(0);
				break;
			}

			varWriter->WriteSlice( varBuffer );

		}

		varWriter->CloseVariable();
	}

	if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);

	// Close the variables. We're done writing.
	if (varWriter) {
		delete varWriter;
	}

	return(rc);
}

int DoIndependentVars2d(
	WRFReader * WRFRead,
	const size_t dim[3],
	const map<string, WRF::varInfo_t *> &var_info_map, 
	size_t aVaporTs, 
	int wrfT, 
	MetadataVDC &metadata,
	const WRF::atypVarNames_t &wrfNames,
	vector<string> &varnames

) {
	static float * varBuffer = NULL; 
	WaveletBlock3DRegionWriter * varWriter = NULL;

	varWriter = new WaveletBlock3DRegionWriter(metadata);
	if (WaveletBlock3DRegionWriter::GetErrCode() != 0) return(-1);

	// Allocate buffer big enough for staggered variables
	
	if (! varBuffer) varBuffer = new float[(dim[0]+1)*(dim[1]+1)]; 

	int rc = 0;

	vector <string> vn_copy = varnames;
	for (int i=0; i<vn_copy.size(); i++) {

		VDFIOBase::VarType_T vtype = metadata.GetVarType(vn_copy[i]);

		if (vtype != VDFIOBase::VAR2D_XY) continue;

		map<string, WRF::varInfo_t *>::const_iterator itr;

		itr = var_info_map.find(vn_copy[i]);
		WRF::varInfo_t  *varInfoPtr = itr==var_info_map.end() ? NULL : itr->second; 

		assert(varInfoPtr != NULL);

		varWriter->OpenVariableWrite(aVaporTs, vn_copy[i].c_str(), opt.level);
		if (WaveletBlock3DRegionWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), vn_copy[i]));

		if (WRFRead->OpenVariableRead(aVaporTs, vn_copy[i].c_str()) != 0 ) {
			WRF::SetErrMsg("Error opening variable \"%s\"", vn_copy[i].c_str());
			WRF::SetErrCode(0);
			break;
		}

		if (WRFRead->ReadSlice(0, varBuffer) != 0 ) {
			WRF::SetErrMsg("Error reading variable \"%s\"", vn_copy[i].c_str());
			WRF::SetErrCode(0);
			break;
		}

		if (WRFRead->CloseVariable() != 0 ) {
			WRF::SetErrMsg("Error closing variable \"%s\"", vn_copy[i].c_str());
			WRF::SetErrCode(0);
			break;
		}

		varWriter->WriteRegion( varBuffer );

		varWriter->CloseVariable();
	}

	if (WaveletBlock3DRegionWriter::GetErrCode() != 0) return(-1);

	if (varWriter) delete varWriter;

	return(rc);
}

int GetVDFInfo(
	const char *metafile,
	vector <string> &vars,
	vector <TIME64_T>	&timestamps,
	WRF::atypVarNames_t  &wrfNames,
	size_t dims[3],
	vector <double> &extents
) {

	MetadataVDC *metadata;

	vars.clear();
	timestamps.clear();

	metadata = new MetadataVDC(metafile);

	if (MetadataVDC::GetErrCode() != 0) {
		return(-1);
    }

	vars = metadata->GetVariableNames();

	long numts = metadata->GetNumTimeSteps();
	if (numts < 0) return(-1);

	for (size_t t = 0; t<numts; t++) {
		TIME64_T t0;
		t0 = metadata->GetTSUserTime(t);
		timestamps.push_back(t0);
	}

	const size_t *dptr = metadata->GetDimension();
	for (int i=0; i<3; i++) dims[i] = dptr[i];

	extents = metadata->GetExtents();

	string tag("DependentVarNames");
	string s = metadata->GetUserDataString(tag);
	vector <string> svec;
	CvtToStrVec(s.c_str(), &svec);
	if (svec.size() != 8) {
		MyBase::SetErrMsg("Failed to get Dependent Variable Names");
		return(-1);
	}
		
	// Default values
	wrfNames.U = svec[0];
	wrfNames.V = svec[1];
	wrfNames.W = svec[2];
	wrfNames.PH = svec[3];
	wrfNames.PHB = svec[4];
	wrfNames.P = svec[5];
	wrfNames.PB = svec[6];
	wrfNames.T = svec[7];

	delete metadata;

	return(0);
}

// Create a list of variables to be translated based on what the user
// wants (via command line option, 'opt.varnames', if not empty, else
// via contents of 'vdf_vars'), and what is available in the 
// WRF file (wrf_vars). The list of variables to translate is
// returned in 'copy_vars'

void SelectVariables(
	const vector <string> &vdf_vars, 
	const vector <string> &wrf_vars, 
	const vector <string> &opt_varnames, 
	const WRF::atypVarNames_t &wrfNames,
	vector <string> &copy_vars,
	bool noelev
) {
	copy_vars.clear();

	vector <string> candidates; // Candidate variables for selection

	if (opt_varnames.size()) candidates = opt_varnames;
	else candidates = vdf_vars;

	// Always include "ELEVATION" unless requested no to
	//
	if (! noelev) {
		if (find(candidates.begin(),candidates.end(),"ELEVATION")==candidates.end()){
			candidates.push_back("ELEVATION");
		}
	}

	for (int i=0; i<candidates.size(); i++) {
		string v = candidates[i];

		// If candidate variable is a derived quantity (e.g. ELEVATION)
		// we need to make sure its dependencies are available in the
		// WRF file
		
		if (v.compare("ELEVATION")==0) {

			// Check for existence of dependency variables and make
			// sure variable isn't already on the copy list.
			//
			if (
				(find(wrf_vars.begin(),wrf_vars.end(),wrfNames.PH)!=wrf_vars.end()) &&
				(find(wrf_vars.begin(),wrf_vars.end(),wrfNames.PHB) !=wrf_vars.end())&&
				(find(copy_vars.begin(),copy_vars.end(),v)==copy_vars.end())
			) {
				copy_vars.push_back(v);
			}
		}
		else if (v.compare("PHNorm_")==0) {
			if (
				(find(wrf_vars.begin(),wrf_vars.end(),wrfNames.PHB)!=wrf_vars.end()) &&
				(find(copy_vars.begin(),copy_vars.end(),v)==copy_vars.end())
			) {
				copy_vars.push_back(v);
			}
		}
		else if ((v.compare("PFull_")==0) || (v.compare("PNorm_")==0)) {
			if (
				(find(wrf_vars.begin(),wrf_vars.end(),wrfNames.P)!=wrf_vars.end()) &&
				(find(wrf_vars.begin(),wrf_vars.end(),wrfNames.PB)!=wrf_vars.end()) &&
				(find(copy_vars.begin(),copy_vars.end(),v)==copy_vars.end())
			) {
				copy_vars.push_back(v);
			}
		}
		else if (v.compare("Theta_")==0) {
			if (
				(find(wrf_vars.begin(),wrf_vars.end(),wrfNames.T)!=wrf_vars.end()) &&
				(find(copy_vars.begin(),copy_vars.end(),v)==copy_vars.end())
			) {
				copy_vars.push_back(v);
			}
		}
		else if (v.compare("TK_")==0) {
			if (
				(find(wrf_vars.begin(),wrf_vars.end(),wrfNames.P)!=wrf_vars.end()) &&
				(find(wrf_vars.begin(),wrf_vars.end(),wrfNames.PB)!=wrf_vars.end()) &&
				(find(wrf_vars.begin(),wrf_vars.end(),wrfNames.T)!=wrf_vars.end()) &&
				(find(copy_vars.begin(),copy_vars.end(),v)==copy_vars.end())
			) {
				copy_vars.push_back(v);
			}
		}
		else if (v.compare("UV_")==0) {
			if (
				(find(wrf_vars.begin(),wrf_vars.end(),wrfNames.U)!=wrf_vars.end()) &&
				(find(wrf_vars.begin(),wrf_vars.end(),wrfNames.V)!=wrf_vars.end()) &&
				(find(copy_vars.begin(),copy_vars.end(),v)==copy_vars.end())
			) {
				copy_vars.push_back(v);
			}
		}
		else if (v.compare("UVW_")==0) {
			if (
				(find(wrf_vars.begin(),wrf_vars.end(),wrfNames.U)!=wrf_vars.end()) &&
				(find(wrf_vars.begin(),wrf_vars.end(),wrfNames.V)!=wrf_vars.end()) &&
				(find(wrf_vars.begin(),wrf_vars.end(),wrfNames.W)!=wrf_vars.end()) &&
				(find(copy_vars.begin(),copy_vars.end(),v)==copy_vars.end())
			) {
				copy_vars.push_back(v);
			}
		}
		else if (v.compare("omZ_")==0) {
			if (
				(find(wrf_vars.begin(),wrf_vars.end(),wrfNames.U)!=wrf_vars.end()) &&
				(find(wrf_vars.begin(),wrf_vars.end(),wrfNames.V)!=wrf_vars.end()) &&
				(find(copy_vars.begin(),copy_vars.end(),v)==copy_vars.end())
			) {
				copy_vars.push_back(v);
			}
		}
		else {
			// Variable is not a derived quantity
			//
			if ((find(wrf_vars.begin(),wrf_vars.end(),v)!=wrf_vars.end()) &&
				(find(copy_vars.begin(),copy_vars.end(),v)==copy_vars.end())
			) {
				copy_vars.push_back(v);
			}
		}
	}

}

int get_dep_var_info(
	int ncid, 
	const vector <WRF::ncdim_t> &ncdims,
	const string &vname,
	map <string, WRF::varInfo_t *> &var_info_map
) {
	if (var_info_map.find(vname) == var_info_map.end()) {
		var_info_map[vname] = new WRF::varInfo_t();
		if (WRF::GetVarInfo(ncid, vname.c_str(), ncdims, *(var_info_map[vname])) < 0) {
			return(-1);
		}
	}
	return(0);
}
	
// Get info for all the variables we will have to read. This includes
// both variables that we will write (derived or native), as well as 
// those that are
// only needed to compute derived quantities. Variable info 
// will be stored in 'var_info_map'. 

int GetVarsInfo(
	int ncid,
	const vector <string> &copy_vars, 
	const WRF::atypVarNames_t &wrfNames,
	map <string, WRF::varInfo_t *> &var_info_map
) {
	int nvars;          // number of variables (not used)
	int ngatts;         // number of global attributes
	int xdimid;         // id of unlimited dimension (not used)
	int ndims;

	int nc_status;

	
	// Find the number of dimensions, variables, and global 
	// attributes, and check
	// the existance of the unlimited dimension
	nc_status = nc_inq(ncid, &ndims, &nvars, &ngatts, &xdimid );
	NC_ERR_READ( nc_status );

	// build list of all dimensions found in the file
	//
	vector <WRF::ncdim_t> ncdims;
	for (int dimid = 0; dimid < ndims; dimid++) {
		WRF::ncdim_t dim;

		nc_status = nc_inq_dim(
			ncid, dimid, dim.name, &dim.size
		);
		NC_ERR_READ(nc_status);
		ncdims.push_back(dim);
	}

	vector <string> derived;
	derived.push_back("ELEVATION");
	derived.push_back("PHNorm_");
	derived.push_back("PFull_");
	derived.push_back("PNorm_");
	derived.push_back("Theta_");
	derived.push_back("TK_");
	derived.push_back("UV_");
	derived.push_back("UVW_");
	derived.push_back("omZ_");

	// Get variable info for all variables
	//
	for (int i=0; i<copy_vars.size(); i++) {
		string vn = copy_vars[i];

		// Get Info for variable if not a derived quantity.
		//
		if (find(derived.begin(), derived.end(), vn) == derived.end()) {
			var_info_map[vn] = new WRF::varInfo_t();

			if (WRF::GetVarInfo(ncid, vn.c_str(), ncdims,*(var_info_map[vn]))<0)
				return(-1);
		}
	}

	// For all derivied quantities, get variable info for
	// dependent variables, if it doesn't already exist, 

	vector <string>::const_iterator itr;
	itr = find(copy_vars.begin(), copy_vars.end(), "ELEVATION");
	if (itr != copy_vars.end()) {

		if (get_dep_var_info(ncid, ncdims, wrfNames.PH, var_info_map) < 0)
			return(-1);

		if (get_dep_var_info(ncid, ncdims, wrfNames.PHB, var_info_map) < 0)
			return(-1);

	}

	itr = find(copy_vars.begin(), copy_vars.end(), "PHNorm_");
	if (itr != copy_vars.end()) {

		if (get_dep_var_info(ncid, ncdims, wrfNames.PHB, var_info_map) < 0)
			return(-1);
	}

	itr = find(copy_vars.begin(), copy_vars.end(), "PFull_");
	if (itr != copy_vars.end()) {

		if (get_dep_var_info(ncid, ncdims, wrfNames.P, var_info_map) < 0)
			return(-1);

		if (get_dep_var_info(ncid, ncdims, wrfNames.PB, var_info_map) < 0)
			return(-1);
	}

	itr = find(copy_vars.begin(), copy_vars.end(), "PNorm_");
	if (itr != copy_vars.end()) {

		if (get_dep_var_info(ncid, ncdims, wrfNames.P, var_info_map) < 0)
			return(-1);

		if (get_dep_var_info(ncid, ncdims, wrfNames.PB, var_info_map) < 0)
			return(-1);
	}

	itr = find(copy_vars.begin(), copy_vars.end(), "Theta_");
	if (itr != copy_vars.end()) {

		if (get_dep_var_info(ncid, ncdims, wrfNames.T, var_info_map) < 0)
			return(-1);

	}

	itr = find(copy_vars.begin(), copy_vars.end(), "TK_");
	if (itr != copy_vars.end()) {

		if (get_dep_var_info(ncid, ncdims, wrfNames.P, var_info_map) < 0)
			return(-1);

		if (get_dep_var_info(ncid, ncdims, wrfNames.PB, var_info_map) < 0)
			return(-1);

		if (get_dep_var_info(ncid, ncdims, wrfNames.T, var_info_map) < 0)
			return(-1);
	}

	itr = find(copy_vars.begin(), copy_vars.end(), "UV_");
	if (itr != copy_vars.end()) {

		if (get_dep_var_info(ncid, ncdims, wrfNames.U, var_info_map) < 0)
			return(-1);

		if (get_dep_var_info(ncid, ncdims, wrfNames.V, var_info_map) < 0)
			return(-1);
	}

	itr = find(copy_vars.begin(), copy_vars.end(), "UVW_");
	if (itr != copy_vars.end()) {

		if (get_dep_var_info(ncid, ncdims, wrfNames.U, var_info_map) < 0)
			return(-1);

		if (get_dep_var_info(ncid, ncdims, wrfNames.V, var_info_map) < 0)
			return(-1);

		if (get_dep_var_info(ncid, ncdims, wrfNames.W, var_info_map) < 0)
			return(-1);
	}

	itr = find(copy_vars.begin(), copy_vars.end(), "omZ_");
	if (itr != copy_vars.end()) {

		if (get_dep_var_info(ncid, ncdims, wrfNames.U, var_info_map) < 0)
			return(-1);

		if (get_dep_var_info(ncid, ncdims, wrfNames.V, var_info_map) < 0)
			return(-1);
	}

	return(0);
}

void ErrMsgCBHandler(const char *msg, int) {
	cerr << ProgName << " : " << msg << endl;
}

int	main(int argc, char **argv) {
	OptionParser op;
	float vertexts[2] = {0.0, 0.0};
	float *vertexts_ptr = vertexts;
	vector<string> twrfvars3d, twrfvars2d;
	vector<pair< TIME64_T, vector <float> > > wrf_timesExtents;
	vector<pair<string, double> > tgl_attr;
	vector<pair<string, double> >::iterator sf_pair_iter;
	vector<string> infiles;
	pair<string, double> attr;string startdate, tstartdate;
	string mapprocetion, tmapprojection;
	string ErrMsgStr;
	float dx, dy; // Not really needed for vorticity calculation, since its in metadata
	int ncid;
	
	const char	*metafile;
	
	string	s;
	MetadataVDC	*metadata;

	infiles.clear();

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
		cerr << "Usage: " << ProgName << " [options] metafile wrffiles..." << endl << endl;
		op.PrintOptionHelp(stderr);
		exit(0);
	}

	if (argc < 3) {
		cerr << "Usage: " << ProgName << " [options] metafile wrffiles..." << endl;
		op.PrintOptionHelp(stderr);
		exit(1);
	}

	WRF::atypVarNames_t wrfNames;

	argv++;
	argc--;
	metafile = *argv;

	argv++;
	argc--;

	if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	// Get all the metadata info we need from the VDC metafile
	//
	vector <string> vdf_vars;		// vars contained in VDF
	vector <TIME64_T> vdf_timestamps;	// User timestamps contained in VDF
	size_t vdf_dims[3];				// VDF dimensions
	vector <double> vdf_extents;
	int rc;
	rc = GetVDFInfo(
		metafile, vdf_vars, vdf_timestamps, wrfNames, vdf_dims, vdf_extents
	);
	if (rc < 0) exit(1);
	//Calculate DX and DY from vdf metadata
	float DX,DY;
	DX = (vdf_extents[3]-vdf_extents[0])/(float)(vdf_dims[0]-1);
	DY = (vdf_extents[4]-vdf_extents[1])/(float)(vdf_dims[1]-1);

	for (vector<string>::iterator itr = opt.varnames.begin(); itr!=opt.varnames.end(); itr++) {
		if (find(vdf_vars.begin(),vdf_vars.end(),*itr)==vdf_vars.end()) {
			MyBase::SetErrMsg(
				"Requested variable \"%s\" not found in VDF file %s",
				itr->c_str(), metafile);
			exit(1);
		}
	}

	// Process the netCDF files

	metadata = new MetadataVDC(metafile);
	if (MetadataVDC::GetErrCode() != 0) {
		cerr << "Unable to process VDC !" << endl;
		exit(1);
	}

	int MaxTimeSteps = (opt.numts < 0) ? INT_MAX : opt.numts;
	size_t TotalTimeSteps = 0;
	vector <string> copy_vars;		// list of vars to copy
	vector <long> copy_vdf_timesteps;	// time steps to copy to in VDC
	vector <long> copy_wrf_timesteps;	// time steps to copy from in netCDF file
        float p2si = 1.0;
	bool p2si_flag = false;

	for (int arg = 0; arg<argc && TotalTimeSteps < MaxTimeSteps; arg++) {

		vector <string> wrf_vars;		// vars contained in netCDF file
		size_t wrf_dims[4];				// dims of 3d vars

		WRF wrf(argv[arg]);
		if (WRF::GetErrCode() != 0) {
			WRF::SetErrMsg(
				"Error processing file %s, skipping.", argv[arg]);
			WRF::SetErrCode(0);
			continue;

		}
		wrf.GetWRFMeta(
			vertexts_ptr,wrf_dims,tstartdate,tmapprojection,twrfvars3d,
			twrfvars2d,tgl_attr,wrf_timesExtents);

		if (vertexts_ptr[0] >= vertexts_ptr[1]) {
		WRF::SetErrMsg(
			"Error processing file %s (invalid vertical extents), skipping.",
				argv[arg]);
			WRF::SetErrCode(0);
			continue;
		}

		// Grab the dx and dy values from attrib list;
        	// Grab the P2SI value if there, Used to convert PlanetWRF timestamps to SI times. 

		for(sf_pair_iter= tgl_attr.begin();sf_pair_iter< tgl_attr.end();sf_pair_iter++) {
			attr = *sf_pair_iter;
			if (attr.first == "DX") 
				dx = attr.second;
			if (attr.first == "DY") 
				dy = attr.second;
			if(attr.first == "P2SI") {
				p2si = attr.second;
				p2si_flag = true;
			}
		}
		if(dx < 0.0 || dy < 0.0) {
			ErrMsgStr.assign("Error: DX and DY attributes not found in ");
			ErrMsgStr += argv[arg];
			ErrMsgStr.append(", skipping.");
			WRF::SetErrMsg(ErrMsgStr.c_str());
			WRF::SetErrCode(0);
			continue;
		}

		if (dx < 0.f) dx = DX;
		if (dy < 0.f) dy = DY;

		if (
			vdf_dims[0] != wrf_dims[0] || 
			vdf_dims[1] != wrf_dims[1] ||
			vdf_dims[2] != wrf_dims[2]
		) {

			cerr << "Dimension mismatch, skipping file " << argv[arg] << endl;
			continue;
		}

		// Figure out which timesteps we're copying 
		
		TIME64_T tfirst = *(vdf_timestamps.begin());
		TIME64_T tlast = *(vdf_timestamps.end()-1);
		if (strlen(opt.tsstart) != 0) {
			if (WRF::WRFTimeStrToEpoch(opt.tsstart, &tfirst) < 0) exit(1);
		}
		if (strlen(opt.tsend) != 0) {
			if (WRF::WRFTimeStrToEpoch(opt.tsend, &tlast) < 0) exit(1);
		}
			
		for (size_t t=0; t<wrf_timesExtents.size(); t++) {
			TIME64_T tstamp = wrf_timesExtents[t].first;
			if(p2si_flag) {
				tstamp *= p2si;
			}
			vector <TIME64_T>::iterator itr;
			if (
				tstamp >= tfirst && 
				tstamp <= tlast &&
				((itr = find(vdf_timestamps.begin(), vdf_timestamps.end(), tstamp))!=vdf_timestamps.end())
			) {
				infiles.push_back(argv[arg]);
				copy_wrf_timesteps.push_back(t);
				copy_vdf_timesteps.push_back(itr-vdf_timestamps.begin());
			}
		} // End for t.

		// Create one list of wrf variables.

		for(size_t vt=0; vt <twrfvars2d.size(); vt++)
			wrf_vars.push_back(twrfvars2d[vt]); 
		for(size_t vt=0; vt <twrfvars3d.size(); vt++)
			wrf_vars.push_back(twrfvars3d[vt]); 

		// Figure out which variables we're copying

		SelectVariables(
			vdf_vars, wrf_vars, opt.varnames, wrfNames, copy_vars,
			(opt.noelev != 0));

		if (! opt.noelev && find(copy_vars.begin(),copy_vars.end(),"ELEVATION")==copy_vars.end()) {
			cerr << "Elevation could not be computed, skipping file " << argv[arg] << endl;
			continue;
		}

		nc_close(ncid); // Close the netCDF file

	} // End for arg, files from command line.

	MetadataWRF *WRFMeta;
	WRFMeta = new MetadataWRF(infiles);
	if (!WRFMeta) {
		WRF::SetErrMsg("Unable to create MetadataWRF object!");
		exit(1);
	}
	WRFReader *WRFRead;
	WRFRead = new WRFReader(*WRFMeta);
	if (!WRFRead) {
		WRF::SetErrMsg("Unable to create WRFReader object!");
		exit(1);
	}

	// Grab the gravity value if there or set to Earth's value.

	double grav = 9.81;
	vector < pair <string, double> > global_attr = WRFMeta->GetGlobalAttributes();
	for (int i=0; i < global_attr.size(); i++) {
		if (global_attr[i].first == "G")
			grav = global_attr[i].second;
	}

	// Copy the current file, one timestep at a time

	for (int t=0; t<copy_wrf_timesteps.size() && TotalTimeSteps < MaxTimeSteps; t++) {
		double wrf_vexts[2];		// vertical extents of wrf time step

		int wrf_ts = copy_wrf_timesteps[t]; 
		size_t vdf_ts = copy_vdf_timesteps[t];
		size_t tmp_vdf_ts = -1;
		vector <string> wrk_vars = copy_vars;
		string wrf_filename;
		size_t tmp_wrf_ts;
		map <string, WRF::varInfo_t *> var_info_map;

		// Since maybe a mismatch between what the MetadataWRF and the
		// MetadataVDC with the vdf timestep.  This is due the processing
		// not all of the datafiles at the same time.  But there is a way
		// to solve this.

		for(int i = 0; i < WRFMeta->GetNumTimeSteps(); i++) {
			if( metadata->GetTSUserTime(vdf_ts) == WRFMeta->GetTSUserTime(i)) {
				tmp_vdf_ts = i;
				break;
			}
		}
	
		WRFMeta->MapVDCTimestep(tmp_vdf_ts, wrf_filename, tmp_wrf_ts);

		// Open the netCDF file for reading
		//
		rc = OpenWrfFile(wrf_filename.c_str(), ncid);
		if (rc<0) {
			WRF::SetErrMsg("Failed to open, skipping file \"%s\"",
				wrf_filename.c_str());
			continue;
		}

		// Get info about specific netCDF variables
		//

		if (GetVarsInfo(ncid, wrk_vars, wrfNames, var_info_map)  < 0) {
			WRF::SetErrMsg("Couldn't get needed metadata, skipping file \"%s\"",
				wrf_filename.c_str());
			continue;
		}

		if (! opt.quiet) {
			cout << endl << "\tTime step : " << vdf_ts << " (VDC), " << wrf_ts << 
				" (WRF)" << endl;
		}

		if (! opt.quiet) {
			cout << "Processing file : " << wrf_filename.c_str() << endl;
			cout << "\tTransforming variables : ";
			for (int i=0; i<copy_vars.size(); i++) {
				cout << copy_vars[i] << " ";
			}
			cout << endl;
		}

		// Loop over the variables.

		while(wrk_vars.size() > 0) {

			rc = DoGeopotStuff(
				WRFRead, vdf_dims, var_info_map, vdf_ts, wrf_ts,
				*metadata, wrfNames, wrk_vars, wrf_vexts, grav);

			if ((wrf_vexts[0] < (vdf_extents[2]-0.000001)) && ! opt.quiet && ! opt.noelev) {
				cerr << ProgName << " : Warning, extents of file " << wrf_filename << " out of range\n";
			}
			if ((wrf_vexts[1] < (vdf_extents[5]-0.000001)) && ! opt.quiet && ! opt.noelev) {
				cerr << ProgName << " : Warning, extents of file " << wrf_filename << " out of range\n";
			}
			
			// Find wind speeds, if necessary
			rc = DoWindStuff( 
				WRFRead, vdf_dims, dx, dy, var_info_map, vdf_ts, wrf_ts,
				*metadata, wrfNames, wrk_vars);
			if (rc<0) MyBase::SetErrCode(0);

			// Find P- or T-related derived variables, if necessary
			rc = DoPTStuff(
				WRFRead, vdf_dims, var_info_map, vdf_ts, wrf_ts,
				*metadata, wrfNames, wrk_vars);
			if (rc<0) MyBase::SetErrCode(0);

			// Remaining 3D variables
			rc = DoIndependentVars3d(
				WRFRead, vdf_dims, var_info_map, vdf_ts, wrf_ts,
				*metadata, wrfNames, wrk_vars);
			if (rc<0) MyBase::SetErrCode(0);

			// Remaining 2D variables
			rc = DoIndependentVars2d(
				WRFRead, vdf_dims, var_info_map, vdf_ts, wrf_ts,
				*metadata, wrfNames, wrk_vars);
			if (rc<0) MyBase::SetErrCode(0);

			// Free elements from var_info_map, which are dynmically allocated

			map<string, WRF::varInfo_t *>::iterator iter;
			for( iter = var_info_map.begin(); iter != var_info_map.end(); iter++ ) {
				if (iter->second) delete iter->second;
				iter->second = NULL;
			}
			var_info_map.clear();

			// Shouldn't have any variables left to process
			assert(wrk_vars.size() == 0);
			wrk_vars.clear();

		} // End of while (wrk_vars).
		TotalTimeSteps++;
	} // End of for t.

	if (! opt.quiet) {
		cout << endl;
		cout << "Transformed " << TotalTimeSteps << " time steps" << endl;
	}

	exit(0);
}
