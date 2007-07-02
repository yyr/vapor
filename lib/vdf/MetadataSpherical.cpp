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

using namespace VAPoR;
using namespace VetsUtil;

// Static member initialization
//
const string MetadataSpherical::_gridPermutationTag = "gridPermutation";


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
	for (int i=0; i<3; i++) {
		switch (_permutation[i]) {
		case 0: {	// longitude
			double incr = 360.0 / (double)_dim[i];
			extentsVec[i] = 0 + (incr / 2.0);
			extentsVec[i+3] = 360 - (incr / 2.0);
		periodic_boundary[i] = 1;
		}
		break;
		case 1: {	// lattitude
			double incr = 180.0 / (double)_dim[i];
			extentsVec[i] = 0 + (incr / 2.0);
			extentsVec[i+3] = 180 - (incr / 2.0);
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
			assert(_permutation[i] > 2);
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
) : Metadata(
	dim, numTransforms, bs, nFilterCoef, nLiftingCoef, msbFirst, vdfVersion
) {

	vector <long> v;
	for(int i=0; i<3; i++) {
		_permutation[i] = permutation[i];
		v.push_back((long) permutation[i]);
	}
	_rootnode->SetElementLong(_gridPermutationTag, v);

	SetDefaults();
}

MetadataSpherical::MetadataSpherical(
	const string &path
) : Metadata(path) {

	const vector <long> rvec = _rootnode->GetElementLong(_gridPermutationTag);
	for(int i=0; i<3; i++) {
		_permutation[i] = rvec[i];
	}
}

MetadataSpherical::~MetadataSpherical() {
}
