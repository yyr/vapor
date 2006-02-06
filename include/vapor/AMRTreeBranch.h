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
//	File:		AMRTreeBranch.h
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Thu Jan 5 16:59:42 MST 2006
//
//	Description:	
//
//
#ifndef	_AMRTreeBranch_h_
#define	_AMRTreeBranch_h_

#include <vector>
#include <vaporinternal/common.h>
#include <vapor/MyBase.h>
#include <vapor/EasyThreads.h>
#include <vapor/XmlNode.h>


namespace VAPoR {

//
//! \class AMRTreeBranch
//! \brief This class manages an octree data structure
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! This class manages a branch of Adaptive Mesh Refinement tree data 
//! structure. 
//! In actuality, the tree data structure is simply an octree. There is
//! not much particular about it with regard to AMR. The root of the
//! tree has a single node. Nodes are refined by subdivision into 
//! octants. Each node has an integer cell identifier.  
//! Cell id's are ordered sequentially in a breath first traversal
//! of the octree. The root node has id 0 (zero). The root's children
//! are numbered 1-8, with child #1 coresponding to the octant with lowest
//! coordinate bounds and child #8 coresponding to the octant with the highest
//! coordinate bounds. 
//!
//! This class is derived from the MyBase base
//! class. Hence all of the methods make use of MyBase's
//! error reporting capability - the success of any method
//! (including constructors) can (and should) be tested
//! with the GetErrCode() method. If non-zero, an error
//! message can be retrieved with GetErrMsg().
//!
//!
//! No field data is stored with the AMRTreeBranch class.
//

#define GETBITS(TARG,POSS,N) \
    ((N)<32 ? (((TARG) >> ((POSS)+1-(N))) & ~(~0ULL << (N))) : \
    ((TARG) >> ((POSS)+1-(N))) )

#define SETBITS(TARG, POSS, N, SRC) \
        (TARG) &= ~(~((~0LL) << (N)) << (((POSS)+1) - (N))); \
        (TARG) |= (((SRC) & ~((~0ULL) << (N))) << (((POSS)+1) - (N)))
class AMRTreeBranch : public VetsUtil::MyBase {

public:

 //
 // Tag names for data stored in the XML tree
 //
 static const string _rootTag;			// root of the XML tree
 static const string _refinementLevelTag;	// max refinement depth of branch
 // List of parent cells in breath first traversal order
 static const string _parentTableTag;


 //
 // Attribute names for data stored in the XML tree
 //
 static const string _minExtentsAttr;	// min coord bounds of branch
 static const string _maxExtentsAttr;	// max coord bounds of branch
 static const string _locationAttr;		// toplogical coords of branch


 static const unsigned int	ERROR = ~0U;

 typedef unsigned int UInt32;

 //! Constructor for the AMRTreeBranch class.
 //!
 //! Contructs a root tree with no children. The refinement level is zero (0)
 //! \param[in] parent Parent node of XML tree
 //! \param[in] max_level The maximum refinement level permitted. The
 //! first level is 0 (zero).
 //! \param[in] min A three element array indicating the minimum X,Y,Z bounds 
 //! of the tree branch, specified in user coordinates.
 //! \param[in] max A three element array indicating the maximum X,Y,Z bounds 
 //! of the tree branch, specified in user coordinates.
 //! \param[in] location A three element integer array indicating the
 //! coordinates of this branch relative to other branches in an AMR tree.
 //!
 //! \sa AMRTree
 //
 AMRTreeBranch(
	XmlNode *parent,
	const double min[3],
	const double max[3],
	const size_t location[3]
 );

 ~AMRTreeBranch();

 //! Delete a node from the tree branch
 //!
 //! Deletes the indicated node from the tree branch
 //! \param[in] cellid Cell id of node to delete
 //!
 //! \retval status Returns a non-negative value on success
 //
 int	DeleteCell(UInt32 cellid);

