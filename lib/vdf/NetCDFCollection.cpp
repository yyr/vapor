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
	_dimNames.clear();
	_missingValAttName.clear();
	_times.clear();
	_timesMap.clear();
	_ovr_local_ts = 0;
	_ovr_slice = 0;
	_ovr_slicebuf = NULL;
	_ovr_slicebufsz = 0;
	_ovr_linebuf = NULL;
	_ovr_linebufsz = 0;
	_ovr_has_missing = false;
	_ovr_missing_value = 0.0;
	_ovr_open = false;
	_ncdfptr = NULL;
	_ncdfmap.clear();
	_failedVars.clear();
	_reverseRead = false;


}
NetCDFCollection::~NetCDFCollection() {

	map <string, NetCDFSimple *>::iterator itr;
	for (itr = _ncdfmap.begin(); itr != _ncdfmap.end(); ++itr) {
		delete itr->second;
	}
	if (_ovr_slicebuf) delete [] _ovr_slicebuf;
	if (_ovr_linebuf) delete [] _ovr_linebuf;
}

int NetCDFCollection::Initialize(
	const vector <string> &files, const vector <string> &time_dimnames, 
	const vector <string> &time_coordvars
) {
	_variableList.clear();
	_times.clear();
	_timesMap.clear();
	_ovr_local_ts = 0;
	_ovr_slice = 0;
	_ovr_slicebuf = NULL;
	_ovr_slicebufsz = 0;
	_ovr_linebuf = NULL;
	_ovr_linebufsz = 0;
	_ovr_has_missing = false;
	_ovr_missing_value = 0.0;
	_ovr_open = false;
	_failedVars.clear();
	_reverseRead = false;

	map <string, NetCDFSimple *>::iterator itr;
	for (itr = _ncdfmap.begin(); itr != _ncdfmap.end(); ++itr) {
		delete itr->second;
	}
	_ncdfmap.clear();
	_ncdfptr = NULL;

	//
	// Build a hash table to map a variable's time dimension
	// to its time coordinates
	//
	int file_org; // case 1, 2, 3 (3a or 3b)
	int rc = NetCDFCollection::_InitializeTimesMap(
		files, time_dimnames, time_coordvars, _timesMap, _times, file_org
	);
	if (rc<0) return(-1);
		
	for (int i=0; i<files.size(); i++) {
		NetCDFSimple *netcdf = new NetCDFSimple();
		_ncdfmap[files[i]] = netcdf;

		rc = netcdf->Initialize(files[i]);
		if (rc<0) {
			SetErrMsg("NetCDFSimple::Initialize(%s)", files[i].c_str());
			return(-1);
		}

		//
		// Get dimension names. N.B. _dimNames will contain duplicates
		// until we remove them.
		//
		vector <string> dimnames;
		vector <size_t> dims;
		netcdf->GetDimensions(dimnames, dims);
		for (int j=0; j<dimnames.size(); j++) {
			_dimNames.push_back(dimnames[j]);
		}

		//
		// Get variable info for all variables in current file
		//
		const vector <NetCDFSimple::Variable> &variables =netcdf->GetVariables();

		//
		// For each variable in the file add it to _variablesList
		//
		for (int j=0; j<variables.size(); j++) {
			string name = variables[j].GetName();

			map <string,TimeVaryingVar>::iterator p = _variableList.find(name);

			//
			// If this is the first time we've seen this variable
			// add it to _variablesList
			//
			if (p ==  _variableList.end()) {
				TimeVaryingVar tvv;
				_variableList[name] = tvv;
				p = _variableList.find(name);
			}
			TimeVaryingVar &tvvref = p->second;

			bool enable = EnableErrMsg(false);
			int rc = tvvref.Insert(
				variables[j], files[i], time_dimnames, 
				_timesMap, file_org
			);
			(void) EnableErrMsg(enable); 
			if (rc<0)  {
				SetErrCode(0);
				_failedVars.push_back(files[i] + ": " + variables[j].GetName());
				continue;
			}
		}
	}
	

	//
	// sort and remove duplicates
	//
    sort(_dimNames.begin(), _dimNames.end());
    vector <string>::iterator lasts;
    lasts = unique(_dimNames.begin(), _dimNames.end());
    _dimNames.erase(lasts, _dimNames.end());


	return(0);
}

