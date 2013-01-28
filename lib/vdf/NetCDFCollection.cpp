#include <iostream>
#include <algorithm>
#include <netcdf.h>
#include <vapor/NetCDFCollection.h>

using namespace VAPoR;
using namespace VetsUtil;
using namespace std;

NetCDFCollection::NetCDFCollection() {
	_variableList.clear();
	_staggeredDims.clear();
	_missingValAttName.clear();
	_times.clear();
	_ovr_local_ts = 0;
	_ovr_slice = 0;
	_ovr_slicebuf = NULL;
	_ovr_slicebufsz = 0;
	_ovr_has_missing = false;
	_ovr_missing_value = 0.0;
}

int NetCDFCollection::Initialize(
	const vector <string> &files, const vector <string> &time_dimnames, 
	string time_coordvar
) {
	_variableList.clear();
	_missingValAttName.clear();
	_times.clear();
	_ovr_local_ts = 0;
	_ovr_slice = 0;
	_ovr_slicebuf = NULL;
	_ovr_slicebufsz = 0;
	_staggeredDims.clear();
	_ovr_has_missing = false;
	_ovr_missing_value = 0.0;

	for (int i=0; i<files.size(); i++) {
		NetCDFSimple netcdf;
		int rc = netcdf.Initialize(files[i]);
		if (rc<0) {
			SetErrMsg("NetCDFSimple::Initialize(%s)", files[i].c_str());
			return(-1);
		}


		//
		// Get variable info for all variables in current file
		//
		vector <NetCDFSimple::Variable> variables;
		netcdf.GetVariables(variables);

		//
		// If time_coordvar is not empty it specifies the name of variable
		// the contains user times for each time step. Read the user times
		// and store in 'times' vector
		//
		vector <double> times;
		if (time_coordvar.length()) {
			int timeidx = -1;
			for (int j=0; i<variables.size(); j++) {
				if (time_coordvar.compare(variables[j].GetName()) == 0) {
					timeidx = j;
					break;
				} 
			}
			if (timeidx == -1) {
				SetErrMsg(
					"Time coordinate variable \"%s\" not found",
					time_coordvar.c_str()
				);
				return(-1);
			}
			NetCDFSimple::Variable &tvar = variables[timeidx];
			if (tvar.GetDims().size() != 1) {
				SetErrMsg(
					"Time coordinate variable \"%s\" is invalid",
					time_coordvar.c_str()
				);
			}
			int rc = netcdf.OpenRead(files[i].c_str(), tvar);
			if (rc<0) {
				SetErrMsg(
					"Time coordinate variable \"%s\" is invalid",
					time_coordvar.c_str()
				);
				return(-1);
			}
			size_t start[] = {0};
			size_t count[] = {tvar.GetDims()[0]};
			float *timebuf = new float [tvar.GetDims()[0]];
			rc = netcdf.Read(start, count, timebuf);
			if (rc<0) {
				SetErrMsg(
					"Time coordinate variable \"%s\" is invalid",
					time_coordvar.c_str()
				);
				return(-1);
			}
			netcdf.Close();

			for (int j=0; j<tvar.GetDims()[0]; j++) times.push_back(timebuf[j]);

			delete [] timebuf;

		}

		//
		// For each variable in the file add it to _variablesList
		//
		for (int j=0; j<variables.size(); j++) {
			string name = variables[j].GetName();
			map <string,TimeVaryingVar>::iterator p = _variableList.find(name);

			//
			// If this is the first time we've seen this variable
			//
			if (p ==  _variableList.end()) {
				TimeVaryingVar tvv;
				_variableList[name] = tvv;
				p = _variableList.find(name);
			}
			TimeVaryingVar &tvvref = p->second;

			tvvref.Insert(variables[j], files[i], time_dimnames, times);

			for (int j=0; j<times.size(); j++) _times.push_back(times[j]);
		}
	}
	sort(_times.begin(), _times.end());

	vector <double>::iterator last;
	last = unique(_times.begin(), _times.end());
	_times.erase(last, _times.end());

	return(0);
}

bool NetCDFCollection::VariableExists(size_t ts, string varname) const {
	if (ts >= _times.size()) return(false);

	map <string,TimeVaryingVar>::const_iterator p = _variableList.find(varname);
	if (p ==  _variableList.end()) {
		return(false);
	}
	const TimeVaryingVar &tvvar = p->second;

	double mytime = _times[ts];

	vector <double> times = tvvar.GetTimes();
	for (int i=0; i<times.size(); i++) {
		if (times[i] == mytime) return(true);
	}
	return(false);
}

