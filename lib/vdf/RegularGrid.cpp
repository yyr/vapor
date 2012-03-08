#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <time.h>
#ifdef  Darwin
#include <mach/mach_time.h>
#endif
#ifdef _WINDOWS
#include "windows.h"
#include "Winbase.h"
#include <limits>
#endif

#include "vapor/RegularGrid.h"

using namespace std;

double GetTime() {
    double t;
#ifdef _WINDOWS //Windows does not have a nanosecond time function...
	SYSTEMTIME sTime;
	FILETIME fTime;
	GetSystemTime(&sTime);
	SystemTimeToFileTime(&sTime,&fTime);
    //Resulting system time is in 100ns increments
	__int64 longlongtime = fTime.dwHighDateTime;
	longlongtime <<= 32;
	longlongtime += fTime.dwLowDateTime;
	t = (double)longlongtime;
	t *= 1.e-7;

#endif
#ifndef WIN32
	struct timespec ts;
	ts.tv_sec = ts.tv_nsec = 0;
#endif

#ifdef Linux
    clock_gettime(CLOCK_REALTIME, &ts);
    t = (double) ts.tv_sec + (double) ts.tv_nsec*1.0e-9;
#endif

#ifdef  Darwin
    uint64_t tmac = mach_absolute_time();
    mach_timebase_info_data_t info = {0,0};
    mach_timebase_info(&info);
    ts.tv_sec = tmac * 1e-9;
    ts.tv_nsec = tmac - (ts.tv_sec * 1e9);
    t = (double) ts.tv_sec + (double) ts.tv_nsec*1.0e-9;
#endif

    return(t);
}


int RegularGrid::_RegularGrid(
	const size_t bs[3],
	const size_t min[3],
	const size_t max[3],
	const double extents[6],
	const bool periodic[3],
	float **blks
) {
#ifdef _WINDOWS //Define INFINITY
	float INFINITY = numeric_limits<float>::infinity( );
	float NAN = numeric_limits<float>::quiet_NaN();
#endif
	_nblocks = 1;
	for (int i=0; i<3; i++) {
		assert(max[i] >= min[i]);
		_bs[i] = bs[i];
		if (_bs[i] == 0) _bs[i] = 1;
		_bdims[i] = (max[i]/bs[i]) - (min[i]/bs[i]) + 1;
		_nblocks *= _bdims[i];
		_minabs[i] = min[i];
		_maxabs[i] = max[i];
		_min[i] = min[i] % bs[i];
		_max[i] = _min[i] + (max[i]-min[i]);
		_minu[i] = extents[i];
		_maxu[i] = extents[i+3];
		if (max[i]-min[i]) {
			_delta[i] = fabs((_maxu[i] - _minu[i])) / (double) (max[i]-min[i]);
		}
		else {
			_delta[i] = 0.0; // error
		}
		_periodic[i] = periodic[i];
	}

	//
	// Shallow  copy blocks
	//
	_blks = new float*[_nblocks];
	for (int i=0; i<_nblocks; i++) {
		_blks[i] = blks[i];
	}

	_hasMissing = false;
	_missingValue = INFINITY;
	_invalidAccess = new float[1];
	*_invalidAccess = NAN;
	_interpolationOrder = 1;
	ResetItr();

	return(0);
}

RegularGrid::RegularGrid(
	const size_t bs[3],
	const size_t min[3],
	const size_t max[3],
	const double extents[6],
	const bool periodic[3],
	float **blks
) {
	(void) _RegularGrid(bs,min,max,extents,periodic, blks);
}

RegularGrid::RegularGrid(
	const size_t bs[3],
	const size_t min[3],
	const size_t max[3],
	const double extents[6],
	const bool periodic[3],
	float **blks,
	float missing_value
) {
	(void) _RegularGrid(bs,min,max,extents,periodic, blks);
	_missingValue = missing_value;
	_hasMissing = true;
}

RegularGrid::~RegularGrid() {
	if (_invalidAccess) delete [] _invalidAccess;
	if (_blks) delete [] _blks;
}

float &RegularGrid::AccessIJK( size_t x, size_t y, size_t z) const {
	return(_AccessIJK(_blks, x,y,z));
}

