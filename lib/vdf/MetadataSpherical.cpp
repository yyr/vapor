//
//      $Id$
//
//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		MetadataSpherical.cpp
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Fri Jun 29 15:15:24 MDT 2007
//
//	Description:	Implements the MetadataSpherical class
//


#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <cstring>
#include <vapor/MetadataSpherical.h>
#include "assert.h"

using namespace VAPoR;
using namespace VetsUtil;


int MetadataSpherical::SetDefaults() {



	// Set some default metadata attributes. 
	//
    string gridTypeStr = "regular";
    SetGridType(gridTypeStr);

    string coordSystemType = "spherical";
    SetCoordSystemType(coordSystemType);

	// Compute default extents and periodicity of boundary
	//
	vector<double> extentsVec(6,0.0);
	vector <long> periodic_boundary(3,0);
	const vector <long> rvec = GetGridPermutation();
	const size_t *dim = GetDimension();
	for (int i=0; i<3; i++) {
		switch (rvec[i]) {
		case 0: {	// longitude
			double incr = 360.0 / (double) dim[i];
			extentsVec[i] = -180 + (incr / 2.0);
			extentsVec[i+3] = 180 - (incr / 2.0);
		periodic_boundary[i] = 1;
		}
		break;
		case 1: {	// lattitude
			double incr = 180.0 / (double) dim[i];
			extentsVec[i] = -90 + (incr / 2.0);
			extentsVec[i+3] = 90 - (incr / 2.0);
		periodic_boundary[i] = 1;
		}
		break;
		case 2: {	// radius
			extentsVec[i] = 0.0;
			extentsVec[i+3] = 0.5;
			periodic_boundary[i] = 0;
		}
		break;
		default:
			assert(rvec[i] > 2);
		break;
		}
	}

	SetExtents(extentsVec);
	SetPeriodicBoundary(periodic_boundary);


	return(0);
}

MetadataSpherical::MetadataSpherical(
		const size_t dim[3], size_t numTransforms, size_t bs[3], 
		size_t permutation[3],
		int nFilterCoef, int nLiftingCoef, int msbFirst, int vdfVersion
) : MetadataVDC(
	dim, numTransforms, bs, nFilterCoef, nLiftingCoef, msbFirst, vdfVersion
) {

	vector <long> v;
	for(int i=0; i<3; i++) {
		v.push_back((long) permutation[i]);
	}
	SetGridPermutation(v);

	SetDefaults();
}

MetadataSpherical::MetadataSpherical(
	const string &path
) : MetadataVDC(path) {

}

MetadataSpherical::~MetadataSpherical() {
}
