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
//	File:		AMRTree.cpp
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Thu Jan 5 16:59:07 MST 2006
//
//	Description:	
//
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <vapor/AMRTree.h>
#include <algorithm>

using namespace VAPoR;
using namespace VetsUtil;

const string AMRTree::_rootTag = "AMRTree";
const string AMRTree::_minExtentsAttr = "MinExtents";
const string AMRTree::_maxExtentsAttr = "MaxExtents";
const string AMRTree::_baseDimAttr = "BaseDimension";
const string AMRTree::_fileVersionAttr = "FileVersion";

const int BitsPerByte = 8;


// gets the right-adjusted N bits of quantity TARG
// starting from bit position POSS
//
#define GETBITS(TARG,POSS,N) (((TARG) >> ((POSS)+1-(N))) & ~(~0ULL << (N)))

// set N bits of quantity TARG starting from position
// POSS to the right-most N bits in integer SRC
//
#define PUTBITS(TARG, POSS, N, SRC) \
    (TARG) &= ~(~((~0ULL) << (N)) << (((POSS)+1) - (N))); \
    (TARG) |= (((SRC) & ~((~0ULL) << (N))) << (((POSS)+1) - (N)))

int AMRTree::_AMRTree(
	const size_t basedim[3], 
	const double min[3], 
	const double max[3]
) {

	// Compute # bits used to encode branch and base cellids
	//
	size_t idsz = sizeof(cid_t);
	assert(sizeof(size_t) >= idsz);
	size_t t = basedim[0]*basedim[1]*basedim[2];
	_tbits = 0;
	while (t>0) {
		t = t>>1;
		_tbits++;
	}
	_tbbits = (idsz * BitsPerByte)-_tbits-1; // # bits available for branch id

	_treeBranches = NULL;
	_x_index = 0;

	for(int i=0; i<3; i++) {
		_baseDim[i] = basedim[i];
		_minBounds[i] = min[i];
		_maxBounds[i] = max[i];
	}

	ostringstream oss;
	string empty;
	map <string, string> attrs;

	oss.str(empty);
	oss << _baseDim[0] << " " << _baseDim[1] << " " << _baseDim[2];
	attrs[_baseDimAttr] = oss.str();

	oss.str(empty);
	oss << _minBounds[0] << " " << _minBounds[1] << " " << _minBounds[2];
	attrs[_minExtentsAttr] = oss.str();

	oss.str(empty);
	oss << _maxBounds[0] << " " << _maxBounds[1] << " " << _maxBounds[2];
	attrs[_maxExtentsAttr] = oss.str();

	oss.str(empty);
	oss << 1;
	attrs[_fileVersionAttr] = oss.str();

	_rootNode = new XmlNode(_rootTag, attrs);

	_treeBranches = new AMRTreeBranch*[_baseDim[0] * _baseDim[1] * _baseDim[2]];

	double delta[3];
	for(int i = 0; i<3; i++) {
		delta[i] = (_maxBounds[i] - _minBounds[i]) / (double) _baseDim[i];
	}

	for (int z = 0; z < _baseDim[2]; z++) {
	for (int y = 0; y < _baseDim[1]; y++) {
	for (int x = 0; x < _baseDim[0]; x++) {
		size_t	index = (_baseDim[1] * _baseDim[0] * z) + (_baseDim[0] * y) + x;
		double bmin[3];
		double bmax[3];
		size_t location[3];
		
		bmin[0] = x * delta[0] + _minBounds[0];
		bmax[0] = (x+1) * delta[0] + _minBounds[0];
		bmin[1] = y * delta[1] + _minBounds[1];
		bmax[1] = (y+1) * delta[1] + _minBounds[1];
		bmin[2] = z * delta[2] + _minBounds[2];
		bmax[2] = (z+1) * delta[2] + _minBounds[2];

		location[0] = x;
		location[1] = y;
		location[2] = z;


		_treeBranches[index] = new AMRTreeBranch(_rootNode, bmin, bmax, location);

		
	}
	}
	}

	return(0);
}