float &RegularGrid::_AccessIJK(
	float **blks, size_t x, size_t y, size_t z) const {

	if (x>(_max[0]-_min[0])) return(*_invalidAccess);
	if (y>(_max[1]-_min[1])) return(*_invalidAccess);
	if (z>(_max[2]-_min[2])) return(*_invalidAccess);

	// x,y,z are specified relative to _min[i]
	//
	x += _min[0];
	y += _min[1];
	z += _min[2];

	size_t xb = x / _bs[0];
	size_t yb = y / _bs[1];
	size_t zb = z / _bs[2];
	x = x % _bs[0];
	y = y % _bs[1];
	z = z % _bs[2];
	float *blk = blks[zb*_bdims[0]*_bdims[1] + yb*_bdims[0] + xb];
	return(blk[z*_bs[0]*_bs[1] + y*_bs[0] + x]);
}

float RegularGrid::GetValue(double x, double y, double z) const {

	// Clamp coordinates on periodic boundaries to grid extents
	//
	_ClampCoord(x, y, z);

	// At this point xyz should be within the bounds _minu, _maxu
	//
	if (! InsideGrid(x,y,z)) return(_missingValue);

	if (_interpolationOrder == 0) {
		return (_GetValueNearestNeighbor(x,y,z));
	}
	else {
		return (_GetValueLinear(x,y,z));
	}
}

float RegularGrid::_GetValueNearestNeighbor(double x, double y, double z) const {

	size_t i = 0;
	size_t j = 0;
	size_t k = 0;

	if (_delta[0] != 0.0) i = (size_t) floor ((x-_minu[0]) / _delta[0]);
	if (_delta[1] != 0.0) j = (size_t) floor ((y-_minu[1]) / _delta[1]);
	if (_delta[2] != 0.0) k = (size_t) floor ((z-_minu[2]) / _delta[2]);

	assert(i<=(_max[0]-_min[0]));
	assert(j<=(_max[1]-_min[1]));
	assert(k<=(_max[2]-_min[2]));

	double iwgt = 0.0;
	double jwgt = 0.0;
	double kwgt = 0.0;

	if (_delta[0] != 0.0) iwgt = ((x - _minu[0]) - (i * _delta[0])) / _delta[0];
	if (_delta[1] != 0.0) jwgt = ((y - _minu[1]) - (j * _delta[1])) / _delta[1];
	if (_delta[2] != 0.0) kwgt = ((z - _minu[2]) - (k * _delta[2])) / _delta[2];

	if (iwgt>0.5) i++;
	if (jwgt>0.5) j++;
	if (kwgt>0.5) k++;

	return(AccessIJK(i,j,k));
}

float RegularGrid::_GetValueLinear(double x, double y, double z) const {

	size_t i = 0;
	size_t j = 0;
	size_t k = 0;

	if (_delta[0] != 0.0) i = (size_t) floor ((x-_minu[0]) / _delta[0]);
	if (_delta[1] != 0.0) j = (size_t) floor ((y-_minu[1]) / _delta[1]);
	if (_delta[2] != 0.0) k = (size_t) floor ((z-_minu[2]) / _delta[2]);

	assert(i<=(_max[0]-_min[0]));
	assert(j<=(_max[1]-_min[1]));
	assert(k<=(_max[2]-_min[2]));

	double iwgt = 0.0;
	double jwgt = 0.0;
	double kwgt = 0.0;

	if (_delta[0] != 0.0) iwgt = ((x - _minu[0]) - (i * _delta[0])) / _delta[0];
	if (_delta[1] != 0.0) jwgt = ((y - _minu[1]) - (j * _delta[1])) / _delta[1];
	if (_delta[2] != 0.0) kwgt = ((z - _minu[2]) - (k * _delta[2])) / _delta[2];

	double p0,p1,p2,p3,p4,p5,p6,p7;

	p0 = AccessIJK(i,j,k); 
	if (p0 == _missingValue) return (_missingValue);

	if (iwgt!=0.0) {
		p1 = AccessIJK(i+1,j,k);
		if (p1 == _missingValue) return (_missingValue);
	}
	else p1 = 0.0;

	if (jwgt!=0.0) {
		p2 = AccessIJK(i,j+1,k);
		if (p2 == _missingValue) return (_missingValue);
	}
	else p2 = 0.0;

	if (iwgt!=0.0 && jwgt!=0.0) {
		p3 = AccessIJK(i+1,j+1,k);
		if (p3 == _missingValue) return (_missingValue);
	}
	else p3 = 0.0;

	if (kwgt!=0.0) {
		p4 = AccessIJK(i,j,k+1); 
		if (p4 == _missingValue) return (_missingValue);
	}
	else p4 = 0.0;

	if (kwgt!=0.0 && iwgt!=0.0) {
		p5 = AccessIJK(i+1,j,k+1);
		if (p5 == _missingValue) return (_missingValue);
	}
	else p5 = 0.0;

	if (kwgt!=0.0 && jwgt!=0.0) {
		p6 = AccessIJK(i,j+1,k+1);
		if (p6 == _missingValue) return (_missingValue);
	}
	else p6 = 0.0;

	if (kwgt!=0.0 && iwgt!=0.0 && jwgt!=0.0) {
		p7 = AccessIJK(i+1,j+1,k+1);
		if (p7 == _missingValue) return (_missingValue);
	}
	else p7 = 0.0;


	double c0 = p0+iwgt*(p1-p0) + jwgt*((p2+iwgt*(p3-p2))-(p0+iwgt*(p1-p0)));
	double c1 = p4+iwgt*(p5-p4) + jwgt*((p6+iwgt*(p7-p6))-(p4+iwgt*(p5-p4)));

	return(c0+kwgt*(c1-c0));

}

