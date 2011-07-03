//-- Matrix3d.cpp ------------------------------------------------------------
//
// Copyright (C) 2002-2005 Kenny Gruchalla. All rights reserved. 
// 
//
// A homogeneous 3-d Matrix class
//
//----------------------------------------------------------------------------

#include "Matrix3d.h"

#include <stdlib.h>

using namespace VAPoR;

//-- public  ------------------------------------------------------------------
// Matrix3d::Matrix3d()
// Default constructor -- creates the identity matrix
//-----------------------------------------------------------------------------
Matrix3d::Matrix3d()
{
  //
  // Create the identity matrix
  //
  at(0,0) = 1; at(0,1) = 0; at(0,2) = 0; at(0,3) = 0;
  at(1,0) = 0; at(1,1) = 1; at(1,2) = 0; at(1,3) = 0;
  at(2,0) = 0; at(2,1) = 0; at(2,2) = 1; at(2,3) = 0;
  at(3,0) = 0; at(3,1) = 0; at(3,2) = 0; at(3,3) = 1;
}

//-- public  ------------------------------------------------------------------
// Matrix3d::Matrix3d(...)
// Constructor
//-----------------------------------------------------------------------------
Matrix3d::Matrix3d(float x0, float y0, float z0, float w0,
                   float x1, float y1, float z1, float w1,
                   float x2, float y2, float z2, float w2,
                   float x3, float y3, float z3, float w3)
{
  at(0,0) = x0; at(0,1) = y0;
  at(0,2) = z0; at(0,3) = w0;

  at(1,0) = x1; at(1,1) = y1;
  at(1,2) = z1; at(1,3) = w1;

  at(2,0) = x2; at(2,1) = y2;
  at(2,2) = z2; at(2,3) = w2;

  at(3,0) = x3; at(3,1) = y3;
  at(3,2) = z3; at(3,3) = w3;
}


//-- public  ------------------------------------------------------------------
// Matrix3d::Matrix3d(...)
// Constructor from vectors
//-----------------------------------------------------------------------------
Matrix3d::Matrix3d(const Vect3d &x,
                   const Vect3d &y,
                   const Vect3d &z,
                   const Vect3d &w)
{
  at(0,0) = x.x(); at(0,1) = x.y();
  at(0,2) = x.z(); at(0,3) = x.w();

  at(1,0) = y.x(); at(1,1) = y.y();
  at(1,2) = y.z(); at(1,3) = y.w();

  at(2,0) = z.x(); at(2,1) = z.y();
  at(2,2) = z.z(); at(2,3) = z.w();

  at(3,0) = w.x(); at(3,1) = w.y();
  at(3,2) = w.z(); at(3,3) = w.w();
}

//-- public  ------------------------------------------------------------------
// Matrix3d::Matrix3d(...)
// Construct with transformations
//-----------------------------------------------------------------------------
Matrix3d::Matrix3d(const Transform3d *transform)
{
   Matrix3d matrix;

   for(int i=0; i<transform->transformations().size(); i++)
   {
      Transform3d::TransformBase *base = transform->transformations()[i];
      Transform3d::Translate *t = dynamic_cast<Transform3d::Translate*>(base);
      Transform3d::Rotate *r = dynamic_cast<Transform3d::Rotate*>(base);
      Transform3d::Scale *s = dynamic_cast<Transform3d::Scale*>(base);
      Transform3d::Matrix *m = dynamic_cast<Transform3d::Matrix*>(base);

      if (t) matrix *= Matrix3d(t);
      if (r) matrix *= Matrix3d(r);
      if (s) matrix *= Matrix3d(s);
      if (m) matrix *= Matrix3d(m);
   }

   *this = matrix;
}

//-- public  ------------------------------------------------------------------
// Matrix3d::Matrix3d(...)
// Construct with transformations
//-----------------------------------------------------------------------------
Matrix3d::Matrix3d(const Transform3d::Translate *t)
{
   //
   // Create the translation matrix
   //
   at(0,0) = 1; at(0,1) = 0; at(0,2) = 0; at(0,3) = t->x();
   at(1,0) = 0; at(1,1) = 1; at(1,2) = 0; at(1,3) = t->y();
   at(2,0) = 0; at(2,1) = 0; at(2,2) = 1; at(2,3) = t->z();
   at(3,0) = 0; at(3,1) = 0; at(3,2) = 0; at(3,3) = 1;
   
}

