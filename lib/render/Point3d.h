//-- Point3d.h ---------------------------------------------------------------
//
// Copyright (C) 2002-2005 Kenny Gruchalla. All rights reserved. 
//
// A 3-dimensional point class
//
//-----------------------------------------------------------------------------

#ifndef Point3d_h
#define Point3d_h

#include <math.h>
#include <iostream>

struct RENDER_API Point3d
{
  double x;
  double y;
  double z;
  double w;

public:

  // Default Constructor
  Point3d(double x1 = 0.0, double x2 = 0.0, double x3 = 0.0, double x4 = 1.0) :
    x(x1), y(x2), z(x3), w(x4)
  {}

  // Copy Constructor
  Point3d(const Point3d &p) :
    x(p.x), y(p.y), z(p.z), w(p.w)
  {}

  // Destructor
  virtual ~Point3d()
  {}

  // Assignment.
  inline Point3d &operator=(const Point3d &p)
  {
    if (this != &p)
    {
      x = p.x;
      y = p.y;
      z = p.z;
      w = p.w;
    }
    
    return *this;
  }

  // Addition
  inline Point3d& operator+=(const Point3d &p)
  {
    x += p.x;
    y += p.y;
    z += p.z;

    return *this;
  }

  // Homogenize
  inline void homogenize()
  {
    if (w != 1 && w != 0)
    {
      x = x/w;
      y = y/w;
      z = z/w;
      w = w/w;
    }
  }

  // Index operator
  double &operator()(const int index);
  double operator()(const int index) const;

};

//-- public  ------------------------------------------------------------------
// inline double &Point3d::operator()(const int index)
// Access operator
//-----------------------------------------------------------------------------
inline double &Point3d::operator()(const int index)
{
  switch(index)
  {
    case 0 : return x;
    case 1 : return y;
    case 2 : return z;
  }

  return w;
}

//-- public  ------------------------------------------------------------------
// inline double Point3d::operator()(const int index) const
// Access operator
//-----------------------------------------------------------------------------
inline double Point3d::operator()(const int index) const
{
  switch(index)
  {
    case 0 : return x;
    case 1 : return y;
    case 2 : return z;
  }

  return w;
}

//-- fileScope ----------------------------------------------------------------
// inline bool operator==(const Point3d &q, const Point3d &p)
// Point equality operator
//----------------------------------------------------------------------------
inline bool operator==(const Point3d &q, const Point3d &p)
{
  return (q.x == p.x && q.y == p.y && q.z == p.z && q.w && p.w);
}

//-- fileScope ----------------------------------------------------------------
// inline ostream &operator<<(ostream &o, const Point3d &p)
// Point output operator
//----------------------------------------------------------------------------
inline std::ostream &operator<<(std::ostream &o, const Point3d &p)
{
  o << p.x << " " << p.y << " " << p.z << " " << p.w << std::endl;

  return o;
}


#endif