AMRTree::AMRTree(
	const size_t basedim[3], 
	const double min[3], 
	const double max[3]
) {
    SetDiagMsg(
        "AMRTree::AMRTree([%d,%d,%d], [%f,%f,%f],[%f,%f,%f])",
        basedim[0],basedim[1],basedim[2], min[0],min[1],min[2], 
		max[0],max[1],max[2]
    );
	_objIsInitialized = 0;

	if (_AMRTree(basedim, min, max) < 0) return;
	_objIsInitialized = 1;
}

AMRTree::AMRTree(
) {
    SetDiagMsg("AMRTree::AMRTree()");

	_objIsInitialized = 0;
	size_t basedim[] = {1,1,1};
	double min[] = {0.0, 0.0, 0.0};
	double max[] = {1.0, 1.0, 1.0};

	if (_AMRTree(basedim, min, max) < 0) return;

	_objIsInitialized = 1;
}

AMRTree::AMRTree(
	const string &path
) {
	SetDiagMsg("AMRTree::AMRTree(%s)", path.c_str());

	_objIsInitialized = 0;

	ifstream is;

	_rootNode = NULL;

	is.open(path.c_str());
	if (! is) {
		SetErrMsg("Can't open file \"%s\" for reading", path.c_str());
		return;
	}

	ExpatParseMgr* parseMgr = new ExpatParseMgr(this);
	parseMgr->parse(is);
	delete parseMgr;

	_objIsInitialized = 1;
}

AMRTree::AMRTree(
	const size_t basedim[3], 
	const int	paramesh_gids[][15],
	const float paramesh_bboxs [][3][2],
	const int paramesh_refine_levels[],
	int total_blocks
) {
    SetDiagMsg( "AMRTree::AMRTree(,,,%d)", total_blocks);

	_objIsInitialized = 0;

	vector <int> baseblocks;


	// Compute the dimension of the volue in base blocks and identify all of
	// the base blocks
	//
	if (_parameshGetBaseBlocks(
		baseblocks,basedim,paramesh_gids,paramesh_bboxs, total_blocks
		)<0)return;

	double min[3];
	double max[3];

	min[0] = paramesh_bboxs[baseblocks[0]][0][0];
	min[1] = paramesh_bboxs[baseblocks[0]][1][0];
	min[2] = paramesh_bboxs[baseblocks[0]][2][0];

	max[0] = paramesh_bboxs[baseblocks[(basedim[0]*basedim[1]*basedim[2])-1]][0][1];
	max[1] = paramesh_bboxs[baseblocks[(basedim[0]*basedim[1]*basedim[2])-1]][1][1];
	max[2] = paramesh_bboxs[baseblocks[(basedim[0]*basedim[1]*basedim[2])-1]][2][1];


	assert(total_blocks > 0);
	int lmin, lmax;
	lmin = lmax = paramesh_refine_levels[0];
	for(int i =0; i<total_blocks; i++) {

		if (paramesh_refine_levels[i] < lmin) lmin = paramesh_refine_levels[i];
		if (paramesh_refine_levels[i] > lmax) lmax = paramesh_refine_levels[i];
	}

	if (_AMRTree(basedim, min, max) < 0) return;

	for(int i =0; i<baseblocks.size(); i++) {
		if (paramesh_refine_baseblocks(i, baseblocks[i], paramesh_gids) < 0) return;
	}

	_objIsInitialized = 1;
}

void AMRTree::_freeAMRTree() {

	if (_treeBranches) {
		for (int i=0; i<_baseDim[0] * _baseDim[1] * _baseDim[2]; i++) {
			if (_treeBranches[i]) delete _treeBranches[i];
		}
		delete [] _treeBranches;
		_treeBranches = NULL;
	}

	if (_rootNode) delete _rootNode;
	_rootNode = NULL;

}

AMRTree::~AMRTree() {

    SetDiagMsg( "AMRTree::~AMRTree()");

	_freeAMRTree();

	_objIsInitialized = 0;
}

void   AMRTree::DecodeCellID(
    AMRTree::cid_t cellid,
    AMRTree::cid_t *baseblockidx,
    AMRTree::cid_t *nodeidx
) const {

    SetDiagMsg( "AMRTree::DecodeCellID(%lld,,)", cellid);

	*nodeidx = GETBITS((size_t) cellid,_tbbits - 1,_tbbits);
	*baseblockidx = GETBITS((size_t) cellid,_tbits + _tbbits - 1,_tbits);
}

