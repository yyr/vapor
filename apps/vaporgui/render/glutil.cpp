//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		glutil.cpp
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <iostream>


//#include <GL/gl.h>
#include <qgl.h>

//#include "util.h"
#include "glutil.h"

using namespace VAPoR;
/* The Identity matrix is useful for intializing transformations.
 */
GLfloat idmatrix[16] = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0,
};


/* Most of the 'v' routines are in the form vsomething(src1, src2, dst),
 * dst can be one of the source vectors.
 */
namespace VAPoR {
void vscale(float *v, float s)
{
    /* Scale the vector v in all directions by s.
     */
    v[0] *= s;
    v[1] *= s;
    v[2] *= s;
}
// Scale, putting result in another vector
//
void vmult(const float *v, float s, float *w) {
	w[0] = s*v[0]; 
	w[1] = s*v[1]; 
	w[2] = s*v[2];
}

void vhalf(const float *v1, const float *v2, float *half)
{
    /* Return in 'half' the unit vector 
     * half way between v1 and v2.  half can be v1 or v2.
     */
    float	len;
    
    vadd(v2, v1, half);
    len = vlength(half);
    if(len > 0)
	vscale(half, 1/len);
    else
	vcopy(v1, half);
}


void vcross(const float *v1, const float *v2, float *cross)
{
    /* Vector cross product.
     */
    float	temp[3];
    
    temp[0] = (v1[1] * v2[2]) - (v1[2] * v2[1]);
    temp[1] = (v1[2] * v2[0]) - (v1[0] * v2[2]);
    temp[2] = (v1[0] * v2[1]) - (v1[1] * v2[0]);
    vcopy(temp, cross);
}


void vreflect(const float *in, const float *mirror, float *out)
{
    /* Mirror defines a plane across which in is reflected.
     */
    float	temp[3];
    
    vcopy(mirror, temp);
    vscale(temp,vdot(mirror,in));
    vsub(temp,in,out);
    vadd(temp,out,out);
}


void vtransform(const float *v, GLfloat mat[12], float *vt)
{
    /* Vector transform in software...
     */
    float	t[3];
    
    t[0] = v[0]*mat[0] + v[1]*mat[1] + v[2]*mat[2] + mat[3];
    t[1] = v[0]*mat[4] + v[1]*mat[5] + v[2]*mat[6] + mat[7];
    t[2] = v[0]*mat[8] + v[1]*mat[9] + v[2]*mat[10] + mat[11];
    vcopy(t, vt);
}


void vtransform4(const float *v, GLfloat *mat, float *vt)
{
    /* Homogeneous coordinates.
     */
    float	t[4];
    
    t[0] = v[0]*mat[0] + v[1]*mat[1] + v[2]*mat[2] + mat[3];
    t[1] = v[0]*mat[4] + v[1]*mat[5] + v[2]*mat[6] + mat[7];
    t[2] = v[0]*mat[8] + v[1]*mat[9] + v[2]*mat[10] + mat[11];
    t[3] = v[0]*mat[12] + v[1]*mat[13] + v[2]*mat[14] + mat[15];
    qcopy(t, vt);
}
//Test whether a planar point is right (or left) of the oriented line from
// pt1 to pt2
bool pointOnRight(float* pt1, float* pt2, float* testPt){
	float rhs = pt1[0]*(pt1[1]-pt2[1]) + pt1[1]*(pt2[0]-pt1[0]);
	float test = (pt2[0]-pt1[0])*testPt[1] + (pt1[1]-pt2[1])*testPt[0] - rhs;
	return (test < 0.f);
}

void mcopy(GLfloat *m1, GLfloat *m2)
{
    /* Copy a 4x4 matrix
     */
    int		row;

    for(row = 0 ; row < 16 ; row++)
	m2[row] = m1[row];
}


void mmult(GLfloat *m1, GLfloat *m2, GLfloat *prod)
{
    /* Multiply two 4x4 matricies
     */
    int		row, col;
    GLfloat 	temp[16];
    
    /*for(row = 0 ; row < 4 ; row++) 
       for(col = 0 ; col < 4 ; col++)
	    temp[row + col*4] = (m1[row]   * m2[col*4] + 
				m1[row+4]  * m2[1+col*4] +
				m1[row+8]  * m2[2+col*4] +
				m1[row+12] * m2[3+col*4]);*/
/*
 * Use OpenGL style matrix mult -- Wes.
 */
    for(row = 0 ; row < 4 ; row++) 
       for(col = 0 ; col < 4 ; col++)
	    temp[row*4 + col] = (m1[row*4]  * m2[col] + 
				m1[1+row*4] * m2[col+4] +
				m1[2+row*4] * m2[col+8] +
				m1[3+row*4] * m2[col+12]);
    mcopy(temp, prod);
}


void minvert(GLfloat *mat, GLfloat *result)
{
    /* Invert a 4x4 matrix
     */
    int         i, j, k;
    float       temp;
    float       m[8][4];
   
    mcopy(idmatrix, result);
    for (i = 0;  i < 4;  i++) {
        for (j = 0;  j < 4;  j++) {
            m[i][j] = mat[i+4*j];
            m[i+4][j] = result[i+4*j];
        }
    }
   
    /* Work across by columns
     */
    for (i = 0; i < 4; i++) {
        j = i;
        while (m[i][j] == 0  &&  j < 4)
            j++;
        Verify(j < 4, "cannot invert matrix");

        if (i != j)
            for (k = 0;  k < 8;  k++) {
                temp = m[k][i];
                m[k][i] = m[k][j];
                m[k][j] = temp;
            }

        /* Divide original row
         */
        for (j = 7;  j >= i;  j--)
            m[j][i] /= m[i][i];

        /* Subtract other rows
         */
        for (j = 0;  j < 4;  j++)
            if (i != j)
                for (k = 7;  k >= i;  k--)
                    m[k][j] -= m[k][i] * m[i][j];
    }

    for (i = 0;  i < 4;  i++)
        for (j = 0;  j < 4;  j++)
            result[i+4*j] = m[i+4][j];
}



void LinSolve(const float 	*eqs[],	/* System of eq's to solve */
	      int		n, 	/* of size inmat[n][n+1] */
	      float		*x)	/* Result float of size x[n] */
{
    /* Gaussian Elimination with Scaled Column Pivoting
     * Copied by Wade Olsen, Silicon Graphics, Feb. 12, 1990
     */
    int		i, j, k, p;
    float 	**a;
    
    /* Allocate space to work in.
     * Avoid modifying the equations passed.
     */
    a = CallocType(float *, n);
    MemCheck(a);
    for (i = 0; i < n; i++) {
	a[i] = CallocType(float, n+1);	
	MemCheck(a[i]);
	(void) memcpy(a[i], eqs[i], sizeof(float)*(n+1));
    }
    
    /* The simple single variable case
     */
    if (n == 1)	{
	x[0] = a[0][1] / a[0][0];
	return;
    }
    
    /* Gausian elimination process
     */	
    for (i = 0; i < n - 1; i++)	{
	
	/* find non-zero pivot row
	 */
	p = i;
	while (a[p][i] == 0) {
	    p++;
	    Verify(p != n, "linsolv:  No unique solution exists.");
	}
	
	/* row swap
	 */
	if (p != i) {
	    float *swap = a[i];
	    a[i] = a[p];
	    a[p] = swap;
	}
	
	/* row subtractions
	 */
	for (j = i + 1; j < n; j++) {
	    float	mult = a[j][i] / a[i][i];
	    for (k = i + 1; k < n + 1; k++)
		a[j][k] -= mult * a[i][k];
	}
    }
    
    Verify(a[n-1][n-1] != 0, "linsolv:  No unique solution exists.");
    
    /* backwards substitution
     */
    x[n-1] = a[n-1][n] / a[n-1][n-1];
    for (i = n - 2; i >= 0; i--) {
	float sum = a[i][n];
	for (j = i + 1; j < n; j++)
	    sum -= a[i][j] * x[j];
	x[i] = sum / a[i][i];
    }
    
    /* Free working space 
     */
    for (i = 0; i < n; i++)
	free(a[i]);
    free(a);
}


void qnormal(float *q)
{
    /* Normalize a quaternion
     */
    float	s;

    s = 1 / sqrt((double) (q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]));
    q[0] *= s;
    q[1] *= s;
    q[2] *= s;
    q[3] *= s;
}


