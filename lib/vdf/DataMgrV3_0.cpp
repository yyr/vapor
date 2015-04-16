#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cfloat>
#include <vector>
#include <map>
#include <vapor/VDCNetCDF.h>
#include <vapor/DataMgrV3_0.h>
#ifdef WIN32
#include <float.h>
#endif
using namespace VetsUtil;
using namespace VAPoR;

namespace {

// Format a vector as a space-separated element string
//
template <class T>
string vector_to_string(vector <T> v) {
	ostringstream oss;

	oss << "[";
	for (int i=0; i<v.size(); i++) {
		oss << v[i] << " ";
	}
	oss << "]";
	return(oss.str());
}

// convert tuple of multi-dimensional coordinates, 'coord', for a space
// with min and max bounds to a linear offset from 'min'
//
size_t linearize(
	const vector <size_t> &min, 
	const vector <size_t> &max, 
	const vector <size_t> &coord
) {
	size_t offset = 0;

	assert (min.size() == max.size());
	assert (min.size() == max.size());

	for (int i=0; i<coord.size(); i++) {
		assert(coord[i] >= min[i]);
		assert(coord[i] <= max[i]);
	}

	size_t factor = 1;
	for (int i=0; i<coord.size(); i++) {
		offset += factor * (coord[i] - min[i]);
		factor *= max[i] - min[i] + 1;
	}
	return(offset);
}

// Convert an offset to a coordinate vector. Inverse of linearize()
//
vector <size_t> vectorize(
	const vector <size_t> &min, 
	const vector <size_t> &max, 
	size_t offset 
) {
	vector <size_t> coord;

	assert (min.size() == max.size());

	coord.resize(min.size());

	size_t factor = 1;
	for (int i=0; i<coord.size(); i++) {
		coord[i] = (offset % (factor * (max[i]-min[i]+1))) / factor;
		offset = offset - coord[i] * factor;
		factor *= (max[i]-min[i]+1);
	}
	assert(offset == 0);

	return(coord);
}

// Product of elements in a vector
//
size_t vproduct(vector <size_t> a) {
    size_t ntotal = 1;

    for (int i=0; i<a.size(); i++) ntotal *= a[i];
    return(ntotal);
}

// Find the index for an axis in a vector of axes
//
int lookup_axis_index(
	const vector <int> &axes,
	int axis
) {
	for (int i=0; i<axes.size(); i++) {
		if (axes[i] == axis) return(i);
	}
	return(-1);
}

size_t decimate_length(size_t l) {
	
		if (l % 2) return((l+1) / 2);
		else return(l / 2);
}

// Compute dimensions of a multidimensional array, specified by dims, 
// after undergoing decimiation
//
vector <size_t> decimate_dims(const vector <size_t> &dims, int l) {
	vector <size_t> new_dims = dims;
	for (int i=0; i<l; i++) {
		for (int j=0; j<new_dims.size(); j++) {
			new_dims[j] = decimate_length(new_dims[j]);
		}
	}
	return(new_dims);
}

// Decimate a 1D array with simple averaging. 
//
void decimate1d(
	const vector <size_t> &src_bs, const float *src, float *dst
) {
	assert(src_bs.size() == 1);

	vector <size_t> dst_bs;
	for (int i=0; i<src_bs.size(); i++) {
		dst_bs.push_back(decimate_length(src_bs[i]));
	}

	size_t ni0 = src_bs[0];

	size_t ni1 = dst_bs[0];

	for (size_t i=0; i<ni1; i++) {
		dst[i] = 0.0;
	}

	// Perform in-place 4-node averaging, handling odd boundary cases
	//
	for (size_t i=0, ii=0; i<ni0; i++) {
		float iwgt = 0.5;
		if (i==ni0-1 && ni0 % 2) iwgt = 1.0;	// odd i boundary
		float w = iwgt;

		assert(w <= 1.0);

		dst[ii] += w * src[i];
		if (i % 2) ii++;
	}
}

void decimate2d(
	const vector <size_t> &src_bs, const float *src, float *dst
) {
	assert(src_bs.size() == 2);

	vector <size_t> dst_bs;
	for (int i=0; i<src_bs.size(); i++) {
		dst_bs.push_back(decimate_length(src_bs[i]));
	}

	size_t ni0 = src_bs[0];
	size_t nj0 = src_bs[1];

	size_t ni1 = dst_bs[0];
	size_t nj1 = dst_bs[1];

	for (size_t j=0; j<nj1; j++) {
	for (size_t i=0; i<ni1; i++) {
		dst[j*ni1 + i] = 0.0;
	}
	}

	// Perform in-place 4-node averaging, handling odd boundary cases
	//
	for (size_t j=0, jj=0; j<nj0; j++) {
		float jwgt = 0.0;
		if (j==nj0-1 && nj0 % 2) jwgt = 0.25;	// odd j boundary

		for (size_t i=0, ii=0; i<ni0; i++) {
			float iwgt = 0.25;
			if (i==ni0-1 && ni0 % 2) iwgt = 0.5;	// odd i boundary
			float w = iwgt + jwgt;

			assert(w <= 1.0);

			dst[jj*ni1 + ii] += w * src[j*ni0 + i];
			if (i % 2) ii++;
		}
		if (j % 2) jj++;
	}
}


void decimate3d(
	const vector <size_t> &src_bs, const float *src, float *dst
) {
	assert(src_bs.size() == 3);

	vector <size_t> dst_bs;
	for (int i=0; i<src_bs.size(); i++) {
		dst_bs.push_back(decimate_length(src_bs[i]));
	}

	size_t ni0 = src_bs[0];
	size_t nj0 = src_bs[1];
	size_t nk0 = src_bs[2];

	size_t ni1 = dst_bs[0];
	size_t nj1 = dst_bs[1];
	size_t nk1 = dst_bs[2];

	for (size_t k=0; k<nk1; k++) {
	for (size_t j=0; j<nj1; j++) {
	for (size_t i=0; i<ni1; i++) {
		dst[k*ni1*nj1 + j*ni1 + i] = 0.0;
	}
	}
	}

	// Perform in-place 8-node averaging, handling odd boundary cases
	//
	for (size_t k=0, kk=0; k<nk0; k++) {
		float kwgt = 0.0;
		if (k==nk0-1 && nk0 % 2) kwgt = 0.125;	// odd k boundary

		for (size_t j=0, jj=0; j<nj0; j++) {
			float jwgt = 0.0;
			if (j==nj0-1 && nj0 % 2) jwgt = 0.125;	// odd j boundary

			for (size_t i=0, ii=0; i<ni0; i++) {
				float iwgt = 0.125;
				if (i==ni0-1 && ni0 % 2) iwgt = 0.25;	// odd i boundary
				float w = iwgt + jwgt + kwgt;

				assert(w <= 1.0);

				dst[kk*ni1*nj1 + jj*ni1 + ii] += w * src[k*ni0*nj0 + j*ni0 + i];
				if (i % 2) ii++;
			}
			if (j % 2) jj++;
		}
		if (k % 2) kk++;
	}
}

// Perform decimation of a 1D, 2D, or 3D blocked array
//
void decimate(
	const vector <size_t> &bmin, const vector <size_t> &bmax, 
	const vector <size_t> &src_bs, const float *src, float *dst
) {
	assert(bmin.size() == bmax.size());
	assert(bmin.size() == src_bs.size());
	assert(src_bs.size() >= 1 && src_bs.size() <= 3);

	vector <size_t> dst_bs;
	for (int i=0; i<src_bs.size(); i++) {
		dst_bs.push_back(decimate_length(src_bs[i]));
	}

	size_t src_block_size = vproduct(src_bs);
	size_t dst_block_size = vproduct(dst_bs);

	if (src_bs.size() == 1) {
		for (int i=bmin[0]; i<= bmax[0]; i++) {
			decimate1d(src_bs, src, dst);
			src += src_block_size;
			dst += dst_block_size;
		}
	}
	else if (src_bs.size() == 2) {
		for (int j=bmin[1]; j<= bmax[1]; j++) {
		for (int i=bmin[0]; i<= bmax[0]; i++) {
			decimate2d(src_bs, src, dst);
			src += src_block_size;
			dst += dst_block_size;
		}
		}
	}
	else {
		for (int k=bmin[2]; k<= bmax[2]; k++) {
		for (int j=bmin[1]; j<= bmax[1]; j++) {
		for (int i=bmin[0]; i<= bmax[0]; i++) {
			decimate3d(src_bs, src, dst);
			src += src_block_size;
			dst += dst_block_size;
		}
		}
		}
	}
}


// Map voxel to block coordinates
//
void map_vox_to_blk(
	vector <size_t> bs, const vector <size_t> &vcoord, vector <size_t> &bcoord
) {
	assert(bs.size() == vcoord.size());
	bcoord.clear();

    for (int i=0; i<bs.size(); i++) {
        bcoord.push_back(vcoord[i] / bs[i]);
    }
}

void map_blk_to_vox(
	vector <size_t> bs, 
	const vector <size_t> &bmin, const vector <size_t> &bmax,
	vector <size_t> &vmin, vector <size_t> &vmax
) {
	assert(bs.size() == bmin.size());
	assert(bs.size() == bmax.size());
	vmin.clear();
	vmax.clear();

    for (int i=0; i<bs.size(); i++) {
		vmin.push_back(bmin[i] * bs[i]);
		vmax.push_back(bmax[i] * bs[i] + bs[i] - 1);
    }
}

// Extract various grid related metadata from a BaseVar class object
//
void grid_params(
	const DC::BaseVar &var,
	const vector <size_t> &min,
	const vector <size_t> &max,
	const vector <size_t> &dims,
	bool periodic[3],
	bool &has_missing,
	float &missing
) {
	for (int i=0; i<3; i++) {
		periodic[i] = false;
	}
	has_missing = false;

	vector <bool> has_periodic = var.GetPeriodic();
	for (int i=0; i<3; i++) {
		if (has_periodic[i] && min[i]==0 && max[i]==dims[i]-1) {
			periodic[i] = true;
		}
	}


	const DC::DataVar *dvarptr = dynamic_cast<const DC::DataVar *> (&var);
	if (dvarptr->GetHasMissing()) {
		has_missing = true;
		missing = dvarptr->GetMissingValue();
	}
}

// Return an ordered vector identify the spatial axes of a variable
//
vector <int> get_spatial_axes(const DC::BaseVar &var) {
	vector <int> axis;

	vector <DC::Dimension> dims = var.GetDimensions();
	for (int i=0; i<dims.size(); i++) {
		if (dims[i].GetAxis() != 3) axis.push_back(dims[i].GetAxis());
	}
	return(axis);
}

};


