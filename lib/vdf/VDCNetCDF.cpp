#include <cassert>
#include <sstream>
#include <map>
#include <vector>
#include <sys/stat.h>
#include <netcdf.h>
#include "vapor/VDCNetCDF.h"
#include "vapor/CFuncs.h"

using namespace VAPoR;
using namespace VetsUtil;

namespace {

int vdc_xtype2ncdf_xtype(VDC::XType v_xtype) {
	int n_xtype;
	switch(v_xtype) {
	case VDC::FLOAT:
		n_xtype = NC_FLOAT;
		break;
	case VDC::DOUBLE:
		n_xtype = NC_DOUBLE;
		break;
	case VDC::INT32:
		n_xtype = NC_INT;
		break;
	case VDC::INT64:
		n_xtype = NC_INT64;
		break;
	case VDC::TEXT:
		n_xtype = NC_CHAR;
		break;
	default:
		n_xtype = NC_NAT;
		break;
	}
	return(n_xtype);
}

VDC::XType ncdf_xtype2vdc_xtype(int n_xtype) {

	VDC::XType v_xtype;
	switch(n_xtype) {
	case NC_FLOAT:
		v_xtype = VDC::FLOAT;
		break;
	case NC_DOUBLE:
		v_xtype = VDC::DOUBLE;
		break;
	case NC_INT:
		v_xtype = VDC::INT32;
		break;
	case NC_INT64:
		v_xtype = VDC::INT64;
		break;
	case NC_CHAR:
	case NC_STRING:
		v_xtype = VDC::TEXT;
		break;
	default:
		v_xtype = VDC::INVALID;
		break;
	}
	return(v_xtype);
}

void ws_info(
	const vector <size_t> &sdims, const vector <size_t> &bs,
	size_t &slice_size, size_t &slices_per_slab, size_t &num_slices
) {
	assert(sdims.size() >= 2 && sdims.size() <= 3);
	assert(bs.size() == sdims.size());

	slice_size = sdims[0] * sdims[1];
	if (sdims.size() > 2) {
		slices_per_slab = bs[2];
		num_slices = sdims[2];
	}
	else {
		slices_per_slab = 1;
		num_slices = 1;
	}
}

void vdc_2_ncdfcoords(
	size_t ts0, size_t ts1, bool time_varying, 
	const vector <size_t> &min, const vector <size_t> &max,
	vector <size_t> &start, vector <size_t> &count
) {
	start.clear();
	count.clear();

	assert(min.size() == max.size());;
	assert(max.size() <= 3);
	assert(ts1 >= ts0);

	if (time_varying) {
		start.push_back(ts0);
		count.push_back(ts1-ts0+1);
	}

	for (int i=min.size()-1; i>=0; i--) {
		assert(max[i] >= min[i]);
		start.push_back(min[i]);
		count.push_back(max[i] - min[i] + 1);
	}
}

};

VDCNetCDF::VDCNetCDF(
	int nthreads, size_t master_threshold, size_t variable_threshold
) : VDC() {

	_nthreads = nthreads;
	_master_threshold = master_threshold;
	_variable_threshold = variable_threshold;
	_chunksizehint =  0;
	_master = new WASP();
	_open_file = NULL;
	_open_slice_num = 0;
	_open_write = false;
	_open_level = 0;
	_slice_buffer = NULL;
	_version = 1;
}


VDCNetCDF::~VDCNetCDF() {

	if (_open_file && _open_file != _master) {
		delete _open_file;
	}

	if (_master) {
		_master->Close();
		delete _master;
	}
}


int VDCNetCDF::Initialize(
	const vector <string> &paths, AccessMode mode, size_t chunksizehint
) {
	_chunksizehint =  chunksizehint;

	int rc = VDC::Initialize(paths, mode);
	if (rc<0) return(-1);

	if (mode == VDC::W) {
		size_t chsz = _chunksizehint;
		rc = _master->Create(
			_master_path, NC_64BIT_OFFSET|NC_WRITE, 0, chsz, 1
		);
	}
	else if (mode == VDC::A) {
		rc = _master->Open(_master_path, NC_WRITE);
	}
	else {
		rc = _master->Open(_master_path, NC_NOWRITE);
	}
	if (rc<0) return(-1);
	return(0);
}

string VDCNetCDF::GetDataDir(string master) {
	string path = master;
	if (path.rfind(".nc") != string::npos) path.erase(path.rfind(".nc"));
	path += "_data";
	return(path);
}

//
// Figure out where variable lives. This algorithm will most likely
// change.
//
int VDCNetCDF::GetPath(
	string varname, size_t ts, string &path, size_t &file_ts,
	size_t &max_ts
) const {
	path.clear();
	file_ts = 0;
	max_ts = 0;

	if (! VDC::IsTimeVarying(varname)) {
		ts = 0;	// Could be anything if data aren't time varying;
	}

	VDC::BaseVar var;
	if (! VDC::GetBaseVarInfo(varname, var))  {
        SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(false);
	}


	// Does this variable live in the master file?
	//
	if (_var_in_master(var)) {
		path = _master_path;
		file_ts = ts;
		return(0);
	}

	path = VDCNetCDF::GetDataDir(_master_path);

	path += "/";

	if (VDC::IsDataVar(varname)) {
		path += "data";
		path += "/";
	}
	else {
		path += "coordinates";
		path += "/";
	}

	path += varname;
	path += "/";
	path += varname;

	if (VDC::IsTimeVarying(varname)) {

		size_t nelements = 1;
		size_t ngridpoints = 1;
		for (int i=0; i<var.GetDimensions().size(); i++) {
			nelements *= var.GetDimensions()[i].GetLength(); 
			if (var.GetDimensions()[i].GetAxis() != 3) {
				ngridpoints *= var.GetDimensions()[i].GetLength();
			}
		}

		int idx;
		ostringstream oss;
		size_t numts = VDC::GetNumTimeSteps(varname);
		assert(numts>0);
		max_ts = _variable_threshold / ngridpoints;
		if (max_ts > numts) max_ts = numts;
		if (max_ts == 0) {
			idx = ts;
			file_ts = ts;
			max_ts = 1;
		}
		else {
		 	idx = ts / max_ts;;
			file_ts = ts % max_ts;
		}
		int width = (int) log10((double) numts-1) + 1;
		if (width < 4) width = 4;
		oss.width(width); oss.fill('0'); oss << idx;	

		path += ".";
		path += oss.str();
	}

	path += ".";
	path += "nc";
	
	return(0);
}

