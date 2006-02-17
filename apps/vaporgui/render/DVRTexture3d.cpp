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
#include "TextureBrick.h"
#include "messagereporter.h"
#include "session.h"

#include "BBox.h"
#include "glutil.h"

#include "Matrix3d.h"
#include "Point3d.h"

#ifndef MAX
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#endif

using namespace VAPoR;

//
// Static member data initalization
//
std::map<GLenum, int> DVRTexture3d::_textureSizes;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DVRTexture3d::DVRTexture3d(DataType_T type, int nthreads) :
  _nx(0),
  _ny(0),
  _nz(0),
  _data(0),
  _delta(0.0),
  _maxTexture(maxTextureSize(GL_RGBA8))
{
  _texturePool.push_back(new TextureBrick());
  _bricks.resize(1);

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
  for (int i=0; i<_texturePool.size(); i++)
  {
    delete _texturePool[i];
  }

  _texturePool.empty();
  _bricks.empty();
}

//----------------------------------------------------------------------------
// Derived classes should call this function to ensure that the data and
// texture extents are being set and the texture bricks are being created. 
//----------------------------------------------------------------------------
int DVRTexture3d::SetRegion(void *data, int nx, int ny, int nz,
                            const int data_roi[6],
                            const float extents[6],
                            const int data_box[6],
                            int level) 
{ 
  if (_nx != nx || _ny != ny || _nz != nz)
  {
    _nx = nx; _ny = ny; _nz = nz;
  }

  _vmin.x = extents[0];
  _vmin.y = extents[1];
  _vmin.z = extents[2];
  
  _vmax.x = extents[3];
  _vmax.y = extents[4];
  _vmax.z = extents[5];

  _tmin.x = (float)data_roi[0]/(nx-1);
  _tmin.y = (float)data_roi[1]/(ny-1);
  _tmin.z = (float)data_roi[2]/(nz-1);

  _tmax.x = (float)data_roi[3]/(nx-1);
  _tmax.y = (float)data_roi[4]/(ny-1);
  _tmax.z = (float)data_roi[5]/(nz-1);

  _delta = fabs(_vmin.z-_vmax.z) / nz; 
  _data = data;
  _bricks.clear();

  if (_nx*_ny*_nz <= _maxTexture)
  {
    //
    // The data will fit completely within texture memory, so no bricking is 
    // needed. We can save a few cycles by setting up the single brick here. 
    //
    _bricks.resize(1);
    _bricks[0] = _texturePool[0];

    _bricks[0]->volumeMin(_vmin.x, _vmin.y, _vmin.z);
    _bricks[0]->volumeMax(_vmax.x, _vmax.y, _vmax.z);
    _bricks[0]->textureMin(_tmin.x, _tmin.y, _tmin.z);
    _bricks[0]->textureMax(_tmax.x, _tmax.y, _tmax.z);
    _bricks[0]->data((GLubyte*)data);
    _bricks[0]->size(nx, ny, nz);

    loadTexture(_bricks[0]);
  }
  else
  {
    //
    // The data will not fit completely within texture memory, need to 
    // subdivide into multiple bricks.
    //
    buildBricks(level, data_box, data_roi);
  }

  return 0;
}

//----------------------------------------------------------------------------
// Render the bricks
//----------------------------------------------------------------------------
void DVRTexture3d::renderBricks()
{
  //
  // Get the modelview matrix and its inverse
  //
  Matrix3d modelview;   
  Matrix3d modelviewInverse;

  glGetFloatv(GL_MODELVIEW_MATRIX, modelview.data());  
  modelview.inverse(modelviewInverse);

  //
  // Sort the bricks into rendering order.
  //
  sortBricks(modelview);

  //
  // Render the bricks
  //
  vector<TextureBrick*>::iterator iter;

  for (iter=_bricks.begin(); iter!=_bricks.end(); iter++)
  {
    TextureBrick *brick = *iter;
    
    if (brick->valid())
    {
      glBindTexture(GL_TEXTURE_3D, brick->handle());
      drawViewAlignedSlices(brick, modelview, modelviewInverse);
    }
  }
}