DataMgrV3_0::DataMgrV3_0(
	string format,
	size_t mem_size,
	int nthreads
) {

	SetDiagMsg(
		"DataMgrV3_0::DataMgrV3_0(%s,%d,%d)",
		format.c_str(), nthreads, mem_size
	);

	_format = format;
	_nthreads = nthreads;
	_mem_size = mem_size;

	_dc = NULL;

	_blk_mem_mgr = NULL;

	_PipeLines.clear();

	_regionsList.clear();
	

	_varInfoCache.Clear();
}


DataMgrV3_0::~DataMgrV3_0(
) {
	SetDiagMsg("DataMgrV3_0::~DataMgrV3_0()");

	if (_dc) delete _dc;
	_dc = NULL;


	Clear();
	if (_blk_mem_mgr) delete _blk_mem_mgr;

	_blk_mem_mgr = NULL;

}

int DataMgrV3_0::Initialize(const vector <string> &files) {
	if (!_format.compare("vdc")) {
		_dc = new VDCNetCDF(_nthreads);
	}
	else {
		SetErrMsg("Invalid data collection format : %s", _format.c_str());
		return(-1);
	}

	int rc = _dc->Initialize(files);
	if (rc<0) {
		SetErrMsg("Failed to initialize data importer");
		return(-1);
	}

	rc = _get_time_coordinates(_timeCoordinates);
	if (rc<0) {
		SetErrMsg("Failed to get time coordinates");
		return(-1);
	}
	return(0);
}



vector <string> DataMgrV3_0::GetDataVarNames() const {
	return(_dc->GetDataVarNames());
}

vector <string> DataMgrV3_0::GetDataVarNames(int ndim, bool spatial) const {
	vector <string> allnames = DataMgrV3_0::GetDataVarNames();

	vector <string> names;
	for (int i=0; i<allnames.size(); i++) {
		DC::DataVar var;

		// No error checking here!!!!!
		//
		DataMgrV3_0::GetDataVarInfo(allnames[i], var);
		vector <DC::Dimension> dims = var.GetDimensions();
		int rank;
		if (spatial && DataMgrV3_0::IsTimeVarying(allnames[i])) {
			rank = dims.size() - 1;
		} 
		else {
			rank = dims.size();
		}
		if (rank == ndim) names.push_back(allnames[i]);
	}
	return(names);
}

vector <string> DataMgrV3_0::GetCoordVarNames() const {
	return(_dc->GetCoordVarNames());
}



vector <string> DataMgrV3_0::GetCoordVarNames(int ndim, bool spatial) const {
	vector <string> allnames = DataMgrV3_0::GetCoordVarNames();

	vector <string> names;
	for (int i=0; i<allnames.size(); i++) {
		DC::CoordVar var;

		// No error checking here!!!!!
		//
		GetCoordVarInfo(allnames[i], var);
		vector <DC::Dimension> dims = var.GetDimensions();
		int rank;
		if (spatial && DataMgrV3_0::IsTimeVarying(allnames[i])) {
			rank = dims.size() - 1;
		} 
		else {
			rank = dims.size();
		}
		if (rank == ndim) names.push_back(allnames[i]);
	}
	return(names);
}


bool DataMgrV3_0::GetDataVarInfo(
	string varname, VAPoR::DC::DataVar &var
) const {
	// 
	// NEED TO HANDLE DERIVED VARS
	//
	return(_dc->GetDataVarInfo(varname, var));
}

bool DataMgrV3_0::GetCoordVarInfo(
	string varname, VAPoR::DC::CoordVar &var
) const {
	return(_dc->GetCoordVarInfo(varname, var));
}

bool DataMgrV3_0::GetBaseVarInfo(
	string varname, VAPoR::DC::BaseVar &var
) const {

	// 
	// NEED TO HANDLE DERIVED VARS
	//
	return(_dc->GetBaseVarInfo(varname, var));
}

bool DataMgrV3_0::IsTimeVarying(string varname) const {
		DC::BaseVar var;

		// No error checking here!!!!!
		//
		GetBaseVarInfo(varname, var);
		vector <DC::Dimension> dims = var.GetDimensions();

		for (int i=0; i<dims.size(); i++) {
			if (dims[i].GetAxis() == 3) return (true);
		}
		return(false);
}

bool DataMgrV3_0::IsCompressed(string varname) const {
	DC::BaseVar var;

	// No error checking here!!!!!
	//
	bool status = GetBaseVarInfo(varname, var);
	if (! status) return (status);

	return(var.IsCompressed());
}

int DataMgrV3_0::GetNumTimeSteps(string varname) const {
	DC::BaseVar var;
	int rc = GetBaseVarInfo(varname, var);
	if (rc<0) return(rc);

	vector <DC::Dimension> dims = var.GetDimensions();

	for (int i=0; i<dims.size(); i++) {
		if (dims[i].GetAxis() == 3) return (dims[i].GetLength());
	}
	return(1);	// not time varying
}

int DataMgrV3_0::GetNumRefLevels(string varname) const {
	return(_dc->GetNumRefLevels(varname));
}

int DataMgrV3_0::GetCRatios(string varname, vector <size_t> &cratios) const {
	cratios.clear();

	DC::BaseVar var;
	int rc = GetBaseVarInfo(varname, var);
	if (rc<0) return(rc);

	cratios = var.GetCRatios();
	return(0);
}

RegularGrid *DataMgrV3_0::GetVariable (
	size_t ts, string varname, int level, int lod, bool lock
) {
	SetDiagMsg(
		"DataMgrV3_0::GetVariable(%d,%s,%d,%d,%d, %d)",
		ts,varname.c_str(), level, lod, lock
	);

	int rc = _level_correction(varname, level);
	if (rc<0) return(NULL);

	rc = _lod_correction(varname, lod);
	if (rc<0) return(NULL);

	RegularGrid *rg = _getVariable(ts, varname, level, lod, lock, false);
	if (! rg) {
		SetErrMsg(
			"Failed to read variable \"%s\" at time step (%d), and\n"
			"refinement level (%d) and level-of-detail (%d)",
			varname.c_str(), ts, level, lod
		);
	}
	return(rg);
}

RegularGrid *DataMgrV3_0::GetVariable (
	size_t ts, string varname, int level, int lod,
    vector <double> min, vector <double> max, bool lock
) {
	SetDiagMsg(
		"DataMgrV3_0::GetVariable(%d, %s, %d, %d, %s, %s, %d)",
		ts,varname.c_str(), level, lod, vector_to_string(min).c_str(),
		vector_to_string(max).c_str(), lock
	);

	int rc = _level_correction(varname, level);
	if (rc<0) return(NULL);

	rc = _lod_correction(varname, lod);
	if (rc<0) return(NULL);

	//
	// Find the coordinates in voxels of the grid that contains
	// the axis aligned bounding box specified in user coordinates
	// by min and max
	//
	vector <size_t> min_ui, max_ui;
	rc = _find_bounding_grid(
		ts, varname, level, lod, min, max, min_ui, max_ui
	);
	if (rc<0) return(NULL);

	return(DataMgrV3_0::GetVariable(ts, varname, level, lod, min_ui, max_ui));

}

RegularGrid *DataMgrV3_0::_getVariable(
	size_t ts,
	string varname,
	int level,
	int lod,
	bool	lock,
	bool	dataless
) {

	vector <size_t> dims_at_level;
	vector <size_t> dummy;
	int rc = _dc->GetDimLensAtLevel(
		varname, level, dims_at_level, dummy
	);
	if (rc < 0) {
		SetErrMsg("Invalid variable reference : %s", varname.c_str());
		return(NULL);
	}

	// Remove time dimension if time-varying
	//
	if (DataMgrV3_0::IsTimeVarying(varname)) {
		dims_at_level.pop_back();
	}

	vector <size_t> min;
	vector <size_t> max;
	for (int i=0; i<dims_at_level.size(); i++) {
		min.push_back(0);
		max.push_back(dims_at_level[i]-1);
	}

	return( DataMgrV3_0::_getVariable(
		ts, varname, level, lod, min, max, lock, dataless
	));
}

