//-- Matrix3d.h ---------------------------------------------------------------
//
// Copyright (C) 2002-2005 Kenny Gruchalla. All rights reserved. 
//
// A homogeneous 3-dimensional Matrix class
//
//-----------------------------------------------------------------------------

#ifndef _Matrix3d_
#define _Matrix3d_

#include <math.h>
#include <iostream>

#include "Vect3d.h"
#include "Point3d.h"
#include "Transform3d.h"

namespace VAPoR
{

class RENDER_API Matrix3d
{
public:

  // Default Constructor - Identity Matrix.  
  Matrix3d();

  // Specify the initial cell values 
  Matrix3d(double x0, double y0, double z0, double w0,
           double x1, double y1, double z1, double w1,
           double x2, double y2, double z2, double w2,
           double x3, double y3, double z3, double w3);

  Matrix3d(const double *f);
  Matrix3d(const float *f);

  // Construct with vectors
  Matrix3d(const Vect3d &x,
           const Vect3d &y,
           const Vect3d &z,
           const Vect3d &w);

  // Construct with transformations
  Matrix3d(const Transform3d *transform);
  Matrix3d(const Transform3d::Translate *t);
  Matrix3d(const Transform3d::Rotate *r);
  Matrix3d(const Transform3d::Scale *s);
  Matrix3d(const Transform3d::Matrix *m);

  // Copy Constructor
  Matrix3d(const Matrix3d &m);

  // Destructor
  virtual ~Matrix3d();

  // Assignment.
  Matrix3d &operator=(const Matrix3d &m);
  Matrix3d &operator=(const double *m);
  Matrix3d &operator=(const float *m);
  Matrix3d &operator=(const Transform3d &t);

  // Get/set my value at the given index.
  double &operator()(int x, int y);
  double operator()(int x, int y) const;

  // get/set function
  double at(int x, int y) const;
  double& at(int x, int y);

  // Multiply my entries by the scalar.
  Matrix3d &operator*=(double scalar);

  // Multiply myself by the Matrix.
  Matrix3d &operator*=(const Matrix3d &m);

  // Add the given Matrix to me.
  Matrix3d &operator+=(const Matrix3d &m);

  // Determinant
  double determinant();
  
  // Transpose
  Matrix3d& transpose();

  // Inverse
  Matrix3d& inverse(Matrix3d &out); 

  // Adjoint
  Matrix3d& adjoint(Matrix3d &out);
       
  friend std::ostream &operator<<(std::ostream &o, const Matrix3d &m);

  friend Vect3d   operator*(const Matrix3d& m, const Vect3d &d);
  friend Point3d  operator*(const Matrix3d& m, const Point3d &d);
  friend Matrix3d operator*(const Matrix3d& m, const Matrix3d &d);

  // data
  void data(float *m) const {
    for (int i=0; i<16; i++) m[i] = (float) _data[i];
  }
  void data(double *m) const {
    for (int i=0; i<16; i++) m[i] = _data[i];
  }

protected:

  double determinant3x3(double a1, double a2, double a3, 
                        double b1, double b2, double b3, 
                        double c1, double c2, double c3);
  
protected:

