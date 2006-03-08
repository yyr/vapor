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
#include "vaporinternal/common.h"
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
const string AMRData::_cellDimToken = "CellDimensions";
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
	const size_t min[3],
	const size_t max[3],
	int reflevel 
) {
	_treeData = NULL;
	_treeDataMemSize = NULL;
	_tree = NULL;

	if ((cell_dim[0] != cell_dim[1]) || (cell_dim[0] != cell_dim[2])) {
        SetErrMsg("Cell dimensions must all be equal");
		return;
	}
		
    if (! IsPowerOfTwo(cell_dim[0])) {
        SetErrMsg("Block dimension is not a power of two");
        return;
    }

	_dataRange[0] = _dataRange[1] = 0.0;

	const size_t *bdim = tree->GetBaseDim();

	for(int i=0; i<3; i++) {
		if ((min[i] >= bdim[i]) || (max[i] >= bdim[i]) || (min[i]>max[i])){
			SetErrMsg("Block coordinates invalid");
			return;
		}
    }

	if (reflevel<0 || reflevel>tree->GetRefinementLevel(min,max)) {
		reflevel = tree->GetRefinementLevel(min, max);
	}


	for(int i=0; i<3; i++) {
		_cellDim[i] = cell_dim[i];
		_bmin[i] = min[i];
		_bmax[i] = max[i];
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
		int nnodes = tbranch->GetNumNodes(_maxRefinementLevel);
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
	const size_t min[3],
	const size_t max[3],
	int reflevel
) {
	_AMRData(tree, cell_dim, min, max, reflevel);
}

AMRData::AMRData(
	const AMRTree *tree,
	const size_t cell_dim[3],
	int reflevel
) {

	const size_t *bdim = tree->GetBaseDim();

	size_t	min[3] = {0,0,0};
	size_t	max[3] = {(size_t) bdim[0]-1,(size_t) bdim[1]-1,(size_t) bdim[2]-1};

	_AMRData(tree, cell_dim, min, max, reflevel);
}

AMRData::AMRData(
	const AMRTree *tree,
	const size_t cell_dim[3],
	const int paramesh_gids[][15],
	const float paramesh_unk[],
	int	total_blocks,
	int reflevel
) {

	const size_t *bdim = tree->GetBaseDim();

	size_t	min[3] = {0,0,0};
	size_t	max[3] = {(size_t) bdim[0]-1,(size_t) bdim[1]-1,(size_t) bdim[2]-1};

	_AMRData(tree, cell_dim, min, max, reflevel);
	if (GetErrCode()) return;

	vector <int> baseblocks;

	baseblocks.reserve(bdim[0]*bdim[1]*bdim[2]);

	size_t dummy[3];

	if (tree->_parameshGetBaseBlocks(baseblocks,dummy,paramesh_gids,total_blocks)<0)return;

	_dataRange[0] = _dataRange[1] = paramesh_unk[0];
	for(int i =0; i<baseblocks.size(); i++) {
		if (paramesh_copy_data(i, baseblocks[i], paramesh_gids, paramesh_unk) < 0) {
			return;
		}
	}
}

AMRData::~AMRData() {

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
	const size_t min[3],
	const size_t max[3],
	int reflevel
) {
	const size_t *bdim = _tree->GetBaseDim();

	for(int i=0; i<3; i++) {
		if ((min[i] >= bdim[i]) || (max[i] >= bdim[i]) || (min[i]>max[i])){
			SetErrMsg("Block coordinates invalid");
			return(-1);
		}
    }

	if (reflevel<0 || reflevel>_tree->GetRefinementLevel(min,max)) {
		reflevel = _tree->GetRefinementLevel(min, max);
	}

	for(int i=0; i<3; i++) {
		_bmin[i] = min[i];
		_bmax[i] = max[i];
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
		int nnodes = tbranch->GetNumNodes(_maxRefinementLevel);

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
	const size_t **min,
	const size_t **max,
	int *reflevel
) const {
	*min = _bmin;
	*max = _bmax;
	*reflevel = _maxRefinementLevel;
}

//
// IO size for netCDF
//
#define NC_CHUNKSIZEHINT    1024*1024

int AMRData::WriteNCDF(
	const string &path,
	int reflevel
) const {

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

	AMRTree::CellID num_nodes_total = _tree->GetNumNodes(_bmin, _bmax, reflevel);

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

		branch_nodes = tbranch->GetNumNodes(reflevel);

		const float *data = _treeData[index];

		size_t start[] = {n,0,0,0};
        size_t count[] = {branch_nodes,_cellDim[2],_cellDim[1],_cellDim[0]};

        rc = nc_put_vars_float(
                ncid, varid, start, count, NULL, data
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

int AMRData::ReadNCDF(
	const string &path,
	int reflevel
) {

	int	ncid;
	int rc;

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
	// netCDF Dimensions
	//
	int ncdimid;
	size_t ncdim;
	size_t file_num_nodes;
	
	
	rc = nc_inq_dimid(ncid, _numNodeToken.c_str(), &ncdimid);
	NC_ERR_READ(rc,path)
	rc = nc_inq_dimlen(ncid, ncdimid, &file_num_nodes);
	NC_ERR_READ(rc,path)

	rc = nc_inq_dimid(ncid, _blockSizeXToken.c_str(), &ncdimid);
	NC_ERR_READ(rc,path)
	rc = nc_inq_dimlen(ncid, ncdimid, &ncdim);
	NC_ERR_READ(rc,path)
	if (ncdim != _cellDim[0]) {
		SetErrMsg("File/object mismatch");
		return(-1);
	}

	rc = nc_inq_dimid(ncid, _blockSizeYToken.c_str(), &ncdimid);
	NC_ERR_READ(rc,path)
	rc = nc_inq_dimlen(ncid, ncdimid, &ncdim);
	NC_ERR_READ(rc,path)
	if (ncdim != _cellDim[1]) {
		SetErrMsg("File/object mismatch");
		return(-1);
	}

	rc = nc_inq_dimid(ncid, _blockSizeZToken.c_str(), &ncdimid);
	NC_ERR_READ(rc,path)
	rc = nc_inq_dimlen(ncid, ncdimid, &ncdim);
	NC_ERR_READ(rc,path)
	if (ncdim != _cellDim[2]) {
		SetErrMsg("File/object mismatch");
		return(-1);
	}


	//
	// netCDF Attributes
	//
	int file_bmin[3];
	rc = nc_get_att_int(
		ncid,NC_GLOBAL,_blockMinToken.c_str(),file_bmin
	);
	NC_ERR_READ(rc,path)

	int file_bmax[3];
	rc = nc_get_att_int(
		ncid,NC_GLOBAL,_blockMaxToken.c_str(),file_bmax
	);
	NC_ERR_READ(rc,path)

	int file_reflevel;
	rc = nc_get_att_int(
		ncid,NC_GLOBAL,_refinementLevelToken.c_str(),&file_reflevel
	);
	NC_ERR_READ(rc,path)

	rc = nc_get_att_float(
		ncid,NC_GLOBAL,_scalarRangeToken.c_str(),_dataRange
	);
	NC_ERR_READ(rc,path)

	//
	// netCDF Variables
	//
	int	varid;
	rc = nc_inq_varid(ncid, _varToken.c_str(), &varid);
	NC_ERR_READ(rc,path)


	//
	// Make sure the file values are reasonable
	//
	const size_t *base_dim = _tree->GetBaseDim();
	for (int i=0; i<3; i++) {
		if ((file_bmin[i] > base_dim[i]-1) ||
			(file_bmax[i] > base_dim[i]-1) ||
			(file_bmin[i] > file_bmax[i]-1)) {

			SetErrMsg("Data file doesn't match AMR tree");
			return(-1);
		}
	}

	for(int i=0; i<3; i++) {
		if ((_bmin[i] < file_bmin[i]) ||
			(_bmax[i] > file_bmax[i])) {

			SetErrMsg("Requested region not present in file");
			return(-1);
		}
	}

	if (_maxRefinementLevel > file_reflevel) {
		SetErrMsg("Requested refinement level not present in file");
		return(-1);
	}

	int branch_nodes = 0;	// Num nodes requested
	int file_branch_nodes = 0;	// Num nodes stored in file
	int n = 0;
	for (int z=_bmin[2]; z<=_bmax[2]; z++) {
	for (int y=_bmin[1]; y<=_bmax[1]; y++) {
	for (int x=_bmin[0]; x<=_bmax[0]; x++) {
		size_t xyz[3] = {x,y,z};
		int index = z*base_dim[0]*base_dim[1] + y*base_dim[0] + x;

		assert(n<file_num_nodes);

		const AMRTreeBranch *tbranch = _tree->GetBranch(xyz);

		file_branch_nodes = tbranch->GetNumNodes(file_reflevel);
		branch_nodes = tbranch->GetNumNodes(reflevel);

		float *data = _treeData[index];

        size_t start[] = {n,0,0,0};
        size_t count[] = {branch_nodes,_cellDim[2],_cellDim[1],_cellDim[0]};

        rc = nc_get_vars_float(ncid, varid, start, count, NULL, data);

        if (rc != NC_NOERR) {
            SetErrMsg( "Error reading netCDF file : %s",  nc_strerror(rc) );
            return(-1);
        }

		n += file_branch_nodes;
		
	}
	}
	}

	nc_close(ncid);

	return(0);

}

const float	*AMRData::GetBlock(
	AMRTree::CellID	cellid
) const {

    AMRTreeBranch::UInt32 baseblockidx;
    AMRTreeBranch::UInt32 nodeidx;

	_tree->DecodeCellID(cellid, &baseblockidx, &nodeidx);

	int	stride = _cellDim[0]*_cellDim[1]*_cellDim[2];

	return(&_treeData[baseblockidx][nodeidx*stride]);

}

int	AMRData::ReGrid(
	const size_t min[3],
	const size_t max[3],
	int reflevel,
	float *grid
) const {

	if (reflevel < 0) reflevel = _maxRefinementLevel;

	if (reflevel > _maxRefinementLevel) {
		SetErrMsg("Invalid refinement level %d", reflevel);
		return(-1);
	}

	size_t minb[3];
	size_t maxb[3];
	const size_t *bdim = _tree->GetBaseDim();

	for(int i = 0; i<3; i++) {
		minb[i] = min[i] >> reflevel;
		maxb[i] = max[i] >> reflevel;

		if (minb[i] > maxb[i] || maxb[i] >= bdim[i]) {
			SetErrMsg("Invalid grid coordinates");
			return(-1);
		}
	}

	for (int z = minb[2]; z<= maxb[2]; z++) {
	for (int y = minb[1]; y<= maxb[1]; y++) {
	for (int x = minb[0]; x<= maxb[0]; x++) {

		regrid_branch(x,y,z, min, max, reflevel, grid);

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
				if (src[ii] > _dataRange[0]) _dataRange[1] = src[ii];
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
	float *grid
) const {

	const size_t xyz[3] = {x,y,z};

	const AMRTreeBranch	*tbranch = _tree->GetBranch(xyz);


	// Vectors of cell ids 
	//
	vector <AMRTreeBranch::UInt32> cellidbuf0;
	vector <AMRTreeBranch::UInt32> cellidbuf1;
	vector <AMRTreeBranch::UInt32> *cptr0 = &cellidbuf0;
	vector <AMRTreeBranch::UInt32> *cptr1 = &cellidbuf1;
	vector <AMRTreeBranch::UInt32> *ctmpptr = NULL;

	cptr0->push_back(0);	// Root cell;

	int	level = 0;
	while ((*cptr0).size()) {
		AMRTreeBranch::UInt32	cellid;

		// Process each cell at the current level, checking to see
		// if the cell has children, and if so, refining the cell
		//
		for(int i=0; i<cptr0->size(); i++) {


			cellid = (*cptr0)[i];

			if (tbranch->HasChildren(cellid) && tbranch->GetCellLevel(cellid) < reflevel) {

				AMRTreeBranch::UInt32	child;

				child = tbranch->GetCellChildren(cellid);
				assert(child != AMRTreeBranch::AMR_ERROR);

				// See which, if any, of the children overlap
				// the desired reigon.
				//
				for (int j=0; j<8; j++) {
					unsigned int xyz[3];
					tbranch->GetCellLocation(child+j, xyz);

					xyz[0] = xyz[0] << (reflevel - (level+1));
					xyz[1] = xyz[1] << (reflevel - (level+1));
					xyz[2] = xyz[2] << (reflevel - (level+1));

		
					if ((xyz[0] >= min[0]) && (xyz[0] <= max[0]) &&
						(xyz[1] >= min[1]) && (xyz[1] <= max[1]) &&
						(xyz[2] >= min[2]) && (xyz[2] <= max[2])) {

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
					min, max, reflevel, grid
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
	AMRTreeBranch::UInt32 cellid,
	const size_t min[3],
	const size_t max[3],
	int reflevel,
	float *grid
) const {

	// Extents of grid covered by cell, specified in voxels relative 
	// to origin of the entire tree at the given refinement level
	//
	size_t gridminv[3];
	size_t gridmaxv[3];	

	// 	dimensions of grid in voxels
	//
	int nx = _cellDim[0] * (max[0] - min[0] + 1);
	int ny = _cellDim[1] * (max[1] - min[1] + 1);

	int	stride = _cellDim[0]*_cellDim[1]*_cellDim[2];
	const float *cell_data = &branch_data[cellid*stride];

	// location of cell in block coords at cell's refinment level
	unsigned int xyz[3];	
	tbranch->GetCellLocation(cellid, xyz);

	int level = tbranch->GetCellLevel(cellid);

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

		// first and last voxel covered by cell (in grid coordinates)
		int cfirst = xyz[i] * _cellDim[i];
		int clast = (xyz[i] + (1 << ldelta)) * _cellDim[i] - 1;

		gridminv[i] = min[i] * _cellDim[i];
		gridmaxv[i] = (max[i]+1) * _cellDim[i] - 1;

		// Constrain resampling grid to cell bounds
		//
		if (gridminv[i] < cfirst) gridminv[i] = cfirst;

		if (gridmaxv[i] > clast) gridmaxv[i] = clast;
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
		int cxstart = (gridminv[0] - (xyz[0] * _cellDim[0])) >> ldelta;
		int cystart = (gridminv[1] - (xyz[1] * _cellDim[1])) >> ldelta;
		int czstart = (gridminv[2] - (xyz[2] * _cellDim[2])) >> ldelta;

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
	double xstart = xdelta * (gridminv[0] - (xyz[0] * _cellDim[0]));
	double ystart = ydelta * (gridminv[1] - (xyz[1] * _cellDim[1]));
	double zstart = zdelta * (gridminv[2] - (xyz[2] * _cellDim[2]));

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
