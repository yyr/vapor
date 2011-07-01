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
	int numts;
	int level;
	float tolerance;
	OptionParser::Boolean_T	noelev;
	OptionParser::Boolean_T	help;
	OptionParser::Boolean_T	debug;
	OptionParser::Boolean_T	quiet;
} opt;

OptionParser::OptDescRec_T	set_opts[] = {
	{"varnames",1, 	"",	"Colon delimited list of variable names in WRF file to convert. The default is to convert variables in the set intersection of those variables found in the VDF file and those in the WRF file."},
	{"numts",	1,	"-1","Maximum number of time steps that may be converted. A -1 implies the conversion of all time steps found"},
	{"level",	1, 	"-1","Refinement levels saved. 0=>coarsest, 1=>next refinement, etc. -1=>finest"},
	{"tolerance",	1, 	"0.0000001","Tolerance for comparing relative user times"},
	{"noelev",	0,	"",	"Do not generate the ELEVATION variable required by vaporgui."},
	{"help",	0,	"",	"Print this message and exit."},
	{"debug",	0,	"",	"Enable debugging."},
	{"quiet",	0,	"",	"Operate quietly (outputs only vertical extents that are lower than those in the VDF)."},
	{NULL}
};

OptionParser::Option_T	get_options[] = {
	{"varnames", VetsUtil::CvtToStrVec, &opt.varnames, sizeof(opt.varnames)},
	{"numts", VetsUtil::CvtToInt, &opt.numts, sizeof(opt.numts)},
	{"level", VetsUtil::CvtToInt, &opt.level, sizeof(opt.level)},
	{"tolerance", VetsUtil::CvtToFloat, &opt.tolerance, sizeof(opt.tolerance)},
	{"noelev", VetsUtil::CvtToBoolean, &opt.noelev, sizeof(opt.noelev)},
	{"help", VetsUtil::CvtToBoolean, &opt.help, sizeof(opt.help)},
	{"debug", VetsUtil::CvtToBoolean, &opt.debug, sizeof(opt.debug)},
	{"quiet", VetsUtil::CvtToBoolean, &opt.quiet, sizeof(opt.quiet)},
	{NULL}
};

const char	*ProgName;

float DX = 1.0;
float DY = 1.0;

int CopyVariable(
	WRFReader *wrfreader,
	WaveletBlock3DBufWriter *wbwriter,
	int level,
	string varname,
	const size_t dim[3],
	size_t tsWRF,
	size_t tsVDC
) {

	static size_t sliceBufferSize = 0;
	static float *sliceBuffer = NULL;

	int rc;
	rc = wrfreader->OpenVariableRead(tsWRF, varname.c_str());
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to copy WRF variable %s at WRF time step %d",
			varname.c_str(), tsWRF
		);
		return (-1);
	}

	rc = wbwriter->OpenVariableWrite(tsVDC, varname.c_str(), level);
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to copy WRF variable %s at WRF time step %d",
			varname.c_str(), tsWRF
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
		if (wrfreader->ReadSlice(z, sliceBuffer) < 0 ) {
			MyBase::SetErrMsg(
				"Failed to copy WRF variable %s at WRF time step %d",
				varname.c_str(), tsWRF
			);
			return (-1);
		}
		if (wbwriter->WriteSlice(sliceBuffer) < 0) {
			MyBase::SetErrMsg(
				"Failed to copy WRF variable %s at WRF time step %d",
				varname.c_str(), tsWRF
			);
			return (-1);
		}
	}

	wrfreader->CloseVariable();
	wbwriter->CloseVariable();

	return(0);
}

int CopyVariable2D(
	WRFReader *wrfreader,
	WaveletBlock3DRegionWriter *wb2dwriter,
	int level,
	string varname,
	const size_t dim[3],
	size_t tsWRF,
	size_t tsVDC
) {

	static size_t sliceBufferSize = 0;
	static float *sliceBuffer = NULL;

	int rc;
	rc = wrfreader->OpenVariableRead(tsWRF, varname.c_str());
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to copy WRF variable %s at WRF time step %d",
			varname.c_str(), tsWRF
		);
		return (-1);
	}

	rc = wb2dwriter->OpenVariableWrite(tsVDC, varname.c_str(), level);
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to copy WRF variable %s at WRF time step %d",
			varname.c_str(), tsWRF
		);
		return (-1);
	}

	size_t slice_sz = dim[0] * dim[1];

	if (sliceBufferSize < slice_sz) {
		if (sliceBuffer ) delete [] sliceBuffer;
		sliceBuffer = new float[slice_sz];
		sliceBufferSize = slice_sz;
	}

	if (wrfreader->ReadSlice(0, sliceBuffer) < 0 ) {
		MyBase::SetErrMsg(
			"Failed to copy WRF variable %s at WRF time step %d",
			varname.c_str(), tsWRF
		);
		return (-1);
	}
	if (wb2dwriter->WriteRegion(sliceBuffer) < 0) {
		MyBase::SetErrMsg(
			"Failed to copy WRF variable %s at WRF time step %d",
			varname.c_str(), tsWRF
		);
		return (-1);
	}

	wrfreader->CloseVariable();
	wb2dwriter->CloseVariable();

	return(0);
}

