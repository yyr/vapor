#include <iostream>
#include <cassert>
#include <cmath>
#include <vapor/StretchedGrid.h>

using namespace std;

StretchedGrid::StretchedGrid(
	const size_t bs[3],
	const size_t min[3],
	const size_t max[3],
	const double extents[6],
	const bool periodic[3],
	float ** blks,
    const vector <double> &xcoords,
    const vector <double> &ycoords,
    const vector <double> &zcoords
 ) : RegularGrid(bs,min,max,extents,periodic,blks) {

	for (int i=0; i<3; i++) {
		_min[i] = min[i];
		_max[i] = max[i];
	}

	_xcoords.clear();
	_ycoords.clear();
	_zcoords.clear();

	size_t xdim = max[0]-min[0]+1;
	if (xcoords.size() != xdim) {
		double delta = xdim>1 ? (extents[3]-extents[0]) / (xdim-1) : 0.0;
		for (int i=0; i<xdim; i++) _xcoords.push_back(i*delta+extents[0]);
	}
	else _xcoords = xcoords;

	size_t ydim = max[1]-min[1]+1;
	if (xcoords.size() != xdim) {
		double delta = ydim>1 ? (extents[4]-extents[1]) / (ydim-1) : 0.0;
		for (int i=0; i<ydim; i++) _xcoords.push_back(i*delta+extents[1]);
	}
	else _ycoords = ycoords;

	size_t zdim = max[2]-min[2]+1;
	if (xcoords.size() != xdim) {
		double delta = zdim>1 ? (extents[5]-extents[2]) / (zdim-1) : 0.0;
		for (int i=0; i<zdim; i++) _xcoords.push_back(i*delta+extents[2]);
	}
	else _zcoords = zcoords;

}

StretchedGrid::StretchedGrid(
	const size_t bs[3],
	const size_t min[3],
	const size_t max[3],
	const double extents[6],
	const bool periodic[3],
	float ** blks,
    const vector <double> &xcoords,
    const vector <double> &ycoords,
    const vector <double> &zcoords,
	float missing_value
) : RegularGrid(bs,min,max,extents,periodic,blks, missing_value) {

	_xcoords.clear();
	_ycoords.clear();
	_zcoords.clear();

	size_t xdim = max[0]-min[0]+1;
	if (xcoords.size() != xdim) {
		double delta = xdim>1 ? (extents[3]-extents[0]) / (xdim-1) : 0.0;
		for (int i=0; i<xdim; i++) _xcoords.push_back(i*delta+extents[0]);
	}
	else _xcoords = xcoords;

	size_t ydim = max[1]-min[1]+1;
	if (xcoords.size() != xdim) {
		double delta = ydim>1 ? (extents[4]-extents[1]) / (ydim-1) : 0.0;
		for (int i=0; i<ydim; i++) _xcoords.push_back(i*delta+extents[1]);
	}
	else _ycoords = ycoords;

	size_t zdim = max[2]-min[2]+1;
	if (xcoords.size() != xdim) {
		double delta = zdim>1 ? (extents[5]-extents[2]) / (zdim-1) : 0.0;
		for (int i=0; i<zdim; i++) _xcoords.push_back(i*delta+extents[2]);
	}
	else _zcoords = zcoords;

}


float StretchedGrid::GetValue(double x, double y, double z) const {

	_ClampCoord(x,y,z);

    // At this point xyz should be within the bounds _minu, _maxu
    //
    if (! InsideGrid(x,y,z)) return(GetMissingValue());

    if (GetInterpolationOrder() == 0) {
        return (_GetValueNearestNeighbor(x,y,z));
    }
    else {
        return (_GetValueLinear(x,y,z));
    }
}

float StretchedGrid::_GetValueNearestNeighbor(
	double x, double y, double z
) const {

    // Get the indecies of the cell containing the point
    //
    size_t i, j, k;
    GetIJKIndexFloor(x,y,z, &i,&j,&k);

	double iwgt = 0.0;
	if (i<(_max[0]-_min[0])) {
		iwgt = (x - _xcoords[i]) / (_xcoords[i+1] - _xcoords[i]);
	}
	double jwgt = 0.0;
	if (j<(_max[1]-_min[1])) {
		jwgt = (y - _ycoords[j]) / (_ycoords[j+1] - _ycoords[j]);
	}
	double kwgt = 0.0;
	if (k<(_max[2]-_min[2])) {
		kwgt = (z - _zcoords[k]) / (_zcoords[k+1] - _zcoords[k]);
	}

	if (iwgt>0.5) i++;
	if (jwgt>0.5) j++;
	if (kwgt>0.5) k++;

	return(AccessIJK(i,j,k)); 
}

