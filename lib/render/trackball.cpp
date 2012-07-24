//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		trackball.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:	Implements the Trackball class:   
//		This was implemented from Ken Purcell's trackball
//		methods.  Additional methods provided to set the trackball
//		based on a viewing frame
//
/* Copyright (C) 1992  AHPCRC, Univeristy of Minnesota
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named 'Copying'; if not, write to
 * the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139.
 */

/* Author:
 *	Ken Chin-Purcell (ken@ahpcrc.umn.edu)
 *	Army High Performance Computing Research Center (AHPCRC)
 *	Univeristy of Minnesota
 *
 * This class was extracted from glutil.c  See glutil.cpp for the revision history */


/* Trackball interface for 3D rotation:
 *
 * The trackball is simualted as a globe rotating about its center.
 * Mouse movement is mapped to a spot on the globe, which when moved
 * effects a rotation.
 *
 * In addition, this Trackball also keeps track of translation,
 * in the form of panning and zooming.
 *
 * To use:
 *	At the start of the program, call Trackball constructor to
 *		create a Trackball object.  You will want a separate
 *		Trackball for each rotatable scene.
 *	On a Button or Motion event, call
 *		MouseOnTrackball
 *	When you want to draw, call
 *		TrackballSetMatrix to modify the gl matrix.
 *	Misc. functions:
 *		TrackballReset to come to home position
 *		TrackballFlip to flip about an axis
 *		TrackballSpin to keep rotating by last increment
 *		TrackballStopSpinning to set increment to zero
 *
 * To change how trackball feels:
 *	tball->scale[0]	: modify translation (pan) in x direction
 *	tball->scale[1]	: modify translation (pan) in y direction
 *	tball->scale[2]	: modify translation (zoom) in z direction
 *	tball->ballsize : a bigger ball gives finer rotations
 *			  (default = 0.65)
 */


#include <qgl.h>
#include "glutil.h"
#include "trackball.h" 
#include <math.h>
using namespace VAPoR;
void 
Trackball::TrackballReset()
{
    /* Bring trackball to home position, no translation, zero rotation.
     */
    qzero(qrot);
    qzero(qinc);
    vzero(trans);
	//Default center of rotation:
	center[0] = 0.5f;
	center[1] = 0.5f;
	center[2] = 0.5f;
}


Trackball::Trackball(void)
{
    ballsize = 0.65f;
    vset(scale, 1, 1, 1);
    TrackballReset();
}

void	Trackball::TrackballSetTo(
	float		scale,
	float		rvec[3],
	float		radians,
	float		trans[3]
) {
	if (scale < 1.0) {
		trans[2] = 1 - (1.0 / scale);
	}
	else {
		trans[2] = scale - 1.0; 
	}

	rvec2q(rvec, radians, qrot);

	trans[0] = trans[0];
	trans[1] = trans[1];
	qzero(qinc);
}
	


void Trackball::TrackballSetMatrix()
//Note perspective must be set previously in setFromFrame.
{
    /* Modify the current gl matrix by the trackball 
     * rotation and translation.
     */
    GLfloat 	m[16];
	/*Start with the "home" matrix:
	Currently assuming identity??
	*/
	//sendGLHomeViewpoint();

    if (perspective) {
		glTranslatef(center[0], center[1], center[2]);
	    glTranslatef(trans[0],  trans[1],  trans[2]);
		//qWarning("translate %f %f %f", trans[0], trans[1], trans[2]);
	    qmatrix(qrot, m);
	    glMultMatrixf(m);
		glTranslatef(-center[0], -center[1], -center[2]);
		//float* matrix = (float*)m;
		/*
		qWarning( "trackball perspective Matrix is: \n %f %f %f %f \n %f %f %f %f \n %f %f %f %f \n %f %f %f %f ",
			matrix[0], matrix[1],matrix[2],matrix[3],
			matrix[4], matrix[5],matrix[6],matrix[7],
			matrix[8], matrix[9],matrix[10],matrix[11],
			matrix[12], matrix[13],matrix[14],matrix[15]);*/
    }
    else {
	    GLfloat scale_factor = 1.0;

	    if (trans[2] < 0.0) {
			scale_factor = 5.0 / (1 - trans[2]);
	    }
	    else {
			scale_factor = 5.0 + trans[2];
	    }

	    glTranslatef(trans[0],  trans[1],  trans[2]);
	    qmatrix(qrot, m);
	    glMultMatrixf(m);
		
	    glScalef(scale_factor, scale_factor, scale_factor);
		//qWarning("translate, scale %f %f %f %f", trans[0], trans[1], trans[2], scale_factor);
		//float* matrix = (float*)m;
		/*qWarning( "trackball parallel Matrix is: \n %f %f %f %f \n %f %f %f %f \n %f %f %f %f \n %f %f %f %f ",
			matrix[0], matrix[1],matrix[2],matrix[3],
			matrix[4], matrix[5],matrix[6],matrix[7],
			matrix[8], matrix[9],matrix[10],matrix[11],
			matrix[12], matrix[13],matrix[14],matrix[15]);*/
    }
}

#ifndef	M_SQRT1_2
#define M_SQRT1_2  0.707106781186547524401f
#endif
static float q90[3][4] = {
    { M_SQRT1_2, 0.0, 0.0, M_SQRT1_2 },
    { 0.0, M_SQRT1_2, 0.0, M_SQRT1_2 },
    { 0.0, 0.0,-M_SQRT1_2, M_SQRT1_2 },
};

void Trackball::TrackballFlip( int axis)
{
    /* Rotate by 90 deg about the given axis.
     */
    if (axis >= 0  &&  axis < 3)
	qmult(q90[axis], qrot, qrot);
}


