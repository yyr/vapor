#include <iostream>
#include <cassert>
#include <cmath>
#include <cfloat>
#include "vapor/LayeredGrid.h"

using namespace std;
using namespace VAPoR;

LayeredGrid::LayeredGrid(
	const size_t bs[3],
	const size_t min[3],
	const size_t max[3],
	const double extents[6],
	const bool periodic[3],
	float ** blks,
	float ** coords,
	int varying_dim
 ) : RegularGrid(bs,min,max,extents,periodic,blks) {

    //
    // Shallow  copy blocks
    //
	size_t nblocks = RegularGrid::GetNumBlks();
    _coords = new float*[nblocks];
    for (int i=0; i<nblocks; i++) {
        _coords[i] = coords[i];
    }

	_varying_dim = varying_dim;
	if (_varying_dim < 0 || _varying_dim > 2) _varying_dim = 2;

	//
	// Periodic, varying dimensions are not supported
	//
	if (periodic[_varying_dim]) SetPeriodic(periodic);

	_GetUserExtents(_extents);
	RegularGrid::_SetExtents(_extents);

}

LayeredGrid::LayeredGrid(
	const size_t bs[3],
	const size_t min[3],
	const size_t max[3],
	const double extents[6],
	const bool periodic[3],
	float ** blks,
	float ** coords,
	int varying_dim,
	float missing_value
 ) : RegularGrid(bs,min,max,extents,periodic,blks, missing_value) {

    //
    // Shallow  copy blocks
    //
	size_t nblocks = RegularGrid::GetNumBlks();
    _coords = new float*[nblocks];
    for (int i=0; i<nblocks; i++) {
        _coords[i] = coords[i];
    }

	_varying_dim = varying_dim;
	if (_varying_dim < 0 || _varying_dim > 2) _varying_dim = 2;

	assert(periodic[_varying_dim] == false);

	_GetUserExtents(_extents);
	RegularGrid::_SetExtents(_extents);
}

LayeredGrid::~LayeredGrid() {
    if (_coords) delete [] _coords;
}
void LayeredGrid::GetBoundingBox(
    const size_t min[3],
    const size_t max[3],
    double extents[6]
) const {
	size_t mymin[] = {min[0],min[1],min[2]};
	size_t mymax[] = {max[0],max[1],max[2]};

	//
	// Don't stop until we find valid extents. When missing values
	// are present it's possible that the missing value dimension has 
	// an entire plane of missing values
	//
	bool done = false;
	while (! done) {
		done = true;
		_GetBoundingBox(mymin, mymax, extents);
		if (_varying_dim == 0) {
			if (extents[0] == FLT_MAX) {
				mymin[0] = mymin[0]+1;
				if (mymin[0] <= mymax[0]) done = false;
			}
			if (extents[3] == -FLT_MAX) {
				mymax[0] = mymax[0]-1;
				if (mymax[0] >= mymin[0]) done = false;
			}
		}
		else if (_varying_dim == 1) {
			if (extents[1] == FLT_MAX) {
				mymin[1] = mymin[1]+1;
				if (mymin[1] <= mymax[1]) done = false;
			}
			if (extents[4] == -FLT_MAX) {
				mymax[1] = mymax[1]-1;
				if (mymax[1] >= mymin[1]) done = false;
			}
		}
		else if (_varying_dim == 2) {
			if (extents[2] == FLT_MAX) {
				mymin[2] = mymin[2]+1;
				if (mymin[2] <= mymax[2]) done = false;
			}
			if (extents[5] == -FLT_MAX) {
				mymax[2] = mymax[2]-1;
				if (mymax[2] >= mymin[2]) done = false;
			}
		}
	}

}