//-- public  ------------------------------------------------------------------
// Matrix3d::Matrix3d(...)
// Construct with transformations
//-----------------------------------------------------------------------------
Matrix3d::Matrix3d(const Transform3d::Rotate *r)
{
   //
   // Create the rotation matrix
   //
   float th = r->rad();

   Vect3d axis(r->axisx(), r->axisy(), r->axisz());
   axis.unitize();

   float ux = axis.x();
   float uy = axis.y();
   float uz = axis.z();

   at(0,0) = cos(th) + ux*ux*(1 - cos(th));
   at(0,1) = ux*uy*(1 - cos(th)) - uz*sin(th);
   at(0,2) = ux*uz*(1 - cos(th)) + uy*sin(th);
   at(0,3) = 0;

   at(1,0) = uy*ux*(1 - cos(th)) + uz*sin(th);
   at(1,1) = cos(th) + uy*uy*(1 - cos(th));
   at(1,2) = uy*uz*(1-cos(th)) - ux*sin(th);
   at(1,3) = 0;

   at(2,0) = uz*ux*(1 - cos(th)) - uy*sin(th);
   at(2,1) = uz*uy*(1 - cos(th)) + ux*sin(th);
   at(2,2) = cos(th) + uz*uz*(1 - cos(th));
   at(2,3) = 0;

   at(3,0) = 0;
   at(3,1) = 0;
   at(3,2) = 0;
   at(3,3) = 1;
}



//-- public  ------------------------------------------------------------------
// Matrix3d::Matrix3d(...)
// Construct with transformations
//-----------------------------------------------------------------------------
Matrix3d::Matrix3d(const Transform3d::Scale *s)
{
   //
   // Create the scale matrix
   //
   at(0,0) = s->x(); at(0,1) = 0;      at(0,2) = 0;      at(0,3) = 0;
   at(1,0) = 0;      at(1,1) = s->y(); at(1,2) = 0;      at(1,3) = 0;
   at(2,0) = 0;      at(2,1) = 0;      at(2,2) = s->z(); at(2,3) = 0;
   at(3,0) = 0;      at(3,1) = 0;      at(3,2) = 0;      at(3,3) = 1;   
}

//-- public  ------------------------------------------------------------------
// Matrix3d::Matrix3d(...)
// Construct with transformations
//-----------------------------------------------------------------------------
Matrix3d::Matrix3d(const Transform3d::Matrix *m)
{
   for (int i=0; i<16; i++) _data[i] = m->matrix()[i];
}


//-- public  ------------------------------------------------------------------
// Matrix3d::Matrix3d(const Matrix3d &m)
// Copy Constructor
//-----------------------------------------------------------------------------
Matrix3d::Matrix3d(const Matrix3d &m)
{
  at(0,0) = m.at(0,0); 
  at(0,1) = m.at(0,1);
  at(0,2) = m.at(0,2); 
  at(0,3) = m.at(0,3);

  at(1,0) = m.at(1,0); 
  at(1,1) = m.at(1,1);
  at(1,2) = m.at(1,2);
  at(1,3) = m.at(1,3);

  at(2,0) = m.at(2,0); 
  at(2,1) = m.at(2,1);
  at(2,2) = m.at(2,2); 
  at(2,3) = m.at(2,3);

  at(3,0) = m.at(3,0);
  at(3,1) = m.at(3,1);
  at(3,2) = m.at(3,2);
  at(3,3) = m.at(3,3);
}

//-- public  ------------------------------------------------------------------
// Matrix3d::Matrix3d(const float *m)
// Copy Constructor
//-----------------------------------------------------------------------------
Matrix3d::Matrix3d(const float *m)
{
   memcpy(_data, m, sizeof(float)*16);
}

//-- public  ------------------------------------------------------------------
// Matrix3d::~Matrix3d()
//-----------------------------------------------------------------------------
Matrix3d::~Matrix3d()
{
}

//-- public  ------------------------------------------------------------------
// Matrix3d &Matrix3d::operator=(const Matrix3d &m)
// Assingment
//-----------------------------------------------------------------------------
Matrix3d &Matrix3d::operator=(const Matrix3d &m)
{
  if (this != &m) 
  {
    at(0,0) = m.at(0,0); 
    at(0,1) = m.at(0,1);
    at(0,2) = m.at(0,2); 
    at(0,3) = m.at(0,3);
    
    at(1,0) = m.at(1,0); 
    at(1,1) = m.at(1,1);
    at(1,2) = m.at(1,2);
    at(1,3) = m.at(1,3);
    
    at(2,0) = m.at(2,0); 
    at(2,1) = m.at(2,1);
    at(2,2) = m.at(2,2); 
    at(2,3) = m.at(2,3);
    
    at(3,0) = m.at(3,0);
    at(3,1) = m.at(3,1);
    at(3,2) = m.at(3,2);
    at(3,3) = m.at(3,3);
  }

  return *this;
}

