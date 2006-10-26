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
#include <vapor/AMRTree.h>

using namespace VAPoR;
using namespace VetsUtil;

const string AMRTree::_rootTag = "AMRTree";
const string AMRTree::_minExtentsAttr = "MinExtents";
const string AMRTree::_maxExtentsAttr = "MaxExtents";
const string AMRTree::_baseDimAttr = "BaseDimension";
const string AMRTree::_fileVersionAttr = "FileVersion";


int AMRTree::_AMRTree(
	const size_t basedim[3], 
	const double min[3], 
	const double max[3]
) {

	_treeBranches = NULL;

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
		int	index = (_baseDim[1] * _baseDim[0] * z) + (_baseDim[0] * y) + x;
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
#ifdef	DEAD
#endif
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
	const int	paramesh_gids[][15],
	const float paramesh_bboxs [][3][2],
	const int paramesh_refine_levels[],
	int total_blocks
	
) {
    SetDiagMsg( "AMRTree::AMRTree(,,,%d)", total_blocks);

	_objIsInitialized = 0;

	size_t basedim[3];
	vector <int> baseblocks;


	// Compute the dimension of the volue in base blocks and identify all of
	// the base blocks
	//
	if (_parameshGetBaseBlocks(baseblocks,basedim,paramesh_gids,total_blocks)<0)return;

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
    CellID cellid,
    AMRTreeBranch::UInt32 *baseblockidx,
    AMRTreeBranch::UInt32 *nodeidx
) const {

    SetDiagMsg( "AMRTree::DecodeCellID(%lld,,)", cellid);

	*nodeidx = GetBits64(cellid,31,32);
	*baseblockidx = GetBits64(cellid,47,16);
}

int	AMRTree::DeleteCell(CellID cellid) {
    SetDiagMsg( "AMRTree::DeleteCell(%lld)", cellid);

	SetErrMsg("This function is not implemented");
	return(-1);
}

AMRTree::CellID	AMRTree::FindCell(
	const double ucoord[3], int reflevel
) const {

    SetDiagMsg(
		"AMRTree::DeleteCell([%f,%f,%f], %d)", 
		ucoord[0], ucoord[1], ucoord[2], reflevel
	);

	assert(_treeBranches != NULL);


	unsigned long long	cellid = 0;
	AMRTreeBranch::UInt32 tbid;

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

	int	index = (_baseDim[1] * _baseDim[0] * z) + (_baseDim[0] * y) + x;

	tbid = _treeBranches[index]->FindCell(ucoord, reflevel);
	if (tbid == AMRTreeBranch::AMR_ERROR) return(-1);

	cellid = SetBits64(cellid, 31, 32, tbid);
	cellid = SetBits64(cellid, 47, 16, index);

	return((CellID) cellid);
	
}

int	AMRTree::GetBounds(
	double min[3], double max[3], CellID cellid
) const {

    SetDiagMsg( "AMRTree::GetBounds(,,%lld)", cellid);

	SetErrMsg("This function is not implemented");
	return(-1);
}

const AMRTreeBranch    *AMRTree::GetBranch(const size_t xyz[3]) const {

    SetDiagMsg( "AMRTree::GetBranch([%u,%u,%u])", xyz[0],xyz[1],xyz[2]);


	int	index = (_baseDim[1] * _baseDim[0] * xyz[2]) + (_baseDim[0] * xyz[1]) + xyz[0];
	return(_treeBranches[index]);
}


AMRTree::CellID	AMRTree::GetCellChildren(
	CellID cellid
) const {

    SetDiagMsg( "AMRTree::GetCellChildren(%lld)", cellid);

	assert(_treeBranches != NULL);


	AMRTreeBranch::UInt32	index;
	AMRTreeBranch::UInt32	tbid;

	DecodeCellID(cellid, &index, &tbid);

	if (index >= _baseDim[0] * _baseDim[1] * _baseDim[2]) {
		SetErrMsg("Invalid cell id : %lld\n", cellid);
		return(-1);
	}

	AMRTreeBranch::UInt32 child_tbid = _treeBranches[index]->GetCellChildren(tbid);
	if (child_tbid == AMRTreeBranch::AMR_ERROR) return(-1);

	CellID child_cellid = 0LL;
	child_cellid = SetBits64(child_cellid, 31, 32, child_tbid);
	child_cellid = SetBits64(child_cellid, 47, 16, index);

	return((CellID) child_cellid);

}

