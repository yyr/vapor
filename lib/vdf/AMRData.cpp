//
//      $Id$
//
//***********************************************************************
//                                                                      *
//                      Copyright (C)  2006	                        *
//          University Corporation for Atmospheric Research             *
//                      All Rights Reserved                             *
//                                                                      *
//***********************************************************************
//
//	File:		AMRData.cpp
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Thu Jan 5 17:00:23 MST 2006
//
//	Description:	
//
//

#include <iostream>
#include <cmath>
#include <cstring>
#include <cassert>
#include <netcdf.h>
#ifndef WIN32
#include <unistd.h>
#else
#include "windows.h"
#include <vapor/common.h>
#endif

#include <vapor/AMRData.h>

using namespace VAPoR;
using namespace VetsUtil;


const string AMRData::_numNodeToken = "NumBlocks";
const string AMRData::_blockSizeXToken = "BlocksSizeNx";
const string AMRData::_blockSizeYToken = "BlocksSizeNy";
const string AMRData::_blockSizeZToken = "BlocksSizeNz";
const string AMRData::_varToken = "FieldVariable";
const string AMRData::_blockMinToken = "MinCorner";
const string AMRData::_blockMaxToken = "MaxCorner";
const string AMRData::_refinementLevelToken = "RefinementLevel";
const string AMRData::_scalarRangeToken = "ScalarDataRange";


#define NC_ERR_WRITE(rc, path) \
	if (rc != NC_NOERR) { \
		SetErrMsg( \
			"Error writing netCDF file \"%s\" : %s",  \
			path.c_str(), nc_strerror(rc) \
		); \
		return(-1); \
	}

#define NC_ERR_READ(rc, path) \
	if (rc != NC_NOERR) { \
		SetErrMsg( \
			"Error reading netCDF file \"%s\" : %s",  \
			path.c_str(), nc_strerror(rc) \
		); \
		return(-1); \
	}

void AMRData::_AMRData(
	const AMRTree *tree,
	const size_t cell_dim[3],
	const size_t bmin[3],
	const size_t bmax[3],
	int reflevel 
) {
	_treeData = NULL;
	_treeDataMemSize = NULL;
	_tree = NULL;

	if ((cell_dim[0] != cell_dim[1]) || (cell_dim[0] != cell_dim[2])) {
        SetErrMsg("Cell dimensions must all be equal");
		return;
	}
		
	_dataRange[0] = _dataRange[1] = 0.0;

	const size_t *bdim = tree->GetBaseDim();

	for(int i=0; i<3; i++) {
		if ((bmin[i] >= bdim[i]) || (bmax[i] > bdim[i]) || (bmin[i]>bmax[i])){
			SetErrMsg("Block coordinates invalid");
			return;
		}
    }

//	if (reflevel<0 || reflevel>tree->GetRefinementLevel(bmin,bmax)) {
//		reflevel = tree->GetRefinementLevel(bmin, bmax);
//	}
	if (reflevel<0) {
		reflevel = tree->GetRefinementLevel(bmin, bmax);
	}


	for(int i=0; i<3; i++) {
		_cellDim[i] = cell_dim[i];
		_bmin[i] = bmin[i];
		_bmax[i] = bmax[i];
	}
	_tree = tree;
	_maxRefinementLevel = reflevel;

	int nbb = bdim[0] * bdim[1] * bdim[2];

	_treeData = new float* [nbb];
	assert(_treeData != NULL);

	_treeDataMemSize = new int[nbb];
	assert(_treeDataMemSize != NULL);

	for(int i = 0; i<nbb; i++) {
		_treeData[i] = NULL;
		_treeDataMemSize[i] = 0;
	}

	for(int z=_bmin[2]; z<=_bmax[2]; z++) {
	for(int y=_bmin[1]; y<=_bmax[1]; y++) {
	for(int x=_bmin[0]; x<=_bmax[0]; x++) {

		const size_t xyz[3] = {x,y,z};
		size_t size;
		int	index;

		index = (z * bdim[1] * bdim[0]) + (y*bdim[0]) + x;

		const AMRTreeBranch	*tbranch = tree->GetBranch(xyz);

		// Allocate enough space to store data for this base block. Each
		// node in the tree, both leaf and non-leaf, contains data. The
		// non-leaf data are approximations of the leaf data
		//
		int nnodes = tbranch->GetNumCells();
		size = _cellDim[0] * _cellDim[1] * _cellDim[2] * nnodes;

		_treeData[index] = new(nothrow) float[size];
		if (! _treeData[index]) {
			SetErrMsg("Memory allocation of %llu floats failed", (long long) size);
			return;
		}

		_treeDataMemSize[index] = nnodes;
	}
	}
	}
}