void RegularGrid::_ClampCoord(double &x, double &y, double &z) const {

	if (_minu[0]<_maxu[0]) {
		if (x<_minu[0] && _periodic[0]) {
			while (x<_minu[0]) x+= _maxu[0]-_minu[0];
		}
		if (x>_maxu[0] && _periodic[0]) {
			while (x>_maxu[0]) x-= _maxu[0]-_minu[0];
		}
	}
	else {
		if (x>_minu[0] && _periodic[0]) {
			while (x>_minu[0]) x+= _maxu[0]-_minu[0];
		}
		if (x<_maxu[0] && _periodic[0]) {
			while (x<_maxu[0]) x-= _maxu[0]-_minu[0];
		}
	}

	if (_minu[1]<_maxu[1]) {
		if (y<_minu[1] && _periodic[1]) {
			while (y<_minu[1]) y+= _maxu[1]-_minu[1];
		}
		if (y>_maxu[1] && _periodic[1]) {
			while (y>_maxu[1]) y-= _maxu[1]-_minu[1];
		}
	}
	else {
		if (y>_minu[1] && _periodic[1]) {
			while (y>_minu[1]) y+= _maxu[1]-_minu[1];
		}
		if (y<_maxu[1] && _periodic[1]) {
			while (y<_maxu[1]) y-= _maxu[1]-_minu[1];
		}
	}

	if (_minu[2]<_maxu[2]) {
		if (z<_minu[2] && _periodic[2]) {
			while (z<_minu[2]) z+= _maxu[2]-_minu[2];
		}
		if (z>_maxu[2] && _periodic[2]) {
			while (z>_maxu[2]) z-= _maxu[2]-_minu[2];
		}
	}
	else {
		if (z>_minu[2] && _periodic[2]) {
			while (z>_minu[2]) z+= _maxu[2]-_minu[2];
		}
		if (z<_maxu[2] && _periodic[2]) {
			while (z<_maxu[2]) z-= _maxu[2]-_minu[2];
		}
	}

	//
	// Handle coordinates for dimensions of length 1
	//
	if (_min[0] == _max[0]) x = _minu[0];
	if (_min[1] == _max[1]) y = _minu[1];
	if (_min[2] == _max[2]) z = _minu[2];
}

void RegularGrid::GetUserExtents(double extents[6]) const {
	for (int i=0; i<3; i++) {
		extents[i] = _minu[i];
		extents[i+3] = _maxu[i];
	}
}

void RegularGrid::GetDimensions(size_t dims[3]) const {
	for (int i=0; i<3; i++) dims[i] = _max[i]-_min[i]+1;
}


void RegularGrid::SetInterpolationOrder(int order) {
	if (order<0 || order>1) order = 1;
	_interpolationOrder = order;
}