void CalculateTheta(const float *t, float *theta, const size_t dim[3]) {
	for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ ) {
		theta[i] = t[i] + 300.0;
	}
}

void CalculateTK(const float *t, const float *p, const float *pb, float *tk, const size_t dim[3]) {

	for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ ) {
		tk[i] = (t[i] + 300.0) * pow( (float)(p[i] + pb[i]), 0.286f ) / pow( 100000.0f, 0.286f );
	}
}


void CalculateDeriv2D(const float *a, const float *b, float *c, const size_t dim[3]) {

	double dVdx, dUdy; // Holds derivatives used in calculation of curl
	for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ )
	{
		// On the boundaries of the domain, use forward or backward
		// finite differences
		if ( i % dim[0] == 0 ) // On left edge of domain
			dVdx = (b[i + 1] - b[i])/DX;
		else if ( (i + 1) % dim[0] == 0 ) // On right edge
			dVdx = (b[i] - b[i - 1])/DX;
		else // In the middle--use centered difference
			dVdx = (b[i + 1] - b[i - 1])/(2.0*DX);
		if ( i >= 0 && i < dim[0] ) // On the bottom edge
			dUdy = (a[i + dim[0]] - a[i])/DY;
		else if ( i >= dim[0]*(dim[1] - 1) ) // On top edge
			dUdy = (a[i] - a[i - dim[0]])/DY;
		else // In the middle
			dUdy = (a[i + dim[0]] - a[i - dim[0]])/(2.0*DY);
		// Calculate the vertical vorticity at that grid point
		c[i] = dVdx - dUdy;
	}
}

// mag == sqrt(a^2 + b^2)
//
void CalculateMag2D(const float *a, const float *b, float *c, const size_t dim[3]) {

	for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ ) {
		c[i] = sqrt((a[i]*a[i]) + (b[i]*b[i]));
	}
}

// mag == sqrt(a^2 + b^2 + c^2)
//
void CalculateMag3D(const float *a, const float *b, const float *c, float *d, const size_t dim[3]) {
	for ( size_t i = 0 ; i < dim[0]*dim[1] ; i++ ) {
		d[i] = sqrt((a[i]*a[i]) + (b[i]*b[i]) + (c[i]*c[i]));
	}
}

// Phnorm == (ph + phb) / phb
//
void CalculatePHNorm(const float *ph, const float *phb, float *norm, const size_t dim[3]) {

	for (size_t i=0; i < dim[0]*dim[1]; i++) {
		norm[i] = (ph[i] + phb[i]) / phb[i];
	}
}
// PNorm == p / pb + 1.0
//
void CalculatePNorm(const float *p, const float *pb, float *norm, const size_t dim[3]) {

	for (size_t i=0; i < dim[0]*dim[1]; i++) {
		norm[i] = p[i]/pb[i] + 1.0;
	}
}

void CalculatePFull(const float *p, const float *pb, float *norm, const size_t dim[3]) {

	for (size_t i=0; i < dim[0]*dim[1]; i++) {
		norm[i] = p[i] + pb[i];
	}
}

int DeriveVar1(
	WRFReader *wrfreader,
	WaveletBlock3DBufWriter *wbwriter,
	int level,
	string varwrfA,
	string varvdc,
	const size_t dim[3],
	size_t tsWRF,
	size_t tsVDC, 
	void (*calculate)(const float *, float *, const size_t *)
) {
	//
	// Static resources that never get freed :-(
	//
	static float *sliceBufferA = NULL;
	static float *sliceBufferB = NULL;
	static size_t sliceBufferSize = 0;

	int rc;
	rc = wrfreader->OpenVariableRead(tsWRF, varwrfA.c_str());
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to read WRF variable %s at WRF time step %d",
			varwrfA.c_str(), tsWRF
		);
		return (-1);
	}

	rc = wbwriter->OpenVariableWrite(tsVDC, varvdc.c_str(), level);
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to write VDC variable %s at VDC time step %d",
			varvdc.c_str(), tsVDC
		);
		return (-1);
	}

	size_t slice_sz = dim[0] * dim[1];

	if (sliceBufferSize < slice_sz) {
		if (sliceBufferA ) delete [] sliceBufferA;
		if (sliceBufferB ) delete [] sliceBufferB;
		sliceBufferA = new float[slice_sz];
		sliceBufferB = new float[slice_sz];
		sliceBufferSize = slice_sz;
	}

	for (size_t z = 0; z<dim[2]; z++) {
		if (wrfreader->ReadSlice(z, sliceBufferA) < 0 ) {
			MyBase::SetErrMsg(
				"Failed to read WRF variable %s at WRF time step %d",
				varwrfA.c_str(), tsWRF
			);
			return (-1);
		}

		calculate(sliceBufferA, sliceBufferB, dim);

		if (wbwriter->WriteSlice(sliceBufferB) < 0) {
			MyBase::SetErrMsg(
				"Failed to write VDC variable %s at VDC time step %d",
				varvdc.c_str(), tsVDC
			);
			return (-1);
		}
	}

	wrfreader->CloseVariable();
	wbwriter->CloseVariable();

	return(0);
}