void LayeredGrid::_GetBoundingBox(
    const size_t min[3],
    const size_t max[3],
    double extents[6]
) const {

	// Get extents of non-varying dimension. Values returned for
	// varying dimension are coordinates for first and last grid
	// point, respectively, which in general are not the extents 
	// of the bounding box.
	//
    RegularGrid::GetUserCoordinates(
        min[0], min[1], min[2], &(extents[0]), &(extents[1]), &(extents[2])
    );
    RegularGrid::GetUserCoordinates(
        max[0], max[1], max[2], &(extents[3]), &(extents[4]), &(extents[5])
    );

	// Initialize min and max coordinates of varying dimension with 
	// coordinates of "first" and "last" grid point. Coordinates of 
	// varying dimension are stored as values of a scalar function
	// sampling the coordinate space.
	//
	float mincoord = FLT_MAX;
	float maxcoord = -FLT_MAX;

	float mv = GetMissingValue();

	// Now find the extreme values of the varying dimension's coordinates
	//
	if (_varying_dim == 0) {	// I plane

		// Find min coordinate in first plane
		//
		for (int k = min[2]; k<=max[2]; k++) {
		for (int j = min[1]; j<=max[1]; j++) {
			float v = _AccessIJK(_coords, min[0],j,k);
			if (v == mv) continue;
			if (extents[0] < extents[3]) {
				if (v<mincoord) mincoord = v;
			} else {
				if (v>mincoord) mincoord = v;
			}
		}
		}

		// Find max coordinate in last plane
		//
		for (int k = min[2]; k<=max[2]; k++) {
		for (int j = min[1]; j<=max[1]; j++) {
			float v = _AccessIJK(_coords, max[0],j,k);
			if (v == mv) continue;
			if (extents[0] <extents[3]) {
				if (v>maxcoord) maxcoord = v;
			} else {
				if (v<maxcoord) maxcoord = v;
			}
		}
		}
	}
	else if (_varying_dim == 1) {	// J plane
		for (int k = min[2]; k<=max[2]; k++) {
		for (int i = min[0]; i<=max[0]; i++) {
			float v = _AccessIJK(_coords, i,min[1],k);
			if (v == mv) continue;
			if (extents[1] < extents[4]) {
				if (v<mincoord) mincoord = v;
			} else {
				if (v>mincoord) mincoord = v;
			}
		}
		}

		for (int k = min[2]; k<=max[2]; k++) {
		for (int i = min[0]; i<=max[0]; i++) {
			float v = _AccessIJK(_coords, i,max[1],k);
			if (v == mv) continue;
			if (extents[1] < extents[4]) {
				if (v>maxcoord) maxcoord = v;
			} else {
				if (v<maxcoord) maxcoord = v;
			}
		}
		}
	}
	else {	// _varying_dim == 2 (K plane)
		for (int j = min[1]; j<=max[1]; j++) {
		for (int i = min[0]; i<=max[0]; i++) {
			float v = _AccessIJK(_coords, i,j,min[2]);
			if (v == mv) continue;
			if (extents[2] < extents[5]) {
				if (v<mincoord) mincoord = v;
			} else {
				if (v>mincoord) mincoord = v;
			}
		}
		}

		for (int j = min[1]; j<=max[1]; j++) {
		for (int i = min[0]; i<=max[0]; i++) {
			float v = _AccessIJK(_coords, i,j,max[2]);
			if (v == mv) continue;
			if (extents[2] < extents[5]) {
				if (v>maxcoord) maxcoord = v;
			} else {
				if (v<maxcoord) maxcoord = v;
			}
		}
		}
	}

	extents[_varying_dim] = mincoord;
	extents[_varying_dim+3] = maxcoord;
}