AMRData::AMRData(
	const AMRTree *tree,
	const size_t cell_dim[3],
	const size_t bmin[3],
	const size_t bmax[3],
	int reflevel
) {
	_AMRData(tree, cell_dim, bmin, bmax, reflevel);
}

AMRData::AMRData(
	const AMRTree *tree,
	const size_t cell_dim[3],
	int reflevel
) {

	const size_t *bdim = tree->GetBaseDim();

	size_t	bmin[3] = {0,0,0};
	size_t	bmax[3] = {(size_t) bdim[0]-1,(size_t) bdim[1]-1,(size_t) bdim[2]-1};

	_AMRData(tree, cell_dim, bmin, bmax, reflevel);
}

AMRData::AMRData(
	const AMRTree *tree
) {

	const size_t *bdim = tree->GetBaseDim();
	int reflevel = tree->GetRefinementLevel();

	size_t	cell_dim[3] = {1,1,1};
	size_t	bmin[3] = {0,0,0};
	size_t	bmax[3] = {(size_t) bdim[0]-1,(size_t) bdim[1]-1,(size_t) bdim[2]-1};

	_AMRData(tree, cell_dim, bmin, bmax, reflevel);
}

AMRData::AMRData(
	const AMRTree *tree,
	const size_t cell_dim[3],
	const int paramesh_gids[][15],
	const float paramesh_bboxs [][3][2],
	const float paramesh_unk[],
	int	total_blocks,
	int reflevel
) {

	const size_t *bdim = tree->GetBaseDim();

	size_t	bmin[3] = {0,0,0};
	size_t	bmax[3] = {(size_t) bdim[0]-1,(size_t) bdim[1]-1,(size_t) bdim[2]-1};

	_AMRData(tree, cell_dim, bmin, bmax, reflevel);
	if (GetErrCode()) return;

	vector <int> baseblocks;

	baseblocks.reserve(bdim[0]*bdim[1]*bdim[2]);

	if (tree->_parameshGetBaseBlocks(baseblocks,bdim,paramesh_gids,paramesh_bboxs, total_blocks)<0)return;

	_dataRange[0] = _dataRange[1] = paramesh_unk[0];
	for(int i =0; i<baseblocks.size(); i++) {
		if (paramesh_copy_data(i, baseblocks[i], paramesh_gids, paramesh_unk) < 0) {
			return;
		}
	}
}

void AMRData::_AMRDataFree() {

	if (! _tree) return;

	const size_t *bdim = _tree->GetBaseDim();
	int	size = bdim[0] * bdim[1] * bdim[2];

	if (_treeData) {
		for (int i=0; i<size; i++) {
			if (_treeData[i]) delete [] _treeData[i];
			_treeData[i] = NULL;
		}
		delete [] _treeData;
	}
	if (_treeDataMemSize) delete [] _treeDataMemSize;
	_treeData = NULL;
	_treeDataMemSize = NULL;

}

int AMRData::SetRegion(
	const size_t bmin[3],
	const size_t bmax[3],
	int reflevel
) {
	const size_t *bdim = _tree->GetBaseDim();

	for(int i=0; i<3; i++) {
		if ((bmin[i] >= bdim[i]) || (bmax[i] >= bdim[i]) || (bmin[i]>bmax[i])){
			SetErrMsg("Block coordinates invalid");
			return(-1);
		}
    }

	if (reflevel<0 || reflevel>_tree->GetRefinementLevel(bmin,bmax)) {
		reflevel = _tree->GetRefinementLevel(bmin, bmax);
	}

	for(int i=0; i<3; i++) {
		_bmin[i] = bmin[i];
		_bmax[i] = bmax[i];
	}
	_maxRefinementLevel = reflevel;

	for(int z=_bmin[2]; z<=_bmax[2]; z++) {
	for(int y=_bmin[1]; y<=_bmax[1]; y++) {
	for(int x=_bmin[0]; x<=_bmax[0]; x++) {

		const size_t xyz[3] = {x,y,z};
		size_t size;
		int	index;

		index = (z * bdim[1] * bdim[0]) + (y*bdim[0]) + x;

		const AMRTreeBranch	*tbranch = _tree->GetBranch(xyz);

		// Allocate enough space to store data for this base block. Each
		// node in the tree, both leaf and non-leaf, contains data. The
		// non-leaf data are approximations of the leaf data
		//
		// N.B. if the new region is smaller than the old region we
		// don't free data from the old region that is no longer in 
		// the new region.
		//
		int nnodes = tbranch->GetNumCells();

		if (_treeDataMemSize[index] < nnodes) {

			if (_treeData[index]) delete [] _treeData[index];

			size = _cellDim[0] * _cellDim[1] * _cellDim[2] * nnodes;

			_treeData[index] = new(nothrow) float[size];
			if (! _treeData[index]) {
				SetErrMsg("Memory allocation of %llu floats failed", (long long) size);
				return(-1);
			}
			_treeDataMemSize[index] = nnodes;
		}
	}
	}
	}

	return(0);
}