  // Member data. Column-major for OpenGL compatibility.
  double _data[16];
};

//
// Inline functions.
//

// -- public -----------------------------------------------------------------
// inline double &Matrix3d::operator()(int x, int y)
// Index operator
//----------------------------------------------------------------------------
inline double &Matrix3d::operator()(int x, int y)
{
  return _data[x*4 + y];
}

// -- public -----------------------------------------------------------------
// inline double Matrix3d::operator()(int x, int y) const
// Index operator
//----------------------------------------------------------------------------
inline double Matrix3d::operator()(int x, int y) const
{
  return _data[x*4 + y];
}


// -- public -----------------------------------------------------------------
// inline double Matrix3d::at(int x, int y) const
// const index operator
//----------------------------------------------------------------------------
inline double Matrix3d::at(int x, int y) const
{
  return _data[x*4 + y];
}

// -- public -----------------------------------------------------------------
// inline double& Matrix3d::at(int x, int y)
// non-const index operator
//----------------------------------------------------------------------------
inline double& Matrix3d::at(int x, int y)
{
  return _data[x*4 + y];
}



// -- fileScope --------------------------------------------------------------
// inline int operator==(const Matrix3d &m1, const Matrix3d &m2)
// Logical equality comparison between two Matrices.
//----------------------------------------------------------------------------
inline int operator==(const Matrix3d &m1, const Matrix3d &m2)
{
  return ((m1.at(0,0) == m2.at(0,0)) && 
          (m1.at(0,1) == m2.at(0,1)) && 
          (m1.at(0,2) == m2.at(0,2)) && 
          (m1.at(0,3) == m2.at(0,3)) && 
          (m1.at(1,0) == m2.at(1,0)) && 
          (m1.at(1,1) == m2.at(1,1)) && 
          (m1.at(1,2) == m2.at(1,2)) && 
          (m1.at(1,3) == m2.at(1,3)) && 
          (m1.at(2,0) == m2.at(2,0)) && 
          (m1.at(2,1) == m2.at(2,1)) && 
          (m1.at(2,2) == m2.at(2,2)) && 
          (m1.at(2,3) == m2.at(2,3)) && 
          (m1.at(3,0) == m2.at(3,0)) && 
          (m1.at(3,1) == m2.at(3,1)) && 
          (m1.at(3,2) == m2.at(3,2)) && 
          (m1.at(3,3) == m2.at(3,3)));
}



// -- fileScope --------------------------------------------------------------
// inline int operator!=(const Matrix3d &m1, const Matrix3d &m2)
// Logical inequality comparison between two Matrices.
//---------------------------------------------------------------------------
inline int operator!=(const Matrix3d &m1, const Matrix3d &m2)
{
  return ((m1.at(0,0) != m2.at(0,0)) || 
          (m1.at(0,1) != m2.at(0,1)) || 
          (m1.at(0,2) != m2.at(0,2)) || 
          (m1.at(0,3) != m2.at(0,3)) || 
          (m1.at(1,0) != m2.at(1,0)) || 
          (m1.at(1,1) != m2.at(1,1)) || 
          (m1.at(1,2) != m2.at(1,2)) || 
          (m1.at(1,3) != m2.at(1,3)) || 
          (m1.at(2,0) != m2.at(2,0)) || 
          (m1.at(2,1) != m2.at(2,1)) || 
          (m1.at(2,2) != m2.at(2,2)) || 
          (m1.at(2,3) != m2.at(2,3)) || 
          (m1.at(3,0) != m2.at(3,0)) || 
          (m1.at(3,1) != m2.at(3,1)) || 
          (m1.at(3,2) != m2.at(3,2)) || 
          (m1.at(3,3) != m2.at(3,3)));
}



// -- fileScope --------------------------------------------------------------
// inline Matrix3d operator*(const Matrix3d &m1, double scalar)
// Multiply a Matrix by a scalar.
//----------------------------------------------------------------------------
inline Matrix3d operator*(const Matrix3d &m1, double scalar)
{
  Matrix3d m(m1);

  m *= scalar;

  return m;
}



// -- fileScope --------------------------------------------------------------
// inline Matrix3d operator*(double scalar, const Matrix3d &m1)
// Multiply a Matrix by a scalar.
//----------------------------------------------------------------------------
inline Matrix3d operator*(double scalar, const Matrix3d &m1)
{
  Matrix3d m(m1);

  m *= scalar;

  return m;
}



// -- fileScope --------------------------------------------------------------
// inline Vect3d operator*(Matrix3d &m, Vect3d &v)
// Multiply a Matrix by a vector.
//----------------------------------------------------------------------------
inline Vect3d operator*(const Matrix3d &m, const Vect3d &v)
{
  return Vect3d 
    (m._data[0]*v(0) + m._data[4]*v(1) + m._data[8]* v(2) + m._data[12]*v(3),
     m._data[1]*v(0) + m._data[5]*v(1) + m._data[9]* v(2) + m._data[13]*v(3),
     m._data[2]*v(0) + m._data[6]*v(1) + m._data[10]*v(2) + m._data[14]*v(3),
     m._data[3]*v(0) + m._data[7]*v(1) + m._data[11]*v(2) + m._data[15]*v(3));
}

// -- fileScope --------------------------------------------------------------
// inline Point3d operator*(Matrix3d &m, Point3d &p)
// Multiply a Matrix by a point.
//----------------------------------------------------------------------------
inline Point3d operator*(const Matrix3d &m, const Point3d &p)
{
  return Point3d
    (m._data[0]*p(0) + m._data[4]*p(1) + m._data[8]* p(2) + m._data[12]*p(3),
     m._data[1]*p(0) + m._data[5]*p(1) + m._data[9]* p(2) + m._data[13]*p(3),
     m._data[2]*p(0) + m._data[6]*p(1) + m._data[10]*p(2) + m._data[14]*p(3),
     m._data[3]*p(0) + m._data[7]*p(1) + m._data[11]*p(2) + m._data[15]*p(3));
}



// -- fileScope --------------------------------------------------------------
// inline Matrix3d operator+(const Matrix3d &m1, const Matrix3d &m2)
// Add two Matrices.
//----------------------------------------------------------------------------
inline Matrix3d operator+(const Matrix3d &m1, const Matrix3d &m2)
{
  Matrix3d m(m1);

  m += m2;

  return m;
}



// -- fileScope --------------------------------------------------------------
// inline Matrix3d operator*(const Matrix3d &m1, const Matrix3d &m2)
// Multiply two Matrices.
//----------------------------------------------------------------------------
inline Matrix3d operator*(const Matrix3d &m1, const Matrix3d &m2)
{
  Matrix3d m(m1);

  m *= m2;

  return m;
}


// -- fileScope --------------------------------------------------------------
// inline double determinant3x3(double a1, double a2, double a3, 
//                              double b1, double b2, double b3, 
//                              double c1, double c2, double c3)
// Multiply two Matrices.
//----------------------------------------------------------------------------
inline double Matrix3d::determinant3x3(double a1, double a2, double a3, 
                                       double b1, double b2, double b3, 
                                       double c1, double c2, double c3)
{
  return (  a1 * (b2 * c3 - b3 * c2)
          - b1 * (a2 * c3 - a3 * c2)
          + c1 * (a2 * b3 - a3 * b2));
}

};

#endif