bool NetCDFCollection::IsStaggeredVar(string varname) const {
	map <string,TimeVaryingVar>::const_iterator p = _variableList.find(varname);
	if (p ==  _variableList.end()) {
		return(false);
	}
	const TimeVaryingVar &var = p->second;

	vector <string> dimnames = var.GetSpatialDimNames();

	for (int i=0; i<dimnames.size(); i++) {
		if (IsStaggeredDim(dimnames[i])) return(true);
	}
	return(false);
}

bool NetCDFCollection::IsStaggeredDim(string dimname) const {

	for (int i=0; i<_staggeredDims.size(); i++) {
		if (dimname.compare(_staggeredDims[i]) == 0) return(true);
	}
	return(false);
}

vector <string> NetCDFCollection::GetVariableNames(int ndims) const {
	map <string,TimeVaryingVar>::const_iterator p = _variableList.begin();

	vector <string> names;
	for (; p!=_variableList.end(); ++p) {
		const TimeVaryingVar &tvvars = p->second;
		if (tvvars.GetSpatialDims().size() == ndims) {
			names.push_back(p->first);
		}
	}
	return(names);
}

vector <size_t> NetCDFCollection::GetDims(string varname) const {
	vector <size_t> dims;
	dims.clear();

	map <string,TimeVaryingVar>::const_iterator p = _variableList.find(varname);
	if (p ==  _variableList.end()) {
		return(dims);
	}
	const TimeVaryingVar &tvvars = p->second;

	vector <size_t> mydims = tvvars.GetSpatialDims();
	vector <string> dimnames = tvvars.GetSpatialDimNames();

	//
	// Deal with any staggered dimensions
	//
	for (int i=0; i<mydims.size(); i++) {
		dims.push_back(mydims[i]);
		for (int j=0; j<_staggeredDims.size(); j++) {
			if (dimnames[i].compare(_staggeredDims[j]) == 0) {
				dims[i] = mydims[i] - 1;
			}
		}
	} 
	return(dims);
}


int NetCDFCollection::GetTime(size_t ts, double &time) const {
	time = 0.0;
	if (ts >= _times.size()) {
		SetErrMsg("Invalid time step: %d", ts);
		return(-1);
	}
	time = _times[ts];
	return(0);
}

int NetCDFCollection::GetTimes(
	string varname, vector <double> &times
) const {
	map <string,TimeVaryingVar>::const_iterator p = _variableList.find(varname);
	if (p ==  _variableList.end()) {
		SetErrMsg("Invalid variable \"%s\"", varname.c_str());
		return(-1);
	}
	const TimeVaryingVar &tvvars = p->second;

	times = tvvars.GetTimes();
	return(0);
}

int NetCDFCollection::GetFile(size_t ts, string varname, string &file) const {

	map <string,TimeVaryingVar>::const_iterator p = _variableList.find(varname);
	if (p ==  _variableList.end()) {
		SetErrMsg("Invalid variable \"%s\"", varname.c_str());
		return(-1);
	}
	const TimeVaryingVar &tvvars = p->second;

	double time;
	int rc = GetTime(ts, time);
	if (rc<0) return(-1);

	size_t var_ts;
	rc = tvvars.GetTimeStep(time, var_ts);
	if (rc<0) return(-1);

	return(tvvars.GetFile(var_ts, file));
}

int NetCDFCollection::GetVariableInfo(
	size_t ts, string varname, NetCDFSimple::Variable &variable
) const {

	map <string,TimeVaryingVar>::const_iterator p = _variableList.find(varname);
	if (p ==  _variableList.end()) {
		SetErrMsg("Invalid variable \"%s\"", varname.c_str());
		return(-1);
	}
	const TimeVaryingVar &tvvars = p->second;

	double time;
	int rc = GetTime(ts, time);
	if (rc<0) return(-1);

	size_t var_ts;
	rc = tvvars.GetTimeStep(time, var_ts);
	if (rc<0) return(-1);

	return(tvvars.GetVariableInfo(var_ts, variable));
}

