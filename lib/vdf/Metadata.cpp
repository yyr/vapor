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
//	File:		Metadata.cpp
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Thu Sep 30 12:13:03 MDT 2004
//
//	Description:	Implements the Metadata class
//


#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <cstring>
#include <expat.h>
#include <cassert>
#include <algorithm>
#include <vapor/Metadata.h>
#include <vapor/CFuncs.h>

using namespace VAPoR;
using namespace VetsUtil;

vector <string> Metadata::GetVariableNames() const {
	vector <string> svec1, svec2;
	svec1 = GetVariables3D();
	for(int i=0; i<svec1.size(); i++) svec2.push_back(svec1[i]);

	svec1 = GetVariables2DXY();
	for(int i=0; i<svec1.size(); i++) svec2.push_back(svec1[i]);
	svec1 = GetVariables2DXZ();
	for(int i=0; i<svec1.size(); i++) svec2.push_back(svec1[i]);
	svec1 = GetVariables2DYZ();
	for(int i=0; i<svec1.size(); i++) svec2.push_back(svec1[i]);

	return(svec2);
};


void Metadata::GetDim(
	size_t dim[3], int reflevel
) const {

    if (reflevel < 0 || reflevel > GetNumTransforms()) {
		reflevel = GetNumTransforms();
	}

    int  ldelta = GetNumTransforms() - reflevel;

    const size_t *maxdim = GetDimension();

    for (int i=0; i<3; i++) {
        dim[i] = maxdim[i] >> ldelta;

        // Deal with odd dimensions
		if ((dim[i] << ldelta) < maxdim[i]) dim[i]++;
    }
}

void    Metadata::GetDimBlk(
    size_t bdim[3], int reflevel
) const {
    size_t dim[3];

    Metadata::GetDim(dim, reflevel);

	size_t bs[3];
	GetBlockSize(bs, reflevel);
    for (int i=0; i<3; i++) {
        bdim[i] = (size_t) ceil ((double) dim[i] / (double) bs[i]);
    }
}

void    Metadata::MapVoxToBlk(
	const size_t vcoord[3], size_t bcoord[3]
) const {
	const size_t *bs = GetBlockSize();

	for (int i=0; i<3; i++) {
		bcoord[i] = vcoord[i] / bs[i];
	}
}
 
void	Metadata::MapVoxToUser(
    size_t timestep, 
	const size_t vcoord0[3], double vcoord1[3],
	int	reflevel
) const {

    if (reflevel < 0 || reflevel > GetNumTransforms()) {
		reflevel = GetNumTransforms();
	}
    int  ldelta = GetNumTransforms() - reflevel;

	size_t	dim[3];

	vector <double> extents = GetTSExtents(timestep);
	if (! extents.size()) extents = GetExtents();

	Metadata::GetDim(dim, -1);	// finest dimension
	for(int i = 0; i<3; i++) {

		// distance between voxels along dimension 'i' in user coords
		double deltax = (extents[i+3] - extents[i]) / (dim[i] - 1);

		// coordinate of first voxel in user space
		double x0 = extents[i];

		// Boundary shrinks and step size increases with each transform
		for(int j=0; j<(int)ldelta; j++) {
			x0 += 0.5 * deltax;
			deltax *= 2.0;
		}
		vcoord1[i] = x0 + (vcoord0[i] * deltax);
	}
}

void	Metadata::MapUserToVox(
    size_t timestep, const double vcoord0[3], size_t vcoord1[3],
	int	reflevel
) const {

    if (reflevel < 0 || reflevel > GetNumTransforms()) {
		reflevel = GetNumTransforms();
	}
    int  ldelta = GetNumTransforms() - reflevel;

	size_t	dim[3];
	vector <double> extents = GetTSExtents(timestep);
	if (! extents.size()) extents = GetExtents();

	vector <double> lextents = extents;
	const size_t *maxdim = GetDimension();


	Metadata::GetDim(dim, reflevel);	
	for(int i = 0; i<3; i++) {
		double a;

		// distance between voxels along dimension 'i' in user coords
		double deltax = (lextents[i+3] - lextents[i]) / (maxdim[i] - 1);

		// coordinate of first voxel in user space
		double x0 = lextents[i];

		// Boundary shrinks and step size increases with each transform
		for(int j=0; j<(int)ldelta; j++) {
			x0 += 0.5 * deltax;
			deltax *= 2.0;
		}
		lextents[i] = x0;
		lextents[i+3] = lextents[i] + (deltax * (dim[i]-1));

		a = (vcoord0[i] - lextents[i]) / (lextents[i+3]-lextents[i]);
		vcoord1[i] = (size_t) rint(a * (double) (dim[i]-1));

		if (vcoord1[i] > (dim[i]-1)) vcoord1[i] = dim[i]-1;
	}
}

VAPoR::Metadata::VarType_T Metadata::GetVarType(
	const string &varname
) const {

    {
        const vector <string> &vars = GetVariables3D();
        for (int i=0; i<vars.size(); i++ ) {
            if (vars[i].compare(varname) == 0) return(VAR3D);
        }
    }
    {
        const vector <string> &vars = GetVariables2DXY();
        for (int i=0; i<vars.size(); i++ ) {
            if (vars[i].compare(varname) == 0) return(VAR2D_XY);
        }
    }
    {
        const vector <string> &vars = GetVariables2DXZ();
        for (int i=0; i<vars.size(); i++ ) {
            if (vars[i].compare(varname) == 0) return(VAR2D_XZ);
        }
    }
    {
        const vector <string> &vars = GetVariables2DYZ();
        for (int i=0; i<vars.size(); i++ ) {
            if (vars[i].compare(varname) == 0) return(VAR2D_YZ);
        }
    }
    return(VARUNKNOWN);
}

int	Metadata::IsValidRegion(
	const size_t min[3],
	const size_t max[3],
	int reflevel
) const {
	size_t dim[3];

	Metadata::GetDim(dim, reflevel);

	for(int i=0; i<3; i++) {
		if (min[i] > max[i]) return (0);
		if (max[i] >= dim[i]) return (0);
	}
	return(1);

}

int	Metadata::IsValidRegionBlk(
	const size_t min[3],
	const size_t max[3],
	int reflevel
) const {
	size_t dim[3];

	Metadata::GetDimBlk(dim, reflevel);

	for(int i=0; i<3; i++) {
		if (min[i] > max[i]) return (0);
		if (max[i] >= dim[i]) return (0);
	}
	return(1);
}
