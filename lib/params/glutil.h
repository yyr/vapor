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
#include <QGLWidget>
#include <math.h>
#include <vapor/common.h>

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

//#define vset(a,x,y,z)	(a[0] = x, a[1] = y, a[2] = z)
//#define Verify(expr,estr)	if (!(expr)) BailOut(estr,__FILE__,__LINE__)

#define CallocType(type,i)	(type *) calloc(i,sizeof(type))


#define	YMAXSTEREO	491
#define	YOFFSET		532
#ifndef	M_PI
#define M_PI 3.14159265358979323846
#endif
/*extern GLfloat *idmatrix;*/
namespace VAPoR {
	

/* glutil.c */

PARAMS_API bool powerOf2(uint n);
PARAMS_API uint nextPowerOf2(uint n);

PARAMS_API void	computeGradientData(
	int dim[3], int numChan, unsigned char *volume, unsigned char *gradient
);
PARAMS_API void	makeModelviewMatrix(float* vpos, float* vdir, float* upvec, float* matrix);
PARAMS_API void	makeTransMatrix(float* transVec, float* matrix);
PARAMS_API void	vscale (float *v, float s);
PARAMS_API void	vscale (double *v, double s);
PARAMS_API void	vmult(const float *v, float s, float *w); 
PARAMS_API void	vhalf (const float *v1, const float *v2, float *half);
PARAMS_API void	vcross (const float *v1, const float *v2, float *cross);
PARAMS_API void	vreflect (const float *in, const float *mirror, float *out);
PARAMS_API void	vtransform (const float *v, GLfloat *mat, float *vt);
PARAMS_API void	vtransform (const float *v, GLfloat *mat, double *vt);
PARAMS_API void	vtransform (const double *v, GLfloat *mat, double *vt);
PARAMS_API void	vtransform4 (const float *v, GLfloat *mat, float *vt);
PARAMS_API void	vtransform3 (const float *v, float *mat, float *vt);
PARAMS_API void	vtransform3t (const float *v, float *mat, float *vt);
PARAMS_API bool	pointOnRight(float* pt1, float* pt2, float* testPt);
PARAMS_API void	mcopy (GLfloat *m1, GLfloat *m2);
PARAMS_API void	mmult (GLfloat *m1, GLfloat *m2, GLfloat *prod);
PARAMS_API int	minvert (GLfloat *mat, GLfloat *result);

//Some routines to handle 3x3 rotation matrices, represented as 9 floats, 
//where the column index increments faster (like in 4x4 case
PARAMS_API void	mmult33(const float* m1, const float* m2, float* result);

//Same as above, but use the transpose (i.e. inverse for rotations) on the left
PARAMS_API void	mmultt33(const float* m1Trans, const float* m2, float* result);

//Determine a rotation matrix from (theta, phi, psi) (radians), that is, 
//find the rotation matrix that first rotates in (x,y) by psi, then takes the vector (0,0,1) 
//to the vector with direction (theta,phi) by rotating by phi in the (x,z) plane and then
//rotating in the (x,y)plane by theta.
PARAMS_API void	getRotationMatrix(float theta, float phi, float psi, float* matrix);

//Determine a rotation matrix about an axis:
PARAMS_API void getAxisRotation(int axis, float rotation, float* matrix);

//Determine the psi, phi, theta from a rotation matrix:
PARAMS_API void getRotAngles(float* theta, float* phi, float* psi, const float* matrix);
PARAMS_API int rayBoxIntersect(const float rayStart[3], const float rayDir[3],const float boxExts[6], float results[2]);

PARAMS_API void	qnormal (float *q);
PARAMS_API void qinv(const float q1[4], float q2[4]);
PARAMS_API void	qmult (const float *q1, const float *q2, float *dest);
PARAMS_API void	qmatrix (const float *q, GLfloat *m);
PARAMS_API float	ProjectToSphere (float r, float x, float y);
PARAMS_API void	CalcRotation (float *q, float newX, float newY, float oldX, float oldY, float ballsize);
PARAMS_API float	ScalePoint (long pt, long origin, long size);
PARAMS_API void	rvec2q(const float	rvec[3],float		radians,float		q[4]);
PARAMS_API void	rotmatrix2q(float* m, float *q );
PARAMS_API float   getScale(GLfloat* rotmatrix);
PARAMS_API void view2Quat(float vdir[3], float upvec[3], float q[4]);
PARAMS_API void imagQuat2View(const float startquat[4], const float q[3], float vdir[3],float upvec[3]);
PARAMS_API void view2ImagQuat(const float startquat[4], float vdir[3],float upvec[3], float q[3]);
PARAMS_API void calcStartQuat(const float quata[4],const float quatb[4], float startQuat[4]);

inline void vset(float* a, const float x, const float y, const float z){a[0] = x, a[1] = y, a[2] = z;}
inline float vdot(const float* a, const float* b)
	{return (a[0]*b[0]+a[1]*b[1]+a[2]*b[2]);}
inline double vdot(const double* a, const double* b)
	{return (a[0]*b[0]+a[1]*b[1]+a[2]*b[2]);}
inline float vlength(const float*a) {return sqrt(vdot(a,a));}
inline double vlength(const double*a) {return sqrt(vdot(a,a));}
inline float vdist(const float* a, const float*b) {
	return (sqrt((a[0]-b[0])*(a[0]-b[0])+(a[1]-b[1])*(a[1]-b[1])+(a[2]-b[2])*(a[2]-b[2]))); }
inline void vnormal(float *a) {vscale(a, 1/vlength(a));}
inline void vnormal(double *a) {vscale(a, 1/vlength(a));}
inline void vcopy(const float* a, float* b) {b[0] = a[0], b[1] = a[1], b[2] = a[2];}
inline void vcopy(const double* a, double* b) {b[0] = a[0], b[1] = a[1], b[2] = a[2];}
inline void vsub(const float* a, const float* b, float* c)
	{c[0] = a[0]-b[0], c[1] = a[1]-b[1], c[2] = a[2]-b[2];}
inline void vsub(const double* a, const double* b, double* c)
	{c[0] = a[0]-b[0], c[1] = a[1]-b[1], c[2] = a[2]-b[2];}
inline void vadd(const float* a, const float* b, float* c)
	{c[0] = a[0]+b[0], c[1] = a[1]+b[1], c[2] = a[2]+b[2];}
inline void vadd(const double* a, const double* b, double* c)
	{c[0] = a[0]+b[0], c[1] = a[1]+b[1], c[2] = a[2]+b[2];}
inline void vzero(float *a) {a[0] = a[1] = a[2] = 0.f;}
inline void qset(float* a,  float x,  float y,  float z,  float w)
	{a[0] = x, a[1] = y, a[2] = z, a[3] = w;}
inline void qcopy(const float*a, float* b)	
	{b[0] = a[0], b[1] = a[1], b[2] = a[2], b[3] = a[3];}
inline void qzero(float* a)	{a[0] = a[1] = a[2] = 0, a[3] = 1;}
inline void qadd(const float* a,const float* b,float* c)	
	{vadd(a,b,c), c[3]=a[3]+b[3];}
inline float qlength(const float q[4]){
	return sqrt(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3]);
}

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

PARAMS_API int printOglError(const char *file, int line, const char *msg = NULL);

#define printOpenGLError() printOglError(__FILE__, __LINE__)
#define printOpenGLErrorMsg(msg) printOglError(__FILE__, __LINE__, msg)

class PARAMS_API Point4{
public:
	Point4() {point[0]=point[1]=point[2]=point[3]=0.f;}
	Point4(const float data[4]){setVal(data);}
	void setVal(const float sourceData[4]){
		for (int i = 0; i< 4; i++) point[i] = sourceData[i];
	}
	void set3Val(const float sourceData[3]){
		for (int i = 0; i< 3; i++) point[i] = sourceData[i];
	}
	void set1Val(int index, float sourceData){
		point[index] = sourceData;
	}
	float getVal(int i){return point[i];}
	void copy3Vals(float* dest) {dest[0]=point[0];dest[1]=point[1];dest[2]=point[2];}
protected:
	float point[4];
};
};
#endif	// _glutil_h_