int DeriveVar2(
	WRFReader *wrfreader,
	WaveletBlock3DBufWriter *wbwriter,
	int level,
	string varwrfA,
	string varwrfB,
	string varvdc,
	const size_t dim[3],
	size_t tsWRF,
	size_t tsVDC, 
	void (*calculate)(const float *, const float *, float *, const size_t *)
) {
	//
	// Static resources that never get freed :-(
	//
	static float *sliceBufferA = NULL;
	static float *sliceBufferB = NULL;
	static float *sliceBufferC = NULL;
	static size_t sliceBufferSize = 0;
	WRFReader *wrfreaderB = NULL; 

	wrfreaderB = new WRFReader ((const MetadataWRF &) *wrfreader);

	int rc;
	rc = wrfreader->OpenVariableRead(tsWRF, varwrfA.c_str());
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to read WRF variable %s at WRF time step %d",
			varwrfA.c_str(), tsWRF
		);
		return (-1);
	}

	rc = wrfreaderB->OpenVariableRead(tsWRF, varwrfB.c_str());
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to read WRF variable %s at WRF time step %d",
			varwrfB.c_str(), tsWRF
		);
		return (-1);
	}

	rc = wbwriter->OpenVariableWrite(tsVDC, varvdc.c_str(), level);
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to write VDC variable %s at VDC time step %d",
			varvdc.c_str(), tsVDC
		);
		return (-1);
	}

	size_t slice_sz = dim[0] * dim[1];

	if (sliceBufferSize < slice_sz) {
		if (sliceBufferA ) delete [] sliceBufferA;
		if (sliceBufferB ) delete [] sliceBufferB;
		if (sliceBufferC ) delete [] sliceBufferC;
		sliceBufferA = new float[slice_sz];
		sliceBufferB = new float[slice_sz];
		sliceBufferC = new float[slice_sz];
		sliceBufferSize = slice_sz;
	}

	for (size_t z = 0; z<dim[2]; z++) {
		if (wrfreader->ReadSlice(z, sliceBufferA) < 0 ) {
			MyBase::SetErrMsg(
				"Failed to read WRF variable %s at WRF time step %d",
				varwrfA.c_str(), tsWRF
			);
			return (-1);
		}

		if (wrfreaderB->ReadSlice(z, sliceBufferB) < 0 ) {
			MyBase::SetErrMsg(
				"Failed to read WRF variable %s at WRF time step %d",
				varwrfB.c_str(), tsWRF
			);
			return (-1);
		}

		calculate(sliceBufferA, sliceBufferB, sliceBufferC, dim);

		if (wbwriter->WriteSlice(sliceBufferC) < 0) {
			MyBase::SetErrMsg(
				"Failed to write VDC variable %s at VDC time step %d",
				varvdc.c_str(), tsVDC
			);
			return (-1);
		}
	}

	wrfreader->CloseVariable();
	wrfreaderB->CloseVariable();
	wbwriter->CloseVariable();

	delete wrfreaderB;

	return(0);
}

