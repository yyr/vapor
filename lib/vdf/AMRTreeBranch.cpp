//
//      $Id$
//

#include <cassert>
#include <sstream>
#include <map>
#include <vapor/AMRTreeBranch.h>

#ifdef	QUADTREE
#define	NREGIONS	4
#define NBITS	2
#else
#define	NREGIONS	8
#define NBITS	3
#endif

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

const int AMRTreeBranch::XLOC = 0;
const int AMRTreeBranch::YLOC = 1;
const int AMRTreeBranch::ZLOC = 2;
const int AMRTreeBranch::CHILDMASK = 3;
const int AMRTreeBranch::LEVEL = 4;
const int AMRTreeBranch::SZ = 5;

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
	
	_numNodes = 1;

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
}

AMRTreeBranch::~AMRTreeBranch() 
{
	SetDiagMsg("AMRTreeBranch::~AMRTreeBranch()");
}

int	AMRTreeBranch::DeleteCell(UInt32 cellid) {

	SetDiagMsg("AMRTreeBranch::DeleteCell(%u)", cellid);

	if (cellid >= _numNodes) {
		SetErrMsg("Invalid cell id %d\n", cellid);
		return(ERROR);
	}

	return(ERROR);
}

AMRTreeBranch::UInt32	AMRTreeBranch::FindCell(
	const double ucoord[3], 
	int refinementlevel
) const {

	SetDiagMsg(
		"AMRTreeBranch::FindCell([%f,%f,%f], %d)", 
		ucoord[0],ucoord[1],ucoord[2], refinementlevel
	);

	vector <long> &ref_level = _rootNode->GetElementLong(_refinementLevelTag);
	vector <long> &parent_table = _rootNode->GetElementLong(_parentTableTag);

	UInt32 cellid;

	for(int i =0; i<3; i++) {
		if (ucoord[i] < _minBounds[i] || ucoord[i] > _maxBounds[i]) {
			SetErrMsg("Point not contained in this tree branch\n");
			return(ERROR);
		}
	}

	// If the requested refinement level is 0, or if the root has no
	// children, return the root cell id
	//
	if (refinementlevel == 0 || ! HasChildren(0)) return(0); // root cell;

	if (refinementlevel > ref_level[0]) {
		SetErrMsg("Invalid refinement level : %d \n", refinementlevel);
		return(ERROR);
	}

	UInt32	cellidx = 0;	// internally root cell is -1, first child is 0
	unsigned int parentidx;
	unsigned int dim = 2;	// dimenion of branch, in cells at current level

	for (int level = 1; level <= ref_level[0]; level++) {

		double deltax = _maxBounds[0] - _minBounds[0] / (double) dim;
		double deltay = _maxBounds[1] - _minBounds[1] / (double) dim;
		double deltaz = _maxBounds[2] - _minBounds[2] / (double) dim;

		parentidx = cellidx >> NBITS;

		vector<long>::const_iterator itr;
		itr = parent_table.begin() + (parentidx*SZ);

		if (((itr[XLOC] + 1) * deltax) < ucoord[0]) cellidx += 1;
		if (((itr[YLOC] + 1) * deltay) < ucoord[1]) cellidx += 2;
		if (((itr[ZLOC] + 1) * deltaz) < ucoord[2]) cellidx += 4;

		if (level == refinementlevel) return(cellidx+1);

		if (! HasChildren(cellidx+1)) break;

		cellid = GetCellChildren(cellidx+1);
		if (cellid == ERROR) return (ERROR);

		cellidx = cellid-1;

		dim *= 2;
	}

	if (refinementlevel == -1) return(cellidx+1);

	SetErrMsg("Requested refinement level doesn't exist");
	return(ERROR);

}

