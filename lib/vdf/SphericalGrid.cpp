#include <iostream>
#include <cassert>
#ifdef _WINDOWS
#define _USE_MATH_DEFINES
#pragma warning(disable : 4251 4100)
#endif
#include <cmath>
#include "vapor/SphericalGrid.h"

using namespace std;
using namespace VAPoR;

SphericalGrid::SphericalGrid(
	const size_t bs[3],
	const size_t min[3],
	const size_t max[3],
	const double extents[6],
	const size_t permutation[3],
	const bool periodic[3],
	float ** blks
 ) : RegularGrid(bs,min,max,extents,periodic,blks) {

	for (int i=0; i<3; i++) {
		_permutation.push_back(permutation[i]);
	}

	double extentsC[6];
	_GetUserExtents(extentsC);
	RegularGrid::_SetExtents(extentsC);
}

SphericalGrid::SphericalGrid(
	const size_t bs[3],
	const size_t min[3],
	const size_t max[3],
	const double extents[6],
	const size_t permutation[3],
	const bool periodic[3],
	float ** blks,
	float missing_value
 ) : RegularGrid(bs,min,max,extents,periodic,blks, missing_value) {

	for (int i=0; i<3; i++) {
		_permutation.push_back(permutation[i]);
	}

	double extentsC[6];
	_GetUserExtents(extentsC);
	RegularGrid::_SetExtents(extentsC);
}


float SphericalGrid::GetValue(double x, double y, double z) const {

	if (! InsideGrid(x,y,z)) return(GetMissingValue());

	cerr << "SphericalGrid::GetValue - NOT IMPLEMENTED" << endl;
	return(GetMissingValue());
}

void SphericalGrid::_GetUserExtents(double extentsC[6]) const {

	// Extents in spherical coords
	//
	double extentsS[6];
	RegularGrid::GetUserExtents(extentsS);

	//
	// Extents in spherical coords, permuted to order: lon, lat, radius
	//
	double extentsSP[6];
	_permute(_permutation, extentsSP, extentsS[0], extentsS[1], extentsS[2]);
	_permute(_permutation, extentsSP+3, extentsS[3], extentsS[4], extentsS[5]);

	size_t dims[3];
	GetDimensions(dims);
	double dimsP[3];
	_permute(_permutation, dimsP, dims[0], dims[1], dims[2]);

	double lon0 = extentsSP[0] * M_PI / 180.0;
	double lat0 = extentsSP[1] * M_PI / 180.0;
	//double r0 = extentsSP[2];
	double lon1 = extentsSP[3] * M_PI / 180.0;
	double lat1 = extentsSP[4] * M_PI / 180.0;
	double r1 = extentsSP[5];

	double delta_phi = (lon1-lon0) / dimsP[0];
	double delta_theta = (lat1-lat0) / dimsP[1];

	//
	// Compute extents in cartesian coordinates
	//
	double phi;
	double theta;
	double x,y,z;
	for (int j=0; j<dimsP[1]; j++) {
	for (int i=0; i<dimsP[0]; i++) {
		phi = lon0 + (i*delta_phi);
		theta = lat0 + (j*delta_theta);

		x = r1 * cos(phi) * sin(theta);	
		y = r1 * sin(phi) * sin(theta);	
		z = r1 * sin(theta);
		if (i==0 && j==0) {
			extentsC[0] = extentsC[3] = x;
			extentsC[1] = extentsC[4] = y;
			extentsC[2] = extentsC[5] = z;
		}
		if (x < extentsC[0]) extentsC[0] = x;
		if (y < extentsC[1]) extentsC[1] = y;
		if (z < extentsC[2]) extentsC[2] = z;
		if (x > extentsC[3]) extentsC[3] = x;
		if (y > extentsC[4]) extentsC[4] = y;
		if (z > extentsC[5]) extentsC[5] = z;
	}
	}
}

