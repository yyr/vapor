#include <iostream>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cfloat>
#include <vector>
#include <map>
#include <vapor/DataMgr.h>
#include <vapor/common.h>
#include <vapor/errorcodes.h>
#ifdef WIN32
#include <float.h>
#endif
using namespace VetsUtil;
using namespace VAPoR;


int	DataMgr::_DataMgr(
	size_t mem_size
) {
	SetClassName("DataMgr");

	_blk_mem_mgr = NULL;

	_PipeLines.clear();

	_regionsList.clear();
	
	_mem_size = mem_size;

	return(0);
}

DataMgr::DataMgr(
	size_t mem_size
) {

	SetDiagMsg("DataMgr::DataMgr(,%d)", mem_size);

	if (_DataMgr(mem_size) < 0) return;
}


DataMgr::~DataMgr(
) {
	SetDiagMsg("DataMgr::~DataMgr()");

	Clear();
	if (_blk_mem_mgr) delete _blk_mem_mgr;

	_blk_mem_mgr = NULL;

}

RegularGrid *DataMgr::make_grid(
	size_t ts, string varname, int reflevel, int lod,
    const size_t min[3], const size_t max[3], 
	float *blocks, float *xcblks, float *ycblks, float *zcblks
) {

    int  ldelta = DataMgr::GetNumTransforms() - reflevel;

	size_t bs[3];
	_GetBlockSize(bs,reflevel);

	size_t bdim[3];
	get_dim_blk(bdim,reflevel);

	size_t dim[3];
	DataMgr::GetDim(dim,reflevel);

	size_t bmin[3], bmax[3];
	map_vox_to_blk(min, bmin, reflevel);
	map_vox_to_blk(max, bmax, reflevel);

    //
    // Make sure 2D variables have valid 3rd dimensions
    //
    VarType_T vtype = DataMgr::GetVarType(varname);
	if (vtype == VARUNKNOWN && varname.size() == 0) vtype = VAR3D;
    switch (vtype) {
    case VAR2D_XY:
        dim[2] = bdim[2] = 1;
		bs[2] = 1;
        break;
    case VAR2D_XZ:
        dim[2] = bdim[2] = 1;
		bs[1] = bs[2];
		bs[2] = 1;
        break;
    case VAR2D_YZ:
        dim[2] = bdim[2] = 1;
		bs[0] = bs[1];
		bs[1] = bs[2];
		bs[2] = 1;
        break;
    default:
        break;
    }

	int nblocks = 1;
	size_t block_size = 1;
	float **blkptrs = NULL;
	float **xcblkptrs = NULL;
	float **ycblkptrs = NULL;
	float **zcblkptrs = NULL;

	for (int i=0; i<3; i++) {
		nblocks *= bmax[i]-bmin[i]+1;
		block_size *= bs[i];
	}

	blkptrs = blocks ? new float*[nblocks] : NULL;
	if (DataMgr::GetGridType().compare("layered")==0) {
		zcblkptrs = new float*[nblocks];
	}
	for (int i=0; i<nblocks; i++) {
		if (blkptrs) blkptrs[i] = blocks + i*block_size;

		if (DataMgr::GetGridType().compare("layered")==0) {
			zcblkptrs[i] = zcblks + i*block_size;
		}
	}

	//
	// Get extents of grid. Only valid for coordinates with
	// uniform sampling. That's ok. The non-uniform coordinate dimensions
	// specified by `extents' are ignored by RegularGrid class and 
	// its derivatives
	//
	double extents[6];
	map_vox_to_user_regular(ts,min, extents, reflevel);
	map_vox_to_user_regular(ts,max, extents+3, reflevel);

	//
	// Determine which dimensions are periodic, if any. For a dimension to
	// be periodic the data set must be periodic, and the requested
	// blocks must be boundary blocks
	//
	const vector <long> &periodic_vec = DataMgr::GetPeriodicBoundary();

	bool periodic[3];
	for (int i=0; i<3; i++) {
		if (periodic_vec[i] && min[i]==0 && max[i]==dim[i]-1) {
			periodic[i] = true;
		}
		else {
			periodic[i] = false;
		}
	}

	RegularGrid *rg = NULL;
	float mv;
	bool has_missing = varname.empty() ? false : _GetMissingValue(ts, varname, mv);
	if (DataMgr::GetCoordSystemType().compare("spherical")==0) { 
		vector <long> permv = GetGridPermutation();
		size_t perm[] = {permv[0], permv[1], permv[2]};

		if (has_missing) {
			rg = new SphericalGrid(bs,min,max,extents,perm,periodic,blkptrs,mv);
		}
		else {
			rg = new SphericalGrid(bs,min,max,extents,perm,periodic,blkptrs);
		}

	}
	else if (DataMgr::GetGridType().compare("regular")==0) {
		if (has_missing) {
			rg = new RegularGrid(bs,min,max,extents,periodic,blkptrs,mv);
		}
		else {
			rg = new RegularGrid(bs,min,max,extents,periodic,blkptrs);
		}
	}
	else if (DataMgr::GetGridType().compare("stretched")==0) {
		vector <double> xcoords, ycoords, zcoords;

		coord_array(_GetTSXCoords(ts), xcoords, ldelta);
		coord_array(_GetTSYCoords(ts), ycoords, ldelta);
		coord_array(_GetTSZCoords(ts), zcoords, ldelta);

		if (min[0]>0 && xcoords.size()) 
			xcoords.erase(xcoords.begin(), xcoords.begin() + min[0]);

		if (min[1]>0 && ycoords.size())
			ycoords.erase(ycoords.begin(), ycoords.begin() + min[1]);

		if (min[2]>0 && zcoords.size())
			zcoords.erase(zcoords.begin(), zcoords.begin() + min[2]);

		if (has_missing) {
			rg = new StretchedGrid(
				bs,min,max,extents,periodic,blkptrs,
				xcoords, ycoords, zcoords, mv
			);
		}
		else {
			rg = new StretchedGrid(
				bs,min,max,extents,periodic,blkptrs,
				xcoords, ycoords, zcoords
			);
		}

	} else if ((DataMgr::GetGridType().compare("layered")==0) && vtype !=VAR3D){

		if (has_missing) {
			rg = new RegularGrid(bs,min,max,extents,periodic,blkptrs, mv);
		} 
		else {
			rg = new RegularGrid(bs,min,max,extents,periodic,blkptrs);
		}
	
	} else if ((DataMgr::GetGridType().compare("layered")==0) && vtype ==VAR3D){

		if (has_missing) {
			rg = new LayeredGrid(
				bs,min, max, extents, periodic, blkptrs, zcblkptrs,2, mv
			);
		}
		else {
			rg = new LayeredGrid(
				bs,min, max, extents, periodic, blkptrs, zcblkptrs,2
			);
		}
	}
	if (blkptrs) delete [] blkptrs;
	if (xcblkptrs) delete [] xcblkptrs;
	if (ycblkptrs) delete [] ycblkptrs;
	if (zcblkptrs) delete [] zcblkptrs;

	return(rg);
}

