//-- Vect3d.h -----------------------------------------------------------------
//
// Copyright (C) 2002-2005 Kenny Gruchalla. All rights reserved. 
// 
//
// A homogeneous 3-dimensional vector class with all the bells and wistles
//
//-----------------------------------------------------------------------------

#ifndef Vect3d_h
#define Vect3d_h

#include <math.h>
#include <iostream>
#include <vapor/common.h>
#include "Point3d.h"

class RENDER_API Vect3d
{
public:

  // Default Constructor - 0 vector.  All operations will "work" on vectors 
  // with a size of 0.
  Vect3d();

  // Specify the initial values for common vectors.
  Vect3d(double x1, double x2, double x3, double x4 = 0.0);

  // Constructor creating a vector from point p1 to p2
  Vect3d(const Point3d &p1, const Point3d &p2);

  // Constructor creating a vector from the origin to point p
  Vect3d(const Point3d &p);

  // Copy Constructor
  Vect3d(const Vect3d &vec);

  // Destructor
  virtual ~Vect3d();

  // Assignment.
  Vect3d &operator=(const Vect3d &vec);

  // Index operator
  double &operator()(const int index);
  double operator()(const int index) const;

  // Get my x value.  This behavior is not virtual as it is
  // inlined for performance.
  double x() const;

  // Get my y value.  This behavior is not virtual as it is
  // inlined for performance.
  double y() const;

  // Get my z value.  This behavior is not virtual as it is
  // inlined for performance.
  double z() const;

  // Get my w value.  This behavior is not virtual as it is
  // inlined for performance.
  double w() const;

  // Set my x value.  This behavior is not virtual as it is
  // inlined for performance.
  void x(double);

  // Set my y value.  This behavior is not virtual as it is
  // inlined for performance.
  void y(double);

  // Set my z value.  This behavior is not virtual as it is
  // inlined for performance.
  void z(double);

  // Set my z value.  This behavior is not virtual as it is
  // inlined for performance.
  void w(double);

  // Add the scalar to me.
  Vect3d &operator+=(double);

  // Subtract the scalar from me.
  Vect3d &operator-=(double);

  // Multiply my entries by the scalar.
  Vect3d &operator*=(double);

  // Divide my entries by the scalar.  If the scalar is 0.0, all of my entries
  // will become 0.0.
  Vect3d &operator/=(double);

  // Add the given vector to me.
  Vect3d &operator+=(const Vect3d &vec);

  // Subtract the given vector from me.
  Vect3d &operator-=(const Vect3d &vec);

  // Return the square root of the sum of my elements squared.
  double mag() const;

  // Return the sum of my elements squared.
  double magSq() const;

  // Return a unit vector in the same direction as me.
  virtual Vect3d unit() const;

  // Unitize me.
  virtual void unitize();

  // Clear all of my entries.
  virtual void clear();

  // Negate all of my entries.
  void neg();

  // Return the dot product between another Vect3d and me.
  double dot(const Vect3d &vec) const;

  // Get the cosine of the angle between me and the given vector.  I
  // assume that I and the other vector are not unit vectors.
  double cosAngle(const Vect3d &vect) const;

  // Returns the cross product between myself and the other vector.
  Vect3d cross(const Vect3d &vec) const;

  //
  // Friend functions
  //
  friend std::ostream &operator<<(std::ostream &o, const Vect3d &v);

  // Divide a vector by a scalar.
  friend Vect3d operator/(const Vect3d&, double);

  // Equivalence
  friend int operator==(const Vect3d &v1, const Vect3d &v2);
  friend int operator!=(const Vect3d &v1, const Vect3d &v2);

  // Add/Substract a scalar to each vector component
  friend Vect3d operator+(const Vect3d &v1, double scalar);
  friend Vect3d operator+(double scalar, const Vect3d &v1);
  friend Vect3d operator-(double scalar, const Vect3d &v);
  friend Vect3d operator-(const Vect3d &v, double scalar);