void AMRData::GetRegion(
	const size_t **bmin,
	const size_t **bmax,
	int *reflevel
) const {
	*bmin = _bmin;
	*bmax = _bmax;
	*reflevel = _maxRefinementLevel;
}

void AMRData::Update() {

	AMRTree::cid_t branch_nodes = 0;
    const size_t *base_dim = _tree->GetBaseDim();
	bool first = 1;
	for (size_t z=_bmin[2]; z<=_bmax[2]; z++) {
	for (size_t y=_bmin[1]; y<=_bmax[1]; y++) {
	for (size_t x=_bmin[0]; x<=_bmax[0]; x++) {
		

		size_t xyz[3] = {x,y,z};
		AMRTree::cid_t  index = z*base_dim[0]*base_dim[1] + y*base_dim[0] + x;

		const AMRTreeBranch *tbranch = _tree->GetBranch(xyz);

		branch_nodes = tbranch->GetNumCells(-1);

		const float *data = _treeData[index];

		if (first) {
			_dataRange[0] = data[0];
			_dataRange[1] = data[0];
			first = false;
		}

		for (size_t i = 0; i<branch_nodes*_cellDim[2]*_cellDim[1]*_cellDim[0]; i++) {
			if (data[i] < _dataRange[0]) _dataRange[0] = data[i];
			if (data[i] > _dataRange[1]) _dataRange[1] = data[i];
		}

	}
	}
	}
}

//
// IO size for netCDF
//
#define NC_CHUNKSIZEHINT    1024*1024