int DeriveVar3(
	WRFReader *wrfreader,
	WaveletBlock3DBufWriter *wbwriter,
	int level,
	string varwrfA,
	string varwrfB,
	string varwrfC,
	string varvdc,
	const size_t dim[3],
	size_t tsWRF,
	size_t tsVDC, 
	void (*calculate)(const float *, const float *, const float *, float *, const size_t *)
) {
	//
	// Static resources that never get freed :-(
	//
	static float *sliceBufferA = NULL;
	static float *sliceBufferB = NULL;
	static float *sliceBufferC = NULL;
	static float *sliceBufferD = NULL;
	static size_t sliceBufferSize = 0;
	WRFReader *wrfreaderB = NULL; 
	WRFReader *wrfreaderC = NULL; 

	wrfreaderB = new WRFReader ((const MetadataWRF &) *wrfreader);
	wrfreaderC = new WRFReader ((const MetadataWRF &) *wrfreader);

	int rc;
	rc = wrfreader->OpenVariableRead(tsWRF, varwrfA.c_str());
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to read WRF variable %s at WRF time step %d",
			varwrfA.c_str(), tsWRF
		);
		return (-1);
	}

	rc = wrfreaderB->OpenVariableRead(tsWRF, varwrfB.c_str());
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to read WRF variable %s at WRF time step %d",
			varwrfB.c_str(), tsWRF
		);
		return (-1);
	}
	rc = wrfreaderC->OpenVariableRead(tsWRF, varwrfC.c_str());
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to read WRF variable %s at WRF time step %d",
			varwrfC.c_str(), tsWRF
		);
		return (-1);
	}

	rc = wbwriter->OpenVariableWrite(tsVDC, varvdc.c_str(), level);
	if (rc<0) {
		MyBase::SetErrMsg(
			"Failed to write VDC variable %s at VDC time step %d",
			varvdc.c_str(), tsVDC
		);
		return (-1);
	}

	size_t slice_sz = dim[0] * dim[1];

	if (sliceBufferSize < slice_sz) {
		if (sliceBufferA ) delete [] sliceBufferA;
		if (sliceBufferB ) delete [] sliceBufferB;
		if (sliceBufferC ) delete [] sliceBufferC;
		if (sliceBufferD ) delete [] sliceBufferD;
		sliceBufferA = new float[slice_sz];
		sliceBufferB = new float[slice_sz];
		sliceBufferC = new float[slice_sz];
		sliceBufferD = new float[slice_sz];
		sliceBufferSize = slice_sz;
	}

	for (size_t z = 0; z<dim[2]; z++) {
		if (wrfreader->ReadSlice(z, sliceBufferA) < 0 ) {
			MyBase::SetErrMsg(
				"Failed to read WRF variable %s at WRF time step %d",
				varwrfA.c_str(), tsWRF
			);
			return (-1);
		}

		if (wrfreaderB->ReadSlice(z, sliceBufferB) < 0 ) {
			MyBase::SetErrMsg(
				"Failed to read WRF variable %s at WRF time step %d",
				varwrfB.c_str(), tsWRF
			);
			return (-1);
		}
		if (wrfreaderC->ReadSlice(z, sliceBufferC) < 0 ) {
			MyBase::SetErrMsg(
				"Failed to read WRF variable %s at WRF time step %d",
				varwrfC.c_str(), tsWRF
			);
			return (-1);
		}

		calculate(sliceBufferA, sliceBufferB, sliceBufferC, sliceBufferD, dim);

		if (wbwriter->WriteSlice(sliceBufferD) < 0) {
			MyBase::SetErrMsg(
				"Failed to write VDC variable %s at VDC time step %d",
				varvdc.c_str(), tsVDC
			);
			return (-1);
		}
	}

	wrfreader->CloseVariable();
	wrfreaderB->CloseVariable();
	wrfreaderC->CloseVariable();
	wbwriter->CloseVariable();

	delete wrfreaderB;
	delete wrfreaderC;

	return(0);
}

// Calculates needed and desired quantities involving PH and 
// PHB--elevation (needed for
// on-the-fly interpolation in Vapor) and normalized geopotential,
// and the original variables.

int DoGeopotStuff(
	WRFReader * wrfreader,
	WaveletBlock3DBufWriter *wbwriter,
	int level,
	const size_t dim[3],
	size_t tsWRF, 
	size_t tsVDC, 
	vector<string> &varnames
) {
	map <string, string>::const_iterator iter;

	int rc = 0;
	bool wantEle = find(varnames.begin(),varnames.end(),"ELEVATION")!=varnames.end();
	bool wantPh = find(varnames.begin(),varnames.end(),"PH")!=varnames.end();
	bool wantPhb = find(varnames.begin(),varnames.end(),"PHB")!=varnames.end();
	bool wantPhnorm = find(varnames.begin(),varnames.end(),"PHNorm_")!=varnames.end();

	if (! (wantEle || wantPh || wantPhb || wantPhnorm)) return(0);

	if (wantPh) {
		CopyVariable(
			wrfreader, wbwriter, level, "PH", dim, tsWRF, tsVDC
		);
		MyBase::SetErrCode(0);
		varnames.erase(find(varnames.begin(), varnames.end(), "PH"));
	}
	if (wantPhb) {
		CopyVariable(
			wrfreader, wbwriter, level, "PHB", dim, tsWRF, tsVDC
		);
		MyBase::SetErrCode(0);
		varnames.erase(find(varnames.begin(), varnames.end(), "PHB"));
	}
	if (wantEle) {
		CopyVariable(
			wrfreader, wbwriter, level, "ELEVATION", dim, tsWRF, tsVDC
		);
		MyBase::SetErrCode(0);
		varnames.erase(find(varnames.begin(), varnames.end(), "ELEVATION"));
	}

	//
	// Ugh. have to calcualte PHNorm if needed
	//
	if (wantPhnorm) {
		DeriveVar2(
			wrfreader, wbwriter, level, "PH", "PHB",
			"PHNorm_", dim, tsWRF, tsVDC, CalculatePHNorm
		);
		MyBase::SetErrCode(0);
		varnames.erase(find(varnames.begin(), varnames.end(), "PHNorm_"));
	}

	return(rc);
}

