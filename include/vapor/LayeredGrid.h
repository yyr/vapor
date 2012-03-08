#ifndef _LayeredGrid_
#define _LayeredGrid_
#include <vapor/common.h>
#include "RegularGrid.h"

//
//! \class LayeredGrid
//!
//! \brief This class implements a 2D or 3D layered grid: a generalization
//! of a regular grid where the spacing of grid points along a single dimension
//! may vary at each grid point. The spacing along the remaining one (2D case)
//! or two (3D case) dimensions is invariant between grid points. For example,
//! if K is the layered dimension than the z coordinate is given by some
//! function f(i,j,k):
//!
//! z = f(i,j,k)
//!
//! where f() is monotonically increasing (or decreasing) with k.
//! The remaining x and y coordinates are givey by (i*dx, j*dy)
//! for some real dx and dy .
//!
//

class VDF_API LayeredGrid : public RegularGrid {
public:

 //!
 //! Construct a layered grid sampling a 3D or 2D scalar function
 //!
 //! \param[in] bs A three-element vector specifying the dimensions of
 //! each block storing the sampled scalar function.
 //! \param[in] min A three-element vector specifying the ijk index
 //! of the first point in the grid. The first grid point need not coincide 
 //! with
 //! block boundaries. I.e. the indecies need not be (0,0,0)
 //! \param[in] max A three-element vector specifying the ijk index
 //! of the last point in the grid
 //! \param[in] extents A six-element vector specifying the user coordinates
 //! of the first (first three elements) and last (last three elements) of
 //! the grid points indicated by \p min and \p max, respectively.
 //! The elements of the \p extents parameter corresponding to the
 //! varying dimension are ignored. The varying dimension extents are
 //! provided by the \p coords parameter.
 //! \param[in] periodic A three-element boolean vector indicating
 //! which i,j,k indecies, respectively, are periodic. The varying 
 //! dimension may not be periodic.
 //! \param[in] blks An array of blocks containing the sampled function.
 //! The dimensions of each block
 //! is given by \p bs. The number of blocks is given by the product
 //! of the terms:
 //!
 //! \code (max[i]/bs[i] - min[i]/bs[i] + 1) \endcode
 //!
 //! over i = 0..2.
 //!
 //! \param[in] coords An array of blocks with dimension and number the
 //! same as \p blks specifying the varying dimension grid point coordinates. 
 //! \param[in] varying_dim An enumerant indicating which axis is the
 //! varying dimension: 0 for I, 1 for J, 2 for K
 //!
 LayeredGrid(
	const size_t bs[3],
	const size_t min[3],
	const size_t max[3],
	const double extents[6],
	const bool periodic[3],
	float ** blks,
	float ** coords,
	int varying_dim
);

 //! 
 //! Construct a layered grid sampling a 3D or 2D scalar function
 //! that contains missing values.
 //!
 //! This constructor adds a parameter, \p missing_value, that specifies 
 //! the value of missing values in the sampled function. When 
 //! reconstructing the function at arbitrary coordinates special 
 //! consideration is given to grid points with missing values that 
 //! are used in the reconstruction.
 //! 
 //! \sa GetValue()
 //!
 LayeredGrid(
	const size_t bs[3],
	const size_t min[3],
	const size_t max[3],
	const double extents[6],
	const bool periodic[3],
	float ** blks,
	float ** coords,
	int varying_dim,
	float missing_value
 );

 virtual ~LayeredGrid();


 //! \copydoc RegularGrid::GetValue()
 //!
 float GetValue(double x, double y, double z) const;

 //! Return the extents of the user coordinate system
 //!
 //! This method returns min and max extents of the user coordinate 
 //! system defined on the grid. The minimum extent for the non-varying
 //! (non-layered) dimensions are the non-varying coordinates
 //! of the grid point with index(0,0,0). The maximum extent
 //! for the non-varying dimensions are the non-varying coordinates of 
 //! the grid point with index(nx-1, ny-1, nz-1), 
 //! where \a nx, \a ny, and \a nz are the I, J, K dimensions,
 //! respectively.
 //!
 //! The min extent of the varying (layered) dimension is given
 //! by the coordinate of the grid point on the i==0 plane, where \a i
 //! is the varying index, with the
 //! minimum varying coordinate value if the varying coordinate of the
 //! point (0,0,0) is less than the varying coordinate of the point
 //! (idim-1, jdim-1, kdim-1). Otherwise the varying coordinate along
 //! the i==0 plane with maximum value is used.
 //!
 //! The max extent of the varying dimension is calculated in a similar
 //! way for the i=dim-1 plane. Hence, the min and max extents define
 //! the corner points of the smallest box that bounds the grid
 //!
 //! \param[out] extents A six-element array, the first three values will
 //! contain the minimum coordinate, and the last three values the 
 //! maximum coordinate
 //!
 //! \sa GetDimension(), LayeredGrid()
 //!
 void GetUserExtents(double extents[6]) const;

 //! \copydoc RegularGrid::GetUserCoordinates()
 //!
 int GetUserCoordinates(
	size_t i, size_t j, size_t k, 
	double *x, double *y, double *z
 ) const;

 //! \copydoc RegularGrid::GetIJKIndex()
 //!
 void GetIJKIndex(
	double x, double y, double z,
	size_t *i, size_t *j, size_t *k
 ) const;

 //! \copydoc RegularGrid::GetIJKIndexFloor()
 //!
 void GetIJKIndexFloor(
	double x, double y, double z,
	size_t *i, size_t *j, size_t *k
 ) const;

 //! \copydoc RegularGrid::Reshape()
 //!
 int Reshape(
	const size_t min[3],
	const size_t max[3],
	const bool periodic[3]
 );

 //! \copydoc RegularGrid::InsideGrid()
 //!
 bool InsideGrid(double x, double y, double z) const;


 //! Set periodic boundaries
 //!
 //! This method changes the periodicity of boundaries set 
 //! by the class constructor. It overides the base class method
 //! to ensure the varying dimension is not periodic.
 //!
 //! \param[in] periodic A three-element boolean vector indicating
 //! which i,j,k indecies, respectively, are periodic. The varying dimension
 //! may not be periodic.
 //
 virtual void SetPeriodic(const bool periodic[3]);

 //! \copydoc RegularGrid::GetMinCellExtents()
 //!
 virtual void GetMinCellExtents(double *x, double *y, double *z) const;

private:
 float **_coords;
 int _varying_dim;
 double _extents[6];
 double _zcellmin;	// Minimum grid spacing along Z axis
 bool _zcellmin_cache;	// _zcellmin value is valid

 void _GetUserExtents(double extents[6]) const;
 double _interpolateVaryingCoord(
	size_t i0, size_t j0, size_t k0,
	double x, double y, double z 
 ) const;

};
#endif
