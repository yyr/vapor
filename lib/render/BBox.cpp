//-- BBox.cpp ----------------------------------------------------------------
//
// Copyright (C) 2005 Kenny Gruchalla.
//
// Simple Bound Box class used for texture slice intersections.
//
//----------------------------------------------------------------------------

#include "BBox.h"

#include <stdlib.h>
#include "regionparams.h"

using namespace VAPoR;

//----------------------------------------------------------------------------
// Default constructor (unit cube)
//----------------------------------------------------------------------------
BBox::BBox() :
  _minIndex(0),
  _maxIndex(7)
{
  _corners[0] = Point3d(0,0,0);
  _corners[1] = Point3d(1,0,0);
  _corners[2] = Point3d(0,1,0);
  _corners[3] = Point3d(1,1,0);
  _corners[4] = Point3d(0,0,1);
  _corners[5] = Point3d(1,0,1);
  _corners[6] = Point3d(0,1,1);
  _corners[7] = Point3d(1,1,1);
}

//----------------------------------------------------------------------------
// Construct box from RegionParams extents. 
// Assumes the bounding box is axis-aligned. 
//----------------------------------------------------------------------------
BBox::BBox(RegionParams &regionParams, int timestep) 
{
  
  float extents[6];
  
  regionParams.calcStretchedBoxExtentsInCube(extents, timestep);
  
  _minIndex = 0;
  _maxIndex = 7;

  _corners[0] = Point3d(extents[0], extents[1], extents[2]);
  _corners[1] = Point3d(extents[3], extents[1], extents[2]);
  _corners[2] = Point3d(extents[0], extents[4], extents[2]);
  _corners[3] = Point3d(extents[3], extents[4], extents[2]);
  _corners[4] = Point3d(extents[0], extents[1], extents[5]);
  _corners[5] = Point3d(extents[3], extents[1], extents[5]);
  _corners[6] = Point3d(extents[0], extents[4], extents[5]);
  _corners[7] = Point3d(extents[3], extents[4], extents[5]);

}

//----------------------------------------------------------------------------
// Construct bounding box from minimum & maximum extents. 
// Assumes the bounding box is axis-aligned. 
//----------------------------------------------------------------------------
BBox::BBox(const Point3d& minP, const Point3d& maxP) :
  _minIndex(0),
  _maxIndex(7)
{
  _corners[0] = Point3d(minP.x, minP.y, minP.z);
  _corners[1] = Point3d(maxP.x, minP.y, minP.z);
  _corners[2] = Point3d(minP.x, maxP.y, minP.z);
  _corners[3] = Point3d(maxP.x, maxP.y, minP.z);
  _corners[4] = Point3d(minP.x, minP.y, maxP.z);
  _corners[5] = Point3d(maxP.x, minP.y, maxP.z);
  _corners[6] = Point3d(minP.x, maxP.y, maxP.z);
  _corners[7] = Point3d(maxP.x, maxP.y, maxP.z);

}

//---------------------------------------------------------------------------
// Copy constructor
//----------------------------------------------------------------------------
BBox::BBox(const BBox& box) : 
  _minIndex(box._minIndex),
  _maxIndex(box._maxIndex)
{
  _corners[0] = box._corners[0];
  _corners[1] = box._corners[1];
  _corners[2] = box._corners[2];
  _corners[3] = box._corners[3];
  _corners[4] = box._corners[4];
  _corners[5] = box._corners[5];
  _corners[6] = box._corners[6];
  _corners[7] = box._corners[7];
}


bool BBox::insideBox(const Point3d &point) {

	// Indecies, in proper winding order, of vertices making up planes
	// of BBox.
	//
	static const int planes[6][4] = {
		{4,5,7,6},  // front
		{0,2,3,1},  // back
		{5,1,3,7},  // right
		{4,6,2,0},  // left 
		{6,7,3,2},  // top
		{4,0,1,5}   // bottom
	};

	//
	// Verify point is "behind" each plane making up the bounding box
	//
	for (int i=0; i<6; i++) {

		// Two vectors in plane i
		//
		Point3d p0(_corners[planes[i][0]]);
		Point3d p1(_corners[planes[i][1]]);
		Point3d p2(_corners[planes[i][2]]);
		Vect3d v1(p0, p1);
		Vect3d v2(p0, p2);

		// normal to plane
		//
		Vect3d N = v1.cross(v2);

		// Find d, distance from plane to origin
		//
		// d = - (ax + by + cz) 
		//
		float d = - (N(0)*p0(0) + N(1)*p0(1) + N(2)*p0(2));

		// Compute Hessian normal form of plane equation:
		//
		// Nunit dot x = -p, where:
		//
		//	Nx = a / sqrt(a^2 + b^2 + c^2)
		//	Ny = b / sqrt(a^2 + b^2 + c^2)
		//	Nz = c / sqrt(a^2 + b^2 + c^2)
		//	p = d / sqrt(a^2 + b^2 + c^2)
		//
		Vect3d Nunit = N.unit();
		float p = d / N.mag();

		Vect3d pv(point);
		if ((Nunit.dot(pv) + p) > 0.0) return false;
	}
	return(true);

}

//----------------------------------------------------------------------------
// Assignment operator
//----------------------------------------------------------------------------
BBox& BBox::operator=(const BBox& box)
{
  _minIndex = box._minIndex;
  _maxIndex = box._maxIndex;

  _corners[0] = box._corners[0];
  _corners[1] = box._corners[1];
  _corners[2] = box._corners[2];
  _corners[3] = box._corners[3];
  _corners[4] = box._corners[4];
  _corners[5] = box._corners[5];
  _corners[6] = box._corners[6];
  _corners[7] = box._corners[7];

  return *this;
}
    

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
BBox::~BBox()
{
}


//----------------------------------------------------------------------------
// Transform the bouning box
//----------------------------------------------------------------------------
void BBox::transform(const Matrix3d &m)
{
  _minIndex = 0;
  _maxIndex = 0;

  for (int i=0; i<8; i++)
  {
    _corners[i] = m * _corners[i];

    // z
    if (_corners[_minIndex].z > _corners[i].z) _minIndex = i;
    if (_corners[_maxIndex].z < _corners[i].z) _maxIndex = i; 
  }
}

//----------------------------------------------------------------------------
// Box center
//----------------------------------------------------------------------------
Point3d BBox::center() const
{
  Point3d centerPt;

  int minxi = 0;
  int maxxi = 0;
  int minyi = 0;
  int maxyi = 0;
  int minzi = 0;
  int maxzi = 0;

  for (int i=0; i<8; i++)
  {
    // x
    if (_corners[minxi].x > _corners[i].x) minxi = i;
    if (_corners[maxxi].x < _corners[i].x) maxxi = i; 
    // y
    if (_corners[minyi].y > _corners[i].y) minyi = i;
    if (_corners[maxyi].y < _corners[i].y) maxyi = i; 
    // z
    if (_corners[minzi].z > _corners[i].z) minzi = i;
    if (_corners[maxzi].z < _corners[i].z) maxzi = i; 
  }

  centerPt.x = _corners[minxi].x + 
    (_corners[maxxi].x - _corners[minxi].x) / 2.0;  
  centerPt.y = _corners[minyi].y + 
    (_corners[maxyi].y - _corners[minyi].y) / 2.0;  
  centerPt.z = _corners[minzi].z + 
    (_corners[maxzi].z - _corners[minzi].z) / 2.0;

  return centerPt;
}
