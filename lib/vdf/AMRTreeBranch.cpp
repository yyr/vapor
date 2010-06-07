//
//      $Id$
//

#include <cassert>
#include <sstream>
#include <map>
#include <vapor/AMRTreeBranch.h>

using namespace VAPoR;
using namespace VetsUtil;

// Static member initialization
//
const string AMRTreeBranch::_rootTag = "AMRTreeBranch";
const string AMRTreeBranch::_refinementLevelTag = "RefinementLevel";
const string AMRTreeBranch::_parentTableTag = "ParentTable";

const string AMRTreeBranch::_minExtentsAttr = "MinExtents";
const string AMRTreeBranch::_maxExtentsAttr = "MaxExtents";
const string AMRTreeBranch::_locationAttr = "Location";

AMRTreeBranch::AMRTreeBranch(
	XmlNode *parent,
	const double min[3], 
	const double max[3],
	const size_t loc[3]
) {
	SetDiagMsg(
		"AMRTreeBranch::AMRTreeBranch(,[%f,%f,%f],[%f,%f,%f],[%d,%d,%d])",
		min[0],min[1],min[2], max[0],max[1],max[2], loc[0],loc[1],loc[2]
	);
	
    vector <long> refinement_level;
    vector <long> parent_table;

	for (int i = 0; i<3; i++) {
		_minBounds[i] = min[i];
		_maxBounds[i] = max[i];
		_location[i] = loc[i];
	}

	refinement_level.push_back(0);

	ostringstream oss;
	string empty;
	map <string, string> attrs;

	oss.str(empty);
	oss << _location[0] << " " << _location[1] << " " << _location[2];
	attrs[_locationAttr] = oss.str();

	oss.str(empty);
	oss << _minBounds[0] << " " << _minBounds[1] << " " << _minBounds[2];
	attrs[_minExtentsAttr] = oss.str();

	oss.str(empty);
	oss << _maxBounds[0] << " " << _maxBounds[1] << " " << _maxBounds[2];
	attrs[_maxExtentsAttr] = oss.str();

	_rootNode = parent->NewChild(_rootTag, attrs);

	// Current maximum refinement level of tree branch
	//
	_rootNode->SetElementLong(_refinementLevelTag, refinement_level);

	// Current maximum refinement level of tree branch
	//
	_rootNode->SetElementLong(_parentTableTag, parent_table);

	_octree = new octree();
}

AMRTreeBranch::~AMRTreeBranch() 
{
	SetDiagMsg("AMRTreeBranch::~AMRTreeBranch()");
	if (_octree) delete _octree;
}

void AMRTreeBranch::Update() {
	vector <long> parent_table;
	vector <long> ref_level;

	ref_level.push_back(_octree->get_max_level());

	bool first = true;
	cid_t cellid;
	while ((cellid = _octree->get_next(first)) >= 0) {
		first = false;

		if (_octree->has_children(cellid)) {
			long flag = 0;
			cid_t child = _octree->get_children(cellid);
			for (int i=0; i<8; i++) {
				if (_octree->has_children(child+i)) {
					flag += 1 << i;
				}
			}
			parent_table.push_back(flag);
		}
	}
	_rootNode->SetElementLong(_parentTableTag, parent_table);
	_rootNode->SetElementLong(_refinementLevelTag, ref_level);
}

int AMRTreeBranch::DeleteCell(cid_t cellid) {

	SetDiagMsg("AMRTreeBranch::DeleteCell(%u)", cellid);

	SetErrMsg("AMRTreeBranch::DeleteCell() not supported\n");
	return(-1);
}

AMRTreeBranch::cid_t	AMRTreeBranch::FindCell(
	const double ucoord[3], 
	int ref_level
) const {

	SetDiagMsg(
		"AMRTreeBranch::FindCell([%f,%f,%f], %d)", 
		ucoord[0],ucoord[1],ucoord[2], ref_level
	);

	for(int i =0; i<3; i++) {
		if (ucoord[i] < _minBounds[i] || ucoord[i] > _maxBounds[i]) {
			SetErrMsg("Point not contained in this tree branch\n");
			return(-1);
		}
	}

	if (ref_level < 0) ref_level = _octree->get_max_level();
	if (ref_level > _octree->get_max_level()) {
		SetErrMsg("Invalid refinement level : %d \n", ref_level);
		return(-1);
	}

	// If the requested refinement level is 0, or if the root has no
	// children, return the root cell id
	//
	if (ref_level == 0 || ! _octree->has_children(0)) return(0); 


	cid_t	cellid = 0;
	double minbnd[] = {_minBounds[0], _minBounds[1], _minBounds[2]};
	double maxbnd[] = {_maxBounds[0], _maxBounds[1], _maxBounds[2]};
	for (int level = 0; level <= _octree->get_max_level(); level++) {

		// We're done if the current cell has no children or
		// we're at the specified refinement level
		//
		if (! _octree->has_children(cellid)) return(cellid);	
		if (level == ref_level) return(cellid);

		//
		// Find the child that contains the desired point
		//
		unsigned child = 0;
		for (int i=0; i<3; i++) {
			if (ucoord[i] < ((maxbnd[i] - minbnd[i]) / 2.0)) {
				maxbnd[i] = (maxbnd[i] - minbnd[i]) / 2.0;
			}
			else {
				minbnd[i] = (maxbnd[i] - minbnd[i]) / 2.0;
				child += (1<<i);
			}
		}

		cellid = _octree->get_children(cellid) + child;
	}

	SetErrMsg("Requested refinement level doesn't exist");
	return(-1);

}