void qmult(const float *q1, const float *q2, float *dest)
{
    /* Multiply two quaternions.
     */
    static int	count = 0;
    float 	t1[3], t2[3], t3[3];
    float 	tf[4];
    
    vcopy(q1, t1); 
    vscale(t1, q2[3]);
    
    vcopy(q2, t2); 
    vscale(t2, q1[3]);
    
    vcross(q2, q1, t3);
    vadd(t1, t2, tf);
    vadd(t3, tf, tf);
    tf[3] = q1[3] * q2[3] - vdot(q1, q2);
    
    qcopy(tf, dest);
    
    if (++count >= 97) {
	count = 0;
	qnormal(dest);
    }
}


void qmatrix(const float *q, GLfloat *m)
{
    /* Build a rotation matrix, given a quaternion rotation.
     */
    m[0] = 1 - 2 * (q[1] * q[1] + q[2] * q[2]);
    m[1] = 2 * (q[0] * q[1] - q[2] * q[3]);
    m[2] = 2 * (q[2] * q[0] + q[1] * q[3]);
    m[3] = 0;
    
    m[4] = 2 * (q[0] * q[1] + q[2] * q[3]);
    m[5] = 1 - 2 * (q[2] * q[2] + q[0] * q[0]);
    m[6] = 2 * (q[1] * q[2] - q[0] * q[3]);
    m[7] = 0;
    
    m[8] = 2 * (q[2] * q[0] - q[1] * q[3]);
    m[9] = 2 * (q[1] * q[2] + q[0] * q[3]);
    m[10] = 1 - 2 * (q[1] * q[1] + q[0] * q[0]);
    m[11] = 0;
    
    m[12] = 0;
    m[13] = 0;
    m[14] = 0;
    m[15] = 1;
}
/*  Convert a 4x4 rotation matrix to a quaternion
	Code adapted from 
	http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion
*/
#define M_EPSILON 0.00000001f
void rotmatrix2q(float* m, float *q ){
	float trace = m[0] + m[5] + m[10] + 1.0f;
	if( trace > M_EPSILON ) {
		float s = 0.5f / sqrtf(trace);
		q[3] = 0.25f / s;   
		q[0] = ( m[9] - m[6] ) * s;
		q[1] = ( m[2] - m[8] ) * s;
		q[2] = ( m[4] - m[1] ) * s;
	} else {
		if ( m[0] > m[5] && m[0] > m[10] ) {
			float s = 2.0f * sqrtf( 1.0f + m[0] - m[5] - m[10]);
			q[0] = 0.25f * s;
			q[1] = (m[1] + m[4] ) / s;
			q[2] = (m[2] + m[8] ) / s;
			q[3] = (m[6] - m[9] ) / s; //???? minus?
		} else if (m[5] > m[10]) {
			float s = 2.0f * sqrtf( 1.0f + m[5] - m[0] - m[10]);
			q[0] = (m[1] + m[4] ) / s;
			q[1] = 0.25f * s;
			q[2] = (m[6] + m[9] ) / s;
			q[3] = (m[2] - m[8] ) / s;    
		} else {
			float s = 2.0f * sqrtf( 1.0f + m[10] - m[0] - m[5] );
			q[0] = (m[2] + m[8] ) / s;
			q[1] = (m[6] + m[9] ) / s;
			q[2] = 0.25f * s;
			q[3] = (m[1] - m[4] ) / s;//?? off by minus??
		}
	}
}