float LayeredGrid::GetValue(double x, double y, double z) const {

	_ClampCoord(x,y,z);

	if (! InsideGrid(x,y,z)) return(GetMissingValue());

	size_t dims[3];
	GetDimensions(dims);

	// Get the indecies of the cell containing the point
	//
	size_t i0, j0, k0;
	size_t i1, j1, k1;
	GetIJKIndexFloor(x,y,z, &i0,&j0,&k0);
	if (i0 == dims[0]-1) i1 = i0;
	else i1 = i0+1;
	if (j0 == dims[1]-1) j1 = j0;
	else j1 = j0+1;
	if (k0 == dims[2]-1) k1 = k0;
	else k1 = k0+1;


	// Get user coordinates of cell containing point
	//
	double x0, y0, z0, x1, y1, z1;
	RegularGrid::GetUserCoordinates(i0,j0,k0, &x0, &y0, &z0);
	RegularGrid::GetUserCoordinates(i1,j1,k1, &x1, &y1, &z1);

	//
	// Calculate interpolation weights. We always interpolate along
	// the varying dimension last (the kwgt)
	//
	double iwgt, jwgt, kwgt;
	if (_varying_dim == 0) {
		// We have to interpolate coordinate of varying dimension
		//
		x0 = _interpolateVaryingCoord(i0,j0,k0,x,y,z);
		x1 = _interpolateVaryingCoord(i1,j0,k0,x,y,z);

		if (y1!=y0) iwgt = fabs((y-y0) / (y1-y0));
		else iwgt = 0.0;
		if (z1!=z0) jwgt = fabs((z-z0) / (z1-z0));
		else jwgt = 0.0;
		if (x1!=x0) kwgt = fabs((x-x0) / (x1-x0));
		else kwgt = 0.0;
	}
	else if (_varying_dim == 1) {
		y0 = _interpolateVaryingCoord(i0,j0,k0,x,y,z);
		y1 = _interpolateVaryingCoord(i0,j1,k0,x,y,z);

		if (x1!=x0) iwgt = fabs((x-x0) / (x1-x0));
		else iwgt = 0.0;
		if (z1!=z0) jwgt = fabs((z-z0) / (z1-z0));
		else jwgt = 0.0;
		if (y1!=y0) kwgt = fabs((y-y0) / (y1-y0));
		else kwgt = 0.0;
	}
	else {
		z0 = _interpolateVaryingCoord(i0,j0,k0,x,y,z);
		z1 = _interpolateVaryingCoord(i0,j0,k1,x,y,z);

		if (x1!=x0) iwgt = fabs((x-x0) / (x1-x0));
		else iwgt = 0.0;
		if (y1!=y0) jwgt = fabs((y-y0) / (y1-y0));
		else jwgt = 0.0;
		if (z1!=z0) kwgt = fabs((z-z0) / (z1-z0));
		else kwgt = 0.0;
	}

	if (GetInterpolationOrder() == 0) {
		if (iwgt>0.5) i0++;
		if (jwgt>0.5) j0++;
		if (kwgt>0.5) k0++;

		return(AccessIJK(i0,j0,k0));
	}

	//
	// perform tri-linear interpolation
	//
	double p0,p1,p2,p3,p4,p5,p6,p7;

	p0 = AccessIJK(i0,j0,k0); 
	if (p0 == GetMissingValue()) return (GetMissingValue());

	if (iwgt!=0.0) {
		p1 = AccessIJK(i1,j0,k0);
		if (p1 == GetMissingValue()) return (GetMissingValue());
	}
	else p1 = 0.0;

	if (jwgt!=0.0) {
		p2 = AccessIJK(i0,j1,k0);
		if (p2 == GetMissingValue()) return (GetMissingValue());
	}
	else p2 = 0.0;

	if (iwgt!=0.0 && jwgt!=0.0) {
		p3 = AccessIJK(i1,j1,k0);
		if (p3 == GetMissingValue()) return (GetMissingValue());
	}
	else p3 = 0.0;

	if (kwgt!=0.0) {
		p4 = AccessIJK(i0,j0,k1); 
		if (p4 == GetMissingValue()) return (GetMissingValue());
	}
	else p4 = 0.0;

	if (kwgt!=0.0 && iwgt!=0.0) {
		p5 = AccessIJK(i1,j0,k1);
		if (p5 == GetMissingValue()) return (GetMissingValue());
	}
	else p5 = 0.0;

	if (kwgt!=0.0 && jwgt!=0.0) {
		p6 = AccessIJK(i0,j1,k1);
		if (p6 == GetMissingValue()) return (GetMissingValue());
	}
	else p6 = 0.0;

	if (kwgt!=0.0 && iwgt!=0.0 && jwgt!=0.0) {
		p7 = AccessIJK(i1,j1,k1);
		if (p7 == GetMissingValue()) return (GetMissingValue());
	}
	else p7 = 0.0;


	double c0 = p0+iwgt*(p1-p0) + jwgt*((p2+iwgt*(p3-p2))-(p0+iwgt*(p1-p0)));
	double c1 = p4+iwgt*(p5-p4) + jwgt*((p6+iwgt*(p7-p6))-(p4+iwgt*(p5-p4)));

	return(c0+kwgt*(c1-c0));

}