int AMRData::WriteNCDF(
	const string &path,
	int reflevel
) {

	int	ncid;
	int rc;

	if (reflevel < 0) reflevel = _maxRefinementLevel;
	if (reflevel > _maxRefinementLevel) reflevel = _maxRefinementLevel;

	size_t chsz = NC_CHUNKSIZEHINT;
	rc = nc__create(path.c_str(), NC_64BIT_OFFSET, 0, &chsz, &ncid);
	NC_ERR_WRITE(rc,path)

	// Disable data filling - may not be necessary
	//
	int mode;
	nc_set_fill(ncid, NC_NOFILL, &mode);


	//
	// Define netCDF dimensions
	//
	int node_dim;

	AMRTree::cid_t num_nodes_total = _tree->GetNumCells(_bmin, _bmax, reflevel);

	rc = nc_def_dim(ncid, _numNodeToken.c_str(), num_nodes_total, &node_dim);
	NC_ERR_WRITE(rc,path)

	int bx_dim, by_dim, bz_dim;

	rc = nc_def_dim(ncid, _blockSizeXToken.c_str(), _cellDim[0], &bx_dim);
	NC_ERR_WRITE(rc,path)

	rc = nc_def_dim(ncid, _blockSizeYToken.c_str(), _cellDim[1], &by_dim);
	NC_ERR_WRITE(rc,path)

	rc = nc_def_dim(ncid, _blockSizeZToken.c_str(), _cellDim[2], &bz_dim);
	NC_ERR_WRITE(rc,path)


	//
	// Define netCDF variables
	//
	int dim_ids[] = {node_dim, bz_dim, by_dim, bx_dim};
	int varid;

	rc = nc_def_var(
		ncid, _varToken.c_str(), NC_FLOAT, 
		sizeof(dim_ids)/sizeof(dim_ids[0]), dim_ids, &varid
	);
	NC_ERR_WRITE(rc,path)


	//
	// Define netCDF global attributes
	//
	int bmin_int[] = {_bmin[0], _bmin[1], _bmin[2]};
	rc = nc_put_att_int(
		ncid,NC_GLOBAL,_blockMinToken.c_str(),NC_INT, 3, bmin_int
	);
	NC_ERR_WRITE(rc,path)

	int bmax_int[] = {_bmax[0], _bmax[1], _bmax[2]};
	rc = nc_put_att_int(
		ncid,NC_GLOBAL,_blockMaxToken.c_str(),NC_INT, 3, bmax_int
	);
	NC_ERR_WRITE(rc,path)

	rc = nc_put_att_int(
		ncid,NC_GLOBAL,_refinementLevelToken.c_str(),NC_INT, 1, 
		&reflevel
	);
	NC_ERR_WRITE(rc,path)

	Update();	// update data range
	rc = nc_put_att_float(
		ncid,NC_GLOBAL,_scalarRangeToken.c_str(),NC_FLOAT, 2, _dataRange
	);
	NC_ERR_WRITE(rc,path)

	rc = nc_enddef(ncid);
	NC_ERR_WRITE(rc,path)

    const size_t *base_dim = _tree->GetBaseDim();

	int branch_nodes = 0;
	int n = 0;
	for (int z=_bmin[2]; z<=_bmax[2]; z++) {
	for (int y=_bmin[1]; y<=_bmax[1]; y++) {
	for (int x=_bmin[0]; x<=_bmax[0]; x++) {

		size_t xyz[3] = {x,y,z};
		int index = z*base_dim[0]*base_dim[1] + y*base_dim[0] + x;

		assert(n<num_nodes_total);

		const AMRTreeBranch *tbranch = _tree->GetBranch(xyz);

		branch_nodes = tbranch->GetNumCells(reflevel);

		const float *data = _treeData[index];

		size_t start[] = {n,0,0,0};
        size_t count[] = {branch_nodes,_cellDim[2],_cellDim[1],_cellDim[0]};
		ptrdiff_t stride[] = {1,1,1,1};

        rc = nc_put_vars_float(
                ncid, varid, start, count, stride, data
        );
        if (rc != NC_NOERR) {
            SetErrMsg( "Error writing netCDF file : %s",  nc_strerror(rc) );
            return(-1);
        }

	

		n += branch_nodes;

	}
	}
	}

	nc_close(ncid);

	return(0);
}


int AMRData::ReadAttributesNCDF(
	const string &path,
	size_t cell_dim[3],
	size_t min[3],
	size_t max[3],
	float data_range[2],
	int &reflevel,
	size_t &num_nodes
) {

	int	ncid;
	int rc;

	size_t chsz = NC_CHUNKSIZEHINT;
	int ii = 0;
	do {
		rc = nc__open(path.c_str(), NC_NOWRITE, &chsz, &ncid);
#ifdef WIN32
		if (rc == EAGAIN) Sleep(100);//milliseconds
#else
		if (rc == EAGAIN) sleep(1);//seconds
#endif
            ii++;

	} while (rc != NC_NOERR && ii < 10);

	NC_ERR_READ(rc,path)


	//
	// netCDF Dimensions
	//
	int ncdimid;
	
	rc = nc_inq_dimid(ncid, _numNodeToken.c_str(), &ncdimid);
	NC_ERR_READ(rc,path)
	rc = nc_inq_dimlen(ncid, ncdimid, &num_nodes);
	NC_ERR_READ(rc,path)

	rc = nc_inq_dimid(ncid, _blockSizeXToken.c_str(), &ncdimid);
	NC_ERR_READ(rc,path)
	rc = nc_inq_dimlen(ncid, ncdimid, &cell_dim[0]);
	NC_ERR_READ(rc,path)

	rc = nc_inq_dimid(ncid, _blockSizeYToken.c_str(), &ncdimid);
	NC_ERR_READ(rc,path)
	rc = nc_inq_dimlen(ncid, ncdimid, &cell_dim[1]);
	NC_ERR_READ(rc,path)

	rc = nc_inq_dimid(ncid, _blockSizeZToken.c_str(), &ncdimid);
	NC_ERR_READ(rc,path)
	rc = nc_inq_dimlen(ncid, ncdimid, &cell_dim[2]);
	NC_ERR_READ(rc,path)

	//
	// netCDF Attributes
	//
	int min_int[3];
	rc = nc_get_att_int( ncid,NC_GLOBAL,_blockMinToken.c_str(),min_int);
	NC_ERR_READ(rc,path)
	for (int i=0; i<3; i++) min[i] = (size_t) min_int[i];

	int max_int[3];
	rc = nc_get_att_int( ncid,NC_GLOBAL,_blockMaxToken.c_str(),max_int);
	NC_ERR_READ(rc,path)
	for (int i=0; i<3; i++) max[i] = (size_t) max_int[i];

	rc = nc_get_att_int(ncid,NC_GLOBAL,_refinementLevelToken.c_str(),&reflevel);
	NC_ERR_READ(rc,path)

	rc = nc_get_att_float(ncid,NC_GLOBAL,_scalarRangeToken.c_str(),data_range);
	NC_ERR_READ(rc,path)

	nc_close(ncid);
	return(0);
}

