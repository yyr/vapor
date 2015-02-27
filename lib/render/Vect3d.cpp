//-- Vect3d.cpp --------------------------------------------------------------
//
// Copyright (C) 2002-2005 Kenny Gruchalla. All rights reserved. 
// 
//
// A homogeneous 3-d vector with all the bells and wistles
//
//----------------------------------------------------------------------------

#include "Vect3d.h"

#include <iostream>
#include <stdlib.h>

//-- public  ------------------------------------------------------------------
// Vect3d::Vect3d()
// Default Constructor (NULL vector)
//-----------------------------------------------------------------------------
Vect3d::Vect3d() : _x(0.0), _y(0.0), _z(0.0), _w(0.0)
{
}

//-- public  ------------------------------------------------------------------
// Vect3d::Vect3d(double x1, double x2, double x3, double x4)
// Constructor
//-----------------------------------------------------------------------------
Vect3d::Vect3d(double x1, double x2, double x3, double x4) :
  _x(x1),
  _y(x2),
  _z(x3),
  _w(x4)
{
}

//-- public  ------------------------------------------------------------------
// Vect3d::Vect3d(const Point3D &p1, const Point3D &p2)
// Constructor creates p1p2 by p2 - p1
//-----------------------------------------------------------------------------
Vect3d::Vect3d(const Point3d &p1, const Point3d &p2) :
  _x(p2.x - p1.x),
  _y(p2.y - p1.y),
  _z(p2.z - p1.z),
  _w(p2.w - p1.w)
{
}

//-- public  ------------------------------------------------------------------
// Vect3d::Vect3d(const Point3D &p)
// Point3d Conversion Constructor
//-----------------------------------------------------------------------------
Vect3d::Vect3d(const Point3d &p) :
  _x(p.x),
  _y(p.y),
  _z(p.z),
  _w(0.0)
{
}


//-- public  ------------------------------------------------------------------
// Vect3d::Vect3d(const Vect3d &v)
// Copy constructor
//-----------------------------------------------------------------------------
Vect3d::Vect3d(const Vect3d &v) :
  _x(v._x),
  _y(v._y),
  _z(v._z),
  _w(v._w)
{
}

//-- public  ------------------------------------------------------------------
// Vect3d::~Vect3d()
// Destructor
//-----------------------------------------------------------------------------
Vect3d::~Vect3d()
{
}

//-- public  ------------------------------------------------------------------
// Vect3d &Vect3d::operator=(const Vect3d &v)
// Assignment
//-----------------------------------------------------------------------------
Vect3d &Vect3d::operator=(const Vect3d &v)
{
  if (this != &v) 
  {
    _x = v._x;
    _y = v._y;
    _z = v._z;
    _w = v._w;
  }

  return *this;
}

//-- public  ------------------------------------------------------------------
// Vect3d &Vect3d::operator+=(double scalar)
// Add a scalar to myself
//-----------------------------------------------------------------------------
Vect3d &Vect3d::operator+=(double scalar)
{
  _x += scalar;
  _y += scalar;
  _z += scalar;
  _w += scalar;

  return *this;
}


//-- public  ------------------------------------------------------------------
// Vect3d &Vect3d::operator-=(double scalar)
// Subtract a scalar from myself
//-----------------------------------------------------------------------------
Vect3d &Vect3d::operator-=(double scalar)
{
  _x -= scalar;
  _y -= scalar;
  _z -= scalar;
  _w -= scalar;

  return *this;
}

//-- public  ------------------------------------------------------------------
// Vect3d &Vect3d::operator*=(double scalar)
// Multiply myself by a scalar
//-----------------------------------------------------------------------------
Vect3d &Vect3d::operator*=(double scalar)
{
  _x *= scalar;
  _y *= scalar;
  _z *= scalar;
  _w *= scalar;

  return *this;
}


//-- public  ------------------------------------------------------------------
// Vect3d &Vect3d::operator/=(double scalar)
// Divide myself by a scalar
//-----------------------------------------------------------------------------
Vect3d &Vect3d::operator/=(double scalar)
{
  if (scalar != 0.0) 
  {
    _x /= scalar;
    _y /= scalar;
    _z /= scalar;
    _w /= scalar;
  }
  else 
  {
    _x = 0.0;
    _y = 0.0;
    _z = 0.0;
    _w = 0.0;
  }

  return *this;
}