 //! Find a cell containing a point.
 //!
 //! This method returns the cell id of the node containing a point.
 //! The coordinates of the point are specfied in the user coordinates
 //! defined by the during the branch's construction (it is assumed that
 //! the branch occupies Cartesian space and that all nodes are rectangular.
 //! By default, the leaf node containing the point is returned. However,
 //! an ancestor node may be returned by specifying limiting the refinement 
 //! level.
 //!
 //! \param[in] ucoord A three element array containing the coordintes
 //! of the point to be found. 
 //! \param[in] refinementlevel The refinement level of the cell to be
 //! returned. If -1, a leaf node is returned.
 //! \retval cellid A valid cell id is returned if the branch contains 
 //! the indicated point. Otherwise AMRTreeBranch::ERROR is returned.
 //
 UInt32	FindCell(const double ucoord[3], int refinementlevel = -1) const;

 //! Return the user coordinate bounds of a cell
 //!
 //! This method returns the minimum and maximum extents of the 
 //! indicated cell. The extents are in user coordinates as defined
 //! by the constructor for the class. The bounds for the root cell
 //! are guaranteed to be the same as those used to construct the class.
 //!
 //! \param[in] cellid The cell id of the cell whose bounds are to be returned
 //! \param[out] minu A three element array to which the minimum cell 
 //! extents will be copied.
 //! \param[out] maxu A three element array to which the maximum cell 
 //! extents will be copied.
 //! \retval status Returns a non-negative value on success
 //!
 int	GetCellBounds(
	UInt32 cellid, double minu[3], double maxu[3]
 ) const;

 //! Return the topological coordinates of a cell within the branch.
 //!
 //! This method returns topological coordinates of a cell within
 //! the tree, relative to the cell's refinement level. Each cell
 //! in a branch has an i-j-k location for it's refinement level. The range
 //! of i-j-k values runs from \e min to \e max, where \e min is given by
 //! location * 2^j, where \e location is provided by the \p location
 //! parameter of the constructor and \e j is the refinement level
 //! of the cell. The value of \e max is \min + 2^j, where \e j is
 //! again the cell's refinement level. Hence, the cell's location
 //! is relative to the branch's location.
 //!
 //! \param[in] cellid The cell id of the cell whose bounds are to be returned
 //! \param[out] maxu A three element integer array to which the cell 
 //! location will be copied
 //! \retval status Returns a non-negative value on success
 //!
 int	GetCellLocation(UInt32 cellid, unsigned int xyz[3]) const;

 // Returns the cell id for the first of the eight children of the 
 // cell with cell id, 'cellid'. The cell ids for the remaining seven
 // children may be contructed by incrementing the returned id
 //

 //! Returns the cell id of the first child of a node
 //!
 //! This method returns the cell id of the first of the eight children
 //! of the cell indicated by \p cellid. The remaining seven children's
 //! ids may be calulated by incrementing the first child's id successively.
 //! I.e. the children have contiguous integer ids. Octants are ordered
 //! with the X axis varying fastest, followed by Y, then Z.
 //!
 //! \param[in] cellid The cell id of the cell whose first child is
 //! to be returned.
 //! \retval cellid A valid cell id is returned if the branch contains 
 //! the indicated point. Otherwise AMRTreeBranch::ERROR is returned.
 //
 UInt32	GetCellChildren(UInt32 cellid) const;

 //! Return the refinement level of a cell
 //!
 //! Returns the refinement level of the cell indicated by \p cellid. 
 //! The base (coarsest) refinement level is zero (0).
 //!
 //! \param[in] cellid The cell id of the cell whose refinement level is
 //! to be returned.
 //! \retval status Returns a non-negative value on success
 //
 int	GetCellLevel(UInt32 cellid) const;


 //! Return the cell id of a cell's neighbor.
 //!
 //! Returns the cell id of the cell adjacent to the indicated face
 //! of the cell with id \p cellid. The \p face parameter is an integer
 //! in the range [0..5], where 0 coresponds to the face on the XY plane 
 //! with Z = 0; 1 is the YZ plane with X = 1; 2 is the XY plane, Z = 1;
 //! 3 is the YZ plane; Z = 0, 4 is the XZ plane, Y = 0; and 5 is the
 //! XZ plane, Z = 1.
 //!
 //! \param[in] cellid The cell id of the cell whose neighbor is
 //! to be returned.
 //! \param[in] face Indicates the cell's face adjacent to the desired
 //! neighbor.
 //! \retval cellid A valid cell id is returned if the branch contains 
 //! the indicated point. Otherwise AMRTreeBranch::ERROR is returned.
 UInt32	GetCellNeighbor(UInt32 cellid, int face) const;