int	AMRTree::DeleteCell(AMRTree::cid_t cellid) {
    SetDiagMsg( "AMRTree::DeleteCell(%lld)", cellid);

	SetErrMsg("This function is not implemented");
	return(-1);
}

AMRTree::cid_t	AMRTree::FindCell(
	const double ucoord[3], int reflevel
) const {

    SetDiagMsg(
		"AMRTree::DeleteCell([%f,%f,%f], %d)", 
		ucoord[0], ucoord[1], ucoord[2], reflevel
	);

	assert(_treeBranches != NULL);


	AMRTree::cid_t tbid, index, cellid;

	for(int i =0; i<3; i++) {
		if (ucoord[i] < _minBounds[i] || ucoord[i] > _maxBounds[i]) {
			SetErrMsg("Point not contained in this tree branch\n");
			return(-1);
		}
	}

	double delta[3];
	for(int i = 0; i<3; i++) {
		delta[i] = (_maxBounds[i] - _minBounds[i]) / (double) _baseDim[i];
	}

	int x = (int) ((ucoord[0] - _minBounds[0]) / delta[0]);
	int y = (int) ((ucoord[1] - _minBounds[1]) / delta[1]);
	int z = (int) ((ucoord[2] - _minBounds[2]) / delta[2]);

	index = (_baseDim[1] * _baseDim[0] * z) + (_baseDim[0] * y) + x;

	tbid = _treeBranches[index]->FindCell(ucoord, reflevel);
	if (tbid < 0) return(-1);

	_encode_cellid(index, tbid, &cellid);

	return(cellid);
	
}

int	AMRTree::GetCellLocation(
	AMRTree::cid_t cellid, size_t xyz[3], int *reflevel
) const {

	AMRTree::cid_t	index;
	AMRTree::cid_t	tbid;
	DecodeCellID(cellid, &index, &tbid);

	if (index >= _baseDim[0] * _baseDim[1] * _baseDim[2]) {
		SetErrMsg("Invalid cell id : %lld\n", cellid);
		return(-1);
	}

	return(_treeBranches[index]->GetCellLocation(tbid, xyz, reflevel));
	
}

AMRTree::cid_t	AMRTree::GetCellID(
	const size_t xyz[3], int reflevel
) const {

	if (reflevel < 0) reflevel = GetRefinementLevel();

	if (reflevel > GetRefinementLevel()) {
		SetErrMsg("Invalid cell refinement level : %d\n", reflevel);
		return(-1);
	}

	size_t x = xyz[0];
	size_t y = xyz[1];
	size_t z = xyz[2];

    for (int l=0; l<reflevel; l++) {
		x = x >> 1;
		y = y >> 1;
		z = z >> 1;
    }

	AMRTree::cid_t  index;
	AMRTree::cid_t	tbid;
	index = (_baseDim[1] * _baseDim[0] * z) + (_baseDim[0] * y) + x;
	if (index >= _baseDim[0] * _baseDim[1] * _baseDim[2]) {
		SetErrMsg("Invalid location (%d %d %d) : ", xyz[0], xyz[1], xyz[2]);
		return(-1);
	}

	tbid = _treeBranches[index]->GetCellID(xyz, reflevel);
	if (tbid < 0) return(-1);

	AMRTree::cid_t	cellid;
	_encode_cellid(index, tbid, &cellid);
	return(cellid);
}



int    AMRTree::GetCellBounds(
    cid_t cellid, double minu[3], double maxu[3]
 ) const {

    SetDiagMsg( "AMRTree::GetCellBounds(%lld,,)", cellid);
	assert(_treeBranches != NULL);


	AMRTree::cid_t	index;
	AMRTree::cid_t	tbid;
	DecodeCellID(cellid, &index, &tbid);

	if (index >= _baseDim[0] * _baseDim[1] * _baseDim[2]) {
		SetErrMsg("Invalid cell id : %lld\n", cellid);
		return(-1);
	}

	return(_treeBranches[index]->GetCellBounds(tbid, minu, maxu));

}