int	AMRTreeBranch::GetCellBounds(
	UInt32 cellid, double minu[3], double maxu[3]
) const {

	SetDiagMsg(
		"AMRTreeBranch::GetCellBounds(%u, [%f,%f,%f], [%f,%f,%f])", 
		cellid, minu[0],minu[1],minu[2], maxu[0],maxu[1],maxu[2]
	);

	vector <long> &parent_table = _rootNode->GetElementLong(_parentTableTag);

	if (cellid >= _numNodes) {
		SetErrMsg("Invalid cell id %d\n", cellid);
		return(ERROR);
	}

	if (cellid == 0) {	// root cell
		for (int i=0; i<3; i++) {
			minu[i] = _minBounds[i];
			maxu[i] = _maxBounds[i];
		}
		return(0);
	}

	// Internally the Id of the first child after the root node is 0
	UInt32 cellidx = cellid -1;

	int parentidx = cellidx >> NBITS;
	assert((parentidx * SZ) < parent_table.size());

	vector<long>::const_iterator itr;
	itr = parent_table.begin() + (parentidx*SZ);

	// calulate the location codes from the parent based on which
	// of the parent's octants are being refined.
	//
	unsigned int xloc = (unsigned int) itr[XLOC] << 1;
	unsigned int yloc = (unsigned int) itr[YLOC] << 1;
	unsigned int zloc = (unsigned int) itr[ZLOC] << 1;
	if (cellidx & (0x1 << 0)) xloc += 1;
	if (cellidx & (0x1 << 1)) yloc += 1;
	if (cellidx & (0x1 << 2)) zloc += 1;

	int level = (int) itr[LEVEL] + 1;

	double dim = 1.0;
	for(int i = 0; i<level; i++, dim *= 2);

	double deltax = _maxBounds[0] - _minBounds[0] / (double) dim;
	double deltay = _maxBounds[1] - _minBounds[1] / (double) dim;
	double deltaz = _maxBounds[2] - _minBounds[2] / (double) dim;

	minu[0] = _minBounds[0] + (xloc * deltax);
	minu[1] = _minBounds[1] + (yloc * deltay);
	minu[2] = _minBounds[2] + (zloc * deltaz);

	maxu[0] = minu[0] + deltax;
	maxu[1] = minu[1] + deltay;
	maxu[2] = minu[2] + deltaz;

	return(0);
}

int	AMRTreeBranch::GetCellLocation(
	UInt32 cellid, unsigned int xyz[3]
) const {

	SetDiagMsg(
		"AMRTreeBranch::GetCellLocation(%u, [%u,%u,%u])", 
		cellid, xyz[0],xyz[1],xyz[2]
	);

	vector <long> &parent_table = _rootNode->GetElementLong(_parentTableTag);

	if (cellid >= _numNodes) {
		SetErrMsg("Invalid cell id %d\n", cellid);
		return(ERROR);
	}

	if (cellid == 0) {	// root cell
		for (int i=0; i<3; i++) {
			xyz[i] = (unsigned int) _location[i];
		}
		return(0);
	}

	// Internally the Id of the first child after the root node is 0
	UInt32 cellidx = cellid -1;

	int parentidx = cellidx >> NBITS;
	assert((parentidx * SZ) < parent_table.size());

	vector<long>::const_iterator itr;
	itr = parent_table.begin() + (parentidx*SZ);

	// calulate the location codes from the parent based on which
	// of the parent's octants are being refined.
	//
	unsigned int xloc = (unsigned int) itr[XLOC] << 1;
	unsigned int yloc = (unsigned int) itr[YLOC] << 1;
	unsigned int zloc = (unsigned int) itr[ZLOC] << 1;
	if (cellidx & (0x1 << 0)) xloc += 1;
	if (cellidx & (0x1 << 1)) yloc += 1;
	if (cellidx & (0x1 << 2)) zloc += 1;

	int level = (int) itr[LEVEL] + 1;


	// Combine the location codes of the cell, which are relative to
	// the base cell, with the location of the base cell, adjusted for
	// the requested level
	//
	xyz[0] = ((unsigned int) _location[0] << level) + xloc;
	xyz[1] = ((unsigned int) _location[1] << level) + yloc;
	xyz[2] = ((unsigned int) _location[2] << level) + zloc;

	return(0);
}

