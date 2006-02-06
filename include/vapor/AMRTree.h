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
//	File:		AMRTree.h
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Thu Jan 5 16:58:05 MST 2006
//
//	Description:	
//
//

#ifndef	_AMRTree_h_
#define	_AMRTree_h_

#include <vector>
#include <vaporinternal/common.h>
#include <vapor/MyBase.h>
#include <vapor/EasyThreads.h>
#include <vapor/AMRTreeBranch.h>
#include <vapor/ExpatParseMgr.h>


namespace VAPoR {

//
//! \class AMRTree
//! \brief This class manages an AMR tree data structure
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! This class manages an Adaptive Mesh Refinement tree data
//! object for a block structured AMR grid.
//! The AMR grid is composed of a Cartesian base grid of cells
//! (blocks) that are recursively refined into octants. Each base
//! block is represented by a AMRTreeBranch object class.
//!
//! This class is derived from the MyBase base
//! class. Hence all of the methods make use of MyBase's
//! error reporting capability - the success of any method
//! (including constructors) can (and should) be tested
//! with the GetErrCode() method. If non-zero, an error
//! message can be retrieved with GetErrMsg().
//!
//! No field data is stored with the AMRTreeBranch class.
//!
//! \sa AMRTreeBranch
//

class AMRTree : public VetsUtil::MyBase, public ParsedXml {

public:

 typedef long long CellID;



 //! Constructor for the AMRTree class.
 //!
 //! \param[in] basedim A three element array specifying the topological
 //! dimensions of the tree in base blocks
 //! \param[in] max_level The maximum refinment level permitted by
 //! tree. Optimal performance results are achieved if this value matches
 //! that of the tree once it is populated.
 //! \param[in] min A tree element array specifying the minimum XYZ extents
 //! of the tree in user-defined coordinates.
 //! \param[in] max A tree element array specifying the maximum XYZ extents
 //! of the tree in user-defined coordinates.
 //!
 //! \retval status A non-negative value is returned on success
 //
 AMRTree(
	const size_t basedim[3], 
	const double min[3], 
	const double max[3]
 );

 AMRTree(
	const string &path
 );

 //! Constructor an AMRTree from a Paramesh AMR grid
 //!
 //! This constructor initializes and fully populates an AMR tree from
 //! a Paramesh block structured AMR grid stored in an HDF5 file.
 //! The arguments to this constructor are read directly from HDF5
 //!
 //! param[in] paramesh_gids An array of Paramesh global identifiers
 //! param[in] paramesh_bboxs An array of Paramesh bounding boxes
 //! param[in] paramesh_refine_levels An array of Paramesh refinement levels
 //! param[in] paramesh_total_blocks Total number of nodes (both leaf and
 //! non-leaf) stored in the Paramesh AMR tree.
 //!
 //! \sa http://ct.gsfc.nasa.gov/paramesh/Users_manual/amr.html
 //! \sa http://flash.uchicago.edu/website/home/
 //
 AMRTree(
	const int paramesh_gids[][15],
	const float paramesh_bboxs [][3][2],
	const int paramesh_refine_levels[],
	int paramesh_total_blocks
 );

 AMRTree();

 virtual ~AMRTree();

 //! Decode an AMRTree cell id 
 //!
 //! Decode an AMRTree cell indentifier into a AMRTreeBranch cell id 
 //! and a base block index (branch index). 
 //!
 //! \param[in] cellid The AMRTree cell id to decode
 //! \param[out] baseblockidx The decoded branch index
 //! \param[out] nodeidx The decoded AMRTreeBranch cell id
 //!
 //! \sa AMRTreeBranch
 //
 void	DecodeCellID(
	CellID cellid,
	AMRTreeBranch::UInt32 *baseblockidx,
	AMRTreeBranch::UInt32 *nodeidx
 ) const;

 //! Delete a node from the tree 
 //!
 //! Deletes the indicated node from the tree 
 //! \param[in] cellid Cell id of node to delete
 //!
 //! \retval status Returns a non-negative value on success
 //
 int	DeleteCell(CellID cellid);


 //! Find a cell containing a point.
 //!
 //! This method returns the cell id of the node containing a point.
 //! The coordinates of the point are specfied in the user coordinates
 //! defined by the during the tree's construction (it is assumed that
 //! the tree occupies Cartesian space and that all nodes are rectangular.
 //! By default, the leaf node containing the point is returned. However,
 //! an ancestor node may be returned by specifying limiting the refinement
 //! level.
 //!
 //! \param[in] ucoord A three element array containing the coordintes
 //! of the point to be found.
 //! \param[in] reflevel The refinement level of the cell to be
 //! returned. If -1, a leaf node is returned.
 //! \retval cellid A valid cell id is returned if the tree contains
 //! the indicated point. Otherwise a negative integer is returned
 //
 CellID	FindCell(const double ucoord[3], int reflevel = -1) const;

 //! Get the dimensions of the base grid
 //!
 //! This method returns the XYZ dimensions of the tree's base grid
 //!
 //! \retval dim A three element array containing the tree's base grid
 //! dimensions.
 //
 const size_t *GetBaseDim() const { return(_baseDim); };

 //! Return the AMRTreeBranch associated with a base block
 //!
 //! This method returns a pointer to the AMRTreeBranch object for
 //! the block with topological coordinates given by \p xyz.
 //!
 //! \param[in] xyz The toplogical (i-j-k) coordinates of the base
 //! block tree branch to return
 //! \retval treebranch A pointer to a AMRTreeBranch object
 //
 const AMRTreeBranch	*GetBranch(const size_t xyz[3]) const;