int	AMRTreeBranch::GetCellBounds(
	cid_t cellid, double minu[3], double maxu[3]
) const {

	SetDiagMsg(
		"AMRTreeBranch::GetCellBounds(%u, [%f,%f,%f], [%f,%f,%f])", 
		cellid, minu[0],minu[1],minu[2], maxu[0],maxu[1],maxu[2]
	);

	size_t xyz[3];
	int level;

	int rc = _octree->get_location(cellid, xyz, &level);
	if (rc < 0) {
		SetErrMsg("Invalid cellid : %dl \n", cellid);
		return(-1);
	}

	unsigned int dim = 1 << level;

	for (int i=0; i<3; i++) {
		double delta = (_maxBounds[i] - _minBounds[i]) / (double) dim;
		minu[i] = _minBounds[i] + (xyz[i] * delta);
		maxu[i] = _minBounds[i] + ((xyz[i]+1) * delta);
	}

	return(0);

}

int	AMRTreeBranch::GetCellLocation(
	cid_t cellid, size_t xyz[3], int *ref_level
) const {

	size_t xyz_local[3];

	int rc = _octree->get_location(cellid, xyz_local, ref_level);
	if (rc < 0) {
		SetErrMsg("Invalid cellid : %dl \n", cellid);
		return(-1);
	}

	for (int i=0; i<3; i++) {
		xyz[i] = (_location[i] << *ref_level) + xyz_local[i];
	}

	return(0);
}

AMRTreeBranch::cid_t	AMRTreeBranch::GetCellID(
	const size_t xyz[3], int ref_level
) const {

	SetDiagMsg(
		"AMRTreeBranch::GetCellID([%d,%d,%d], %d)", 
		xyz[0],xyz[1],xyz[2], ref_level
	);

	if (ref_level < 0) ref_level = _octree->get_max_level();

	size_t xyz_local[3];
	for (int i=0; i<3; i++) {
		xyz_local[i]  = xyz[i] - (_location[i] >> ref_level);
	}

	cid_t cellid = _octree->get_cellid(xyz_local, ref_level);
	if (cellid < 0) {
		SetErrMsg("Invalid location : %d %d %d\n", xyz[0], xyz[1], xyz[2]);
		return(-1);
	}

	return(cellid);
}

AMRTreeBranch::cid_t	AMRTreeBranch::GetCellChildren(
	cid_t cellid
) const {

	SetDiagMsg("AMRTreeBranch::GetCellChildren(%u)", cellid);

	cid_t child = _octree->get_children(cellid);

	if (child < 0) {
		SetErrMsg("Invalid cellid : %dl \n", cellid);
		return(-1);
	}
	return(child);
}

int	AMRTreeBranch::GetCellLevel(cid_t cellid) const {

	SetDiagMsg("AMRTreeBranch::GetCellLevel(%u)", cellid);

	int level = _octree->get_level(cellid);

	if (level < 0) {
		SetErrMsg("Invalid cellid : %dl \n", cellid);
		return(-1);
	}
	return(level);
}


AMRTreeBranch::cid_t	AMRTreeBranch::GetCellNeighbor(
	cid_t cellid, int face
) const {

	SetDiagMsg("AMRTreeBranch::GetCellNeighbor(%u, %d)", cellid, face);

	SetErrMsg("AMRTreeBranch::GetCellNeighbor() not supported");
	return(-1);
}

AMRTreeBranch::cid_t	AMRTreeBranch::GetCellParent(
	cid_t cellid
) const {

	cid_t parent = _octree->get_parent(cellid);
	if (cellid < 0) {
		SetErrMsg("Invalid cellid : %dl \n", cellid);
		return(-1);
	}

	return(parent);
}