int RegularGrid::GetUserCoordinates(
    size_t i, size_t j, size_t k,
    double *x, double *y, double *z
) const {

	if (i>_max[0]-_min[0]) return(-1);
	if (j>_max[1]-_min[1]) return(-1);
	if (k>_max[2]-_min[2]) return(-1);

	if (_minu[0]<_maxu[0]) {
		*x = i * _delta[0] + _minu[0];
	}
	else {
		*x = i * _delta[0] * -1.0 + _minu[0];
	}

	if (_minu[1]<_maxu[1]) {
		*y = j * _delta[1] + _minu[1];
	}
	else {
		*y = j * _delta[1] * -1.0 + _minu[1];
	}

	if (_minu[2]<_maxu[2]) {
		*z = k * _delta[2] + _minu[2];
	}
	else {
		*z = k * _delta[2] * -1.0 + _minu[2];
	}
	return(0);

}

void RegularGrid::GetIJKIndex(
    double x, double y, double z,
    size_t *i, size_t *j, size_t *k
) const {

	_ClampCoord(x,y,z);

	// Make sure the point xyz is within the bounds _minu, _maxu. 
	//
	if (_minu[0]<_maxu[0]) {
		if (x<_minu[0]) x = _minu[0];
		if (x>_maxu[0]) x = _maxu[0];
	}
	else {
		if (x>_minu[0]) x = _minu[0];
		if (x<_maxu[0]) x = _maxu[0];
	}
	if (_minu[1]<_maxu[1]) {
		if (y<_minu[1]) y = _minu[1];
		if (y>_maxu[1]) y = _maxu[1];
	}
	else {
		if (y>_minu[1]) y = _minu[1];
		if (y<_maxu[1]) y = _maxu[1];
	}
	if (_minu[2]<_maxu[2]) {
		if (z<_minu[2]) z = _minu[2];
		if (z>_maxu[2]) z = _maxu[2];
	}
	else {
		if (z>_minu[2]) z = _minu[2];
		if (z<_maxu[2]) z = _maxu[2];
	}

	*i = *j = *k = 0;

	if (_delta[0] != 0.0) *i = (size_t) floor ((x-_minu[0]) / _delta[0]);
	if (_delta[1] != 0.0) *j = (size_t) floor ((y-_minu[1]) / _delta[1]);
	if (_delta[2] != 0.0) *k = (size_t) floor ((z-_minu[2]) / _delta[2]);

	assert(*i<=(_max[0]-_min[0]));
	assert(*j<=(_max[1]-_min[1]));
	assert(*k<=(_max[2]-_min[2]));

	double iwgt = 0.0;
	double jwgt = 0.0;
	double kwgt = 0.0;

	if (_delta[0] != 0.0) iwgt = ((x - _minu[0]) - (*i * _delta[0]))/_delta[0];
	if (_delta[1] != 0.0) jwgt = ((y - _minu[1]) - (*j * _delta[1]))/_delta[1];
	if (_delta[2] != 0.0) kwgt = ((z - _minu[2]) - (*k * _delta[2]))/_delta[2];

	if (iwgt>0.5) *i += 1;
	if (jwgt>0.5) *j += 1;
	if (kwgt>0.5) *k += 1;

}

void RegularGrid::GetIJKIndexFloor(
    double x, double y, double z,
    size_t *i, size_t *j, size_t *k
) const {

	_ClampCoord(x,y,z);

	// Make sure the point xyz is within the bounds _minu, _maxu. 
	//
	if (_minu[0]<_maxu[0]) {
		if (x<_minu[0]) x = _minu[0];
		if (x>_maxu[0]) x = _maxu[0];
	}
	else {
		if (x>_minu[0]) x = _minu[0];
		if (x<_maxu[0]) x = _maxu[0];
	}
	if (_minu[1]<_maxu[1]) {
		if (y<_minu[1]) y = _minu[1];
		if (y>_maxu[1]) y = _maxu[1];
	}
	else {
		if (y>_minu[1]) y = _minu[1];
		if (y<_maxu[1]) y = _maxu[1];
	}
	if (_minu[2]<_maxu[2]) {
		if (z<_minu[2]) z = _minu[2];
		if (z>_maxu[2]) z = _maxu[2];
	}
	else {
		if (z>_minu[2]) z = _minu[2];
		if (z<_maxu[2]) z = _maxu[2];
	}

	*i = *j = *k = 0;

	if (_delta[0] != 0.0) *i = (size_t) floor ((x-_minu[0]) / _delta[0]);
	if (_delta[1] != 0.0) *j = (size_t) floor ((y-_minu[1]) / _delta[1]);
	if (_delta[2] != 0.0) *k = (size_t) floor ((z-_minu[2]) / _delta[2]);

	assert(*i<=(_max[0]-_min[0]));
	assert(*j<=(_max[1]-_min[1]));
	assert(*k<=(_max[2]-_min[2]));

}