int VDCNetCDF::GetDimLensAtLevel(
    string varname, int level, vector <size_t> &dims_at_level,
	vector <size_t> &bs_at_level
) const {
	dims_at_level.clear();
	bs_at_level.clear();

	int nlevels = VDC::GetNumRefLevels(varname);

	if (level > nlevels-1) level = nlevels-1;
	if (level < 0) level = level + nlevels;
	if (level < 0) level = 0;

	string wname;
	vector <VDC::Dimension> dims;
	VDC::BaseVar var;

	if (! VDC::GetBaseVarInfo(varname, var)) {
        SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(-1); 
	}

	dims = var.GetDimensions();

	vector <size_t> dimlens;
	size_t numts;
	VDC::ParseDimensions(dims, dimlens, numts);
	if (VDC::IsTimeVarying(varname)) dimlens.push_back(numts);

	vector <size_t> bs = var.GetBS();	

	assert(bs.size() == dimlens.size());


	if (var.IsCompressed()) {
		wname = var.GetWName();

		reverse(bs.begin(), bs.end());
		reverse(dimlens.begin(), dimlens.end());
		WASP::InqDimsAtLevel(
			wname, level, dimlens, bs, dims_at_level,bs_at_level
		);
		reverse(bs_at_level.begin(), bs_at_level.end());
		reverse(dims_at_level.begin(), dims_at_level.end());
	}
	else {
		bs_at_level = bs;
		dims_at_level = dimlens;
	}

	return(0);
}

bool VDCNetCDF::DataDirExists(string master) {

	string path = VDCNetCDF::GetDataDir(master);

	struct stat statbuf;

	if (stat(path.c_str(), &statbuf) < 0) return (false);

	return(true);
}


int VDCNetCDF::OpenVariableRead(
    size_t ts, string varname, int level, int lod
) {

	CloseVariable();
	_open_file = NULL; 
	_open_write = false;
	_open_slice_num = 0; 
	_open_ts = 0;
	_open_file_ts = 0;
	_open_varname.clear();
	_open_level = 0;

	VDC::BaseVar var;
	if (! VDC::GetBaseVarInfo(varname, var))  {
        SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(false);
	}
	_open_var = var;


	string path;
	size_t file_ts;
	size_t max_ts;
	int rc = GetPath(varname, ts, path, file_ts, max_ts);
	if (rc<0) return(-1);

	vector <VDC::Dimension> dims = _open_var.GetDimensions();

	int nlevels = VDC::GetNumRefLevels(varname);

	if (level > nlevels-1) level = nlevels-1;
	if (level < 0) level = level + nlevels;
	if (level < 0) level = 0;
	_open_level = level;

	int ncratios = var.GetCRatios().size();

	if (lod > ncratios-1) lod = ncratios-1;
	if (lod < 0) lod = lod + ncratios;
	if (lod < 0) lod = 0;
	
	WASP *wasp = NULL;

	if (path.compare(_master_path) == 0) {
		wasp = _master;
	}
	else {
		wasp = new WASP(_nthreads);
		rc = wasp->Open(path, NC_NOWRITE);
		if (rc<0) return(-1);
	}

	_open_file = wasp;

	rc = wasp->OpenVarRead(varname, level, lod);
	if (rc<0) return(-1);

	_open_slice_num = 0; 
	_open_ts = ts;
	_open_file_ts = file_ts;
	_open_varname = varname;
    return(0);
}

int VDCNetCDF::OpenVariableWrite(size_t ts, string varname, int lod) {

	CloseVariable();
	_open_file = NULL; 
	_open_write = true;
	_open_slice_num = 0; 
	_open_ts = 0;
	_open_file_ts = 0;
	_open_varname.clear();

	VDC::BaseVar var;
	if (! VDC::GetBaseVarInfo(varname, var))  {
        SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(false);
	}
	_open_var = var;

	string path;
	size_t file_ts;
	size_t max_ts;
	int rc = GetPath(varname, ts, path, file_ts, max_ts);
	if (rc<0) return(-1);

	WASP *wasp = NULL;

	if (path.compare(_master_path) == 0) {
		wasp = _master;
	}
	else if (_master->ValidFile(path)) {
		wasp = new WASP(_nthreads);
		rc = wasp->Open(path, NC_WRITE);
	}
	else {
		wasp = new WASP(_nthreads);
		string dir;
		DirName(path, dir);
		rc = MkDirHier(dir);
		if (rc<0) return(-1);

		size_t chsz = _chunksizehint;
		rc = wasp->Create(
			path, NC_WRITE | NC_64BIT_OFFSET, 0, chsz, 
			_open_var.GetCRatios().size()
		);
		if (rc<0) return(-1);

		rc = _DefVar(wasp, var, max_ts);
		if (rc<0) return(-1);

		rc = wasp->EndDef();
		if (rc<0) return(-1);

	}
	rc = wasp->OpenVarWrite(varname, lod);
	if (rc<0) return(-1);

	_open_file = wasp;

	_open_slice_num = 0; 
	_open_ts = ts;
	_open_file_ts = file_ts;
	_open_varname = varname;
    return(0);
}