//-- public  ------------------------------------------------------------------
// Matrix3d::Matrix3d(const float *m)
// Copy Constructor
//-----------------------------------------------------------------------------
Matrix3d &Matrix3d::operator=(const float *m)
{
   memcpy(_data, m, sizeof(float)*16);

   return *this;
}

//-- public  ------------------------------------------------------------------
// Copy Transform3d
//-----------------------------------------------------------------------------
Matrix3d& Matrix3d::operator=(const Transform3d &t)
{
   Matrix3d matrix;

   for(int i=0; i<t.transformations().size(); i++)
   {
      Transform3d::TransformBase *base = t.transformations()[i];
      Transform3d::Translate *t = dynamic_cast<Transform3d::Translate*>(base);
      Transform3d::Rotate *r = dynamic_cast<Transform3d::Rotate*>(base);
      Transform3d::Scale *s = dynamic_cast<Transform3d::Scale*>(base);
      Transform3d::Matrix *m = dynamic_cast<Transform3d::Matrix*>(base);

      if (t) matrix *= Matrix3d(t);
      if (r) matrix *= Matrix3d(r);
      if (s) matrix *= Matrix3d(s);
      if (m) matrix *= Matrix3d(m);
   }

   *this = matrix;
   
   return *this;
}

//-- public  ------------------------------------------------------------------
// Matrix3d &Matrix3d::operator*=(float scalar)
// Multiply myself by a scalar
//-----------------------------------------------------------------------------
Matrix3d &Matrix3d::operator*=(float scalar)
{
  at(0,0) *= scalar;
  at(0,1) *= scalar;
  at(0,2) *= scalar;
  at(0,3) *= scalar;
  
  at(1,0) *= scalar;
  at(1,1) *= scalar;
  at(1,2) *= scalar;
  at(1,3) *= scalar;
  
  at(2,0) *= scalar;
  at(2,1) *= scalar;
  at(2,2) *= scalar;
  at(2,3) *= scalar;
  
  at(3,0) *= scalar;
  at(3,1) *= scalar;
  at(3,2) *= scalar;
  at(3,3) *= scalar;

  return *this;
}