int	DataMgrV3_0::_setupCoordVecs(
	size_t ts,
	string varname,
	int level,
	int lod,
	const vector <size_t> &min,
	const vector <size_t> &max,
	vector <string> &varnames,
	vector < vector <size_t > > &dimsvec,
	vector < vector <size_t > > &dims_at_levelvec,
	vector < vector <size_t > > &bsvec,
	vector < vector <size_t > > &bs_at_levelvec,
	vector < vector <size_t > > &bminvec,
	vector < vector <size_t > > &bmaxvec
) const {
	varnames.clear();
	dimsvec.clear();
	dims_at_levelvec.clear();
	bsvec.clear();
	bs_at_levelvec.clear();
	bminvec.clear();
	bmaxvec.clear();

	DC::DataVar dvar;
	if (DataMgrV3_0::GetDataVarInfo(varname, dvar) < 0) {
		SetErrMsg("Unrecognized variable name : %s", varname.c_str());
		return(-1);
	}

	// Spatial dimension axes
	//
	vector <int> data_axes = get_spatial_axes(dvar);

	// Get names of all coordinate variables
	//
	vector <string> cvarnames;
	string dummy1;
	int rc = _get_coord_vars(varname, cvarnames, dummy1);
	if (rc<0) return(-1); 

	// data varname + coord varnames
	//
	varnames.push_back(varname);
	varnames.insert(varnames.end(), cvarnames.begin(), cvarnames.end());

	// Set up various dimension vectors for each of the coordinate
	// variables, which in general have different shape then the 
	// data variable
	//
	for (int i=0; i<varnames.size(); i++) {
		DC::BaseVar var;
		if (DataMgrV3_0::GetBaseVarInfo(varnames[i], var) < 0) {
			SetErrMsg("Unrecognized variable name : %s", varnames[i].c_str());
			return(-1);
		}

		// Grid and block dimensions at max refinement
		//
		vector <size_t> dims;
		vector <size_t> bs;
		int rc = _dc->GetDimLensAtLevel(varnames[i], -1, dims, bs);
		if (rc < 0) {
			SetErrMsg("Invalid variable reference : %s", varnames[i].c_str());
			return(-1);
		}

		// Remove time dimension if time-varying
		//
		if (var.IsTimeVarying()) {
			dims.pop_back();
			bs.pop_back();
		}
		dimsvec.push_back(dims);
		bsvec.push_back(bs);

		// Grid and block dimensions at requested refinement
		//
		vector <size_t> dims_at_level;
		vector <size_t> bs_at_level;
		rc = _dc->GetDimLensAtLevel(varnames[i], level, dims_at_level, bs_at_level);
		if (rc < 0) {
			SetErrMsg("Invalid variable reference : %s", varnames[i].c_str());
			return(-1);
		}

		// Remove time dimension if time-varying
		//
		if (var.IsTimeVarying()) {
			dims_at_level.pop_back();
			bs_at_level.pop_back();
		}

		dims_at_levelvec.push_back(dims_at_level);
		bs_at_levelvec.push_back(bs_at_level);

		// Spatial dimension axes
		//
		vector <int> axes = get_spatial_axes(var);


		// Reshape min and max to match coordinate variable
		//
		vector <size_t> mymin;
		vector <size_t> mymax;
		for (int i=0; i<axes.size(); i++) {
			int index = lookup_axis_index(data_axes, axes[i]);
			assert(index>=0 && index<=2);

			mymin.push_back(min[index]);
			mymax.push_back(max[index]);
		}

		// Map voxel coordinates into block coordinates
		//
		vector <size_t> bmin, bmax;
		map_vox_to_blk(bs_at_level, mymin, bmin);
		map_vox_to_blk(bs_at_level, mymax, bmax);

		bminvec.push_back(bmin);
		bmaxvec.push_back(bmax);
	}
	return(0);
}

RegularGrid *DataMgrV3_0::_getVariable(
	size_t ts,
	string varname,
	int level,
	int lod,
	vector <size_t> min,
	vector <size_t> max,
	bool	lock,
	bool	dataless
) {
	RegularGrid *rg = NULL;

	DC::DataVar dvar;
	if (DataMgrV3_0::GetDataVarInfo(varname, dvar) < 0) {
		SetErrMsg("Unrecognized variable name : %s", varname.c_str());
		return(NULL);
	}

	vector <string> varnames;
	vector < vector <size_t > > dimsvec;
	vector < vector <size_t > > dims_at_levelvec;
	vector < vector <size_t > > bsvec;
	vector < vector <size_t > > bs_at_levelvec;
	vector < vector <size_t > > bminvec;
	vector < vector <size_t > > bmaxvec;
	

	int	rc = _setupCoordVecs(
		ts, varname, level, lod, min, max, varnames,
		dimsvec, dims_at_levelvec, bsvec, bs_at_levelvec, bminvec, bmaxvec
	);
	if (rc<0) return(NULL);

	//
	// if dataless we only load coordinate data
	//
	if (dataless) varnames[0].clear();

    vector <float *> blkvec;
	rc = DataMgrV3_0::_get_regions(
		ts, varnames, level, lod, true, bsvec, bminvec, bmaxvec, blkvec
	);
	if (rc < 0) return(NULL);

	if (DataMgrV3_0::IsVariableDerived(varname) && ! blkvec[0]) {
		//
		// Derived variable that is not in cache, so we need to 
		// create it
		//
#ifdef	DEAD
		rg = execute_pipeline(
				ts, varname, level, lod, min, max, lock,
				xcblks, ycblks, zcblks
			);
#endif
	}
	else {
		rg = _make_grid(
			dvar, min, max, dims_at_levelvec[0], blkvec, 
			bs_at_levelvec, bminvec, bmaxvec
		);
	}

	if (!rg) {
		for (int i=0; i<blkvec.size(); i++) {
			if (blkvec[i]) _unlock_blocks(blkvec[i]);
		}
		return (NULL);
	}

	// 
	// Safe to remove locks now that were not explicitly requested
	//
	if (! lock) {
		for (int i=0; i<blkvec.size(); i++) {
			if (blkvec[i]) _unlock_blocks(blkvec[i]);
		}
	}

	return(rg);
}

RegularGrid *DataMgrV3_0::GetVariable(
	size_t ts,
	string varname,
	int level,
	int lod,
	vector <size_t> min,
	vector <size_t> max,
	bool	lock
) {
	SetDiagMsg(
		"DataMgrV3_0::GetVariable(%d, %s, %d, %d, %s, %s, %d)",
		ts,varname.c_str(), level, lod, vector_to_string(min).c_str(),
		vector_to_string(max).c_str(), lock
	);

	int rc = _level_correction(varname, level);
	if (rc<0) return(NULL);

	rc = _lod_correction(varname, lod);
	if (rc<0) return(NULL);

	RegularGrid *rg = _getVariable(
		ts, varname, level, lod, min, max, lock, false
	);
	if (! rg) {
		SetErrMsg(
			"Failed to read variable \"%s\" at time step (%d), and\n"
			"refinement level (%d) and level-of-detail (%d)",
			varname.c_str(), ts, level, lod
		);
	}
	return(rg);
}

int DataMgrV3_0::GetVariableExtents(
    size_t ts, string varname, int level,
    vector <double> &min , vector <double> &max
) {
	min.clear();
	max.clear();

	int rc = _level_correction(varname, level);
	if (rc<0) return(-1);

	vector <string> cvars;
	string dummy;
	rc = _get_coord_vars(varname, cvars, dummy);
	if (rc<0) return(-1);

	string key = "VariableExtents";
	vector <double> values;
	if (_varInfoCache.Get(ts, cvars, level, 0, key, values)) {
		int n = values.size();
		for (int i=0; i<n/2; i++) {
				min.push_back(values[i]);
				max.push_back(values[i+n]);
		}
		return(0);
	}


	RegularGrid *rg = _getVariable(ts, varname, level, level, false, true);
	if (! rg) return(-1);

	double extents[6];
	rg->GetUserExtents(extents);
	size_t dimlens[3];
	rg->GetDimensions(dimlens);

	for (int i=0; i<3; i++) {
		if (dimlens[i] > 1) {
			min.push_back(extents[i]);
			max.push_back(extents[i+3]);
		}
	}

	// Cache results 
	//
	values.clear();
	for (int i=0; i<min.size(); i++) values.push_back(min[i]);
	for (int i=0; i<max.size(); i++) values.push_back(max[i]);

	return(0);
}

#ifdef	DEAD