int VDCNetCDF::CloseVariable() {

	if (_open_file) {
		_open_file->CloseVar();
	}
	if (_open_file && _open_file != _master) {
		_open_file->Close();
		delete _open_file;
		_open_file = NULL;
	}
	_open_slice_num = 0;
	return(0);
}

int VDCNetCDF::Write(const float *data) {
	if (! _open_file ||  ! _open_write) {
		SetErrMsg("No variable open for writing");
		return(-1);
	}

	vector <size_t> sdims;
	size_t numts;
	VDC::ParseDimensions(_open_var.GetDimensions(), sdims, numts);
	assert(sdims.size() >= 0 && sdims.size() <= 3);

	bool time_varying = VDC::IsTimeVarying(_open_var.GetName());

	vector <size_t> start;
	vector <size_t> count;

	vector <size_t> mins(sdims.size(), 0);
	vector <size_t> maxs = sdims;
	for (int i=0; i<maxs.size(); i++) maxs[i] -= 1;

	vdc_2_ncdfcoords(
		_open_file_ts, _open_file_ts, time_varying, mins, maxs, start, count
	);

	return(_open_file->PutVara(start, count, data));
}

int VDCNetCDF::_WriteSlice(WASP *file, const float *slice) {
	vector <size_t> sdims;
	size_t numts;
	VDC::ParseDimensions(_open_var.GetDimensions(), sdims, numts);
	assert(sdims.size() >= 2 && sdims.size() <= 3);
	
	vector <size_t> sbs = _open_var.GetBS();	// bs, spatial dims only

	bool time_varying = false;
	if (VDC::IsTimeVarying(_open_var.GetName())) {
		sbs.pop_back();
		time_varying = true;
	}

	assert(sbs.size() == sdims.size());

	size_t slice_size, slices_per_slab, num_slices;
	ws_info(sdims, sbs, slice_size, slices_per_slab, num_slices);

	_slice_buffer = (float *) _sb_slice_buffer.Alloc(
		slice_size * slices_per_slab * sizeof (*slice)
	);

	size_t offset = (_open_slice_num  % slices_per_slab) * slice_size;

	memcpy(_slice_buffer + offset, slice, slice_size*sizeof(*slice));
	_open_slice_num++;

	if (
		((_open_slice_num % slices_per_slab) == 0) ||
		_open_slice_num == num_slices
	) {
		//
		// Min and max extents of entire volume
		//
		vector <size_t> mins(sdims.size(), 0);
		vector <size_t> maxs = sdims;
		for (int i=0; i<maxs.size(); i++) maxs[i] -= 1;


		if (maxs.size() > 2) {
			mins[mins.size()-1] = (_open_slice_num - 1) / sbs[sbs.size()-1] * sbs[sbs.size()-1];
			maxs[maxs.size()-1] = _open_slice_num - 1;
			if (maxs[maxs.size()-1] >= num_slices) {
				maxs[maxs.size()-1] = num_slices-1;
			}
		}

		//
		// Map from VDC to NetCDF coordinates
		//
		vector <size_t> start;
		vector <size_t> count;
		vdc_2_ncdfcoords(
			_open_file_ts, _open_file_ts, time_varying, mins, maxs, start, count
		);

		int rc = file->PutVara(start, count, _slice_buffer);
		return(rc);
	}
	return(0);
}


int VDCNetCDF::WriteSlice(const float *slice) {
	if (! _open_file ||  ! _open_write) {
		SetErrMsg("No variable open for writing");
		return(-1);
	}

	return(_WriteSlice( _open_file, slice));
}

int VDCNetCDF::Read(float *data) {
	if (! _open_file ||  _open_write) {
		SetErrMsg("No variable open for reading");
		return(-1);
	}

	vector <size_t> sdims;
	size_t numts;
	VDC::ParseDimensions(_open_var.GetDimensions(), sdims, numts);
	assert(sdims.size() >= 0 && sdims.size() <= 3);

	bool time_varying = VDC::IsTimeVarying(_open_var.GetName());

	vector <size_t> start;
	vector <size_t> count;

	vector <size_t> mins(sdims.size(), 0);
	vector <size_t> maxs = sdims;
	for (int i=0; i<maxs.size(); i++) maxs[i] -= 1;

	vdc_2_ncdfcoords(
		_open_file_ts, _open_file_ts, time_varying, mins, maxs, start, count
	);

	return(_open_file->GetVara(start, count, data));
}

int VDCNetCDF::_ReadSlice(WASP *file, float *slice) {

	vector <size_t> dims_at_level;
	vector <size_t> bs_at_level;
	(void) file->InqVarDimlens(
		_open_var.GetName(), _open_level, dims_at_level, bs_at_level
	);
	reverse(dims_at_level.begin(), dims_at_level.end());  // NetCDF order
	reverse(bs_at_level.begin(), bs_at_level.end());  // NetCDF order


	vector <size_t> sdims = dims_at_level;
	vector <size_t> sbs = dims_at_level;
	size_t numts = 0;
	bool time_varying = false;
	if (VDC::IsTimeVarying(_open_var.GetName())) {
		numts = sdims[sdims.size()-1];
		sdims.pop_back();
		sbs.pop_back();
		time_varying = true;
	}
	assert(sdims.size() >= 2 && sdims.size() <= 3);

	size_t slice_size, slices_per_slab, num_slices;
	ws_info(sdims, sbs, slice_size, slices_per_slab, num_slices);

	if (_open_slice_num == 0) {

		_slice_buffer = (float *) _sb_slice_buffer.Alloc(
			slice_size * slices_per_slab * sizeof (*slice)
		);
	}

	if (
		((_open_slice_num % slices_per_slab) == 0) ||
		_open_slice_num == num_slices	// not block aligned in Z
	) {
		//
		// Min and max extents of entire volume
		//
		vector <size_t> mins(sdims.size(), 0);
		vector <size_t> maxs = sdims;
		for (int i=0; i<maxs.size(); i++) maxs[i] -= 1;

		if (maxs.size() > 2) {
			mins[mins.size()-1] = _open_slice_num;
			maxs[maxs.size()-1] = _open_slice_num +sbs[sbs.size()-1] - 1;
			if (maxs[maxs.size()-1] >= num_slices) maxs[maxs.size()-1] = num_slices-1;
		}


		//
		// Map from VDC to NetCDF coordinates
		//
		vector <size_t> start;
		vector <size_t> count;
		vdc_2_ncdfcoords(
			_open_file_ts, _open_file_ts, time_varying, mins, maxs, start, count
		);

		int rc = file->GetVara(start, count, _slice_buffer);
		if (rc<0) return(rc);
	}

	size_t offset = (_open_slice_num  % slices_per_slab) * slice_size;
	memcpy(slice, _slice_buffer + offset, slice_size*sizeof(*slice));
	_open_slice_num++;

	return(0);
}


