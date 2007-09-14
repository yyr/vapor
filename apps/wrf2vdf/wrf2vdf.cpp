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
//      File:		ncdf2vdf.cpp
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

#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/Metadata.h>
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
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"varnames",1, 	"",	"Colon delimited list of variable names in\n\t\t\t\tmetadata to convert"},
	{"tsincr",	1,	"1","Increment between Vapor times steps to convert\n\t\t\t\t(e.g., 3=every third), from Vapor time \n\t\t\t\tstep 0 (NOT CURRENTLY SUPPORTED!!!)"},
	{"numts",	1,	"-1","Maximum number of time steps that may be converted"},
	{"tsstart", 1,  "", "Starting time stamp for conversion (default is\n\t\t\t\tfound in VDF"},
	{"tsend",	1,  "", "Last time stamp to convert (default is latest\n\t\t\t\ttime stamp)"},
	{"level",	1, 	"-1","Refinement levels saved. 0=>coarsest, 1=>next\n\t\t\t\trefinement, etc. -1=>finest"},
	{"help",	0,	"",	"Print this message and exit"},
	{"debug",	0,	"",	"Enable debugging"},
	{"quiet",	0,	"",	"Operate quietly (outputs only vertical extents\n\t\t\t\tthat are lower than those in the VDF)"},
	{NULL}
};