void RegularGrid::GetRange(float range[2]) const {

	RegularGrid::ConstIterator itr;
	bool first = true;
	range[0] = range[1] = _missingValue;
	for (itr = this->begin(); itr!=this->end(); ++itr) {
		if (first && *itr != _missingValue) {
			range[0] = range[1] = *itr;
			first = false;
		}

		if (! first) {
			if (*itr < range[0] && *itr != _missingValue) range[0] = *itr;
			else if (*itr > range[1] && *itr != _missingValue) range[1] = *itr;
		}
	}
}


int RegularGrid::Reshape(
	const size_t min[3],
	const size_t max[3],
	const bool periodic[3]
) {
	size_t lmin[3], lmax[3];
	for (int i=0; i<3; i++) {

		if (min[i]>max[i]) return(-1);

		if (((max[i]/_bs[i]) - (min[i]/_bs[i]) + 1) != _bdims[i]) return(-1);

		size_t l = max[i] - min[i];
		lmin[i] = min[i] % _bs[i];
		lmax[i] = lmin[i] + l;
	}

	double minu[3], maxu[3];
	RegularGrid::GetUserCoordinates(
		lmin[0],lmin[1],lmin[2], &minu[0],&minu[1],&minu[2]
	);
	RegularGrid::GetUserCoordinates(
		lmax[0],lmax[1],lmax[2], &maxu[0],&maxu[1],&maxu[2]
	);

	for (int i=0; i<3; i++) {
		_minabs[i] = min[i];
		_maxabs[i] = max[i];
		_min[i] = min[i] % _bs[i];
		_max[i] = _min[i] + (max[i]-min[i]);
		_minu[i] = minu[i];
		_maxu[i] = maxu[i];
		_periodic[i] = periodic[i];
	}

	ResetItr();

	return(0);
}

bool RegularGrid::InsideGrid(double x, double y, double z) const
{
	if (_minu[0]<_maxu[0]) {
		if (x<_minu[0] || x>_maxu[0]) return(false);
	}
	else {
		if (x>_minu[0] || x<_maxu[0]) return(false);
	}
	if (_minu[1]<_maxu[1]) {
		if (y<_minu[1] || y>_maxu[1]) return(false);
	}
	else {
		if (y>_minu[1] || y<_maxu[1]) return(false);
	}
	if (_minu[2]<_maxu[2]) {
		if (z<_minu[2] || z>_maxu[2]) return(false);
	}
	else {
		if (z>_minu[2] || z<_maxu[2]) return(false);
	}

	return(true);
	
}



float RegularGrid::Next() {

	if (_end) return(0);

	if (_xb<_bs[0] && _x<_max[0]) {
		_xb++;
		_x++;
		return(*_itr++);
	}

	_xb = 0;
	if (_x > _max[0]) {
		_x = _xb = _min[0];
		_y++;
	}
	if (_y > _max[1]) {
		_y = _min[1];
		_z++;
	}
	if (_z > _max[2]) {
		_end = true;
		return(0);
	}

	size_t xb = _x / _bs[0];
	size_t yb = _y / _bs[1];
	size_t zb = _z / _bs[2];
	size_t x = _x % _bs[0];
	size_t y = _y % _bs[1];
	size_t z = _z % _bs[2];
	float *blk = _blks[zb*_bdims[0]*_bdims[1] + yb*_bdims[0] + xb];
	_itr = &blk[z*_bs[0]*_bs[1] + y*_bs[0] + x];
	_x++;
	_xb++;
	return(*_itr++);
}

void RegularGrid::ResetItr() {
	_xb = _min[0];
	_x = _min[0];
	_y = _min[1];
	_z = _min[2];
	_itr = &_blks[0][_z*_bs[0]*_bs[1] + _y*_bs[0] + _x];
	_end = false;
}
RegularGrid::Iterator::Iterator(RegularGrid *rg) {
	_rg = rg;
	_xb = rg->_min[0];
	_x = rg->_min[0];
	_y = rg->_min[1];
	_z = rg->_min[2];
	_itr = &rg->_blks[0][_z*rg->_bs[0]*rg->_bs[1] + _y*rg->_bs[0] + _x];
	_end = false;
}