int VDCNetCDF::ReadSlice(float *slice) {
	if (! _open_file ||  _open_write) {
		SetErrMsg("No variable open for writing");
		return(-1);
	}

	return(_ReadSlice(_open_file, slice));
}
    
int VDCNetCDF::ReadRegion(
    const vector<size_t> &min, const vector<size_t> &max, float *region
) {
	vector <size_t> sdims;
	size_t numts;
	VDC::ParseDimensions(_open_var.GetDimensions(), sdims, numts);
	assert(sdims.size() >= 0 && sdims.size() <= 3);

	bool time_varying = VDC::IsTimeVarying(_open_var.GetName());

	vector <size_t> start;
	vector <size_t> count;
	vdc_2_ncdfcoords(
		_open_file_ts, _open_file_ts, time_varying, min, max, start, count
	);

	return(_open_file->GetVara(start, count, region));
}     

int VDCNetCDF::ReadRegionBlock(
    const vector<size_t> &min, const vector<size_t> &max, float *region
) {
	vector <size_t> sdims;
	size_t numts;
	VDC::ParseDimensions(_open_var.GetDimensions(), sdims, numts);
	assert(sdims.size() >= 0 && sdims.size() <= 3);

	bool time_varying = VDC::IsTimeVarying(_open_var.GetName());

	vector <size_t> start;
	vector <size_t> count;
	vdc_2_ncdfcoords(
		_open_file_ts, _open_file_ts, time_varying, min, max, start, count
	);

	return(_open_file->GetVaraBlock(start, count, region));
}     

int VDCNetCDF::PutVar(string varname, int lod, const float *data) {
	CloseVariable();

	vector <size_t> dims_at_level;
	vector <size_t> dummy;
	int rc = VDCNetCDF::GetDimLensAtLevel(
		varname, -1, dims_at_level, dummy
	);
	if (rc<0) return(-1);

	// If not a 1D time-varying variable. 
	//
	if (! (VDC::IsTimeVarying(varname) && dims_at_level.size() == 1)) {


		// Remove time dimension if time-varying
		//
		if (VDC::IsTimeVarying(varname)) dims_at_level.pop_back();

		// Number of per time step
		//
		size_t var_size = 1;
		for (int i=0; i<dims_at_level.size(); i++) var_size *= dims_at_level[i];

		int numts = VDC::GetNumTimeSteps(varname);

		const float *ptr = data;
		for (size_t ts = 0; ts<numts; ts++) {
			rc = VDCNetCDF::PutVar(ts, varname, lod, ptr);
			if (rc<0) return(-1);

			ptr += var_size;
		}

		return(0);
	}

	// Write 1D time-varying variables directly with 
	// NetCDFCpp class. 
	//

	VDC::BaseVar var;
	if (! VDC::GetBaseVarInfo(varname, var)) {
        SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(false);
	}

	// Don't currently handle case where a variable is split across
	// multiple files.
	//
	if (! _var_in_master(var)) {
		SetErrMsg("Distributed variable reads not supported");
		return(-1);
	}

	// N.B. calling NetCDFCpp::PutVar
	//
	rc = ((NetCDFCpp *) _master)->PutVar(varname, data);
	if (rc<0) return(-1);

	return(0);

}

int VDCNetCDF::PutVar(
	size_t ts, string varname, int lod, const float *data
) {
	CloseVariable();

	int rc = VDCNetCDF::OpenVariableWrite(ts, varname, lod);
	if (rc<0) return(-1);

	rc = VDCNetCDF::Write(data);
	if (rc<0) return(-1);

	rc = CloseVariable();
	if (rc<0) return(-1);

	return(0);
}

int VDCNetCDF::GetVar(string varname, int level, int lod, float *data) {
	CloseVariable();

	vector <size_t> dims_at_level;
	vector <size_t> dummy;
	int rc = VDCNetCDF::GetDimLensAtLevel(
		varname, level, dims_at_level, dummy
	);
	if (rc<0) return(-1);

	// If not a 1D time-varying variable. 
	//
	if (! (VDC::IsTimeVarying(varname) && dims_at_level.size() == 1)) {

		// Remove time dimension if time-varying
		//
		if (VDC::IsTimeVarying(varname)) dims_at_level.pop_back();

		// Number of per time step
		//
		size_t var_size = 1;
		for (int i=0; i<dims_at_level.size(); i++) var_size *= dims_at_level[i];

		size_t numts = VDC::GetNumTimeSteps(varname);

		float *ptr = data;
		for (size_t ts = 0; ts<numts; ts++) {
			rc = VDCNetCDF::GetVar(ts, varname, level, lod, ptr);
			if (rc<0) return(-1);

			ptr += var_size;
		}

		return(0);
	}

	// Read 1D time-varying variables directly with 
	// NetCDFCpp class
	//

	VDC::BaseVar var;
	if (! VDC::GetBaseVarInfo(varname, var)) {
        SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(false);
	}

	// Don't currently handle case where a variable is split across
	// multiple files.
	//
	if (! _var_in_master(var)) {
		SetErrMsg("Distributed variable reads not supported");
		return(-1);
	}

	// N.B. calling NetCDFCpp::GetVar()
	//
	rc = ((NetCDFCpp *) _master)->GetVar(varname, data);
	if (rc<0) return(-1);

	return(0);

}