void	rvec2q(
	const float	rvec[3],
	float		radians,
	float		q[4]
) {

	double  rvec_normal[3];
	double  d;

	d = sqrt(rvec[0]*rvec[0] + rvec[1]*rvec[1] + rvec[2]*rvec[2]);

	if (d != 0.0) {
		rvec_normal[0] = rvec[0] / d;
		rvec_normal[1] = rvec[1] / d;
		rvec_normal[2] = rvec[2] / d;
	}
	else {
		rvec_normal[0] = 0.0;
		rvec_normal[1] = 0.0;
		rvec_normal[2] = 1.0;
	}

	q[0] = sin(radians/2.0) * rvec_normal[0];
	q[1] = sin(radians/2.0) * rvec_normal[1];
	q[2] = sin(radians/2.0) * rvec_normal[2];
	q[3] = cos(radians/2.0);
}


float ProjectToSphere(float r, float x, float y)
{
    /* Project an x,y pair onto a sphere of radius r or a hyperbolic sheet
     * if we are away from the center of the sphere.
     *
     * On sphere, 	x*x + y*y + z*z = r*r
     * On hyperbola, 	sqrt(x*x + y*y) * z = 1/2 r*r
     * Tangent at	z = r / sqrt(2)
     */
    float	dd, tt;
    
    dd = x*x + y*y;
    tt = r*r * 0.5;
    
    if (dd < tt)
	return sqrt((double) (r*r - dd));		/* Inside sphere */
    else
	return tt / sqrt((double) dd);		/* On hyperbola */
}


