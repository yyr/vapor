
#include <iostream>
#include <proj_api.h>
#include <vapor/Proj4API.h>

using namespace VAPoR;
using namespace VetsUtil;


Proj4API::Proj4API() {
	_pjSrc = NULL;
	_pjDst = NULL;
}

Proj4API::~Proj4API() {
	if (_pjSrc) pj_free(_pjSrc);
	if (_pjDst) pj_free(_pjDst);
}

int Proj4API::Initialize(string srcdef, string dstdef) {

	if (_pjSrc) pj_free(_pjSrc);
	if (_pjDst) pj_free(_pjDst);
	_pjSrc = NULL;
	_pjDst = NULL;

	if (srcdef.empty() && dstdef.empty()) {
		SetErrMsg("Source and dest.  definition strings can't both be empty");
		return(-1);
	}

	if (! srcdef.empty()) {
		_pjSrc = pj_init_plus(srcdef.c_str());
		if (! _pjSrc) {
			SetErrMsg("pj_init_plus(%s) : %s",srcdef.c_str(),ProjErr().c_str());
			return(-1);
		}
	}

	if (! dstdef.empty()) {
		_pjDst = pj_init_plus(dstdef.c_str());
		if (! _pjDst) {
			SetErrMsg("pj_init_plus(%s) : %s",dstdef.c_str(),ProjErr().c_str());
			return(-1);
		}
	}

	//
	// If either the source or destination definition string is 
	// not provided - but not both - generate a "latlong" conversion
	//
	if (srcdef.empty()) {
		_pjSrc = pj_latlong_from_proj(_pjDst);
		if (! _pjSrc) {
			SetErrMsg("pj_latlong_from_proj() : %s", ProjErr().c_str());
			return(-1);
		}
	}
	else if (dstdef.empty()) {
		_pjDst = pj_latlong_from_proj(_pjSrc);
		if (! _pjDst) {
			SetErrMsg("pj_latlong_from_proj() : %s", ProjErr().c_str());
			return(-1);
		}
	}
	return(0);
}

bool Proj4API::IsLatLonSrc() const {
	if (! _pjSrc) return(false);

	return((bool) pj_is_latlong(_pjSrc));
}

bool Proj4API::IsLatLonDst() const {
	if (! _pjDst) return(false);

	return((bool) pj_is_latlong(_pjDst));
}

bool Proj4API::IsGeocentSrc() const {
	if (! _pjSrc) return(false);

	return((bool) pj_is_geocent(_pjSrc));
}

bool Proj4API::IsGeocentDst() const {
	if (! _pjDst) return(false);

	return((bool) pj_is_geocent(_pjDst));
}

string Proj4API::GetSrcStr() const {
	if (! _pjSrc) return("");

	return((string) pj_get_def(_pjSrc, 0));
}

string Proj4API::GetDstStr() const {
	if (! _pjDst) return("");

	return((string) pj_get_def(_pjDst, 0));
}

int Proj4API::Transform(double *x, double *y, size_t n, int offset) const {

	return(Proj4API::Transform(x,y,NULL,n,offset));
}

int Proj4API::Transform(
	double *x, double *y, double *z, size_t n, int offset
) const {
	if (_pjSrc == NULL || _pjDst == NULL) {
		SetErrMsg("Not initialized");
		return(-1);
	}

	//
	// Convert from degrees to radians if source is in 
	// geographic coordinates
	//
	if (pj_is_latlong(_pjSrc)) {
		if (x) {
			for (size_t i=0; i<n; i++) {
				x[i * (size_t) offset] *= DEG_TO_RAD;
			}
		}
		if (y) {
			for (size_t i=0; i<n; i++) {
				y[i * (size_t) offset] *= DEG_TO_RAD;
			}
		}
		if (z) {
			for (size_t i=0; i<n; i++) {
				z[i * (size_t) offset] *= DEG_TO_RAD;
			}
		}
	}

	int rc = pj_transform(_pjSrc, _pjDst, n, offset, x, y, NULL);
	if (rc != 0) {
		SetErrMsg("pj_transform() : %s", ProjErr().c_str());
		return(-1);
	}

	//
	// Convert from radians degrees if destination is in 
	// geographic coordinates
	//
	if (pj_is_latlong(_pjDst)) {
		if (x) {
			for (size_t i=0; i<n; i++) {
				x[i * (size_t) offset] *= RAD_TO_DEG;
			}
		}
		if (y) {
			for (size_t i=0; i<n; i++) {
				y[i * (size_t) offset] *= RAD_TO_DEG;
			}
		}
		if (z) {
			for (size_t i=0; i<n; i++) {
				z[i * (size_t) offset] *= RAD_TO_DEG;
			}
		}
	}
	return(0);
}

int Proj4API::Transform(float *x, float *y, size_t n, int offset) const {

	return(Proj4API::Transform(x,y,NULL,n,offset));
}

int Proj4API::Transform(
	float *x, float *y, float *z, size_t n, int offset
) const {
	double *xd = NULL;
	double *yd = NULL;
	double *zd = NULL;

	if (x) {
		xd = new double[n];
		for (size_t i = 0; i<n; i++) xd[i] = x[i];
	}
	if (y) {
		yd = new double[n];
		for (size_t i = 0; i<n; i++) yd[i] = y[i];
	}
	if (z) {
		zd = new double[n];
		for (size_t i = 0; i<n; i++) zd[i] = z[i];
	}

	int rc = Proj4API::Transform(xd,yd,zd,n,offset);

	if (xd) {
		for (size_t i = 0; i<n; i++) x[i] = xd[i];
		delete [] xd;
	}
	if (yd) {
		for (size_t i = 0; i<n; i++) y[i] = yd[i];
		delete [] yd;
	}
	if (zd) {
		for (size_t i = 0; i<n; i++) z[i] = zd[i];
		delete [] zd;
	}
	return(rc);
}


string Proj4API::ProjErr() const {
	return (pj_strerrno(*pj_get_errno_ref()));
}