AMRTreeBranch::cid_t AMRTreeBranch::GetNumCells(
) const {
	SetDiagMsg("AMRTreeBranch::GetNumCells()");

	return(GetNumCells((cid_t) -1));
}

AMRTreeBranch::cid_t AMRTreeBranch::GetNumCells(
	int ref_level
) const {
	SetDiagMsg("AMRTreeBranch::GetNumCells(%d)", ref_level);

	AMRTreeBranch::cid_t num;

	if ((ref_level < 0) || ref_level > _octree->get_max_level()) {
		ref_level = _octree->get_max_level();
	}

	num = _octree->get_num_cells(ref_level);
	if (num < 0) {
		SetErrMsg("Invalid refinement level : %d \n", ref_level);
		return(-1);
	}

	return(num);
}
		

int	AMRTreeBranch::HasChildren(
	cid_t cellid
) const {

	SetDiagMsg("AMRTreeBranch::HasChildren(%u)", cellid);

	return(_octree->has_children(cellid));

}


//
// It is an error to try and refine a cell that is already refined. I.e. 
// cellid must refer to a leaf node.
//
AMRTreeBranch::cid_t	AMRTreeBranch::RefineCell(
	cid_t cellid
) {

	SetDiagMsg("AMRTreeBranch::RefineCell(%u)", cellid);

	cid_t child = _octree->refine_cell(cellid);
	if (child < 0) {
		SetErrMsg("Invalid (or already refined) cell id %d\n", cellid);
		return(-1);
	}
	return(child);
}

AMRTreeBranch::cid_t	AMRTreeBranch::GetNextCell(
	bool restart
) {

	SetDiagMsg("AMRTreeBranch::GetNextCell(%d)", restart);

	return(_octree->get_next(restart));
}

int AMRTreeBranch::SetParentTable(const vector <long> &table) {

	SetDiagMsg("AMRTreeBranch::SetParentTable()");

	vector <long> parent_table;
	vector <long> ref_level;

	parent_table.reserve(table.size());

	for(int i=0; i<table.size(); i++) {
		parent_table.push_back(table[i]);
	}
	_rootNode->SetElementLong(_parentTableTag, parent_table);

	_octree->clear();
	if (table.size() == 0) return(0);	// we're done

	vector <cid_t> p1, p2;
	p1.push_back(_octree->refine_cell(0));

	vector <long> flags;

	vector <long>::const_iterator itr = table.begin();

	while (itr != table.end()) {
		if (p1.size() == 0) {
			SetErrMsg("Invalid table");
			return(-1);
		}
		flags.clear();
		for (int i=0; i<p1.size(); i++) {
			if (itr == table.end()) {
				SetErrMsg("Invalid table");
				return(-1);
			}
			flags.push_back(*itr);
			itr++;
		}
		p2.clear();
		for (int i=0; i<flags.size(); i++) {
			unsigned int flag = flags[i];
			for (int j=0; j<8; j++) {
				if (flag & 1<<j) { 
					cid_t c = _octree->refine_cell(p1[i]+j);
					assert(c>0);
					p2.push_back(c);
				}
			}
		}
		p1 = p2;
	}

	ref_level.push_back(_octree->get_max_level());
	_rootNode->SetElementLong(_refinementLevelTag, ref_level);

	return(0);
}

void AMRTreeBranch::octree::_init() {
	_node_t node;

	_tree.clear();
	_num_cells.clear();

	node.parent = -1;
	node.child = -1;
	_tree.push_back(node);
	_num_cells.push_back(1);

	_bft_next.clear();
	_bft_itr = _bft_next.begin();
	_bft_children.clear();
	_max_level = 0;
}

AMRTreeBranch::octree::octree() {
	_init();
}

void AMRTreeBranch::octree::clear() {
	_init();
}

AMRTreeBranch::cid_t AMRTreeBranch::octree::refine_cell(cid_t cellid) {
	if (cellid > _tree.size()) return(-1);

	_node_t pnode = _tree[cellid];

	if (pnode.child >= 0) return(-1);	// cell already refined


	pnode.child = _tree.size();
	_tree[cellid] = pnode;

	_node_t cnode;

	cnode.parent = cellid;
	cnode.child = -1;
	for (int i=0; i<8; i++) {
		_tree.push_back(cnode);
	}

	// Calculate new max refinement level  and update node count
	//
	cid_t	 parent = cellid;
	int level = 1;	// level of new children 
	while ((parent = get_parent(parent)) >= 0) level++;
	if (level > _max_level) {
		_max_level = level;
		_num_cells.push_back(8);
	}
	else {
		assert(_num_cells.size() > level);
		_num_cells[level] += 8;
	}

	return(pnode.child);
}
		