void CalcRotation(float *q, float newX, float newY,
		  float oldX, float oldY, float ballsize)
{
    /* Given old and new mouse positions (scaled to [-1, 1]),
     * Find the rotation quaternion q.
     */
    float	p1[3], p2[3];	/* 3D mouse points  */
    float	L;		/* sin^2(2 * phi)   */
   
    /* Check for zero rotation
     */
    if (newX == oldX  &&  newY == oldY) {
	qzero(q); 
	return;
    }
    
    /* Form two vectors based on input points, find rotation axis
     */
    vset(p1, newX, newY, ProjectToSphere(ballsize, newX, newY));
    vset(p2, oldX, oldY, ProjectToSphere(ballsize, oldX, oldY));
    
    vcross(p1, p2, q);		/* axis of rotation from p1 and p2 */
    
    L = vdot(q, q) / (vdot(p1, p1) * vdot(p2, p2));
    L = sqrt((double) (1 - L));
    
    vnormal(q);				/* q' = axis of rotation */
    vscale(q, sqrt((double) ((1 - L)/2)));	/* q' = q' * sin(phi) */
    q[3] = sqrt((double) ((1 + L)/2));		/* qs = qs * cos(phi) */
}


float ScalePoint(long pt, long origin, long size)
{
    /* Scales integer point to the range [-1, 1]
     */
    float	x;

    x = (float) (pt - origin) / (float) size;
    if (x < 0) x = 0;
    if (x > 1) x = 1;

    return 2 * x - 1;
}


#if 0
#define RLE_DUMP "/usr/local/Utah/bin/rawtorle -r -w %d -h %d -n 4 -o %s"

void DumpRLEImage(int ox, int oy, int sx, int sy, char *fname)
{
    /* Dump the current gl window into an rle file.
     */
    char	cmd[256];
    FILE	*pixout;

    static long			bsize = 0;
    static unsigned long	*wbuf = NULL;

    if (bsize != sx * sy * 4) {
	bsize = sx * sy * 4;
	wbuf = (unsigned long *) realloc(wbuf, bsize);
	MemCheck(wbuf);
    }

    glReadPixels(ox,  oy, ( ox + sx - 1)-(ox)+1, ( oy + sy - 1)-( oy)+1, GL_RGBA, GL_UNSIGNED_BYTE,  wbuf);

    (void) sprintf(cmd, RLE_DUMP, sx, sy, fname);
    pixout = popen(cmd, "w");
    Verify(pixout != NULL, "bad popen on window dump");

    Verify(fwrite(wbuf, bsize, 1, pixout) == 1, "short write on window dump");

    (void) pclose(pixout);
}


void DumpBinaryImage(int ox, int oy, int sx, int sy, char *fname)
{
    /* Dump a binary image in the form rgbrgbrgb...
     * However, lrectread returns data abgrabgrabgr...
     */
    int		i;
    FILE	*pixout;
    char	*pixel;

    static long			bsize = 0;
    static unsigned long	*wbuf = NULL;

    if (bsize != sx * sy * 4) {
		bsize = sx * sy * 4;
		wbuf = (unsigned long *) realloc(wbuf, bsize);
		MemCheck(wbuf);
    }

    glReadPixels(ox,  oy, ( ox + sx - 1)-(ox)+1, ( oy + sy - 1)-( oy)+1, GL_RGBA, GL_UNSIGNED_BYTE,  wbuf);

    pixout = fopen(fname, "wb");

    pixel = (char *) wbuf;
    for (i = 0; i < bsize; i += 4) {
		(void) putc(pixel[i+3], pixout);
		(void) putc(pixel[i+2], pixout);
		(void) putc(pixel[i+1], pixout);
    }

    (void) fclose(pixout);
}