int NetCDFCollection::OpenRead(size_t ts, string varname) {

	map <string,TimeVaryingVar>::const_iterator p = _variableList.find(varname);
	if (p ==  _variableList.end()) {
		SetErrMsg("Invalid variable \"%s\"", varname.c_str());
		return(-1);
	}
	const TimeVaryingVar &tvvars = p->second;

	double time;
	int rc = GetTime(ts, time);
	if (rc<0) return(-1);

	size_t var_ts;
	rc = tvvars.GetTimeStep(time, var_ts);
	if (rc<0) return(-1);

	_ovr_tvvars = tvvars;
	_ovr_local_ts = _ovr_tvvars.GetLocalTimeStep(var_ts);
	_ovr_slice = 0;

	string path;
	NetCDFSimple::Variable varinfo;
	_ovr_tvvars.GetFile(var_ts, path);
	_ovr_tvvars.GetVariableInfo(var_ts, varinfo);

	_ovr_has_missing = false;
	if (_missingValAttName.length()) {
		vector <double> vec;
		varinfo.GetAtt(_missingValAttName, vec);
		if (vec.size()) {
			_ovr_has_missing = true;
			_ovr_missing_value = vec[0];
		}
	}
	
	rc = _ncdf.OpenRead(path, varinfo);
	if (rc<0) {
		SetErrMsg(
			"NetCDFCollection::OpenRead(%d, %s) : failed",
			var_ts, varname.c_str()
		);
		return(-1);
	}
	return(0);

}

int NetCDFCollection::ReadNative(size_t start[], size_t count[], float *data) {
	size_t mystart[NC_MAX_VAR_DIMS];
	size_t mycount[NC_MAX_VAR_DIMS];

	int idx = 0;
	if (_ovr_tvvars.GetTimeVarying()) {
		mystart[idx] = _ovr_local_ts;
		mycount[idx] = 1;
		idx++;
	}
	for (int i=0; i<_ovr_tvvars.GetSpatialDims().size(); i++) {
		mystart[idx] = start[i];
		mycount[idx] = count[i];
		idx++;
	}

	return(_ncdf.Read(mystart, mycount, data));
}
	

int NetCDFCollection::ReadNative(size_t start[], size_t count[], int *data) {
	size_t mystart[NC_MAX_VAR_DIMS];
	size_t mycount[NC_MAX_VAR_DIMS];

	int idx = 0;
	if (_ovr_tvvars.GetTimeVarying()) {
		mystart[idx] = _ovr_local_ts;
		mycount[idx] = 1;
		idx++;
	}
	for (int i=0; i<_ovr_tvvars.GetSpatialDims().size(); i++) {
		mystart[idx] = start[i];
		mycount[idx] = count[i];
		idx++;
	}

	return(_ncdf.Read(mystart, mycount, data));
}

void NetCDFCollection::_InterpolateLine(
	const float *src,
	size_t n,
	size_t stride,
	bool has_missing,
	float mv,
	float *dst
)  const {
	if (! has_missing) {
		for (size_t i=0; i<n-1; i++) {
			dst[i*stride] = 0.5 * (src[i*stride] + src[(i+1)*stride]);
		}
	}
	else {
		for (size_t i=0; i<n-1; i++) {
			if (src[i*stride] == mv || src[(i+1)*stride] == mv) {
				dst[i*stride] = mv;
			}
			else {
				dst[i*stride] = 0.5 * (src[i*stride] + src[(i+1)*stride]);
			}
		}
	}
}

void NetCDFCollection::_InterpolateSlice(
	size_t nx, size_t ny, bool xstag, bool ystag, 
	bool has_missing, float mv, float *slice
) const {

	if (xstag) {

		for (int i=0; i<ny; i++) {
			float *src = slice + (i*nx);
			float *dst = slice + (i*(nx-1));
			_InterpolateLine(src, nx-1, 1, has_missing, mv, dst);
		}
		nx--;
	}

	if (ystag) {
		for (int i=0; i<nx; i++) {
			float *src = slice + i;
			float *dst = slice + i;
			_InterpolateLine(src, ny-1, nx, has_missing, mv, dst);
		}
		ny--;
	}
}