RegularGrid *DataMgr::GetGrid(
	size_t ts,
	string varname,
	int reflevel,
	int lod,
	const size_t min[3],
	const size_t max[3],
	bool	lock
) {
	RegularGrid *rg = NULL;
	bool ondisk = false;

	SetDiagMsg(
		"DataMgr::GetGrid(%d,%s,%d,%d,[%d,%d,%d],[%d,%d,%d],%d)",
		ts,varname.c_str(),reflevel,lod,min[0],min[1],min[2],
		max[0],max[1],max[2], lock
	);

	//
	// Make sure 2D variables have valid 3rd dimensions
	//
	size_t mymin[3] = {min[0], min[1], min[2]};
	size_t mymax[3] = {max[0], max[1], max[2]};
	VarType_T vtype = DataMgr::GetVarType(varname);
	if (vtype == VARUNKNOWN && varname.size() == 0) vtype = VAR3D; 
	switch (vtype) {
	case VAR2D_XY:
	case VAR2D_XZ:
	case VAR2D_YZ:
		mymin[2] = mymax[2] = 0;
		break;
	default:
		break;
	}

	//
	// This code won't work for vtype = (VAR2DXZ or VAR2DYZ)
	//
	size_t bmin[3], bmax[3];
	map_vox_to_blk(mymin, bmin, reflevel);
	map_vox_to_blk(mymax, bmax, reflevel);

	//
	// min_aligned and max_aligned are versions of mymin and mymax
	// that are block-aligned to improve cache hit rate
	//
	size_t bs[3], min_aligned[3], max_aligned[3];
	_GetBlockSize(bs, reflevel);
	for (int i=0; i<3; i++) {
		min_aligned[i] = bmin[i] * bs[i];
		max_aligned[i] = bmax[i] * bs[i] + bs[i]-1;
	}

	float *blks = NULL;		// variable blocks
	float *xcblks = NULL;	// X,Y,Z coordinate blocks
	float *ycblks = NULL;
	float *zcblks = NULL;

	if (DataMgr::IsVariableDerived(varname)) {
		//
		// See if data is already in cache
		//
		blks = get_region_from_cache(
			ts, varname, reflevel, lod, mymin, mymax, true
		);
	}
	else {

		//
		// Get data from cache or disk
		//
		blks = get_region(
			ts, varname, reflevel, lod, min_aligned, max_aligned, true, &ondisk
		);
		if (! blks && varname.size() != 0) return (NULL);
	}

	//
	// For layered data we need the blocks for the elevation grid
	//
	if ((DataMgr::GetGridType().compare("layered")==0) && vtype == VAR3D) {
		bool dummy;
		zcblks = get_region(
			ts, "ELEVATION", reflevel, lod, min_aligned, max_aligned,
			true, &dummy
		);
		if (! zcblks) {
			if (blks) unlock_blocks(blks);
			return(NULL);
		}
	}


	if (DataMgr::IsVariableDerived(varname) && ! blks) {
		//
		// Derived variable that is not in cache, so we need to 
		// create it
		//
		rg = execute_pipeline(
				ts, varname, reflevel, lod, mymin, mymax, lock,
				xcblks, ycblks, zcblks
			);
	}
	else {
		rg = make_grid(
			ts, varname, reflevel, lod, mymin, mymax,
			blks, xcblks, ycblks, zcblks
		);
	}

	if (!rg) {
		if (blks) unlock_blocks(blks);
		if (xcblks) unlock_blocks(xcblks);
		if (ycblks) unlock_blocks(ycblks);
		if (zcblks) unlock_blocks(zcblks);
		return (NULL);
	}

	// 
	// Safe to remove locks now that were not explicitly requested
	//
	if (! lock) {
		if (blks) unlock_blocks(blks);
		if (xcblks) unlock_blocks(xcblks);
		if (ycblks) unlock_blocks(ycblks);
		if (zcblks) unlock_blocks(zcblks);
	}

	if (ondisk) {
		//
		// Make sure we have a valid floating point value
		//
		RegularGrid::Iterator itr;
		float mv;
		if (rg->HasMissingData()) mv = rg->GetMissingValue();
		else mv = 0.0;

		for (itr = rg->begin(); itr!=rg->end(); ++itr) {
#ifdef WIN32
			if ((! _finite(*itr) || _isnan(*itr)) && *itr!=mv) *itr= FLT_MAX;
#else
			if ((! finite(*itr) || isnan(*itr)) && *itr!=mv) *itr= FLT_MAX;
#endif
		}
	}

	return(rg);
}