int	DataMgrV3_0::NewPipeline(PipeLine *pipeline) {

	//
	// Delete any pipeline stage with the same name as the new one. This
	// is a no-op if the stage doesn't exist.
	//
	RemovePipeline(pipeline->GetName());

	// 
	// Make sure outputs don't collide with existing outputs
	//
	const vector <pair <string, VarType_T> > &my_outputs = pipeline->GetOutputs();

	for (int i=0; i<_PipeLines.size(); i++) {
		const vector <pair <string,VarType_T> > &outputs = _PipeLines[i]->GetOutputs();
		for (int j=0; j<my_outputs.size(); j++) {
		for (int k=0; k<outputs.size(); k++) {
			if (my_outputs[j].first.compare(outputs[k].first)==0) {
				SetErrMsg(
					"Pipeline output %s already in use", 
					my_outputs[j].first.c_str()
				);
				return(-1);
			}
		}
		}
	}

	//
	// Now make sure outputs don't match any native variables
	//
	vector <string> native_vars = _get_native_variables();

	for (int i=0; i<native_vars.size(); i++) {
		for (int j=0; j<my_outputs.size(); j++) {
			if (native_vars[i].compare(my_outputs[j].first) == 0) {
				SetErrMsg(
					"Pipeline output %s matches native variable name", 
					my_outputs[i].first.c_str()
				);
				return(-1);
			}
		}
	}
		

	//
	// Add the new stage to a temporary pipeline. Generate a hash
	// table with all the dependencies of the temporary pipeline. And
	// then check for cycles in the graph (e.g. a -> b -> c -> a).
	//
	map <string, vector <string> > graph;
	vector <PipeLine *> tmp_pipe = _PipeLines;
	tmp_pipe.push_back(pipeline);

	for (int i=0; i<tmp_pipe.size(); i++) {
		vector <string> depends;
		for (int j=0; j<tmp_pipe.size(); j++) {
	
			// 
			// See if inputs to tmp_pipe[i] match outputs 
			// of tmp_pipe[j]
			//
			if (depends_on(tmp_pipe[i], tmp_pipe[j])) {
				depends.push_back(tmp_pipe[j]->GetName());
			}
		}
		graph[tmp_pipe[i]->GetName()] = depends;
	}

	//
	// Finally check for cycles in the graph
	//
	if (cycle_check(graph, pipeline->GetName(), graph[pipeline->GetName()])) {
		SetErrMsg("Invalid pipeline : circular dependency detected");
		return(-1);
	}

	_PipeLines.push_back(pipeline);
	return(0);
}

void	DataMgrV3_0::RemovePipeline(string name) {

	vector <PipeLine *>::iterator itr;
	for (itr = _PipeLines.begin(); itr != _PipeLines.end(); itr++) {
		if (name.compare((*itr)->GetName()) == 0) {
			_PipeLines.erase(itr);

			const vector <pair <string, VarType_T> > &output_vars = (*itr)->GetOutputs();
			//
			// Remove any cached instances of variable
			//
			for (int i=0; i<output_vars.size(); i++) {
				string var = output_vars[i].first;
				DataMgrV3_0::PurgeVariable(var);
			}
			break;
		}
	}
}

#endif

bool DataMgrV3_0::VariableExists(
    size_t ts, string varname, int level, int lod
) {

	
    // disable error reporting
    //
    bool enabled = EnableErrMsg(false);
	int rc = _level_correction(varname, level);
	if (rc<0) {
		EnableErrMsg(enabled);
		return(false);
	}

	rc = _lod_correction(varname, lod);
	if (rc<0) {
		EnableErrMsg(enabled);
		return(false);
	}
	EnableErrMsg(enabled);

	string key = "VariableExists";
	vector <size_t> dummy;
	if (_varInfoCache.Get(ts, varname, level, lod, key, dummy)) {
		return(true);
	}

	// If a data variable need to test for existance of all coordinate
	// variables
	//
	vector <string> varnames;
	varnames.push_back(varname);
	VAPoR::DC::DataVar datavar;
	if (DataMgrV3_0::GetDataVarInfo(varname, datavar)) {
		vector <string> cvars = datavar.GetCoordvars();
		for (int i=0; i<cvars.size(); i++) varnames.push_back(cvars[i]);
	}

	// Separate native and derived variables. Derived variables are 
	// recursively tested
	//
	vector <string> native_vars;
	vector <string> derived_vars;
	for (int i = 0; i<varnames.size(); i++) {
		if (DataMgrV3_0::IsVariableNative(varnames[i])) {
			native_vars.push_back(varnames[i]);
		}
		else {
			derived_vars.push_back(varnames[i]);
		}
	}


	// Check native variables
	//
	vector <size_t> exists_vec;
	for (int i=0; i<native_vars.size(); i++) {
		if (_varInfoCache.Get(ts, varname, level, lod, key, exists_vec)) {
			continue;
		}
		bool exists = _dc->VariableExists(ts, varname, level, lod);
		if (exists) {
			_varInfoCache.Set(ts, varname, level, lod, key, exists_vec);
		}
		else {
			return(false);
		}
	}

	// Check derived variables
	//
#ifdef	DEAD
	for (int i=0; i<derived_vars.size(); i++) {

		PipeLine *pipeline = get_pipeline_for_var(varnames[i]);
		if(pipeline == NULL) {
			return (false);
		}

		vector  <string> ivars = pipeline->GetInputs();

		//
		// Recursively test existence of all dependencies
		//
		for (int i=0; i<ivars.size(); i++) {
			if (! VariableExists(ts, ivars[i], level, lod)) {
				return(false);
			}
		}
	}
#endif
	_varInfoCache.Set(ts, varname, level, lod, key, exists_vec);
	return(true);
}


bool DataMgrV3_0::IsVariableNative(string name) const {
	vector <string> svec = _get_native_variables();

	for (int i=0; i<svec.size(); i++) {
		if (name.compare(svec[i]) == 0) return (true);
	}
	return(false);
}

bool DataMgrV3_0::IsVariableDerived(string name) const {
	if (name.size() == 0) return(false);

	vector <string> svec = _get_derived_variables();

	for (int i=0; i<svec.size(); i++) {
		if (name.compare(svec[i]) == 0) return (true);
	}
	return(false);
}

void	DataMgrV3_0::Clear() {

	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); itr++) {
		const region_t &region = *itr;

		if (region.blks) _blk_mem_mgr->FreeMem(region.blks);
			
	}
	_regionsList.clear();
	_varInfoCache.Clear();
}


#ifdef	DEAD
int DataMgrV3_0::GetDataRange(
	size_t ts,
	const char *varname,
	float *range,
	int level,
	int lod
) {
	int rc = _level_correction(varname, level);
	if (rc<0) return(-1);

	rc = _lod_correction(varname, lod);
	if (rc<0) return(-1);

	int	rc;
	range[0] = range[1] = 0.0;

	SetDiagMsg("DataMgrV3_0::GetDataRange(%d,%s)", ts, varname);


	// See if we've already cache'd it.
	//
	if (_VarInfoCache.GetRange(ts, varname, level, lod, range)) {
		return(0);
	}

	// Range isn't cache'd. Need to get it from derived class
	//
	if (DataMgrV3_0::IsVariableNative(varname)) {

		rc = _OpenVariableRead(ts, varname, level,lod);
		if (rc < 0) {
			SetErrMsg(
				"Failed to read variable/timestep/level/lod (%s, %d, %d, %d)",
				varname, ts, 0,0
			);
			return(-1);
		}

		const float *r = _GetDataRange();
		if (r) {
			range[0] = r[0];
			range[1] = r[1];

#ifdef WIN32
			if (! _finite(range[0]) || _isnan(range[0])) range[0] = FLT_MIN;
#else
			if (! finite(range[0]) || isnan(range[0])) range[0] = FLT_MIN;
#endif
#ifdef WIN32
			if (! _finite(range[1]) || _isnan(range[1])) range[1] = FLT_MIN;
#else
			if (! finite(range[1]) || isnan(range[1])) range[1] = FLT_MIN;
#endif

			_VarInfoCache.SetRange(ts, varname, level, lod, range);

			_CloseVariable();
			return(0);
		}
		_CloseVariable();
	}

	//
	// Argh. Child class doesn't know data range (or this is a
	// derived variable). Have to caculate range
	// ourselves
	//
	size_t min[3], max[3];
	rc = DataMgrV3_0::GetValidRegion(ts, varname, level, min, max);
	if (rc<0) return(-1);

	
	const RegularGrid *rg = DataMgrV3_0::GetGrid(
		ts, varname, level, lod, min, max, 1
	);
	if (! rg) return(-1);


	RegularGrid::ConstIterator itr;
	bool first = true;
	float mv = rg->GetMissingValue();
	for (itr = rg->begin(); itr!=rg->end(); ++itr) {
		float v = *itr;
		if (v != mv) {
			if (first) {
				range[0] = range[1] = v;
				first = false;
			}
			if (v < range[0]) range[0] = v;
			if (v > range[1]) range[1] = v;
		}
	}
	DataMgrV3_0::UnlockGrid(rg);
	delete rg;

	_VarInfoCache.SetRange(ts, varname, level, lod, range);

	return(0);
}
#endif
	

void	DataMgrV3_0::UnlockGrid(
	const RegularGrid *rg
) {
	SetDiagMsg("DataMgrV3_0::UnlockGrid()");
	float **blks = rg->GetBlks();
	if (blks) _unlock_blocks(blks[0]);

#ifdef	DEAD
	const LayeredGrid *lg = dynamic_cast<const LayeredGrid *>(rg);
	if (lg) {
		float **coords = lg->GetCoordBlks();
		if (coords) _unlock_blocks(coords[0]);
	}
#endif
}
	