void LayeredGrid::_GetUserExtents(double extents[6]) const {


	size_t dims[3];
	GetDimensions(dims);

	size_t min[3] = {0,0,0};
	size_t max[3] = {dims[0]-1, dims[1]-1, dims[2]-1};

	LayeredGrid::GetBoundingBox(min, max, extents);

}

int LayeredGrid::GetUserCoordinates(
	size_t i, size_t j, size_t k, 
	double *x, double *y, double *z
) const {

	// First get coordinates of non-varying dimensions
	// The varying dimension coordinate returned is ignored
	//
	int rc = RegularGrid::GetUserCoordinates(i,j,k,x,y,z);
	if (rc<0) return(rc);

	// Now get coordinates of varying dimension
	//

	if (_varying_dim == 0) {	
		*x = _GetVaryingCoord(i,j,k);
	}
	else if (_varying_dim == 1) {
		*y = _GetVaryingCoord(i,j,k);
	}
	else {
		*z = _GetVaryingCoord(i,j,k);
	}
	return(0);

}

void LayeredGrid::GetIJKIndex(
	double x, double y, double z,
	size_t *i, size_t *j, size_t *k
) const {

	// First get ijk index of non-varying dimensions
	// N.B. index returned for varying dimension is bogus
	//
	RegularGrid::GetIJKIndex(x,y,z, i,j,k);

	size_t dims[3];
	GetDimensions(dims);

	// At this point the ijk indecies are correct for the non-varying
	// dimensions. We only need to find the index for the varying dimension
	//
	if (_varying_dim == 0) {
		size_t i0, j0, k0;
		LayeredGrid::GetIJKIndexFloor(x,y,z,&i0, &j0, &k0);
		if (i0 == dims[0]-1) {	// on boundary
			*i = i0;
			return;
		}

		double x0 = _interpolateVaryingCoord(i0,j0,k0,x,y,z);
		double x1 = _interpolateVaryingCoord(i0+1,j0,k0,x,y,z);
		if (fabs(x-x0) < fabs(x-x1)) {
			*i = i0;
		}
		else {
			*i = i0+1;
		}
	}
	else if (_varying_dim == 1) {
		size_t i0, j0, k0;
		LayeredGrid::GetIJKIndexFloor(x,y,z,&i0, &j0, &k0);
		if (j0 == dims[1]-1) {	// on boundary
			*j = j0;
			return;
		}

		double y0 = _interpolateVaryingCoord(i0,j0,k0,x,y,z);
		double y1 = _interpolateVaryingCoord(i0,j0+1,k0,x,y,z);
		if (fabs(y-y0) < fabs(y-y1)) {
			*j = j0;
		}
		else {
			*j = j0+1;
		}
	}
	else if (_varying_dim == 2) {
		size_t i0, j0, k0;
		LayeredGrid::GetIJKIndexFloor(x,y,z,&i0, &j0, &k0);
		if (k0 == dims[2]-1) {	// on boundary
			*k = k0;
			return;
		}

		double z0 = _interpolateVaryingCoord(i0,j0,k0,x,y,z);
		double z1 = _interpolateVaryingCoord(i0,j0,k0+1,x,y,z);
		if (fabs(z-z0) < fabs(z-z1)) {
			*k = k0;
		}
		else {
			*k = k0+1;
		}
	}
}