#endif //0
void ViewMatrix(GLfloat *m)
{
    /* Return the total view matrix, including any
     * projection transformation.
     */
    
	GLfloat mp[16], mv[16];
	glGetFloatv(GL_PROJECTION_MATRIX, mp);
	glGetFloatv(GL_MODELVIEW_MATRIX, mv);
	mmult(mv, mp, m);
}


int ViewAxis(int *direction)
{
    /* Return the major axis the viewer is looking down.
     * 'direction' indicates which direction down the axis.
     */
    GLfloat	view[16];
    int		axis;
   
    ViewMatrix(view);
    
    /* The trick is to look down the z column for the largest value.
     * The total view matrix seems to be left hand coordinate
     */

    /*if (fabs((double) view[9]) > fabs((double) view[8]))*/
    if (fabs((double) view[6]) > fabs((double) view[2]))
	axis = 1;
    else
	axis = 0;
    /*if (fabs((double) view[10]) > fabs((double) view[axis+8]))*/
    if (fabs((double) view[10]) > fabs((double) view[2+axis*4]))
	axis = 2;
    
    if (direction)
	*direction = view[2+axis*4] > 0 ? -1 : 1;
	/**direction = view[axis+8] > 0 ? -1 : 1;*/

    return axis;
}


void StereoPerspective(int fovy, float aspect, float neardist, float fardist,
		       float converge, float eye)
{
    /* The first four arguements act like the perspective command
     * of the gl.  converge is the plane of the screen, and eye
     * is the eye distance from the centerline.
     *
     * Sample values: 320, ???, 0.1, 10.0, 3.0, 0.12
     */
    float	left, right, top, bottom;
    float	gltan;
    GLint	mm;
    glGetIntegerv(GL_MATRIX_MODE, &mm);

    glMatrixMode(GL_PROJECTION);
    
    gltan = tan((double) (fovy/2.0/10.0* (M_PI /180.0)) ) ;
    top = gltan * neardist;
    bottom = -top;
    
    gltan = tan((double) (fovy*aspect/2.0/10.0*M_PI/180.0));
    left = -gltan*neardist - eye/converge*neardist;
    right = gltan*neardist - eye/converge*neardist;
    
    glLoadIdentity();
    glFrustum(left,  right,  bottom,  top,  neardist,  fardist);
    glTranslatef(-eye,  0.0,  0.0);

    glMatrixMode(mm);
}
/* Make a translation matrix from a vector:
 */
void
makeTransMatrix(float *trans, float* mtrx){
	for (int i = 0; i<12; i++) mtrx[i] = 0.f;
	mtrx[0] = 1.f;
	mtrx[5] = 1.f;
	mtrx[10] = 1.f;
	mtrx[15] = 1.f;
	vcopy(trans, mtrx+12);
}

/*
 * make a modelview matrix from viewer position, direction, and up vector
 * Vectors must be nonzero
 * side-effect:  will alter input values if not valid.
 */