const AMRTreeBranch    *AMRTree::GetBranch(const size_t xyz[3]) const {

    SetDiagMsg( "AMRTree::GetBranch([%u,%u,%u])", xyz[0],xyz[1],xyz[2]);


	int	index = (_baseDim[1] * _baseDim[0] * xyz[2]) + (_baseDim[0] * xyz[1]) + xyz[0];

	if (index >= (_baseDim[0] * _baseDim[1] * _baseDim[2])) return(NULL);
	return(_treeBranches[index]);
}

const AMRTreeBranch    *AMRTree::GetBranch(cid_t index) const {

    SetDiagMsg( "AMRTree::GetBranch(%d)", index);
	if (index >= (_baseDim[0] * _baseDim[1] * _baseDim[2])) return(NULL);

	return(_treeBranches[index]);
}


AMRTree::cid_t	AMRTree::GetCellChildren(
	AMRTree::cid_t cellid
) const {

    SetDiagMsg( "AMRTree::GetCellChildren(%lld)", cellid);

	assert(_treeBranches != NULL);


	AMRTree::cid_t	index;
	AMRTree::cid_t	tbid;

	DecodeCellID(cellid, &index, &tbid);

	if (index >= _baseDim[0] * _baseDim[1] * _baseDim[2]) {
		SetErrMsg("Invalid cell id : %lld\n", cellid);
		return(-1);
	}

	AMRTree::cid_t child_tbid = _treeBranches[index]->GetCellChildren(tbid);
	if (child_tbid < 0) return(-1);

	_encode_cellid(index, child_tbid, &cellid);

	return(cellid);

}

int	AMRTree::GetCellLevel(AMRTree::cid_t cellid) const {

    SetDiagMsg( "AMRTree::GetCellLevel(%lld)", cellid);

	assert(_treeBranches != NULL);


	AMRTree::cid_t	index;
	AMRTree::cid_t	tbid;

	DecodeCellID(cellid, &index, &tbid);

	if (index >= _baseDim[0] * _baseDim[1] * _baseDim[2]) {
		SetErrMsg("Invalid cell id : %lld\n", cellid);
		return(-1);
	}

	int level = _treeBranches[index]->GetCellLevel(tbid);
	if (level < 0) return(-1);

	return(level);

}


AMRTree::cid_t	AMRTree::GetCellNeighbor(AMRTree::cid_t cellid, int face) const {

    SetDiagMsg( "AMRTree::GetCellNeighbor(%lld, %d)", cellid, face);

	SetErrMsg("This function not implemented yet");
	return(-1);
}

int AMRTree::GetRefinementLevel(
	const size_t min[3],
	const size_t max[3]
) const {

    SetDiagMsg( "AMRTree::GetRefinementLevel([%u,%u,%u], [%u,%u,%u])",
		min[0], min[1], min[2], max[0], max[1], max[2]
	);

	assert(_treeBranches != NULL);


	for (int i = 0; i<3; i++) {
		if (min[i] > max[i] || max[i] >= _baseDim[i]) {
			SetErrMsg("Invalid coordinates");
			return(-1);
		}
	}

	int	maxlevel = 0;
	for (int z = min[2]; z <= max[2]; z++) {
	for (int y = min[1]; y <= max[1]; y++) {
	for (int x = min[0]; x <= max[0]; x++) {
		int	index = (_baseDim[1] * _baseDim[0] * z) + (_baseDim[0] * y) + x;

		if (_treeBranches[index]->GetRefinementLevel() > maxlevel) {
			maxlevel = _treeBranches[index]->GetRefinementLevel();
		}
		
	}
	}
	}

	return(maxlevel);
}

int	AMRTree::GetRefinementLevel() const {

    SetDiagMsg( "AMRTree::GetRefinementLevel()");

	assert(_treeBranches != NULL);


	size_t min[3] = {0,0,0};
	size_t max[3] = {_baseDim[0]-1, _baseDim[1]-1, _baseDim[2]-1};

	return(GetRefinementLevel(min,max));
}