//-- public  ------------------------------------------------------------------
// Vect3d &Vect3d::operator+=(const Vect3d &v)
// Add the myself to the vector.
//-----------------------------------------------------------------------------
Vect3d &Vect3d::operator+=(const Vect3d &v)
{
  _x += v._x;
  _y += v._y;
  _z += v._z;
  _w += v._w;

  return *this;
}


//-- public  ------------------------------------------------------------------
// Vect3d &Vect3d::operator-=(const Vect3d &v)
// Subtract myself from the vector.
//-----------------------------------------------------------------------------
Vect3d &Vect3d::operator-=(const Vect3d &v)
{
  _x -= v._x;
  _y -= v._y;
  _z -= v._z;
  _w -= v._w;

  return *this;
}


//-- public  ------------------------------------------------------------------
// Vect3d Vect3d::unit() const
// Return the unit vector.
//-----------------------------------------------------------------------------
Vect3d Vect3d::unit() const
{
  double x, y, z, w, m = mag();

  if (m <= 0.0) 
  {
    x = 0.0;
    y = 0.0;
    z = 0.0;
    w = 0.0;
  }
  else 
  {
    x = _x / m;
    y = _y / m;
    z = _z / m;
    w = _w / m;
  }

  return Vect3d(x, y, z, w);
}

//-- public  ------------------------------------------------------------------
// void Vect3d::unitize()
// Make myself a unit vector.
//-----------------------------------------------------------------------------
void Vect3d::unitize()
{
  double m = mag();

  if (m <= 0.0) 
  {
    _x = 0.0;
    _y = 0.0;
    _z = 0.0;
    _w = 0.0;
  }
  else 
  {
    _x /= m;
    _y /= m;
    _z /= m;
    _w /= m;
  }
}

//-- public  ------------------------------------------------------------------
// void Vect3d::clear()
//-----------------------------------------------------------------------------
void Vect3d::clear()
{
  _x = 0.0;
  _y = 0.0;
  _z = 0.0;
  _w = 0.0;
}

//-- public  ------------------------------------------------------------------
// void Vect3d::neg()
// Negate myself
//-----------------------------------------------------------------------------
void Vect3d::neg()
{
  _x = -_x;
  _y = -_y;
  _z = -_z;
  _w = -_w;
}

//-- public  ------------------------------------------------------------------
// double Vect3d::dot(const Vect3d &v) const
// Returns the dot product of this . v
//-----------------------------------------------------------------------------
double Vect3d::dot(const Vect3d &v) const
{
  return _x * v._x + _y * v._y + _z * v._z + _w * v._w;
}

//-- public  ------------------------------------------------------------------
// double Vect3d::cosAngle(const Vect3d &vec) const 
// Returns cos(theta) where theta is the angle between myself and the vector 
//-----------------------------------------------------------------------------
double Vect3d::cosAngle(const Vect3d &vec) const 
{
  return dot(vec) / mag() / vec.mag();
}

//-- public  ------------------------------------------------------------------
// Vect3d Vect3d::cross(const Vect3d &v) const
// Returns the cross product of this x v
//-----------------------------------------------------------------------------
Vect3d Vect3d::cross(const Vect3d &v) const
{
  return Vect3d(_y * v._z - _z * v._y,
                _z * v._x - _x * v._z,
                _x * v._y - _y * v._x);
}


//-- fileScope ----------------------------------------------------------------
// operator<<
//-----------------------------------------------------------------------------
std::ostream &operator<<(std::ostream &o, const Vect3d &v)
{
  o << v._x << " " << v._y << " " << v._z << " " << v._w << std::endl;

  return o;
}

//-- fileScope  ---------------------------------------------------------------
// Vect3d operator/(const Vect3d &v1, double scalar)
// Divide the Vector by a scalar
//-----------------------------------------------------------------------------
Vect3d operator/(const Vect3d &v1, double scalar)
{
  double x, y, z, w;

  if (scalar == 0.0) 
  {
    x = 0.0;
    y = 0.0;
    z = 0.0;
    w = 0.0;
  }
  else 
  {
    x = v1.x() / scalar;
    y = v1.y() / scalar;
    z = v1.z() / scalar;
    w = v1.w() / scalar;
  }
      
  return Vect3d(x, y, z, w);
}