void
makeModelviewMatrix(float* vpos, float* vdir, float* upvec, float* mtrx){
	float vtemp[3];
	float left[3] = {-1.f, 0.f, 0.f};
	float ydir[3] = {0.f, 1.f, 0.f};
	float right[3];
	//Normalize the vectors:
	vnormal(upvec);
	vnormal(vdir);
	//Force the up vector to be orthogonal to viewDir
	vcopy(vdir, vtemp);
	vscale(vtemp, vdot(vdir, upvec));
	//Subtract the component of up in the viewdir direction
	vsub(upvec, vtemp, upvec);
	//Make sure it's still valid
	if (vdot(upvec,upvec) == 0.f) {
		//First try up = viewdir x left
		vcross(vdir, left, upvec);
		if (vdot (upvec, upvec) == 0.f) {
			//try viewdir x ydir
			vcross(vdir, ydir, upvec);
		}
	}
	vnormal(upvec);
	//calculate "right" vector:
	vcross(vdir, upvec, right);
	//Construct matrix:
	GLfloat minv[16];
	//Fill in bottom row:
	minv[3] = 0.f;
	minv[7] = 0.f;
	minv[11] = 0.f;
	minv[15] = 1.f;
	//copy in first 3 elements of columns
	vcopy(right, minv);
	vcopy(upvec, minv+4);
	//third col is neg of viewdir
	
	vcopy(vdir, minv + 8);
	vscale(minv+8, -1.f);
	vcopy(vpos, minv+ 12);
	minvert(minv, mtrx);
}
void	matrix4x4_vec3_mult(
	const GLfloat	m[16],
	const GLfloat a[4],
	GLfloat b[4]
) {
	b[0] = m[0]*a[0] + m[4]*a[1] + m[8]*a[2] + m[12]*a[3];
	b[1] = m[1]*a[0] + m[5]*a[1] + m[9]*a[2] + m[13]*a[3];
	b[2] = m[2]*a[0] + m[6]*a[1] + m[10]*a[2] + m[14]*a[3];
	b[3] = m[3]*a[0] + m[7]*a[1] + m[11]*a[2] + m[15]*a[3];

	if (b[3] != 0.0 && b[3] != 1.0) {
		b[0] /= b[3];
		b[1] /= b[3];
		b[2] /= b[3];
		b[3] = 1.0;
	}
}

//
// Matrix Inversion
// Adapted from Richard Carling
// from "Graphics Gems", Academic Press, 1990
//

#define SMALL_NUMBER	1.e-8
/* 
 *   matrix4x4_inverse( original_matrix, inverse_matrix )
 * 
 *    calculate the inverse of a 4x4 matrix
 *
 *     -1     
 *     A  = ___1__ adjoint A
 *         det A
 */

int	matrix4x4_inverse(
	const GLfloat	*in,
	GLfloat *out
) {
    int i, j;
    double det;
	double det4x4(const GLfloat	m[16]);
	void adjoint(const GLfloat	*in, GLfloat *out);

    /* calculate the adjoint matrix */

    adjoint( in, out );

    /*  calculate the 4x4 determinant
     *  if the determinant is zero, 
     *  then the inverse matrix is not unique.
     */

    det = det4x4(in);

    if ( fabs( det ) < SMALL_NUMBER) {
        //	Singular matrix, no inverse!
        return(-1);
    }

    /* scale the adjoint matrix to get the inverse */

    for (i=0; i<4; i++) {
	for(j=0; j<4; j++) {
	    out[i*4+j] = out[i*4+j] / det;
	}
	}
	return(0);
}


/* 
 *   adjoint( original_matrix, inverse_matrix )
 * 
 *     calculate the adjoint of a 4x4 matrix
 *
 *      Let  a   denote the minor determinant of matrix A obtained by
 *           ij
 *
 *      deleting the ith row and jth column from A.
 *
 *                    i+j
 *     Let  b   = (-1)    a
 *          ij            ji
 *
 *    The matrix B = (b  ) is the adjoint of A
 *                     ij
 */