//-- public  ------------------------------------------------------------------
// Matrix3d &Matrix3d::operator*=(const Matrix3d &m)
// Multiply myself by the matrix
//-----------------------------------------------------------------------------
Matrix3d &Matrix3d::operator*=(const Matrix3d &m)
{
  Matrix3d m1(*this);

  //
  // Row 0
  //
  at(0,0) = (m1.at(0,0) * m.at(0,0) +  
             m1.at(0,1) * m.at(1,0) +
             m1.at(0,2) * m.at(2,0) +
             m1.at(0,3) * m.at(3,0));
  
  at(0,1) = (m1.at(0,0) * m.at(0,1) +  
             m1.at(0,1) * m.at(1,1) +
             m1.at(0,2) * m.at(2,1) +
             m1.at(0,3) * m.at(3,1));
  
  at(0,2) = (m1.at(0,0) * m.at(0,2) +  
             m1.at(0,1) * m.at(1,2) +
             m1.at(0,2) * m.at(2,2) +
             m1.at(0,3) * m.at(3,2));
  
  at(0,3) = (m1.at(0,0) * m.at(0,3) +  
             m1.at(0,1) * m.at(1,3) +
             m1.at(0,2) * m.at(2,3) +
             m1.at(0,3) * m.at(3,3));
  //
  // Row 1
  //
  at(1,0) = (m1.at(1,0) * m.at(0,0) +  
             m1.at(1,1) * m.at(1,0) +
             m1.at(1,2) * m.at(2,0) +
             m1.at(1,3) * m.at(3,0));

  at(1,1) = (m1.at(1,0) * m.at(0,1) +  
             m1.at(1,1) * m.at(1,1) +
             m1.at(1,2) * m.at(2,1) +
             m1.at(1,3) * m.at(3,1));
  
  at(1,2) = (m1.at(1,0) * m.at(0,2) +  
             m1.at(1,1) * m.at(1,2) +
             m1.at(1,2) * m.at(2,2) +
             m1.at(1,3) * m.at(3,2));
  
  at(1,3) = (m1.at(1,0) * m.at(0,3) +  
             m1.at(1,1) * m.at(1,3) +
             m1.at(1,2) * m.at(2,3) +
             m1.at(1,3) * m.at(3,3));
  
  //
  // Row 2
  //
  at(2,0) = (m1.at(2,0) * m.at(0,0) +  
             m1.at(2,1) * m.at(1,0) +
             m1.at(2,2) * m.at(2,0) +
             m1.at(2,3) * m.at(3,0));
  
  at(2,1) = (m1.at(2,0) * m.at(0,1) +  
             m1.at(2,1) * m.at(1,1) +
             m1.at(2,2) * m.at(2,1) +
             m1.at(2,3) * m.at(3,1));
  
  at(2,2) = (m1.at(2,0) * m.at(0,2) +  
             m1.at(2,1) * m.at(1,2) +
             m1.at(2,2) * m.at(2,2) +
             m1.at(2,3) * m.at(3,2));
  
  at(2,3) = (m1.at(2,0) * m.at(0,3) +  
             m1.at(2,1) * m.at(1,3) +
             m1.at(2,2) * m.at(2,3) +
             m1.at(2,3) * m.at(3,3));
  
  //
  // Row 3
  //
  at(3,0) = (m1.at(3,0) * m.at(0,0) +  
             m1.at(3,1) * m.at(1,0) +
             m1.at(3,2) * m.at(2,0) +
             m1.at(3,3) * m.at(3,0));
  
  at(3,1) = (m1.at(3,0) * m.at(0,1) +  
             m1.at(3,1) * m.at(1,1) +
             m1.at(3,2) * m.at(2,1) +
             m1.at(3,3) * m.at(3,1));
  
  at(3,2) = (m1.at(3,0) * m.at(0,2) +  
             m1.at(3,1) * m.at(1,2) +
             m1.at(3,2) * m.at(2,2) +
             m1.at(3,3) * m.at(3,2));
  
  at(3,3) = (m1.at(3,0) * m.at(0,3) +  
             m1.at(3,1) * m.at(1,3) +
             m1.at(3,2) * m.at(2,3) +
             m1.at(3,3) * m.at(3,3));

  return *this;
}


//-- public  ------------------------------------------------------------------
// Matrix3d &Matrix3d::operator+=(const Matrix3d &m)
// Add myself to the matrix 
//-----------------------------------------------------------------------------
Matrix3d &Matrix3d::operator+=(const Matrix3d &m)
{
  at(0,0) += m.at(0,0);
  at(0,1) += m.at(0,1);
  at(0,2) += m.at(0,2); 
  at(0,3) += m.at(0,3);
  
  at(1,0) += m.at(1,0);
  at(1,1) += m.at(1,1);
  at(1,2) += m.at(1,2);
  at(1,3) += m.at(1,3);
  
  at(2,0) += m.at(2,0);
  at(2,1) += m.at(2,1);
  at(2,2) += m.at(2,2);
  at(2,3) += m.at(2,3);
  
  at(3,0) += m.at(3,0);
  at(3,1) += m.at(3,1);
  at(3,2) += m.at(3,2);
  at(3,3) += m.at(3,3);

  return *this;
}

//-- public  ------------------------------------------------------------------
// float Matrix3d::determinant()
// Return the determinant of the matrix
//-----------------------------------------------------------------------------
float Matrix3d::determinant()
{
  float a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4;

  a1 = at(0,0); b1 = at(0,1); 
  c1 = at(0,2); d1 = at(0,3);

  a2 = at(1,0); b2 = at(1,1); 
  c2 = at(1,2); d2 = at(1,3);

  a3 = at(2,0); b3 = at(2,1); 
  c3 = at(2,2); d3 = at(2,3);

  a4 = at(3,0); b4 = at(3,1); 
  c4 = at(3,2); d4 = at(3,3);

  return (  a1 * determinant3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4)
            - b1 * determinant3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4)
            + c1 * determinant3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4)
            - d1 * determinant3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4));
}

//-- public ------------------------------------------------------------------
// void Matrix3d::transpose()
//
//----------------------------------------------------------------------------
Matrix3d& Matrix3d::transpose()
{
  int i, j;
  float temp;

  for (i=0; i<4; i++)
  {
    for(j=i; j<4; j++)
    {
      temp = at(i,j);
      at(i,j) = at(j,i);
      at(j,i) = temp;
    }
  }

  return *this;
}