float StretchedGrid::_GetValueLinear(double x, double y, double z) const {

    // Get the indecies of the cell containing the point
    //
    size_t i, j, k;
    GetIJKIndexFloor(x,y,z, &i,&j,&k);

	double iwgt = 0.0;
	if (i<(_max[0]-_min[0])) {
		iwgt = (x - _xcoords[i]) / (_xcoords[i+1] - _xcoords[i]);
	}
	double jwgt = 0.0;
	if (j<(_max[1]-_min[1])) {
		jwgt = (y - _ycoords[j]) / (_ycoords[j+1] - _ycoords[j]);
	}
	double kwgt = 0.0;
	if (k<(_max[2]-_min[2])) {
		kwgt = (z - _zcoords[k]) / (_zcoords[k+1] - _zcoords[k]);
	}

	double p0,p1,p2,p3,p4,p5,p6,p7;

	p0 = AccessIJK(i,j,k); 
	if (p0 == GetMissingValue()) return (GetMissingValue());

	if (iwgt!=0.0) {
		p1 = AccessIJK(i+1,j,k);
		if (p1 == GetMissingValue()) return (GetMissingValue());
	}
	else p1 = 0.0;

	if (jwgt!=0.0) {
		p2 = AccessIJK(i,j+1,k);
		if (p2 == GetMissingValue()) return (GetMissingValue());
	}
	else p2 = 0.0;

	if (iwgt!=0.0 && jwgt!=0.0) {
		p3 = AccessIJK(i+1,j+1,k);
		if (p3 == GetMissingValue()) return (GetMissingValue());
	}
	else p3 = 0.0;

	if (kwgt!=0.0) {
		p4 = AccessIJK(i,j,k+1); 
		if (p4 == GetMissingValue()) return (GetMissingValue());
	}
	else p4 = 0.0;

	if (kwgt!=0.0 && iwgt!=0.0) {
		p5 = AccessIJK(i+1,j,k+1);
		if (p5 == GetMissingValue()) return (GetMissingValue());
	}
	else p5 = 0.0;

	if (kwgt!=0.0 && jwgt!=0.0) {
		p6 = AccessIJK(i,j+1,k+1);
		if (p6 == GetMissingValue()) return (GetMissingValue());
	}
	else p6 = 0.0;

	if (kwgt!=0.0 && iwgt!=0.0 && jwgt!=0.0) {
		p7 = AccessIJK(i+1,j+1,k+1);
		if (p7 == GetMissingValue()) return (GetMissingValue());
	}
	else p7 = 0.0;


	double c0 = p0+iwgt*(p1-p0) + jwgt*((p2+iwgt*(p3-p2))-(p0+iwgt*(p1-p0)));
	double c1 = p4+iwgt*(p5-p4) + jwgt*((p6+iwgt*(p7-p6))-(p4+iwgt*(p5-p4)));

	return(c0+kwgt*(c1-c0));

}

int StretchedGrid::GetUserCoordinates(
	size_t i, size_t j, size_t k, 
	double *x, double *y, double *z
) const {

	if (i>_max[0]-_min[0]) return(-1);
	if (j>_max[1]-_min[1]) return(-1);
	if (k>_max[2]-_min[2]) return(-1);

	*x = _xcoords[i];
	*y = _ycoords[j];
	*z = _zcoords[k];

	return(0);
}

void StretchedGrid::GetIJKIndex(
	double x, double y, double z,
	size_t *i, size_t *j, size_t *k
) const {

	size_t i0, j0, k0;
	GetIJKIndexFloor(x,y,z,&i0,&j0,&k0);

	if (i0<(_max[0]-_min[0])) {
		if (fabs((x-_xcoords[i0]) / (_xcoords[i0+1] - _xcoords[i0])) > 0.5) {
			i0++;
		}
	}
	if (j0<(_max[1]-_min[1])) {
		if (fabs((y-_ycoords[j0]) / (_ycoords[j0+1] - _ycoords[j0])) > 0.5) {
			j0++;
		}
	}
	if (k0<(_max[2]-_min[2])) {
		if (fabs((z-_zcoords[k0]) / (_zcoords[k0+1] - _zcoords[k0])) > 0.5) {
			k0++;
		}
	}

	*i = i0;
	*j = j0;
	*k = k0;
	
}
	