void	adjoint(
	const GLfloat	*in,
	GLfloat *out
) {
    double a1, a2, a3, a4, b1, b2, b3, b4;
    double c1, c2, c3, c4, d1, d2, d3, d4;
	double det3x3(
		double a1, double a2, double a3,
		double b1, double b2, double b3,
		double c1, double c2, double c3
	);

    /* assign to individual variable names to aid  */
    /* selecting correct values  */

	a1 = in[0*4+0]; b1 = in[0*4+1]; 
	c1 = in[0*4+2]; d1 = in[0*4+3];

	a2 = in[1*4+0]; b2 = in[1*4+1]; 
	c2 = in[1*4+2]; d2 = in[1*4+3];

	a3 = in[2*4+0]; b3 = in[2*4+1];
	c3 = in[2*4+2]; d3 = in[2*4+3];

	a4 = in[3*4+0]; b4 = in[3*4+1]; 
	c4 = in[3*4+2]; d4 = in[3*4+3];


    /* row column labeling reversed since we transpose rows & columns */

    out[0*4+0]  =   det3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4);
    out[1*4+0]  = - det3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4);
    out[2*4+0]  =   det3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4);
    out[3*4+0]  = - det3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4);
        
    out[0*4+1]  = - det3x3( b1, b3, b4, c1, c3, c4, d1, d3, d4);
    out[1*4+1]  =   det3x3( a1, a3, a4, c1, c3, c4, d1, d3, d4);
    out[2*4+1]  = - det3x3( a1, a3, a4, b1, b3, b4, d1, d3, d4);
    out[3*4+1]  =   det3x3( a1, a3, a4, b1, b3, b4, c1, c3, c4);
        
    out[0*4+2]  =   det3x3( b1, b2, b4, c1, c2, c4, d1, d2, d4);
    out[1*4+2]  = - det3x3( a1, a2, a4, c1, c2, c4, d1, d2, d4);
    out[2*4+2]  =   det3x3( a1, a2, a4, b1, b2, b4, d1, d2, d4);
    out[3*4+2]  = - det3x3( a1, a2, a4, b1, b2, b4, c1, c2, c4);
        
    out[0*4+3]  = - det3x3( b1, b2, b3, c1, c2, c3, d1, d2, d3);
    out[1*4+3]  =   det3x3( a1, a2, a3, c1, c2, c3, d1, d2, d3);
    out[2*4+3]  = - det3x3( a1, a2, a3, b1, b2, b3, d1, d2, d3);
    out[3*4+3]  =   det3x3( a1, a2, a3, b1, b2, b3, c1, c2, c3);
}
/*
 * double = det4x4( matrix )
 * 
 * calculate the determinant of a 4x4 matrix.
 */
double det4x4(
	const GLfloat	m[16]
) {
    double ans;
    double a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4;

	double det3x3(
		double a1, double a2, double a3,
		double b1, double b2, double b3,
		double c1, double c2, double c3
	);

    /* assign to individual variable names to aid selecting */
	/*  correct elements */

	a1 = m[0*4+0]; b1 = m[0*4+1]; 
	c1 = m[0*4+2]; d1 = m[0*4+3];

	a2 = m[1*4+0]; b2 = m[1*4+1]; 
	c2 = m[1*4+2]; d2 = m[1*4+3];

	a3 = m[2*4+0]; b3 = m[2*4+1]; 
	c3 = m[2*4+2]; d3 = m[2*4+3];

	a4 = m[3*4+0]; b4 = m[3*4+1]; 
	c4 = m[3*4+2]; d4 = m[3*4+3];

    ans = a1 * det3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4)
        - b1 * det3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4)
        + c1 * det3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4)
        - d1 * det3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4);
    return ans;
}


/*
 * double = det3x3(  a1, a2, a3, b1, b2, b3, c1, c2, c3 )
 * 
 * calculate the determinant of a 3x3 matrix
 * in the form
 *
 *     | a1,  b1,  c1 |
 *     | a2,  b2,  c2 |
 *     | a3,  b3,  c3 |
 */

double det3x3(
	double a1, double a2, double a3,
	double b1, double b2, double b3,
	double c1, double c2, double c3
) {
    double ans;
	double det2x2(double a, double b, double c, double d);

    ans = a1 * det2x2( b2, b3, c2, c3 )
        - b1 * det2x2( a2, a3, c2, c3 )
        + c1 * det2x2( a2, a3, b2, b3 );
    return ans;
}

/*
 * double = det2x2( double a, double b, double c, double d )
 * 
 * calculate the determinant of a 2x2 matrix.
 */

double det2x2(double a, double b, double c, double d)
{
    double ans;
    ans = a * d - b * c;
    return ans;
}

int printOglError(char *file, int line)     
{
  //
  // Returns 1 if an OpenGL error occurred, 0 otherwise.
  //
  GLenum glErr;
  int    retCode = 0;

  glErr = glGetError();

  while (glErr != GL_NO_ERROR)
  {
    std::cerr << "glError: " << gluErrorString(glErr) << std::endl;
    std::cerr << "         " << file << ":" << line << std::endl << std::endl;

    retCode = 1;

    glErr = glGetError();
  }

  return retCode;
}