void LayeredGrid::GetIJKIndexFloor(
	double x, double y, double z,
	size_t *i, size_t *j, size_t *k
) const {

	// First get ijk index of non-varying dimensions
	// N.B. index returned for varying dimension is bogus
	//
	size_t i0, j0, k0;
	RegularGrid::GetIJKIndexFloor(x,y,z, &i0,&j0,&k0);

	size_t dims[3];
	GetDimensions(dims);

	size_t i1, j1, k1;
	if (i0 == dims[0]-1) i1 = i0;
	else i1 = i0+1;
	if (j0 == dims[1]-1) j1 = j0;
	else j1 = j0+1;
	if (k0 == dims[2]-1) k1 = k0;
	else k1 = k0+1;


	// At this point the ijk indecies are correct for the non-varying
	// dimensions. We only need to find the index for the varying dimension
	//
	if (_varying_dim == 0) {
		*j = j0; *k = k0;

		i0 = 0;
		i1 = dims[0]-1;
		double x0 = _interpolateVaryingCoord(i0,j0,k0,x,y,z);
		double x1 = _interpolateVaryingCoord(i1,j0,k0,x,y,z);

		// see if point is outside grid or on boundary
		//
		if ((x-x0) * (x-x1) >= 0.0) { 	
			if (x0<=x1) {
				if (x<=x0) *i = 0;
				else if (x>=x1) *i = dims[0]-1;
			}
			else {
				if (x>=x0) *i = 0;
				else if (x<=x1) *i = dims[0]-1;
			}
			return;
		}

		//
		// X-coord of point must be between x0 and x1
		//
		while (i1-i0 > 1) {

			x1 = _interpolateVaryingCoord((i0+i1)>>1,j0,k0,x,y,z);
			if (x1 == x) {  // pathological case
				//*i = (i0+i1)>>1;
				i0 = (i0+i1)>>1;
				break;
			}

			// if the signs of differences change then the coordinate 
			// is between x0 and x1
			//
			if ((x-x0) * (x-x1) <= 0.0) { 
				i1 = (i0+i1)>>1;
			}
			else {
				i0 = (i0+i1)>>1;
				x0 = x1;
			}
		}
		*i = i0;

	}
	else if (_varying_dim == 1) {
		*i = i0; *k = k0;

		// Now search for the closest point
		//
		j0 = 0;
		j1 = dims[1]-1;
		double y0 = _interpolateVaryingCoord(i0,j0,k0,x,y,z);
		double y1 = _interpolateVaryingCoord(i0,j1,k0,x,y,z);

		// see if point is outside grid or on boundary
		//
		if ((y-y0) * (y-y1) >= 0.0) { 	
			if (y0<=y1) {
				if (y<=y0) *j = 0;
				else if (y>=y1) *j = dims[1]-1;
			}
			else {
				if (y>=y0) *j = 0;
				else if (y<=y1) *j = dims[1]-1;
			}
			return;
		}

		//
		// Y-coord of point must be between y0 and y1
		//
		while (j1-j0 > 1) {

			y1 = _interpolateVaryingCoord(i0,(j0+j1)>>1,k0,x,y,z);
			if (y1 == y) {  // pathological case
				//*j = (j0+j1)>>1;
				j0 = (j0+j1)>>1;
				break;
			}

			// if the signs of differences change then the coordinate 
			// is between y0 and y1
			//
			if ((y-y0) * (y-y1) <= 0.0) { 
				j1 = (j0+j1)>>1;
			}
			else {
				j0 = (j0+j1)>>1;
				y0 = y1;
			}
		}
		*j = j0;
	}
	else {	// _varying_dim == 2
		*i = i0; *j = j0;

		// Now search for the closest point
		//
		k0 = 0;
		k1 = dims[2]-1;
		double z0 = _interpolateVaryingCoord(i0,j0,k0,x,y,z);
		double z1 = _interpolateVaryingCoord(i0,j0,k1,x,y,z);

		// see if point is outside grid or on boundary
		//
		if ((z-z0) * (z-z1) >= 0.0) { 	
			if (z0<=z1) {
				if (z<=z0) *k = 0;
				else if (z>=z1) *k = dims[2]-1;
			}
			else {
				if (z>=z0) *k = 0;
				else if (z<=z1) *k = dims[2]-1;
			}
			return;
		}

		//
		// Z-coord of point must be between z0 and z1
		//
		while (k1-k0 > 1) {

			z1 = _interpolateVaryingCoord(i0,j0,(k0+k1)>>1,x,y,z);
			if (z1 == z) {	// pathological case
				//*k = (k0+k1)>>1;
				k0 = (k0+k1)>>1;
				break;
			}

			// if the signs of differences change then the coordinate 
			// is between z0 and z1
			//
			if ((z-z0) * (z-z1) <= 0.0) { 
				k1 = (k0+k1)>>1;
			}
			else {
				k0 = (k0+k1)>>1;
				z0 = z1;
			}
		}
		*k = k0;
	}
}