bool NetCDFCollection::VariableExists(string varname) const {

	map <string,TimeVaryingVar>::const_iterator p = _variableList.find(varname);
	if (p ==  _variableList.end()) {
		return(false);
	}
	return(true);
}

bool NetCDFCollection::VariableExists(size_t ts, string varname) const {
	if (ts >= _times.size()) return(false);

	map <string,TimeVaryingVar>::const_iterator p = _variableList.find(varname);
	if (p ==  _variableList.end()) {
		return(false);
	}
	const TimeVaryingVar &tvvar = p->second;

	if (! tvvar.GetTimeVarying()) return(true);// CV variables exist for all times

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
vector <string> NetCDFCollection::GetDimNames(string varname) const {
	vector <string> dimnames;

	map <string,TimeVaryingVar>::const_iterator p = _variableList.find(varname);
	if (p ==  _variableList.end()) {
		return(dimnames);
	}
	const TimeVaryingVar &tvvars = p->second;

	dimnames = tvvars.GetSpatialDimNames();
	return(dimnames);
}

bool NetCDFCollection::IsTimeVarying(string varname) const {
	map <string,TimeVaryingVar>::const_iterator p = _variableList.find(varname);
	if (p ==  _variableList.end()) {
		return(false);
	}
	const TimeVaryingVar &tvvars = p->second;
	return(tvvars.GetTimeVarying());
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
	if (! tvvars.GetTimeVarying()) {
		times = _times;	// CV variables defined for all times
	}
	else {
		times = tvvars.GetTimes();
	}
	return(0);
}

int NetCDFCollection::GetFile(
	size_t ts, string varname, string &file, size_t &local_ts
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

	local_ts = tvvars.GetLocalTimeStep(var_ts);

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

int NetCDFCollection::GetVariableInfo(
	string varname, NetCDFSimple::Variable &variable
) const {

	map <string,TimeVaryingVar>::const_iterator p = _variableList.find(varname);
	if (p ==  _variableList.end()) {
		SetErrMsg("Invalid variable \"%s\"", varname.c_str());
		return(-1);
	}
	const TimeVaryingVar &tvvars = p->second;

	return(tvvars.GetVariableInfo(0, variable));
}

bool NetCDFCollection::GetMissingValue(
    string varname, double &mv
) const {
    map <string,TimeVaryingVar>::const_iterator p = _variableList.find(varname);
    if (p ==  _variableList.end()) {
        SetErrMsg("Invalid variable \"%s\"", varname.c_str());
        return(false);
    }
    const TimeVaryingVar &tvvars = p->second;

    return(tvvars.GetMissingValue(_missingValAttName, mv));
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
	if (_reverseRead) {
		vector <size_t> dims = _ovr_tvvars.GetSpatialDims();
		size_t nz = dims.size() > 2 ? dims[dims.size()-3] : 1;
		_ovr_slice = nz-1;
	}
	else {
		_ovr_slice = 0;
	}

	string path;
	NetCDFSimple::Variable varinfo;
	_ovr_tvvars.GetFile(var_ts, path);
	_ovr_tvvars.GetVariableInfo(var_ts, varinfo);

	_ovr_has_missing = _ovr_tvvars.GetMissingValue(
		_missingValAttName, _ovr_missing_value
	);
	
	_ncdfptr = _ncdfmap[path];
	rc = _ncdfptr->OpenRead(varinfo);
	if (rc<0) {
		SetErrMsg(
			"NetCDFCollection::OpenRead(%d, %s) : failed",
			var_ts, varname.c_str()
		);
		return(-1);
	}
	_ovr_open = true;
	return(0);

}

int NetCDFCollection::ReadNative(size_t start[], size_t count[], float *data) {
	size_t mystart[NC_MAX_VAR_DIMS];
	size_t mycount[NC_MAX_VAR_DIMS];

	if (! _ovr_open || ! _ncdfptr) {
		SetErrMsg("Data set not opened for reading\n");
		return(-1);
	}

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

	return(_ncdfptr->Read(mystart, mycount, data));
}
	

int NetCDFCollection::ReadNative(size_t start[], size_t count[], int *data) {
	size_t mystart[NC_MAX_VAR_DIMS];
	size_t mycount[NC_MAX_VAR_DIMS];

	if (! _ovr_open || !_ncdfptr) {
		SetErrMsg("Data set not opened for reading\n");
		return(-1);
	}

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

	return(_ncdfptr->Read(mystart, mycount, data));
}

int NetCDFCollection::ReadNative(float *data) {
	size_t start[NC_MAX_VAR_DIMS];
	size_t count[NC_MAX_VAR_DIMS];

	if (! _ovr_open || ! _ncdfptr) {
		SetErrMsg("Data set not opened for reading\n");
		return(-1);
	}
	
	vector <size_t> dims = _ovr_tvvars.GetSpatialDims();
	for (int i=0; i<dims.size(); i++) {
		start[i] = 0;
		count[i] = dims[i];
	}

	return(NetCDFCollection::ReadNative(start, count, data));
}

int NetCDFCollection::ReadNative(int *data) {
	size_t start[NC_MAX_VAR_DIMS];
	size_t count[NC_MAX_VAR_DIMS];

	if (! _ovr_open || ! _ncdfptr) {
		SetErrMsg("Data set not opened for reading\n");
		return(-1);
	}
	
	vector <size_t> dims = _ovr_tvvars.GetSpatialDims();
	for (int i=0; i<dims.size(); i++) {
		start[i] = 0;
		count[i] = dims[i];
	}

	return(NetCDFCollection::ReadNative(start, count, data));
}

int NetCDFCollection::_InitializeTimesMap(
	const vector <string> &files, const vector <string> &time_dimnames, 
	const vector <string> &time_coordvars, 
	map <string, vector <double> > &timesMap, 
	vector <double> &times, int &file_org
) const {
	timesMap.clear();
	if (time_coordvars.size() && (time_coordvars.size()!=time_dimnames.size())){
		SetErrMsg("NetCDFCollection::Initialize() : number of time coordinate variables and time dimensions must match when time coordinate variables specified");
		return(-1);
	} 

	int rc; 
	if (time_dimnames.size() == 0) {
		file_org = 1;
		rc = _InitializeTimesMapCase1(files, timesMap);
	}
	else if ((time_dimnames.size() != 0) && (time_coordvars.size() == 0)) {
		file_org = 2;
		rc = _InitializeTimesMapCase2(files, time_dimnames, timesMap);
	}
	else {
		file_org = 3;
		rc = _InitializeTimesMapCase3(
			files, time_dimnames, time_coordvars, timesMap
		);
	}
	if (rc<0) return(rc);

	// 
	// Generate times: a single vector of all the time coordinates
	//
	map <string, vector <double> >::const_iterator itr1;
	for (itr1 = timesMap.begin(); itr1 != timesMap.end(); ++itr1) {
		const vector <double> &timesref = itr1->second;
		times.insert(times.end(), timesref.begin(), timesref.end());
	}

	//
	// sort and remove duplicates
	//
	sort(times.begin(), times.end());
	vector <double>::iterator lastd;
	lastd = unique(times.begin(), times.end());
	times.erase(lastd, times.end());

	//
	// Create an entry for constant variables, which are defined at all times
	//
	timesMap["constant"] = times;
	return(0);
}

int NetCDFCollection::_InitializeTimesMapCase1(
	const vector <string> &files,
	map <string, vector <double> > &timesMap
) const {
	timesMap.clear();
	map <string, double> currentTime; // current time for each variable

	// Case 1: No TDs or TCVs => synthesize TCV
	// A variable's time coordinate is determined by the ordering
	// of the file that it occurs in. The timesMap key is the 
	// concatentation of file name (where the variable occurs) and
	// variable name. The only type of variables present are ITVV
	//

	for (int i=0; i<files.size(); i++) {
		NetCDFSimple *netcdf = new NetCDFSimple();

		int rc = netcdf->Initialize(files[i]);
		if (rc<0) {
			SetErrMsg("NetCDFSimple::Initialize(%s)", files[i].c_str());
			return(-1);
		}

		const vector <NetCDFSimple::Variable> &variables = netcdf->GetVariables();

		for (int j=0; j<variables.size(); j++) {
			//
			// Skip 0D variables
			//
			if (variables[j].GetDimNames().size() == 0) continue;

			string varname = variables[j].GetName();

			// If first time this variable has been seen 
			// initialize the currentTime
			//
			if (currentTime.find(varname) == currentTime.end()) {
				currentTime[varname] = 0.0;
			}

			string key = files[i] + varname;
			vector <double> times(1, currentTime[varname]);
			timesMap[key] = times;

			currentTime[varname] += 1.0;
		}
		delete netcdf;
	}
	return(0);
}
	
int NetCDFCollection::_InitializeTimesMapCase2(
	const vector <string> &files, const vector <string> &time_dimnames,
	map <string, vector <double> > &timesMap
) const {
	timesMap.clear();
	map <string, double> currentTime; // current time for each variable

	// Case 2: TD specified, but no TCV. 
	// A variable's time coordinate is determined by the ordering
	// of the file that it occurs in, offset by its time dimesion.
	// The timesMap key is the 
	// concatentation of file name (where the variable occurs) and
	// variable name.
	// Both TVV and CV variables may be present.
	//

	for (int i=0; i<files.size(); i++) {
		NetCDFSimple *netcdf = new NetCDFSimple();

		int rc = netcdf->Initialize(files[i]);
		if (rc<0) {
			SetErrMsg("NetCDFSimple::Initialize(%s)", files[i].c_str());
			return(-1);
		}

		const vector <NetCDFSimple::Variable> &variables = netcdf->GetVariables();

		for (int j=0; j<variables.size(); j++) {
			//
			// Skip 0D variables
			//
			if (variables[j].GetDimNames().size() == 0) continue;

			string varname = variables[j].GetName();
			string key = files[i] + varname;
			string timedim = variables[j].GetDimNames()[0];

			// If this is a CV variable (no time dimension) we skip it
			//
			if (find(time_dimnames.begin(), time_dimnames.end(), timedim) == time_dimnames.end()) continue;	// CV variable

			// Number of time steps for this variable
			//
			size_t timedimlen = variables[j].GetDims()[0];

			// If first time this varname has been seen 
			// initialize the currentTime
			//
			if (currentTime.find(varname) == currentTime.end()) {
				currentTime[varname] = 0.0;
			}

			vector <double> times;
			for (int t=0;t<timedimlen;t++) {
				times.push_back(currentTime[varname]);
				currentTime[varname] += 1.0;
			}

			timesMap[key] = times;
		}
		delete netcdf;
	}
	return(0);
}

int NetCDFCollection::_InitializeTimesMapCase3(
	const vector <string> &files, const vector <string> &time_dimnames, 
	const vector <string> &time_coordvars,
	map <string, vector <double> > &timesMap
) const {
	timesMap.clear();

	//
	// tcvcount counts occurrences of each TCV. tcvfile and tcvdim record 
	// the last file name and TD pair used to generate hash key for each TCV
	//
	map <string, int> tcvcount;	// # of files of TCV appears in
	map <string, string> tcvfile;	
	map <string, string> tcvdim;
	for (int i=0; i<time_coordvars.size(); i++) {
		tcvcount[time_coordvars[i]] = 0;
	}

	for (int i=0; i<files.size(); i++) {
		NetCDFSimple *netcdf = new NetCDFSimple();

		int rc = netcdf->Initialize(files[i]);
		if (rc<0) return(-1);

		const vector <NetCDFSimple::Variable> &variables = netcdf->GetVariables();

		//
		// For each TCV see if it exists in current file, if so
		// read it and add times to timesMap
		//
		for (int j=0; j<time_coordvars.size(); j++) {
			int index = _get_var_index(variables, time_coordvars[j]);
			if (index < 0) continue;	// TCV doesn't exist

			// Increment count
			//

			tcvcount[time_coordvars[j]] += 1; 

			// Read TCV
			float *buf= _Get1DVar(netcdf, variables[index]);
			if (! buf) {
				SetErrMsg(	
					"Failed to read time coordinate variable \"%s\"",
					time_coordvars[j].c_str()
				);
				return(-1);
			}
			vector <double> times;
			for (int t=0; t<variables[index].GetDims()[0]; t++) {
				times.push_back(buf[t]);
			}
			delete [] buf;

			//
			// The hash key for timesMap is the file plus the
			// time dimension name
			//
			string timedim = variables[index].GetDimNames()[0];
			string key = files[i] + timedim;

			// record file and timedim used to generate the hash key
			//
			tcvfile[time_coordvars[j]] = files[i];
			tcvdim[time_coordvars[j]] = timedim;

			if (timesMap.find(key) == timesMap.end()) {
				timesMap[key] = times;
			}
			else {
				vector <double> &timesref = timesMap[key];
				for (int t=0;t<times.size();t++) {
					timesref.push_back(times[t]);
				}
			}
		}

		delete netcdf;
	}

	//
	// Finally, if see if this is case 3a (only one file contains the TCV),
	// or case 3b (a TCV is present in any file containing a TVV). 
	// We're only checking for case 3a here. If case 1, replicate the
	// times for each file & time dimension pair
	//
	//

	for (int i=0; i<time_coordvars.size(); i++) {
		if (tcvcount[time_coordvars[i]] == 1) {
			string key1 = tcvfile[time_coordvars[i]] + tcvdim[time_coordvars[i]];
			map <string, vector <double> >::const_iterator itr;
			for (int j=0; j<files.size(); j++) {
				string key = files[j] + tcvdim[time_coordvars[i]];
				timesMap[key] = timesMap[key1];
			}
		}
	}

	return(0);
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
			_InterpolateLine(src, nx, 1, has_missing, mv, dst);
		}
		nx--;
	}

	if (ystag) {
		for (int i=0; i<nx; i++) {
			float *src = slice + i;
			float *dst = slice + i;
			_InterpolateLine(src, ny, nx, has_missing, mv, dst);
		}
		ny--;
	}
}