AMRTreeBranch::UInt32	AMRTreeBranch::GetCellChildren(
	UInt32 cellid
) const {

	SetDiagMsg("AMRTreeBranch::GetCellChildren(%u)", cellid);

	vector <long> &parent_table = _rootNode->GetElementLong(_parentTableTag);

	if (cellid >= _numNodes) {
		SetErrMsg("Invalid cell id %d\n", cellid);
		return(ERROR);
	}

	if (cellid == 0) return(0+1);	// root cell

	// Internally the Id of the first child after the root node is 0
	UInt32 cellidx = cellid -1;

	int parentidx = cellidx >> NBITS;
	assert((parentidx * SZ) < parent_table.size());

	vector<long>::const_iterator itr;
	itr = parent_table.begin() + (parentidx*SZ);


	// octant indicates which octant of the parent cell this cell occupies
	// 
	unsigned int octant = GetBits64(cellidx, NBITS-1,NBITS);

	if (! ((unsigned long) itr[CHILDMASK] & (0x1 << octant))) {
		SetErrMsg("Invalid cell id %d : has no children\n", cellidx+1);
		return(ERROR);
	}

	// find the first element in parent_table at the same refinement 
	// level as the parent.
	// 
	int lstart = get_start_of_level(parentidx);

	// Count the number of refined cells between the first at
	// the parent's level and this cell. The count will give the 
	// offset of *this* cell's entry in the parent_table array
	// from the start of the next level.
	// 
	int count = get_refined_cell_count(lstart, cellidx);

	// get start of next level where this cell will have an entry
	lstart = get_end_of_level(parentidx);

	lstart += count;

	// First child's cell id given by offset of it's parent in
	// parentCells array
	//
	return(((unsigned int) lstart << NBITS) + 1);

}

int	AMRTreeBranch::GetCellLevel(UInt32 cellid) const {

	SetDiagMsg("AMRTreeBranch::GetCellLevel(%u)", cellid);

	vector <long> &parent_table = _rootNode->GetElementLong(_parentTableTag);

	if (cellid >= _numNodes) {
		SetErrMsg("Invalid cell id %d\n", cellid);
		return(ERROR);
	}

	if (cellid == 0) return(0);	// root level

	// Internally the Id of the first child after the root node is 0
	UInt32 cellidx = cellid -1;

	int parentidx = cellidx >> NBITS;
	assert((parentidx * SZ) < parent_table.size());

	vector<long>::const_iterator itr;
	itr = parent_table.begin() + (parentidx*SZ);

	return(itr[LEVEL] + 1);
}

	 

AMRTreeBranch::UInt32	AMRTreeBranch::GetCellNeighbor(
	UInt32 cellid, int face
) const {

	SetDiagMsg("AMRTreeBranch::GetCellNeighbor(%u, %d)", cellid, face);

	if (cellid >= _numNodes) {
		SetErrMsg("Invalid cell id %d\n", cellid);
		return(ERROR);
	}

	return(ERROR);
}

AMRTreeBranch::UInt32	AMRTreeBranch::GetCellParent(
	UInt32 cellid
) const {

	SetDiagMsg("AMRTreeBranch::GetCellParent(%u)", cellid);

	if (cellid >= _numNodes) {
		SetErrMsg("Invalid cell id %d\n", cellid);
		return(ERROR);
	}

	if (cellid < 1) {
		SetErrMsg("Invalid cell id %d\n", cellid);
		return(ERROR);
	}

	if (cellid < (NREGIONS+1)) return (0); // root cell

	// Internally the Id of the first child after the root node is 0
	UInt32 cellidx = cellid -1;

	// parent's offset into the parent_table array. Note, this is 
	// *not* the same as the parents cell id
	//
	int parentidx = cellidx >> NBITS;

	// Get the offset in the parent_table array between the 
	// parent and the first cell at the parent's level.
	// 
	int offset = parentidx - get_start_of_level(parentidx) + 1;

	// Compute the parent's cell id by finding the parent's parent
	// entry in the parentCell array.
	//
	return(get_ith_refined_cell(parentidx-1, offset) + 1);

}

AMRTreeBranch::UInt32 AMRTreeBranch::GetNumNodes(
	int refinementlevel
) const {

	SetDiagMsg("AMRTreeBranch::GetNumNodes(%d)", refinementlevel);

	vector <long> &ref_level = _rootNode->GetElementLong(_refinementLevelTag);
	vector <long> &parent_table = _rootNode->GetElementLong(_parentTableTag);

	if (refinementlevel < 0) refinementlevel = ref_level[0];

	if (refinementlevel > ref_level[0]) {
		refinementlevel = ref_level[0];
	}

	UInt32 nnodes = 1;
	for(int i = 0; i<parent_table.size(); i+=SZ) {

		vector<long>::const_iterator itr;
		itr = parent_table.begin() + i;

		if (itr[LEVEL] <= refinementlevel) nnodes += NREGIONS;
	}
	return(nnodes);
}
		