int AMRData::ReadNCDF(
	const string &path,
	const size_t bmin[3],
	const size_t bmax[3],
	int reflevel
) {

	int	ncid;
	int rc;


	//
	// Attributes contained in the file
	//
	size_t file_cell_dim[3];
	size_t file_bmin[3];
	size_t file_bmax[3];
	float file_data_range[2];
	int file_reflevel;
	size_t file_num_nodes;

	rc = ReadAttributesNCDF(
		path, file_cell_dim, file_bmin, file_bmax, file_data_range,
		file_reflevel, file_num_nodes
	);
	if (rc < 0) return(-1);

	// 
	// Make sure requested subregion is contained within file 
	//
	for(int i=0; i<3; i++) {
		if ((bmin[i] < file_bmin[i]) || (bmax[i] > file_bmax[i])) {
			SetErrMsg("Block coordinates out of range");
			return(-1);
		}
    }


	bool rebuild = false;
	bool setregion = false;
	for (int i=0; i<3; i++) {
		if (file_cell_dim[i] != _cellDim[i]) rebuild = true;
		if (bmin[i] != _bmin[i]) setregion = true;
		if (bmax[i] != _bmax[i]) setregion = true;
	}
	if (file_reflevel != _maxRefinementLevel) setregion = true;

	if (rebuild) {
		const AMRTree *tree = _tree;
		_AMRDataFree();
		_AMRData(tree, file_cell_dim, bmin, bmax, file_reflevel);
	}
	else if (setregion) {
		rc = SetRegion(bmin, bmax, file_reflevel);
		if (rc<0) return(rc);
	}
	_dataRange[0] = file_data_range[0];
	_dataRange[1] = file_data_range[1];

	if (reflevel < 0) reflevel = _maxRefinementLevel;
	if (reflevel > _maxRefinementLevel) reflevel = _maxRefinementLevel;

	size_t chsz = NC_CHUNKSIZEHINT;
	int ii = 0;
	do {
		rc = nc__open(path.c_str(), NC_NOWRITE, &chsz, &ncid);
#ifdef WIN32
		if (rc == EAGAIN) Sleep(100);//milliseconds
#else
		if (rc == EAGAIN) sleep(1);//seconds
#endif
            ii++;

	} while (rc != NC_NOERR && ii < 10);

	NC_ERR_READ(rc,path)

	//
	// netCDF Variables
	//
	int	varid;
	rc = nc_inq_varid(ncid, _varToken.c_str(), &varid);
	NC_ERR_READ(rc,path)

	int branch_nodes = 0;	// Num nodes requested
	int file_branch_nodes = 0;	// Num nodes stored in file
	int n = 0;
	const size_t *base_dim = _tree->GetBaseDim();

	for (int z=file_bmin[2]; z<=file_bmax[2]; z++) {
	for (int y=file_bmin[1]; y<=file_bmax[1]; y++) {
	for (int x=file_bmin[0]; x<=file_bmax[0]; x++) {
		size_t xyz[3] = {x,y,z};

		assert(n<file_num_nodes);

		const AMRTreeBranch *tbranch = _tree->GetBranch(xyz);

		file_branch_nodes = tbranch->GetNumCells(file_reflevel);


		//
		// Read data only if in ROI
		//
		if (x >= bmin[0] && x <= bmax[0] && 
			y >= bmin[1] && y <= bmax[1] && 
			z >= bmin[2] && z <= bmax[2]) {

			int index = z*base_dim[0]*base_dim[1] + y*base_dim[0] + x;
			branch_nodes = tbranch->GetNumCells(reflevel);

			float *data = _treeData[index];

			size_t start[] = {n,0,0,0};
			size_t count[] = {branch_nodes,_cellDim[2],_cellDim[1],_cellDim[0]};

			rc = nc_get_vars_float(ncid, varid, start, count, NULL, data);

			if (rc != NC_NOERR) {
				SetErrMsg( "Error reading netCDF file : %s",  nc_strerror(rc) );
				return(-1);
			}
		}

		n += file_branch_nodes;
		
	}
	}
	}

	nc_close(ncid);

	return(0);
}

