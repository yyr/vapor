//
//      $Id$
//


#ifndef	_Metadata_h_
#define	_Metadata_h_

#include <stack>
#include <expat.h>
#include <vapor/MyBase.h>
#include <vaporinternal/common.h>
#include "vapor/XmlNode.h"
#include "vapor/ExpatParseMgr.h"
#ifdef WIN32
#pragma warning(disable : 4251)
#endif

namespace VAPoR {

//
//! \class Metadata
//! \brief A class for managing data set metadata
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//!
class VDF_API Metadata {
public:

 enum VarType_T {
	VARUNKNOWN = -1,
	VAR3D, VAR2D_XY, VAR2D_XZ, VAR2D_YZ
 };

 virtual ~Metadata() {};

 //! Return the internal blocking factor use for WaveletBlock files
 //!
 //! \retval size Internal block factor
 //
 virtual const size_t *GetBlockSize() const = 0;

 //! Returns the X,Y,Z coordinate dimensions of the data in grid coordinates
 //! \retval dim A three element vector containing the voxel dimension of 
 //! the data at its native resolution
 //!
 //!
 virtual const size_t *GetDimension() const = 0;

 //! Return number of transformations in hierarchy 
 //!
 //!
 //! \note Starts from 0 or 1????
 //
 virtual int GetNumTransforms() const = 0;

 //! Return the domain extents specified in user coordinates
 //!
 //! \retval extents A six-element array containing the min and max
 //! bounds of the data domain in user-defined coordinates
 //!
 //
 virtual const vector<double> &GetExtents() const = 0;

 //! Return the number of time steps in the collection
 //!
 //! \retval value The number of time steps or a negative number on error
 //!
 //
 virtual long GetNumTimeSteps() const = 0;


 //! Return the names of the variables in the collection 
 //!
 //! \retval value is a space-separated list of variable names
 //!
 //
 virtual const vector <string> &GetVariableNames() const = 0;

 //! Return the names of the 3D variables in the collection 
 //!
 //! \retval value is a space-separated list of 3D variable names
 //!
 //
 virtual const vector <string> &GetVariables3D() const = 0;

 //! Return the names of the 2D, XY variables in the collection 
 //!
 //! \retval value is a space-separated list of 2D XY variable names
 //!
 //
 virtual const vector <string> &GetVariables2DXY() const = 0;
 virtual const vector <string> &GetVariables2DXZ() const = 0;
 virtual const vector <string> &GetVariables2DYZ() const = 0;

 //! Return a three-element boolean array indicating if the X,Y,Z
 //! axes have periodic boundaries, respectively.
 //!
 //! \retval boolean-vector  
 //!
 //
 virtual const vector<long> &GetPeriodicBoundary() const = 0;

 //! Return a three-element integer array indicating the coordinate
 //! ordering permutation.
 //!
 //! \retval integer-vector  
 //!
 //! \remarks Optional element 
 //
 virtual const vector<long> &GetGridPermutation() const = 0;

 //! Return the time for a time step, if it exists, 
 //!
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \retval value A single element vector specifying the time
 //!
 //
 virtual const vector<double> &GetTSUserTime(size_t ts) const = 0;

 virtual void GetTSUserTimeStamp(size_t ts, string &s) const = 0;

 //! Return the domain extents specified in user coordinates
 //! for the indicated time step
 //!
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \retval extents A six-element array containing the min and max
 //! bounds of the data domain in user-defined coordinates.
 //! An empty vector is returned if the extents for the specified time
 //! step is not defined.
 //!
 //! \remarks Optional element
 //
 virtual const vector<double> &GetTSExtents(size_t ts) const = 0;

 //! Get the dimension of a volume
 //! 
 //! Get the resulting dimension of the volume
 //! after undergoing a specified number of forward transforms.
 //! If the number of transforms is zero, and the
 //! grid type is not layered, the
 //! value returned is the native volume dimension as specified in the
 //! Metadata structure used to construct the class.
 //! With layered data, the third coordinate depends
 //! on the user-specified interpolation.
 //! \param[in] reflevel Refinement level of the variable
 //! \param[out] dim Transformed dimension.
 //!
 //! \sa Metadata::GetDimension()
 //
 virtual void   GetDim(size_t dim[3], int reflevel = 0) const;

 //! Get dimension of a volume in blocks
 //!
 //! Performs same operation as GetDim() except returns
 //! dimensions in block coordinates instead of voxels.
 //! \param[in] reflevel Refinement level of the variable. A value of -1
 //! indicates the maximum refinment level defined for the VDC
 //! \param[out] bdim Transformed dimension in blocks.
 //!
 //! \sa Metadata::GetDimension()
 //
 virtual void   GetDimBlk(size_t bdim[3], int reflevel = 0) const; 


 //! Map integer voxel coordinates into integer block coordinates. 
 //! 
 //! Compute the integer coordinate of the block containing
 //! a specified voxel. 
 //! \param[in] vcoord Coordinate of input voxel in integer (voxel)
 //! coordinates
 //! \param[out] bcoord Coordinate of block in integer coordinates containing
 //! the voxel. 
 //!
 virtual void	MapVoxToBlk(const size_t vcoord[3], size_t bcoord[3]) const;
		

