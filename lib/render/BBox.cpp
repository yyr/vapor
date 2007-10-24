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
BBox::BBox(RegionParams &regionParams) 
{
  
  float extents[6];
  
  regionParams.calcStretchedBoxExtentsInCube(extents);
  
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