void StretchedGrid::GetIJKIndexFloor(
	double x, double y, double z,
	size_t *i, size_t *j, size_t *k
) const {

    _ClampCoord(x,y,z);
	double extents[6];
	GetUserExtents(extents);

    // Make sure the point xyz is within the bounds
    //
    if (extents[0]<extents[3]) {
        if (x<extents[0]) x = extents[0];
        if (x>extents[3]) x = extents[3];
    }
    else {
        if (x>extents[0]) x = extents[0];
        if (x<extents[3]) x = extents[3];
    }
    if (extents[1]<extents[4]) {
        if (y<extents[1]) y = extents[1];
        if (y>extents[4]) y = extents[4];
    }
    else {
        if (y>extents[1]) y = extents[1];
        if (y<extents[4]) y = extents[4];
    }
    if (extents[2]<extents[5]) {
        if (z<extents[2]) z = extents[2];
        if (z>extents[5]) z = extents[5];
    }
    else {
        if (z>extents[2]) z = extents[2];
        if (z<extents[5]) z = extents[5];
    }

	
	if (extents[0]<extents[3]) {
		for (int ii = 0; ii<_max[0]-_min[0]; ii++)  {
			if (x>=_xcoords[ii] && x<_xcoords[ii+1]) {
				*i = ii;
				break;
			}
		}
	}
	else {
		for (int ii = 0; ii<_max[0]-_min[0]; ii++)  {
			if (x<=_xcoords[ii] && x>_xcoords[ii+1]) {
				*i = ii;
				break;
			}
		}
	}

	if (extents[1]<extents[4]) {
		for (int jj = 0; jj<_max[1]-_min[1]; jj++)  {
			if (y>=_ycoords[jj] && y<_ycoords[jj+1]) {
				*j = jj;
				break;
			}
		}
	}
	else {
		for (int jj = 0; jj<_max[1]-_min[1]; jj++)  {
			if (y<=_ycoords[jj] && y>_ycoords[jj+1]) {
				*j = jj;
				break;
			}
		}
	}
			
	if (extents[2]<extents[5]) {
		for (int kk = 0; kk<_max[2]-_min[2]; kk++)  {
			if (z>=_zcoords[kk] && z<_zcoords[kk+1]) {
				*k = kk;
				break;
			}
		}
	}
	else {
		for (int kk = 0; kk<_max[2]-_min[2]; kk++)  {
			if (z<=_zcoords[kk] && z>_zcoords[kk+1]) {
				*k = kk;
				break;
			}
		}
	}
			
}

int StretchedGrid::Reshape(
	const size_t min[3],
	const size_t max[3],
	const bool periodic[3]
) {

	int rc = RegularGrid::Reshape(min, max, periodic);
	if (rc<0) return(rc);

	vector <double> xcoords;
	for (int i=min[0]; i<=max[0]; i++) xcoords.push_back(_xcoords[i-_min[0]]);
	_xcoords = xcoords;

	vector <double> ycoords;
	for (int i=min[1]; i<=max[1]; i++) ycoords.push_back(_ycoords[i-_min[1]]);
	_ycoords = ycoords;

	vector <double> zcoords;
	for (int i=min[2]; i<=max[2]; i++) zcoords.push_back(_zcoords[i-_min[2]]);
	_zcoords = zcoords;

	return(0);
}

void StretchedGrid::GetMinCellExtents(double *x, double *y, double *z) const {

	*x = *y = *z = 0.0;

	size_t dims[3];
	GetDimensions(dims);

	for (int i=0; i<_xcoords.size()-1; i++) {
		double tmp = fabs(_xcoords[i]-_xcoords[i+1]);

		if (i==0) *x = tmp;
		if (tmp<*x) *x = tmp;
	}
	for (int i=0; i<_ycoords.size()-1; i++) {
		double tmp = fabs(_ycoords[i]-_ycoords[i+1]);

		if (i==0) *y = tmp;
		if (tmp<*y) *y = tmp;
	}
	for (int i=0; i<_zcoords.size()-1; i++) {
		double tmp = fabs(_zcoords[i]-_zcoords[i+1]);

		if (i==0) *z = tmp;
		if (tmp<*z) *z = tmp;
	}
	
}