AMRTree::cid_t AMRTree::GetNumCells(
    const size_t min [3],
    const size_t max[3],
    int reflevel
) const {

    SetDiagMsg(
		"AMRTree::GetNumCells([%u,%u,%u], [%u,%u,%u], %d)",
		min[0], min[1], min[2], max[0], max[1], max[2], reflevel
	);

	assert(_treeBranches != NULL);


	for (int i = 0; i<3; i++) {
		if (min[i] > max[i] || max[i] >= _baseDim[i]) {
			SetErrMsg("Invalid coordinates");
			return(-1);
		}
	}

	AMRTree::cid_t nnodes = 0;
	for (int z = min[2]; z <= max[2]; z++) {
	for (int y = min[1]; y <= max[1]; y++) {
	for (int x = min[0]; x <= max[0]; x++) {

		int	index = (_baseDim[1] * _baseDim[0] * z) + (_baseDim[0] * y) + x;
		nnodes += _treeBranches[index]->GetNumCells(reflevel);
		
	}
	}
	}

	return(nnodes);
}

AMRTree::cid_t AMRTree::GetNumCells(
    int reflevel
) const {

    SetDiagMsg( "AMRTree::GetNumCells(%d)", reflevel);

	size_t min[3] = {0,0,0};
	size_t max[3] = {_baseDim[0]-1, _baseDim[1]-1, _baseDim[2]-1};

	return(GetNumCells(min, max, reflevel));
}




AMRTree::cid_t	AMRTree::GetCellParent(AMRTree::cid_t cellid) const
{
    SetDiagMsg( "AMRTree::GetCellParent(%lld)", cellid);
	assert(_treeBranches != NULL);


	AMRTree::cid_t	index;
	AMRTree::cid_t	tbid;

	DecodeCellID(cellid, &index, &tbid);

	if (index >= _baseDim[0] * _baseDim[1] * _baseDim[2]) {
		SetErrMsg("Invalid cell id : %lld\n", cellid);
		return(-1);
	}

	AMRTree::cid_t parent_tbid = _treeBranches[index]->GetCellParent(tbid);
	if (parent_tbid < 0) return(-1);

	_encode_cellid(index, parent_tbid, &cellid);

	return(cellid);

}

AMRTree::cid_t	AMRTree::RefineCell(AMRTree::cid_t cellid) {

    SetDiagMsg( "AMRTree::RefineCell(%lld)", cellid);

	assert(_treeBranches != NULL);


	AMRTree::cid_t	index;
	AMRTree::cid_t	tbid;

	DecodeCellID(cellid, &index, &tbid);

	if (index >= _baseDim[0] * _baseDim[1] * _baseDim[2]) {
		SetErrMsg("Invalid cell id : %lld\n", cellid);
		return(-1);
	}

	int child_tbid = _treeBranches[index]->RefineCell(tbid);
	if (child_tbid < 0) return(-1);

	_encode_cellid(index, child_tbid, &cellid);

	return(cellid);

}

AMRTree::cid_t	AMRTree::GetNextCell(bool restart) {

    SetDiagMsg( "AMRTree::GetNextCell(%d)", restart);

	assert(_treeBranches != NULL);

	cid_t	cellid;
	cid_t	tbid;
	bool tb_restart = false;	// tree branch restart;

	if (restart) {
		_x_index = 0;
		tb_restart = true;
	}

	while (1) {

		if (_x_index >= _baseDim[0] * _baseDim[1] * _baseDim[2]) return(-1);

		tbid = _treeBranches[_x_index]->GetNextCell(tb_restart);

		if (tbid >= 0) {
			_encode_cellid(_x_index, tbid, &cellid);
			return(cellid);	// we're done
		}

		tb_restart = false;
		if (tbid < 0) { 
			_x_index++;
			tb_restart = true;
		}
	}

	return(-1); // should never get here
}


int	AMRTree::MapUserToVoxel(
	int reflevel, const double ucoord[3], size_t vcoord[3]
) {
    SetDiagMsg(
		"AMRTree::MapUserToVoxel([%f,%f,%f],[],%d)", 
		ucoord[0], ucoord[1], ucoord[2], reflevel
	);

	SetErrMsg("This funciton is not implemented");
	return(-1);
}