OptionParser::Option_T	get_options[] = {
	{"varnames", VetsUtil::CvtToStrVec, &opt.varnames, sizeof(opt.varnames)},
	{"tsincr", VetsUtil::CvtToInt, &opt.tsincr, sizeof(opt.tsincr)},
	{"numts", VetsUtil::CvtToInt, &opt.numts, sizeof(opt.numts)},
	{"tsstart", VetsUtil::CvtToString, &opt.tsstart, sizeof(opt.tsstart)},
	{"tsend", VetsUtil::CvtToString, &opt.tsend, sizeof(opt.tsend)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
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
//
int OpenWrfFile(
	const char * netCDFfile, // Path of WRF output file to be opened (input)
	int & ncid // ID of netCDF file (output)
) {
	
	int nc_status;		// Holds error codes for debugging
	NC_ERR_READ( nc_status = nc_open(netCDFfile, NC_NOWRITE, &ncid ));

	return(0);
}

    

// Calculates needed and desired quantities involving PH and 
// PHB--elevation (needed for
// on-the-fly interpolation in Vapor) and normalized geopotential,
// and the original variables.
//
int DoGeopotStuff(
	int ncid, 
	const size_t dim[3],
	const map<string, WRF::varInfo_t *> &var_info_map, 
	size_t aVaporTs, 
	int wrfT, 
	Metadata *metadata, 
	const WRF::atypVarNames_t &wrfNames,
	vector<string> &varnames, 
	float extents[2]
) {

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

	static float * phBufferAbove = NULL;
	static float * phbBufferAbove = NULL;

	size_t slice_sz;
	if ((! phBuffer || ! phBufferAbove) && phInfoPtr) {
		slice_sz = phInfoPtr->dimlens[phInfoPtr->ndimids-1] *
            phInfoPtr->dimlens[phInfoPtr->ndimids-2];

		if (! phBuffer) phBuffer = new float[slice_sz];

		if ((*phInfoPtr).stag[2] ) {
			if (!phBufferAbove) phBufferAbove = new float[slice_sz];
		}
	}
	if ((! phbBuffer || ! phbBufferAbove) && phbInfoPtr) {
		slice_sz = phbInfoPtr->dimlens[phbInfoPtr->ndimids-1] *
            phbInfoPtr->dimlens[phbInfoPtr->ndimids-2];

		if (! phbBuffer) phbBuffer = new float[slice_sz];

		if ((*phbInfoPtr).stag[2] ) {
			if (!phbBufferAbove) phbBufferAbove = new float[slice_sz];
		}
	}
	if (! workBuffer ) {
		if (! workBuffer) workBuffer = new float[dim[0]*dim[1]];
	}


	// Prepare wavelet block writers
	//
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
		phbWriter->OpenVariableWrite(aVaporTs, "PHNorm_", opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), "PHNorm_"));
	}


	// Switches to prevent redundant reads, in case of vertical staggering
	bool needAnotherPh = true;
	bool needAnotherPhb = true;

	// Loop over z slices
	int rc = 0;
	for ( size_t z = 0 ; z < dim[2] ; z++ ) {

		// Read needed slices
		rc = WRF::GetZSlice(
			ncid, *phInfoPtr, wrfT, z, phBuffer, phBufferAbove, 
			needAnotherPh, dim 
		);
		if (rc<0) break;

		rc = WRF::GetZSlice(
			ncid, *phbInfoPtr, wrfT, z, phbBuffer, phbBufferAbove, 
			needAnotherPhb, dim 
		);
		if (rc<0) break;

	
		// Write PH and/or PHB, if desired
		if ( wantPh ) phWriter->WriteSlice( phBuffer );
		if ( wantPhb ) phbWriter->WriteSlice( phbBuffer );

		// Write geopotential height, with name ELEVATION.  This is required by Vapor
		// to handle leveled data.
		for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ ) {
			workBuffer[i] = (phBuffer[i] + phbBuffer[i])/9.81;
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
				workBuffer[i] *= 9.81/phbBuffer[i];
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
// 2D wind speed (U-V plane) and 3D wind speed
int DoWindSpeed(
	int ncid, 
	const size_t dim[3],
	const map<string, WRF::varInfo_t *> &var_info_map, 
	size_t aVaporTs, 
	int wrfT, 
	Metadata *metadata,
	const WRF::atypVarNames_t &wrfNames,
	vector<string> &varnames
) {
	bool wantU = find(varnames.begin(),varnames.end(),wrfNames.U)!=varnames.end();
	bool wantV = find(varnames.begin(),varnames.end(),wrfNames.V)!=varnames.end();
	bool wantW = find(varnames.begin(),varnames.end(),wrfNames.W)!=varnames.end();
	bool wantUV = find(varnames.begin(),varnames.end(),"UV_")!=varnames.end();
	bool wantUVW = find(varnames.begin(),varnames.end(),"UVW_")!=varnames.end();

	// Make sure we have work to do.
	//
	if (! (wantU || wantV || wantW || wantUV || wantUVW)) return(0);

	map<string, WRF::varInfo_t *>::const_iterator itr;


	itr = var_info_map.find(wrfNames.U);
	WRF::varInfo_t  *uInfoPtr = itr==var_info_map.end() ? NULL : itr->second; 

	itr = var_info_map.find(wrfNames.V);
	WRF::varInfo_t  *vInfoPtr = itr==var_info_map.end() ? NULL : itr->second; 

	itr = var_info_map.find(wrfNames.W);
	WRF::varInfo_t  *wInfoPtr = itr==var_info_map.end() ? NULL : itr->second; 

	// Allocate static storage array
	static float * uBuffer = NULL; 
	static float * vBuffer = NULL;
	static float * wBuffer = NULL;
	static float * uvBuffer = NULL;
	static float * uvwBuffer = NULL;

	static float * uBufferAbove = NULL; 
	static float * vBufferAbove = NULL;
	static float * wBufferAbove = NULL;

	size_t slice_sz;

	if (wantU || wantUV || wantUVW) {
		slice_sz = uInfoPtr->dimlens[uInfoPtr->ndimids-1] *
            uInfoPtr->dimlens[uInfoPtr->ndimids-2];

		if (! uBuffer) uBuffer = new float[slice_sz];

		if ( uInfoPtr->stag[2] )
			if (! uBufferAbove) uBufferAbove = new float[slice_sz];
	}
	if (wantV || wantUV || wantUVW) {
		slice_sz = vInfoPtr->dimlens[vInfoPtr->ndimids-1] *
            vInfoPtr->dimlens[vInfoPtr->ndimids-2];

		if (! vBuffer) vBuffer = new float[slice_sz];
		if ( vInfoPtr->stag[2] )
			if (! vBufferAbove) vBufferAbove = new float[slice_sz];
	}
	if (wantW || wantUVW) {
		slice_sz = wInfoPtr->dimlens[wInfoPtr->ndimids-1] *
            wInfoPtr->dimlens[wInfoPtr->ndimids-2];

		if (! wBuffer) wBuffer = new float[slice_sz];
		if ( wInfoPtr->stag[2] )
			if (! wBufferAbove) wBufferAbove = new float[slice_sz];
	}
	if ( wantUV ) {
		if (! uvBuffer) uvBuffer = new float[dim[0]*dim[1]]; 
	}
	if ( wantUVW ) {
		if (! uvwBuffer) uvwBuffer = new float[dim[0]*dim[1]]; 
	}

	// Prepare wavelet block writers
	//
	WaveletBlock3DBufWriter * uWriter = NULL;
	WaveletBlock3DBufWriter * vWriter = NULL;
	WaveletBlock3DBufWriter * wWriter = NULL;
	WaveletBlock3DBufWriter * uvWriter = NULL;
	WaveletBlock3DBufWriter * uvwWriter = NULL;

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


	// Switches to prevent redundant reads in case of vertical staggering
	bool needAnotherU = true;
	bool needAnotherV = true;
	bool needAnotherW = true;

	// Read, calculate, and write the data
	int rc = 0;
	for ( size_t z = 0 ; z < dim[2] ; z++ )
	{
		// Read (if desired or needed)
		if ( wantU || wantUV || wantUVW ) {
			rc = WRF::GetZSlice(
				ncid, (*uInfoPtr), wrfT, z, uBuffer, uBufferAbove, 
				needAnotherU, dim
			);
			if (rc<0) break;
		}

		if ( wantV || wantUV || wantUVW ) {
			rc = WRF::GetZSlice(
				ncid, (*vInfoPtr), wrfT, z, vBuffer, vBufferAbove, 
				needAnotherV, dim
			);
			if (rc<0) break;
		}

		if ( wantW || wantUVW ) {
			rc = WRF::GetZSlice(
				ncid, (*wInfoPtr), wrfT, z, wBuffer, wBufferAbove, 
				needAnotherW, dim
			);
			if (rc<0) break;
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

		if ( wantUVW ) {
			for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
				uvwBuffer[i] = sqrt( uBuffer[i]*uBuffer[i] + vBuffer[i]*vBuffer[i] + wBuffer[i]*wBuffer[i] );
			uvwWriter->WriteSlice( uvwBuffer );
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

	return(rc);

}


// Read/calculates and outputs some or all of P, PB, PFull_, PNorm_,
// T, Theta_, and TK_.  Sets flags so that variables are not read twice.
int DoPTStuff(
	int ncid, 
	const size_t dim[3],
	const map<string, WRF::varInfo_t *> &var_info_map, 
	size_t aVaporTs, 
	int wrfT, 
	Metadata *metadata,
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

	// Make sure we have work to do.
	//
	if (! (wantP || wantPb || wantT || wantPfull || wantPnorm || wantTheta || wantTk)) return(0);

	map<string, WRF::varInfo_t *>::const_iterator itr;

	itr = var_info_map.find(wrfNames.P);
	WRF::varInfo_t  *pInfoPtr = itr==var_info_map.end() ? NULL : itr->second; 

	itr = var_info_map.find(wrfNames.PB);
	WRF::varInfo_t  *pbInfoPtr = itr==var_info_map.end() ? NULL : itr->second; 

	itr = var_info_map.find(wrfNames.T);
	WRF::varInfo_t  *tInfoPtr = itr==var_info_map.end() ? NULL : itr->second; 

	
	// Allocate static storage array
	static float * pBuffer = NULL; 
	static float * pbBuffer = NULL;
	static float * tBuffer = NULL;
	static float * workBuffer = NULL;

	static float * pBufferAbove = NULL; 
	static float * pbBufferAbove = NULL;
	static float * tBufferAbove = NULL;

	size_t slice_sz;
	if ( wantP || wantPfull || wantPnorm || wantTk ) {
		slice_sz = pInfoPtr->dimlens[pInfoPtr->ndimids-1] *
            pInfoPtr->dimlens[pInfoPtr->ndimids-2];

		if (! pBuffer) pBuffer = new float[slice_sz];
		if ( pInfoPtr->stag[2] )
			if (! pBufferAbove) pBufferAbove = new float[slice_sz];
	}
	if ( wantPb || wantPfull || wantPnorm || wantTk ) {
		slice_sz = pbInfoPtr->dimlens[pbInfoPtr->ndimids-1] *
            pbInfoPtr->dimlens[pbInfoPtr->ndimids-2];

		if (! pbBuffer) pbBuffer = new float[slice_sz];
		if ( pbInfoPtr->stag[2] )
			if (! pbBufferAbove) pbBufferAbove = new float[slice_sz];
	}
	if ( wantT || wantTheta || wantTk ) {
		slice_sz = tInfoPtr->dimlens[tInfoPtr->ndimids-1] *
            tInfoPtr->dimlens[tInfoPtr->ndimids-2];

		if (! tBuffer) tBuffer = new float[slice_sz];
		if ( tInfoPtr->stag[2] )
			if (! tBufferAbove) tBufferAbove = new float[slice_sz];
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


	// Switches to prevent redundant reads in the case of vertical staggering
	bool needAnotherP = true;
	bool needAnotherPb = true;
	bool needAnotherT = true;

	// Read, calculate, and write
	int rc = 0;
	for ( size_t z = 0 ; z < dim[2] ; z++ )
	{
		// Read
		if ( wantP || wantPfull || wantPnorm || wantTk ) {
			rc = WRF::GetZSlice(
				ncid, (*pInfoPtr), wrfT, z, pBuffer, pBufferAbove,
				needAnotherP, dim 
			);
			if (rc<0) break;
		}
		if ( wantPb || wantPfull || wantPnorm || wantTk ) {
			rc = WRF::GetZSlice(
				ncid, (*pbInfoPtr), wrfT, z, pbBuffer, pbBufferAbove,
				needAnotherPb, dim
			);
			if (rc<0) break;
		}
		if ( wantT || wantTheta || wantTk ) {
			rc = WRF::GetZSlice(
				ncid, (*tInfoPtr), wrfT, z, tBuffer, tBufferAbove,
				needAnotherT, dim
			);
			if (rc<0) break;
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
				workBuffer[i] = 0.03719785781*(tBuffer[i] + 300.0)
								* pow( pBuffer[i] + pbBuffer[i], 0.285896414f );
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


int DoIndependentVars(
	int ncid, 
	const size_t dim[3],
	const map<string, WRF::varInfo_t *> &var_info_map, 
	size_t aVaporTs, 
	int wrfT, 
	Metadata *metadata,
	const WRF::atypVarNames_t &wrfNames,
	vector<string> &varnames

) {
	static float * varBuffer = NULL; 
	static float * varBufferAbove = NULL; 
	WaveletBlock3DBufWriter * varWriter = NULL;

	varWriter = new WaveletBlock3DBufWriter(metadata);
	if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);

	if (! varBuffer) {
		varBuffer = new float[dim[0]*dim[1]]; 
	}

	int rc = 0;
	vector <string> vn_copy = varnames;
	for (int i=0; i<vn_copy.size(); i++) {

		map<string, WRF::varInfo_t *>::const_iterator itr;

		itr = var_info_map.find(vn_copy[i]);
		WRF::varInfo_t  *varInfoPtr = itr==var_info_map.end() ? NULL : itr->second; 

		assert(varInfoPtr != NULL);

		if ( varInfoPtr->stag[2] ) {
			if (! varBufferAbove) varBufferAbove = new float[dim[0]*dim[1]];
		}

		varWriter->OpenVariableWrite(aVaporTs, vn_copy[i].c_str(), opt.level);
		if (WaveletBlock3DBufWriter::GetErrCode() != 0) return(-1);
		varnames.erase(find(varnames.begin(), varnames.end(), vn_copy[i]));

		// Switches to prevent redundant reads in the case of 
		// vertical staggering
		bool needAnother = true;

		// Read and write
		for ( size_t z = 0 ; z < dim[2] ; z++ ) {
			// Read
			rc = WRF::GetZSlice(
				ncid, *varInfoPtr, wrfT, z, varBuffer, varBufferAbove,
				needAnother, dim 
			);
			if (rc<0) break;

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

int GetVDFInfo(
	const char *metafile,
	vector <string> &vars,
	vector <long>	&timestamps,
	WRF::atypVarNames_t  &wrfNames,
	size_t dims[3],
	vector <double> &extents
) {

	Metadata *metadata;

	vars.clear();
	timestamps.clear();

	metadata = new Metadata(metafile);

	if (Metadata::GetErrCode() != 0) {
		return(-1);
    }

	vars = metadata->GetVariableNames();

	long numts = metadata->GetNumTimeSteps();
	if (numts < 0) return(-1);

	for (size_t t = 0; t<numts; t++) {
		
#ifdef	DEAD
		const vector <double> &v = metadata->GetTSUserTime(t);
		timestamps.push_back((long) v[0]);
#else

		string tag("UserTimeStampString");
		string s = metadata->GetTSUserDataString(t, tag);
		StrRmWhiteSpace(s);
			
		long t0;
		if (WRF::WRFTimeStrToEpoch(s, &t0) < 0) return(-1);
		timestamps.push_back(t0);
#endif
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

int GetWRFInfo(
	const char *file,
	vector <string> &vars,
	vector <long>	&timestamps,
	size_t dims[3]
) {
	int rc;

	vars.clear();
	timestamps.clear();

	const WRF::atypVarNames_t atypnames_dummy;
	float dx, dy;
	size_t dimLens[4];
	string startDate;

	rc = WRF::OpenWrfGetMeta(
		file, atypnames_dummy, dx, dy, NULL, dimLens, startDate,
		vars, timestamps
	);
	for (int i=0; i<3; i++) dims[i] = dimLens[i];

	return(0);
}
	
	
// Create a list of variables to be translated based on what the user
// wants (via command line option, 'opt.varnames', if not empty, else
// via contents of 'vdf_vars'), and what is available in the 
// WRF file (wrf_vars). The list of variables to translate is
// returned in 'copy_vars'
//
void SelectVariables(
	const vector <string> &vdf_vars, 
	const vector <string> &wrf_vars, 
	const vector <string> &opt_varnames, 
	const WRF::atypVarNames_t &wrfNames,
	vector <string> &copy_vars
) {
	copy_vars.clear();

	vector <string> candidates; // Candidate variables for selection

	if (opt_varnames.size()) candidates = opt_varnames;
	else candidates = vdf_vars;

	for (int i=0; i<candidates.size(); i++) {
		string v = candidates[i];

		// If candidate variable is a derived quantity (e.g. ELEVATION)
		// we need to make sure its dependencies are available in the
		// WRF file
		//
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

	// Always include "ELEVATION"
	//
	if (find(copy_vars.begin(),copy_vars.end(),"ELEVATION")==copy_vars.end()){
		copy_vars.push_back("ELEVATION");
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
//
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
	NC_ERR_READ( nc_status = nc_inq(ncid, &ndims, &nvars, &ngatts, &xdimid ) );

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
	//

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

	itr = find(copy_vars.begin(), copy_vars.end(), "UV_");
	if (itr != copy_vars.end()) {

		if (get_dep_var_info(ncid, ncdims, wrfNames.U, var_info_map) < 0)
			return(-1);

		if (get_dep_var_info(ncid, ncdims, wrfNames.V, var_info_map) < 0)
			return(-1);

		if (get_dep_var_info(ncid, ncdims, wrfNames.W, var_info_map) < 0)
			return(-1);
	}

	return(0);
}

void ErrMsgCBHandler(const char *msg, int) {
	cerr << ProgName << " : " << msg << endl;
}

int	main(int argc, char **argv) {
	OptionParser op;
	
	const char	*metafile;
	
	string	s;
	Metadata	*metadata;

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
		cerr << "Usage: " << ProgName << " [options] metafile wrffiles..." << endl;
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
	vector <long> vdf_timestamps;	// User timestamps contained in VDF
	size_t vdf_dims[3];				// VDF dimensions
	vector <double> vdf_extents;
	int rc;
	rc = GetVDFInfo(
		metafile, vdf_vars, vdf_timestamps, wrfNames, vdf_dims, vdf_extents
	);
	if (rc < 0) exit(1);


	// Process the netCDF files
	//
	metadata = new Metadata(metafile);
	if (Metadata::GetErrCode() != 0) exit(1);


	int MaxTimeSteps = (opt.numts < 0) ? INT_MAX : opt.numts;
	size_t TotalTimeSteps = 0;
	for (int arg = 0; arg<argc && TotalTimeSteps < MaxTimeSteps; arg++) {

		vector <string> wrf_vars;		// vars contained in netCDF file
		vector <long> wrf_timestamps;	// timestamps contained in netCDF file
		size_t wrf_dims[4];				// dims of 3d vars
		vector <string> copy_vars;		// list of vars to copy
		int ncid;

		// time steps to copy to in VDC
		vector <long> copy_vdf_timesteps;

		// time steps to copy from in netCDF file
		vector <long> copy_wrf_timesteps;

		rc = GetWRFInfo(argv[arg], wrf_vars, wrf_timestamps, wrf_dims);
		if (rc<0) {
			cerr << "Skipping file " << argv[arg] << endl;
			continue;
		}

		if (
			vdf_dims[0] != wrf_dims[0] || 
			vdf_dims[1] != wrf_dims[1] ||
			vdf_dims[2] != wrf_dims[2]
		) {

			cerr << "Dimension mismatch, skipping file " << argv[arg] << endl;
			continue;
		}

		// Figure out which timesteps we're copying 
		//
		long tfirst = *(vdf_timestamps.begin());
		long tlast = *(vdf_timestamps.end()-1);
		if (strlen(opt.tsstart) != 0) {
			if (WRF::WRFTimeStrToEpoch(opt.tsstart, &tfirst) < 0) exit(1);
		}
		if (strlen(opt.tsend) != 0) {
			if (WRF::WRFTimeStrToEpoch(opt.tsend, &tlast) < 0) exit(1);
		}
			
		for (size_t t=0; t<wrf_timestamps.size(); t++) {
			long tstamp = wrf_timestamps[t];
			vector <long>::iterator itr;
			if (
				tstamp >= tfirst && 
				tstamp <= tlast &&
				((itr = find(vdf_timestamps.begin(), vdf_timestamps.end(), tstamp))!=vdf_timestamps.end())
			) {

				copy_wrf_timesteps.push_back(t);
				copy_vdf_timesteps.push_back(itr-vdf_timestamps.begin());
			}
		}

		// Figure out which variables we're copying
		//
		SelectVariables(vdf_vars, wrf_vars, opt.varnames, wrfNames, copy_vars);

		if (find(copy_vars.begin(),copy_vars.end(),"ELEVATION")==copy_vars.end()) {
			cerr << "Elevation could not be computed, skipping file " << argv[arg] << endl;
			continue;
		}

		// Open the netCDF file for reading
		//
		rc = OpenWrfFile(argv[arg], ncid);
		if (rc<0) {
			cerr << "Failed to open, skipping file " << argv[arg] << endl;
			continue;
		}

		// Get info about specific netCDF variables
		//
		map <string, WRF::varInfo_t *> var_info_map;
		if (GetVarsInfo(ncid, copy_vars, wrfNames, var_info_map)  < 0) {
			cerr << "Couldn't get needed metadata, skipping file " << argv[arg] << endl;
			continue;
		}

		if (! opt.quiet) {
			cout << "Processing file : " << argv[arg] << endl;
			cout << "\tTransforming variables : ";
			for (int i=0; i<copy_vars.size(); i++) {
				cout << copy_vars[i] << " ";
			}
			cout << endl;
		}


		// Copy the current file, one timestep at a time
		//
		for (int t=0; t<copy_wrf_timesteps.size() && TotalTimeSteps < MaxTimeSteps; t++) {
			float wrf_vexts[2];		// vertical extents of wrf time step

			int wrf_ts = copy_wrf_timesteps[t]; 
			size_t vdf_ts = copy_vdf_timesteps[t]; 
			vector <string> vars = copy_vars;	// Do* routines modify vars

			if (! opt.quiet) {
				cout << "\tTime step : " << vdf_ts << " (VDC), " << wrf_ts << 
					" (WRF)" << endl;
			}
			
			rc = DoGeopotStuff(
				ncid, vdf_dims, var_info_map, vdf_ts, wrf_ts,
				metadata, wrfNames, vars, wrf_vexts
			);
			if (rc<0) MyBase::SetErrCode(0);
			if (wrf_vexts[0] < vdf_extents[2] && ! opt.quiet) {
				cerr << ProgName << " : Warning, extents of file " << argv[arg] << " out of range\n";
			}
			if (wrf_vexts[1] < vdf_extents[5] && ! opt.quiet) {
				cerr << ProgName << " : Warning, extents of file " << argv[arg] << " out of range\n";
			}

			// Find wind speeds, if necessary
			rc = DoWindSpeed( 
				ncid, vdf_dims, var_info_map, vdf_ts, wrf_ts,
				metadata, wrfNames, vars
			);
			if (rc<0) MyBase::SetErrCode(0);

			// Find P- or T-related derived variables, if necessary
			rc = DoPTStuff(
				ncid, vdf_dims, var_info_map, vdf_ts, wrf_ts,
				metadata, wrfNames, vars
			);
			if (rc<0) MyBase::SetErrCode(0);

			// Find P- or T-related derived variables, if necessary
			rc = DoIndependentVars(
				ncid, vdf_dims, var_info_map, vdf_ts, wrf_ts,
				metadata, wrfNames, vars
			);
			if (rc<0) MyBase::SetErrCode(0);

			TotalTimeSteps++;
		}

		
		// Free elements from var_info_map, which are dynmically allocated
		//
		map<string, WRF::varInfo_t *>::iterator iter;
		for( iter = var_info_map.begin(); iter != var_info_map.end(); iter++ ) {
			if (iter->second) delete iter->second;
			iter->second = NULL;
		}
		var_info_map.clear();

		nc_close(ncid); // Close the netCDF file
	}

	if (! opt.quiet) {
		cout << endl;
		cout << "Transformed " << TotalTimeSteps << " time steps" << endl;
	}

	exit(0);
}