//----------------------------------------------------------------------------
// Draw the proxy geometry
//----------------------------------------------------------------------------
void DVRTexture3d::drawViewAlignedSlices(const TextureBrick *brick,
                                         const Matrix3d &modelview,
                                         const Matrix3d &modelviewInverse)
{
  //
  //  
  //
  BBox volumeBox  = brick->volumeBox();
  BBox textureBox = brick->textureBox();
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

//----------------------------------------------------------------------------
// Brick the texture data so that each brick will fit into texture memory.
//----------------------------------------------------------------------------
void DVRTexture3d::buildBricks(int level, const int bbox[6], const int roi[6])
{
  // NOTE: This uses the simplest subdivision algorithm, dividing only along 
  // the z-axis. This allows us to handle contiguous blocks of memory. This
  // is probably not adquate for the general case, and needs to be improved.  

  int nbricks = 2;

  //
  // Allocate bricks
  //
  GLint width = 0;

  while (_nx * _ny * _nz / nbricks > _maxTexture)
  {
    nbricks *= 2;

    if (nbricks >= _nz/2.0)
    {
      QString msg("Texture too large. Need better bricking algorithm.");
      BailOut(msg.ascii(),__FILE__,__LINE__);
    }
  }

  glTexImage3D(GL_PROXY_TEXTURE_3D, 0, GL_RGBA8, _nx, _ny, _nz/nbricks, 0,
               GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
    
  glGetTexLevelParameteriv(GL_PROXY_TEXTURE_3D, 0,
                           GL_TEXTURE_WIDTH, &width);

  if (width == 0)
  {
    QString msg("Texture too large. Need better bricking algorithm.");
    BailOut(msg.ascii(),__FILE__,__LINE__);
  }

  //
  // Add an extra brick to account for brick overlap
  //
  nbricks += 1;

  if (nbricks != _bricks.size())
  {
    //
    // Keep a resident pool of texture bricks to minimize dynamic memory 
    // allocations and deallocations
    //
    while(_texturePool.size() < nbricks) 
    {
      _texturePool.push_back(new TextureBrick());
    }

    //
    // Initialize the brick list
    //
    _bricks.resize(nbricks);

    for (int i=0; i<nbricks; i++)
    {
      _bricks[i] = _texturePool[i];
    }
  }

  //
  // Populate bricks 
  //
  TextureBrick *brick = NULL;
  int bnz = _nz/(nbricks-1);  // size of brick in voxels

  // brick block box
  int bbbox[6] = {bbox[0], bbox[1], bbox[2], bbox[3], bbox[4], bbox[2]+bnz};

  // brick region of interest
  int broi[6] = {roi[0], roi[1], roi[2], roi[3], roi[4], roi[5]};

  for(int z=0; z<_bricks.size(); ++z)
  {
    brick = _bricks[z]; 

    //
    // Set the extents of the brick's data box
    //
    brick->setDataBlock(level, bbbox);

    if (_vmin.z >= brick->dataMax().z || _vmax.z <= brick->dataMin().z)
    {
      //
      // This brick's data box does not overlap the region of interest.
      // We'll turn it off (invalidate it). 
      //
      brick->invalidate();
    }     
    else
    {
      //
      // Determine the region of interest for the brick, leaving an extra
      // voxel on either end for fragment interpolation (except fo bricks on 
      // the extreme ends).
      // 
      if (bbbox[2] > roi[2])
      {
        broi[2] = bbbox[2] + 1;
      }
      else
      {
        broi[2] = roi[2];
      }

      if (bbbox[5] < roi[5])
      {
        broi[5] = bbbox[5] - 1;
      }
      else
      {
        broi[5] = roi[5];
      }

      brick->setROI(level, broi);

      //
      // Set the texture coordinates of the brick.
      //
      brick->textureMin(_tmin.x,_tmin.y, (broi[2]-bbbox[2])/(float)bnz);
      brick->textureMax(_tmax.x,_tmax.y, (broi[5]-bbbox[2])/(float)bnz);

      //
      // Since we are only subdividing on the z-axis, we can just offset
      // the data pointer. 
      //
      brick->data((GLubyte*)_data + _nx * _ny * ((bnz-2) * z));
      brick->size(_nx, _ny, bnz);

      loadTexture(brick);
    }

    //
    // increment the brick block box (for the next brick) by the size of a
    // brick (minus 2 for texture overlap).
    // 
    bbbox[2] += bnz-2;
    bbbox[5] += bnz-2;
  }
  
  // 
  // This needs to be revisited. This is may not be safe for the last brick.
  // There will be an array overflow when GL copies the data into texture
  // memory; although, that memory should never be used. Should probably 
  // shift the last brick up, so that, all of its addressable memory is 
  // valid. 
  // 
}

//----------------------------------------------------------------------------
// Sort the bricks in back to front order.
//----------------------------------------------------------------------------
void DVRTexture3d::sortBricks(const Matrix3d &modelview)
{
  if (_bricks.size() > 1)
  {
    vector<double> sortvals;
    
    for(int i=0; i<_bricks.size(); i++)
    {
      Vect3d center = modelview * _bricks[i]->center();
  
      sortvals.push_back(center.magSq());
    }

    for(int i=0; i<_bricks.size(); i++)
    {
      for(int j=i+1; j<_bricks.size(); ++j)
      {
        if(sortvals[i] < sortvals[j])
        {
          double valtmp          = sortvals[i];
          TextureBrick *bricktmp = _bricks[i];
          
          sortvals[i] = sortvals[j]; 
          sortvals[j] = valtmp;
          
          _bricks[i] = _bricks[j]; 
          _bricks[j] = bricktmp;
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
// Determine and return the maximum texture size (in bytes). 
//----------------------------------------------------------------------------
long DVRTexture3d::maxTextureSize(GLenum format)
{
  if (_textureSizes.find(format) == _textureSizes.end())
  {
    for (int i = 128; i < 130000; i*=2)
    {
      glTexImage3D(GL_PROXY_TEXTURE_3D, 0, format, i, i, i, 0,
                   GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

      GLint width;

      glGetTexLevelParameteriv(GL_PROXY_TEXTURE_3D, 0,
                               GL_TEXTURE_WIDTH, &width);
      if (width == 0)
      {
        i /= 2;
        
        _textureSizes[format] = i*i*i;

        return i*i*i;
      }
    }
  }

  return _textureSizes[format];
}