int	DataMgr::NewPipeline(PipeLine *pipeline) {

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
	vector <string> native_vars = get_native_variables();

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

void	DataMgr::RemovePipeline(string name) {

	vector <PipeLine *>::iterator itr;
	for (itr = _PipeLines.begin(); itr != _PipeLines.end(); itr++) {
		if (name.compare((*itr)->GetName()) == 0) {
			_PipeLines.erase(itr);
			break;
		}
	}
}

int DataMgr::VariableExists(
    size_t ts, const char *varname, int reflevel, int lod
) {
	if (reflevel < 0) reflevel = DataMgr::GetNumTransforms();
	if (lod < 0) lod = DataMgr::GetCRatios().size()-1;

	//
	// If layered data than the variable "ELEVATION" must also exist
	//
	string elevation = "ELEVATION";
	if ((DataMgr::GetGridType().compare("layered") == 0) && (elevation.compare(varname) !=0)) {
		if (! DataMgr::VariableExists(ts, elevation.c_str(), reflevel, lod)) {
			return(0);
		}
	} 
		

	//
	// See if in cache
	//
	bool exists;
	if (_VarInfoCache.GetExist(ts, varname, reflevel, lod, exists)) { 
		return((int) exists);
	}

	if (DataMgr::IsVariableNative(varname)) {
		exists = _VariableExists(ts, varname, reflevel, lod);
		_VarInfoCache.SetExist(ts, varname, reflevel, lod, exists);
		return((int) exists);
	}

	PipeLine *pipeline = get_pipeline_for_var(varname);
	if(pipeline == NULL) {
		_VarInfoCache.SetExist(ts, varname, reflevel, lod, false);
		return 0;
	}

	const vector <pair <string, VarType_T> > &ovars = pipeline->GetOutputs();
	//
	// Recursively test existence of all dependencies
	//
	for (int i=0; i<ovars.size(); i++) {
		if (! VariableExists(ts, ovars[i].first.c_str(), reflevel, lod)) {
			_VarInfoCache.SetExist(ts, varname, reflevel, lod, false);
			return(0);
		}
	}
	_VarInfoCache.SetExist(ts, varname, reflevel, lod, true);
	return(1);
}


bool DataMgr::IsVariableNative(string name) const {
	vector <string> svec = get_native_variables();

	for (int i=0; i<svec.size(); i++) {
		if (name.compare(svec[i]) == 0) return (true);
	}
	return(false);
}

bool DataMgr::IsVariableDerived(string name) const {
	if (name.size() == 0) return(false);

	vector <string> svec = get_derived_variables();

	for (int i=0; i<svec.size(); i++) {
		if (name.compare(svec[i]) == 0) return (true);
	}
	return(false);
}

vector <string> DataMgr::GetVariableNames() const {
    vector <string> svec1, svec2;
    svec1 = GetVariables3D();
    for(int i=0; i<svec1.size(); i++) svec2.push_back(svec1[i]);

    svec1 = GetVariables2DXY();
    for(int i=0; i<svec1.size(); i++) svec2.push_back(svec1[i]);
    svec1 = GetVariables2DXZ();
    for(int i=0; i<svec1.size(); i++) svec2.push_back(svec1[i]);
    svec1 = GetVariables2DYZ();
    for(int i=0; i<svec1.size(); i++) svec2.push_back(svec1[i]);

    return(svec2);
};


vector <string> DataMgr::GetVariables3D() const {

	// Get native variables (variables contained in the data set)
	//
	vector <string> svec = _GetVariables3D();

	// Now get derived variables 
	//
	for (int i=0; i<_PipeLines.size(); i++) {
		const vector <pair <string,VarType_T> > &outputs = _PipeLines[i]->GetOutputs();
		for (int j=0; j<outputs.size(); j++) {
			if (outputs[j].second == VAR3D) svec.push_back(outputs[j].first);
		}
	}
	return(svec);
}

vector <string> DataMgr::GetVariables2DXY() const {

	// Get native variables (variables contained in the data set)
	//
	vector <string> svec = _GetVariables2DXY();

	// Now get derived variables 
	//
	for (int i=0; i<_PipeLines.size(); i++) {
		const vector <pair <string,VarType_T> > &outputs = _PipeLines[i]->GetOutputs();
		for (int j=0; j<outputs.size(); j++) {
			if (outputs[j].second == VAR2D_XY) svec.push_back(outputs[j].first);
		}
	}
	return(svec);
}
vector <string> DataMgr::GetVariables2DXZ() const {

	// Get native variables (variables contained in the data set)
	//
	vector <string> svec = _GetVariables2DXZ();

	// Now get derived variables 
	//
	for (int i=0; i<_PipeLines.size(); i++) {
		const vector <pair <string,VarType_T> > &outputs = _PipeLines[i]->GetOutputs();
		for (int j=0; j<outputs.size(); j++) {
			if (outputs[j].second == VAR2D_XZ) svec.push_back(outputs[j].first);
		}
	}
	return(svec);
}

vector <string> DataMgr::GetVariables2DYZ() const {

	// Get native variables (variables contained in the data set)
	//
	vector <string> svec = _GetVariables2DYZ();

	// Now get derived variables 
	//
	for (int i=0; i<_PipeLines.size(); i++) {
		const vector <pair <string,VarType_T> > &outputs = _PipeLines[i]->GetOutputs();
		for (int j=0; j<outputs.size(); j++) {
			if (outputs[j].second == VAR2D_YZ) svec.push_back(outputs[j].first);
		}
	}
	return(svec);
}


int DataMgr::GetDataRange(
	size_t ts,
	const char *varname,
	float *range,
	int reflevel,
	int lod
) {
	if (reflevel < 0) reflevel = DataMgr::GetNumTransforms();
	if (lod < 0) lod = DataMgr::GetCRatios().size()-1;

	int	rc;
	range[0] = range[1] = 0.0;

	SetDiagMsg("DataMgr::GetDataRange(%d,%s)", ts, varname);


	// See if we've already cache'd it.
	//
	if (_VarInfoCache.GetRange(ts, varname, reflevel, lod, range)) {
		return(0);
	}

	// Range isn't cache'd. Need to get it from derived class
	//
	if (DataMgr::IsVariableNative(varname)) {

		rc = _OpenVariableRead(ts, varname, reflevel,lod);
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

			_VarInfoCache.SetRange(ts, varname, reflevel, lod, range);

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
	rc = DataMgr::GetValidRegion(ts, varname, reflevel, min, max);
	if (rc<0) return(-1);

	
	const RegularGrid *rg = DataMgr::GetGrid(
		ts, varname, reflevel, lod, min, max, 1
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
	DataMgr::UnlockGrid(rg);
	delete rg;

	_VarInfoCache.SetRange(ts, varname, reflevel, lod, range);

	return(0);
}
	
int DataMgr::GetValidRegion(
	size_t ts,
	const char *varname,
	int reflevel,
	size_t min[3],
	size_t max[3]
) {
	if (reflevel < 0) reflevel = DataMgr::GetNumTransforms();

	int	rc;

	SetDiagMsg("DataMgr::GetValidRegion(%d,%s,%d)",ts,varname,reflevel);
	for (int i=0; i<3; i++) {
		min[i] = max[i] = 0;
	}

	// See if we've already cache'd it.
	//

	if (_VarInfoCache.GetRegion(ts, varname, reflevel, min, max)) {
		return(0);
	}

	if (DataMgr::IsVariableNative(varname)) {

		// Range isn't cache'd. Need to read it from the file
		//
		rc = _OpenVariableRead(ts, varname, reflevel, 0);
		if (rc < 0) {
			SetErrMsg(
				"Failed to read variable/timestep/level/lod (%s, %d, %d %d)",
				varname, ts, reflevel, 0
			);
			return(-1);
		}

		_GetValidRegion(min, max, reflevel);

		_VarInfoCache.SetRegion(ts,varname, reflevel, min, max);

		_CloseVariable();
	}
	else if (DataMgr::IsVariableDerived(varname)) {

		//
		// Initialize min1 and max1 to maximum extents
		//
		size_t dim[3], min1[3], max1[3];
		DataMgr::GetDim(dim, reflevel);
		for (int i=0; i<3; i++) {
			min1[i] = 0;
			max1[i] = dim[i]-1;
		}

		//
		// Get the pipline stage for computing this variable
		//
		PipeLine *pipeline = get_pipeline_for_var(varname);
		assert(pipeline != NULL);

		const vector  <string> ivars = pipeline->GetInputs();

		//
		// Recursively compute the intersection of valid regions for 
		// all variables that a dependencies of this variable
		//
		for (int i=0; i<ivars.size(); i++) {
			size_t min2[3], max2[3];
			
			int rc = DataMgr::GetValidRegion(
				ts, ivars[i].c_str(), reflevel, min2, max2
			);
			if (rc<0) return(-1);

			for (int j=0; j<3; j++) {
				if (min2[j] > min1[j]) min1[j] = min2[j];
				if (max2[j] < max1[j]) max1[j] = max2[j];
			}
		}
		for (int i=0; i<3; i++) {
			min[i] = min1[i];
			max[i] = max1[i];
		}
	}
	else {
		SetErrMsg("Invalid variable : %s", varname);
		return(-1);
	}

	//
	// Cache results
	//
	_VarInfoCache.SetRegion(ts,varname, reflevel, min, max);
		
	return(0);
}

	
void	DataMgr::unlock_blocks(
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
	"DataMgr::unlock_blocks(%xll) - Failed to free blocks\n", blks
);
	return;
}

void	DataMgr::UnlockGrid(
	const RegularGrid *rg
) {
	SetDiagMsg("DataMgr::UnlockGrid()");
	float **blks = rg->GetBlks();
	if (blks) unlock_blocks(blks[0]);

	const LayeredGrid *lg = dynamic_cast<const LayeredGrid *>(rg);
	if (lg) {
		float **coords = lg->GetCoordBlks();
		if (coords) unlock_blocks(coords[0]);
	}
}
	


float	*DataMgr::get_region_from_cache(
	size_t ts,
	string varname,
	int reflevel,
	int lod,
	const size_t min[3],
	const size_t max[3],
	bool	lock
) {

	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); itr++) {
		region_t &region = *itr;

		if (region.ts == ts &&
			region.varname.compare(varname) == 0 &&
			region.reflevel == reflevel &&
			region.lod == lod &&
			region.min[0] == min[0] &&
			region.min[1] == min[1] &&
			region.min[2] == min[2] &&
			region.max[0] == max[0] &&
			region.max[1] == max[1] &&
			region.max[2] == max[2] ) {

			// Increment the lock counter
			region.lock_counter += lock ? 1 : 0;

			// Move region to front of list
			region_t tmp_region = region;
			_regionsList.erase(itr);
			_regionsList.push_back(tmp_region);

			SetDiagMsg(
				"DataMgr::GetGrid() - data in cache %xll\n", tmp_region.blks
			);
			return(tmp_region.blks);
		}
	}

	return(NULL);

}