int VDCNetCDF::GetVar(
	size_t ts, string varname, int level, int lod, float *data
) {
	CloseVariable();

	int rc = VDCNetCDF::OpenVariableRead(ts, varname, level, lod);
	if (rc<0) return(-1);

	rc = VDCNetCDF::Read(data);
	if (rc<0) return(-1);

	rc = VDCNetCDF::CloseVariable();
	if (rc<0) return(-1);

	return(0);
}

bool VDCNetCDF::CompressionInfo(
    std::vector <size_t> bs, string wname, size_t &nlevels, size_t &maxcratio
 ) const { 
	nlevels = 1;
	maxcratio = 1;
	if (wname.empty()) return(true);

    std::reverse(bs.begin(), bs.end()); // NetCDF order
    return(WASP::InqCompressionInfo(bs, wname, nlevels, maxcratio));
}


bool VDCNetCDF::VariableExists(
    size_t ts,
    string varname,
    int reflevel,
    int lod
) const {

	VDC::BaseVar var;
	if (! VDC::GetBaseVarInfo(varname, var))  return(false);

	string path;
	size_t file_ts;
	size_t max_ts;
	int rc = GetPath(varname, ts, path, file_ts, max_ts);
	if (rc<0) return(-1);

	vector <string> paths;
	if (! var.IsCompressed()) {
		paths.push_back(path);
	}
	else {
		int numfiles = var.GetCRatios().size();
		paths = WASP::GetPaths(path, numfiles);
	}

	int ncratios = var.GetCRatios().size();

	if (lod > ncratios-1) lod = ncratios-1;
	if (lod < 0) lod = lod + ncratios;
	if (lod < 0) lod = 0;

	for (int i=0; i<=lod; i++) {
		struct stat statbuf;
		if (stat(path.c_str(), &statbuf) < 0) return (false);
	}
    return(true);
}

    

int VDCNetCDF::_WriteMasterMeta() {


	int rc;
	map <string, Dimension>::const_iterator itr;
	for (itr = _dimsMap.begin(); itr != _dimsMap.end(); ++itr) {
		const Dimension &dimension = itr->second;

		rc = _master->DefDim(
			dimension.GetName(), dimension.GetLength()
		);
		if (rc<0) return(-1);
	}
	

	rc = _master->PutAtt("", "VDC.Version", _version);
	if (rc<0) return(rc);

	rc = _master->PutAtt("", "VDC.BlockSize", _bs);
	if (rc<0) return(rc);

	rc = _master->PutAtt("", "VDC.WaveName", _wname);
	if (rc<0) return(rc);

	rc = _master->PutAtt("", "VDC.CompressionRatios", _cratios);
	if (rc<0) return(rc);

	rc = _master->PutAtt("", "VDC.MasterThreshold", _master_threshold);
	if (rc<0) return(rc);

	rc = _master->PutAtt("", "VDC.VariableThreshold",_variable_threshold);
	if (rc<0) return(rc);

	vector <int> periodic;
	for (int i=0; i<_periodic.size(); i++) {
		periodic.push_back((int) _periodic[i]);
	}
	rc = _master->PutAtt("", "VDC.Periodic", periodic);
	if (rc<0) return(rc);


	rc = _WriteMasterDimensions();
	if (rc<0) return(rc);

	rc = _WriteMasterAttributes();
	if (rc<0) return(rc);

	rc = _WriteMasterCoordVarsDefs();
	if (rc<0) return(rc);

	rc = _WriteMasterDataVarsDefs();
	if (rc<0) return(rc);

	rc = _master->EndDef();
	if (rc<0) return(rc);


	return(0);
}

int VDCNetCDF::_ReadMasterMeta() {

    int rc = _master->Open(_master_path, 0);
	if (rc<0) return(-1);

	rc = _master->GetAtt("", "VDC.Version", _version);
	if (rc<0) return(rc);

	rc = _master->GetAtt("", "VDC.BlockSize", _bs);
	if (rc<0) return(rc);

	rc = _master->GetAtt("", "VDC.WaveName", _wname);
	if (rc<0) return(rc);

	rc = _master->GetAtt("", "VDC.CompressionRatios", _cratios);
	if (rc<0) return(rc);

	rc = _master->GetAtt("", "VDC.MasterThreshold", _master_threshold);
	if (rc<0) return(rc);

	rc = _master->GetAtt("", "VDC.VariableThreshold",_variable_threshold);
	if (rc<0) return(rc);

	vector <int> periodic;
	rc = _master->GetAtt("", "VDC.Periodic", periodic);

	_periodic.clear();
	for (int i=0; i<periodic.size(); i++) {
		_periodic.push_back((bool) periodic[i]);
	}
	if (rc<0) return(rc);

	rc = _ReadMasterDimensions();
	if (rc<0) return(rc);

	rc = _ReadMasterAttributes();
	if (rc<0) return(rc);

	rc = _ReadMasterCoordVarsDefs();
	if (rc<0) return(rc);

	rc = _ReadMasterDataVarsDefs();
	if (rc<0) return(rc);

	return(0);
}

int VDCNetCDF::_ReadMasterDimensions() {

	_dimsMap.clear();

	string tag = "VDC.DimensionNames";
	vector <string> dimnames;
	int rc = _master->GetAtt("", tag, dimnames);
	if (rc<0) return(rc);
	
	for (int i=0; i<dimnames.size(); i++) {

		tag = "VDC.Dimension." + dimnames[i] + ".Length";
		int length;
		rc = _master->GetAtt("", tag, length);
		if (rc<0) return(rc);

		tag = "VDC.Dimension." + dimnames[i] + ".Axis";
		int axis;
		rc = _master->GetAtt("", tag, axis);
		if (rc<0) return(rc);

		VDC::Dimension dimension(dimnames[i], (size_t) length, (size_t) axis);

		_dimsMap[dimnames[i]] = dimension;
		
	}
	return(0);
}