  // Multiply a vector by a scalar
  friend Vect3d operator*(double scalar, const Vect3d &v);
  friend Vect3d operator*(const Vect3d &v, double scalar);

  // Add/Substract two vectors
  friend Vect3d operator+(const Vect3d &v1, const Vect3d &v2);
  friend Vect3d operator-(const Vect3d &v1, const Vect3d &v2);

  // Add a vector to a point
  friend Point3d operator+(const Point3d &p, const Vect3d &v);
  
  // Substract two points
  friend Vect3d operator-(const Point3d &p1, const Point3d &p2);

protected:

  // Member data.
  double _x;
  double _y;
  double _z;
  double _w;
};

// Inline functions.

//-- public  ------------------------------------------------------------------
// inline double &Vect3d::operator()(const int index)
// Access operator
//-----------------------------------------------------------------------------
inline double &Vect3d::operator()(const int index)
{
  switch(index)
  {
    case 0 : return _x;
    case 1 : return _y;
    case 2 : return _z;
    case 3 : return _w;
  }

  return _w;
}

//-- public  ------------------------------------------------------------------
// inline double Vect3d::operator()(const int index) const
// Access operator
//-----------------------------------------------------------------------------
inline double Vect3d::operator()(const int index) const
{
  switch(index)
  {
    case 0 : return _x;
    case 1 : return _y;
    case 2 : return _z;
    case 3 : return _w;
  }

  return _w;
}

//-- public  ------------------------------------------------------------------
// inline double Vect3d::x() const
//-----------------------------------------------------------------------------
inline double Vect3d::x() const
{
  return _x;
}

//-- public  ------------------------------------------------------------------
// inline double Vect3d::y() const
//-----------------------------------------------------------------------------
inline double Vect3d::y() const
{
  return _y;
}

//-- public  ------------------------------------------------------------------
// inline double Vect3d::z() const
//-----------------------------------------------------------------------------
inline double Vect3d::z() const
{
  return _z;
}

//-- public  ------------------------------------------------------------------
// inline double Vect3d::w() const
//-----------------------------------------------------------------------------
inline double Vect3d::w() const
{
  return _w;
}

//-- public  ------------------------------------------------------------------
// inline void Vect3d::x(double v)
//-----------------------------------------------------------------------------
inline void Vect3d::x(double v)
{
  _x = v;
}

//-- public  ------------------------------------------------------------------
// inline void Vect3d::y(double v)
//-----------------------------------------------------------------------------
inline void Vect3d::y(double v)
{
  _y = v;
}

//-- public  ------------------------------------------------------------------
// inline void Vect3d::z(double v)
//-----------------------------------------------------------------------------
inline void Vect3d::z(double v)
{
  _z = v;
}

//-- public  ------------------------------------------------------------------
// inline void Vect3d::w(double v)
//-----------------------------------------------------------------------------
inline void Vect3d::w(double v)
{
  _w = v;
}

//-- public  ------------------------------------------------------------------
// inline int operator==(const Vect3d &v1, const Vect3d &v2)
// Logical equality comparison between two vectors.
//-----------------------------------------------------------------------------
inline int operator==(const Vect3d &v1, const Vect3d &v2)
{
  return ((v1._x == v2._x) && (v1._y == v2._y) && 
          (v1._z == v2._z) && (v1._w == v2._w));
}

//-- public  ------------------------------------------------------------------
// inline int operator!=(const Vect3d &v1, const Vect3d &v2)
// Logical inequality comparison between two vectors.
//-----------------------------------------------------------------------------
inline int operator!=(const Vect3d &v1, const Vect3d &v2)
{
  return ((v1._x != v2._x) || (v1._y != v2._y) ||
          (v1._z != v2._z) || (v1._w != v2._w));
}

//-- public  ------------------------------------------------------------------
// inline double Vect3d::magSq() const

//-----------------------------------------------------------------------------
inline double Vect3d::magSq() const
{
  return _x * _x + _y * _y + _z * _z + _w * _w;
}

//-- public  ------------------------------------------------------------------
// inline double Vect3d::mag() const
 