//-- public ------------------------------------------------------------------
// Matrix3d& Matrix3d::inverse(Matrix3d &out)
//
// Returns the inverse of this matrix (adapted from vtkMatrix4x4)
//
// calculate the inverse of a 4x4 matrix
//
//     -1     1  
//     A  = _____ adjoint A
//          det A
//
//----------------------------------------------------------------------------
Matrix3d& Matrix3d::inverse(Matrix3d &out)
{
  int i, j;
  float det;

  // calculate the 4x4 determinent
  // if the determinent is zero, 
  // then the inverse matrix is not unique.

  det = determinant();

  if ( det == 0.0 ) 
  {
    return out;
  }

  // calculate the adjoint matrix
  adjoint(out);

  // scale the adjoint matrix to get the inverse
  for (i=0; i<4; i++)
  {
    for(j=0; j<4; j++)
    {
      out.at(i,j) = out.at(i,j) / det;
    }
  }

  return out;
}

//-- public ------------------------------------------------------------------
// Matrix3d& Matrix3d::adjoint(Matrix3d &out)
//
// Calculate the adjoint of this matrix  (adapted from vtkMatrix4x4) 
//
// Let a   denote the minor determinant of matrix A obtained by
//      ij
// deleting the ith row and jth column from A.
//
//                    i+j
//     Let  b   = (-1)    a
//           ij            ji
//
//    The matrix B = (b  ) is the adjoint of A
//                     ij
//----------------------------------------------------------------------------
Matrix3d& Matrix3d::adjoint(Matrix3d &out)
{
  float a1, a2, a3, a4, b1, b2, b3, b4;
  float c1, c2, c3, c4, d1, d2, d3, d4;

  a1 = at(0,0); b1 = at(0,1); 
  c1 = at(0,2); d1 = at(0,3);

  a2 = at(1,0); b2 = at(1,1); 
  c2 = at(1,2); d2 = at(1,3);

  a3 = at(2,0); b3 = at(2,1);
  c3 = at(2,2); d3 = at(2,3);

  a4 = at(3,0); b4 = at(3,1); 
  c4 = at(3,2); d4 = at(3,3);

  // row column labeling reversed since we transpose rows & columns

  out.at(0,0) = determinant3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4);
  out.at(1,0) = - determinant3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4);
  out.at(2,0) = determinant3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4);
  out.at(3,0) = - determinant3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4);

  out.at(0,1) = - determinant3x3( b1, b3, b4, c1, c3, c4, d1, d3, d4);
  out.at(1,1) = determinant3x3( a1, a3, a4, c1, c3, c4, d1, d3, d4);
  out.at(2,1) = - determinant3x3( a1, a3, a4, b1, b3, b4, d1, d3, d4);
  out.at(3,1) = determinant3x3( a1, a3, a4, b1, b3, b4, c1, c3, c4);
        
  out.at(0,2) = determinant3x3( b1, b2, b4, c1, c2, c4, d1, d2, d4);
  out.at(1,2) = - determinant3x3( a1, a2, a4, c1, c2, c4, d1, d2, d4);
  out.at(2,2) = determinant3x3( a1, a2, a4, b1, b2, b4, d1, d2, d4);
  out.at(3,2) = - determinant3x3( a1, a2, a4, b1, b2, b4, c1, c2, c4);
        
  out.at(0,3) = - determinant3x3( b1, b2, b3, c1, c2, c3, d1, d2, d3);
  out.at(1,3) = determinant3x3( a1, a2, a3, c1, c2, c3, d1, d2, d3);
  out.at(2,3) = - determinant3x3( a1, a2, a3, b1, b2, b3, d1, d2, d3);
  out.at(3,3) = determinant3x3( a1, a2, a3, b1, b2, b3, c1, c2, c3);

  return out;
}

//-- public -------------------------------------------------------------------
// operator<<
//-----------------------------------------------------------------------------
std::ostream &operator<<(std::ostream &o, const Matrix3d &m)
{
  o << m.at(0,0) << " " << m.at(0,1) << " " << m.at(0,2) 
    << " " << m.at(0,3) << std::endl
    << m.at(1,0) << " " << m.at(1,1) << " " << m.at(1,2) 
    << " " << m.at(1,3) << std::endl
    << m.at(2,0) << " " << m.at(2,1) << " " << m.at(2,2) 
    << " " << m.at(2,3) << std::endl
    << m.at(3,0) << " " << m.at(3,1) << " " << m.at(3,2) 
    << " " << m.at(3,3) << std::endl;

  return o;
}