 //! Map integer voxel coordinates to user-defined floating point coords.
 //!
 //! Map the integer coordinates of the specified voxel to floating
 //! point coordinates in a user defined space. The voxel coordinates,
 //! \p vcoord0 are specified relative to the refinement level
 //! indicated by \p reflevel for time step \p timestep.  
 //! The mapping is performed by using linear interpolation 
 //! The user-defined coordinate system is obtained
 //! from the Metadata structure passed to the class constructor.
 //! The user coordinates are returned in \p vcoord1.
 //! Results are undefined if vcoord is outside of the volume 
 //! boundary.
 //!
 //! \param[in] timestep Time step of the variable 
 //! \param[in] vcoord0 Coordinate of input voxel in integer (voxel)
 //! coordinates
 //! \param[out] vcoord1 Coordinate of transformed voxel in user-defined,
 //! floating point  coordinates
 //! \param[in] reflevel Refinement level of the variable. A value of -1
 //! indicates the maximum refinment level defined for the VDC
 //!
 //! \sa Metatdata::GetGridType(), Metadata::GetExtents(), 
 //! GetTSXCoords()
 //
 virtual void	MapVoxToUser(
	size_t timestep,
	const size_t vcoord0[3], double vcoord1[3], int ref_level = 0
 ) const;

 //! Map floating point coordinates to integer voxel offsets.
 //!
 //! Map floating point coordinates, specified relative to a 
 //! user-defined coordinate system, to the closest integer voxel 
 //! coordinates for a voxel at a given refinement level. 
 //! The integer voxel coordinates, \p vcoord1, 
 //! returned are specified relative to the refinement level
 //! indicated by \p reflevel for time step, \p timestep.
 //! The mapping is performed by using linear interpolation 
 //! The user defined coordinate system is obtained
 //! from the Metadata structure passed to the class constructor.
 //! Results are undefined if \p vcoord0 is outside of the volume 
 //! boundary.
 //!
 //! If a user coordinate system is not defined for the specified
 //! time step, \p timestep, the global extents for the VDC will 
 //! be used.
 //!
 //! \param[in] timestep Time step of the variable 
 //! \param[in] vcoord0 Coordinate of input point in floating point
 //! coordinates
 //! \param[out] vcoord1 Integer coordinates of closest voxel, at the 
 //! indicated refinement level, to the specified point.
 //! integer coordinates
 //! \param[in] reflevel Refinement level of the variable. A value of -1
 //! indicates the maximum refinment level defined for the VDC
 //!
 //! \sa Metatdata::GetGridType(), Metadata::GetExtents(), 
 //! GetTSXCoords()
 //
 virtual void	MapUserToVox(
	size_t timestep,
	const double vcoord0[3], size_t vcoord1[3], int reflevel = 0
 ) const;

 //! Map floating point coordinates to integer block offsets.
 //!
 //! Map floating point coordinates, specified relative to a 
 //! user-defined coordinate system, to integer coordinates of the block
 //! containing the point at a given refinement level. 
 //! The integer voxel coordinates, \p vcoord1
 //! are specified relative to the refinement level
 //! indicated by \p reflevel for time step, \p timestep.
 //! The mapping is performed by using linear interpolation 
 //! The user defined coordinate system is obtained
 //! from the Metadata structure passed to the class constructor.
 //! The user coordinates are returned in \p vcoord1.
 //! Results are undefined if \p vcoord0 is outside of the volume 
 //! boundary.
 //!
 //! If a user coordinate system is not defined for the specified
 //! time step, \p timestep, the global extents for the VDC will 
 //! be used.
 //!
 //! \param[in] timestep Time step of the variable 
 //! \param[in] vcoord0 Coordinate of input point in floating point
 //! coordinates
 //! \param[out] vcoord1 Integer coordinates of block containing the point
 //! \param[in] reflevel Refinement level of the variable. A value of -1
 //! indicates the maximum refinment level defined for the VDC
 //!
 //! \sa Metatdata::GetGridType(), Metadata::GetExtents(), 
 //! GetTSXCoords()
 //
 virtual void	MapUserToBlk(
	size_t timestep,
	const double vcoord0[3], size_t bcoord0[3], int reflevel = 0
 ) const {
	size_t v[3];
	MapUserToVox(timestep, vcoord0, v, reflevel);
	Metadata::MapVoxToBlk(v, bcoord0);
 }


 virtual VarType_T Metadata::GetVarType(const string &varname) const; 

 //! Return true if indicated region coordinates are valid
 //!
 //! Returns true if the region defined by \p reflevel,
 //! \p min, \p max is valid. I.e. returns true if the indicated
 //! volume subregion is contained within the volume. Coordinates are
 //! specified relative to the refinement level.
 //! 
 //! \param[in] min Minimum region extents in voxel coordinates
 //! \param[in] max Maximum region extents in voxel coordinates
 //! \retval boolean True if region is valid
 //! \param[in] reflevel Refinement level of the variable. A value of -1
 //! indicates the maximum refinment level defined for the VDC
 //
 virtual int IsValidRegion(
	const size_t min[3], const size_t max[3], int reflevel = 0
 ) const;

 //! Return true if indicated region coordinates are valid
 //!
 //! Returns true if the region defined by \p reflevel, and the block
 //! coordinates
 //! \p min, \p max are valid. I.e. returns true if the indicated
 //! volume subregion is contained within the volume.
 //! Coordinates are
 //! specified relative to the refinement level.
 //!
 //! \param[in] min Minimum region extents in block coordinates
 //! \param[in] max Maximum region extents in block coordinates
 //! \param[in] reflevel Refinement level of the variable. A value of -1
 //! indicates the maximum refinment level defined for the VDC
 //! \retval boolean True if region is valid
 //
 virtual int IsValidRegionBlk(
	const size_t min[3], const size_t max[3], int reflevel = 0
 ) const;


};


};

#endif	//	_Metadata_h_