float *DataMgr::get_region_from_fs(
	size_t ts, string varname, int reflevel, int lod,
    const size_t min[3], const size_t max[3], bool	lock
) {
	VarType_T vtype = DataMgr::GetVarType(varname);

	size_t bmin[3], bmax[3];
	map_vox_to_blk(min, bmin, reflevel);
	map_vox_to_blk(max, bmax, reflevel);

	float *blks = alloc_region(
		ts,varname.c_str(),vtype, reflevel, lod, 
		min,max,lock,false
	);
	if (! blks) return(NULL);

    int rc = _OpenVariableRead(
		ts, varname.c_str(), reflevel, lod
	);
    if (rc < 0) {
		free_region(ts,varname.c_str(),reflevel,lod,min,max);
		return(NULL);
	}

	rc = _BlockReadRegion(bmin, bmax, blks);
    if (rc < 0) {
		free_region(ts,varname.c_str(),reflevel,lod,min,max);
		return(NULL);
	}

	_CloseVariable();

	SetDiagMsg("DataMgr::GetGrid() - data read from fs\n");
	return(blks);
}

float *DataMgr::get_region(
	size_t ts, string varname, int reflevel, int lod, 
	const size_t min[3], const size_t max[3], bool lock, 
	bool *ondisk
) {
	if (varname.size() == 0) return(NULL);
	// See if region is already in cache. If not, read from the 
	// file system.
	//
	*ondisk = false;
	float *blks = get_region_from_cache(
		ts, varname, reflevel, lod, min, max, lock
	);
	if (! blks && ! DataMgr::IsVariableDerived(varname)) {
		blks = (float *) get_region_from_fs(
			ts, varname, reflevel, lod, min, max, lock
		);
		if (! blks) {
			SetErrMsg(
				"Failed to read region from variable/timestep/level/lod (%s, %d, %d, %d)",
				varname.c_str(), ts, reflevel, lod
			);
			return(NULL);
		}
		*ondisk = true;
	}
	return(blks);
}