float	*DataMgrV3_0::_get_region_from_cache(
	size_t ts,
	string varname,
	int level,
	int lod,
	const vector <size_t> &bmin,
	const vector <size_t> &bmax,
	bool	lock
) {

	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); itr++) {
		region_t &region = *itr;

		if (region.ts == ts &&
			region.varname.compare(varname) == 0 &&
			region.level == level &&
			region.lod == lod &&
			region.bmin == bmin &&
			region.bmax == bmax) {

			// Increment the lock counter
			region.lock_counter += lock ? 1 : 0;

			// Move region to front of list
			region_t tmp_region = region;
			_regionsList.erase(itr);
			_regionsList.push_back(tmp_region);

			SetDiagMsg(
				"DataMgrV3_0::_get_region_from_cache() - data in cache %xll\n",
				 tmp_region.blks
			);
			return(tmp_region.blks);
		}
	}

	return(NULL);
}

float *DataMgrV3_0::_get_region_from_fs(
	size_t ts, string varname, int level, int lod,
    const vector <size_t> &bs, const vector <size_t> &bmin, 
	const vector <size_t> &bmax, bool lock
) {

	float *blks = _alloc_region(
		ts, varname, level, lod, bmin, bmax, bs, sizeof(float), lock, false
	);
	if (! blks) return(NULL);

    vector <size_t> min, max;
	map_blk_to_vox(bs, bmin, bmax, min, max);

	int rc = _dc->OpenVariableRead(ts, varname, level, lod);
    if (rc < 0) return(NULL);

	rc = _dc->ReadRegionBlock(min, max, blks);
    if (rc < 0) {
		_free_region(ts,varname ,level,lod,bmin,bmax);
		_dc->CloseVariable(); 
		return(NULL);
	}

	rc = _dc->CloseVariable(); 
	if (rc<0) return(NULL);

	SetDiagMsg("DataMgrV3_0::GetGrid() - data read from fs\n");
	return(blks);
}

float *DataMgrV3_0::_get_region(
	size_t ts,
	string varname,
	int level,
	int nlevels,
	int lod,
	int nlods,
	const vector <size_t> &bs,
	const vector <size_t> &bmin,
	const vector <size_t> &bmax,
	bool lock
) {
	if (lod < -nlods) lod = -nlods;


	// See if region is already in cache. If not, read from the 
	// file system.
	//
	float *blks = _get_region_from_cache(
		ts, varname, level, lod, bmin, bmax, lock
	);
	if (! blks ) {

		// If level not available we recursively decimate
		//
		if (level < -nlevels) {
			level++;

			blks = _get_region(
				ts, varname, level, nlevels, lod, nlods,
				bs, bmin, bmax, false
			);
			if (blks) {
				vector <size_t> bs_at_level = decimate_dims(bs, -level - 1);
				vector <size_t> bs_at_level_m1 = decimate_dims(bs, -level);

				float *newblks = _alloc_region(
					ts, varname, level-1, lod, bmin, bmax, bs_at_level_m1, 
					sizeof(float), false, false
				);
				if (! blks) return(NULL);

				decimate(bmin, bmax, bs_at_level, blks, newblks); 
				return(newblks);
			}
		} 
		else if (! DataMgrV3_0::IsVariableDerived(varname)) {
			vector <size_t> bs_at_level = decimate_dims(bs, -level - 1);

			blks = (float *) _get_region_from_fs(
				ts, varname, level, lod, bs_at_level, bmin, bmax, lock
			);
		}
		if (! blks) {
			SetErrMsg(
				"Failed to read region from variable/timestep/level/lod (%s, %d, %d, %d)",
				varname.c_str(), ts, level, lod
			);
			return(NULL);
		}
	}
	return(blks);
}

int DataMgrV3_0::_get_regions(
	size_t ts, 
	const vector <string> &varnames, 
	int level, int lod, 
	bool lock,
	const vector < vector <size_t > > &bsvec,	// native coordinates
	const vector < vector <size_t> > &bminvec,
	const vector < vector <size_t> > &bmaxvec,
	vector <float *> &blkvec
) {

	blkvec.clear();

	for (int i=0; i<varnames.size(); i++) {
		float *blks = NULL;

		if (varnames[i].empty()) {	// nothing to do
			blkvec.push_back(NULL);
			continue;
		}

		int nlevels = DataMgrV3_0::GetNumRefLevels(varnames[i]);
		if (nlevels < 0) return(-1);

		DC::BaseVar var;
		int rc = GetBaseVarInfo(varnames[i], var);
		if (rc<0) return(rc);

		int nlods = var.GetCRatios().size();

		size_t my_ts = ts;

		// If variable isn't time varying time step should always be 0
		//
		if (! DataMgrV3_0::IsTimeVarying(varnames[i])) my_ts = 0; 

		blks = _get_region(
			my_ts, varnames[i], level, nlevels, lod, nlods,
			bsvec[i], bminvec[i], bmaxvec[i], true
		);
		if (! blks) {
			for (int i=0; i<blkvec.size(); i++) {
				if (blkvec[i]) _unlock_blocks(blkvec[i]);
			}
			return(-1);
		}
		blkvec.push_back(blks);
	}

	// 
	// Safe to remove locks now that were not explicitly requested
	//
	if (! lock) {
		for (int i=0; i<blkvec.size(); i++) {
			if (blkvec[i]) _unlock_blocks(blkvec[i]);
		}
	}
	return(0);
			
}


float	*DataMgrV3_0::_alloc_region(
	size_t ts,
	string varname,
	int level,
	int lod,
	vector <size_t> bmin,
	vector <size_t> bmax,
	vector <size_t> bs,
	int element_sz,
	bool	lock,
	bool fill
) {
	assert(bmin.size() == bmax.size());
	assert(bmin.size() == bs.size());

	size_t mem_block_size;
	if (! _blk_mem_mgr) {

		mem_block_size = 1024 * 1024;

		size_t num_blks = (_mem_size * 1024 * 1024) / mem_block_size;

		BlkMemMgr::RequestMemSize(mem_block_size, num_blks);
		_blk_mem_mgr = new BlkMemMgr();
		if (BlkMemMgr::GetErrCode() != 0) {
			return(NULL);
		}
	}
	mem_block_size = BlkMemMgr::GetBlkSize();

	// Free region already exists
	//
	_free_region(ts,varname,level,lod,bmin,bmax);


	size_t size = element_sz;
	for (int i=0; i<bmin.size(); i++) {
		size *= (bmax[i]-bmin[i]+1) * bs[i];
	}
	
	size_t nblocks = (size_t) ceil((double) size / (double) mem_block_size);
		
	float *blks;
	while (! (blks = (float *) _blk_mem_mgr->Alloc(nblocks, fill))) {
		if (! _free_lru()) {
			SetErrMsg("Failed to allocate requested memory");
			return(NULL);
		}
	}

	region_t region;

	region.ts = ts;
	region.varname = varname;
	region.level = level;
	region.lod = lod;
	region.bmin = bmin;
	region.bmax = bmax;
	region.lock_counter = lock ? 1 : 0;
	region.blks = blks;

	_regionsList.push_back(region);

	return(region.blks);
}

void	DataMgrV3_0::_free_region(
	size_t ts,
	string varname,
	int level,
	int lod,
	vector <size_t> bmin,
	vector <size_t> bmax
) {

	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); itr++) {
		const region_t &region = *itr;

		if (region.ts == ts &&
			region.varname.compare(varname) == 0 &&
			region.level == level &&
			region.lod == lod &&
			region.bmin == bmin &&
			region.bmax == bmax)  {

			if (region.lock_counter == 0) {
				if (region.blks) _blk_mem_mgr->FreeMem(region.blks);
				
				_regionsList.erase(itr);
				return;
			}
		}
	}

	return;
}


void	DataMgrV3_0::_free_var(string varname) {

	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); ) {
		const region_t &region = *itr;

		if (region.varname.compare(varname) == 0) {

			if (region.blks) _blk_mem_mgr->FreeMem(region.blks);
				
			_regionsList.erase(itr);
			itr = _regionsList.begin();
		}
		else itr++;
	}

}


bool	DataMgrV3_0::_free_lru(
) {

	// The least recently used region is at the front of the list
	//
	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); itr++) {
		const region_t &region = *itr;

		if (region.lock_counter == 0) {
			if (region.blks) _blk_mem_mgr->FreeMem(region.blks);
			_regionsList.erase(itr);
			return(true);
		}
	}

	// nothing to free
	return(false);
}
	

#ifdef	DEAD
PipeLine *DataMgrV3_0::get_pipeline_for_var(string varname) const {

	for (int i=0; i<_PipeLines.size(); i++) {
		const vector <pair <string, VarType_T> > &output_vars = _PipeLines[i]->GetOutputs();

		for (int j=0; j<output_vars.size(); j++) {
			if (output_vars[j].first.compare(varname) == 0) {
				return(_PipeLines[i]);
			}
		}
	}
	return(NULL);
}