int	AMRTree::Write(
	const string &path
) const {
    SetDiagMsg("AMRTree::Write(%s)", path.c_str());

	for (int z = 0; z < _baseDim[2]; z++) {
	for (int y = 0; y < _baseDim[1]; y++) {
	for (int x = 0; x < _baseDim[0]; x++) {
		int	index = (_baseDim[1] * _baseDim[0] * z) + (_baseDim[0] * y) + x;

		_treeBranches[index]->Update();
	}
	}
	}


    ofstream fileout;
    fileout.open(path.c_str());
    if (! fileout) {
        SetErrMsg("Can't open file \"%s\" for writing", path.c_str());
        return(-1);
    }
    fileout << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>" << endl;
    fileout << *_rootNode;

    return(0);

}

int	AMRTree::Read(
	const string &path
) {
	ifstream is;

	is.open(path.c_str());
	if (! is) {
		SetErrMsg("Can't open file \"%s\" for reading", path.c_str());
		return(-1);
	}

	ExpatParseMgr* parseMgr = new ExpatParseMgr(this);
	parseMgr->parse(is);
	delete parseMgr;

	if (ExpatParseMgr::GetErrCode() != 0) return(-1);

	return(0);
}

typedef struct {
	int gid;
	float x;
	float y;
	float z;
} tmp_struct_t;

bool block_cmp(tmp_struct_t a, tmp_struct_t b) {
	if (a.z < b.z) return (true);
	else if (a.z == b.z && a.y < b.y) return (true);
	else if (a.z == b.z && a.y == b.y && a.x < b.x) return (true);

	return(false);
}

int	AMRTree::_parameshGetBaseBlocks(
	vector <int> &baseblocks,
	const size_t basedim[3],
	const int	gids[][15],
	const float paramesh_bboxs [][3][2],
	int	totalblocks
) const {
	vector <int> tmpbb;	// base blocks

	// base blocks are ones with no parents
	//
	for (int i=0; i<totalblocks; i++) {
		if (gids[i][6] < 0) {
			tmpbb.push_back(i);
		}
	}

	// Sanity check
	//
	if (tmpbb.size() != (basedim[0]*basedim[1]*basedim[2])) {
		SetErrMsg("Invalid Paramesh gids record");
		return(-1);
	}


	vector <tmp_struct_t> bblocks;
	for (size_t i=0; i<tmpbb.size(); i++) {
		tmp_struct_t bblock;
		bblock.gid = tmpbb[i];
		bblock.x = paramesh_bboxs[tmpbb[i]][0][0];
		bblock.y = paramesh_bboxs[tmpbb[i]][1][0];
		bblock.z = paramesh_bboxs[tmpbb[i]][2][0];
		bblocks.push_back(bblock);
	}

	// Sort base blocks so they're in X-Y-Z order
	//
	std::sort(bblocks.begin(), bblocks.end(), block_cmp); 

	baseblocks.clear();
	for (size_t i=0; i<bblocks.size(); i++) {
		baseblocks.push_back(bblocks[i].gid);
	}
		
	return(0);
}
			

int	AMRTree::paramesh_refine_baseblocks(
	int	index,
	int	pid,
	const int gids[][15]
) {


	if (gids[pid][7] < 0) return(0);	// no children

	// Vectors of paramesh cell ids (gids)
	//
	vector <int> gidbuf0;
	vector <int> gidbuf1;
	vector <int> *gptr0 = &gidbuf0;
	vector <int> *gptr1 = &gidbuf1;
	vector <int> *gtmpptr = NULL;

	// Vectors of cell ids 
	//
	vector <AMRTreeBranch::cid_t> cellidbuf0;
	vector <AMRTreeBranch::cid_t> cellidbuf1;
	vector <AMRTreeBranch::cid_t> *cptr0 = &cellidbuf0;
	vector <AMRTreeBranch::cid_t> *cptr1 = &cellidbuf1;
	vector <AMRTreeBranch::cid_t> *ctmpptr = NULL;

	gptr0->push_back(pid);
	cptr0->push_back(_treeBranches[index]->GetRoot());


	while ((*gptr0).size()) {
		int	gid;
		AMRTreeBranch::cid_t child;

		// Process each cell at the current level, checking to see
		// if the cell has children, and if so, refining the cell
		//
		for(int i=0; i<gptr0->size(); i++) {

			gid = (*gptr0)[i];
			if (gids[gid][7] >= 0) {	// does the cell have children?


				child = _treeBranches[index]->RefineCell((*cptr0)[i]);


				if (child < 0) return(-1);

				// Push all the children of this cell onto the list
				// for subsequent processing. Note: ids referenced in
				// gids array are off by one (hence the -1).
				//
				for (int j=0; j<8; j++) {
					(*gptr1).push_back(gids[gid][j+7] - 1);
					(*cptr1).push_back(child+j);
				}
			}
		}

		// Swap pointers to buffers
		//
		gtmpptr = gptr0;
		gptr0 = gptr1;
		gptr1 = gtmpptr;
		(*gptr1).clear();

		ctmpptr = cptr0;
		cptr0 = cptr1;
		cptr1 = ctmpptr;
		(*cptr1).clear();
	}

	return(0);
		
}