int	AMRTreeBranch::HasChildren(
	UInt32 cellid
) const {

	SetDiagMsg("AMRTreeBranch::HasChildren(%u)", cellid);

	vector <long> &parent_table = _rootNode->GetElementLong(_parentTableTag);

	if (cellid == 0) return(parent_table.size() > 0);	// root cell

	// Internally the Id of the first child after the root node is 0
	UInt32 cellidx = cellid -1;

	int parentidx = cellidx >> NBITS;
	if (parentidx*SZ >= parent_table.size()) return(0);

	vector<long>::const_iterator itr;
	itr = parent_table.begin() + (parentidx*SZ);


	// octant indicates which octant of the parent cell this cell occupies
	// 
	unsigned int octant = GetBits64(cellidx, NBITS-1,NBITS);

	if (itr[CHILDMASK] & (0x1 << octant)) return(1);

	return(0);
}



//
// It is an error to try and refine a cell that is already refined. I.e. 
// cellid must refer to a leaf node.
//
AMRTreeBranch::UInt32	AMRTreeBranch::RefineCell(
	UInt32 cellid
) {

	SetDiagMsg("AMRTreeBranch::RefineCell(%u)", cellid);

	vector <long> &ref_level = _rootNode->GetElementLong(_refinementLevelTag);
	vector <long> &parent_table = _rootNode->GetElementLong(_parentTableTag);

	if (cellid >= _numNodes) {
		SetErrMsg("Invalid cell id %d\n", cellid);
		return(ERROR);
	}

	vector <long> cell(SZ, 0);
	cell.reserve(SZ);
	vector<long>::iterator cellitr0 = cell.begin();
	vector<long>::iterator cellitr1 = cell.end();
	
	vector<long>::iterator itr;

	// is this the root cell? Special handling required
	//
	if (cellid == 0) {


		if (parent_table.size()) {
			SetErrMsg("Can't refine cell %d\n : already refined", cellid);
			return(ERROR);
		}

		// this is redundant
		//
		cell[XLOC] = 0;
		cell[YLOC] = 0;
		cell[ZLOC] = 0;
		cell[CHILDMASK] = 0;	//no child of the children are refined yet
		cell[LEVEL] = 0;		// refinement level of parent cell

		itr = parent_table.begin();
		parent_table.insert(itr, cellitr0, cellitr1);


		ref_level[0] = 1;

		_numNodes += NREGIONS;
		return(0+1);	// First child of root cell
		
	}

	UInt32 cellidx = cellid -1;


	// Get the offset into the parentsCell array  for the parent of 
	// this cell;
	//
	int parentidx = cellidx >> NBITS;
	assert((parentidx * SZ) < parent_table.size());

	unsigned int octant = GetBits64(cellidx, NBITS-1,NBITS);

	itr = parent_table.begin() + (parentidx*SZ);

	if ((unsigned int) itr[CHILDMASK] & (0x1 << octant)) {
		SetErrMsg("Can't refine cell %d : already refined", cellidx+1);
		return(ERROR);
	} 

	itr[CHILDMASK] = (unsigned int) itr[CHILDMASK] | (0x1 << octant);

	// calulate the location codes from the parent based on which
	// of the parent's octants are being refined.
	//
	cell[XLOC] = (unsigned int) itr[XLOC] << 1;
	cell[YLOC] = (unsigned int) itr[YLOC] << 1;
	cell[ZLOC] = (unsigned int) itr[ZLOC] << 1;

	if (cellidx & (0x1 << 0)) cell[XLOC] += 1;	// X+1 octant
	if (cellidx & (0x1 << 1)) cell[YLOC] += 1;	// Y+1 octant
	if (cellidx & (0x1 << 2)) cell[ZLOC] += 1;	// Z+1 octant

	cell[LEVEL] = itr[LEVEL] + 1;
	cell[CHILDMASK] = 0;

	if (cell[LEVEL] + 1 > ref_level[0]) {
		ref_level[0] = cell[LEVEL] + 1;
	}

	// Get offset into parentCells to the first element at 
	// the parent's level. Count refined cells between first 
	// cell at this level and the refined cell we're about to add
	//
	int lstart = get_start_of_level(parentidx);
	int count = get_refined_cell_count(lstart, cellidx);

	// 
	// Jump to next level and add an entry for the newly refined cell
	//
	lstart = get_end_of_level(parentidx);

	lstart += count;

	itr = parent_table.begin() + (lstart * SZ);
	parent_table.insert(itr, cellitr0, cellitr1);

	_numNodes += NREGIONS;

	// First child's cell id given by offset of it's parent in
	// parentCells array
	//
	return(((unsigned int) lstart << NBITS) + 1);

}