int VDCNetCDF::_ReadMasterAttributes (
	string prefix, map <string, Attribute> &atts
) {
	atts.clear();
	
	string tag = prefix + ".AttributeNames";
	vector <string> attnames;
	int rc = _master->GetAtt("", tag, attnames);
	if (rc<0) return(rc);

	
	for (int i=0; i<attnames.size(); i++) {

		tag = prefix + ".Attribute." + attnames[i] + ".XType";
		VDC::XType xtype;
		rc = _master->GetAtt("", tag, (int &) xtype);
		if (rc<0) return(rc);

		tag = prefix + ".Attribute." + attnames[i] + ".Values";
		switch (xtype) {
			case FLOAT:
			case DOUBLE: {
				vector <double> values;
				rc = _master->GetAtt("", tag, values);
				if (rc<0) return(rc);
				VDC::Attribute attr(attnames[i], xtype, values);
				atts[attnames[i]] = attr;
				
			break;
			}
			case INT32:
			case INT64: {
				vector <int> values;
				rc = _master->GetAtt("", tag, values);
				if (rc<0) return(rc);
				VDC::Attribute attr(attnames[i], xtype, values);
				atts[attnames[i]] = attr;
			break;
			}
			case TEXT: {
				string values;
				rc = _master->GetAtt("", tag, values);
				if (rc<0) return(rc);
				VDC::Attribute attr(attnames[i], xtype, values);
				atts[attnames[i]] = attr;
			break;
			}
			default:
				SetErrMsg("Invalid attribute xtype : %d", xtype);
				return(-1);
			break;
		}
	}
	return(0);
}

int VDCNetCDF::_ReadMasterAttributes () {

	string prefix = "VDC";
	return (_ReadMasterAttributes(prefix, _atts));
}

int VDCNetCDF::_ReadMasterBaseVarDefs(string prefix, BaseVar &var) {
	
	string tag;

	tag = prefix + "." + var.GetName() + ".DimensionNames";
	vector <string> dimnames;
	int rc = _master->GetAtt("", tag, dimnames);
	if (rc<0) return(rc);

	vector <VDC::Dimension> dimensions;
	for (int i=0; i<dimnames.size(); i++) {
		std::map <string, Dimension>::iterator itr = _dimsMap.find(dimnames[i]);
		if (itr == _dimsMap.end()) {
            SetErrMsg("Dimension name %s not defined", dimnames[i].c_str());
			return(-1);
		}
		dimensions.push_back(itr->second);
	}
	var.SetDimensions(dimensions);

	tag = prefix + "." + var.GetName() + ".Units";
	string units;
	rc = _master->GetAtt("", tag, units);
	if (rc<0) return(rc);
	var.SetUnits(units);

	tag = prefix + "." + var.GetName() + ".XType";
	int xtype;
	rc = _master->GetAtt("", tag, xtype);
	if (rc<0) return(rc);
	var.SetXType((VAPoR::VDC::XType) xtype);

	tag = prefix + "." + var.GetName() + ".Periodic";
	vector <int> iperiodic;
	rc = _master->GetAtt("", tag, iperiodic);
	vector <bool> periodic;
	for (int i=0; i<iperiodic.size(); i++) periodic.push_back(iperiodic[i]);
	if (rc<0) return(rc);
	var.SetPeriodic(periodic);

	tag = prefix + "." + var.GetName() + ".BlockSize";
	vector <size_t> bs;
	rc = _master->GetAtt("", tag, bs);
	if (rc<0) return(rc);
	var.SetBS(bs);

	tag = prefix + "." + var.GetName() + ".WaveName";
	string wname;
	rc = _master->GetAtt("", tag, wname);
	if (rc<0) return(rc);
	var.SetWName(wname);

	tag = prefix + "." + var.GetName() + ".CompressionRatios";
	vector <size_t> cratios;
	rc = _master->GetAtt("", tag, cratios);
	if (rc<0) return(rc);
	var.SetCRatios(cratios);

	
	prefix += "." + var.GetName();
	map <string, Attribute> atts;
	rc = _ReadMasterAttributes(prefix, atts);
	if (rc<0) return(rc);

	var.SetAttributes(atts);
	return(0);
}

int VDCNetCDF::_ReadMasterCoordVarsDefs() {
	_coordVars.clear();

	string tag = "VDC.CoordVarNames";
	vector <string> varnames;
	int rc = _master->GetAtt("", tag, varnames);
	if (rc<0) return(rc);


	string prefix = "VDC.CoordVar";
	for (int i=0; i<varnames.size(); i++) {
		CoordVar cvar;
		cvar.SetName(varnames[i]);

		tag = prefix + "." + cvar.GetName() + ".Axis";
		int axis;
		int rc = _master->GetAtt("", tag, axis);
		if (rc<0) return(rc);
		cvar.SetAxis(axis);

		tag = prefix + "." + cvar.GetName() + ".UniformHint";
		int uniform;
		rc = _master->GetAtt("", tag, uniform);
		if (rc<0) return(rc);
		cvar.SetUniform(uniform);

		rc = _ReadMasterBaseVarDefs(prefix, cvar);
		if (rc<0) return(rc);

		_coordVars[varnames[i]] = cvar;
	}
	return(0);
}