void Trackball::TrackballSpin()
{
    /* Rotationaly spin the trackball by the current increment.
     * Use this to implement rotational glide.
     */
    qmult(qinc, qrot, qrot);
}


void Trackball::TrackballStopSpinning()
{
    /* Cease any rotational glide by zeroing the increment.
     */
    qzero(qinc);
}


int Trackball::TrackballSpinning()
{
    /* If the trackball is gliding then the increment's angle
     * will be non-zero, and cos(theta) != 1, hence q[3] != 1.
     */
    return (qinc[3] != 1);
}


void Trackball::TrackballSetPosition(float newx, float newy)
{
    /* Call this when the user does a mouse down.  
     * Stop the trackball glide, then remember the mouse
     * down point (for a future rotate, pan or zoom).
     */
    TrackballStopSpinning();
    lastx = newx;
    lasty = newy;
}


void Trackball::TrackballRotate(float newx, float newy)
{
	/* OGLXXX glBegin: Use GL_LINES if only one line segment is desired. */
    /* Call this when the mouse glBegin(GL_LINE_STRIP); glVertex3s(i.e. PointerMotion, $2, $3).
     * Using the current coordinates and the last coordinates
     * (initialized with TrackballSetPosition), calc the 
     * glide increment, then spin the trackball once.
     */
    CalcRotation(qinc, newx, newy, lastx, lasty, 
		 ballsize);
    TrackballSpin();

    lastx = newx;	/* remember for next time */
    lasty = newy;
}


void Trackball::TrackballPan( float newx, float newy)
{
	/* OGLXXX glBegin: Use GL_LINES if only one line segment is desired. */
    /* Call this when the mouse glBegin(GL_LINE_STRIP); glVertex3s(i.e. PointerMotion, $2, $3).
     */
    trans[0] += (newx - lastx) * scale[0];
    trans[1] += (newy - lasty) * scale[1];

    lastx = newx;	/* remember for next time */
    lasty = newy;
}


void Trackball::TrackballZoom(float newx, float newy)
{
	/* OGLXXX glBegin: Use GL_LINES if only one line segment is desired. */
    /* Call this when the mouse glBegin(GL_LINE_STRIP); glVertex3s(i.e. PointerMotion, $2, $3).
     */
    trans[2] += (newy - lasty) * scale[2];

    lastx = newx;	/* remember for next time */
    lasty = newy;
}


void Trackball::TrackballCopyTo(Trackball *dst)
{
    /* Copy the current roation of the trackball
     */
    qcopy(qrot, dst->qrot);
}

/*
 * eventNum is 0, 1, or 2 for press, move, or release
 * thisButton is Qt:LeftButton, RightButton, or MidButton
 */

void Trackball::MouseOnTrackball(int eventNum, Qt::MouseButton thisButton, int xcrd, int ycrd, unsigned width, unsigned height)
{
    /* Alter a Trackball structure given  mouse event.
     *   This routine *assumes* button 1 rotates, button 2 pans,
     *   and button 3 zooms!
     *
     * event		: ButtonPress, MotionNotify or ButtonRelease
     * width, height	: of event window
     * tball		: trackball to modify
     */
    float	x, y;
	static Qt::MouseButton	button;
	//Ignore time: Qt doesn't provide time with events (may need to revisit this later????)
    //static Time	downTime;

    
    switch (eventNum) {

	  case 0: //ButtonPress:
		x = ScalePoint(xcrd, 0, width);
		y = ScalePoint(ycrd, height, -((long) height));
		TrackballSetPosition(x, y);
		//Remember the last button pressed:
		button = thisButton;
		//downTime = event->xbutton.time;
		break;
	
	  case 1://MotionNotify:
		x = ScalePoint(xcrd, 0, width);
		y = ScalePoint(ycrd, height, -((long) height));
		switch (button) {
			case Qt::LeftButton :
				TrackballRotate( x, y);
				break;
			case Qt::MidButton :
				TrackballPan( x, y);
				break;
			case Qt::RightButton :
				TrackballZoom( x, y);
				break;
			default:
				break;
		}
		//downTime = event->xmotion.time;
		break;
	
	  case 2: //ButtonRelease:
		button = Qt::LeftButton;
	//	if (event->xbutton.time - downTime > 250)
		//??? if (event->xbutton.time - downTime > 100)
		//???	TrackballStopSpinning(tball);
		break;
    }
}
// Set the quaternion and translation from a viewer frame
// Also happens to construct modelview matrix, but we don't use its translation
void Trackball::
setFromFrame(float* posvec, float* dirvec, float* upvec, float* centerRot, bool persp){
	//First construct the rotation matrix:
	double mtrx1[16];
	double trnsMtrx[16];
	double mtrx[16];
	setCenter(centerRot);
	makeTransMatrix(centerRot, trnsMtrx);
	makeModelviewMatrixD(posvec, dirvec, upvec, mtrx);
	
	//Translate on both sides by translation
	//first on left,
	mmult(trnsMtrx, mtrx, mtrx1);
	//then on right by negative:
	trnsMtrx[12] = -trnsMtrx[12];
	trnsMtrx[13] = -trnsMtrx[13];
	trnsMtrx[14] = -trnsMtrx[14];
	mmult(mtrx1,trnsMtrx, mtrx);
	double qrotd[4];
	//convert rotation part to quaternion:
	rotmatrix2q(mtrx, qrotd);
	for (int i = 0; i<4; i++) qrot[i] = (float)qrotd[i];
	//set the translation?
	//If parallel (ortho) transform, z used for translation
	perspective = persp;
	//vcopy(posvec, trans);
	for (int i = 0; i<3; i++) trans[i] = (float)mtrx[i+12];
	
}



