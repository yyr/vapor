//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		trackball.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:	Defines the Trackball class:   
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
 */

#ifndef	TRACKBALL_H
#define	TRACKBALL_H


//#include <GL/gl.h>
#include <qgl.h>

/* These vector and quaternion macros complement similar
 * routines.
 */


/* The Trackball package gives that nice 3D rotation interface.
 * A trackball class is needed for each rotated scene.
 */
namespace VAPoR {
class Trackball {
public:
	Trackball();
	void	TrackballSetMatrix ();
	void	TrackballFlip (int axis);
	void	TrackballSpin ();
	void	TrackballStopSpinning ();
	int		TrackballSpinning ();
	void	TrackballSetPosition ( float newx, float newy);
	void	TrackballRotate ( float newx, float newy);
	void	TrackballPan (float newx, float newy);
	void	TrackballZoom (float newx, float newy);
	void	TrackballCopyTo (Trackball *dst);
	void    TrackballSetTo(float scale, float rvec[3], float radians, float trans[3]);
	void	TrackballReset ();
	void	MouseOnTrackball ( int eventType, Qt::ButtonState thisButton, int xcrd, int ycrd, unsigned width, unsigned height);
	bool	isLocal() {return local;}
	void	setFromFrame(float* posvec, float* dirvec, float* upvec, bool perspective);
	
private:
	// flag indicating whether this is a local or global tball
	bool local;
    float	qrot[4];
    float	qinc[4];
    float	trans[3];
    float	scale[3];
    float	ballsize;
    float	lastx, lasty;
	bool perspective;
} ;
};










#endif	// TRACKBALL_H