bool	AMRTree::elementStartHandler(
	ExpatParseMgr* pm, int level , std::string& tagstr, const char **attrs
) {
	
	// Invoke the appropriate start element handler depending on 
	// the XML tree depth
	//
	switch  (level) {
		case 0:
			_startElementHandler0(pm, tagstr, attrs);
			break;
		case 1:
			_startElementHandler1(pm, tagstr, attrs);
			break;
		case 2:
			_startElementHandler2(pm, tagstr, attrs);
			break;
		default:
			pm->parseError("Invalid tag : %s", tagstr.c_str());
			return false;
	}
	return true;
}

bool	AMRTree::elementEndHandler(
	ExpatParseMgr* pm, int level , std::string& tagstr
) {

	

	// Invoke the appropriate end element handler for an element at
	// XML tree depth, 'level'
	//
	switch  (level) {
	case 0:
		_endElementHandler0(pm, tagstr);
		break;
	case 1:
		_endElementHandler1(pm, tagstr);
		break;
	case 2:
		_endElementHandler2(pm, tagstr);
		break;
	default:
		pm->parseError("Invalid state");
		return false;
	}
	return true;
}



// Level 0 start element handler. The only element tag permitted at this
// level is the '_rootTag' tag
//
void	AMRTree::_startElementHandler0(ExpatParseMgr* pm,
	const string &tag, const char **attrs
) {

	ExpatStackElement *state = pm->getStateStackTop();
	state->has_data = 0;

	size_t base_dim[3];
	double min_bounds[3];
	double max_bounds[3];


	// Verify valid level 0 element
	//
	if (StrCmpNoCase(tag, _rootTag) != 0) {
		pm->parseError("Invalid tag : \"%s\"", tag.c_str());
		return;
	}

	// Parse attributes
	//
	while (*attrs) {
		string attr = *attrs;
		attrs++;
		string value = *attrs;
		attrs++;

		istringstream ist(value);
		if (StrCmpNoCase(attr, _baseDimAttr) == 0) {
			ist >> base_dim[0]; ist >> base_dim[1]; ist >> base_dim[2];
		}
		else if (StrCmpNoCase(attr, _minExtentsAttr) == 0) {
			ist >> min_bounds[0]; ist >> min_bounds[1]; ist >> min_bounds[2];
		}
		else if (StrCmpNoCase(attr, _maxExtentsAttr) == 0) {
			ist >> max_bounds[0]; ist >> max_bounds[1]; ist >> max_bounds[2];
		}
		else if (StrCmpNoCase(attr, _fileVersionAttr) == 0) {
			ist >> _fileVersion;
		}
		else {
			pm->parseError("Invalid tag attribute : \"%s\"", attr.c_str());
		}
	}

	if (_objIsInitialized) {
		_freeAMRTree();
	}

	_AMRTree(base_dim, min_bounds, max_bounds);
	if (GetErrCode()) {
		string s(GetErrMsg()); pm->parseError("%s", s.c_str());
		return;
	}
}