int	AMRTree::GetCellLevel(CellID cellid) const {

    SetDiagMsg( "AMRTree::GetCellLevel(%lld)", cellid);

	assert(_treeBranches != NULL);


	AMRTreeBranch::UInt32	index;
	AMRTreeBranch::UInt32	tbid;

	DecodeCellID(cellid, &index, &tbid);

	if (index >= _baseDim[0] * _baseDim[1] * _baseDim[2]) {
		SetErrMsg("Invalid cell id : %lld\n", cellid);
		return(-1);
	}

	int level = _treeBranches[index]->GetCellLevel(tbid);
	if (level == AMRTreeBranch::AMR_ERROR) return(-1);

	return(level);

}


AMRTree::CellID	AMRTree::GetCellNeighbor(CellID cellid, int face) const {

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


AMRTree::CellID AMRTree::GetNumNodes(
    const size_t min [3],
    const size_t max[3],
    int reflevel
) const {

    SetDiagMsg(
		"AMRTree::GetNumNodes([%u,%u,%u], [%u,%u,%u], %d)",
		min[0], min[1], min[2], max[0], max[1], max[2], reflevel
	);

	assert(_treeBranches != NULL);


	for (int i = 0; i<3; i++) {
		if (min[i] > max[i] || max[i] >= _baseDim[i]) {
			SetErrMsg("Invalid coordinates");
			return(-1);
		}
	}

	CellID nnodes = 0;
	for (int z = min[2]; z <= max[2]; z++) {
	for (int y = min[1]; y <= max[1]; y++) {
	for (int x = min[0]; x <= max[0]; x++) {

		int	index = (_baseDim[1] * _baseDim[0] * z) + (_baseDim[0] * y) + x;
		nnodes += _treeBranches[index]->GetNumNodes(reflevel);
		
	}
	}
	}

	return(nnodes);
}

AMRTree::CellID AMRTree::GetNumNodes(
    int reflevel
) const {

    SetDiagMsg( "AMRTree::GetNumNodes(%d)", reflevel);

	size_t min[3] = {0,0,0};
	size_t max[3] = {_baseDim[0]-1, _baseDim[1]-1, _baseDim[2]-1};

	return(GetNumNodes(min, max, reflevel));
}




AMRTree::CellID	AMRTree::GetCellParent(CellID cellid) const
{
    SetDiagMsg( "AMRTree::GetCellParent(%lld)", cellid);
	assert(_treeBranches != NULL);


	AMRTreeBranch::UInt32	index;
	AMRTreeBranch::UInt32	tbid;

	DecodeCellID(cellid, &index, &tbid);

	if (index >= _baseDim[0] * _baseDim[1] * _baseDim[2]) {
		SetErrMsg("Invalid cell id : %lld\n", cellid);
		return(-1);
	}

	AMRTreeBranch::UInt32 parent_tbid = _treeBranches[index]->GetCellParent(tbid);
	if (parent_tbid == AMRTreeBranch::AMR_ERROR) return(-1);

	CellID parent_cellid = 0LL;
	parent_cellid = SetBits64(parent_cellid, 31, 32, parent_tbid);
	parent_cellid = SetBits64(parent_cellid, 47, 16, index);

	return((CellID) parent_cellid);

}

AMRTree::CellID	AMRTree::RefineCell(CellID cellid) {

    SetDiagMsg( "AMRTree::RefineCell(%lld)", cellid);

	assert(_treeBranches != NULL);


	AMRTreeBranch::UInt32	index;
	AMRTreeBranch::UInt32	tbid;

	DecodeCellID(cellid, &index, &tbid);

	if (index >= _baseDim[0] * _baseDim[1] * _baseDim[2]) {
		SetErrMsg("Invalid cell id : %lld\n", cellid);
		return(-1);
	}

	int child_tbid = _treeBranches[index]->RefineCell(tbid);
	if (child_tbid == AMRTreeBranch::AMR_ERROR) return(-1);

	CellID child_cellid = 0LL;
	child_cellid = SetBits64(child_cellid, 31, 32, child_tbid);
	child_cellid = SetBits64(child_cellid, 47, 16, index);

	return((CellID) child_cellid);

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

int	AMRTree::_parameshGetBaseBlocks(
	vector <int> &baseblocks,
	size_t basedim[3],
	const int	gids[][15],
	int	totalblocks
) const {
	vector <int> tmpbb;	// base blocks

	// base blocks are ones with no parents
	//
	for (int i=0; i<totalblocks; i++) {
		if (gids[i][6] == -1) {
			tmpbb.push_back(i);
		}
	}

	//
	// find the first base block - the boundary corner block 
	// with the smallest integer coordinates
	//
	int	firstblk = -1;
	for (int i=0; i<tmpbb.size() && firstblk == -1; i++) {
		if ((gids[tmpbb[i]][0] <= -20) &&
			(gids[tmpbb[i]][2] <= -20) &&
			(gids[tmpbb[i]][4] <= -20)) {

			firstblk = tmpbb[i];
		}
	}
	if (firstblk == -1) {
		SetErrMsg("Invalid Paramesh gids record");
		return(-1);
	}


	// Compute the XYZ dimensions of the volume in base blocks
	//
	int	nbr; // id of block's neighbor
	int	idx = 0; // current block index;

	basedim[0] = 1;
	idx = firstblk;
	while (1) {
		// If X+1 neighbor is boundary, quit
		//
		if ((nbr = gids[idx][1]) <= -20) break;
		idx = nbr-1; // elements in gids are off by 1
		basedim[0]++;
	}

	basedim[1] = 1;
	idx = firstblk;
	while (1) {
		// If Y+1 neighbor is boundary, quit
		//
		if ((nbr = gids[idx][3]) <= -20) break;
		idx = nbr-1;
		basedim[1]++;
	}

	basedim[2] = 1;
	idx = firstblk;
	while (1) {
		// If Z+1 neighbor is boundary, quit
		//
		if ((nbr = gids[idx][5]) <= -20) break;
		idx = nbr-1;
		basedim[2]++;
	}
		
	// Sanity check
	//
	if (tmpbb.size() != (basedim[0]*basedim[1]*basedim[2])) {
		SetErrMsg("Invalid Paramesh gids record");
		return(-1);
	}

	// 
	// Now order all the blocks
	//
	baseblocks.push_back(firstblk);

	int	err = 0;
	int	i;

	idx = 0;
	for(int z=0; z<basedim[2] && !err; z++) {

		for(int y=0; y<basedim[1] && !err; y++) {

			for(int x=0; x<basedim[0] && !err; x++) {

				nbr = gids[baseblocks[idx]][1];	// X+1 neighbor
				if (x < basedim[0]-1) {
					nbr--;	// elements in gids are off by 1

					for(i=0; i<tmpbb.size() && nbr !=tmpbb[i]; i++);

					if (nbr != tmpbb[i]) err = 1;
					baseblocks.push_back(tmpbb[i]);
				}
				else if (nbr > -20) err = 1;
				idx++;
				
			}
			if (err) break;
			nbr = gids[baseblocks[idx-basedim[0]]][3]; // Y+1 neighbor
			if (y < basedim[1]-1) {
				nbr--;	// elements in gids are off by 1

				for(i=0; i<tmpbb.size() && nbr !=tmpbb[i]; i++);

				if (nbr != tmpbb[i]) err = 1;
				baseblocks.push_back(tmpbb[i]);
			}
			else if (nbr > -20) err = 1;

		}
		if (err) break;
		nbr = gids[baseblocks[idx-(basedim[0]*basedim[1])]][5]; // Z+1 neighbor
		if (z < basedim[2]-1) {
			nbr--;	// elements in gids are off by 1

			for(i=0; i<tmpbb.size() && nbr !=tmpbb[i]; i++);

			if (nbr != tmpbb[i]) err = 1;
			baseblocks.push_back(tmpbb[i]);
		}
		else if (nbr > -20) err = 1;
	}


	if (err) {
		SetErrMsg("Invalid Paramesh gids record");
		return(-1);
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
	vector <AMRTreeBranch::UInt32> cellidbuf0;
	vector <AMRTreeBranch::UInt32> cellidbuf1;
	vector <AMRTreeBranch::UInt32> *cptr0 = &cellidbuf0;
	vector <AMRTreeBranch::UInt32> *cptr1 = &cellidbuf1;
	vector <AMRTreeBranch::UInt32> *ctmpptr = NULL;

	gptr0->push_back(pid);
	cptr0->push_back(_treeBranches[index]->GetRoot());


	while ((*gptr0).size()) {
		int	gid;
		AMRTreeBranch::UInt32 child;

		// Process each cell at the current level, checking to see
		// if the cell has children, and if so, refining the cell
		//
		for(int i=0; i<gptr0->size(); i++) {

			gid = (*gptr0)[i];
			if (gids[gid][7] >= 0) {	// does the cell have children?


				child = _treeBranches[index]->RefineCell((*cptr0)[i]);


				if (child == AMRTreeBranch::AMR_ERROR) return(-1);

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

