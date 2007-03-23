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
#include "params.h"
#include "DVRTexture3d.h"
#include "TextureBrick.h"
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


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DVRTexture3d::DVRTexture3d(DataType_T type, int nthreads) :
  _nx(0),
  _ny(0),
  _nz(0),
  _data(0),
  _bx(0),
  _by(0),
  _bz(0),  
  _delta(0.0),
  _maxTexture(maxTextureSize(GL_LUMINANCE8)),
  _maxBrickDim(128), // NOTE: This should always be <= _maxTexture
  _lastRegion()
{
  if (type != UINT8) 
  {
    QString strng("Volume Renderer error; Unsupported voxel type: ");
    strng += QString::number(type);
	Params::BailOut(strng.ascii(),__FILE__,__LINE__);
  }

}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
DVRTexture3d::~DVRTexture3d() 
{
  for (int i=0; i<_bricks.size(); i++)
  {
    delete _bricks[i];
    _bricks[i] = NULL;
  }

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
  _data = data;

  if (_lastRegion.update(nx, ny, nz, data_roi, data_box, extents))
  {
    if (_nx != nx || _ny != ny || _nz != nz)
    {
      _nx = _bx = nextPowerOf2(nx); 
      _ny = _by = nextPowerOf2(ny); 
      _nz = _bz = nextPowerOf2(nz);
    }
    
    _vmin.x = extents[0];
    _vmin.y = extents[1];
    _vmin.z = extents[2];
    _vmax.x = extents[3];
    _vmax.y = extents[4];
    _vmax.z = extents[5];
    
    _delta = fabs(_vmin.z-_vmax.z) / nz; 
    
    for (int i=0; i<_bricks.size(); i++)
    {
      delete _bricks[i];
      _bricks[i] = NULL;
    }
    
    _bricks.clear();
    
    if (_nx <= _maxTexture && _ny <= _maxTexture && _nz <= _maxTexture)
    {
      //
      // The data will fit completely within texture memory, so no bricking is 
      // needed. We can save a few cycles by setting up the single brick here,
      // rather than calling buildBricks(...). 
      //
      _bricks.push_back(new TextureBrick());
      
      _bricks[0]->volumeMin(extents[0], extents[1], extents[2]);
      _bricks[0]->volumeMax(extents[3], extents[4], extents[5]);
      
       // Compute min and max texture coordinates. When texture wrap mode is 
       // set to GL_CLAMP_TO_EDGE the min and max coordinates are:
       // min = 1.0/2N and max = 1.0 - min.
       //
       float mins = 1.0 / (2*_nx);
       float maxs = 1.0 - mins;
       float mint = 1.0 / (2*_ny);
       float maxt = 1.0 - mint;
       float minr = 1.0 / (2*_nz);
       float maxr = 1.0 - minr;

      _bricks[0]->textureMin(
        mins + (data_roi[0] * (maxs-mins)/(_nx-1)), 
        mint + (data_roi[1] * (maxt-mint)/(_ny-1)), 
        minr + (data_roi[2] * (maxr-minr)/(_nz-1)));

      _bricks[0]->textureMax(
        mins + (data_roi[3] * (maxs-mins)/(_nx-1)), 
        mint + (data_roi[4] * (maxt-mint)/(_ny-1)), 
        minr + (data_roi[5] * (maxr-minr)/(_nz-1)));
      
      _bricks[0]->fill((GLubyte*)data, nx, ny, nz);
      
      loadTexture(_bricks[0]);
    } 
    else
    {
      //
      // The data will not fit completely within texture memory, need to 
      // subdivide into multiple bricks.
      //
      buildBricks(level, data_box, data_roi, nx, ny, nz);
    }
  }
  else
  {
    //
    // Only the data has changed; therefore, we'll refill and reuse the
    // the existing bricks (texture objects).
    //
    for (int i=0; i<_bricks.size(); i++)
    {
      _bricks[i]->refill((GLubyte*)data);
    }
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

float clamp(float x, float min, float max) {
	assert(min<=max);
	if (x < min) return min;
	if (x > max) return max;
	return(x);
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
      tverts[intersections].x(clamp(
        t0.x + t*(t1.x - t0.x), textureBox.minZ().x, textureBox.maxZ().x)
      );
      tverts[intersections].y(clamp(
        t0.y + t*(t1.y - t0.y), textureBox.minZ().y, textureBox.maxZ().y)
      );
      tverts[intersections].z(clamp(
        t0.z + t*(t1.z - t0.z), textureBox.minZ().z, textureBox.maxZ().z)
      );

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
void DVRTexture3d::buildBricks(int level, const int box[6], const int roi[6],
                               int nx, int ny, int nz)
{
  // 
  // Calculate the brick size (_bx, _by, _bz are initialized to _nx, _ny, _nz
  // in DVRTexture3d::SetRegion, before this method is called).
  //
  // Note: we are using _maxBrickDim as the maximum brick dimension, not 
  // _maxTexture. The _maxTexture will typically be on the order of 512 
  // (i.e., a maximum texture of 512^3); however, we can get better performance
  // using a smaller brick size, since we are using uniform brick sizing and 
  // bricks on the borders may contain very small subset of data.
  //
  while (_bx > _maxBrickDim)
  {
    _bx /= 2;
  }

  while (_by > _maxBrickDim)
  {
    _by /= 2;
  }

  while (_bz > _maxBrickDim)
  {
    _bz /= 2;
  }

  //
  // Compute the number of bricks in the cardinal directions
  //
  int nbricks[3] = {(int)ceil((double)(_nx-1)/(_bx-1)),
                    (int)ceil((double)(_ny-1)/(_by-1)),
                    (int)ceil((double)(_nz-1)/(_bz-1))};

  //
  // Populate bricks 
  //
  Point3d vmin;
  Point3d vmax;
  Point3d tmin;
  Point3d tmax;

  TextureBrick *brick = NULL;

  // brick data/block box
  int bbox[6] = {0,0,0, _bx-1, _by-1, _bz-1};

  // brick region of interest
  int broi[6] = {roi[0], roi[1], roi[2], roi[3], roi[4], roi[5]};

  // data offset
  int offset[3] = {0, 0, 0};

  for(int z=0; z<nbricks[2]; ++z)
  {
    //
    // Determine the y boundaries of the brick's data box. 
    //
    bbox[1]   = 0;
    bbox[4]   = _by-1;
    offset[1] = 0;
    
    //
    // Determine the z region of interest for the brick, leaving an extra
    // voxel on either end for fragment interpolation (except fo bricks
    // on the extreme ends).
    // 
    broi[2] = (bbox[2] > roi[2]) ? bbox[2] + 1 : roi[2];
    broi[5] = (bbox[5] < roi[5]) ? bbox[5] - 1 : roi[5];

    for (int y=0; y<nbricks[1]; y++)
    {
      //
      // Determine the x boundaries of the brick's data box
      //
      bbox[0]   = 0;
      bbox[3]   = _bx-1;
      offset[0] = 0;

      //
      // Determine the y region of interest for the brick, leaving an extra
      // voxel on either end for fragment interpolation (except fo bricks
      // on the extreme ends).
      // 
      broi[1] = (bbox[1] > roi[1]) ? bbox[1] + 1 : roi[1];
      broi[4] = (bbox[4] < roi[4]) ? bbox[4] - 1 : roi[4];

      for (int x=0; x<nbricks[0]; x++)
      {
        brick = new TextureBrick();

        //
        // Set the extents of the brick's data box
        //
        brick->setDataBlock(level, box, bbox);

        if (_vmin.z >= brick->dataMax().z || _vmax.z <= brick->dataMin().z ||
            _vmin.y >= brick->dataMax().y || _vmax.y <= brick->dataMin().y ||
            _vmin.x >= brick->dataMax().x || _vmax.x <= brick->dataMin().x)
        {
          //
          // This brick's data box does not overlap the region of interest.
          // Toss it.
          //
          delete brick;
          brick = NULL;
        }     
        else
        {
          //
          // Determine the x region of interest for the brick, leaving an extra
          // voxel on either end for fragment interpolation (except fo bricks
          // on the extreme ends).
          // 
          broi[0] = (bbox[0] > roi[0]) ? bbox[0] + 1 : roi[0];
          broi[3] = (bbox[3] < roi[3]) ? bbox[3] - 1 : roi[3];

          brick->setROI(level, box, broi);

          //
          // Set the texture coordinates of the brick.
          //
          brick->textureMin((broi[0]-bbox[0])/(float)(_bx-1),
                            (broi[1]-bbox[1])/(float)(_by-1),
                            (broi[2]-bbox[2])/(float)(_bz-1));
          brick->textureMax((broi[3]-bbox[0])/(float)(_bx-1),
                            (broi[4]-bbox[1])/(float)(_by-1),
                            (broi[5]-bbox[2])/(float)(_bz-1));

          //
          // Fill the brick
          //
          brick->fill((GLubyte*)_data, 
                      _bx, _by, _bz, 
                      nx, ny, nz,
                      offset[0], offset[1], offset[2]);

          loadTexture(brick);

          _bricks.push_back(brick); 
        }

        //
        // increment the brick block box (for the next brick in the x-axis) by 
        // the size of a brick (minus 2 for texture overlap).
        // 
        bbox[0]    = bbox[3]-2;
        offset[0] += _bx-2;
        bbox[3]   += _bx-2;
      }

      //
      // increment the brick block box (for the next brick in the y-axis)
      // by the size of a brick (minus 2 for texture overlap).
      // 
      bbox[1]    = bbox[4]-2;
      offset[1] += _by-2;
      bbox[4]   += _by-2;
    }

    //
    // increment the brick block box (for the next brick in the z-axis) 
    // by the size of a brick (minus 2 for texture overlap).
    // 
    bbox[2]    = bbox[5]-2;
    offset[2] += _bz-2;
    bbox[5]   += _bz-2;
  }
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
int DVRTexture3d::maxTextureSize(GLenum format)
{

#if defined(__sgi) || defined(Darwin)

  return 128;

#else

  // Upper limit on texture size - maximum value returned by this 
  // function. 
  //
  // N.B. The texture proxy method of determining maximum texture
  // size supported by the card has not been reliable with nVidia
  // drivers, in some instances returning values larger than what the
  // card will actually support.
  //
  const int MAX_TEX_SZ = 512; 

  if (GLEW_ATI_fragment_shader)
  {
    return 256;
  }

  for (int i = 128; i < 2*MAX_TEX_SZ; i*=2)
  {
    glTexImage3D(GL_PROXY_TEXTURE_3D, 0, format, i, i, i, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

    GLint width;
    GLint height;
    GLint depth;
    
    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_3D, 0,
                             GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_3D, 0,
                             GL_TEXTURE_HEIGHT, &height);
    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_3D, 0,
                             GL_TEXTURE_DEPTH, &depth);

    if (width == 0 || height == 0 || depth == 0)
    {
      i /= 2;

      return i;
    }
  }

  return MAX_TEX_SZ;

#endif
}


//============================================================================
// subclass DVRTexture3d::RegionState
//============================================================================

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DVRTexture3d::RegionState::RegionState()
{ 
  _dim[0] = _dim[1] = _dim[2] = 0;
  _roi[0] = _roi[1] = _roi[2] = _roi[3] = _roi[4] = _roi[5] = 0;
  _box[0] = _box[1] = _box[2] = _box[3] = _box[4] = _box[5] = 0;
  _ext[0] = _ext[1] = _ext[2] = _ext[3] = _ext[4] = _ext[5] = 0.0;
}

//----------------------------------------------------------------------------
// Update the region state. Returns true if the region has changed, false
// otherwise.
//----------------------------------------------------------------------------
bool DVRTexture3d::RegionState::update(int nx, int ny, int nz,
                                       const int roi[6], 
                                       const int box[6], 
                                       const float extents[6])
{
  if (nx == _dim[0] && ny == _dim[1] &&  nz == _dim[2] &&
      _roi[0] == roi[0] &&
      _roi[1] == roi[1] &&
      _roi[2] == roi[2] &&
      _roi[3] == roi[3] &&
      _roi[4] == roi[4] &&
      _roi[5] == roi[5] &&

      _ext[0] == extents[0] &&
      _ext[1] == extents[1] &&
      _ext[2] == extents[2] &&
      _ext[3] == extents[3] &&
      _ext[4] == extents[4] &&
      _ext[5] == extents[5] &&

      _box[0] == box[0] &&
      _box[1] == box[1] &&
      _box[2] == box[2] &&
      _box[3] == box[3] &&
      _box[4] == box[4] &&
      _box[5] == box[5])
  {
    return false;
  }

 _dim[0] = nx; 
 _dim[1] = ny; 
 _dim[2] = nz;

 _roi[0] = roi[0];
 _roi[1] = roi[1];
 _roi[2] = roi[2];
 _roi[3] = roi[3];
 _roi[4] = roi[4];
 _roi[5] = roi[5];

 _ext[0] = extents[0];
 _ext[1] = extents[1];
 _ext[2] = extents[2];
 _ext[3] = extents[3];
 _ext[4] = extents[4];
 _ext[5] = extents[5];

 _box[0] = box[0];
 _box[1] = box[1];
 _box[2] = box[2];
 _box[3] = box[3];
 _box[4] = box[4];
 _box[5] = box[5];

 return true;

}