int VDCNetCDF::_ReadMasterDataVarsDefs() {

	string tag = "VDC.DataVarNames";
	vector <string> varnames;
	int rc = _master->GetAtt("", tag, varnames);
	if (rc<0) return(rc);


	string prefix = "VDC.DataVar";
	for (int i=0; i<varnames.size(); i++) {
		DataVar var;
		var.SetName(varnames[i]);

		tag = prefix + "." + var.GetName() + ".CoordVars";
		vector <string> coordvars;
		int rc = _master->GetAtt("", tag, coordvars);
		if (rc<0) return(rc);
		var.SetCoordvars(coordvars);

		tag = prefix + "." + var.GetName() + ".HasMissing";
		int has_missing;
		rc = _master->GetAtt("", tag, has_missing);
		if (rc<0) return(rc);
		var.SetHasMissing(has_missing);

		if (has_missing) {
			tag = prefix + "." + var.GetName() + ".MissingValue";
			double missing_value;
			rc = _master->GetAtt("", tag, missing_value);
			if (rc<0) return(rc);
			var.SetMissingValue(missing_value);
		}

		rc = _ReadMasterBaseVarDefs(prefix, var);
		if (rc<0) return(rc);

		_dataVars[varnames[i]] = var;
	}
	return(0);

}


int VDCNetCDF::_WriteMasterDimensions() {
	map <string, Dimension>::const_iterator itr;
	string s;
	for (itr = _dimsMap.begin(); itr != _dimsMap.end(); ++itr) {
		s += itr->first;
		s+= " ";
	}
	string tag = "VDC.DimensionNames";
	int rc = _master->PutAtt("", tag, s);
	if (rc<0) return(rc);

	
	for (itr = _dimsMap.begin(); itr != _dimsMap.end(); ++itr) {
		const Dimension &dimension = itr->second;

		tag = "VDC.Dimension." + dimension.GetName() + ".Length";
		rc = _master->PutAtt("", tag, dimension.GetLength());
		if (rc<0) return(rc);

		tag = "VDC.Dimension." + dimension.GetName() + ".Axis";
		rc = _master->PutAtt("", tag, dimension.GetAxis());
		if (rc<0) return(rc);
		
	}
	return(0);
}

int VDCNetCDF::_WriteMasterAttributes (
	string prefix, const map <string, Attribute> &atts
) {

	map <string, Attribute>::const_iterator itr;
	string s;
	for (itr = atts.begin(); itr != atts.end(); ++itr) {
		s += itr->first;
		s+= " ";
	}
	string tag = prefix + ".AttributeNames";
	int rc = _master->PutAtt("", tag, s);
	if (rc<0) return(rc);

	
	for (itr = atts.begin(); itr != atts.end(); ++itr) {
		const Attribute &attr = itr->second;

		tag = prefix + ".Attribute." + attr.GetName() + ".XType";
		rc = _master->PutAtt("", tag, attr.GetXType());
		if (rc<0) return(rc);

		tag = prefix + ".Attribute." + attr.GetName() + ".Values";
	
		rc = _PutAtt(_master, "", tag, attr);
		if (rc<0) return(rc);

	}
	return(0);
}

int VDCNetCDF::_WriteMasterAttributes () {

	string prefix = "VDC";
	return (_WriteMasterAttributes(prefix, _atts));
}

int VDCNetCDF::_WriteMasterBaseVarDefs(string prefix, const BaseVar &var) {
	
	string tag;

	tag = prefix + "." + var.GetName() + ".DimensionNames";
	const vector <Dimension> &dimensions = var.GetDimensions();
	string s;
	for (int i=0; i<dimensions.size(); i++) {
		s += dimensions[i].GetName();
		s+= " ";
	}

	int rc = _master->PutAtt("", tag, s);
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".Units";
	rc = _master->PutAtt("", tag, var.GetUnits());
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".XType";
	rc = _master->PutAtt("", tag, (int) var.GetXType());
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".Periodic";
	vector <bool> periodic = var.GetPeriodic();
	vector <int> iperiodic;
	for (int i=0; i<periodic.size(); i++) iperiodic.push_back(periodic[i]);
	rc = _master->PutAtt("", tag, iperiodic);
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".BlockSize";
	rc = _master->PutAtt("", tag, var.GetBS());
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".WaveName";
	rc = _master->PutAtt("", tag, var.GetWName());
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".CompressionRatios";
	rc = _master->PutAtt("", tag, var.GetCRatios());
	if (rc<0) return(rc);

	
	prefix += "." + var.GetName();
	return (_WriteMasterAttributes(prefix, var.GetAttributes()));
		
}

int VDCNetCDF::_WriteMasterCoordVarsDefs() {
	map <string, CoordVar>::const_iterator itr;
	string s;
	for (itr = _coordVars.begin(); itr != _coordVars.end(); ++itr) {
		s += itr->first;
		s+= " ";
	}

	string tag = "VDC.CoordVarNames";
	int rc = _master->PutAtt("", tag, s);
	if (rc<0) return(rc);


	string prefix = "VDC.CoordVar";
	for (itr = _coordVars.begin(); itr != _coordVars.end(); ++itr) {
		const CoordVar &cvar = itr->second;

		int rc; 
		if (_var_in_master(cvar)) {
			size_t numts = VDC::GetNumTimeSteps(cvar.GetName());
			rc = _DefVar(_master, cvar, numts);
			if (rc<0) return(-1);
		}

		tag = prefix + "." + cvar.GetName() + ".Axis";
		rc = _master->PutAtt("", tag, cvar.GetAxis());
		if (rc<0) return(rc);

		tag = prefix + "." + cvar.GetName() + ".UniformHint";
		rc = _master->PutAtt("", tag, (int) cvar.GetUniform());
		if (rc<0) return(rc);

		rc = _WriteMasterBaseVarDefs(prefix, cvar);
		if (rc<0) return(rc);

	}
	return(0);

}