RegularGrid::Iterator::Iterator() {
	_rg = NULL;
	_xb = 0;
	_x = 0;
	_y = 0;
	_z = 0;
	_itr = NULL;
	_end = true;
}

RegularGrid::Iterator &RegularGrid::Iterator::operator++() {
	if (_end) return(*this);

	_xb++;
	_itr++;
	_x++;
	if (_xb < _rg->_bs[0] && _x<_rg->_max[0]) {
		return(*this);
	}

	_xb = 0;
	if (_x > _rg->_max[0]) {
		_x = _xb = _rg->_min[0];
		_y++;
	}
	if (_y > _rg->_max[1]) {
		_y = _rg->_min[1];
		_z++;
	}
	if (_z > _rg->_max[2]) {
		_end = true;
		return(*this);
	}

	size_t xb = _x / _rg->_bs[0];
	size_t yb = _y / _rg->_bs[1];
	size_t zb = _z / _rg->_bs[2];
	size_t x = _x % _rg->_bs[0];
	size_t y = _y % _rg->_bs[1];
	size_t z = _z % _rg->_bs[2];
	float *blk = _rg->_blks[zb*_rg->_bdims[0]*_rg->_bdims[1] + yb*_rg->_bdims[0] + xb];
	_itr = &blk[z*_rg->_bs[0]*_rg->_bs[1] + y*_rg->_bs[0] + x];
	return(*this);
}

RegularGrid::Iterator RegularGrid::Iterator::operator++(int) {
	if (_end) return(*this);

	Iterator temp(*this);
	++(*this);
	return(temp);
}

bool RegularGrid::Iterator::operator!=(const Iterator &other) {
	if (this->_end && other._end) return(false);

	return(!(
		this->_rg == other._rg &&
		this->_xb == other._xb &&
		this->_x == other._x &&
		this->_y == other._y &&
		this->_z == other._z &&
		this->_itr == other._itr &&
		this->_end == other._end
	));
}



RegularGrid::ConstIterator::ConstIterator(const RegularGrid *rg) {
	_rg = rg;
	_xb = rg->_min[0];
	_x = rg->_min[0];
	_y = rg->_min[1];
	_z = rg->_min[2];
	_itr = &rg->_blks[0][_z*rg->_bs[0]*rg->_bs[1] + _y*rg->_bs[0] + _x];
	_end = false;
}

RegularGrid::ConstIterator::ConstIterator() {
	_rg = NULL;
	_xb = 0;
	_x = 0;
	_y = 0;
	_z = 0;
	_itr = NULL;
	_end = true;
}

RegularGrid::ConstIterator &RegularGrid::ConstIterator::operator++() {
	if (_end) return(*this);

	_xb++;
	_itr++;
	_x++;
	if (_xb < _rg->_bs[0] && _x<_rg->_max[0]) {
		return(*this);
	}

	_xb = 0;
	if (_x > _rg->_max[0]) {
		_x = _xb = _rg->_min[0];
		_y++;
	}
	if (_y > _rg->_max[1]) {
		_y = _rg->_min[1];
		_z++;
	}
	if (_z > _rg->_max[2]) {
		_end = true;
		return(*this);
	}

	size_t xb = _x / _rg->_bs[0];
	size_t yb = _y / _rg->_bs[1];
	size_t zb = _z / _rg->_bs[2];
	size_t x = _x % _rg->_bs[0];
	size_t y = _y % _rg->_bs[1];
	size_t z = _z % _rg->_bs[2];
	float *blk = _rg->_blks[zb*_rg->_bdims[0]*_rg->_bdims[1] + yb*_rg->_bdims[0] + xb];
	_itr = &blk[z*_rg->_bs[0]*_rg->_bs[1] + y*_rg->_bs[0] + x];
	return(*this);
}

RegularGrid::ConstIterator RegularGrid::ConstIterator::operator++(int) {
	if (_end) return(*this);

	ConstIterator temp(*this);
	++(*this);
	return(temp);
}

bool RegularGrid::ConstIterator::operator!=(const ConstIterator &other) {
	if (this->_end && other._end) return(false);

	return(!(
		this->_rg == other._rg &&
		this->_xb == other._xb &&
		this->_x == other._x &&
		this->_y == other._y &&
		this->_z == other._z &&
		this->_itr == other._itr &&
		this->_end == other._end
	));
}