int SphericalGrid::GetUserCoordinates(
	size_t i, size_t j, size_t k, 
	double *x, double *y, double *z
) const {

	size_t min[3], max[3];

    if (i>_max[0]-_min[0]) return(-1);
    if (j>_max[1]-_min[1]) return(-1);
    if (k>_max[2]-_min[2]) return(-1);

	double extentsS[6];
	RegularGrid::GetUserExtents(extentsS);

	double extentsSP[6];
	_permute(_permutation, extentsSP, extentsS[0], extentsS[1], extentsS[2]);
	_permute(_permutation, extentsSP+3, extentsS[3], extentsS[4], extentsS[5]);

	size_t dims[3];
	GetDimensions(dims);
	double dimsP[3];
	_permute(_permutation, dimsP, dims[0], dims[1], dims[2]);

	double lon0 = extentsSP[0] * M_PI / 180.0;
	double lat0 = extentsSP[1] * M_PI / 180.0;
	double r0 = extentsSP[2];
	double lon1 = extentsSP[3] * M_PI / 180.0;
	double lat1 = extentsSP[4] * M_PI / 180.0;
	double r1 = extentsSP[5];

	double delta_phi = (lon1-lon0) / dimsP[0];
	double delta_theta = (lat1-lat0) / dimsP[1];
	double delta_r = (r1-r0) / dimsP[2];

	double ijkP[6];
	_permute(_permutation, ijkP, i, j, k);

	double phi = lon0 + (ijkP[0] * delta_phi);
	double theta = lat0 + (ijkP[1] * delta_theta);
	double r = r0 + (ijkP[2] * delta_r);

	*x = r1 * cos(phi) * sin(theta);	
	*y = r1 * sin(phi) * sin(theta);	
	*z = r1 * sin(theta);

	return(0);

}

void SphericalGrid::GetIJKIndex(
	double x, double y, double z,
	size_t *i, size_t *j, size_t *k
) const {
	cerr << "SphericalGrid::GetValue - NOT IMPLEMENTED" << endl;
	return;
}

void SphericalGrid::GetIJKIndexFloor(
	double x, double y, double z,
	size_t *i, size_t *j, size_t *k
) const {
	cerr << "SphericalGrid::GetValue - NOT IMPLEMENTED" << endl;
	return;

}

int SphericalGrid::Reshape(
	const size_t min[3],
	const size_t max[3],
	const bool periodic[3]
) {
	int rc = RegularGrid::Reshape(min,max,periodic);
	if (rc<0) return(-1);

	double extentsC[6];
	_GetUserExtents(extentsC);
	RegularGrid::_SetExtents(extentsC);

	return(0);
}

bool SphericalGrid::InsideGrid(double x, double y, double z) const {

	double extentsC[6];
	RegularGrid::GetUserExtents(extentsC);


	// Do a quick check to see if the point is completely outside of 
	// the grid bounds.
	//
	if (extentsC[0] < extentsC[3]) {
		if (x<extentsC[0] || x>extentsC[3]) return (false);
	}
	else {
		if (x>extentsC[0] || x<extentsC[3]) return (false);
	}
	if (extentsC[1] < extentsC[4]) {
		if (y<extentsC[1] || y>extentsC[4]) return (false);
	}
	else {
		if (y>extentsC[1] || y<extentsC[4]) return (false);
	}
	if (extentsC[2] < extentsC[5]) {
		if (z<extentsC[2] || z>extentsC[5]) return (false);
	}
	else {
		if (z>extentsC[2] || z<extentsC[5]) return (false);
	}

	// Extents in spherical coords
	//
	double extentsS[6];
	RegularGrid::GetUserExtents(extentsS);

	//
	// Extents in spherical coords, permuted to order: lon, lat, radius
	//
	double extentsSP[6];
	_permute(_permutation, extentsSP, extentsS[0], extentsS[1], extentsS[2]);
	_permute(_permutation, extentsSP+3, extentsS[3], extentsS[4], extentsS[5]);

	double lon0 = extentsSP[0] * M_PI / 180.0;
	double lat0 = extentsSP[1] * M_PI / 180.0;
	double r0 = extentsSP[2];
	double lon1 = extentsSP[3] * M_PI / 180.0;
	double lat1 = extentsSP[4] * M_PI / 180.0;
	double r1 = extentsSP[5];

	double r = sqrt(x*x + y*y + z*z);
	if (r<r0 || r>r1) return(false);
	assert(r!=0);

	bool iper, jper, kper;
	HasPeriodic(&iper, &jper, &kper);
	
	double permP[3]; 
	_permute(_permutation, permP, iper, jper, kper);
	if (permP[0] || permP[1]) return(true);

	double theta = acos(z/r) - M_PI_2;	// acos is in range [0, pi];
	if (theta < lat0 || theta > lat1) return(false);

	double phi = atan2(y,x); // atan2 is in range [-pi, pi];
	if (phi < lon0 || theta > lon1) return(false);

	return(true);

}


//----------------------------------------------------------------------------
// Rearrange the x, y, and z components according to the permutation matrix
//----------------------------------------------------------------------------
void SphericalGrid::_permute(
	const vector<long>& permutation,
   double result[3], double x, double y, double z
) const {
	result[permutation[0]] = x;
	result[permutation[1]] = y;
	result[permutation[2]] = z;
}

