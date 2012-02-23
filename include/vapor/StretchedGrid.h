#ifndef _StretchedGrid_
#define _StretchedGrid_

#include <vector>
#include <vapor/common.h>
#include "RegularGrid.h"

//
//! \class StretchedGrid
//!
//! \brief This class implements a 2D or 3D stretched grid: a generalization
//! of a regular grid where the spacing of grid points along each dimension
//! may vary along the dimension.  I.e. the coordinates along each
//! dimension are a function of the dimension index. E.g. x = x(i), 
//! for some monotonically increasing function x(i).
//!
//

class VDF_API StretchedGrid : public RegularGrid {
public:

 //!
 //! Construct a stretched grid sampling a 3D or 2D scalar function
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
 //! \param[in] xcoords A vector of length (max[0]-min[0]+1) specifying
 //! the user coordinate of each grid point along the X dimension. If
 //! length(xcoord) != max[0]-min[0]+1 then the grid spacing is assumed to 
 //! be uniform.
 //! \param[in] ycoords A vector of length (max[1]-min[1]+1) specifying
 //! the user coordinate of each grid point along the Y dimension. If
 //! length(ycoord) != max[1]-min[1]+1 then the grid spacing is assumed to 
 //! be uniform.
 //! \param[in] zcoords A vector of length (max[2]-min[2]+1) specifying
 //! the user coordinate of each grid point along the Z dimension. If
 //! length(zcoord) != max[2]-min[2]+1 then the grid spacing is assumed to 
 //! be uniform.
 //!
 StretchedGrid(
	const size_t bs[3],
	const size_t min[3],
	const size_t max[3],
	const double extents[6],
	const bool periodic[3],
	float ** blks,
	const std::vector <double> &xcoords,
	const std::vector <double> &ycoords,
	const std::vector <double> &zcoords
);

 //! 
 //! Construct a stretched grid sampling a 3D or 2D scalar function
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
 StretchedGrid(
	const size_t bs[3],
	const size_t min[3],
	const size_t max[3],
	const double extents[6],
	const bool periodic[3],
	float ** blks,
	const std::vector <double> &xcoords,
	const std::vector <double> &ycoords,
	const std::vector <double> &zcoords,
	float missing_value
 );


 //! \copydoc RegularGrid::GetValue()
 //!
 float GetValue(double x, double y, double z) const;

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

 int Reshape(
	const size_t min[3],
	const size_t max[3],
	const bool periodic[3]
 );

private:
 size_t _min[3];
 size_t _max[3];
 std::vector <double> _xcoords;
 std::vector <double> _ycoords;
 std::vector <double> _zcoords;

 float _GetValueNearestNeighbor(double x, double y, double z) const;
 float _GetValueLinear(double x, double y, double z) const;



};
#endif