std::ostream &operator<<(std::ostream &o, const RegularGrid &rg)
{
	o << "RegularGrid " << endl
	<< " Dimensions " 
	<< rg._max[0] - rg._min[0] +1 << " " 
	<< rg._max[1] - rg._min[1] +1 << " "
	<< rg._max[2] - rg._min[2] +1 << endl
	<< " Min voxel offset " 
	<< rg._min[0] << " " 
	<< rg._min[1] << " "
	<< rg._min[2] << endl
	<< " Max voxel offset " 
	<< rg._max[0] << " " 
	<< rg._max[1] << " "
	<< rg._max[2] << endl
	<< " Min coord " 
	<< rg._minu[0] << " " 
	<< rg._minu[1] << " "
	<< rg._minu[2] << endl
	<< " Max coord " 
	<< rg._maxu[0] << " " 
	<< rg._maxu[1] << " "
	<< rg._maxu[2] << endl
	<< " Block dimensions" 
	<< rg._bs[0] << " " 
	<< rg._bs[1] << " "
	<< rg._bs[2] << endl
	<< " Periodicity " 
	<< rg._periodic[0] << " " 
	<< rg._periodic[1] << " "
	<< rg._periodic[2] << endl
	<< " Missing value " << rg._missingValue << endl
	<< " Interpolation order " << rg._interpolationOrder << endl;

	return o;
}


#ifdef	DEAD

const size_t NX = 512;
const size_t NY = 512;
const size_t NZ = 512;

const size_t XOFF = 0;
const size_t YOFF = 0;
const size_t ZOFF = 0;

const size_t NXB = 64;
const size_t NYB = 64;
const size_t NZB = 64;