int AMRData::ReadNCDF(
	const string &path,
	int reflevel
) {
	return(ReadNCDF(path, _bmin, _bmax, reflevel));
}

float	*AMRData::GetBlock(
	AMRTree::cid_t	cellid
) const {

    AMRTreeBranch::cid_t baseblockidx;
    AMRTreeBranch::cid_t nodeidx;

	_tree->DecodeCellID(cellid, &baseblockidx, &nodeidx);

	const AMRTreeBranch	*tbranch = _tree->GetBranch(baseblockidx);

	long offset = tbranch->GetCellOffset(nodeidx);

	int	stride = _cellDim[0]*_cellDim[1]*_cellDim[2];

	return(&_treeData[baseblockidx][offset*stride]);

}

int	AMRData::ReGrid(
	const size_t bmin[3],
	const size_t bmax[3],
	int reflevel,
	float *grid,
	const size_t dim[3]
) const {

	if (reflevel < 0) reflevel = _maxRefinementLevel;

	if (reflevel > _maxRefinementLevel) {
		SetErrMsg("Invalid refinement level %d", reflevel);
		return(-1);
	}

	size_t bminb[3];
	size_t bmaxb[3];
	const size_t *bdim = _tree->GetBaseDim();

	for(int i = 0; i<3; i++) {
		bminb[i] = bmin[i] >> reflevel;
		bmaxb[i] = bmax[i] >> reflevel;

		if (bminb[i] > bmaxb[i]) {
			SetErrMsg("Invalid grid coordinates");
			return(-1);
		}
	}

	for (int z = bminb[2]; z<= bmaxb[2] && z<bdim[2]; z++) {
	for (int y = bminb[1]; y<= bmaxb[1] && y<bdim[1]; y++) {
	for (int x = bminb[0]; x<= bmaxb[0] && x<bdim[0]; x++) {

		regrid_branch(x,y,z, bmin, bmax, reflevel, grid, dim);

	}
	}
	}

	return(0);
	
}



int	AMRData::paramesh_copy_data(
    int index,
    int pid,
    const int paramesh_gids[][15],
	const float paramesh_unk[]
) {
	int	stride = _cellDim[0]*_cellDim[1]*_cellDim[2];
	float *dst = _treeData[index];

	// Vectors of paramesh cell ids (gids) used to perform a breath first
	// traversal of the tree.
	//
	vector <int> gidbuf0;
	vector <int> gidbuf1;
	vector <int> *gptr0 = &gidbuf0;
	vector <int> *gptr1 = &gidbuf1;
	vector <int> *gtmpptr = NULL;

	gptr0->push_back(pid);

	//
	// Assumes breath first data traversal
	//
	int level = 0;
	while ((*gptr0).size() && level <= _maxRefinementLevel) {
		int	gid;

		// Copy data for all cells at current level, while at the
		// same time building the list of child cells to be copied
		// during the next iteration of this loop.
		//
		for(int i=0; i<gptr0->size(); i++) {
			const float *src;

			gid = (*gptr0)[i];

			src = &paramesh_unk[gid*stride];

			for (int ii=0; ii<stride; ii++) {
				dst[ii] = src[ii];
				if (src[ii] < _dataRange[0]) _dataRange[0] = src[ii];
				if (src[ii] > _dataRange[1]) _dataRange[1] = src[ii];
			}
			// memcpy(dst, src, stride*sizeof(src[0]));
			dst += stride;

			if (paramesh_gids[gid][7] >= 0) {	// does the cell have children?

				// Push all the children of this cell onto the list
				// for subsequent processing. Note: ids referenced in
				// gids array are off by one (hence the -1).
				//
				for (int j=0; j<8; j++) {
					(*gptr1).push_back(paramesh_gids[gid][j+7] - 1);
				}
			}
		}

		// Swap pointers to buffers
		//
		gtmpptr = gptr0;
		gptr0 = gptr1;
		gptr1 = gtmpptr;
		(*gptr1).clear();

		level++;

	}

	return(0);
}

