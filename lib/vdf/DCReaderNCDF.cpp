//
//      $Id$
//



#include <vapor/DCReaderNCDF.h>
#ifdef WIN32
#pragma warning(disable : 4251)
#endif

using namespace VAPoR;
using namespace std;

DCReaderNCDF::DCReaderNCDF(
	const std::vector <string> &files, 
	const std::vector <string> &time_dimnames,
    const std::vector <string> &time_coordvars, 
	const std::vector <string> &staggered_dims, 
	string missing_attr, size_t dims[3]
) {

	_dims.clear();
	_vars3d.clear();
	_vars3dExcluded.clear();
	_vars2dXY.clear();
	_vars2dXZ.clear();
	_vars2dYZ.clear();
	_vars2dExcluded.clear();
	_ovr_fd = -1;

	_ncdfC = new NetCDFCollection();

	_ncdfC->SetStaggeredDims(staggered_dims);
	_ncdfC->SetMissingValueAttName(missing_attr);

	int rc = _ncdfC->Initialize(files, time_dimnames, time_coordvars);
	if (rc<0) {
		SetErrMsg("Failed to initialize netCDF data collection for reading");
		return;
	}
	

	//
	// Get all of the 3D and 2d variables in the collection
	//
	vector <string> candidate_vars3d = _ncdfC->GetVariableNames(3, true);
	vector <string> candidate_vars2d = _ncdfC->GetVariableNames(2, true);

	//
	// Figure out what the grid dimensions are if none were provided.
	// Use the dimesions of the first 3D variable in the list. If none,
	// use the dimensions of the first 2D variable
	//
	if (dims) {
		for (int i=0; i<3; i++) _dims.push_back(dims[i]);
	}
	else if (candidate_vars3d.size()) {
		vector <size_t> dimsv = _GetSpatialDims(candidate_vars3d[0]);
		for (int i=0; i<3; i++) _dims.push_back(dimsv[i]);
	}
	else if (candidate_vars2d.size()) {
		vector <size_t> dimsv = _GetSpatialDims(candidate_vars2d[0]);
		for (int i=0; i<2; i++) _dims.push_back(dimsv[i]);
		_dims.push_back(1);
	}
	else {
		for (int i=0; i<3; i++) _dims.push_back(1);
		return;	// No data!!
	}

	//
	// Now segregate 3d variables - those that match the grid dimension
	// and those that don't
	//
	for (int i=0; i<candidate_vars3d.size(); i++) {
		vector <size_t> dims = _GetSpatialDims(candidate_vars3d[i]);

		if (dims == _dims) _vars3d.push_back(candidate_vars3d[i]);
		else _vars3dExcluded.push_back(candidate_vars3d[i]);
	}
	vector <size_t> dims2dXY;
	vector <size_t> dims2dXZ;
	vector <size_t> dims2dYZ;

	dims2dXY.push_back(_dims[0]);
	dims2dXY.push_back(_dims[1]);

	dims2dXZ.push_back(_dims[0]);
	dims2dXZ.push_back(_dims[2]);

	dims2dYZ.push_back(_dims[1]);
	dims2dYZ.push_back(_dims[2]);

	for (int i=0; i<candidate_vars2d.size(); i++) {
		vector <size_t> dims = _GetSpatialDims(candidate_vars2d[i]);

		if (dims == dims2dXY) {
			_vars2dXY.push_back(candidate_vars2d[i]);
		}
		else if (dims == dims2dXZ) {
			_vars2dXZ.push_back(candidate_vars2d[i]);
		}
		else if (dims == dims2dYZ) {
			_vars2dYZ.push_back(candidate_vars2d[i]);
		}
		else {
			_vars2dExcluded.push_back(candidate_vars2d[i]);
		}
	}

    sort(_vars3d.begin(), _vars3d.end());
    sort(_vars2dXY.begin(), _vars2dXY.end());
    sort(_vars2dXZ.begin(), _vars2dXZ.end());
    sort(_vars2dYZ.begin(), _vars2dYZ.end());
    sort(_vars3dExcluded.begin(), _vars3dExcluded.end());
    sort(_vars2dExcluded.begin(), _vars2dExcluded.end());
}

vector <size_t> DCReaderNCDF::_GetSpatialDims(string varname) const {
	vector <size_t> ncdfdims = _ncdfC->GetSpatialDims(varname);
	vector <size_t> dims;

	// reverse the order
	for (int i=ncdfdims.size()-1; i>=0; --i) {
		dims.push_back(ncdfdims[i]);
	}
	return(dims);
}

DCReaderNCDF::~DCReaderNCDF() {
	if (_ncdfC) delete _ncdfC;
}

void   DCReaderNCDF::GetGridDim(size_t dim[3]) const {
	for (int i=0; i<3; i++) dim[i] = _dims[i];
}

void DCReaderNCDF::GetBlockSize(size_t bs[3], int ) const {
	DCReaderNCDF::GetGridDim(bs);
}

string DCReaderNCDF::GetCoordSystemType() const {
	return("cartesian"); 
}

string DCReaderNCDF::GetGridType() const {
	return("regular"); 
}

std::vector <double> DCReaderNCDF::GetExtents(size_t ts) const {
	vector <double> d;
	d.push_back(0); d.push_back(0); d.push_back(0);
	d.push_back(1.0); d.push_back(1.0); d.push_back(1.0);
	return(d);
}

vector <string> DCReaderNCDF::GetCoordinateVariables() const {
    vector <string> v;
    v.push_back("NONE"); v.push_back("NONE"); v.push_back("NONE");
    return(v);
}

vector<long> DCReaderNCDF::GetPeriodicBoundary() const {
	vector <long> p;
	p.push_back(0); p.push_back(0); p.push_back(0);
	return(p);
}

vector<long> DCReaderNCDF::GetGridPermutation() const {
	vector <long> p;
	p.push_back(0); p.push_back(1); p.push_back(2);
	return(p);
}

double DCReaderNCDF::GetTSUserTime(size_t ts) const {
	double t;
	int rc = _ncdfC->GetTime(ts, t);
	if (rc<0) t = 0.0;
	return(t);
};

void DCReaderNCDF::GetTSUserTimeStamp(size_t ts, string &s) const {
	s.clear();
}

bool DCReaderNCDF::GetMissingValue(string varname, float &value) const {
	double mv;
	bool status;
	status = _ncdfC->GetMissingValue(varname, mv);
	value = (float) mv;
	return(status);
}

bool DCReaderNCDF::IsCoordinateVariable(string varname) const {
	return(false);
}

int DCReaderNCDF::OpenVariableRead(
    size_t timestep, string varname, int, int 
) {
	DCReaderNCDF::CloseVariable();

	_ovr_fd = _ncdfC->OpenRead(timestep, varname);
	return(_ovr_fd);
}

int DCReaderNCDF::CloseVariable() {
	if (_ovr_fd < 0) return(0);

	int rc = _ncdfC->Close(_ovr_fd);
	_ovr_fd = -1;
	return(rc);
}

int DCReaderNCDF::ReadSlice(float *slice) {
	return(_ncdfC->ReadSlice(slice, _ovr_fd));
}
