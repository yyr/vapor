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

#ifdef DARWIN
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <vector>
#include <vapor/common.h>

/* These vector and quaternion macros complement similar
 * routines.
 */


/* The Trackball package gives that nice 3D rotation interface.
 * A trackball class is needed for each rotated scene.
 */
namespace VAPoR {
class GLWindow;
class RENDER_API Trackball {
public:
	Trackball();
	void	TrackballSetMatrix ();
	void	TrackballFlip (int axis);
	void	TrackballSpin ();
	void	TrackballStopSpinning ();
	int		TrackballSpinning ();
	void	TrackballSetPosition ( double newx, double newy);
	void	TrackballRotate ( double newx, double newy);
	void	TrackballPan (double newx, double newy);
	void	TrackballZoom (double newx, double newy);
	void	TrackballCopyTo (Trackball *dst);
	void    TrackballSetTo(double scale, double rvec[3], double radians, double trans[3]);
	void	TrackballReset ();
	//Note:  button is 1,2,3 for left, middle, right
	void	MouseOnTrackball ( int eventType, int thisButton, int xcrd, int ycrd, unsigned width, unsigned height);
	bool	IsLocal() {return local;}
	//Initialize the trackball, provide viewer position, direction, upvector,
	//and the center of rotation (all in trackball coordinate space)
	void	setFromFrame(const std::vector<double>& posvec, const std::vector<double>& dirvec, const std::vector<double>& upvec, const std::vector<double>& centerRot,
				bool perspective);
	
	
private:
	void	setCenter(const std::vector<double>& newCenter){
		center[0]=newCenter[0];center[1]=newCenter[1];center[2]=newCenter[2];}
	// flag indicating whether this is a local or global tball
	bool local;
    double	qrot[4];
    double	qinc[4];
    double	trans[3];
    double	scale[3];
	double	center[3];
    double	ballsize;
    double	lastx, lasty;
	
	bool perspective;
} ;

};










#endif	// TRACKBALL_H