RegularGrid *DataMgrV3_0::execute_pipeline(
	size_t ts,
	string varname,
	int level,
	int lod,
	const size_t min[3],
	const size_t max[3],
	bool	lock,
	float *xcblks,
	float *ycblks,
	float *zcblks
) {

	if (level < 0) level = GetNumTransforms();
	if (lod < 0) lod = GetCRatios().size()-1;

	_VarInfoCache.PurgeRange(ts, varname, level, lod);
	_VarInfoCache.PurgeRegion(ts, varname, level);
	_VarInfoCache.PurgeExist(ts, varname, level, lod);

	PipeLine *pipeline = get_pipeline_for_var(varname);
 
	assert(pipeline != NULL);

	const vector <string> &input_varnames = pipeline->GetInputs();
	const vector <pair <string, VarType_T> > &output_vars = pipeline->GetOutputs();

    VarType_T vtype = DataMgrV3_0::GetVarType(varname);

	//
	// Ptrs to space for input and output variables
	//
	vector <const RegularGrid *> in_grids;
	vector <RegularGrid *> out_grids;

	//
	// Get input variables, and lock them into memory
	//
	for (int i=0; i<input_varnames.size(); i++) {
		size_t min_in[] = {min[0], min[1], min[2]};
		size_t max_in[] = {max[0], max[1], max[2]};
		VarType_T vtype_in = DataMgrV3_0::GetVarType(input_varnames[i]);

		// 
		// If the requested output variable is 2D and an input variable
		// is 3D we need to make sure that the 3rd dimension of the
		// 3D input variable covers the full domain
		//
		if (vtype != VAR3D && vtype_in == VAR3D) {
			size_t dims[3];
			DataMgrV3_0::GetDim(dims, level);

			switch (vtype) {
			case VAR2D_XY:
				min_in[2] = 0;
				max_in[2] = dims[2]-1;
				break;
			case VAR2D_XZ:
				min_in[1] = 0;
				max_in[1] = dims[1]-1;
				break;
			case VAR2D_YZ:
				min_in[0] = 0;
				max_in[0] = dims[0]-1;
				break;
			default:
				break;
			}
		}

		RegularGrid *rg = GetGrid(
						ts, input_varnames[i], level, lod, 
						min_in, max_in, true
		);
		if (! rg) {
			// Unlock any locked variables and abort
			//
			for (int j=0; j<in_grids.size(); j++) UnlockGrid(in_grids[j]);
			return(NULL);
		}
		in_grids.push_back(rg);
	}

	//
	// Get space for all output variables generated by the pipeline,
	// including the single variable that we will return.
	//
	int output_index = -1;
	for (int i=0; i<output_vars.size(); i++) {

		string v = output_vars[i].first;
		VarType_T vtype_out = output_vars[i].second;

		//
		// if output variable i is the one we are interested in record
		// the index and use the lock value passed in to this method
		//
		if (v.compare(varname) == 0) {
			output_index = i;
		}

		float *blks = _alloc_region(
			ts,v.c_str(),vtype_out, level, lod, min,max,true,
			true
		);
		if (! blks) {
			// Unlock any locked variables and abort
			//
			for (int j=0;j<in_grids.size();j++) UnlockGrid(in_grids[j]);
			for (int j=0;j<out_grids.size();j++) UnlockGrid(out_grids[j]);
			return(NULL);
		}
		RegularGrid *rg = _make_grid(
			ts, v, level, lod, min, max, blks, 
			xcblks, ycblks, zcblks
		);
		if (! rg) {
			for (int j=0;j<in_grids.size();j++) UnlockGrid(in_grids[j]);
			for (int j=0;j<out_grids.size();j++) UnlockGrid(out_grids[j]);
			return(NULL);
		}
		out_grids.push_back(rg);
	}
	assert(output_index >= 0);

	int rc = pipeline->Calculate(
		in_grids, out_grids, ts, level, lod 
	);

	//
	// Unlock input variables and output variables that are not 
	// being returned.
	//
	// N.B. unlocking a variable doesn't necessarily free it, but
	// makes the space available if needed later
	//

	//
	// Always unlock/free all input variables
	//
	for (int i=0; i<in_grids.size(); i++) {
		UnlockGrid(in_grids[i]);
		delete in_grids[i];
	}

	//
	// Unlock/free all outputs on error
	//
	if (rc < 0) {
		for (int i=0; i<out_grids.size(); i++) {
			
			UnlockGrid(out_grids[i]);
			delete out_grids[i];
		}
		return(NULL);
	}

	//
	// Unlock/free outputs not being returned
	//
	for (int i=0; i<out_grids.size(); i++) {
		if (i != output_index) {
			UnlockGrid(out_grids[i]);
			delete out_grids[i];
		}
		else if (! lock) UnlockGrid(out_grids[i]);
	}

	return(out_grids[output_index]);
}

bool DataMgrV3_0::cycle_check(
	const map <string, vector <string> > &graph,
	const string &node, 
	const vector <string> &depends
) const {

	if (depends.size() == 0) return(false);

	for (int i=0; i<depends.size(); i++) {
		if (node.compare(depends[i]) == 0) return(true);
	}

	for (int i=0; i<depends.size(); i++) {
		const map <string, vector <string> >::const_iterator itr = 
			graph.find(depends[i]);
		assert(itr != graph.end());

		if (cycle_check(graph, node, itr->second)) return(true);
	}

	return(false);
}

//
// return true iff 'a' depends on 'b' - true if a has inputs that
// match b's outputs.
//
bool DataMgrV3_0::depends_on(
	const PipeLine *a, const PipeLine *b
) const {
	const vector <string> &input_varnames = a->GetInputs();
	const vector <pair <string, VarType_T> > &output_vars = b->GetOutputs();

	for (int i=0; i<input_varnames.size(); i++) {
	for (int j=0; j<output_vars.size(); j++) {
		if (input_varnames[i].compare(output_vars[j].first) == 0) {
			return(true);
		}
	}
	}
	return(false);
}

#endif

//
// return complete list of native variables
//
vector <string> DataMgrV3_0::_get_native_variables() const {
	vector <string> v1 = _dc->GetDataVarNames();
	vector <string> v2 = _dc->GetCoordVarNames();

	v1.insert(v1.end(), v2.begin(), v2.end());
	return(v1);
}

//
// return complete list of derived variables
//
vector <string> DataMgrV3_0::_get_derived_variables() const {

    vector <string> svec;

#ifdef	DEAD
	for (int i=0; i<_PipeLines.size(); i++) {
		const vector <pair <string, VarType_T> > &ovars = _PipeLines[i]->GetOutputs();
		for (int j=0; j<ovars.size(); j++) {
			svec.push_back(ovars[j].first);
		}
	}
#endif
    return(svec);
}

#ifdef	DEAD

DataMgrV3_0::VarType_T DataMgrV3_0::GetVarType(const string &varname) const {
	if (! DataMgrV3_0::IsVariableDerived(varname)) {
		vector <string> vars = GetVariables3D();
		for (int i=0; i<vars.size(); i++ ) {
			if (vars[i].compare(varname) == 0) return(VAR3D);
		}

		vars = GetVariables2DXY();
		for (int i=0; i<vars.size(); i++ ) {
			if (vars[i].compare(varname) == 0) return(VAR2D_XY);
		}

		vars = GetVariables2DXZ();
		for (int i=0; i<vars.size(); i++ ) {
			if (vars[i].compare(varname) == 0) return(VAR2D_XZ);
		}

		vars = GetVariables2DYZ();
		for (int i=0; i<vars.size(); i++ ) {
			if (vars[i].compare(varname) == 0) return(VAR2D_YZ);
		}
		return(VARUNKNOWN);
	}
	for (int i = 0; i< _PipeLines.size(); i++){
		const vector<pair<string, VarType_T> > &ovars = _PipeLines[i]->GetOutputs();
		for (int j = 0; j<ovars.size(); j++){
			if(ovars[j].first == varname){
				return ovars[j].second;
			}
		}
	}
	return VARUNKNOWN;
}


void DataMgrV3_0::PurgeVariable(string varname){
	_free_var(varname);
	_VarInfoCache.PurgeVariable(varname);
}

#endif


string DataMgrV3_0::VarInfoCache::_make_hash(
	string key, size_t ts, vector <string> varnames, int level, int lod
) {
	ostringstream oss;

	oss << ts << ":"; 
	for (int i=0; i<varnames.size(); i++) {
		oss << varnames[i] << ":";
	}
	oss << level << ":";
	oss << lod;

	return(oss.str());
}

void DataMgrV3_0::VarInfoCache::Set(
	size_t ts, vector <string> varnames, int level, int lod, string key,
	const vector <size_t> &values
) {
	string hash = _make_hash(key, ts, varnames, level, lod);
	_cacheSize_t[hash] = values;
}

bool DataMgrV3_0::VarInfoCache::Get(
	size_t ts, vector <string> varnames, int level, int lod, string key,
	vector <size_t> &values
) const {
	values.clear();

	string hash = _make_hash(key, ts, varnames, level, lod);
	map <string, vector <size_t> >::const_iterator itr = _cacheSize_t.find(hash);

	if (itr == _cacheSize_t.end()) return(false);

	values = itr->second;
	return(true);
}
	
void DataMgrV3_0::VarInfoCache::PurgeSize_t(
	size_t ts, vector <string> varnames, int level, int lod, string key
) {
	string hash = _make_hash(key, ts, varnames, level, lod);
	map <string, vector <size_t> >::iterator itr = _cacheSize_t.find(hash);

	if (itr == _cacheSize_t.end()) return;

	_cacheSize_t.erase(itr);
}

void DataMgrV3_0::VarInfoCache::Set(
	size_t ts, vector <string> varnames, int level, int lod, string key,
	const vector <double> &values
) {
	string hash = _make_hash(key, ts, varnames, level, lod);
	_cacheDouble[hash] = values;
}