float	*DataMgr::alloc_region(
	size_t ts,
	const char *varname,
	VarType_T vtype,
	int reflevel,
	int lod,
	const size_t min[3],
	const size_t max[3],
	bool	lock,
	bool fill
) {

	size_t mem_block_size;
	if (! _blk_mem_mgr) {


		size_t bs[3];
		_GetBlockSize(bs, -1);
		mem_block_size = bs[0]*bs[1]*bs[2];

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
	free_region(ts,varname,reflevel,lod,min,max);

	size_t bmin[3], bmax[3];
	map_vox_to_blk(min, bmin, reflevel);
	map_vox_to_blk(max, bmax, reflevel);

	int	vs = 4;

	size_t bs[3];
	_GetBlockSize(bs, reflevel);

	size_t size;
	switch (vtype) {
	case VAR2D_XY:
		size = ((bmax[0]-bmin[0]+1)*bs[0]) * ((bmax[1]-bmin[1]+1)*bs[1]) * vs;
		break;

	case VAR2D_XZ:
		size = ((bmax[0]-bmin[0]+1)*bs[0]) * ((bmax[1]-bmin[1]+1)*bs[2]) * vs;
		break;
	case VAR2D_YZ:
		size = ((bmax[0]-bmin[0]+1)*bs[1]) * ((bmax[1]-bmin[1]+1)*bs[2]) * vs;
		break;

	case VAR3D:
		size = ((bmax[0]-bmin[0]+1)*bs[0]) * ((bmax[1]-bmin[1]+1)*bs[1]) *
			((bmax[2]-bmin[2]+1)*bs[2]) * vs;
		break;

	default:
		SetErrMsg("Invalid variable type");
		return(NULL);
	}
	size_t nblocks = (size_t) ceil((double) size / (double) mem_block_size);
		
	float *blks;
	while (! (blks = (float *) _blk_mem_mgr->Alloc(nblocks, fill))) {
		if (free_lru() < 0) {
			SetErrMsg("Failed to allocate requested memory");
			return(NULL);
		}
	}

	region_t region;

	region.ts = ts;
	region.varname.assign(varname);
	region.reflevel = reflevel;
	region.lod = lod;
	region.min[0] = min[0];
	region.min[1] = min[1];
	region.min[2] = min[2];
	region.max[0] = max[0];
	region.max[1] = max[1];
	region.max[2] = max[2];
	region.lock_counter = lock ? 1 : 0;
	region.blks = blks;

	_regionsList.push_back(region);

	return(region.blks);
}

void	DataMgr::free_region(
	size_t ts,
	string varname,
	int reflevel,
	int lod,
	const size_t min[3],
	const size_t max[3]
) {

	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); itr++) {
		const region_t &region = *itr;

		if (region.ts == ts &&
			region.varname.compare(varname) == 0 &&
			region.reflevel == reflevel &&
			region.lod == lod &&
			region.min[0] == min[0] &&
			region.min[1] == min[1] &&
			region.min[2] == min[2] &&
			region.max[0] == max[0] &&
			region.max[1] == max[1] &&
			region.max[2] == max[2] ) {

			if (region.lock_counter == 0) {
				if (region.blks) _blk_mem_mgr->FreeMem(region.blks);
				
				_regionsList.erase(itr);
				return;
			}
		}
	}

	return;
}

