#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cfloat>
#include <vector>
#include <map>
#include <vapor/DataMgrV3_0.h>
#ifdef WIN32
#include <float.h>
#endif
using namespace VetsUtil;
using namespace VAPoR;

namespace {


void map_vox_to_blk(
	vector <size_t> bs, const vector <size_t> &vcoord, vector <size_t> &bcoord
) {
	assert(bs.size() == bcoord.size());
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

void grid_params(
	const VDC::BaseVar &var,
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


	const VDC::DataVar *dvarptr = dynamic_cast<const VDC::DataVar *> (&var);
	if (dvarptr->GetHasMissing()) {
		has_missing = true;
		missing = dvarptr->GetMissingValue();
	}
}

vector <int> get_spatial_axis(const VDC::BaseVar &var) {
	vector <int> axis;

	vector <VDC::Dimension> dims = var.GetDimensions();
	for (int i=0; i<dims.size(); i++) {
		if (dims[i].GetAxis() != 3) axis.push_back(dims[i].GetAxis());
	}
	return(axis);
}

#ifdef	dead
void intersect3d(
	const float min[3], const float max[3], 
	blkexts, bmin, bmax 
) {
	for (k=0; k<bdims[2]; k++) { 
	for (j=0; j<bdims[1]; j++) { 
	for (i=0; i<bdims[0]; i++) { 
		
	}
	}
	}
}
#endif

};

int	DataMgrV3_0::_DataMgrV3_0(
	size_t mem_size
) {
	SetClassName("DataMgrV3_0");

	_blk_mem_mgr = NULL;

	_PipeLines.clear();

	_regionsList.clear();
	
	_mem_size = mem_size;

	_varInfoCache.Clear();

	return(0);
}

int DataMgrV3_0::Initialize(const vector <string> &files) {
	int rc = _GetTimeCoordinates(_timeCoordinates);
	if (rc<0) {
		SetErrMsg("Failed to get time coordinates");
		return(-1);
	}
	return(_Initialize(files));
}


int DataMgrV3_0::_get_coord_vars(string varname, vector <string> cvars) const {
	cvars.clear();

	VAPoR::VDC::DataVar datavar;
	int rc = DataMgrV3_0::GetDataVarInfo(varname, datavar);
	if (rc<0) {
		SetErrMsg("Failed to get metadata for variable %s", varname.c_str());
		return(-1);
	}
	cvars = datavar.GetCoordvars();

	return(0);
}

vector <string> DataMgrV3_0::GetDataVarNames() const {
	return(_GetDataVarNames());
}

vector <string> DataMgrV3_0::GetDataVarNames(int ndim, bool spatial) const {
	vector <string> allnames = DataMgrV3_0::GetDataVarNames();

	vector <string> names;
	for (int i=0; i<allnames.size(); i++) {
		VDC::DataVar var;

		// No error checking here!!!!!
		//
		DataMgrV3_0::GetDataVarInfo(allnames[i], var);
		vector <VDC::Dimension> dims = var.GetDimensions();
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
	return(_GetCoordVarNames());
}



vector <string> DataMgrV3_0::GetCoordVarNames(int ndim, bool spatial) const {
	vector <string> allnames = DataMgrV3_0::GetCoordVarNames();

	vector <string> names;
	for (int i=0; i<allnames.size(); i++) {
		VDC::CoordVar var;

		// No error checking here!!!!!
		//
		GetCoordVarInfo(allnames[i], var);
		vector <VDC::Dimension> dims = var.GetDimensions();
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

int DataMgrV3_0::_GetTimeCoordinates(vector <double> &timecoords) {
	timecoords.clear();

	vector <string> allnames = DataMgrV3_0::GetCoordVarNames();

	for (int i=0; i<allnames.size(); i++) {
		VDC::CoordVar var;

		// No error checking here!!!!!
		//
		GetCoordVarInfo(allnames[i], var);
		vector <VDC::Dimension> dims = var.GetDimensions();
		if (dims.size() == 1 && dims[i].GetAxis() == 3) {

			size_t n = dims[i].GetLength();
			float *buf = new float[n];
			int rc = _ReadVariable(allnames[i], -1, -1, buf);
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

bool DataMgrV3_0::GetDataVarInfo(
	string varname, VAPoR::VDC::DataVar &var
) const {
	return(_GetDataVarInfo(varname, var));
}

bool DataMgrV3_0::GetCoordVarInfo(
	string varname, VAPoR::VDC::CoordVar &var
) const {
	return(_GetCoordVarInfo(varname, var));
}

bool DataMgrV3_0::GetBaseVarInfo(
	string varname, VAPoR::VDC::BaseVar &var
) const {
	return(_GetBaseVarInfo(varname, var));
}

bool DataMgrV3_0::IsTimeVarying(string varname) const {
		VDC::BaseVar var;

		// No error checking here!!!!!
		//
		GetBaseVarInfo(varname, var);
		vector <VDC::Dimension> dims = var.GetDimensions();

		for (int i=0; i<dims.size(); i++) {
			if (dims[i].GetAxis() == 3) return (true);
		}
		return(false);
}

bool DataMgrV3_0::IsCompressed(string varname) const {
	VDC::BaseVar var;

	// No error checking here!!!!!
	//
	GetBaseVarInfo(varname, var);

	return(var.GetCompressed());
}

int DataMgrV3_0::GetNumTimeSteps(string varname) const {
	VDC::BaseVar var;
	int rc = GetBaseVarInfo(varname, var);
	if (rc<0) return(rc);

	vector <VDC::Dimension> dims = var.GetDimensions();

	for (int i=0; i<dims.size(); i++) {
		if (dims[i].GetAxis() == 3) return (dims[i].GetLength());
	}
	return(0);	// not time varying
}

int DataMgrV3_0::GetNumRefLevels(string varname) const {
	return(_GetNumRefLevels(varname));
}

int DataMgrV3_0::GetCRatios(string varname, vector <size_t> &cratios) const {
	cratios.clear();

	VDC::BaseVar var;
	int rc = GetBaseVarInfo(varname, var);
	if (rc<0) return(rc);

	cratios = var.GetCRatios();
	return(0);
}


#ifdef	DEAD
int DataMgrV3_0::_map_user_to_block_coords(
	size_t ts, string varname, int level, int lod, 
	vector <float> min, vector <float> max, 
	vector <size_t> &bmin, vector <size_t> &bmax
) {

	size_t hash_ts = 0;
	for (int i=0; i<cvars.size(); i++) {
		if (IsTimeVarying(cvars[i])) hash_ts = ts;
	}
	string hash = _make_hash(hash_ts, cvars, level, lod);

	if (! defined hash) {
	}
	const DataMgrV3_0::blkexts &blkexts = ExtentsTable[hash];

	_intersect(min, max, blkexts, bmin, bmax)

}
#endif
	


DataMgrV3_0::DataMgrV3_0(
	size_t mem_size
) {

	SetDiagMsg("DataMgrV3_0::DataMgrV3_0(,%d)", mem_size);

	if (_DataMgrV3_0(mem_size) < 0) return;
}


DataMgrV3_0::~DataMgrV3_0(
) {
	SetDiagMsg("DataMgrV3_0::~DataMgrV3_0()");

	Clear();
	if (_blk_mem_mgr) delete _blk_mem_mgr;

	_blk_mem_mgr = NULL;

}

RegularGrid *DataMgrV3_0::_make_grid_regular(
	const VDC::DataVar &var,
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

//	var: variable info
//  min: min ROI offsets in voxels, full domain 
//  min: max ROI offsets in voxels, full domain 
//	dims: dimensions of full variable domain in voxels
//	blkvec: data blocks, and coordinate blocks
//	bsvec: data block dimensions, and coordinate block dimensions
//  bminvec: ROI offsets in blocks, full domain, data and coordinates
//  bmaxvec: ROI offsets in blocks, full domain, data and coordinates
//

RegularGrid *DataMgrV3_0::_make_grid(
	const VDC::DataVar &var,
    const vector <size_t> &min,
	const vector <size_t> &max, 
	const vector <size_t> &dims,
	const vector <float *> &blkvec,
	const vector < vector <size_t > > &bsvec,
	const vector < vector <size_t > > &bminvec,
	const vector < vector <size_t > > &bmaxvec
) const {

	vector <string> cvars = var.GetCoordvars();
	vector <VDC::CoordVar> cvarsinfo;

	for (int i=0; i<cvars.size(); i++) {
		VDC::CoordVar cvarinfo;

		if (GetCoordVarInfo(cvars[i], cvarinfo) < 0) {
			SetErrMsg("Unrecognized variable name : %s", cvars[i].c_str());
			return(NULL);
		}
		cvarsinfo.push_back(cvarinfo);
	}


    vector < vector <int> > coord_axis;
    vector <bool> uniform;
    for (int i=0; i<cvars.size(); i++) {
        coord_axis.push_back(get_spatial_axis(cvarsinfo[i]));
        uniform.push_back(cvarsinfo[i].GetUniform());
    }


    //
    // First check for RegularGrid
    //
    bool regular_grid = true;
    for (int i=0; i<coord_axis.size(); i++) {
        if (coord_axis[i].size() != 1 || ! uniform[i]) {
            regular_grid = false;
        }
    }

	RegularGrid *rg = NULL;
    if (regular_grid) {
		rg = _make_grid_regular(
			var, min, max, dims, blkvec, bsvec[0], bminvec[0], bmaxvec[0]
		);
	}

	return(rg);
}

RegularGrid *DataMgrV3_0::GetVariable (
	size_t ts, string varname, int level, int lod, bool lock
) {

	SetDiagMsg(
		"DataMgrV3_0::GetVariable(%d,%s,%d,%d,%d)",
		ts,varname.c_str(),level,lod, lock
	);
	return(_getVariable(ts, varname, level, lod, lock, false));
}

#ifdef	DEAD
RegularGrid *DataMgrV3_0::GetVariable (
	size_t ts, string varname, int level, int lod,
    vector <double> min, vector <double> max, bool lock
) {

	vector <string> cvars;
	int rc = _get_coord_vars(varname, cvars);
	if (rc<0) return(NULL)

	_map_user_to_block_coords(
		ts, varname, level, lod, min, max, bmin, bmax
	);
}
#endif

RegularGrid *DataMgrV3_0::_getVariable(
	size_t ts,
	string varname,
	int level,
	int lod,
	bool	lock,
	bool	dataless
) {

	vector <size_t> dims_at_level;
	vector <size_t> bs_at_level;
	int rc = _GetDimLensAtLevel(
		varname, level, dims_at_level, bs_at_level
	);
	if (rc < 0) {
		SetErrMsg("Invalid variable reference : %s", varname.c_str());
		return(NULL);
	}

	vector <size_t> min;
	vector <size_t> max;
	for (int i=0; i<dims_at_level.size(); i++) {
		min.push_back(0);
		max.push_back(dims_at_level[i]);
	}

	return( DataMgrV3_0::_getVariable(
		ts, varname, level, lod, min, max, lock, dataless
	));
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

	VDC::DataVar var;
	if (DataMgrV3_0::GetDataVarInfo(varname, var) < 0) {
		SetErrMsg("Unrecognized variable name : %s", varname.c_str());
		return(NULL);
	}
	vector <string> cvars = var.GetCoordvars();
	
	vector <string> varnames;
	varnames.push_back(varname);
	vector <VDC::BaseVar> varinfo;
	varinfo.push_back(var);

	for (int i=0; i<cvars.size(); i++) {
		varnames.push_back(cvars[i]);

		VDC::CoordVar cvar;
		if (DataMgrV3_0::GetCoordVarInfo(cvars[i], cvar) < 0) {
			SetErrMsg("Unrecognized variable name : %s", cvars[i].c_str());
			return(NULL);
		}
		varinfo.push_back(cvar);
	}

	vector <float *> blkvec;
	vector < vector <size_t > > dimsvec;
	vector < vector <size_t > > bsvec;
	vector < vector <size_t > > bminvec;
	vector < vector <size_t > > bmaxvec;

	for (int i=0; i<varnames.size(); i++) {

		vector <size_t> dims_at_level;
		vector <size_t> bs_at_level;
		int rc = _GetDimLensAtLevel(
			varnames[i], level, dims_at_level, bs_at_level
		);
		if (rc < 0) {
			SetErrMsg("Invalid variable reference : %s", varnames[i].c_str());
			return(NULL);
		}

		if (min.size() != bs_at_level.size() || max.size() != bs_at_level.size()) {
			SetErrMsg("Invalid variable reference : %s", varnames[i].c_str());
			return(NULL);
		}

		vector <int> caxis = get_spatial_axis(varinfo[i]);

		vector <size_t> permuted_dims;
		vector <size_t> permuted_bs;
		vector <size_t> permuted_min;
		vector <size_t> permuted_max;
		for (int j=0; j<caxis.size(); j++) {
			permuted_dims.push_back(dims_at_level[caxis[j]]);
			permuted_bs.push_back(bs_at_level[caxis[j]]);
			permuted_min.push_back(min[caxis[j]]);
			permuted_max.push_back(max[caxis[j]]);
		}

		// Map voxel coordinates into block coordinates
		//
		vector <size_t> bmin, bmax;
		map_vox_to_blk(permuted_bs, permuted_min, bmin);
		map_vox_to_blk(permuted_bs, permuted_min, bmax);

		dimsvec.push_back(permuted_dims);
		bsvec.push_back(permuted_bs);
		bminvec.push_back(bmin);
		bmaxvec.push_back(bmax);
	}

	//
	// if dataless we only load coordinate data
	//
	if (dataless) varnames[0].clear();
	int rc = DataMgrV3_0::_get_regions(
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
		assert(blkvec[0] != NULL);
		rg = _make_grid(
			var, min, max, dimsvec[0], blkvec, bsvec, bminvec, bmaxvec
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
		"DataMgrV3_0::GetVariable(%d,%s,%d,%d,...)",
		ts,varname.c_str(),level,lod
	);
	return(_getVariable(ts, varname, level, lod, min, max, lock, false));

}

int DataMgrV3_0::GetVariableExtents(
    size_t ts, string varname, int level,
    vector <double> &min , vector <double> &max
) {
	min.clear();
	max.clear();

	vector <string> cvars;
	int rc = _get_coord_vars(varname, cvars);
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
	VAPoR::VDC::DataVar datavar;
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
		bool exists = _VariableExists(ts, varname, level, lod);
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

#ifdef	DEAD
int DataMgrV3_0::GetDataRange(
	size_t ts,
	const char *varname,
	float *range,
	int level,
	int lod
) {
	if (level < 0) level = DataMgrV3_0::GetNumTransforms();
	if (lod < 0) lod = DataMgrV3_0::GetCRatios().size()-1;

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
SetDiagMsg(
	"DataMgrV3_0::_unlock_blocks(%xll) - Failed to free blocks\n", blks
);
	return;
}

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
				"DataMgrV3_0::GetGrid() - data in cache %xll\n", tmp_region.blks
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

	int rc = _ReadVariableBlock(ts, varname, level, lod, min, max, blks);
    if (rc < 0) {
		_free_region(ts,varname ,level,lod,bmin,bmax);
		return(NULL);
	}

	SetDiagMsg("DataMgrV3_0::GetGrid() - data read from fs\n");
	return(blks);
}

float *DataMgrV3_0::_get_region(
	size_t ts,
	string varname,
	int level,
	int lod, 
	const vector <size_t> &bs,
	const vector <size_t> &bmin,
	const vector <size_t> &bmax,
	bool lock
) {
	// See if region is already in cache. If not, read from the 
	// file system.
	//
	float *blks = _get_region_from_cache(
		ts, varname, level, lod, bmin, bmax, lock
	);
	if (! blks && ! DataMgrV3_0::IsVariableDerived(varname)) {
		blks = (float *) _get_region_from_fs(
			ts, varname, level, lod, bs, bmin, bmax, lock
		);
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
	const vector < vector <size_t > > &bsvec,
	const vector < vector <size_t> > &bminvec,
	const vector < vector <size_t> > &bmaxvec,
	vector <float *> &blkvec
) {

	blkvec.clear();

	for (int i=0; i<varnames.size(); i++) {
		float *blks = NULL;

		if (! varnames[i].empty()) {
			blks = _get_region(
				ts, varnames[i], level, lod, bsvec[i], 
				bminvec[i], bmaxvec[i], true
			);
			if (! blks) {
				for (int i=0; i<blkvec.size(); i++) {
					if (blkvec[i]) _unlock_blocks(blkvec[i]);
				}
				return(-1);
			}
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


void	DataMgrV3_0::Clear() {

	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); itr++) {
		const region_t &region = *itr;

		if (region.blks) _blk_mem_mgr->FreeMem(region.blks);
			
	}
	_regionsList.clear();
	_varInfoCache.Clear();
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
	vector <string> v1 = _GetDataVarNames();
	vector <string> v2 = _GetCoordVarNames();

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
) const {
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

#ifdef	DEAD
DataMgr::blkexts::blkexts(
	vector <size_t> bs, vector <size_t> bdims, 
	const float *blks
) {
	assert(bs.size() == bdims.size());
	_mins.clear();
	_maxs.clear();
	_bdims = bdims;

	size_t blksize = 1;
	for (int i=0; i<bs.size()) *= bs[i];

	size_t gridsize = 1;
	for (int i=0; i<bdims.size()) *= bdims[i];

	const float *blkptr = blks;
	for (int i=0; i<gridsize; i++) {

		float min = *blkptr;
		float max = *blkptr;

		// Get min and max for entire block. This is wastefull - only
		// need to check the boundary - but simple
		//
		for (int j=0; j<blksize; j++) {
			if (blkptr[j] < min) min = blkptr[j];
			if (blkptr[j] > max) max = blkptr[j];
		}
		_mins.push_back(min);
		_maxs.push_back(max);
		blkptr += blksize
	} 
}

void DataMgr::blkexts::getexts(
	vector <size_t> coords, float &min, float &max
) const {
	min = 0.0;
	max = 0.0;
	assert(coords.size() == _bdims.size());

    size_t offset = 0;

	for (int i=0; i<coords.size(); i++) {
		assert(coords[i] < _bdims[i]);
	}

	size_t factor = 1;
	for (int i=coords.size()-1; i>=0; i--) {
		offset += factor * coords[i];
		factor *= _bdims[i];
	}
	assert(offset < _mins.size());

	min = _mins[offset];
	max = _maxs[offset];
}
#endif