void AMRData::regrid_branch(
	unsigned int x,
	unsigned int y,
	unsigned int z, 
	const size_t min[3],
	const size_t max[3],
	int reflevel,
	float *grid,
	const size_t dim[3]
) const {

	const size_t xyz[3] = {x,y,z};
	size_t minl[3], maxl[3]; 	// bounds adjusted for current level

	const AMRTreeBranch	*tbranch = _tree->GetBranch(xyz);


	// Vectors of cell ids 
	//
	vector <AMRTreeBranch::cid_t> cellidbuf0;
	vector <AMRTreeBranch::cid_t> cellidbuf1;
	vector <AMRTreeBranch::cid_t> *cptr0 = &cellidbuf0;
	vector <AMRTreeBranch::cid_t> *cptr1 = &cellidbuf1;
	vector <AMRTreeBranch::cid_t> *ctmpptr = NULL;

	cptr0->push_back(0);	// Root cell;

	int	level = 0;
	while ((*cptr0).size()) {
		AMRTreeBranch::cid_t	cellid;

		// Process each cell at the current level, checking to see
		// if the cell has children, and if so, refining the cell
		//

		minl[0] = min[0] >> (reflevel - (level+1));
		minl[1] = min[1] >> (reflevel - (level+1));
		minl[2] = min[2] >> (reflevel - (level+1));
		maxl[0] = (max[0]+1) >> (reflevel - (level+1));
		maxl[1] = (max[1]+1) >> (reflevel - (level+1));
		maxl[2] = (max[2]+1) >> (reflevel - (level+1));
		for(int i=0; i<cptr0->size(); i++) {


			cellid = (*cptr0)[i];

			if (tbranch->HasChildren(cellid) && tbranch->GetCellLevel(cellid) < reflevel) {

				AMRTreeBranch::cid_t	child;

				child = tbranch->GetCellChildren(cellid);
				assert(child >= 0);

				// See which, if any, of the children overlap
				// the desired reigon.
				//
				for (int j=0; j<8; j++) {
					size_t xyz[3];
					int level;
					tbranch->GetCellLocation(child+j, xyz, &level);

					if ((xyz[0] >= minl[0]) && (xyz[0] <= maxl[0]) &&
						(xyz[1] >= minl[1]) && (xyz[1] <= maxl[1]) &&
						(xyz[2] >= minl[2]) && (xyz[2] <= maxl[2])) {

						// The child cell overlaps the region of interest
						//
						(*cptr1).push_back(child+j);
					}
				}
			}

			else  {
				const size_t *bdim = _tree->GetBaseDim();

				int index = (z * bdim[1] * bdim[0]) + (y*bdim[0]) + x;

				// Resample the cell
				//
				regrid_cell(
					tbranch, _treeData[index], cellid, 
					min, max, reflevel, grid, dim
				);
			}

		}
		// Swap pointers to buffers
		//
		ctmpptr = cptr0;
		cptr0 = cptr1;
		cptr1 = ctmpptr;
		(*cptr1).clear();

		level++;
	}
}

