//-- DomainWidget.cpp -------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// OpenGL-based widget used to scale and move the a transfer function domain. 
//
//----------------------------------------------------------------------------

#include "DomainWidget.h"
#include "params.h"

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "MappingFrame.h"
#include "glutil.h"

#include <iostream>

using namespace std;
using namespace VAPoR;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DomainWidget::DomainWidget(MappingFrame *parent, float min, float max) :
  GLWidget(parent),
  _parent(parent),
  _minValue(min),
  _maxValue(max),
  _handleRadius(0.06),
  _quadHandle(gluNewQuadric())
{
  _minY = 1.14;
  _maxY = 1.3;
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
DomainWidget::~DomainWidget()
{
}

//----------------------------------------------------------------------------
// Widget's width in the world coordinate frame
//----------------------------------------------------------------------------
float DomainWidget::width()
{
  return right() - left();
}

//----------------------------------------------------------------------------
// Move the entire domain
//----------------------------------------------------------------------------
void DomainWidget::move(float dx, float, float)
{
  if (_selected != NONE)
  {
    float scaling = (_parent->maxDataValue() - _parent->minDataValue());
    _minValue += dx*scaling;
    _maxValue += dx*scaling;

    emit changingDomain(_minValue, _maxValue);
  }
}

//----------------------------------------------------------------------------
// Drag an element of the domain.
//----------------------------------------------------------------------------
void DomainWidget::drag(float dx, float, float)
{
  float scaling = (_parent->maxDataValue() - _parent->minDataValue());

  if (_selected == LEFT && _minValue + dx*scaling < _maxValue)
  {
    _minValue += dx*scaling;
  }

  else if (_selected == RIGHT && _maxValue + dx*scaling > _minValue)
  {
    _maxValue += dx*scaling;
  }

  else if (_selected == BAR)
  {
    _minValue += dx*scaling;
    _maxValue += dx*scaling;
  }    
  
  emit changingDomain(_minValue, _maxValue);
}

//----------------------------------------------------------------------------
// Render the widget
//----------------------------------------------------------------------------
void DomainWidget::paintGL()
{

  float length = 0.03;
  float y = (_maxY + _minY) / 2.0;

  printOpenGLError();

  glPushName(_id);
  glPushMatrix();
  {
    if (_enabled)
    {
      glColor3f(1, 0.3, 0.3);

      glPushMatrix();
      {
        glPushName(LEFT);
#ifdef	Darwin
	glBegin(GL_TRIANGLES);
		glVertex3f(left()-length, y, 0.0);
		glVertex3f(left(), y+_handleRadius, 0.0);
		glVertex3f(left(), y-_handleRadius, 0.0);
	glEnd();
#else
	glTranslatef(left()-length, y, 0.0);
	glRotatef(90, 0, 1, 0);
	gluCylinder(_quadHandle, 0.0, _handleRadius, length, 10, 2);
#endif
        glPopName();
      }
      glPopMatrix();

      glPushMatrix();
      {
        glPushName(RIGHT);
#ifdef	Darwin
	glBegin(GL_TRIANGLES);
		glVertex3f(right()+length, y, 0.0);
		glVertex3f(right(), y+_handleRadius, 0.0);
		glVertex3f(right(), y-_handleRadius, 0.0);
	glEnd();
#else
	glTranslatef(right(), y, 0.0);
	glRotatef(90, 0, 1, 0);
	gluCylinder(_quadHandle, _handleRadius, 0.0, length, 10, 2);
#endif
        glPopName();
      }
      glPopMatrix();

      glColor3f(0.7, 0.0, 0.0);
      
      glPushMatrix();
      {
        glPushName(BAR);
#ifdef	Darwin
	glBegin(GL_QUADS);
		glVertex3f(left(), y + 0.4*_handleRadius, 0.0);
		glVertex3f(right(), y + 0.4*_handleRadius, 0.0);
		glVertex3f(right(), y - 0.4*_handleRadius, 0.0);
		glVertex3f(left(), y - 0.4*_handleRadius, 0.0);
	glEnd();
#else
	glTranslatef(left(), y, 0.0);
	glRotatef(90, 0, 1, 0);
	gluCylinder(_quadHandle, 0.4*_handleRadius, 0.4*_handleRadius, 
		width(), 10, 2);

#endif
        glPopName();
      }
      glPopMatrix();
    }
  }  
  glPopMatrix();
  glPopName();

  printOpenGLError();
}

//----------------------------------------------------------------------------
// Set the widget's geometry 
//----------------------------------------------------------------------------
void DomainWidget::setGeometry(float x0, float x1, float y0, float y1)
{
  GLWidget::setGeometry(x0, x1, y0, y1);

  _handleRadius = (_maxY - _minY) * 0.375;
}


//----------------------------------------------------------------------------
// Rightmost extent of the widget in world coodinates
//----------------------------------------------------------------------------
float DomainWidget::right()
{
  return dataToWorld(_maxValue);
}

//----------------------------------------------------------------------------
// Leftmost extent of the widget in world coordinates
//----------------------------------------------------------------------------
float DomainWidget::left()
{
  return dataToWorld(_minValue);
}

//----------------------------------------------------------------------------
// Transform the x position in the data (model) space into the opengl world 
// space 
//----------------------------------------------------------------------------
float DomainWidget::dataToWorld(float x)
{
  return (x - _parent->minDataValue()) / 
    (_parent->maxDataValue() - _parent->minDataValue());
}

//----------------------------------------------------------------------------
// Transform the x position in the opengl world space into the data (model) 
// space
//----------------------------------------------------------------------------
float DomainWidget::worldToData(float x)
{
  return _parent->minDataValue() + 
    (x * (_parent->maxDataValue() - _parent->minDataValue()));
}