AMRTreeBranch::cid_t AMRTreeBranch::octree::get_parent(cid_t cellid) const {
	if (cellid < 0 || cellid > _tree.size()) return(-1);

	return (_tree[cellid].parent);
}

AMRTreeBranch::cid_t AMRTreeBranch::octree::get_children(cid_t cellid) const {
	if (cellid < 0 || cellid > _tree.size()) return(-1);

	return (_tree[cellid].child);
}


AMRTreeBranch::cid_t AMRTreeBranch::octree::get_next(bool restart) {

	if (restart) {
		_bft_next.clear();
		_bft_next.push_back(0);
		_bft_itr = _bft_next.begin();
		_bft_children.clear();
	}

	if (_bft_itr == _bft_next.end()) {
		_bft_next = _bft_children;
		_bft_itr = _bft_next.begin();
		_bft_children.clear();
	}

	if (_bft_itr == _bft_next.end()) return(-1); // we're done.

	cid_t cellid = *_bft_itr;
	_bft_itr++;

	_node_t node = _tree[cellid];
	if (node.child >= 0) {
		for (int i=0; i<8; i++) {
			_bft_children.push_back(node.child+i);
		}
	}
	return(cellid);
}

int AMRTreeBranch::octree::get_octant(cid_t cellid) const {

	cid_t parent = get_parent(cellid);

	if (parent < 0) return(-1);

	return(cellid - _tree[parent].child);
}

AMRTreeBranch::cid_t AMRTreeBranch::octree::get_cellid(
	const size_t xyz[3], int level
) const {
	if (level < 0) level = get_max_level();
	if (level > get_max_level()) return(-1);

	for (int i=0; i<3; i++) {
		if ((xyz[i] >> level) != 0) return(-1);
	}
	if (level == 0) return(0);	// root node

	cid_t cellid = 0;
	for (int l=1; l<=level; l++) {
		_node_t node = _tree[cellid];

		if (node.child < 0) return(-1);	// no children
		
		size_t x = xyz[0] >> (level-l);
		size_t y = xyz[1] >> (level-l);
		size_t z = xyz[2] >> (level-l);

		int offset = 0;
		if (x%2) offset += 1;
		if (y%2) offset += 2;
		if (z%2) offset += 4;

		cellid = node.child + offset;
	}
	return(cellid);
}

int AMRTreeBranch::octree::get_location(cid_t cellid, size_t xyz[3], int *level) const {

	if (cellid < 0 || cellid >= _tree.size()) return(-1);

	vector <cid_t> cellids;

	xyz[0] = 0;
	xyz[1] = 0;
	xyz[2] = 0;
	*level = 0;

	if (cellid == 0) return(0);	// root node, we're done.

	cellids.push_back(cellid);
	cid_t parent = cellid;
	while (parent != 0) {
		_node_t node = _tree[parent];
		cellids.push_back(node.parent);
		parent = node.parent;
		assert(parent >= 0);
	}

	parent = cellids.back();
	cellids.pop_back();
	while (cellids.size()) {
		cid_t child = cellids.back();
		cellids.pop_back();
		_node_t node = _tree[parent];

		cid_t offset = child - node.child;

		*level = *level + 1;
		xyz[0] = xyz[0] << 1;
		if (offset % 2) xyz[0] += 1;
		offset = offset >> 1;

		xyz[1] = xyz[1] << 1;
		if (offset % 2) xyz[1] += 1;
		offset = offset >> 1;

		xyz[2] = xyz[2] << 1;
		if (offset % 2) xyz[2] += 1;
		offset = offset >> 1;

		parent = child;
	}
	return(0);
}

int AMRTreeBranch::octree::get_level(cid_t cellid) const {

	if (cellid < 0 || cellid >= _tree.size()) return(-1);

	cid_t	 parent = cellid;
	int level = 0;
	while ((parent = get_parent(parent)) >= 0) level++;

	return(level);
}

AMRTreeBranch::cid_t AMRTreeBranch::octree::get_num_cells(int ref_level) const {

	if (ref_level > _max_level) return(-1);

	cid_t n = 0;
	for (int i=0; i<_num_cells.size() && i <= ref_level; i++) n+= _num_cells[i];

	return(n);
}

bool AMRTreeBranch::octree::has_children(cid_t cellid) const {

	if (cellid < 0 || cellid >= _tree.size()) return(false);

	_node_t node = _tree[cellid];

	return(node.child >= 0);
}