// Reads/calculates and outputs any of these quantities: U, V, W,
// 2D wind speed (U-V plane), 3D wind speed, and vertical vorticity (relative to
// a WRF level, i.e., not vertically interpolated to a Cartesian grid)
int DoWindStuff(
	WRFReader * wrfreader,
	WaveletBlock3DBufWriter *wbwriter,
	int level,
	const size_t dim[3],
	size_t tsWRF, 
	size_t tsVDC, 
	vector<string> &varnames
) {
	map <string, string>::const_iterator iter;

	int rc = 0;
	bool wantU = find(varnames.begin(),varnames.end(),"U")!=varnames.end();
	bool wantV = find(varnames.begin(),varnames.end(),"V")!=varnames.end();
	bool wantW = find(varnames.begin(),varnames.end(),"W")!=varnames.end();
	bool wantUV = find(varnames.begin(),varnames.end(),"UV_")!=varnames.end();
	bool wantUVW = find(varnames.begin(),varnames.end(),"UVW_")!=varnames.end();
	bool wantOmZ = find(varnames.begin(),varnames.end(),"omZ_")!=varnames.end();

	// Make sure we have work to do.

	if (! (wantU || wantV || wantW || wantUV || wantUVW || wantOmZ)) return(0);

	if (wantU) {
		CopyVariable(
			wrfreader, wbwriter, level, "U", dim, tsWRF, tsVDC
		);
		MyBase::SetErrCode(0);
		varnames.erase(find(varnames.begin(), varnames.end(), "U"));
	}
	if (wantV) {
		CopyVariable(
			wrfreader, wbwriter, level, "V", dim, tsWRF, tsVDC
		);
		MyBase::SetErrCode(0);
		varnames.erase(find(varnames.begin(), varnames.end(), "V"));
	}
	if (wantW) {
		CopyVariable(
			wrfreader, wbwriter, level, "W", dim, tsWRF, tsVDC
		);
		MyBase::SetErrCode(0);
		varnames.erase(find(varnames.begin(), varnames.end(), "W"));
	}


	if (wantUV) {
		DeriveVar2(
			wrfreader, wbwriter, level, "U", "V",
			"UV_", dim, tsWRF, tsVDC, CalculateMag2D
		);
		MyBase::SetErrCode(0);
		varnames.erase(find(varnames.begin(), varnames.end(), "UV_"));
	}

	if (wantUVW) {
		DeriveVar3(
			wrfreader, wbwriter, level, "U", "V", "W",
			"UVW_", dim, tsWRF, tsVDC, CalculateMag3D
		);
		MyBase::SetErrCode(0);
		varnames.erase(find(varnames.begin(), varnames.end(), "UVW_"));
	}

	if (wantOmZ) {

		DeriveVar2(
			wrfreader, wbwriter, level, "U", "V",
			"omZ_", dim, tsWRF, tsVDC, CalculateDeriv2D
		);
		MyBase::SetErrCode(0);
		varnames.erase(find(varnames.begin(), varnames.end(), "omZ_"));
	}

	return(rc);
}


// Read/calculates and outputs some or all of P, PB, PFull_, PNorm_,
// T, Theta_, and TK_.  Sets flags so that variables are not read twice.
int DoPTStuff(
	WRFReader * wrfreader,
	WaveletBlock3DBufWriter *wbwriter,
	int level,
	const size_t dim[3],
	size_t tsWRF, 
	size_t tsVDC, 
	vector<string> &varnames
) {
	int rc = 0;

	map <string, string>::const_iterator iter;

	bool wantP = find(varnames.begin(),varnames.end(),"P")!=varnames.end();
	bool wantPb = find(varnames.begin(),varnames.end(),"PB")!=varnames.end();
	bool wantT = find(varnames.begin(),varnames.end(),"T")!=varnames.end();
	bool wantPfull = find(varnames.begin(),varnames.end(),"PFull_")!=varnames.end();
	bool wantPnorm = find(varnames.begin(),varnames.end(),"PNorm_")!=varnames.end();
	bool wantTheta = find(varnames.begin(),varnames.end(),"Theta_")!=varnames.end();
	bool wantTk = find(varnames.begin(),varnames.end(),"TK_")!=varnames.end();

	// Make sure we have work to do.

	if (! (wantP || wantPb || wantT || wantPfull || wantPnorm || wantTheta || wantTk)) return(0);

	if (wantP) {
		CopyVariable(
			wrfreader, wbwriter, level, "P", dim, tsWRF, tsVDC
		);
		MyBase::SetErrCode(0);
		varnames.erase(find(varnames.begin(), varnames.end(), "P"));
	}

	if (wantPb) {
		CopyVariable(
			wrfreader, wbwriter, level, "PB", dim, tsWRF, tsVDC
		);
		MyBase::SetErrCode(0);
		varnames.erase(find(varnames.begin(), varnames.end(), "PB"));
	}

	if (wantT) {
		CopyVariable(
			wrfreader, wbwriter, level, "T", dim, tsWRF, tsVDC
		);
		MyBase::SetErrCode(0);
		varnames.erase(find(varnames.begin(), varnames.end(), "T"));
	}

	if ( wantTheta ) {
		DeriveVar1(
			wrfreader, wbwriter, level, "T", 
			"Theta_", dim, tsWRF, tsVDC, CalculateTheta
		);
		MyBase::SetErrCode(0);
		varnames.erase(find(varnames.begin(), varnames.end(), "Theta_"));
	}

	if ( wantPnorm ) {
		DeriveVar2(
			wrfreader, wbwriter, level, "P", "PB",
			"PNorm_", dim, tsWRF, tsVDC, CalculatePNorm
		);
		MyBase::SetErrCode(0);
		varnames.erase(find(varnames.begin(), varnames.end(), "PNorm_"));
	}

	if ( wantPfull ) {
		DeriveVar2(
			wrfreader, wbwriter, level, "P", "PB",
			"PFull_", dim, tsWRF, tsVDC, CalculatePFull
		);
		MyBase::SetErrCode(0);
		varnames.erase(find(varnames.begin(), varnames.end(), "PFull_"));
	}

	if ( wantTk ) {	
		DeriveVar3(
			wrfreader, wbwriter, level, "T", "P", "PB",
			"TK_", dim, tsWRF, tsVDC, CalculateTK
		);
		MyBase::SetErrCode(0);
		varnames.erase(find(varnames.begin(), varnames.end(), "TK_"));
	}
	return(rc);
}