 //! Return number of nodes in the branch
 //!
 //! Returns the total number of nodes in a branch, including parent nodes
 //!
 UInt32 GetNumNodes() const {return (_numNodes); };

 //! Return number of nodes in the branch
 //!
 //! Returns the total number of nodes in a branch, including parent nodes,
 //! up to and including the indicated refinement level.
 //!
 UInt32 GetNumNodes(int refinementlevel = -1) const;

 //! Returns the cell id of the parent of a child node
 //!
 //! This method returns the cell id of the parent 
 //! of the cell indicated by \p cellid. 
 //!
 //! \param[in] cellid The cell id of the cell whose parent is
 //! to be returned.
 //! \retval cellid A valid cell id is returned if the branch contains 
 //! the indicated point. Otherwise AMRTreeBranch::ERROR is returned.
 //
 UInt32	GetCellParent(UInt32 cellid) const;

 //! Returns the maximum refinement level of any cell in this branch
 //
 int	GetRefinementLevel() const {
	return ((int) (_rootNode->GetElementLong(_refinementLevelTag)[0]));
 };

 //! Returns the cellid root node
 //
 UInt32	GetRoot() const {return(0);};

 //! Returns true if the cell with id \p cellid has children. 
 //! 
 //! Returns true or false indicated whether the specified cell
 //! has children or not.
 //
 int	HasChildren(UInt32 cellid) const;


 //! Refine a cell
 //!
 //! This method refines a cell, creating eight new child octants. Upon
 //! success the cell id of the first child is returned.
 //!
 //! \param[in] cellid The cell id of the cell to be refined
 //! \retval cellid A valid cell id is returned if the branch contains 
 //! the indicated point. Otherwise AMRTreeBranch::ERROR is returned.
 //!
 //
 AMRTreeBranch::UInt32	RefineCell(UInt32 cellid);

 int SetParentTable(const vector<long> &table);

private:


 size_t _location[3];    // Topological coords of branch  in blocks
 double _minBounds[3];  // Min domain extents expressed in user coordinates
 double _maxBounds[3];  // Max domain extents expressed in user coordinates

 // Offsets into parentTable array
 //
 //static const int YLOC = XLOC+1;	// X,Y,Z coordinates of cell's corner
 //static const int ZLOC = YLOC+1;
 //static const int CHILDMASK = ZLOC+1;	// 8-bit mask indicating which, 
 //static const int LEVEL = CHILDMASK+1;// refinement level of this parent cell
				// if any, of the children of 
				// this parent node
				// are themselves parents
 //static const int SZ = LEVEL+1;	// Size of record;

 static const int XLOC;			
 static const int YLOC;
 static const int ZLOC;
 static const int CHILDMASK;
 static const int LEVEL;	
 static const int SZ;

 long long _numNodes;			// Number of nodes in tree


 XmlNode	*_rootNode;	// root of XML tree used to represent AMR tree

 // Return the offset for the first element in the _parentsCells
 // array that has the same level as the element 'idx'. 
 //
 int	get_start_of_level(int idx) const;

 // Return the offset for the last element in the _parentsCells
 // array that has the same level as the element 'idx'. 
 //
 int	get_end_of_level(int idx) const;

 // Return the cell id for the ith refined cell from the _parentCells
 // array, starting from the 'parentidx' element. The ith parent will
 // be at the same level as parentidx 
 //
 int get_ith_refined_cell(
    int parentidx,
    unsigned int ithcell
 ) const;

 // Return the count of refined cells from the _parentCells array
 // between the first cellid at offset 'parentidx' and the cell
 // with cellid, 'cellid', inclusive.
 //
 int get_refined_cell_count(
    int parentidx,
    unsigned int cellid
 ) const;

 unsigned int getbits(
    unsigned int x, int p, int n
 ) {
    return((x >> (p+1-n)) & ~(~0 << n));
 }

};

};

#endif	//	_AMRTreeBranch_h_