#define DEAD
#ifdef	DEAD

// Macros
#define LINEAR_INDEX(dim, x, y, z) ((x) + (dim[0]) * ((y) + (z) * (dim[1])))
#define sqr(x) ((x)*(x))

void computeGradientData(int dim[3], int numChan,
			 unsigned char *volume, 
			 unsigned char *gradient)
{
  
  for(int z = 0; z < dim[2]; z++)
  for(int y = 0; y < dim[1]; y++)
  for(int x = 0; x < dim[0]; x++) {
    
    float gradient_temp[3];

    // The following code computes the gradient for the volume data. 
    // For volumes with more than one channel, only the first channel
    // is used to compute the gradient! The voxel index computation is
    // very inefficient and can be optimized a lot using a simple 
    // incremental computation.
    
    // Handle border cases correctly by using forward and backward 
    // differencing for the boundaries.
    if (x == 0)
      // forward differencing
      gradient_temp[0] = ((float) volume[numChan * LINEAR_INDEX(dim, x+1, y, z)] - 
			  (float) volume[numChan * LINEAR_INDEX(dim, x, y, z)]);
    else if (x == dim[0]-1) 
      // backward differencing
      gradient_temp[0] = ((float) volume[numChan * LINEAR_INDEX(dim,x,y,z)] - 
			  (float) volume[numChan * LINEAR_INDEX(dim, x-1,y,z)]);
    else
      // central differencing
      gradient_temp[0] = (((float) volume[numChan * LINEAR_INDEX(dim,x+1,y,z)] - 
			   (float) volume[numChan * LINEAR_INDEX(dim, x-1,y,z)] )/2.0);
    
    if (y == 0)
      gradient_temp[1] = ((float) volume[numChan * LINEAR_INDEX(dim,x,y+1,z)] - 
			  (float) volume[numChan * LINEAR_INDEX(dim, x, y, z)]);
    else if (y == dim[1]-1) 
      gradient_temp[1] = ((float) volume[numChan * LINEAR_INDEX(dim,x,y,z)] - 
			  (float) volume[numChan * LINEAR_INDEX(dim, x,y-1,z)]);
    else
      gradient_temp[1] = (((float) volume[numChan * LINEAR_INDEX(dim,x,y+1,z)] - 
			   (float) volume[numChan * LINEAR_INDEX(dim, x,y-1,z)])/2.0);
    
    if (z == 0)
      gradient_temp[2] = ((float) volume[numChan * LINEAR_INDEX(dim,x,y,z+1)] - 
			  (float) volume[numChan * LINEAR_INDEX(dim, x, y, z)]);
    else if (z == dim[2]-1) 
      gradient_temp[2] = ((float) volume[numChan * LINEAR_INDEX(dim,x,y,z)] - 
			  (float) volume[numChan * LINEAR_INDEX(dim, x,y,z-1)]);
    else
      gradient_temp[2] = (((float) volume[numChan * LINEAR_INDEX(dim,x,y,z+1)] - 
			   (float) volume[numChan * LINEAR_INDEX(dim, x,y,z-1)])/2.0);
    
    // compute the magintude for the gradient
    double mag = (sqrt(sqr(gradient_temp[0]) + 
		       sqr(gradient_temp[1]) + 
		       sqr(gradient_temp[2])));
    
    // avoid any divide by zeros!
    if (mag > 0.01) {
      gradient_temp[0] /= mag;
      gradient_temp[1] /= mag;
      gradient_temp[2] /= mag;
    }
    else {
      gradient_temp[0] = gradient_temp[1] = gradient_temp[2] = 0.0; 
    }

    // Map the floating point gradient values to the appropriate range
    // in unsigned byte
    unsigned long index = 4 * (LINEAR_INDEX(dim, x, y, z));
    for (int i=0; i < 3; i++) {
      gradient[index + i] = (signed char) floor (gradient_temp[i]*128.0);
    }   
    
    // Set the alpha to be 1.0
    gradient[index + 3] = 255;
  }
}

#endif
};