void	AMRTree::_startElementHandler1(ExpatParseMgr* pm,
	const string &tag, const char **attrs
) {

	ExpatStackElement *state = pm->getStateStackTop();
	state->has_data = 0;

	size_t location[3];
	double min_bounds[3];
	double max_bounds[3];


	// Verify valid level 0 element
	//
	if (StrCmpNoCase(tag, AMRTreeBranch::_rootTag) != 0) {
		pm->parseError("Invalid tag : \"%s\"", tag.c_str());
		return;
	}

	// Parse attributes
	//
	while (*attrs) {
		string attr = *attrs;
		attrs++;
		string value = *attrs;
		attrs++;

		istringstream ist(value);
		if (StrCmpNoCase(attr, AMRTreeBranch::_locationAttr) == 0) {
			ist >> location[0]; ist >> location[1]; ist >> location[2];

			_xml_help_loc = _baseDim[0] * _baseDim[1] * location[2] +
				_baseDim[0] * location[1] +
				location[0];

		}
		else if (StrCmpNoCase(attr, AMRTreeBranch::_minExtentsAttr) == 0) {
			ist >> min_bounds[0]; ist >> min_bounds[1]; ist >> min_bounds[2];
		}
		else if (StrCmpNoCase(attr, AMRTreeBranch::_maxExtentsAttr) == 0) {
			ist >> max_bounds[0]; ist >> max_bounds[1]; ist >> max_bounds[2];
		}
		else {
			pm->parseError("Invalid tag attribute : \"%s\"", attr.c_str());
		}
	}

	// Do nothing with the attributes we parsed for the tree 
	// branches - they're recalulated by the AMRTree constructor

}

void	AMRTree::_startElementHandler2(ExpatParseMgr* pm,
	const string &tag, const char **attrs
) {
	ExpatStackElement *state = pm->getStateStackTop();

	string type;

	string attr = *attrs;
	attrs++;
	string value = *attrs;
	attrs++;

	if (*attrs) {
		pm->parseError("Too many attributes");
		return;
	}
	istringstream ist(value);


	state->has_data = 1;

	if (StrCmpNoCase(attr, _typeAttr) != 0) {
		pm->parseError("Invalid attribute : %s", attr.c_str());
		return;
	}


	ist >> type;
	state->data_type = type;

	if (StrCmpNoCase(tag, AMRTreeBranch::_refinementLevelTag) == 0) {
		if (StrCmpNoCase(type, _longType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, AMRTreeBranch::_parentTableTag) == 0) {
		if (StrCmpNoCase(type, _longType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else {
		pm->parseError("Invalid tag : \"%s\"", tag.c_str());
		return;
	}
}


void	AMRTree::_endElementHandler0(ExpatParseMgr* pm,
	const string &tag
) {
	ExpatStackElement *state = pm->getStateStackTop();

	// this test is probably superfluous
	if (StrCmpNoCase(tag, state->tag) != 0) {
		pm->parseError("Invalid tag : \"%s\"", tag.c_str());
		return;
	}
}

void	AMRTree::_endElementHandler1(ExpatParseMgr* pm,
	const string &tag
) {
	ExpatStackElement *state = pm->getStateStackTop();

	// this test is probably superfluous
	if (StrCmpNoCase(tag, state->tag) != 0) {
		pm->parseError("Invalid tag : \"%s\"", tag.c_str());
		return;
	}
}

void	AMRTree::_endElementHandler2(ExpatParseMgr* pm,
	const string &tag
) {
	// ExpatStackElement *state = pm->getStateStackTop();

	if (StrCmpNoCase(tag, AMRTreeBranch::_refinementLevelTag) == 0) {
	}
	else if (StrCmpNoCase(tag, AMRTreeBranch::_parentTableTag) == 0) {
		int index = _xml_help_loc;
		if (index < 0 || index >= _baseDim[0]*_baseDim[1]*_baseDim[2]) {
			SetErrMsg("Error parsing file");
			return;
		}

		_treeBranches[index]->SetParentTable(pm->getLongData());
		if (AMRTreeBranch::GetErrCode() != 0) return;
	}
	else {
		pm->parseError("Invalid tag : \"%s\"", tag.c_str());
		return;
	}
}

void   AMRTree::_encode_cellid(
    AMRTreeBranch::cid_t baseblockidx,
    AMRTreeBranch::cid_t nodeidx,
    AMRTreeBranch::cid_t *cellid
) const {

	size_t t = 0;
	PUTBITS(t,_tbbits - 1,_tbbits, nodeidx);
	PUTBITS(t,_tbits + _tbbits - 1,_tbits, baseblockidx);
	*cellid = t;
}