bool DataMgrV3_0::VarInfoCache::Get(
	size_t ts, vector <string> varnames, int level, int lod, string key,
	vector <double> &values
) const {
	values.clear();

	string hash = _make_hash(key, ts, varnames, level, lod);
	map <string, vector <double> >::const_iterator itr = _cacheDouble.find(hash);

	if (itr == _cacheDouble.end()) return(false);

	values = itr->second;
	return(true);
}
	
void DataMgrV3_0::VarInfoCache::PurgeDouble(
	size_t ts, vector <string> varnames, int level, int lod, string key
) {
	string hash = _make_hash(key, ts, varnames, level, lod);
	map <string, vector <double> >::iterator itr = _cacheDouble.find(hash);

	if (itr == _cacheDouble.end()) return;

	_cacheDouble.erase(itr);
}

DataMgrV3_0::BlkExts::BlkExts() {
	_bmin.clear();
	_bmax.clear();
	_mins.clear();
	_maxs.clear();
}
	

DataMgrV3_0::BlkExts::BlkExts(
	const std::vector <size_t> &bmin, const std::vector <size_t> &bmax
) {
	assert(bmin.size() == bmax.size());
	assert(bmin.size() >= 1 && bmax.size() <= 3);	

	_bmin = bmin;
	_bmax = bmax;

	size_t nelements = linearize(bmin, bmax, bmax) + 1;

	_mins.resize(nelements);
	_maxs.resize(nelements);
}

void DataMgrV3_0::BlkExts::Insert(
    const std::vector <size_t> &bcoord,
    const std::vector <double> &min,
    const std::vector <double> &max
) {

	size_t offset = linearize(_bmin, _bmax, bcoord);

	_mins[offset] = min;
	_maxs[offset] = max;

}

bool DataMgrV3_0::BlkExts::Intersect(
    const std::vector <double> &min,
    const std::vector <double> &max,
    std::vector <size_t> &bmin,
    std::vector <size_t> &bmax
) const {
	assert(_mins.size() >= 1);

	bmin = _bmax;
	bmax = _bmin;

	bool intersection = false;

	// Test for intersection with the axis aligned bounding box of each
	// block. 
	//
	for (size_t offset=0; offset<_mins.size(); offset++) {
		bool overlap = true;
		for (int j=0; j<min.size(); j++) {
			if (_maxs[offset][j] < min[j] || _mins[offset][j] > max[j]) {
				overlap = false;
				continue;
			}
		}

		// If the current block intersects the specified bounding volume
		// compute the block coordinates of the first and last block
		// that intersect the volume
		//
		if (overlap) {
			intersection = true;	// at least one block intersects

			vector <size_t> coord = vectorize(_bmin, _bmax, offset);
			for (int i=0; i<coord.size(); i++) {
				if (coord[i] < bmin[i]) bmin[i] = coord[i]; 
				if (coord[i] > bmax[i]) bmax[i] = coord[i]; 
			}
		}
	}

	return(intersection);
}