int NetCDFCollection::ReadSliceNative(float *data) {

	const TimeVaryingVar &var = _ovr_tvvars;
	vector <size_t> dims = var.GetSpatialDims();

	size_t nx = dims[dims.size()-1];
	size_t ny = dims[dims.size()-2];
	size_t nz = dims.size() > 2 ? dims[dims.size()-3] : 1;

	if (_ovr_slice >= nz) return(0);

	size_t start[] = {0,0,0};
	size_t count[] = {1,1,1};

	if (dims.size() > 2) {
		start[0] = _ovr_slice;
		count[1] = ny;
		count[2] = nx;
	}
	else {
		count[0] = ny;
		count[1] = nx;
	}
	
	int rc = ReadNative(start, count, data);
	_ovr_slice++;
	if (rc<0) return(rc);
	return(1);
}

	
int NetCDFCollection::ReadSlice(float *data) {
	const TimeVaryingVar &var = _ovr_tvvars;
	vector <size_t> dims = var.GetSpatialDims();
	vector <string> dimnames = var.GetSpatialDimNames();

	if (dims.size() < 2 || dims.size() > 3) {
		SetErrMsg("Only 2D and 3D variables supported");
		return(-1);
	}
	if (! IsStaggeredVar(var.GetName())) {
		return(ReadSliceNative(data));
	}

	bool xstag = IsStaggeredDim(dimnames[dimnames.size()-1]);
	bool ystag = IsStaggeredDim(dimnames[dimnames.size()-2]);
	bool zstag = dims.size() == 3 ? IsStaggeredDim(dimnames[dimnames.size()-3]) : false;

	size_t nx = dims[dims.size()-1];
	size_t ny = dims[dims.size()-2];

	size_t nxus = xstag ? nx-1 : nx;
	size_t nyus = ystag ? ny-1 : ny;

	if (_ovr_slicebufsz < (2*nx*ny*sizeof(*data))) {
		if (_ovr_slicebuf) delete [] _ovr_slicebuf;
		_ovr_slicebuf = (unsigned char *) new float [2*nx*ny];
		_ovr_slicebufsz = 2*nx*ny*sizeof(*data);
	}

	float *buffer = (float *) _ovr_slicebuf;	// cast to float*
	float *slice1 = buffer;
	float *slice2 = NULL;
	if (zstag) {

		//
		// If this is the first read we read the first slice into
		// the front (first half) of buffer, and the second into 
		// the back (second half) of
		// buffer. For all other reads the previous slice is in
		// the front of buffer, so we read the current slice into 
		// the back of buffer
		//
		if (_ovr_slice == 0) {
			slice1 = buffer;
			slice2 = buffer + (nxus * nyus);
		}
		else {
			slice1 = buffer + (nxus * nyus);
			slice2 = buffer;
		}
	}

	int rc = ReadSliceNative(slice1);
	if (rc < 1) return(rc);	// eof or error

	_InterpolateSlice(
		nx, ny, xstag, ystag, _ovr_has_missing, _ovr_missing_value, slice1
	);

	if (zstag) {
		if (_ovr_slice == 1) {	// N.B. ReadSliceNative increments _ovr_slice
			rc = ReadSliceNative(slice2);
			if (rc < 1) return(rc);	// eof or error

			_InterpolateSlice(
				nx, ny, xstag, ystag, 
				_ovr_has_missing, _ovr_missing_value, slice2
			);
		}

		//
		// At this point the ith slice is in the front of buffer
		// and the ith+1 slice is in the back of buffer. Moreover,
		// any horizontal staggering has be taken care of. Hence, the
		// horizontal dimensions are given by nxus x nyus.
		//
		//
		for (int i=0; i < nxus*nyus; i++) {
			float *src = buffer + i;
			float *dst = buffer + i;
			_InterpolateLine(
				src, 2, nxus*nyus, _ovr_has_missing, _ovr_missing_value, dst
			);
		}
	}

	memcpy(data, buffer, sizeof(*data) * nxus * nyus);

	if (zstag) {

		//
		// move the ith+1 slice to the front of the buffer
		//
		memcpy(
			buffer, buffer+(nxus*nyus), 
			sizeof(*data) * nxus * nyus
		);
	}

	return(1);
}

int NetCDFCollection::Read(size_t start[], size_t count[], float *data) {
	SetErrMsg("Not implemented");
	return(-1);
}

int NetCDFCollection::Close() {
	return (_ncdf.Close());
}



NetCDFCollection::TimeVaryingVar::TimeVaryingVar() {
	_variables.clear();
	_files.clear();
	_tvmaps.clear();
	_dims.clear();
	_dim_names.clear();
	_name.clear();
	_time_varying = false;
}