int AMRTreeBranch::SetParentTable(const vector <long> &table) {

	SetDiagMsg("AMRTreeBranch::SetParentTable()");

	vector <long> &parent_table = _rootNode->GetElementLong(_parentTableTag);
	vector <long> &ref_level = _rootNode->GetElementLong(_refinementLevelTag);

	if (table.size() % SZ) {
		SetErrMsg("Invalid parent table");
		return(-1);
	}

	parent_table.clear();
	parent_table.reserve(table.size());

	for(int i=0; i<table.size(); i++) {
		parent_table.push_back(table[i]);
	}

	ref_level[0] = 0;
	for(int parentidx=0; parentidx<(table.size()/SZ); parentidx++) {

		long level = parent_table[parentidx*SZ+LEVEL];
		if (level > ref_level[0]) ref_level[0] = level;
	}
	ref_level[0] += 1;

	int nparents = parent_table.size() / SZ;

	_numNodes = (nparents + 1) * NREGIONS;
	return(0);
}

	

int AMRTreeBranch::get_start_of_level(int parentidx) const {

	vector <long> &parent_table = _rootNode->GetElementLong(_parentTableTag);

	assert(parentidx >= 0 && (parentidx * SZ) < parent_table.size());

	int startidx = parentidx;

	long level = parent_table[parentidx*SZ+LEVEL];

	while (startidx>= 0 && parent_table[startidx*SZ+LEVEL] == level) {
		startidx--;
	}
	startidx++;

	return(startidx);
}

int AMRTreeBranch::get_end_of_level(int parentidx) const {

	vector <long> &parent_table = _rootNode->GetElementLong(_parentTableTag);

	assert(parentidx >= 0 && (parentidx * SZ) < parent_table.size());

	int startidx = parentidx;

	long level = parent_table[parentidx*SZ+LEVEL];

	while (startidx*SZ<parent_table.size() && parent_table[startidx*SZ+LEVEL] == level) {
		startidx++;
	}
	startidx--;

	return(startidx);
}

int AMRTreeBranch::get_ith_refined_cell(
	int parentidx, 
	unsigned int ithcell
) const {

	vector <long> &parent_table = _rootNode->GetElementLong(_parentTableTag);

	assert(parentidx >= 0 && (parentidx * SZ) < parent_table.size());
	
	long level = parent_table[parentidx*SZ+LEVEL];

	int	count = 0;
	while (
		level == parent_table[parentidx*SZ+LEVEL] && 
		parentidx*SZ < parent_table.size()) {

		for (int i = 0; i<NREGIONS; i++) {
			if ((unsigned long) parent_table[parentidx*SZ+CHILDMASK] & (0x1 << i)) {
				count++;
			}
			if (count == ithcell) {
				return ((parentidx << NBITS-1) + i);
			}
		}
		parentidx++;
	}
	assert(level == parent_table[parentidx*SZ+LEVEL] && 
		parentidx*SZ < parent_table.size());

	return(0);
}

int AMRTreeBranch::get_refined_cell_count(
	int parentidx, 
	unsigned int cellidx
) const {

	vector <long> &parent_table = _rootNode->GetElementLong(_parentTableTag);

	assert(parentidx >= 0 && (parentidx * SZ) < parent_table.size());

	int	idx1 = cellidx >> NBITS;

	assert((idx1 * SZ) < parent_table.size());
	
	unsigned int octant = GetBits64(cellidx, NBITS-1,NBITS);

	unsigned int count = 0;
	for (int i = parentidx; i<=idx1; i++) {
		int jlast = i == idx1 ? octant : NREGIONS-1;
		for (int j = 0; j<=jlast; j++) {

			if (parent_table[i*SZ+CHILDMASK] & (0x1 << j)) count++;
		}
	}
	return(count);
}