int VDCNetCDF::_WriteMasterDataVarsDefs() {
	map <string, DataVar>::const_iterator itr;
	string s;
	for (itr = _dataVars.begin(); itr != _dataVars.end(); ++itr) {
		s += itr->first;
		s+= " ";
	}

	string tag = "VDC.DataVarNames";
	int rc = _master->PutAtt("", tag, s);
	if (rc<0) return(rc);


	string prefix = "VDC.DataVar";
	for (itr = _dataVars.begin(); itr != _dataVars.end(); ++itr) {
		const DataVar &var = itr->second;

		if (_var_in_master(var)) {
			size_t numts = VDC::GetNumTimeSteps(var.GetName());
			rc = _DefVar(_master, var, numts);
			if (rc<0) return(-1);
		}

		tag = prefix + "." + var.GetName() + ".CoordVars";
		int rc = _master->PutAtt("", tag, var.GetCoordvars());
		if (rc<0) return(rc);

		tag = prefix + "." + var.GetName() + ".HasMissing";
		rc = _master->PutAtt("", tag, (int) var.GetHasMissing());
		if (rc<0) return(rc);

		if (var.GetHasMissing()) {
			tag = prefix + "." + var.GetName() + ".MissingValue";
			rc = _master->PutAtt("", tag, var.GetMissingValue());
			if (rc<0) return(rc);
		}

		rc = _WriteMasterBaseVarDefs(prefix, var);
		if (rc<0) return(rc);

	}
	return(0);

}

int VDCNetCDF::_DefVar(
	WASP *wasp,
	const VDC::BaseVar &var,
	size_t max_ts
) {
	vector <VDC::Dimension> dims = var.GetDimensions();

	vector <string> dimnames;
	for (int i=0; i<dims.size(); i++) {
		size_t len = dims[i].GetAxis() == 3 ? max_ts : dims[i].GetLength();

		// Don't define same dimension twice
		//
		if (! wasp->InqDimDefined(dims[i].GetName())) {
			int rc = wasp->DefDim(dims[i].GetName(), len);
			if (rc<0) return(-1);
		}

		dimnames.push_back(dims[i].GetName());
	}
	reverse(dimnames.begin(), dimnames.end());	// NetCDF order


	vector <size_t> bs = var.GetBS();
	reverse(bs.begin(), bs.end());	// NetCDF order
	int rc = wasp->DefVar(
		var.GetName(), vdc_xtype2ncdf_xtype(var.GetXType()), 
		dimnames, var.GetWName(), bs, var.GetCRatios()
	);
	if (rc<0) return(-1);

	// 
	// Attributes
	//
	const CoordVar *cvarptr = dynamic_cast<const CoordVar *> (& var);
	if (cvarptr) {
	
		rc = wasp->PutAtt(cvarptr->GetName(), "Axis", cvarptr->GetAxis());
		if (rc<0) return(-1);

		rc = wasp->PutAtt(
			cvarptr->GetName(), "UniformHint", cvarptr->GetUniform()
		);
		if (rc<0) return(-1);
	}

	const DataVar *varptr = dynamic_cast<const DataVar *> (& var);
	if (varptr) {
		rc = wasp->PutAtt(
			varptr->GetName(), "CoordVars", varptr->GetCoordvars()
		);
		if (rc<0) return(-1);

		if (varptr->GetHasMissing()) {
			rc = wasp->PutAtt(
				varptr->GetName(), "HasMissing", varptr->GetMissingValue()
			);
			if (rc<0) return(rc);
		}
	}

	rc = wasp->PutAtt(var.GetName(), "Units", var.GetUnits());
	if (rc<0) return(rc);

	rc = wasp->PutAtt(var.GetName(), "BlockSize", var.GetBS());
	if (rc<0) return(rc);

	vector <bool> periodic = var.GetPeriodic();
	vector <int> iperiodic;
	for (int i=0; i<periodic.size(); i++) iperiodic.push_back(periodic[i]);
	rc = wasp->PutAtt(var.GetName(), "Periodic", iperiodic);
	if (rc<0) return(rc);

	const map <string, Attribute> &atts = var.GetAttributes();
	map <string, Attribute>::const_iterator itr;
	for (itr = atts.begin(); itr != atts.end(); ++itr) {
		const Attribute &attr = itr->second;
	
		rc = _PutAtt(wasp, var.GetName(), "", attr);
		if (rc<0) return(-1);
	}

	return(0);
}

int VDCNetCDF::_PutAtt(
	WASP *wasp,
	string varname,
	string tag,
	const Attribute &attr
) {
	if (tag.empty()) tag = attr.GetName();

	int rc;
	switch (attr.GetXType()) {
	case FLOAT:
	case DOUBLE: {
		vector <double> values;
		attr.GetValues(values);
		rc = wasp->PutAtt(varname, tag, values);
		if (rc<0) return(rc);
	break;
	}
	case INT32:
	case INT64: {
		vector <int> values;
		attr.GetValues(values);
		rc = wasp->PutAtt(varname, tag, values);
		if (rc<0) return(rc);
	break;
	}
	case TEXT: {
		string values;
		attr.GetValues(values);
		rc = wasp->PutAtt(varname, tag, values);
		if (rc<0) return(rc);
	break;
	}
	default:
		SetErrMsg(
			"Invalid value, Attribute::GetXType() : %d", 
			attr.GetXType()
		);
		return(-1);
	break;
	}

	return(0);
}

bool VDCNetCDF::_var_in_master(const VDC::BaseVar &var) const {

	if (var.IsTimeVarying() && var.GetDimensions().size() > 3) return(false);

	size_t nelements = 1;
	size_t ngridpoints = 1;
	for (int i=0; i<var.GetDimensions().size(); i++) {
		nelements *= var.GetDimensions()[i].GetLength(); 
		if (var.GetDimensions()[i].GetAxis() != 3) {
			ngridpoints *= var.GetDimensions()[i].GetLength();
		}
	}
	if (nelements < _master_threshold &&  ! var.IsCompressed()) {
		return(true);
	}

	return(false);
}