float *NetCDFCollection::_Get1DVar(
	NetCDFSimple *netcdf, 
	const NetCDFSimple::Variable &variable
) const { 
	if (variable.GetDims().size() != 1) return(NULL); 
	int rc = netcdf->OpenRead(variable);
	if (rc<0) {
		SetErrMsg(
			"Time coordinate variable \"%s\" is invalid",
			variable.GetName().c_str()
		);
		return(NULL);
	}
	size_t start[] = {0};
	size_t count[] = {variable.GetDims()[0]};
	float *buf = new float [variable.GetDims()[0]];
	rc = netcdf->Read(start, count, buf);
	if (rc<0) {
		return(NULL);
	}
	netcdf->Close();
	return(buf);
}

int NetCDFCollection::_get_var_index(
	const vector <NetCDFSimple::Variable> variables, string varname
) const {
	for (int i=0; i<variables.size(); i++) {
		if (varname.compare (variables[i].GetName()) == 0) return (i);
	}
	return(-1);	
}


int NetCDFCollection::ReadSliceNative(float *data) {

	if (! _ovr_open) {
		SetErrMsg("Data set not opened for reading\n");
		return(-1);
	}

	const TimeVaryingVar &var = _ovr_tvvars;
	vector <size_t> dims = var.GetSpatialDims();

	size_t nx = dims[dims.size()-1];
	size_t ny = dims[dims.size()-2];
	size_t nz = dims.size() > 2 ? dims[dims.size()-3] : 1;

	if (_reverseRead) {
		if (_ovr_slice < 0) return(0);
	}
	else {
		if (_ovr_slice >= nz) return(0);
	}

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
	if (_reverseRead) {
		_ovr_slice--;
	}
	else {
		_ovr_slice++;
	}
	if (rc<0) return(rc);
	return(1);
}

	
int NetCDFCollection::ReadSlice(float *data) {

	if (! _ovr_open) {
		SetErrMsg("Data set not opened for reading\n");
		return(-1);
	}

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

	int firstSlice = 0;	// index of first and second slice read
	int secondSlice = 1;
	if (_reverseRead) {
		if (dims.size() == 3) {
			firstSlice = dims[2]-1;
			secondSlice = dims[2]-2;
		}
	}
		
	if (zstag) {

		//
		// If this is the first read we read the first slice into
		// the front (first half) of buffer, and the second into 
		// the back (second half) of
		// buffer. For all other reads the previous slice is in
		// the front of buffer, so we read the current slice into 
		// the back of buffer
		//
		if (_ovr_slice == firstSlice) {
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
		// N.B. ReadSliceNative increments _ovr_slice
		//
		if (_ovr_slice == secondSlice) {	
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
	if (! _ovr_open) {
		SetErrMsg("Data set not opened for reading\n");
		return(-1);
	}
	SetErrMsg("Not implemented");
	return(-1);
}
int NetCDFCollection::Read(size_t start[], size_t count[], int *data) {
	if (! _ovr_open) {
		SetErrMsg("Data set not opened for reading\n");
		return(-1);
	}
	SetErrMsg("Not implemented");
	return(-1);
}

int NetCDFCollection::Read(float *data) {

	if (! _ovr_open) {
		SetErrMsg("Data set not opened for reading\n");
		return(-1);
	}

	const TimeVaryingVar &var = _ovr_tvvars;
	vector <size_t> dims = var.GetSpatialDims();
	vector <string> dimnames = var.GetSpatialDimNames();

	//
	// Handle different dimenion cases
	//
	if (dims.size() > 3) {
		SetErrMsg("Only 0D, 1D, 2D and 3D variables supported");
		return(-1);
	} 
	else if (dims.size() == 0) {
		size_t start[] = {0};
		size_t count[] = {1};
		return(NetCDFCollection::ReadNative(start, count, data));
	}
	else if (dims.size() == 1) {
		size_t nx = dims[dims.size()-1];
		size_t start[] = {0};
		size_t count[] = {nx};
		bool xstag = IsStaggeredDim(dimnames[dimnames.size()-1]);

		if (xstag) {	// Deal with staggered data

			if (_ovr_linebufsz < (nx*sizeof(*data))) {
				if (_ovr_linebuf) delete [] _ovr_linebuf;
				_ovr_linebuf = (unsigned char *) new float [nx];
				_ovr_linebufsz = nx*sizeof(*data);
			}
			int rc = NetCDFCollection::ReadNative(
				start, count, (float *) _ovr_linebuf
			);
			if (rc<0) return(-1);
			_InterpolateLine(
				(const float *) _ovr_linebuf, nx, 1, _ovr_has_missing, 
				_ovr_missing_value, data
			);
			return(0);
		}
		else {
			return (NetCDFCollection::ReadNative(start, count, data));
		}
	}


	bool xstag = IsStaggeredDim(dimnames[dimnames.size()-1]);
	bool ystag = IsStaggeredDim(dimnames[dimnames.size()-2]);
	bool zstag = dims.size() == 3 ? IsStaggeredDim(dimnames[dimnames.size()-3]) : false;

	size_t nx = dims[dims.size()-1];
	size_t ny = dims[dims.size()-2];
	size_t nz = dims.size() == 3 ? dims[dims.size()-3] : 1;

	size_t nxus = xstag ? nx-1 : nx;
	size_t nyus = ystag ? ny-1 : ny;
	size_t nzus = zstag ? nz-1 : nz;

	float *buf = data;
	for (int i=0; i<nzus; i++) {
		int rc = NetCDFCollection::ReadSlice(buf);
		if (rc<0) return(rc);
		buf += nxus*nyus;
	}
	return(0);
}

int NetCDFCollection::Close() {
	_ovr_open = false;
	if (! _ncdfptr) return(0);
	return (_ncdfptr->Close());
}

namespace VAPoR {
std::ostream &operator<<(
    std::ostream &o, const NetCDFCollection &ncdfc
) {
	o << "NetCDFCollection" << endl;
	o << " _staggeredDims : ";
	for (int i=0; i<ncdfc._staggeredDims.size(); i++) {
		o << ncdfc._staggeredDims[i] << " ";
	}
	o << endl;
	o << " _times : ";
	for (int i=0; i<ncdfc._times.size(); i++) {
		o << ncdfc._times[i] << " ";
	}
	o << endl;
	o << " _missingValAttName : " << ncdfc._missingValAttName << endl;

	o << " _variableList : " << endl;
	map <string, NetCDFCollection::TimeVaryingVar >::const_iterator itr;
	for (itr=ncdfc._variableList.begin(); itr!=ncdfc._variableList.end();++itr){
		o << itr->second;
		o << endl;
	}

	return(o);
}
};


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
    const vector <string> &time_dimnames, 
	const map <string, vector <double> > &timesmap,
	int file_org
) {
	bool first = (_variables.size() == 0);	// first insertion?

	vector <size_t> space_dims = variable.GetDims();
	vector <string> space_dim_names = variable.GetDimNames();

	//
	// Check if variable is time varying. I.e. if its slowest varying
	// dimension name matches a dimension name specified in time_dimnames
	//
	bool time_varying = false;
	string key;	// hash key for timesmap

	if (variable.GetDimNames().size()) {
		string timedim = variable.GetDimNames()[0];

		if (find(time_dimnames.begin(), time_dimnames.end(), timedim) != time_dimnames.end()) {
			time_varying = true;

			space_dims.erase(space_dims.begin());
			space_dim_names.erase(space_dim_names.begin());
		}
	}
	if (! time_varying) {
		key = "constant";
	} 
	else if (file_org == 1 || file_org == 2) {
		key = file + variable.GetName();
	}
	else {
		key = file + variable.GetDimNames()[0];
	}

	if (first) {
		_dims = space_dims;
		_dim_names = space_dim_names;
		_time_varying = time_varying;
		_name = variable.GetName();
	}
	else {
		//
		// If this isn't the first variable to be inserted the new variable
		// must match the existing ones
		//
		if (!( _dims == space_dims && _time_varying == time_varying)) {
			SetErrMsg(
				"Multiple definitions of variable \"%s\"", 
				variable.GetName().c_str()
			);
			return(-1);
		}
	}
	_variables.push_back(variable);
	_files.push_back(file);

	map <string, vector<double> >::const_iterator itr;
	itr = timesmap.find(key);
	if (itr == timesmap.end()) {
		SetErrMsg("Time coordinates not available for variable");
		return(-1);
	}
	const vector <double> &timesref = itr->second;

	size_t local_ts =  0;
	for (int i=0; i<timesref.size(); i++) {
		tvmap_t tvmap;
		tvmap._fileidx = _files.size()-1;
		tvmap._varidx = _variables.size()-1;
		tvmap._time = timesref[i];
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
	if (! _time_varying) {
		ts = 0;
		return(0);
	}

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

bool NetCDFCollection::TimeVaryingVar::GetMissingValue(
	string attname, double &mv
) const {
	mv = 0.0;

	if (! attname.length()) return(false);

	if (! _variables.size()) return(false);

	//
	// Get missing value from the first variable. Assumes missing value
	// is invariant across all time steps
	//
	const NetCDFSimple::Variable &variable = _variables[0];

	vector <double> vec;
	variable.GetAtt(attname, vec);
	if (! vec.size()) return(false);

	mv = vec[0];
	return(true);
}

namespace VAPoR {
std::ostream &operator<<(
	std::ostream &o, const NetCDFCollection::TimeVaryingVar &var
) {
	o << " TimeVaryingVar" << endl;
	o << " Variable : " << var._name << endl;
	o << "  Files : " << endl;
	for (int i=0; i<var._files.size(); i++) {
		o << "   " << var._files[i] << endl;
	}
	o << "  Dims : ";
	for (int i=0; i<var._dims.size(); i++) {
		o << var._dims[i] << " ";
	}
	o << endl;
	o << "  Dim Names : ";
	for (int i=0; i<var._dim_names.size(); i++) {
		o << var._dim_names[i] << " ";
	}
	o << endl;

	o << "  Time Varying : " << var._time_varying << endl;

	o << "  Time Varying Map : " << endl;
	for (int i=0; i<var._tvmaps.size(); i++) {
		o << "   _fileidx : " << var._tvmaps[i]._fileidx << endl;
		o << "   _varidx : " <<  var._tvmaps[i]._varidx << endl;
		o << "   _time : " <<  var._tvmaps[i]._time << endl;
		o << "   _local_ts : " <<  var._tvmaps[i]._local_ts << endl;
		o << endl;
	}

	return(o);
}
};

