//-- BBox.h ------------------------------------------------------------------
//
// Copyright (C) 2005 Kenny Gruchalla.
//
// Simple Bound Box class used for texture slice intersections. 
//
//----------------------------------------------------------------------------
#ifndef BBox_h
#define BBox_h

#include "Matrix3d.h"
#include "Point3d.h"

namespace VAPoR {

  class RegionParams;

  class BBox 
  {
    
  public:
    BBox();
    BBox(RegionParams& params);
    BBox(const Point3d& min, const Point3d& max);
    BBox(const BBox&);

    BBox& operator=(const BBox&);

    ~BBox();

    // Transform the bounding box 
    void transform(const Matrix3d &m);

    Point3d center() const;

    // Return the minimum & maximum extents
    inline const Point3d& minZ() const { return _corners[_minIndex]; }
    inline const Point3d& maxZ() const { return _corners[_maxIndex]; }

    inline int minIndex() const { return _minIndex; }
    inline int maxIndex() const { return _maxIndex; }

	// Return true if point, p, is inside the possibly non-axis aligned
	// bounding box (may be non axis-aligned if transform has been called).
	//
    bool insideBox(const Point3d &p);

    // return the corner point
    inline const Point3d& operator[](int i) const { return _corners[i]; };

  private:

    int _minIndex;
    int _maxIndex;
    
    Point3d _corners[8];

    //       6*--------*7
    //       /|       /|
    //      / |      / |
    //     /  |     /  |
    //    /  4*----/---*5
    //  2*--------*3  /
    //   |  /     |  /
    //   | /      | /
    //   |/       |/
    //  0*--------*1
    //             

  };
} // End namespace VAPoR


#endif