namespace VAPoR {
  bool tvmap_cmp(
    NetCDFCollection::TimeVaryingVar::tvmap_t a,
    NetCDFCollection::TimeVaryingVar::tvmap_t b
	) { return(a._time < b._time); };
}

int NetCDFCollection::TimeVaryingVar::Insert(
	const NetCDFSimple::Variable &variable, string file, 
    vector <string> time_dimnames, vector <double> times
) {
	bool first = (_variables.size() == 0);	// first insertion?
	//
	// If this isn't the first variable to be inserted the new variable
	// must match the existing ones
	//
	if (! first && ! _variables[0].Match(variable)) {
		SetErrMsg(
			"Multiple definitions of variable \"%s\"", 
			variable.GetName().c_str()
		);
		return(-1);
	}


	vector <size_t> space_dims = variable.GetDims();
	vector <string> space_dim_names = variable.GetDimNames();
//	if (space_dims.size() < 1) return(0);

	//
	// Check if variable is time varying. I.e. if its slowest varying
	// dimension name matches a dimension name specified in time_dimnames
	//
	bool time_varying = false;
	int ntimes = 1;	// # of time steps
	for (int i=0; i<time_dimnames.size(); i++) {
		if (variable.GetDimNames().size() && 
				variable.GetDimNames()[0].compare(time_dimnames[i]) == 0) {

			time_varying = true;
			ntimes = space_dims[0];
			space_dims.erase(space_dims.begin());
			space_dim_names.erase(space_dim_names.begin());
			break;
		}
	}
//	if (space_dims.size() < 1) return(0);

	if (times.size() && times.size() < ntimes) {
		SetErrMsg(
			"Time coordinate dimension does not match variable time dimension"
		);
		return(-1);
	}

	//
	// If no 'times' vector was specified (empty size) we automatically
	// compute the times based on the current time step;
	//
	vector <double> mytimes = times;
	if (mytimes.size() == 0) {
		double time = _tvmaps.size() ? _tvmaps[_tvmaps.size()-1]._time : 0.0;
		for (int i=0; i<ntimes; i++) {
			mytimes.push_back(time);
			time += 1.0;
		}
	}

	if (first) {
		_dims = space_dims;
		_dim_names = space_dim_names;
		_time_varying = time_varying;
		_name = variable.GetName();
	}
	_variables.push_back(variable);
	_files.push_back(file);
	size_t local_ts =  0;
	for (int i=0; i<mytimes.size(); i++) {
		tvmap_t tvmap;
		tvmap._fileidx = _files.size()-1;
		tvmap._varidx = _variables.size()-1;
		tvmap._time = mytimes[i];
		tvmap._local_ts = local_ts;
		_tvmaps.push_back(tvmap);
		local_ts++;
	}

	//
	// Sort variable by time
	//
	std::sort(_tvmaps.begin(), _tvmaps.end(), tvmap_cmp);
		

	return(0);
}

int NetCDFCollection::TimeVaryingVar::GetTime(
	size_t ts, double &time
) const {
	if (ts>=_tvmaps.size()) return(-1);

	time = _tvmaps[ts]._time;

	return(0);
}

vector <double> NetCDFCollection::TimeVaryingVar::GetTimes(
) const {
	vector <double> times;

	for (int i=0; i<_tvmaps.size(); i++) times.push_back(_tvmaps[i]._time);

	return(times);
}

int NetCDFCollection::TimeVaryingVar::GetTimeStep(
	double time, size_t &ts
) const {
	for (size_t i=0; i<_tvmaps.size(); i++) {
		if (_tvmaps[i]._time == time) {
			ts = i;
			return(0);
		}
	}
	SetErrMsg("Invalid time %f", time);
	return(-1);
}

size_t NetCDFCollection::TimeVaryingVar::GetLocalTimeStep(size_t ts) const {
	if (ts>=_tvmaps.size()) return(0);

	return(_tvmaps[ts]._local_ts);
}

int NetCDFCollection::TimeVaryingVar::GetFile(
	size_t ts, string &file
) const {

	if (ts>=_tvmaps.size()) return(-1);

	int fileidx = _tvmaps[ts]._fileidx;
	file = _files[fileidx];
	return(0);
}

int NetCDFCollection::TimeVaryingVar::GetVariableInfo(
	size_t ts, NetCDFSimple::Variable &variable
) const {

	if (ts>=_tvmaps.size()) return(-1);

	int varidx = _tvmaps[ts]._varidx;
	variable = _variables[varidx];
	return(0);
}