int LayeredGrid::Reshape(
	const size_t min[3],
	const size_t max[3],
	const bool periodic[3]
) {
	int rc = RegularGrid::Reshape(min,max,periodic);
	if (rc<0) return(-1);

	double myextents[6];

	_GetUserExtents(myextents);
	RegularGrid::_SetExtents(myextents);

	return(0);
}
void LayeredGrid::SetPeriodic(const bool periodic[3]) {

	bool pvec[3];
	for (int i=0; i<3; i++) pvec[i] = periodic[i];
	pvec[_varying_dim] = false;
	RegularGrid::SetPeriodic(pvec);
}

bool LayeredGrid::InsideGrid(double x, double y, double z) const {

	// Clamp coordinates on periodic boundaries to reside within the 
	// grid extents (vary-dimensions can not have periodic boundaries)
	//
	_ClampCoord(x,y,z);

	// Do a quick check to see if the point is completely outside of 
	// the grid bounds.
	//
	double extents[6];
	GetUserExtents(extents);
	if (extents[0] < extents[3]) {
		if (x<extents[0] || x>extents[3]) return (false);
	}
	else {
		if (x>extents[0] || x<extents[3]) return (false);
	}
	if (extents[1] < extents[4]) {
		if (y<extents[1] || y>extents[4]) return (false);
	}
	else {
		if (y>extents[1] || y<extents[4]) return (false);
	}
	if (extents[2] < extents[5]) {
		if (z<extents[2] || z>extents[5]) return (false);
	}
	else {
		if (z>extents[2] || z<extents[5]) return (false);
	}

	// If we get to here the point's non-varying dimension coordinates
	// must be inside the grid. It is only the varying dimension
	// coordinates that we need to check
	//

	// Get the indecies of the cell containing the point
	// Only the indecies for the non-varying dimensions are correctly
	// returned by GetIJKIndexFloor()
	//
	size_t dims[3];
	GetDimensions(dims);

	size_t i0, j0, k0;
	size_t i1, j1, k1;
	RegularGrid::GetIJKIndexFloor(x,y,z, &i0,&j0,&k0);
	if (i0 == dims[0]-1) i1 = i0;
	else i1 = i0+1;
	if (j0 == dims[1]-1) j1 = j0;
	else j1 = j0+1;
	if (k0 == dims[2]-1) k1 = k0;
	else k1 = k0+1;

	//
	// See if the varying dimension coordinate of the point is 
	// completely above or below all of the corner points in the first (last)
	// cell in the column of cells containing the point
	//
	double t00, t01, t10, t11;	// varying dimension coord of "top" cell
	double b00, b01, b10, b11;	// varying dimension coord of "bottom" cell
	double vc; // varying coordinate value
	if (_varying_dim == 0) {

		t00 = _GetVaryingCoord( dims[0]-1, j0, k0);
		t01 = _GetVaryingCoord( dims[0]-1, j1, k0);
		t10 = _GetVaryingCoord( dims[0]-1, j0, k1);
		t11 = _GetVaryingCoord( dims[0]-1, j1, k1);

		b00 = _GetVaryingCoord( 0, j0, k0);
		b01 = _GetVaryingCoord( 0, j1, k0);
		b10 = _GetVaryingCoord( 0, j0, k1);
		b11 = _GetVaryingCoord( 0, j1, k1);
		vc = x;

	}
	else if (_varying_dim == 1) {

		t00 = _GetVaryingCoord( i0, dims[1]-1, k0);
		t01 = _GetVaryingCoord( i1, dims[1]-1, k0);
		t10 = _GetVaryingCoord( i0, dims[1]-1, k1);
		t11 = _GetVaryingCoord( i1, dims[1]-1, k1);

		b00 = _GetVaryingCoord( i0, 0, k0);
		b01 = _GetVaryingCoord( i1, 0, k0);
		b10 = _GetVaryingCoord( i0, 0, k1);
		b11 = _GetVaryingCoord( i1, 0, k1);
		vc = y;

	}
	else { // _varying_dim == 2

		t00 = _GetVaryingCoord( i0, j0, dims[2]-1);
		t01 = _GetVaryingCoord( i1, j0, dims[2]-1);
		t10 = _GetVaryingCoord( i0, j1, dims[2]-1);
		t11 = _GetVaryingCoord( i1, j1, dims[2]-1);

		b00 = _GetVaryingCoord( i0, j0, 0);
		b01 = _GetVaryingCoord( i1, j0, 0);
		b10 = _GetVaryingCoord( i0, j1, 0);
		b11 = _GetVaryingCoord( i1, j1, 0);
		vc = z;
	}

	if (b00<t00) {
		// If coordinate is between all of the cell's top and bottom
		// coordinates the point must be in the grid
		//
		if (b00<vc && b01<vc && b10<vc && b11<vc &&
			t00>vc && t01>vc && t10>vc && t11>vc) {

			return(true);
		}
		//
		// if coordinate is above or below all corner points 
		// the input point must be outside the grid
		//
		if (b00>vc && b01>vc && b10>vc && b11>vc) return(false);
		if (t00<vc && t01<vc && t10<vc && t11<vc) return(false);
	}
	else {
		if (b00>vc && b01>vc && b10>vc && b11>vc &&
			t00<vc && t01<vc && t10<vc && t11<vc) {

			return(true);
		}
		if (b00<vc && b01<vc && b10<vc && b11<vc) return(false);
		if (t00>vc && t01>vc && t10>vc && t11>vc) return(false);
	}

	// If we get this far the point is either inside or outside of a
	// boundary cell on the varying dimension. Need to interpolate
	// the varying coordinate of the cell

	// Varying dimesion coordinate value returned by GetUserCoordinates
	// is not valid
	//
	double x0, y0, z0, x1, y1, z1;
	RegularGrid::GetUserCoordinates(i0,j0,k0, &x0, &y0, &z0);
	RegularGrid::GetUserCoordinates(i1,j1,k1, &x1, &y1, &z1);

	double iwgt, jwgt;
	if (_varying_dim == 0) {
		if (y1!=y0) iwgt = fabs((y-y0) / (y1-y0));
		else iwgt = 0.0;
		if (z1!=z0) jwgt = fabs((z-z0) / (z1-z0));
		else jwgt = 0.0;
	}
	else if (_varying_dim == 1) {
		if (x1!=x0) iwgt = fabs((x-x0) / (x1-x0));
		else iwgt = 0.0;
		if (z1!=z0) jwgt = fabs((z-z0) / (z1-z0));
		else jwgt = 0.0;
	}
	else {
		if (x1!=x0) iwgt = fabs((x-x0) / (x1-x0));
		else iwgt = 0.0;
		if (y1!=y0) jwgt = fabs((y-y0) / (y1-y0));
		else jwgt = 0.0;
	}

	double t = t00+iwgt*(t01-t00) + jwgt*((t10+iwgt*(t11-t10))-(t00+iwgt*(t01-t00)));
	double b = b00+iwgt*(b01-b00) + jwgt*((b10+iwgt*(b11-b10))-(b00+iwgt*(b01-b00)));

	if (b<t) {
		if (vc<b || vc>t) return(false);
	} 
	else {
		if (vc>b || vc<t) return(false);
	}
	return(true);
}