//-----------------------------------------------------------------------------
inline double Vect3d::mag() const
{
  return sqrt(magSq());
}


//-- public  ------------------------------------------------------------------
// inline Vect3d operator+(const Vect3d &v1, double scalar)
// Add a vector and a scalar.
//-----------------------------------------------------------------------------
inline Vect3d operator+(const Vect3d &v1, double scalar)

{
  return Vect3d(v1._x + scalar, v1._y + scalar, 
                v1._z + scalar, v1._w + scalar);
}



//-- public  ------------------------------------------------------------------
// inline Vect3d operator+(double scalar, const Vect3d &v1)
// Add a vector and a scalar.
//-----------------------------------------------------------------------------
inline Vect3d operator+(double scalar, const Vect3d &v1)
{
  return Vect3d(v1._x + scalar, v1._y + scalar, 
                v1._z + scalar, v1._w + scalar);
}



//-- public  ------------------------------------------------------------------
// inline Vect3d operator-(double scalar, const Vect3d &v)
// Subtract a vector and a scalar.
//-----------------------------------------------------------------------------
inline Vect3d operator-(double scalar, const Vect3d &v)
{
  return Vect3d(scalar - v._x, scalar - v._y,
                scalar - v._z, scalar - v._w);
}



//-- public  ------------------------------------------------------------------
// inline Vect3d operator-(const Vect3d &v, double scalar)
// Subtract a vector and a scalar.
//-----------------------------------------------------------------------------
inline Vect3d operator-(const Vect3d &v, double scalar)
{
  return Vect3d(v._x - scalar, v._y - scalar,
                v._z - scalar, v._w - scalar);
}



//-- public  ------------------------------------------------------------------
// inline Vect3d operator*(double scalar, const Vect3d &v)
// Multiply a vector and a scalar.
//-----------------------------------------------------------------------------
inline Vect3d operator*(double scalar, const Vect3d &v)
{
  return Vect3d(v._x * scalar, v._y * scalar,
                v._z * scalar, v._w * scalar);
}



//-- public  ------------------------------------------------------------------
// inline Vect3d operator*(const Vect3d &v, double scalar)
// Multiply a vector and a scalar.
//-----------------------------------------------------------------------------
inline Vect3d operator*(const Vect3d &v, double scalar)
{
  return Vect3d(v._x * scalar, v._y * scalar,
                v._z * scalar, v._w * scalar);
}


//-- public  ------------------------------------------------------------------
// inline Point3d operator+(const Point3d &p, const Vect3d &v)
// Add a vector to a point
//-----------------------------------------------------------------------------
inline Point3d operator+(const Point3d &p, const Vect3d &v)
{
  return Point3d(p.x + v._x, p.y + v._y,
                 p.z + v._z, p.w + v._w);
}

//-- public  ------------------------------------------------------------------
// inline Vect3d operator+(const Vect3d &v1, const Vect3d &v2)
// Add two vectors
//-----------------------------------------------------------------------------
inline Vect3d operator+(const Vect3d &v1, const Vect3d &v2)
{
  return Vect3d(v1._x + v2._x, v1._y + v2._y,
                v1._z + v2._z, v1._w + v2._w);
}


//-- public  ------------------------------------------------------------------
// inline Vect3d operator-(const Vect3d &v1, const Vect3d &v2)
// Subtract to vectors
//-----------------------------------------------------------------------------
inline Vect3d operator-(const Vect3d &v1, const Vect3d &v2)
{
  return Vect3d(v1._x - v2._x, v1._y - v2._y,
                v1._z - v2._z, v1._w - v2._w);
}

//-- public  ------------------------------------------------------------------
// inline Vect3d operator-(const Point3d &v1, const Point3d &v2)
// Subtract to points (makes a vector)
//-----------------------------------------------------------------------------
inline Vect3d operator-(const Point3d &p1, const Point3d &p2)
{
  return Vect3d(p1.x - p2.x, p1.y - p2.y,
                p1.z - p2.z, p1.w - p2.w);
}

#endif