int DoIndependentVars3d(
	WRFReader * wrfreader,
	WaveletBlock3DBufWriter *wbwriter,
	int level,
	const size_t dim[3],
	size_t tsWRF, 
	size_t tsVDC, 
	vector<string> &varnames
) {

	int rc = 0;

	vector <string> vn_copy = varnames;
	for (int i=0; i<vn_copy.size(); i++) {
		if (wrfreader->GetVarType(vn_copy[i]) == Metadata::VAR3D) {
			CopyVariable(
				wrfreader, wbwriter, level, vn_copy[i], dim, tsWRF, tsVDC
			);
			MyBase::SetErrCode(0);
			varnames.erase(find(varnames.begin(), varnames.end(), vn_copy[i]));
		}
	}

	return(rc);
}

int DoIndependentVars2d(
	WRFReader * wrfreader,
	WaveletBlock3DRegionWriter *wb2dwriter,
	int level,
	const size_t dim[3],
	size_t tsWRF, 
	size_t tsVDC, 
	vector<string> &varnames
) {

	int rc = 0;

	size_t dim2d[] = {dim[0], dim[1], 1};

	vector <string> vn_copy = varnames;
	for (int i=0; i<vn_copy.size(); i++) {
		if (wrfreader->GetVarType(vn_copy[i]) == Metadata::VAR2D_XY) {
			CopyVariable2D(
				wrfreader,wb2dwriter,level, vn_copy[i], dim2d, tsWRF, tsVDC
			);
			MyBase::SetErrCode(0);
			varnames.erase(find(varnames.begin(), varnames.end(), vn_copy[i]));
		}
	}

	return(rc);
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
	vector <string> &copy_vars,
	bool noelev
) {
	map <string, string>::const_iterator iter;
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
				(find(wrf_vars.begin(),wrf_vars.end(),"PH")!=wrf_vars.end()) &&
				(find(wrf_vars.begin(),wrf_vars.end(),"PHB") !=wrf_vars.end())&&
				(find(copy_vars.begin(),copy_vars.end(),v)==copy_vars.end())
			) {
				copy_vars.push_back(v);
			}
		}
		else if (v.compare("PHNorm_")==0) {
			if (
				(find(wrf_vars.begin(),wrf_vars.end(),"PHB")!=wrf_vars.end()) &&
				(find(copy_vars.begin(),copy_vars.end(),v)==copy_vars.end())
			) {
				copy_vars.push_back(v);
			}
		}
		else if ((v.compare("PFull_")==0) || (v.compare("PNorm_")==0)) {
			if (
				(find(wrf_vars.begin(),wrf_vars.end(),"P")!=wrf_vars.end()) &&
				(find(wrf_vars.begin(),wrf_vars.end(),"PB")!=wrf_vars.end()) &&
				(find(copy_vars.begin(),copy_vars.end(),v)==copy_vars.end())
			) {
				copy_vars.push_back(v);
			}
		}
		else if (v.compare("Theta_")==0) {
			if (
				(find(wrf_vars.begin(),wrf_vars.end(),"T")!=wrf_vars.end()) &&
				(find(copy_vars.begin(),copy_vars.end(),v)==copy_vars.end())
			) {
				copy_vars.push_back(v);
			}
		}
		else if (v.compare("TK_")==0) {
			if (
				(find(wrf_vars.begin(),wrf_vars.end(),"P")!=wrf_vars.end()) &&
				(find(wrf_vars.begin(),wrf_vars.end(),"PB")!=wrf_vars.end()) &&
				(find(wrf_vars.begin(),wrf_vars.end(),"T")!=wrf_vars.end()) &&
				(find(copy_vars.begin(),copy_vars.end(),v)==copy_vars.end())
			) {
				copy_vars.push_back(v);
			}
		}
		else if (v.compare("UV_")==0) {
			if (
				(find(wrf_vars.begin(),wrf_vars.end(),"U")!=wrf_vars.end()) &&
				(find(wrf_vars.begin(),wrf_vars.end(),"V")!=wrf_vars.end()) &&
				(find(copy_vars.begin(),copy_vars.end(),v)==copy_vars.end())
			) {
				copy_vars.push_back(v);
			}
		}
		else if (v.compare("UVW_")==0) {
			if (
				(find(wrf_vars.begin(),wrf_vars.end(),"U")!=wrf_vars.end()) &&
				(find(wrf_vars.begin(),wrf_vars.end(),"V")!=wrf_vars.end()) &&
				(find(wrf_vars.begin(),wrf_vars.end(),"W")!=wrf_vars.end()) &&
				(find(copy_vars.begin(),copy_vars.end(),v)==copy_vars.end())
			) {
				copy_vars.push_back(v);
			}
		}
		else if (v.compare("omZ_")==0) {
			if (
				(find(wrf_vars.begin(),wrf_vars.end(),"U")!=wrf_vars.end()) &&
				(find(wrf_vars.begin(),wrf_vars.end(),"V")!=wrf_vars.end()) &&
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

	

void ErrMsgCBHandler(const char *msg, int) {
	cerr << ProgName << " : " << msg << endl;
}

bool file_ok(
	const MetadataWRF *metadataWRF, 
	const MetadataVDC *metadataVDC
) {
	size_t dimWRF[3], dimVDC[3];

	metadataWRF->GetDim(dimWRF, -1);
	metadataVDC->GetDim(dimVDC, -1);
	for (int i=0; i<3; i++) {
		if (dimWRF[i] != dimVDC[i]) {
			MyBase::SetErrMsg("Dimension mismatch");
			return(false);
		}
	}

	vector <double> extentsWRF = metadataWRF->GetExtents();
	vector <double> extentsVDC = metadataVDC->GetExtents();

	if ((extentsWRF.size() != 6) || (extentsVDC.size() != 6)) {
		MyBase::SetErrMsg("Incomplete user extents");
		return(false);
	}

	// Check vertical extents
	//
	if (extentsWRF[0] < (extentsVDC[0]-0.0001)) {
		MyBase::SetErrMsg("Minimum vertical user extents out of range");
		return(false);
	}
	if (extentsWRF[5] < (extentsVDC[5]-0.0001)) {
		MyBase::SetErrMsg("Maximum vertical user extents out of range");
		return(false);
	}

	for (int i=0; i<2; i++) {
		if (extentsWRF[i] < (extentsVDC[i]-0.0001)) {
			MyBase::SetErrMsg("Minimum horizontal user extents out of range");
			return(false);
		}
	}
	for (int i=3; i<5; i++) {
		if (extentsWRF[i] > (extentsVDC[i]+0.0001)) {
			MyBase::SetErrMsg("Maximum horizontal user extents out of range");
			return(false);
		}
	}
	return(true);
}

bool map_VDF2WRF_time(
	MetadataWRF *metadataWRF, 
	size_t tsWRF, 
	MetadataVDC *metadataVDC,
	size_t *tsVDC,
	double tolerance
) {
	double userTimeWRF = metadataWRF->GetTSUserTime(tsWRF);

	for (size_t ts = 0; ts<metadataVDC->GetNumTimeSteps(); ts++) {
		double userTimeVDC = metadataVDC->GetTSUserTime(ts);
		if (abs((userTimeWRF - userTimeVDC) / userTimeVDC) < tolerance) {
			*tsVDC = ts;
			return(true);
		}
	}
	return(false);
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
		cerr << "Usage: " << ProgName << " [options] metafile wrffiles..." << endl << endl;
		op.PrintOptionHelp(stderr);
		exit(0);
	}

	if (argc < 3) {
		cerr << "Usage: " << ProgName << " [options] metafile wrffiles..." << endl;
		op.PrintOptionHelp(stderr);
		exit(1);
	}

	if (opt.debug) MyBase::SetDiagMsgFilePtr(stderr);

	argv++;
	argc--;

	string metafile = *argv;
	MetadataVDC	*metadataVDC = new MetadataVDC(metafile);
	if (MetadataVDC::GetErrCode() != 0) { 
		MyBase::SetErrMsg("Error processing VDC metafile : %s",metafile.c_str());
		exit(1);
	}

	WaveletBlock3DBufWriter *wbwriter = new WaveletBlock3DBufWriter(*metadataVDC);
	if (WaveletBlock3DBufWriter::GetErrCode() != 0) { 
		MyBase::SetErrMsg("Error processing VDC metafile : %s",metafile.c_str());
		exit(1);
	}

	//
	// Ugh! WaveletBlock3DBufWriter class does not handle 2D data!!!
	//
	WaveletBlock3DRegionWriter *wb2dwriter = new WaveletBlock3DRegionWriter(*metadataVDC);
	if (WaveletBlock3DRegionWriter::GetErrCode() != 0) { 
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
	// Deal with required variables that may have atypical names
	//
	map <string, string> atypnames;
    string tag("DependentVarNames");
    string s = metadataVDC->GetUserDataString(tag);
	if (! s.empty()) {
		vector <string> svec;
		CvtToStrVec(s.c_str(), &svec);
		if (svec.size() != 8) {
			MyBase::SetErrMsg("Invalid DependentVarNames tag: %s",tag.c_str());
			return(-1);
		}
		atypnames["U"] = svec[0];
		atypnames["V"] = svec[1];
		atypnames["W"] = svec[2];
		atypnames["PH"] = svec[3];
		atypnames["PHB"] = svec[4];
		atypnames["P"] = svec[5];
		atypnames["PB"] = svec[6];
		atypnames["T"] = svec[7];
	}

	size_t TotalTimeSteps = 0;
	bool done = false;
	for (int arg = 0; arg<argc && ! done; arg++) {
		vector <string> wrf_file;
		wrf_file.push_back(argv[arg]);


		MetadataWRF	*metadataWRF = new MetadataWRF(wrf_file, atypnames);
		if ((MetadataWRF::GetErrCode()!=0)||(metadataWRF->GetNumTimeSteps()==0)){ 
			MyBase::SetErrMsg(
				"Error processing WRF file %s, skipping", wrf_file[0].c_str()
			);
			MyBase::SetErrCode(0);
			estatus++;
			continue;
		}

		//
		// Make sure WRF and VDC metadata agree
		//
		if (! file_ok(metadataWRF, metadataVDC)) {
			MyBase::SetErrMsg(
				"Error processing WRF file %s, skipping", wrf_file[0].c_str()
			);
			MyBase::SetErrCode(0);
			estatus++;
			continue;
		} 

		WRFReader *wrfreader = new WRFReader(*metadataWRF);
		if (WRFReader::GetErrCode() != 0) { 
			MyBase::SetErrMsg(
				"Error processing WRF file %s, skipping", wrf_file[0].c_str()
			);
			MyBase::SetErrCode(0);
			estatus++;
			continue;
		}

		vector <pair <string, double> > ga = wrfreader->GetGlobalAttributes();
		for (int i=0; i<ga.size(); i++) {
			if (ga[i].first.compare("DX") == 0) DX = ga[i].second;
			if (ga[i].first.compare("DY") == 0) DY = ga[i].second;
		}

		vector <string> varsWRF = metadataWRF->GetVariableNames();

		vector <string> copy_vars;
		SelectVariables(
			varsVDC, varsWRF, opt.varnames, copy_vars,
			(opt.noelev != 0)
		);

		if (! opt.quiet) {
			cout << "Processing file : " << wrf_file[0].c_str() << endl;
			cout << "\tTransforming 3D variables : ";
			for (int i=0; i<copy_vars.size(); i++) {
				if (metadataVDC->GetVarType(copy_vars[i]) == Metadata::VAR3D) {
					cout << copy_vars[i] << " ";
				}
			}
			cout << endl;
			cout << endl;

			cout << "\tTransforming 2D variables : ";
			for (int i=0; i<copy_vars.size(); i++) {
				if (metadataVDC->GetVarType(copy_vars[i]) != Metadata::VAR3D) {
					cout << copy_vars[i] << " ";
				}
			}
			cout << endl;
		}

		size_t numTimeStepsWRF = metadataWRF->GetNumTimeSteps();
		for (size_t tsWRF=0; tsWRF<numTimeStepsWRF && ! done ; tsWRF++) {


			size_t tsVDC;
			if (!map_VDF2WRF_time(metadataWRF, tsWRF, metadataVDC, &tsVDC, opt.tolerance)) {
				MyBase::SetErrMsg(
					"Error processing WRF file %s, at time step %d : no matching VDC user time, skipping", 
					wrf_file[0].c_str(), tsWRF
				);
				MyBase::SetErrCode(0);
				estatus++;
				continue;
			}

			if (! opt.quiet) {
				cout << endl << "\tTime step : " << tsVDC << " (VDC), " 
				<< tsWRF << " (WRF)" << endl;
			}

			vector <string> wrk_vars = copy_vars;

			DoGeopotStuff(
				wrfreader, wbwriter, opt.level,dimsVDC, tsWRF, tsVDC,
				wrk_vars
			);

			// Find wind speeds, if necessary
			DoWindStuff( 
				wrfreader, wbwriter, opt.level,dimsVDC, tsWRF, tsVDC,
				wrk_vars
			);

			DoPTStuff( 
				wrfreader, wbwriter, opt.level,dimsVDC, tsWRF, tsVDC,
				wrk_vars
			);

			// Remaining 3D variables
			DoIndependentVars3d(
				wrfreader, wbwriter, opt.level,dimsVDC, tsWRF, tsVDC,
				wrk_vars
			);

			// Remaining 2D variables
			DoIndependentVars2d(
				wrfreader, wb2dwriter, opt.level,dimsVDC, tsWRF, tsVDC,
				wrk_vars
			);

			assert(wrk_vars.size() == 0);
			TotalTimeSteps++;
			if (opt.numts > 0 && TotalTimeSteps >= opt.numts) {
				done = true;
			}
		}
	}

	if (! opt.quiet) {
		cout << endl;
		cout << "Transformed " << TotalTimeSteps << " time steps" << endl;
	}

	exit(estatus);
}
