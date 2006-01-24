//--DVRTexture3d.h -----------------------------------------------------------
//
// Copyright (C) 2005 Kenny Gruchalla.  All rights reserved.
//
// Volume rendering engine based on 3d-textured view aligned slices. 
// Provides proxy geometry, color lookup should be provided by derived 
// classes. Derived classes must to initialize the data and texture
// extents. 
//
//----------------------------------------------------------------------------

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <qgl.h>

#include "DVRTexture3d.h"
#include "messagereporter.h"

#include "BBox.h"
#include "glutil.h"

#include "Matrix3d.h"
#include "Point3d.h"

using namespace VAPoR;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DVRTexture3d::DVRTexture3d(DataType_T type, int nthreads) :
  _delta(0.0)
{
  if (type != UINT8) 
  {
    QString strng("Volume Renderer error; Unsupported voxel type: ");
    strng += QString::number(type);
    BailOut(strng.ascii(),__FILE__,__LINE__);
  }
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
DVRTexture3d::~DVRTexture3d() 
{
}

//----------------------------------------------------------------------------
// Derived classes should call this function to ensure that the data and
// texture extents are being set. 
//----------------------------------------------------------------------------
int DVRTexture3d::SetRegion(void *, int nx, int ny, int nz,
                                  const int data_roi[6],
                                  const float extents[6]) 
{ 
  _vmin.x = extents[0]; _vmin.y = extents[1]; _vmin.z = extents[2];
  _vmax.x = extents[3]; _vmax.y = extents[4]; _vmax.z = extents[5];

  _tmin.x = (float)data_roi[0]/nx; 
  _tmin.y = (float)data_roi[1]/ny; 
  _tmin.z = (float)data_roi[2]/nz;
  
  _tmax.x = (float)data_roi[3]/nx; 
  _tmax.y = (float)data_roi[4]/ny; 
  _tmax.z = (float)data_roi[5]/nz;
  
  _delta = fabs(extents[2]-extents[5]) / nz; 

  return 0;
}

//----------------------------------------------------------------------------
// Draw the proxy geometry
//----------------------------------------------------------------------------
void DVRTexture3d::drawViewAlignedSlices()
{
  //
  // Get the modelview matrix and its inverse
  //
  Matrix3d modelview;   
  Matrix3d modelviewInverse;

  glGetFloatv(GL_MODELVIEW_MATRIX, modelview.data());  
  modelview.inverse(modelviewInverse);

  //
  //  
  //
  BBox volumeBox(_vmin, _vmax);
  BBox textureBox(_tmin, _tmax);
  BBox rotatedBox(volumeBox);

  //
  // transform the volume into world coordinates
  //
  rotatedBox.transform(modelview);

  //
  // Define the slice view aligned slice plane. The plane will be positioned
  // at the volume corner furthest from the eye. (model coordinates)
  //
  Vect3d slicePoint = volumeBox[rotatedBox.minIndex()]; 

  //
  // Calculate the slice plane normal (i.e., the view plane normal transformed
  // into model space). 
  //
  // slicePlaneNormal = modelviewInverse * viewPlaneNormal; 
  //                                       (0,0,1);
  //
  Vect3d slicePlaneNormal(modelviewInverse(2,0), 
                          modelviewInverse(2,1),
                          modelviewInverse(2,2)); 
  slicePlaneNormal.unitize();
   
  //
  // Calculate the distance between slices
  //

  // 
  // Calculate the the minimum and maximum z-position of the rotated bounding
  // box. Equivalent to but quicker than:
  //
  // Vect3d maxd(0, 0, rotatedBox.maxZ().z); 
  // Vect3d mind(0, 0, rotatedBox.minZ().z); 
  // maxv = modelviewInverse * maxd;
  // minv = modelviewInverse * mind;
  //
  Vect3d maxv(modelviewInverse(2,0)*rotatedBox.maxZ().z,
              modelviewInverse(2,1)*rotatedBox.maxZ().z,
              modelviewInverse(2,2)*rotatedBox.maxZ().z);
  Vect3d minv(modelviewInverse(2,0)*rotatedBox.minZ().z,
              modelviewInverse(2,1)*rotatedBox.minZ().z,
              modelviewInverse(2,2)*rotatedBox.minZ().z);
  
  maxv -= minv;

  float sampleDistance = maxv.mag();

  Vect3d sliceDelta = slicePlaneNormal * _delta;

  int samples = (int)(sampleDistance / _delta);

  //
  // Calculate edge intersections between the plane and the boxes
  //
  Vect3d verts[6];   // for edge intersections
  Vect3d tverts[6];  // for texture intersections
  Vect3d rverts[6];  // for transformed edge intersections

  for(int i = 0 ; i <= samples; i++)
  { 
    int order[6] ={0,1,2,3,4,5};
    int size = 0;

    //
    // Intersect the slice plane with the bounding boxes
    //
    size = intersect(slicePoint, slicePlaneNormal, 
                     volumeBox, verts, 
                     textureBox, tverts, 
                     rotatedBox, rverts);
    //
    // Calculate the convex hull of the vertices (world coordinates)
    //
    findVertexOrder(rverts, order, size);

    //
    // Draw slice and texture map it
    //
	glBegin(GL_POLYGON); 
	{
      for(int j=0; j< size; ++j)
      {
        
        glTexCoord3f(tverts[order[j]].x(), 
                     tverts[order[j]].y(), 
                     tverts[order[j]].z());
        
        glVertex3f(verts[order[j]].x(), 
                   verts[order[j]].y(), 
                   verts[order[j]].z());
      }
	}
	glEnd();

    //
    // increment the slice plane by the slice distance
    //
    slicePoint += sliceDelta;
  }

  glFlush();
}

//----------------------------------------------------------------------------
// Find the intersections of the plane with the boxes.
//----------------------------------------------------------------------------
int DVRTexture3d::intersect(const Vect3d &sp, 
                            const Vect3d &spn, 
                            const BBox &volumeBox, 
                            Vect3d verts[6],
                            const BBox &textureBox, 
                            Vect3d tverts[6],
                            const BBox &rotatedBox, 
                            Vect3d rverts[6])
{
  static const int edges[12][2] = { {0, 1}, // front bottom edge
                                    {0, 2}, // front left edge
                                    {1, 3}, // front right edge
                                    {4, 0}, // left bottom edge
                                    {1, 5}, // right bottom edge
                                    {2, 3}, // front top edge
                                    {4, 5}, // back bottom edge
                                    {4, 6}, // back left edge
                                    {5, 7}, // back right edge
                                    {6, 7}, // back top edge
                                    {2, 6}, // left top edge
                                    {3, 7}  // right top edge
                                  };

  int intersections = 0;

  for (int i=0; i<12; i++)
  {
    const Point3d &p0 = volumeBox[ edges[i][0] ];
    const Point3d &p1 = volumeBox[ edges[i][1] ];

    float t = spn.dot(sp - p0) / spn.dot(p1 - p0);

    if( (t>=0) && (t<=1) )
    {
      const Point3d &t0 = textureBox[ edges[i][0] ];
      const Point3d &t1 = textureBox[ edges[i][1] ];

      const Point3d &r0 = rotatedBox[ edges[i][0] ];
      const Point3d &r1 = rotatedBox[ edges[i][1] ];

      // Compute the line intersection
      verts[intersections].x(p0.x + t*(p1.x - p0.x));
      verts[intersections].y(p0.y + t*(p1.y - p0.y));
      verts[intersections].z(p0.z + t*(p1.z - p0.z));
      
      // Compute the texture interseciton
      tverts[intersections].x(t0.x + t*(t1.x - t0.x));
      tverts[intersections].y(t0.y + t*(t1.y - t0.y));
      tverts[intersections].z(t0.z + t*(t1.z - t0.z));
      
      // Compute view coordinate intersection
      rverts[intersections].x(r0.x + t*(r1.x - r0.x));
      rverts[intersections].y(r0.y + t*(r1.y - r0.y));
      rverts[intersections].z(r0.z + t*(r1.z - r0.z));

      intersections++;
    }
  } 

  return intersections;
}

//----------------------------------------------------------------------------
// Given a set of vertices compute the convex hull (convex polygon). 
// 
// There may be a faster algorithm to do this since we know that all the 
// points are extreme. 
//----------------------------------------------------------------------------
void DVRTexture3d::findVertexOrder(const Vect3d verts[6], int order[6], 
                                   int degree)
{
  //
  // Find the center of the polygon
  //
  float centroid[2] = {0.0, 0.0};

  for( int i=0; i < degree; i++)
  { 
    centroid[0] += verts[i].x();
    centroid[1] += verts[i].y();
  } 

  centroid[0] /= degree;
  centroid[1] /= degree;	 
  
  //
  // Compute the angles from the centroid
  //
  float angle[6];
  float dx;
  float dy;

  for(int i=0; i< degree; i++)
  { 
    dx = verts[i].x() - centroid[0];
    dy = verts[i].y() - centroid[1];

    angle[i] = dy/(fabs(dx) + fabs(dy)); 

    if( dx < 0.0 )  // quadrants 2&3
    {
      angle[i] = 2.0 - angle[i];
    }
    else if( dy < 0.0 ) // quadrant 4
    {
      angle[i] = 4.0 + angle[i]; 
    }
  } 

  //
  // Sort the angles (bubble sort)
  //
  for(int i=0; i < degree; i++) 
  {
    for(int j=i+1; j < degree; j++) 
    {
      if(angle[j] < angle[i]) 
      {
        float tmpd = angle[i];
        angle[i] = angle[j];
        angle[j] = tmpd;

        int tmpi = order[i];
        order[i] = order[j];
        order[j] = tmpi;
      }
    }
  }
}