#ifdef	DEAD
float *DataMgrV3_0::_get_unblocked_region_from_fs(
	size_t ts, string varname, int level, 
    const vector <size_t> &bs, const vector <size_t> &bmin, 
	const vector <size_t> &bmax, bool lock
) {

	// Get
	vector <size_t> dims;
	vector <size_t> bs;
	int rc = _dc->GetDimLensAtLevel(varnames, -1, dims, bs);
	if (rc<0) return(NULL);

	float *blks = _alloc_region(
		ts, varname, level, lod, bmin, bmax, bs, sizeof(float), lock, false
	);
	if (! blks) return(NULL);

    vector <size_t> min, max;
	map_blk_to_vox(bs, bmin, bmax, min, max);

	int rc = _dc->OpenVariableRead(ts, varname, level, lod);
    if (rc < 0) (return(-1);

	rc = _dc->ReadVariableBlock(min, max, blks);
    if (rc < 0) {
		_free_region(ts,varname ,level,lod,bmin,bmax);
		_dc->CloseVariable(); 
		return(NULL);
	}

	rc = _dc->CloseVariable(); 
	if (rc<0) return(-1);

	SetDiagMsg("DataMgrV3_0::GetGrid() - data read from fs\n");
	return(blks);
}
#endif

int DataMgrV3_0::_level_correction(string varname, int &level) const {
	int nlevels = DataMgrV3_0::GetNumRefLevels(varname);
	if (nlevels < 0) return(-1);

	if (level >= 0) level = -nlevels + level;
	if (level < -nlevels) level = -nlevels;

	return(0);
}

int DataMgrV3_0::_lod_correction(string varname, int &lod) const {
	DC::BaseVar var;
	int rc = GetBaseVarInfo(varname, var);
	if (rc<0) return(rc);

	int nlod = var.GetCRatios().size();

	if (lod >= 0) lod = -nlod + lod;
	if (lod < -nlod) lod = -nlod;

	return(0);
}

//
// Get coordiante variable names for a data variable, return as
// a list of spatial coordinate variables, and a single (if it exists)
// time coordinate variable
//
int DataMgrV3_0::_get_coord_vars(
	string varname, vector <string> &scvars, string &tcvar
) const {
	scvars.clear();
	tcvar.clear();

	DC::DataVar datavar;
	int rc = DataMgrV3_0::GetDataVarInfo(varname, datavar);
	if (rc<0) {
		SetErrMsg("Failed to get metadata for variable %s", varname.c_str());
		return(-1);
	}

	vector <string> cvars;
	cvars = datavar.GetCoordvars();

	for (int i=0; i<cvars.size(); i++) {
		DC::CoordVar cvarinfo;

		if (DataMgrV3_0::GetCoordVarInfo(cvars[i], cvarinfo) < 0) {
			SetErrMsg("Failed to get metadata for variable %s",cvars[i].c_str());
			return(-1);
		}
		
		if (
			cvarinfo.GetDimensions().size() && 
			cvarinfo.GetDimensions()[0].GetAxis() == 3
		)  {
			tcvar = cvars[i];
		}
		else {
			scvars.push_back(cvars[i]);
		}
	}

	return(0);
}

int DataMgrV3_0::_get_time_coordinates(vector <double> &timecoords) {
	timecoords.clear();

	vector <string> allnames = DataMgrV3_0::GetCoordVarNames();

	for (int i=0; i<allnames.size(); i++) {
		DC::CoordVar var;

		// No error checking here!!!!!
		//
		GetCoordVarInfo(allnames[i], var);
		vector <DC::Dimension> dims = var.GetDimensions();
		if (dims.size() == 1 && dims[0].GetAxis() == 3) {

			size_t n = dims[0].GetLength();
			float *buf = new float[n];
			int rc = _dc->GetVar(allnames[i], -1, -1, buf);
			if (rc<0) {
				return(-1);
			}

			for (int j=0; j<n; j++) {
				timecoords.push_back(buf[j]);
			}
			delete [] buf;
		}
	}
	return(0);
}

RegularGrid *DataMgrV3_0::_make_grid_regular(
	const DC::DataVar &var,
    const vector <size_t> &min,
	const vector <size_t> &max, 
	const vector <size_t> &dims,
    const vector <float *> &blkvec,
	const vector <size_t> &bs,
	const vector <size_t> &bmin,
	const vector <size_t> &bmax

) const {
	assert (min.size() == max.size());
	assert (min.size() == dims.size());
	assert (min.size() == min.size());
	assert (min.size() == max.size());
	assert (min.size() == bs.size());
	assert (min.size() == bmin.size());
	assert (min.size() == bmax.size());

	size_t a_min[3] = {0,0,0};
	size_t a_max[3] = {0,0,0};
	size_t a_bs[3] = {0,0,0};
	for (int i=0; i<min.size(); i++) {
		a_min[i] = min[i];
		a_max[i] = max[i];
		a_bs[i] = bs[i];
	}

	double extents[6] = {0.0,0.0,0.0,0.0,0.0,0.0};
	for (int i = 0; i<min.size(); i++) { 
		float *coords = blkvec[i+1];
		int x0 = min[i] % bs[i];
        int x1 = x0 + (max[i]-min[i]);
		extents[i] = coords[x0];
		extents[i+3] = coords[x1];
	}

	bool periodic[3];
	bool has_missing;
	float mv;
	grid_params(var, min, max, dims, periodic, has_missing, mv);

	size_t nblocks = 1;
	size_t block_size = 1;
    for (int i=0; i<bs.size(); i++) {
        nblocks *= bmax[i]-bmin[i]+1;
        block_size *= bs[i];
    }

    float **blkptrs = blkvec[0] ? new float*[nblocks] : NULL;
    for (int i=0; i<nblocks; i++) {
        if (blkptrs) blkptrs[i] = blkvec[0] + i*block_size;
	}

	RegularGrid *rg;
	if (has_missing) {
		rg = new RegularGrid(a_bs,a_min,a_max,extents,periodic,blkptrs,mv);
	}
	else {
		rg = new RegularGrid(a_bs,a_min,a_max,extents,periodic,blkptrs);
	}
	delete [] blkptrs;

	return(rg);
}

LayeredGrid *DataMgrV3_0::_make_grid_layered(
	const DC::DataVar &var,
    const vector <size_t> &min,
	const vector <size_t> &max, 
	const vector <size_t> &dims,
    const vector <float *> &blkvec,
	const vector <size_t> &bs,
	const vector <size_t> &bmin,
	const vector <size_t> &bmax
) const {
	assert (min.size() == max.size());
	assert (min.size() == dims.size());
	assert (min.size() == min.size());
	assert (min.size() == max.size());
	assert (min.size() == bs.size());
	assert (min.size() == bmin.size());
	assert (min.size() == bmax.size());

	size_t a_min[3] = {0,0,0};
	size_t a_max[3] = {0,0,0};
	size_t a_bs[3] = {0,0,0};
	for (int i=0; i<min.size(); i++) {
		a_min[i] = min[i];
		a_max[i] = max[i];
		a_bs[i] = bs[i];
	}

	double extents[6] = {0.0,0.0,0.0,0.0,0.0,0.0};
	for (int i = 0; i<min.size()-1; i++) { 
		float *coords = blkvec[i+1];
		int x0 = min[i] % bs[i];
        int x1 = x0 + (max[i]-min[i]);
		extents[i] = coords[x0];
		extents[i+3] = coords[x1];
	}

	bool periodic[3];
	bool has_missing;
	float mv;
	grid_params(var, min, max, dims, periodic, has_missing, mv);

	size_t nblocks = 1;
	size_t block_size = 1;
    for (int i=0; i<bs.size(); i++) {
        nblocks *= bmax[i]-bmin[i]+1;
        block_size *= bs[i];
    }

    float **blkptrs = blkvec[0] ? new float*[nblocks] : NULL;
    float **zcblkptrs = new float*[nblocks];
    for (int i=0; i<nblocks; i++) {
        if (blkptrs) blkptrs[i] = blkvec[0] + i*block_size;
        zcblkptrs[i] = blkvec[3] + i*block_size;
	}

	LayeredGrid *lg;
	if (has_missing) {
		lg = new LayeredGrid(
			a_bs,a_min,a_max,extents,periodic,blkptrs, zcblkptrs, 2, mv
		);
	}
	else {
		lg = new LayeredGrid(
			a_bs,a_min,a_max,extents,periodic,blkptrs, zcblkptrs, 2
		);
	}
	if (blkptrs) delete [] blkptrs;
	if (zcblkptrs) delete [] zcblkptrs;

	return(lg);
}

//	var: variable info
//  min: min ROI offsets in voxels, full domain 
//  min: max ROI offsets in voxels, full domain 
//	dims: spatial dimensions of full variable domain in voxels
//	blkvec: data blocks, and coordinate blocks
//	bsvec: data block dimensions, and coordinate block dimensions
//  bminvec: ROI offsets in blocks, full domain, data and coordinates
//  bmaxvec: ROI offsets in blocks, full domain, data and coordinates
//

RegularGrid *DataMgrV3_0::_make_grid(
	const DC::DataVar &var,
    const vector <size_t> &min,
	const vector <size_t> &max, 
	const vector <size_t> &dims,
	const vector <float *> &blkvec,
	const vector < vector <size_t > > &bsvec,
	const vector < vector <size_t > > &bminvec,
	const vector < vector <size_t > > &bmaxvec
) const {


	vector <string> cvars;
	string dummy;
	int rc = _get_coord_vars(var.GetName(), cvars, dummy);
	if (rc<0) return(NULL); 

	vector <DC::CoordVar> cvarsinfo;
	vector < vector <size_t> > cdimlens;
	for (int i=0; i<cvars.size(); i++) {
		DC::CoordVar cvarinfo;

		if (GetCoordVarInfo(cvars[i], cvarinfo) < 0) {
			SetErrMsg("Unrecognized variable name : %s", cvars[i].c_str());
			return(NULL);
		}

		vector <size_t> cdims;
		size_t dummy;
		DC::ParseDimensions(cvarinfo.GetDimensions(), cdims, dummy);

		cdimlens.push_back(cdims);

		cvarsinfo.push_back(cvarinfo);
	}
	assert(cdimlens.size() == cvarsinfo.size());


    //
    // First check for RegularGrid
    //
    bool regular_grid = true;
    for (int i=0; i<cdimlens.size(); i++) {
        if (cdimlens[i].size() != 1 || ! cvarsinfo[i].GetUniform()) {
            regular_grid = false;
        }
    }
	bool layered_grid = false;
	if (! regular_grid) {
		if (
			cdimlens.size()==3 && 
			cvarsinfo[0].GetUniform() && cvarsinfo[1].GetUniform() && 
			cdimlens[2].size() == 3
		) {
			layered_grid = true;
		}
	}
		

	RegularGrid *rg = NULL;
    if (regular_grid) {
		rg = _make_grid_regular(
			var, min, max, dims, blkvec, bsvec[0], bminvec[0], bmaxvec[0]
		);
	}
	else if (layered_grid) {
		rg = _make_grid_layered(
			var, min, max, dims, blkvec, bsvec[0], bminvec[0], bmaxvec[0]
		);
	}

	return(rg);
}

// Find the grid coordinates, in voxels, for the region containing 
// the axis aligned bounding box specified by min and max
//
int DataMgrV3_0::_find_bounding_grid(
	size_t ts, string varname, int level, int lod, 
	vector <double> min, vector <double> max, 
	vector <size_t> &min_ui, vector <size_t> &max_ui
) {
	min_ui.clear();
	max_ui.clear();

	vector <string> scvars; 
	string tcvar;

	int rc = _get_coord_vars(varname, scvars, tcvar);
	if (rc<0) return(-1);

	size_t hash_ts = 0;
	for (int i=0; i<scvars.size(); i++) {
		if (IsTimeVarying(scvars[i])) hash_ts = ts;
	}

	vector <size_t> dims_at_level;
	vector <size_t> bs_at_level;
	rc = _dc->GetDimLensAtLevel(varname, level, dims_at_level, bs_at_level);
	if (rc < 0) {
		SetErrMsg("Invalid variable reference : %s", varname.c_str());
		return(-1);
	}

	// Remove time dimension if time-varying
	//
	if (DataMgrV3_0::IsTimeVarying(varname)) {
		dims_at_level.pop_back();
		bs_at_level.pop_back();
	}

	// hash tag for block coordinate cache
	//
	string hash = VarInfoCache::_make_hash(
		"BlkExts", hash_ts, scvars, level, lod
	);

	// See if bounding volumes for individual blocks are already 
	// cached for this grid
	//
	map <string, BlkExts >::iterator itr = _blkExtsCache.find(hash);

	if (itr == _blkExtsCache.end()) {
		SetDiagMsg(
			"DataMgrV3_0::_find_bounding_grid() - coordinates not in cache"
		);

		// Get a "dataless" RegularGrid - a RegularGrid class the contains
		// coordiante information, but not data
		//
		RegularGrid *rg = _getVariable(ts, varname, level, lod, false, true);
		if (! rg) return(-1);

		// Voxel and block min and max coordinates of entire grid
		//
		vector <size_t> vmin, vmax;
		vector <size_t> bmin, bmax;

		for (int i=0; i<dims_at_level.size(); i++) {
			vmin.push_back(0);
			vmax.push_back(dims_at_level[i]-1);
		}
		map_vox_to_blk(bs_at_level, vmin, bmin);
		map_vox_to_blk(bs_at_level, vmax, bmax);

		BlkExts blkexts(bmin, bmax);

		// For each block in the grid compute the block's bounding 
		// box. Include a one-voxel halo region on all non-boundary
		// faces
		//
		size_t nblocks = linearize(bmin, bmax, bmax) + 1;
		for (size_t offset = 0; offset<nblocks; offset++) {
			vector <double> my_min, my_max;

			// Get coordinates for current block
			//
			vector <size_t> bcoord = vectorize(bmin, bmax, offset);

			for (int i=0; i<bcoord.size(); i++) {
				vmin[i] = bcoord[i] * bs_at_level[i];
				if (vmin[i] > 0) vmin[i] -= 1;	// not boundary face

				vmax[i] = bcoord[i] * bs_at_level[i] + bs_at_level[i] - 1;
				if (vmax[i] < dims_at_level[i]-1) vmax[i] += 1;
			}

			// Use the regular grid class to compute the user-coordinate
			// axis aligned bounding volume for the block+halo
			//
			rg->GetBoundingBox(vmin, vmax, my_min, my_max);

			// Insert the bounding volume into blkexts
			//
			blkexts.Insert(bcoord, my_min, my_max);

		} 

		// Add to the hash table
		//
		_blkExtsCache[hash] = blkexts;
		itr = _blkExtsCache.find(hash);
		assert (itr != _blkExtsCache.end());

	}
	else {
		SetDiagMsg(
			"DataMgrV3_0::_find_bounding_grid() - coordinates in cache"
		);
	}
	

	const BlkExts &blkexts = itr->second;

	// Find block coordinates of region that contains the bounding volume
	//
	vector <size_t> bmin, bmax;
	blkexts.Intersect(min, max, bmin, bmax);

	// Finally, map from block to voxel coordinates
	//
	map_blk_to_vox(bs_at_level, bmin, bmax, min_ui, max_ui);

	return(0);
}

void	DataMgrV3_0::_unlock_blocks(
	const float *blks
) {

	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); itr++) {
		region_t &region = *itr;

		if (region.blks == blks && region.lock_counter>0) {
			region.lock_counter--;
			return;
		}
	}
	return;
}
