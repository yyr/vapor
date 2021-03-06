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

#include "glutil.h"	// Must be included first!!!
#include <qgl.h>
#include <qgl.h>
#include <cctype>
#include "params.h"
#include "DVRTexture3d.h"
#include "TextureBrick.h"
#include "BBox.h"

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
DVRTexture3d::DVRTexture3d(
  int precision, int nvars, int nthreads
) :
  _nx(0),
  _ny(0),
  _nz(0),
  _renderFast(false),
  _delta(0.0),
  _samples(0),
  _samplingRate(2.0),
  _minimumSamples(256),
  _minimumSamplesFast(64),
  _maxTexture(0),
  _maxBrickDim(128), // NOTE: This should always be <= _maxTexture
  _lastRegion(),
  _precision(precision),
  _nvars(nvars)
{
	MyBase::SetDiagMsg(
		"DVRTexture3d::DVRTexture3d( %d %d %d)", 
		precision, nvars, nthreads
	);
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
DVRTexture3d::~DVRTexture3d() 
{
	MyBase::SetDiagMsg("DVRTexture3d::~DVRTexture3d()");

  for (int i=0; i<_bricks.size(); i++)
  {
    delete _bricks[i];
    _bricks[i] = NULL;
  }

  _bricks.clear();
}

//----------------------------------------------------------------------------
// Derived classes should call this function to ensure that the data and
// texture extents are being set and the texture bricks are being created. 
//----------------------------------------------------------------------------
int DVRTexture3d::SetRegion(
	const RegularGrid *rg, const float range[2], int num
) { 
  size_t dims[3];
  rg->GetDimensions(dims);

  _dmin.x = 0;
  _dmin.y = 0;
  _dmin.z = 0;
  _dmax.x = dims[0]-1;
  _dmax.y = dims[1]-1;
  _dmax.z = dims[2]-1;

  if (_maxTexture == 0) {
    _maxTexture = TextureBrick::maxTextureSize(rg, _precision, _nvars);
  }

  if (_lastRegion.update(rg, _maxTexture))
  {
    assert(num == 0);
    _nx = dims[0];
    _ny = dims[1];
    _nz = dims[2];

    double extents[6];
    rg->GetUserExtents(extents);

    _vmin.x = extents[0];
    _vmin.y = extents[1];
    _vmin.z = extents[2];
    _vmax.x = extents[3];
    _vmax.y = extents[4];
    _vmax.z = extents[5];
    
    for (int i=0; i<_bricks.size(); i++)
    {
      delete _bricks[i];
      _bricks[i] = NULL;
    }
    
    _bricks.clear();
    
    if (_nx <= _maxTexture && _ny <= _maxTexture && _nz <= _maxTexture)
    {
      size_t bx = _nx; size_t by = _ny; size_t bz = _nz;
      MyBase::SetDiagMsg("DVRTexture3d::SetRegion() - Not bricking textures");
      //
      // The data will fit completely within texture memory, so no bricking is 
      // needed. We can save a few cycles by setting up the single brick here,
      // rather than calling buildBricks(...). 
      //
      _bricks.push_back(new TextureBrick(rg, _precision, _nvars));

      _bricks[0]->dataMin(extents[0], extents[1], extents[2]);
      _bricks[0]->dataMax(extents[3], extents[4], extents[5]);
      _bricks[0]->volumeMin(extents[0], extents[1], extents[2]);
      _bricks[0]->volumeMax(extents[3], extents[4], extents[5]);

      //
      // Set the texture coordinates of the brick.
      // Sample width is 1.0 / #texels. First sample located
      // at 1/2 sampling width.
      //
      float deltatx = 1.0 / (float) bx;
      float deltaty = 1.0 / (float) by;
      float deltatz = 1.0 / (float) bz;
      _bricks[0]->textureMin(deltatx/2.0, deltaty/2.0, deltatz/2.0);
      _bricks[0]->textureMax(1.0-(deltatx*0.5), 1.0-(deltaty*0.5), 1.0-(deltatz*0.5));
      _bricks[0]->fill(rg, range, num);
      
      if (num == _nvars - 1) loadTexture(_bricks[0]);
    } 
    else
    {
      MyBase::SetDiagMsg("DVRTexture3d::SetRegion() - Bricking textures");
      //
      // The data will not fit completely within texture memory, need to 
      // subdivide into multiple bricks.
      //
      buildBricks(rg, range, num);
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
      _bricks[i]->refill(rg, range, num);
      if (num == _nvars - 1) loadTexture(_bricks[i]);
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
  float m[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, m);
  Matrix3d modelview(m);   

  Matrix3d modelviewInverse;
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
      renderBrick(brick, modelview, modelviewInverse);
    }
  }
}

//----------------------------------------------------------------------------
// Calculate the sampling distance
//----------------------------------------------------------------------------
void DVRTexture3d::calculateSampling()
{
  //
  // Get the modelview matrix and its inverse
  //

  float m[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, m);
  Matrix3d modelview(m);   

  Matrix3d modelviewInverse;
  modelview.inverse(modelviewInverse);

  //
  // Normal transformation matrix and inverse
  //
  Matrix3d modviewNorm(modelviewInverse);
  modviewNorm.transpose();

  Matrix3d modviewNormInverse;
  modviewNorm.inverse(modviewNormInverse);

  //
  // Calculate the slice plane normal (i.e., the view plane normal 
  // inverse transformed from Eye into Object space). 
  //
  Vect3d viewPlaneNormal(0,0,1,0);
  _slicePlaneNormal = modviewNormInverse * viewPlaneNormal;
  _slicePlaneNormal.w(0.0);
  _slicePlaneNormal.unitize();

  //
  // The volume in Object coordinates, and transformed into Eye
  // coordinates. 
  //
  BBox volumeBox(_vmin, _vmax); // user extent units
  BBox volumeBoxObjectCoord(_vmin, _vmax);
  BBox voxelBox(_dmin, _dmax);  // voxel units
  BBox voxelBoxObjectCoord(_dmin, _dmax);

  volumeBox.transform(modelview);
  voxelBox.transform(modelview);

  //
  // Define the view aligned slice plane. The plane will be positioned
  // at the volume corner (_slicePoint) furthest from the eye. 
  // (object coordinates)
  //
  // Compute the distance between the first and last sampling plane
  // First compute plane equation for plane passing through _slicePoint
  // (first sampling plane), and perpendicular to _slicePlaneNormal.
  //
  // Use plane equation: Ax + Bx + Cx + D = 0
  //
  // Then compute distance between farPoint and the plane 
  //
  // Use distance to plane equation: d = (Ax+Bx+Cz+D)/sqrt(A*A+B*B+C*C)
  //
  _slicePoint = volumeBoxObjectCoord[volumeBox.minIndex()]; 
  Vect3d farPoint = volumeBoxObjectCoord[volumeBox.maxIndex()]; 

  double A = _slicePlaneNormal.x();
  double B = _slicePlaneNormal.y();
  double C = _slicePlaneNormal.z();
  double D = -1.0*(A*_slicePoint.x() + B*_slicePoint.y() + C*_slicePoint.z());

  double distance = (A*farPoint.x() + B*farPoint.y() + C*farPoint.z() + D) /
    sqrt(A*A + B*B + C*C);

  distance = fabs(distance);

  //
  // Next compute the distance between far and near sampling planes
  // in voxel units
  //
  Vect3d slicePointVoxel = voxelBoxObjectCoord[voxelBox.minIndex()]; 
  Vect3d farPointVoxel = voxelBoxObjectCoord[voxelBox.maxIndex()]; 

  D = -1.0*(A*slicePointVoxel.x() + B*slicePointVoxel.y() + C*slicePointVoxel.z());

  double distanceVoxel = (A*farPointVoxel.x() + B*farPointVoxel.y() + C*farPointVoxel.z() + D) /
    sqrt(A*A + B*B + C*C);


  //
  // Sampling theory tells us that we need two samples per voxel for
  // reconstruction of the signal. 
  //
  _samplingRate = 2.0;
  _samples = _samplingRate * distanceVoxel;
  if (_samples < 2) _samples = 2;

  //
  // But we'll oversample to _minimumSamples (if we're not in fast render mode)
  // for a smooth appearance at low refinement levels.
  //
  if ((_renderFast && _samples < _minimumSamplesFast) ||
	(!_renderFast && _samples < _minimumSamples)) 
  {
    if (_renderFast) _samples = _minimumSamplesFast;
    else _samples = _minimumSamples;

    //
    // Recalculate the sampling rate -- used later for the opacity correction
    //
    _samplingRate = _samples / distanceVoxel;
  }

  _delta = distance / (_samples-1);

//cout << "_delta = " << _delta << endl;
}

//----------------------------------------------------------------------------
// Render a single brick
//----------------------------------------------------------------------------
void DVRTexture3d::renderBrick(const TextureBrick *brick,
                                         const Matrix3d &modelview,
                                         const Matrix3d &modelviewInverse)

{
	drawViewAlignedSlices(brick, modelview, modelviewInverse);
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
  // transform the volume into eye coordinates
  //
  rotatedBox.transform(modelview);

  Vect3d slicePoint = _slicePoint;

  //
  // Calculate the distance between slices
  //
  Vect3d sliceDelta = _slicePlaneNormal * _delta;

  //
  // Calculate edge intersections between the plane and the boxes
  //
  Vect3d verts[6];   // for edge intersections
  Vect3d tverts[6];  // for texture intersections
  Vect3d rverts[6];  // for transformed edge intersections


  for(int i = 0 ; i < _samples; i++)
  { 
    int order[6] ={0,1,2,3,4,5};
    int size = 0;

    //
    // Intersect the slice plane with the bounding boxes
    //
    size = intersect(slicePoint, _slicePlaneNormal, 
                     volumeBox, verts, 
                     textureBox, tverts, 
                     rotatedBox, rverts);
    //
    // Calculate the convex hull of the vertices (eye coordinates)
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
	//
	// Geometrically, a plane intersecting a box should only result in 
	// up to 6 intersection points. However, with floating point calculations
	// more intersections can occur. The right way to addres this would be
	// to determine which intersection point is bogus, but here we simply
	// punt if more than 6 intersections result.
	//
	if (intersections == 6) return (intersections);
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
void DVRTexture3d::buildBricks(
	const RegularGrid *rg, const float range[2], int num
) {
  bool layered = ((dynamic_cast<const LayeredGrid *>(rg)) != NULL);
  size_t dims[3];
  rg->GetDimensions(dims);
  // 
  // Note: we are using _maxBrickDim as the maximum brick dimension, not 
  // _maxTexture. The _maxTexture will typically be on the order of 512 
  // (i.e., a maximum texture of 512^3); however, we can get better performance
  // using a smaller brick size, since we are using uniform brick sizing and 
  // bricks on the borders may contain very small subset of data.
  //
  int brick_dim = min(_maxTexture, _maxBrickDim);
  size_t bxp2 = nextPowerOf2(_nx);
  size_t byp2 = nextPowerOf2(_ny);
  size_t bzp2 = nextPowerOf2(_nz);
  while (bxp2 > brick_dim)
  {
    bxp2 /= 2;
  }

  while (byp2 > brick_dim)
  {
    byp2 /= 2;
  }

  while (bzp2 > brick_dim)
  {
    bzp2 /= 2;
  }

  //
  // Compute the number of bricks in the cardinal directions
  //
  int nbricks[3] = {(int)ceil((double)(_nx-1)/(bxp2-1)),
                    (int)ceil((double)(_ny-1)/(byp2-1)),
                    (int)ceil((double)(_nz-1)/(bzp2-1))};

  //
  // Populate bricks. Interior bricks have a ghost zone on the far-most
  // face.  Thus interior brick dimensions are one greater than 
  // one what actually gets rendered
  //
  TextureBrick *brick = NULL;

  // brick region of interest
  size_t broi[6] = {0,0,0,dims[0]-1, dims[1]-1, dims[2]-1};

  // data offset
  int offset[3];

  int bricknum = 0;
  offset[2] = 0;
  broi[2] = 0;
  broi[5] = bzp2-2;

  size_t bz = bzp2;
  for(int z=0; z<nbricks[2]; ++z)
  {
    if (broi[5] > dims[2]-1) {	// boundary Z brick
      broi[5] = dims[2]-1;
      bz = broi[5]-broi[2]+1;
    }

    offset[1] = 0;
    broi[1] = 0;
    broi[4] = byp2-2;

    size_t by = byp2;
    for (int y=0; y<nbricks[1]; y++)
    {
      if (broi[4] > dims[1]-1) {	// boundary Y brick
        broi[4] = dims[1]-1;
        by = broi[4]-broi[1]+1;
      }

      offset[0] = 0;
      broi[0] = 0;
      broi[3] = bxp2-2;

      size_t bx = bxp2;
      for (int x=0; x<nbricks[0]; x++)
      {
        if (broi[3] > dims[0]-1) {	// boundary X brick
          broi[3] = dims[0]-1;
          bx = broi[3]-broi[0]+1;
        }

        if (num != 0) {
          assert(_bricks.size() > bricknum);
          brick = _bricks[bricknum];
        }
        else {
          brick = new TextureBrick(rg, _precision, _nvars);
          _bricks.push_back(brick); 

          //
          // Set the extents of the brick's data box
          //

          double extents[6];
          rg->GetBoundingBox(broi, broi+3, extents);
          if (layered) {

            // For layered data the Z extents of neighboring bricks in X 
            // and Y can vary substantially. Also, with ROMS data, for
            // example, some bricks can be extremly thin in Z direction.
            // This possibly results in precision errors that are corrected
            // by the code below, which ensures all bricks in the same X-Y
            // plane have the same Z extents.
            //
            size_t bottom[] = {0,0,broi[2]};
            size_t top[] = {dims[0]-1, dims[1]-1,broi[5]};
            double zexts[6];
            rg->GetBoundingBox(bottom, top, zexts);
            extents[2] = zexts[2];
            extents[5] = zexts[5];
          }
          brick->dataMin(extents[0], extents[1], extents[2]);
          brick->dataMax(extents[3], extents[4], extents[5]);
          brick->volumeMin(extents[0], extents[1], extents[2]);
          brick->volumeMax(extents[3], extents[4], extents[5]);

          //
          // Set the texture coordinates of the brick.
          // Sample width is 1.0 / #texels. First sample located
          // at 1/2 sampling width.
          //
          //
          float deltatx = 1.0 / (float) bx;
          float deltaty = 1.0 / (float) by;
          float deltatz = 1.0 / (float) bz;
          brick->textureMin(deltatx*0.5, deltaty*0.5, deltatz*0.5);
          brick->textureMax(1.0-(deltatx*0.5), 1.0-(deltaty*0.5), 1.0-(deltatz*0.5));
        }

        //
        // Fill the brick
        //
        brick->fill(
            rg,range,num,
			bx,by,bz, bx,by,bz,
            offset[0], offset[1], offset[2]
        );

        if (num == _nvars - 1) loadTexture(brick);

        //
        // increment the brick block box (for the next brick in the x-axis) by 
        // the size of a brick (minus 1 for texture overlap).
        // 
        offset[0] += bx-2;
        broi[0] = broi[3];
        broi[3] += bx-2;
        bricknum++;
      }

      //
      // increment the brick block box (for the next brick in the y-axis)
      // by the size of a brick (minus 1 for texture overlap).
      // 
      offset[1] += by-2;
      broi[1] = broi[4];
      broi[4] += by-2;
    }

    //
    // increment the brick block box (for the next brick in the z-axis) 
    // by the size of a brick (minus 1 for texture overlap).
    // 
    offset[2] += bz-2;
    broi[2] = broi[5];
    broi[5] += bz-2;
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


void DVRTexture3d::SetMaxTexture(int texsize) {
	_maxTexture = texsize;
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
  _ext[0] = _ext[1] = _ext[2] = _ext[3] = _ext[4] = _ext[5] = 0.0;
}

//----------------------------------------------------------------------------
// Update the region state. Returns true if the region has changed, false
// otherwise.
//----------------------------------------------------------------------------
bool DVRTexture3d::RegionState::update(
	const RegularGrid *rg, int tex_size
)
{
  double extents[6];
  rg->GetUserExtents(extents);

  size_t dims[3];
  rg->GetDimensions(dims);

  if (dims[0] == _dim[0] && dims[1] == _dim[1] &&  dims[2] == _dim[2] &&

      _ext[0] == extents[0] &&
      _ext[1] == extents[1] &&
      _ext[2] == extents[2] &&
      _ext[3] == extents[3] &&
      _ext[4] == extents[4] &&
      _ext[5] == extents[5] &&

      _tex_size == tex_size)
  {
    return false;
  }

 _dim[0] = dims[0]; 
 _dim[1] = dims[1]; 
 _dim[2] = dims[2];

 _ext[0] = extents[0];
 _ext[1] = extents[1];
 _ext[2] = extents[2];
 _ext[3] = extents[3];
 _ext[4] = extents[4];
 _ext[5] = extents[5];

 _tex_size = tex_size;

 return true;

}
