//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		glutil.h
//
//	Adaptor:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:  Methods to facilitate use of trackball navigation,
//		adapted from Ken Purcell's code to work in QT window
//
// Copyright (C) 1992  AHPCRC, Univeristy of Minnesota
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program in a file named 'Copying'; if not, write to
// the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139.
//

// Original Author:
//	Ken Chin-Purcell (ken@ahpcrc.umn.edu)
//	Army High Performance Computing Research Center (AHPCRC)
//	Univeristy of Minnesota
//

#ifndef	_glutil_h_
#define	_glutil_h_


//#include <GL/gl.h>
#include <qgl.h>

/* These vector and quaternion macros complement similar
 * routines.
 */


#ifdef	ArchLinux
#define	sqrtf(fval)	((float)sqrt((double)(fval)))
#define fabsf(fval)	((float)fabs((double)(fval)))
#define sinf(fval)	((float)sin((double)(fval)))
#define cosf(fval)	((float)cos((double)(fval)))
#define tanf(fval)	((float)tan((double)(fval)))
#endif

#define vset(a,x,y,z)	(a[0] = x, a[1] = y, a[2] = z)
#define Verify(expr,estr)	if (!(expr)) BailOut(estr,__FILE__,__LINE__)
#define MemCheck(ptr)	if (!(ptr)) BailOut("Out of memory",__FILE__,__LINE__)
#define CallocType(type,i)	(type *) calloc(i,sizeof(type))

inline void vcopy(const float* a, float* b) {b[0] = a[0], b[1] = a[1], b[2] = a[2];}
#define vzero(a)	(a[0] = a[1] = a[2] = 0)
#define vadd(a,b,c)	(c[0]=a[0]+b[0], c[1]=a[1]+b[1], c[2]=a[2]+b[2])
#define vsub(a,b,c)	(c[0]=a[0]-b[0], c[1]=a[1]-b[1], c[2]=a[2]-b[2])
#define vdot(a,b)	(a[0]*b[0] + a[1]*b[1] + a[2]*b[2])
#define vlength(a)	sqrt((double) vdot(a,a))
#define vnormal(a)	vscale(a, 1/vlength(a))

#define qset(a,x,y,z,w)	(a[0] = x, a[1] = y, a[2] = z, a[3] = w)
#define qcopy(a,b)	(b[0] = a[0], b[1] = a[1], b[2] = a[2], b[3] = a[3])
#define qzero(a)	(a[0] = a[1] = a[2] = 0, a[3] = 1)
#define qadd(a,b,c)	(vadd(a,b,c), c[3]=a[3]+b[3])

#define	YMAXSTEREO	491
#define	YOFFSET		532

/*extern GLfloat *idmatrix;*/
namespace VAPoR {
void	BailOut (char *errstr, char *fname, int lineno);
/* glutil.c */
void	computeGradientData(
	int dim[3], int numChan, unsigned char *volume, unsigned char *gradient
);
void	makeModelviewMatrix(float* vpos, float* vdir, float* upvec, float* matrix);
void	makeTransMatrix(float* transVec, float* matrix);
void	vscale (float *v, float s);
void	vmult( float *v, float s, float *w); 
void	vhalf (const float *v1, const float *v2, float *half);
void	vcross (const float *v1, const float *v2, float *cross);
void	vreflect (const float *in, const float *mirror, float *out);
void	vtransform (const float *v, GLfloat *mat, float *vt);
void	vtransform4 (const float *v, GLfloat *mat, float *vt);
bool	pointOnRight(float* pt1, float* pt2, float* testPt);
void	mcopy (GLfloat *m1, GLfloat *m2);
void	mmult (GLfloat *m1, GLfloat *m2, GLfloat *prod);
void	minvert (GLfloat *mat, GLfloat *result);
void	LinSolve (const float *eqs [], int n, float *x);
void	qnormal (float *q);
void	qmult (const float *q1, const float *q2, float *dest);
void	qmatrix (const float *q, GLfloat *m);
float	ProjectToSphere (float r, float x, float y);
void	CalcRotation (float *q, float newX, float newY, float oldX, float oldY, float ballsize);
float	ScalePoint (long pt, long origin, long size);
void	rvec2q(const float	rvec[3],float		radians,float		q[4]);
void	rotmatrix2q(float* m, float *q );
float   getScale(GLfloat* rotmatrix);

//Forward declarations for utility functions.
//These should really go in glutil!

int	matrix4x4_inverse(
	const GLfloat	*in,
	GLfloat *out
);
void	matrix4x4_vec3_mult(
	const GLfloat	m[16],
	const GLfloat a[4],
	GLfloat b[4]
);
void	adjoint(
	const GLfloat	*in,
	GLfloat *out
);
double det4x4(
	const GLfloat	m[16]
);
double det2x2(double a, double b, double c, double d);
double det3x3(
	double a1, double a2, double a3,
	double b1, double b2, double b3,
	double c1, double c2, double c3
);

void	ViewMatrix (GLfloat *m);
int	ViewAxis (int *direction);
void	StereoPerspective (int fovy, float aspect, float nearDist, float farDist, float converge, float eye);

};
#endif	// _glutil_h_