 //! Return the user coordinate bounds of a region
 //!
 //! This method returns the minimum and maximum extents of the
 //! indicated region. By default the region is the entire domain of
 //! the tree. The extents are provided in user coordinates as defined
 //! by the constructor for the class. The bounds for the root cell
 //! are guaranteed to be the same as those used to construct the class.
 //! If the parmater \p cellid is non negative, the bounds of the cell with
 //! cell id \cellid are returned.
 //!
 //! \param[in] cellid The cell id of the cell whose bounds are to be returned
 //! \param[out] minu A three element array to which the minimum cell
 //! extents will be copied.
 //! \param[out] maxu A three element array to which the maximum cell
 //! extents will be copied.
 //! \retval status Returns a non-negative value on success
 //!
 int	GetBounds(double minu[3], double maxu[3], CellID cellid = -1) const;



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
 //! the indicated point. Otherwise a negative value is returned.
 //
 CellID	GetCellChildren(CellID cellid) const;


 //! Return the refinement level of a cell
 //!
 //! Returns the refinement level of the cell indicated by \p cellid.
 //! The base (coarsest) refinement level is zero (0).
 //!
 //! \param[in] cellid The cell id of the cell whose refinement level is
 //! to be returned.
 //! \retval status Returns a non-negative value on success
 //
 int	GetCellLevel(CellID cellid) const;



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
 //! the indicated point. Otherwise a negative value is returned
 //
 CellID	GetCellNeighbor(CellID cellid, int face) const;


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
 CellID	GetCellParent(CellID cellid) const;

 //! Returns the maximum refinement of the indicated subregion
 //!
 int    GetRefinementLevel(const size_t min[3], const size_t max[3]) const;

 //! Returns the maximum refinement of the entire tree
 //
 int    GetRefinementLevel() const; 

 //! Return number of nodes in the indicated subtree
 //!
 //! The number of leaf and non-leaf nodes in a subtree indicated by 
 //! \p min and \p max is returned. If the 
 //! \reflevel parameter is non-negative, the number of nodes
 //! in the tree up to the refinement level indicated by \p reflevel
 //! are returned.
 //!
 //! \param[in] min Minumum topological (i-j-k) coordinates, specified
 //! in base blocks, of the region of interest
 //! \param[in] max Maximum topological (i-j-k) coordinates, specified
 //! in base blocks, of the region of interest
 //! \param[in] reflevel Requested refinment level. The maximum 
 //! refinement level is used if the argument is negative
 //!
 //!
 CellID GetNumNodes(
	const size_t min [3], 
	const size_t max[3], 
	int reflevel = -1
 ) const;

 //! Return number of nodes in the tree
 //!
 //! The number of leaf and non-leaf nodes is returned. If the 
 //! \reflevel parameter is non-negative, the number of nodes
 //! in the tree up to the refinement level indicated by \p reflevel
 //! are returned.
 //!
 //! \param[in] reflevel Requested refinment level. The maximum 
 //! refinement level is used if the argument is negative
 //!
 //! \retval numnodes The number of nodes
 //
 CellID GetNumNodes(int reflevel = -1) const;


 //! Refine a cell
 //!
 //! This method refines a cell, creating eight new child octants. Upon
 //! success the cell id of the first child is returned.
 //!
 //! \param[in] cellid The cell id of the cell to be refined
 //! \retval cellid A valid cell id is returned if the branch contains
 //! the indicated point. Otherwise a negative value is returned
 //!
 CellID	RefineCell(CellID cellid);


 int	MapUserToVoxel(
	int reflevel, const double ucoord[3], size_t vcoord[3]
 );

 //! Write the AMR tree to a file as XML data
 //!
 //! \param[in] path Name of the file to write to
 //! \retval status Returns a non-negative integer on success
 //
 int Write(const string &path) const;

 //! Read the AMR tree from a file as XML data
 //!
 //! \param[in] path Name of the file to write to
 //! \retval status Returns a non-negative integer on success
 //
 int Read(const string &path);

 int _parameshGetBaseBlocks(
	vector <int> &baseblocks,
	size_t basedim[3],
	const int gids[][15],
	int totalblocks
 ) const;

private:

 static const string _rootTag;
 static const string _minExtentsAttr;
 static const string _maxExtentsAttr;
 static const string _baseDimAttr;
 static const string _fileVersionAttr;

 int _objIsInitialized;

 size_t _baseDim[3];	// Dimension of tree expressed in base blocks
 double _minBounds[3];	// Min domain extents expressed in user coordinates
 double _maxBounds[3];	// Max domain extents expressed in user coordinates


 AMRTreeBranch	**_treeBranches;	// One branch per base block;

 XmlNode    *_rootNode; // root of XML tree used to represent AMR tree 

 int _fileVersion;

 int	_AMRTree(
	const size_t basedim[3], 
	const double min[3], 
	const double max[3]
 );

 int paramesh_refine_baseblocks(
	int index,
	int pid,
	const int gids[][15]
 );

 bool elementStartHandler(
	ExpatParseMgr*, int depth , std::string& tag, const char **attr
 );
 bool elementEndHandler(ExpatParseMgr*, int depth , std::string&);

 // XML Expat element handler helps. A different handler is defined
 // for each possible state (depth of XML tree) from 0 to 3
 //
 void _startElementHandler0(ExpatParseMgr*,const string &tag, const char **attrs);
 void _startElementHandler1(ExpatParseMgr*,const string &tag, const char **attrs);
 void _startElementHandler2(ExpatParseMgr*,const string &tag, const char **attrs);
 void _endElementHandler0(ExpatParseMgr*,const string &tag);
 void _endElementHandler1(ExpatParseMgr*,const string &tag);
 void _endElementHandler2(ExpatParseMgr*,const string &tag);



};

};

#endif	//	_AMRTree_h_
