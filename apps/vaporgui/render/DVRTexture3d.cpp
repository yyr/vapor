//--DVRTexture3d.h -----------------------------------------------------------
//
// Copyright (C) 2005 Kenny Gruchalla.  All rights reserved.
//
// Volume rendering engine based on 3d-textured view aligned slices. 
//
//----------------------------------------------------------------------------

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <qgl.h>

#include "DVRTexture3d.h"
#include "messagereporter.h"

#include "ShaderProgram.h"
#include "BBox.h"
#include "glutil.h"

#include "Matrix3d.h"
#include "Point3d.h"

using namespace VAPoR;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DVRTexture3d::DVRTexture3d(int * , char **, DataType_T type, int nthreads) :
  _data(NULL),
  _shader(NULL),
  _colormap(NULL),
  _delta(0.0),
  _texid(0),
  _cmapid(0),
  _nx(0),
  _ny(0),
  _nz(0)
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
//
//----------------------------------------------------------------------------
int DVRTexture3d::GraphicsInit() 
{
  glewInit();

  initTextures();

  //
  // Create, Load & Compile the shader program
  //
  _shader = new ShaderProgram();
  _shader->create();

  if (!_shader->loadFragmentShader("volume.frag"))
  {
    BailOut("OpenGL: Could not open file volume.frag",__FILE__,__LINE__);
  }

  if (!_shader->compile())
  {
    return -1;
  }

  //
  // Set up initial uniform values
  //
  _shader->enable();
  glUniform1i(_shader->uniformLocation("colormap"), 1);
  glUniform1i(_shader->uniformLocation("volumeTexture"), 0);
  glUniform1i(_shader->uniformLocation("shading"), 0);
  _shader->disable();

  return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRTexture3d::SetRegion(void *data,
                            int nx, int ny, int nz,
                            const int data_roi[6],
                            const float extents[6]) 
{ 
  glActiveTexture(GL_TEXTURE0);

  if (_nx != nx || _ny != ny || _nz != nz)
  {
    _nx = nx; _ny = ny; _nz = nz;

    _shader->enable();
    glUniform3f(_shader->uniformLocation("dimensions"), nx, ny, nz);
    _shader->disable();

    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, _nx, _ny, _nz, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, data);
  }
  else if (_data != data)
  {
    _data = data;

    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, _nx, _ny, _nz, GL_LUMINANCE,
                    GL_UNSIGNED_BYTE, data);  
  }

  _vmin.x = extents[0]; _vmin.y = extents[1]; _vmin.z = extents[2];
  _vmax.x = extents[3]; _vmax.y = extents[4]; _vmax.z = extents[5];

  _tmin.x = (float)data_roi[0]/nx; 
  _tmin.y = (float)data_roi[1]/ny; 
  _tmin.z = (float)data_roi[2]/nz;
  
  _tmax.x = (float)data_roi[3]/nx; 
  _tmax.y = (float)data_roi[4]/ny; 
  _tmax.z = (float)data_roi[5]/nz;
  
  _delta = fabs(extents[2]-extents[5]) / _nz; 

  glFlush();

  return 0;
}



//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRTexture3d::Render(const float matrix[16])

{
  _shader->enable();

  glEnable(GL_CULL_FACE);
  glPolygonMode(GL_FRONT, GL_FILL);
  glPolygonMode(GL_BACK, GL_LINE);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_3D);
  glBindTexture(GL_TEXTURE_3D, _texid);

  glEnable(GL_TEXTURE_1D);

  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);  
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  drawViewAlignedSlices();

  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_3D);
  glDisable(GL_TEXTURE_1D);

  glDepthMask(GL_TRUE);

  _shader->disable();

  return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRTexture3d::HasType(DataType_T type) 
{
  if (type == UINT8) 
    return(1);
  else 
    return(0);
}

//----------------------------------------------------------------------------
// Set the color table used to render the volume without any opacity 
// correction.
//----------------------------------------------------------------------------
void DVRTexture3d::SetCLUT(const float ctab[256][4]) 
{
  for (int i=0; i<256; i++)
  {
    _colormap[i*4+0] = ctab[i][0];
    _colormap[i*4+1] = ctab[i][1];
    _colormap[i*4+2] = ctab[i][2];
    _colormap[i*4+3] = ctab[i][3];
  }

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_1D, _cmapid);
  
  glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGBA,
                  GL_FLOAT, _colormap);
}

//----------------------------------------------------------------------------
// Set the color table used to render the volume applying an opacity 
// correction.
//----------------------------------------------------------------------------
void DVRTexture3d::SetOLUT(const float atab[256][4], const int numRefinements)
{
  //
  // Calculate opacity correction. Delta is 1 for the fineest refinement, 
  // multiplied by 2^n (the sampling distance)
  //
  double delta = (double)(1<<numRefinements);

  for(int i=0; i<256; i++) 
  {
    double opac = atab[i][3];

    opac = 1.0 - pow((1.0 - opac), delta);
  
    if (opac > 1.0)
    {
      opac = 1.0;
    }

    _colormap[i*4+0] = atab[i][0];
    _colormap[i*4+1] = atab[i][1];
    _colormap[i*4+2] = atab[i][2];
    _colormap[i*4+3] = opac;
  }

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_1D, _cmapid);
  
  glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGBA,
                  GL_FLOAT, _colormap);

  glFlush();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRTexture3d::SetLightingOnOff(int on) 
{
  _shader->enable();

  glUniform1i(_shader->uniformLocation("shading"), on);

  _shader->enable();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRTexture3d::SetLightingCoeff(float kd, float ka, float ks, float expS)
{
  _shader->enable();

  glUniform1f(_shader->uniformLocation("kd"), kd);
  glUniform1f(_shader->uniformLocation("ka"), ka);
  glUniform1f(_shader->uniformLocation("ks"), ks);
  glUniform1f(_shader->uniformLocation("expS"), expS);

  _shader->enable();
}


//----------------------------------------------------------------------------
// Initalize the textures (i.e., 3d volume texture and the 1D colormap texture)
//----------------------------------------------------------------------------
void DVRTexture3d::initTextures()
{
  //
  // Setup the 3d texture
  //
  glGenTextures(1, &_texid);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_3D, _texid);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  

  // Set texture border behavior
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  
  // Set texture interpolation method
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


  //
  // Setup the colormap texture
  //
  glGenTextures(1, &_cmapid);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_1D, _cmapid);

  _colormap = new float[256*4];
  
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA,
               GL_FLOAT, _colormap);

  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

  glFlush();
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