void AMRData::regrid_cell(
	const AMRTreeBranch	*tbranch,
	const float *branch_data,
	AMRTreeBranch::cid_t cellid,
	const size_t min[3],
	const size_t max[3],
	int reflevel,
	float *grid,
	const size_t dim[3]
) const {

	// Extents of grid covered by cell, specified in voxels relative 
	// to origin of the entire tree at the given refinement level
	//
	size_t gridminv[3];
	size_t gridmaxv[3];	

	// 	dimensions of grid in voxels
	//
//	int nx = _cellDim[0] * (max[0] - min[0] + 1);
//	int ny = _cellDim[1] * (max[1] - min[1] + 1);
	int nx = dim[0];
	int ny = dim[1];
	assert (nx >= _cellDim[0] * (max[0] - min[0] + 1));
	assert (ny >= _cellDim[1] * (max[1] - min[1] + 1));

	int	stride = _cellDim[0]*_cellDim[1]*_cellDim[2];
	const float *cell_data = &branch_data[cellid*stride];

	// location of cell in block coords at cell's refinment level
	size_t xyz[3];	
	int level;
	tbranch->GetCellLocation(cellid, xyz, &level);

	// difference between cell's refinement level and grid's
	int	ldelta = reflevel - level;

	// Map the cell's location coordinates to the grid's, which may
	// exist at a higher refinement level
	//
	xyz[0] = xyz[0] << ldelta;
	xyz[1] = xyz[1] << ldelta;
	xyz[2] = xyz[2] << ldelta;

	//
	// Convert coordinates from blocks to voxels
	//
	for(int i=0; i<3; i++) {

		if (xyz[i] > min[i]) {
			gridminv[i] = (xyz[i] - min[i]) * _cellDim[i];
		}
		else {
			gridminv[i] = 0;
		}

		if ((xyz[i] + (1<<ldelta)) < max[i] + 1) {
			gridmaxv[i] = gridminv[i] + (1<<ldelta) * _cellDim[i] - 1;
		}
		else {
			gridmaxv[i] = (max[i]-min[i]+1) * _cellDim[i] - 1;
		}
	}

	int cx, cy, cz;	
	int srcidx, dstidx;

	int gx, gy, gz;

	// Handle non resampling case where cell resolution matches grid
	// resolution
	//


	if (ldelta == 0) {
		// Coordinates of first voxel in cell to be resampled, specified
		// relative to the refinment level of the cell
		//
		int cxstart = 0;
		int cystart = 0;
		int czstart = 0;

		for (gz=gridminv[2], cz=czstart; gz<=gridmaxv[2]; gz++, cz++) {
		for (gy=gridminv[1], cy=cystart; gy<=gridmaxv[1]; gy++, cy++) {
		for (gx=gridminv[0], cx=cxstart; gx<=gridmaxv[0]; gx++, cx++) {

			srcidx = cz * _cellDim[1]*_cellDim[0] + cy*_cellDim[0] + cx;
			dstidx = gz * ny*nx + gy*nx+ gx;

			grid[dstidx] = cell_data[srcidx];

		}
		}
		}

		return;
	}

	// compute step size between samples
	//
	double eps = 0.000001;
	double xdelta = (double) (_cellDim[0] - 1) / (double) ((_cellDim[0] << ldelta) - 1) - eps;
	double ydelta = (double) (_cellDim[1] - 1) / (double) ((_cellDim[1] << ldelta) - 1) - eps;
	double zdelta = (double) (_cellDim[2] - 1) / (double) ((_cellDim[2] << ldelta) - 1) - eps;

	// Position of the first sample, in real coordinates, relative to
	// the cell's origin.
	//
	double xstart = 0;
	double ystart = 0;
	double zstart = 0;

	if (xyz[0] > min[0]) xstart = 0.0;
	else xstart = xdelta * ((min[0]-xyz[0])*_cellDim[0]);

	if (xyz[1] > min[1]) ystart = 0.0;
	else ystart = ydelta * ((min[1]-xyz[1])*_cellDim[1]);

	if (xyz[2] > min[2]) zstart = 0.0;
	else zstart = zdelta * ((min[2]-xyz[2])*_cellDim[2]);

	double xw0, xw1, yw0, yw1, zw0, zw1;
	double xx, yy, zz;


	for (zz=zstart,gz=gridminv[2]; gz<=gridmaxv[2]; gz++,zz+=zdelta) { 
		cz = (int) zz;
		zw0 = 1.0 - (zz - cz);
		zw1 = zz - cz;

		for (yy=ystart,gy=gridminv[1]; gy<=gridmaxv[1]; gy++,yy+=ydelta) { 
			cy = (int) yy;
			yw0 = 1.0 - (yy - cy);
			yw1 = yy - cy;

			for (xx=xstart,gx=gridminv[0]; gx<=gridmaxv[0]; gx++,xx+=ydelta) { 
				cx = (int) xx;

				xw0 = 1.0 - (xx - cx);
				xw1 = xx - cx;

				srcidx = cz * _cellDim[1]*_cellDim[0] + cy*_cellDim[0] + cx;
				dstidx = gz * ny*nx + gy*nx+ gx;

		double p0 = cell_data[srcidx];
		double p1 = cell_data[srcidx+1];
		double p2 = cell_data[srcidx+_cellDim[0]];
		double p3 = cell_data[srcidx+_cellDim[0]+1];
		double p4 = cell_data[srcidx+(_cellDim[0]*_cellDim[1])];
		double p5 = cell_data[srcidx+(_cellDim[0]*_cellDim[1])+1];
		double p6 = cell_data[srcidx+(_cellDim[0]*_cellDim[1])+_cellDim[0]];
		double p7 = cell_data[srcidx+(_cellDim[0]*_cellDim[1])+_cellDim[0]+1];

				grid[dstidx] = (zw0 * (yw0 * (xw0 * p0 + xw1 * p1) + 
					yw1 * (xw0 * p2 + xw1 * p3))) + 
					(zw1 * (yw0 * (xw0 * p4 + xw1 * p5) + 
					yw1 * (xw0 * p6 + xw1 * p7)));
			}
		
		}
	}
}