void LayeredGrid::GetMinCellExtents(double *x, double *y, double *z) const {

	//
	// Get X and Y dimension minimums
	//
	RegularGrid::GetMinCellExtents(x,y,z);

	size_t dims[3];
	GetDimensions(dims);

	if (dims[2] < 2) return;

	double tmp;
	*z = fabs(_extents[5] - _extents[3]);

	for (int k=0; k<dims[2]-1; k++) {
	for (int j=0; j<dims[1]; j++) {
	for (int i=0; i<dims[0]; i++) {
		float z0 = _GetVaryingCoord(i,j,k);
		float z1 = _GetVaryingCoord(i,j,k+1);
		
		tmp = fabs(z1-z0);
		if (tmp<*z) *z = tmp;
	}
	}
	}
}

double LayeredGrid::_GetVaryingCoord(size_t i, size_t j, size_t k) const {
	double mv = GetMissingValue();
	double c = _AccessIJK(_coords, i, j, k);
	if (c != mv) return (c);

	size_t dims[3];
	GetDimensions(dims);
	
	//
	// Varying coordinate for i,j,k is missing. Look along varying dimension
	// axis for first grid point with non-missing value. First search
	// below, then above. If no valid coordinate is found, use the 
	// grid minimum extent
	//
	
	if (_varying_dim == 0) {
		size_t ii = i;
		while (ii>0 && (c=_AccessIJK(_coords,ii-1,j,k)) == mv) ii--; 
		if (c!=mv) return (c);

		ii = i;
		while (ii<dims[0]-1 && (c=_AccessIJK(_coords,ii+1,j,k)) == mv) ii++; 
		if (c!=mv) return (c);

		return(_extents[0]);
		
	} else if (_varying_dim == 1) {
		size_t jj = j;
		while (jj>0 && (c=_AccessIJK(_coords,i,jj-1,k)) == mv) jj--; 
		if (c!=mv) return (c);

		jj = j;
		while (jj<dims[1]-1 && (c=_AccessIJK(_coords,i, jj+1,k)) == mv) jj++; 
		if (c!=mv) return (c);

		return(_extents[1]);

	} else if (_varying_dim == 2) {
		size_t kk = k;
		while (kk>0 && (c=_AccessIJK(_coords,i,j,kk-1)) == mv) kk--; 
		if (c!=mv) return (c);

		kk = k;
		while (kk<dims[2]-1 && (c=_AccessIJK(_coords,i,j,kk+1)) == mv) kk++; 
		if (c!=mv) return (c);

		return(_extents[2]);
	}
	return(0.0);
}