main(int argc, char **argv) {
	assert(XOFF<NXB);
	assert(YOFF<NYB);
	assert(ZOFF<NZB);

	size_t dimsblk[] = {
		(size_t) ceil((double) (XOFF+NX) / (double) NXB),
		(size_t) ceil((double) (YOFF+NY) / (double) NYB),
		(size_t) ceil((double) (ZOFF+NZ) / (double) NZB)
	};
	size_t dims[] = {dimsblk[0]*NXB, dimsblk[1]*NYB, dimsblk[2]*NZB};

	size_t bs[] = {NXB,NYB,NZB};
	size_t min[] = {XOFF,YOFF,ZOFF};
	size_t max[] = {XOFF+NX-1, YOFF+NY-1, ZOFF+NZ-1};
	size_t block_size = bs[0]*bs[1]*bs[2];
	double extents[] = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
	bool periodic[] = {false, false, false};

	float *buf = new float[dims[0]*dims[1]*dims[2]];
	float *array = new float[dims[0]*dims[1]*dims[2]];

	float **blks = new float *[dims[0]*dims[1]*dims[2]/block_size];

	for (size_t i = 0; i<dims[0]*dims[1]*dims[2]/block_size; i++) {
		blks[i] = &buf[i*block_size];
	}

	RegularGrid rg(bs, min, max, extents, periodic, blks);


	size_t index;
	float v = 0.0;
	for (size_t z=ZOFF; z<NZ+ZOFF; z++) {
	for (size_t y=YOFF; y<NY+YOFF; y++) {
	for (size_t x=XOFF; x<NX+XOFF; x++) {
		index = z*dims[1]*dims[0] + y*dims[0] + x;
		array[index] = v;
		rg.AccessIJK(x-XOFF,y-YOFF,z-ZOFF) = v;
		v+= 1.0;
	}
	}
	}


	double t0;
	t0 = GetTime();

	cout << "Direct access, unit stride start" << endl;
	double total1 = 0.0;
	for (size_t z=ZOFF; z<NZ+ZOFF; z++) {
	for (size_t y=YOFF; y<NY+YOFF; y++) {
	for (size_t x=XOFF; x<NX+XOFF; x++) {
		index = z*dims[1]*dims[0] + y*dims[0] + x;
		total1 += array[index];
	}
	}
	}
	cout << "Direct access time : " << GetTime() - t0 << endl;


	double total2 = 0.0;
	t0 = GetTime();
	cout << "Class access, unit stride" << endl;
	for (size_t z=ZOFF; z<NZ+ZOFF; z++) {
	for (size_t y=YOFF; y<NY+YOFF; y++) {
	for (size_t x=XOFF; x<NX+XOFF; x++) {
		total2 += rg.AccessIJK(x-XOFF,y-YOFF,z-ZOFF);
	}
	}
	}
	if (total1 != total2) {
		cerr << "Mismatch" << endl;
		exit(1);
	}
	cout << "Class access time : " << GetTime() - t0 << endl;


	t0 = GetTime();
	cout << "Class GetNext access, unit stride" << endl;
	total2 = 0.0;
	for (size_t z=ZOFF; z<NZ+ZOFF; z++) {
	for (size_t y=YOFF; y<NY+YOFF; y++) {
	for (size_t x=XOFF; x<NX+XOFF; x++) {
		v = rg.Next();
		total2 += v;
	}
	}
	}
	if (total1 != total2) {
		cerr << "Mismatch" << endl;
		exit(1);
	}
	cout << "Class Next access time : " << GetTime() - t0 << endl;

	t0 = GetTime();
	cout << "Class iterator accss" << endl;
	total2 = 0.0;
	RegularGrid::Iterator itr;
	for (itr = rg.begin(); itr!=rg.end(); ++itr) {
		v = *itr;
		total2 += v;
	}
	if (total1 != total2) {
		cerr << "Mismatch" << endl;
		exit(1);
	}
	cout << "Class iterator access time : " << GetTime() - t0 << endl;


#ifdef	DEAD

	cout << "Direct access, non-unit stride start" << endl;
	total1 = 0.0;
	for (size_t x=XOFF; x<NX+XOFF; x++) {
	for (size_t y=YOFF; y<NY+YOFF; y++) {
	for (size_t z=ZOFF; z<NZ+ZOFF; z++) {
		index = z*NX*NY + y*NX +x;
		total1 += array[index];
	}
	}
	}

	cout << "Direct access time : " << GetTime() - t0 << endl;


	t0 = GetTime();
	cout << "Class access, non-unit stride" << endl;
	total2 = 0.0;
	for (size_t x=XOFF; x<NX+XOFF; x++) {
	for (size_t y=YOFF; y<NY+YOFF; y++) {
	for (size_t z=ZOFF; z<NZ+ZOFF; z++) {
		total2 += rg.AccessIJK(x-XOFF,y-YOFF,z-ZOFF);
	}
	}
	}
	cout << "Class access time : " << GetTime() - t0 << endl;
	if (total1 != total2) {
		cerr << "Mismatch" << endl;
		exit(1);
	}
#endif






	t0 = GetTime();
	double deltax = 1.0 / (double) (NX-1);
	double deltay = 1.0 / (double) (NY-1);
	double xf = 0.0;
	double yf = 0.0;
	for (size_t y=YOFF; y<NY+YOFF; y++) {
		xf = 0.0;
		for (size_t x=XOFF; x<NX+YOFF; x++) {
			total2 += rg.GetValue(xf,yf,0.5);
			xf += deltax;
		}
		yf += deltay;
	}
	cout << "Class access XY slice time : " << GetTime() - t0 << endl;

	t0 = GetTime();
	deltax = 1.0 / (double) (NX-1);
	double deltaz = 1.0 / (double) (NZ-1);
	xf = 0.0;
	double zf = 0.0;
	for (size_t z=ZOFF; z<NZ+ZOFF; z++) {
		xf = 0.0;
		for (size_t x=XOFF; x<NX+XOFF; x++) {
			total2 += rg.GetValue(xf,0.5,zf);
			xf += deltax;
		}
		zf += deltay;
	}
	cout << "Class access XZ slice time : " << GetTime() - t0 << endl;


	t0 = GetTime();
	deltay = 1.0 / (double) (NY-1);
	deltaz = 1.0 / (double) (NZ-1);
	yf = 0.0;
	zf = 0.0;
	for (size_t z=ZOFF; z<NZ+ZOFF; z++) {
		yf = 0.0;
		for (size_t y=YOFF; y<NY+YOFF; y++) {
			total2 += rg.GetValue(0.5,yf, zf);
			yf += deltax;
		}
		zf += deltay;
	}
	cout << "Class access YZ slice time : " << GetTime() - t0 << endl;
}
#endif