void	DataMgr::Clear() {

	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); itr++) {
		const region_t &region = *itr;

		if (region.blks) _blk_mem_mgr->FreeMem(region.blks);
			
	}
	_regionsList.clear();
	_VarInfoCache.Clear();
}

void	DataMgr::free_var(const string &varname, int do_native) {

	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); ) {
		const region_t &region = *itr;

		if (region.varname.compare(varname) == 0 && do_native) {

			if (region.blks) _blk_mem_mgr->FreeMem(region.blks);
				
			_regionsList.erase(itr);
			itr = _regionsList.begin();
		}
		else itr++;
	}

}


int	DataMgr::free_lru(
) {

	// The least recently used region is at the front of the list
	//
	list <region_t>::iterator itr;
	for(itr = _regionsList.begin(); itr!=_regionsList.end(); itr++) {
		const region_t &region = *itr;

		if (region.lock_counter == 0) {
			if (region.blks) _blk_mem_mgr->FreeMem(region.blks);
			_regionsList.erase(itr);
			return(0);
		}
	}

	// nothing to free
	return(-1);
}
	

PipeLine *DataMgr::get_pipeline_for_var(string varname) const {

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

RegularGrid *DataMgr::execute_pipeline(
	size_t ts,
	string varname,
	int reflevel,
	int lod,
	const size_t min[3],
	const size_t max[3],
	bool	lock,
	float *xcblks,
	float *ycblks,
	float *zcblks
) {

	if (reflevel < 0) reflevel = GetNumTransforms();
	if (lod < 0) lod = GetCRatios().size()-1;

	_VarInfoCache.PurgeRange(ts, varname, reflevel, lod);
	_VarInfoCache.PurgeRegion(ts, varname, reflevel);
	_VarInfoCache.PurgeExist(ts, varname, reflevel, lod);

	PipeLine *pipeline = get_pipeline_for_var(varname);
 
	assert(pipeline != NULL);

	const vector <string> &input_varnames = pipeline->GetInputs();
	const vector <pair <string, VarType_T> > &output_vars = pipeline->GetOutputs();

    VarType_T vtype = DataMgr::GetVarType(varname);

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
		VarType_T vtype_in = DataMgr::GetVarType(input_varnames[i]);

		// 
		// If the requested output variable is 2D and an input variable
		// is 3D we need to make sure that the 3rd dimension of the
		// 3D input variable covers the full domain
		//
		if (vtype != VAR3D && vtype_in == VAR3D) {
			size_t dims[3];
			DataMgr::GetDim(dims, reflevel);

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
						ts, input_varnames[i], reflevel, lod, 
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

		float *blks = alloc_region(
			ts,v.c_str(),vtype_out, reflevel, lod, min,max,true,
			true
		);
		if (! blks) {
			// Unlock any locked variables and abort
			//
			for (int j=0;j<in_grids.size();j++) UnlockGrid(in_grids[j]);
			for (int j=0;j<out_grids.size();j++) UnlockGrid(out_grids[j]);
			return(NULL);
		}
		RegularGrid *rg = make_grid(
			ts, v, reflevel, lod, min, max, blks, 
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
		in_grids, out_grids, ts, reflevel, lod 
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

bool DataMgr::cycle_check(
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
bool DataMgr::depends_on(
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

//
// return complete list of native variables
//
vector <string> DataMgr::get_native_variables() const {
    vector <string> svec1, svec2;

    svec1 = _GetVariables3D();
    for(int i=0; i<svec1.size(); i++) svec2.push_back(svec1[i]);
    svec1 = _GetVariables2DXY();
    for(int i=0; i<svec1.size(); i++) svec2.push_back(svec1[i]);
    svec1 = _GetVariables2DXZ();
    for(int i=0; i<svec1.size(); i++) svec2.push_back(svec1[i]);
    svec1 = _GetVariables2DYZ();
    for(int i=0; i<svec1.size(); i++) svec2.push_back(svec1[i]);

    return(svec2);
}

//
// return complete list of derived variables
//
vector <string> DataMgr::get_derived_variables() const {

    vector <string> svec;

	for (int i=0; i<_PipeLines.size(); i++) {
		const vector <pair <string, VarType_T> > &ovars = _PipeLines[i]->GetOutputs();
		for (int j=0; j<ovars.size(); j++) {
			svec.push_back(ovars[j].first);
		}
	}
    return(svec);
}

DataMgr::VarType_T DataMgr::GetVarType(const string &varname) const {
	if (! DataMgr::IsVariableDerived(varname)) {
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
	

 
void DataMgr::PurgeVariable(string varname){
	free_var(varname,1);
	_VarInfoCache.PurgeVariable(varname);
}


void    DataMgr::map_vox_to_blk(
	const size_t vcoord[3], size_t bcoord[3], int reflevel
) const {
    size_t bs[3];
    _GetBlockSize(bs, reflevel);

    for (int i=0; i<3; i++) {
        bcoord[i] = vcoord[i] / bs[i];
    }
}

void    DataMgr::get_dim_blk(
	size_t bdim[3], int reflevel
) const {
	size_t dim[3];
	DataMgr::GetDim(dim, -1);

	for (int i=0; i<3; i++) {
		if (dim[i]) dim[i]--;
	}
	size_t bmax[3];
	map_vox_to_blk(dim, bmax, reflevel);

	for (int i=0; i<3; i++) {
		bdim[i] = bmax[i] + 1;
	}
}

void DataMgr::coord_array(
	const vector <double> &xin,
	vector <double> &xout,
	size_t ldelta
) const {
	xout.clear();
	if (ldelta == 0) {
		for (int i=0; i<xin.size(); i++) {
			xout.push_back(xin[i]);
		}
		return;
	}
	for (int i=0; i<(xin.size() >> ldelta); i++) {
		size_t xin_idx = (size_t) ((((double) (1<<ldelta) * 0.5) - 0.5) + (1<<ldelta)*i);
		assert(xin_idx+1 < xin.size());
		double x = (xin[xin_idx] + xin[xin_idx+1]) * 0.5;
		xout.push_back(x);
	}
}


void	DataMgr::map_vox_to_user_regular(
    size_t timestep, 
	const size_t vcoord0[3], double vcoord1[3],
	int	reflevel
) const {

    if (reflevel < 0 || reflevel > DataMgr::GetNumTransforms()) {
		reflevel = DataMgr::GetNumTransforms();
	}
    int  ldelta = DataMgr::GetNumTransforms() - reflevel;

	size_t	dim[3];

	vector <double> extents = _GetExtents(timestep);

	DataMgr::GetDim(dim, -1);	// finest dimension
	for(int i = 0; i<3; i++) {

		// distance between voxels along dimension 'i' in user coords
		double deltax = (extents[i+3] - extents[i]) / (dim[i] - 1);

		// coordinate of first voxel in user space
		double x0 = extents[i];

		// Boundary shrinks and step size increases with each transform
		for(int j=0; j<(int)ldelta; j++) {
			x0 += 0.5 * deltax;
			deltax *= 2.0;
		}
		vcoord1[i] = x0 + (vcoord0[i] * deltax);
	}
}


void   DataMgr::MapUserToVox(
    size_t timestep,
    const double xyz[3], size_t ijk[3], int reflevel
) {

	SetDiagMsg(
		"DataMgr::MapUserToVox(%d, (%f, %f, %f), (,,) %d)",
		timestep, xyz[0], xyz[1], xyz[2], reflevel
	);
	ijk[0] = 0;
	ijk[1] = 0;
	ijk[2] = 0;

	size_t dims[3];
	DataMgr::GetDim(dims, reflevel);
	size_t min[] = {0,0,0};
	size_t max[] = {dims[0]-1, dims[1]-1, dims[2]-1};

	bool enable = EnableErrMsg(false);
	RegularGrid *rg = GetGrid(
		timestep,"", reflevel, 0, min, max, false
	);
	EnableErrMsg(enable);
	
	if (! rg) return;

	rg->GetIJKIndex(xyz[0],xyz[1],xyz[2], &ijk[0], &ijk[1], &ijk[2]);
	delete rg;
}

void   DataMgr::MapVoxToUser(
    size_t timestep,
    const size_t ijk[3], double xyz[3], int reflevel
) {
	SetDiagMsg(
		"DataMgr::MapVoxToUser(%d, (%d, %d, %d), (,,) %d)",
		timestep, ijk[0], ijk[1], ijk[2], reflevel
	);

	xyz[0] = 0.0;
	xyz[1] = 0.0;
	xyz[2] = 0.0;

	size_t dims[3];
	DataMgr::GetDim(dims, reflevel);
	size_t min[] = {0,0,0};
	size_t max[] = {dims[0]-1, dims[1]-1, dims[2]-1};

	bool enable = EnableErrMsg(false);
	RegularGrid *rg = (RegularGrid *) GetGrid(
		timestep,"", reflevel, 0, min, max, false
	);
	EnableErrMsg(enable);

	if (! rg) return;

	rg->GetUserCoordinates(ijk[0], ijk[1], ijk[2], &xyz[0],&xyz[1],&xyz[2]);
	delete rg;
}


void    DataMgr::GetEnclosingRegion(
    size_t ts, const double minu[3], const double maxu[3],
    size_t min[3], size_t max[3],
    int reflevel
) {
    size_t dims[3];
    DataMgr::GetDim(dims, reflevel);

	for (int i=0; i<3; i++) {
		min[i] = 0;
		max[i] = dims[i]-1;
	}
	bool enable = EnableErrMsg(false);
	RegularGrid *rg = GetGrid(ts,"", reflevel, 0, min, max, false);
	EnableErrMsg(enable);

	if (! rg) return;

	//
	// Need to turn off peridic boundary handling or we get undesired 
	// results.
	//
	bool p[] = {false,false,false};
	rg->SetPeriodic(p);

	rg->GetEnclosingRegion(minu, maxu, min, max);
	delete rg;
}

DataMgr::VarInfoCache::var_info *DataMgr::VarInfoCache::get_var_info(
	size_t ts, string varname
) const {
	map <size_t, map <string, var_info> >::const_iterator itr1;

	itr1 = _cache.find(ts);
	if (itr1 == _cache.end()) return (NULL);

	map <string, var_info >::const_iterator itr2;
	itr2 = itr1->second.find(varname);
	if (itr2 == itr1->second.end()) return(NULL);

	return((VarInfoCache::var_info *) &(itr2->second));
}


bool DataMgr::VarInfoCache::GetRange(
	size_t ts, string varname, int ref, int lod, float range[2]
) const {

	var_info *viptr = get_var_info(ts, varname);
	if (! viptr) return(false);

	size_t uid = ((size_t) ref << 8) + (size_t) lod;

	map <size_t, vector <float> >::const_iterator itr;
	itr = viptr->range.find(uid);
	if (itr == viptr->range.end()) return(false);

	range[0] = itr->second[0];
	range[1] = itr->second[1];
	return(true);
}

void DataMgr::VarInfoCache::SetRange(
	size_t ts, string varname, int ref, int lod, const float range[2]
) {

	vector <float> myrange;
	myrange.push_back(range[0]);
	myrange.push_back(range[1]);

	size_t uid = ((size_t) ref << 8) + (size_t) lod;
	
	_cache[ts][varname].range[uid] = myrange;
}

void DataMgr::VarInfoCache::PurgeRange(
	size_t ts, string varname, int ref, int lod
) {
	var_info *viptr = get_var_info(ts, varname);
	if (! viptr) return;

	size_t uid = ((size_t) ref << 8) + (size_t) lod;

	map <size_t, vector <float> >::iterator itr;
	itr = viptr->range.find(uid);
	if (itr == viptr->range.end()) return;

	viptr->range.erase(itr);
}


bool DataMgr::VarInfoCache::GetRegion(
	size_t ts, string varname, int ref, size_t min[3], size_t max[3]
) const {

	var_info *viptr = get_var_info(ts, varname);
	if (! viptr) return(false);

	size_t uid = (size_t) ref;

	map <size_t, vector <size_t> >::const_iterator itr;
	itr = viptr->region.find(uid);
	if (itr == viptr->region.end()) return(false);

	min[0] = itr->second[0];
	min[1] = itr->second[1];
	min[2] = itr->second[2];
	max[0] = itr->second[3];
	max[1] = itr->second[4];
	max[2] = itr->second[5];
	return(true);
}

void DataMgr::VarInfoCache::SetRegion(
	size_t ts, string varname, int ref,
	const size_t min[3], const size_t max[3]
) {

	vector <size_t> minmax;
	minmax.push_back(min[0]);
	minmax.push_back(min[1]);
	minmax.push_back(min[2]);
	minmax.push_back(max[0]);
	minmax.push_back(max[1]);
	minmax.push_back(max[2]);

	size_t uid = (size_t) ref;
	
	_cache[ts][varname].region[uid] = minmax;
}

void DataMgr::VarInfoCache::PurgeRegion(
	size_t ts, string varname, int ref
) {

	var_info *viptr = get_var_info(ts, varname);
	if (! viptr) return;

	size_t uid = (size_t) ref;

	map <size_t, vector <size_t> >::iterator itr;
	itr = viptr->region.find(uid);
	if (itr == viptr->region.end()) return;

	viptr->region.erase(itr);
}

bool DataMgr::VarInfoCache::GetExist(
	size_t ts, string varname, int ref, int lod, bool &exist
) const {

	var_info *viptr = get_var_info(ts, varname);
	if (! viptr) return(false);

	size_t uid = ((size_t) ref << 8) + (size_t) lod;

	map <size_t, bool>::const_iterator itr;
	itr = viptr->exist.find(uid);
	if (itr == viptr->exist.end()) return(false);

	exist = itr->second;
	return(true);
}

void DataMgr::VarInfoCache::SetExist(
	size_t ts, string varname, int ref, int lod, bool exist
) {

	size_t uid = ((size_t) ref << 8) + (size_t) lod;
	
	_cache[ts][varname].exist[uid] = exist;
}

void DataMgr::VarInfoCache::PurgeExist(
	size_t ts, string varname, int ref, int lod
) {

	var_info *viptr = get_var_info(ts, varname);
	if (! viptr) return;

	size_t uid = ((size_t) ref << 8) + (size_t) lod;

	map <size_t, bool>::iterator itr;
	itr = viptr->exist.find(uid);
	if (itr == viptr->exist.end()) return;

	viptr->exist.erase(itr);
}

void DataMgr::VarInfoCache::PurgeVariable(string varname) {

	map <size_t, map <string, var_info> >::iterator itr1;

	for (itr1 = _cache.begin(); itr1 != _cache.end(); ++itr1) {

		map <string, var_info >::iterator itr2;
		itr2 = itr1->second.find(varname);
		if (itr2 != itr1->second.end()) {
			itr1->second.erase(itr2);
		}
	}
}

bool DataMgr::IsCoordinateVariable(string varname) const {
	vector <string> coordvars = GetCoordinateVariables();
	for (int i=0; i<coordvars.size(); i++) {
		if (coordvars[i].compare(varname) == 0) return (true);
	}
	return(false);
}

vector<double> DataMgr::GetExtents(size_t ts) const {
	vector <double> extents = _GetExtents(ts); 
	if (DataMgr::GetCoordSystemType().compare("spherical")==0) {
		vector <long> permv = GetGridPermutation();
		double r = extents[3+permv[2]];
		extents[0] = extents[1] = extents[2] = -r;
		extents[3] = extents[4] = extents[5] = r;
	}

	return(extents);
};