double LayeredGrid::_interpolateVaryingCoord(
	size_t i0, size_t j0, size_t k0,
	double x, double y, double z) const {

	// varying dimension coord at corner grid points of cell face
	//
	double c00, c01, c10, c11;	

	size_t dims[3];
	GetDimensions(dims);

	size_t i1, j1, k1;
	if (i0 == dims[0]-1) i1 = i0;
	else i1 = i0+1;
	if (j0 == dims[1]-1) j1 = j0;
	else j1 = j0+1;
	if (k0 == dims[2]-1) k1 = k0;
	else k1 = k0+1;

	// Coordinates of grid points for non-varying dimensions 
	double x0, y0, z0, x1, y1, z1;	
	RegularGrid::GetUserCoordinates(i0,j0,k0, &x0, &y0, &z0);
	RegularGrid::GetUserCoordinates(i1,j1,k1, &x1, &y1, &z1);
	double iwgt, jwgt;

	if (_varying_dim == 0) {

		// Now get user coordinates of vary-dimension grid point
		// N.B. Could just call LayeredGrid:GetUserCoordinates() for 
		// each of the four points but this is more efficient
		//
		c00 = _AccessIJK(_coords, i0, j0, k0);
		c01 = _AccessIJK(_coords, i0, j1, k0);
		c10 = _AccessIJK(_coords, i0, j0, k1);
		c11 = _AccessIJK(_coords, i0, j1, k1);

		if (y1!=y0) iwgt = fabs((y-y0) / (y1-y0));
		else iwgt = 0.0;
		if (z1!=z0) jwgt = fabs((z-z0) / (z1-z0));
		else jwgt = 0.0;
	}
	else if (_varying_dim == 1) {

		c00 = _AccessIJK(_coords, i0, j0, k0);
		c01 = _AccessIJK(_coords, i1, j0, k0);
		c10 = _AccessIJK(_coords, i0, j0, k1);
		c11 = _AccessIJK(_coords, i1, j0, k1);

		if (x1!=x0) iwgt = fabs((x-x0) / (x1-x0));
		else iwgt = 0.0;
		if (z1!=z0) jwgt = fabs((z-z0) / (z1-z0));
		else jwgt = 0.0;

	}
	else { // _varying_dim == 2

		c00 = _AccessIJK(_coords, i0, j0, k0);
		c01 = _AccessIJK(_coords, i1, j0, k0);
		c10 = _AccessIJK(_coords, i0, j1, k0);
		c11 = _AccessIJK(_coords, i1, j1, k0);

		if (x1!=x0) iwgt = fabs((x-x0) / (x1-x0));
		else iwgt = 0.0;
		if (y1!=y0) jwgt = fabs((y-y0) / (y1-y0));
		else jwgt = 0.0;

	}

	double c = c00+iwgt*(c01-c00) + jwgt*((c10+iwgt*(c11-c10))-(c00+iwgt*(c01-c00)));
	return(c);
}
